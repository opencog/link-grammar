/***************************************************************************/
/* Copyright (c) 2016 Amir Plivatsky                                       */
/* lg_isatty() is based on code sent to the Cygwin discussion group by     */
/* Corinna Vinschen, Cygwin Project Co-Leader, on 2012.                    */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software.      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

/* Used for terminal resizing */
#ifndef _WIN32
#endif /* _WIN32 */

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#include <io.h>
#include <shellapi.h>                   // CommandLineToArgvW()
#include <errhandlingapi.h>             // GetLastError()
#else
#include <pwd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#endif /* _WIN32 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser-utilities.h"

/**
 * Expand an initial '~' to home directory.
 *
 * @param filename The filename to expand. Its length must be >= 1.
 * @return A newly-allocated filename, expanded if possible. Freed by caller.
 *
 * Note: ~user is not supported here on Windows.
 */
char *expand_homedir(const char *filename)
{
	if (filename[0] != '~') return strdup(filename);

#ifndef _WIN32
	char *user = NULL;
	const char *user_end = &filename[strcspn(filename, "/")];
	if (user_end != &filename[1])
	{
		user = strdup(filename + 1);
		user[user_end - filename - 1] = '\0';
	}
#endif /* _WIN32 */

#ifdef _WIN32
	char *home;

	const char *homepath = getenv("HOMEPATH");
	if ((homepath == NULL) || (homepath[0] == '\0')) return strdup(filename);
	const char *homedrive = getenv("HOMEDRIVE");
	if (homedrive == NULL) homedrive = "";

	home = malloc(strlen(homepath) + strlen(homedrive) + 1);
	strcpy(home, homedrive);
	strcat(home, homepath);
	filename++;
#else
	const char *home;

	if (user == NULL)
	{
		home = getenv("HOME");
		if ((home == NULL) || (home[0] == '\0')) return strdup(filename);
		filename++;
	}
	else
	{
		struct passwd *pwd;
		pwd = getpwnam(user);
		free(user);
		if (pwd == NULL) return strdup(filename);
		home = pwd->pw_dir;
		filename = user_end;
	}
#endif

	size_t filename_len = strlen(filename);
	size_t home_len = strlen(home);

	char *eh_filename = malloc(home_len + filename_len + 1);
	memcpy(eh_filename, home, home_len);
	memcpy(eh_filename + home_len, filename, filename_len + 1);

#ifdef _WIN32
	free(home);
#endif

	return eh_filename;
}

#ifdef _WIN32
#define INPUT_UTF16_SIZE 4096
/**
 * Get a line from the console in UTF-8.
 * This function bypasses the code page conversion and reads Unicode
 * directly from the console.
 * @param inbuf Buffer for the returned line (in UTF-8 encoding).
 * @param inbuf_size The size of this buffer.
 * @return -1: error; 0: EOF; 1: Success.
 */
int get_console_line(char *inbuf, int inbuf_size)
{
	static HANDLE console_handle = NULL;
	wchar_t winbuf[INPUT_UTF16_SIZE];
	static bool eof;

	if (eof) return 0;

	if (NULL == console_handle)
	{
		console_handle = CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ,
		                             NULL, OPEN_EXISTING, 0, NULL);
		if (!console_handle || (INVALID_HANDLE_VALUE == console_handle))
		{
			snprintf(inbuf, inbuf_size, "CreateFileA CONIN$: Error %d", GetLastError());
			return -1;
		}
	}

	DWORD nchar;
	if (!ReadConsoleW(console_handle, &winbuf, INPUT_UTF16_SIZE-1, &nchar, NULL))
	{
		snprintf(inbuf, inbuf_size, "ReadConsoleW: Error %d\n", GetLastError());
		return -1;
	}
	winbuf[nchar] = L'\0'; /* nchar is always <= INPUT_UTF16_SIZE-1.  */

	nchar = WideCharToMultiByte(CP_UTF8, 0, winbuf, -1, inbuf,
	                            inbuf_size, NULL, NULL);
	if (0 == nchar)
	{
		DWORD err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER)
			snprintf(inbuf, inbuf_size, "Input line too long (>%d)", inbuf_size-3);
		else
			snprintf(inbuf, inbuf_size, "WideCharToMultiByte CP_UTF8: Error %d", err);
		return -1;
	}

	/* Make sure we don't have conversion problems, by searching for U+FFFD. */
	const char *invalid_char  = strstr(inbuf, "\xEF\xBF\xBD");
	if (NULL != invalid_char)
	{
		prt_error("Error: Unable to process UTF8 in input string.\n");
		inbuf[0] = '\0'; /* Just ignore the input line. */
	}

	/* ^Z is read as a character. Convert it to an EOF indication. */
	char *ctrl_z_position = strchr(inbuf, '\x1A');
	if (ctrl_z_position != NULL)
	{
		if (ctrl_z_position == inbuf) return 0;
		*ctrl_z_position = '\n';
		eof = true;
		return 1;
	}

	return 1;
}


static int console_input_cp;
static int console_output_cp;
static void restore_console_cp(void)
{
	SetConsoleCP(console_input_cp);
	SetConsoleOutputCP(console_output_cp);
}

static BOOL CtrlHandler(DWORD fdwCtrlType)
{
	if ((CTRL_C_EVENT == fdwCtrlType) || (CTRL_BREAK_EVENT  == fdwCtrlType))
	{
		fprintf(stderr, "Interrupt\n");
		restore_console_cp();
		exit(2);
	}
	return FALSE;
}

/**
 * Set the output conversion attributes for transparency.
 * This way UTF-8 output doesn't pass any conversion.
 */
static void win32_set_utf8_output(void)
{
	if (-1 == _setmode(fileno(stdout), _O_BINARY))
	{
		prt_error("Warning: _setmode(fileno(stdout), _O_BINARY): %s.\n",
			strerror(errno));
	}

	console_input_cp = GetConsoleCP();
	console_output_cp = GetConsoleOutputCP();
	atexit(restore_console_cp);
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		prt_error("Warning: Cannot not set code page restore handler.\n");
	}
	/* For file output. It is too late for output pipes.
	 * If output pipe is desired, one can set CP_UTF8 by the
	 * command "chcp 65001" before invoking link-parser. */
	if (!SetConsoleCP(CP_UTF8))
	{
		prt_error("Warning: Cannot set input codepage %d (error %d).\n",
			CP_UTF8, (int)GetLastError());
	}
	/* For Console output. */
	if (!SetConsoleOutputCP(CP_UTF8))
	{
		prt_error("Warning: Cannot set output codepage %d (error %d).\n",
			CP_UTF8, (int)GetLastError());
	}
}

#include <winternl.h>
/**
 * isatty() compatibility for running under Cygwin when compiling
 * using the Windows native C library.
 * WIN32 isatty() gives a wrong result (for Cygwin) on Cygwin's
 * pseudo-ttys, because they are implemented as Windows pipes.
 * Here is compatibility layer, originally sent to the Cygwin
 * discussion group by Corinna Vinschen, Cygwin Project Co-Leader.
 * See: https://www.cygwin.com/ml/cygwin/2012-11/msg00214.html
 */
int lg_isatty(int fd)
{
	HANDLE fh;
	long buf[66];  /* NAME_MAX + 1 + sizeof ULONG */
	PFILE_NAME_INFO pfni = (PFILE_NAME_INFO)buf;
	PWCHAR cp;

	/* Fetch the underlying HANDLE. */
	fh = (HANDLE)_get_osfhandle(fd);
	if (!fh || (INVALID_HANDLE_VALUE == fh))
	{
		errno = EBADF;
		return 0;
	}

/* Windows _isatty() is buggy: It returns a nonzero for the NUL device! */
#if 0
	if (_isatty(fd))
		return 1;
#else
	/* This detects a console device reliably. */
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	DWORD mode;
	if (GetConsoleMode(fh, &mode) || GetConsoleScreenBufferInfo(fh, &sbi))
		return 1;
#endif

	/* Must be a pipe. */
	if (GetFileType(fh) != FILE_TYPE_PIPE)
		goto no_tty;

	if (!GetFileInformationByHandleEx(fh, FileNameInfo, pfni, sizeof(buf)))
	{
		printf("GetFileInformationByHandleEx: Error %d\n", (int)GetLastError());
		goto no_tty;
	}

	/* The filename is not guaranteed to be NUL-terminated. */
	pfni->FileName[pfni->FileNameLength / sizeof (WCHAR)] = L'\0';

	/* Now check the name pattern.  The filename of a Cygwin pseudo tty pipe
	   looks like this:

			 \{cygwin,msys}-%16llx-pty%d-{to,from}-master

		%16llx is the hash of the Cygwin installation, (to support multiple
		parallel installations), %d id the pseudo tty number, "to" or "from"
		differs the pipe direction. "from" is a stdin, "to" a stdout-like
		pipe. */
	cp = pfni->FileName;
	if ((!wcsncmp(cp, L"\\cygwin-", 8) && !wcsncmp(cp + 24, L"-pty", 4)) ||
	    (!wcsncmp(cp, L"\\msys-", 6)   && !wcsncmp(cp + 22, L"-pty", 4)))
	{
		cp = wcschr(cp + 26, '-');
		if (!cp)
			goto no_tty;
		if (!wcscmp(cp, L"-from-master") || !wcscmp(cp, L"-to-master"))
			return 1;
	}
no_tty:
	errno = EINVAL;
	return 0;
}

/*
 * Clean up Windows argv initialization.
 * Needed for MSVC and MinGW.
 */
static char **utf8_argv;
static int utf8_argc;
static void argv2utf8_free(void)
{
	for (int i = 0; i < utf8_argc; i++)
		free(utf8_argv[i]);
	free(utf8_argv);
}

/**
 * Convert argv from the startup locale to UTF-8.
 */
static char **argv2utf8(int argc)
{
	char **nargv = malloc(argc * sizeof(char *));
	LPWSTR *warglist = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (NULL == warglist) return NULL;

	for (int i = 0; i < argc; i++)
	{
		int n = WideCharToMultiByte(CP_UTF8, 0, warglist[i], -1, NULL, 0, NULL, NULL);

		nargv[i] = malloc(n);
		n = WideCharToMultiByte(CP_UTF8, 0, warglist[i], -1, nargv[i], n, NULL, NULL);
		if (0 == n)
		{
			prt_error("Error: WideCharToMultiByte CP_UTF8 failed: Error %d.\n",
			         (int)GetLastError());
			return NULL;
		}
	}
	LocalFree(warglist);


	utf8_argv = nargv;
	utf8_argc = argc;
	atexit(argv2utf8_free);

	return nargv;
}

static bool running_under_cygwin;

char **ms_windows_setup(int argc)
{
	/* If compiled with MSVC/MinGW, we still support running under Cygwin.
	 * This is done by checking running_under_cygwin to resolve
	 * incompatibilities. */
	const char *ostype = getenv("OSTYPE");
	if ((NULL != ostype) && (0 == strcmp(ostype, "cygwin")))
		running_under_cygwin = true;

	/* argv encoding is in the current locale. */
	char **argv = argv2utf8(argc);
	if (NULL == argv)
	{
		prt_error("Fatal error: Unable to parse command line\n");
		exit(-1);
	}

	win32_set_utf8_output();

	return argv;
}

char *get_utf8_line(char *input_string, int max_input_line, FILE *in)
{
	char *pline;

	if (!running_under_cygwin)
		pline = get_console_line();
	else
		pline = fgets(input_string, max_input_line, in);

	return pline;
}
#else
char *get_utf8_line(char *input_string, int max_input_line, FILE *in)
{
	return fgets(input_string, max_input_line, in);
}
#endif /* _WIN32 */

static unsigned int screen_width = INITIAL_SCREEN_WIDTH;
/**
 * Query the system for the current tty/console width.
 * May be called directly or as SIGWINCH handler (if SIGWINCH exists).
 * Put the found screen width in a global variable \c screen_width;
 * @param sig Used only for type check in case SIGWINCH exists.
 * @return Void.
 */
static void get_screen_width(int sig)
{
	static int isatty_stdout = -1;

	if (isatty_stdout == -1) isatty_stdout = isatty(fileno(stdout));
	if (!isatty_stdout) return;

	int fd = fileno(stdout);

#ifdef _WIN32
	HANDLE console;
	CONSOLE_SCREEN_BUFFER_INFO info;

	/* Create a handle to the console screen. */
	console = (HANDLE)_get_osfhandle(fd);
	if (!console || (console == INVALID_HANDLE_VALUE)) return;

	/* Calculate the size of the console window. */
	if (GetConsoleScreenBufferInfo(console, &info) == 0) return;

	screen_width = info.dwSize.X;
	return;
#else
	struct winsize ws;

	/* If there is no controlling terminal, the fileno will fail. This
	 * seems to happen while building docker images, I don't know why.
	 */
	if (fd < 0) return;

	if (0 != ioctl(fd, TIOCGWINSZ, &ws))
	{
		perror("stdout: ioctl TIOCGWINSZ");
		return;
	}

	/* Set the screen width only if the returned value seems
	 * rational: it's positive and not insanely tiny.
	 */
	if ((10 < ws.ws_col) && (16123 > ws.ws_col))
		screen_width = ws.ws_col;
#endif /* _WIN32 */
}

/**
 * Set the "width" user variable from the global \c screen_size.
 * If SIGWINCH is not defined, poll the screen width beforehand.
 */
void set_screen_width(Command_Options* copts)
{
#if !defined SIGWINCH || defined _WIN32
	get_screen_width(0);
#endif

	copts->screen_width = screen_width;
}

#ifdef INTERRUPT_EXIT
static void interrupt_exit(int n)
{
	exit(128+n);
}
#endif

void initialize_screen_width(Command_Options *copts)
{
#ifdef SIGWINCH
#if HAVE_SIGACTION
	struct sigaction winch_act = { { 0 } };
	winch_act.sa_handler = get_screen_width;
	sigemptyset(&winch_act.sa_mask);
	winch_act.sa_flags = 0;
	if (sigaction(SIGWINCH, &winch_act, NULL) == -1)
		perror("sigaction SIGWINCH");
#else
	if (signal(SIGWINCH, get_screen_width) == SIG_ERR)
		perror("signal SIGWINCH");
#endif /* HAVE_SIGACTION */
#endif /* SIGWINCH */

#ifdef INTERRUPT_EXIT
	(void)signal(SIGINT, interrupt_exit);
	(void)signal(SIGTERM, interrupt_exit);
#endif

	get_screen_width(0);
	set_screen_width(copts);
}

#ifdef __MINGW32__
/*
 * A workaround for printing UTF-8 on the console.
 * These functions are also implemented in the LG library.
 * For the strange story see the comment in utilities.c there.
 */

int __mingw_vfprintf (FILE * __restrict__ stream, const char * __restrict__ fmt, va_list vl)
{
	int n = vsnprintf(NULL, 0, fmt, vl);
	if (0 > n) return n;
	char *buf = malloc(n+1);
	n = vsnprintf(buf, n+1, fmt, vl);
	if (0 > n)
	{
		free(buf);
		return n;
	}

	n = fputs(buf, stdout);
	free(buf);
	return n;
}

int __mingw_vprintf (const char * __restrict__ fmt, va_list vl)
{
	return __mingw_vfprintf(stdout, fmt, vl);
}
#endif /* __MINGW32__ */
