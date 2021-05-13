/*************************************************************************/
/* Copyright (c) 2014 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <stdlib.h>

#ifdef USE_WORDGRAPH_DISPLAY
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#ifdef HAVE_FORK
#include <unistd.h>    /* fork() and execl() */
#include <sys/wait.h>  /* waitpid() */
#endif
#ifdef HAVE_PRCTL
#include <sys/prctl.h> /* prctl() */
#endif
#include <signal.h>    /* SIG* */

#include "print/print-util.h" /* for append_string */
#include "utilities.h" /* for dyn_str functions and UNREACHABLE */
#endif /* USE_WORDGRAPH_DISPLAY */

#include "api-structures.h"
#include "error.h"
#include "string-set.h"
#include "tok-structures.h"
#include "wordgraph.h"

#ifdef __APPLE__
#define POPEN_DOT
#endif /* __APPLE__ */

#if USE_WORDGRAPH_DISPLAY || defined(DEBUG)
GNUC_UNUSED const char *gword_morpheme(Sentence sent, const Gword *w)
{
	const char *mt;
	char buff[64];

	switch (w->morpheme_type)
	{
		case MT_INVALID:
			mt = "MT_INVALID";
			break;
		case MT_WORD:
			mt = "MT_WORD";
			break;
		case MT_FEATURE:
			mt = "MT_FEATURE";
			break;
		case MT_INFRASTRUCTURE:
			mt = "MT_I-S";
			break;
		case MT_WALL:
			mt = "MT_WALL";
			break;
		case MT_EMPTY:
			mt = "MT_EMPTY";
			break;
		case MT_UNKNOWN:
			mt = "MT_UNKNOWN";
			break;
		case MT_TEMPLATE:
			mt = "MT_TEMPLATE";
			break;
		case MT_ROOT:
			mt = "MT_ROOT";
			break;
		case MT_CONTR:
			mt = "MT_CONTR";
			break;
		case MT_PUNC:
			mt = "MT_PUNC";
			break;
		case MT_STEM:
			mt = "MT_STEM";
			break;
		case MT_PREFIX:
			mt = "MT_PREFIX";
			break;
		case MT_MIDDLE:
			mt = "MT_MIDDLE";
			break;
		case MT_SUFFIX:
			mt = "MT_SUFFIX";
			break;
		default:
			/* No truncation is expected. */
			snprintf(buff, sizeof(buff), "MT_%d", (int)w->morpheme_type);
			mt = string_set_add(buff, sent->string_set);
	}

	return mt;
}
#endif /* USE_WORDGRAPH_DISPLAY || defined(DEBUG) */

#if USE_WORDGRAPH_DISPLAY
/* === Wordgraph graphical representation === */

static void wordgraph_legend(dyn_str *wgd, unsigned int mode)
{
	size_t i;
	static char const *wst[] = {
		"RE", "Matched a regex",
		"SP", "Result of spell guess",
		"RU", "Separated run-on word",
		"HA", "Has an alternative",
		"UNS", "Also unsplit_word",
		"IN", "In the dict file",
		"FI", "First char is uppercase"
	};

	append_string(wgd,
		"subgraph cluster_legend {\n"
		"label=Legend;\n"
		"%s"
		"legend [label=\"subword\\n(status-flags)\\nmorpheme-type\"];\n"
		"legend [xlabel=\"ordinal-number\\ndebug-label\"];\n"
		"%s"
		"legend_width [width=4.5 height=0 shape=none label=<\n"
		"<table border='0' cellborder='1' cellspacing='0'>\n"
		"<tr><td colspan='2'>status-flags</td></tr>\n",
		(mode & WGR_SUB) ? "subgraph cluster_unsplit_word {\n"
		                   "label=\"ordinal-number unsplit-word\";\n" : "",
		(mode & WGR_SUB) ? "}\n" : ""

	);
	for (i = 0; i < sizeof(wst)/sizeof(wst[0]); i += 2)
	{
		append_string(wgd,
		  "<tr><td align='left'>%s</td><td align='left'>%s</td></tr>\n",
		  wst[i], wst[i+1]);
	}

	append_string(wgd,
		"</table>>];"
		"}\n"
		"subgraph cluster_legend_top_space {\n"
			"style=invis legend_dummy [style=invis height=0 shape=box]\n"
		"};\n"
	);
}

/**
 * Graph node name: Add "Sentence:" for the main node.
 * Also escape " and \ with a \.
 */
static const char *wlabel(Sentence sent, const Gword *w)
{
	const char *s;
	const char sentence_label[] = "Sentence:\\n";
	dyn_str *l = dyn_str_new();
	char c0[] = "\0\0";

	assert((NULL != w) && (NULL != w->subword), "Word must exist");
	if ('\0' == *w->subword)
		 return string_set_add("(nothing)", sent->string_set);

	if (w == sent->wordgraph) dyn_strcat(l, sentence_label);

	for (s = w->subword; *s; s++)
	{
		switch (*s)
		{
			case '\"':
				dyn_strcat(l, "\\\"");
				break;
			case '\\':
				dyn_strcat(l, "\\");
				break;
			default:
				*c0 = *s;
				dyn_strcat(l, c0);
		}
	}

	char *label_str = dyn_str_take(l);
	s = string_set_add(label_str, sent->string_set);
	free(label_str);
	return s;
}

/**
 *  Generate the wordgraph in dot(1) format, for debug.
 */
static dyn_str *wordgraph2dot(Sentence sent, unsigned int mode, const char *modestr)
{
	const Gword *w;
	Gword	**wp;
	dyn_str *wgd = dyn_str_new(); /* the wordgraph in dot representation */
	char nn[2*sizeof(char *) + 2 + 2 + 1]; /* \"%p\" node name: "0x..."+NUL*/

	/* This function is called only if we have a wordgraph, in which case
	 * chain_next is non-NULL. So stop static analyzers to complain that
	 * it can be possibly NULL. */
	UNREACHABLE(NULL == sent->wordgraph->chain_next);

	append_string(wgd, "# Mode: %s\n", modestr);
	dyn_strcat(wgd, "digraph G {\nsize =\"30,20\";\nrankdir=LR;\n");
	if ((mode & (WGR_SUB)) && !(mode & WGR_COMPACT))
		dyn_strcat(wgd, "newrank=true;\n");
	if (mode & WGR_LEGEND) wordgraph_legend(wgd, mode);
	append_string(wgd, "\"%p\" [shape=box,style=filled,color=\".7 .3 1.0\"];\n",
	              sent->wordgraph);

	for (w = sent->wordgraph; w; w = w->chain_next)
	{
		bool show_node;

		if (!(mode & WGR_UNSPLIT) && (MT_INFRASTRUCTURE != w->morpheme_type))
		{
			Gword *wu;

			show_node = false;
			/* In this mode nodes that are only unsplit_word are not shown. */
			for (wu = sent->wordgraph; wu; wu = wu->chain_next)
			{
				if (NULL != wu->next)
				{
					for (wp = wu->next; *wp; wp++)
					{
						if (w == *wp)
						{
							show_node = true;
							break;
						}
					}
				}
			}

			if (!show_node) continue;
		}

		snprintf(nn, sizeof(nn), "\"%p\"", w);

		/* Subword node format:
		 *                     +------------------+
		 *                     +                  +
		 *                     +    w->subword    +
		 *                     +    (w->flags)    +
		 *                     + w->morpheme_type +
		 *                     +                  +
		 *                     +------------------+
		 *          w->node_num  } <- external node label
		 *           w->label    }
		 *
		 * The flags and morpheme type are printed symbolically.
		 * The node_num field is the ordinal number of word creation.
		 * The label shows the code positions that created the subword.
		 * The external node label may appear at other positions near the node.
		 *
		 * FIXME: Use HTML labels.
		 */

		append_string(wgd, "%s [label=\"%s\\n(%s)\\n%s\"];\n", nn,
			wlabel(sent, w), gword_status(sent, w), gword_morpheme(sent, w));

		if (!(mode & WGR_DBGLABEL))
		{
			append_string(wgd, "%s [xlabel=\"%zu",
							  nn, w->node_num);
		}
		else
		{
			append_string(wgd, "%s [xlabel=\"%zu\\n%s",
							  nn, w->node_num, w->label);
		}

		/* For debugging this function: display also hex node names. */
		if (mode & WGR_DOTDEBUG)
			append_string(wgd, "\\n%p-%s", w, wlabel(sent, w));

		dyn_strcat(wgd, "\"];\n");

		if (NULL != w->next)
		{
			for (wp = w->next; *wp; wp++)
			{
				append_string(wgd, "%s->\"%p\" [label=next color=red];\n",
				              nn, *wp);
			}
		}
		if (mode & WGR_PREV)
		{
			if (NULL != w->prev)
			{
				for (wp = w->prev; *wp; wp++)
				{
					append_string(wgd, "%s->\"%p\" [label=prev color=blue];\n",
					              nn, *wp);
				}
			}
		}
		if (mode & WGR_UNSPLIT)
		{
			if (!(mode & WGR_SUB) && (NULL != w->unsplit_word))
			{
				append_string(wgd, "%s->\"%p\" [label=unsplit];\n",
				              nn, w->unsplit_word);
			}
		}
	}

	if (mode & WGR_SUB)
	{
		const Gword *old_unsplit = NULL;

		for (w = sent->wordgraph; w; w = w->chain_next)
		{
			if (NULL != w->unsplit_word)
			{
				if (w->unsplit_word != old_unsplit)
				{
					if (NULL != old_unsplit) dyn_strcat(wgd, "}\n");
					append_string(wgd, "subgraph \"cluster-%p\" {", w->unsplit_word);
					append_string(wgd, "label=\"%zu %s\"; \n",
						w->unsplit_word->node_num, wlabel(sent, w->unsplit_word));

					old_unsplit = w->unsplit_word;
				}
				snprintf(nn, sizeof(nn), "\"%p\"", w);
				if (strstr(dyn_str_value(wgd), nn))
					append_string(wgd, "\"%p\"; ", w);
			}
		}
		dyn_strcat(wgd, "}\n");
	}
	else
	{
#ifdef WGR_SHOW_TERMINATOR_AT_LHS /* not defined - not useful */
		const Gword *terminating_node = NULL;
#endif

		dyn_strcat(wgd, "{rank=same; ");
		for (w = sent->wordgraph->chain_next; w; w = w->chain_next)
		{
			snprintf(nn, sizeof(nn), "\"%p\"", w);
			if (IS_SENTENCE_WORD(sent, w) &&
			    ((mode & WGR_UNSPLIT) || strstr(dyn_str_value(wgd), nn)))
			{
				append_string(wgd, "%s; ", nn);
			}

#ifdef WGR_SHOW_TERMINATOR_AT_LHS
			if (NULL == w->next) terminating_node = w;
#endif
		}
		dyn_strcat(wgd, "}\n");

#ifdef WGR_SHOW_TERMINATOR_AT_LHS
		if (terminating_node)
			append_string(wgd, "{rank=sink; \"%p\"}\n", terminating_node);
#endif
	}

	dyn_strcat(wgd, "\n}\n");

	return wgd;
}

#if defined(HAVE_FORK) && !defined(POPEN_DOT)
static pid_t pid; /* XXX not reentrant */

#ifndef HAVE_PRCTL
/**
 * Cancel the wordgraph viewers, to be used if there is fork() but no prctl().
 */
static void wordgraph_show_cancel(void)
{
		kill(pid, SIGTERM);
}
#endif /* HAVE_FORK */
#endif /* HAVE_PRCTL */

#ifndef DOT_COMMNAD
#define DOT_COMMAND "dot"
#endif

#ifndef DOT_DRIVER
#define DOT_DRIVER "-Txlib"
#endif

/* In case files are used, their names are fixed. So more than one thread
 * (or program) cannot use the word-graph display at the same time. This
 * can be corrected, even though there is no much point to do that
 * (displaying the word-graph is for debug). */
#define DOT_FILENAME "lg-wg.vg"

#define POPEN_DOT_CMD DOT_COMMAND" "DOT_DRIVER
#ifndef POPEN_DOT_CMD_NATIVE
#  ifdef _WIN32
#    ifndef IMAGE_VIEWER
#      define IMAGE_VIEWER "rundll32 PhotoViewer,ImageView_Fullscreen"
#    endif
#    define WGJPG "%TEMP%\\lg-wg.jpg"
#    define POPEN_DOT_CMD_NATIVE \
				DOT_COMMAND" -Tjpg>"WGJPG"&"IMAGE_VIEWER" "WGJPG"&del "WGJPG
#  elif __APPLE__
#    ifndef IMAGE_VIEWER
#      define IMAGE_VIEWER "open -W"
#    endif
#    define WGJPG "$TMPDIR/lg-wg.jpg"
#    define POPEN_DOT_CMD_NATIVE \
				DOT_COMMAND" -Tjpg>"WGJPG";"IMAGE_VIEWER" "WGJPG";rm "WGJPG
#  else
#    define POPEN_DOT_CMD_NATIVE POPEN_DOT_CMD
#  endif
#endif

#if !defined HAVE_FORK || defined POPEN_DOT
#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif
/**
 * popen a command with the given input.
 * If the system doesn't have fork(), popen() is used to launch "dot".
 * This is an inferior implementation than the one below that uses
 * fork(), in which the window remains open and is updated automatically
 * when new sentences are entered. With popen(), the program blocks at
 * pclose() and the user needs to close the window after each sentence.
 */
static bool x_popen(const char *cmd, const char *wgds)
{
	lgdebug(+3, "Invoking: %s\n", cmd);
	FILE *const cmdf = popen(cmd, "w");
	bool rc = true;

	if (NULL == cmdf)
	{
		prt_error("Error: popen of '%s' failed: %s\n", cmd, strerror(errno));
		rc = false;
	}
	else
	{
		if (fputs(wgds, cmdf) == EOF) /* see default_error_handler() */
		{
			prt_error("Error: x_popen(): fputs() error: %s\n", strerror(errno));
			rc = false;
		}
		if (pclose(cmdf) == -1)
		{
			prt_error("Error: x_popen(): pclose() error: %s\n", strerror(errno));
			rc = false;
		}
	}

	return rc;
}
#else
static bool x_forkexec(const char *const argv[], pid_t *vpid, const char err[])
{
	/* Fork/exec a graph viewer, and leave it in the background until we exit.
	 * On exit, send SIGHUP. If prctl() is not available and the program
	 * crashes, then it is left to the user to exit the viewer. */
	if (0 < *vpid)
	{
		pid_t rpid = waitpid(*vpid, NULL, WNOHANG);

		if (0 == rpid) return true; /* viewer still active */
		if (-1 == rpid)
		{
			prt_error("Error: waitpid(%d): %s\n", *vpid, strerror(errno));
			*vpid = 0;
			return false;
		}
	}

	*vpid = fork();
	switch (*vpid)
	{
		case -1:
			prt_error("Error: fork(): %s\n", strerror(errno));
			return false;
		case 0:
#ifdef HAVE_PRCTL
			if (-1 == prctl(PR_SET_PDEATHSIG, SIGHUP))
			{
					prt_error("Error: prctl: %s\n", strerror(errno));
					/* Non-fatal error - continue. */
			}
#endif
			/* Not closing fd 0/1/2, to allow interaction with the program */
			execvp(argv[0], (char **)argv);
			prt_error("Error: execlp of %s: %s%s\n",
			          argv[0], strerror(errno), (ENOENT == errno) ? err : "");
			_exit(1);
		default:
#ifndef HAVE_PRCTL
			if (0 != atexit(wordgraph_show_cancel))
			{
				 prt_error("Warning: atexit(wordgraph_show_cancel) failed.\n");
				/* Non-fatal error - continue. */
			}
#endif
			break;
	}

	return true;
}
#endif /* !defined HAVE_FORK || defined POPEN_DOT */

#ifdef _WIN32
#define TMPDIR (getenv("TEMP") ? getenv("TEMP") : ".")
#else
#define TMPDIR (getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp")
#endif

#define concatfn(fn, fn1, fn2) \
	(fn=alloca(strlen(fn1)+strlen(fn2)+2),\
	 strcpy(fn, fn1), strcat(fn, "/"), strcat(fn, fn2))

static void wordgraph_unlink_xtmpfile(void)
{
	char *fn;

	if (!test_enabled("gvfile"))
	{
		concatfn(fn, TMPDIR, DOT_FILENAME);
		if (unlink(fn) == -1)
			prt_error("Warning: Cannot unlink %s: %s\n", fn, strerror(errno));
	}
}

/**
 * Display the word-graph in the indicated mode.
 * This is for debug and inspiration. It is not reentrant due to the
 * static pid and the possibly created fixed filenames.
 * When Using X11, a "dot -Txlib" program is launched on the graph
 * description file.  The xlib driver refreshes the graph when the file is
 * changed, displaying additional sentences in the same window.  The viewer
 * program exits on program end (see the comments in the code).  When
 * compiled with MSVC or MINGW, the system PhotoViewer is used by default,
 * unless !wg:x is used (for using X11 when available).
 *
 * The "dot" and the "PhotoViewer" programs must be in the PATH.
 *
 * FIXME? "dot" may get a SEGV due to memory corruptions in it (a known
 * problem - exists even in 2.38). This can be worked-around by trying it
 * again until it succeeds (but the window size, if changed by the user,
 * will not be preserved).
 *
 * modestr: a graph display mode as defined in wordgraph.h (default "ldu").
 */
bool sentence_display_wordgraph(Sentence sent, const char *modestr)
{
	dyn_str *wgd;
	char *gvf_name = NULL;
	bool generate_gvfile = test_enabled("gvfile"); /* keep it for debug */
	char *wgds;
	bool gvfile = false;
	uint32_t mode = 0;
	const char *mp;
	bool rc = true;

	for (mp = modestr; '\0' != *mp && ',' != *mp; mp++)
	{
		if ((*mp >= 'a') && (*mp <= 'z')) mode |= 1<<(*mp-'a');
	}
	if ((0 == mode) || (WGR_X11 == mode))
		mode |= WGR_LEGEND|WGR_DBGLABEL|WGR_UNSPLIT;

	wgd = wordgraph2dot(sent, mode, modestr);
	wgds = dyn_str_take(wgd);

#if defined(HAVE_FORK) && !defined(POPEN_DOT)
	gvfile = true;
#endif

	if (gvfile || generate_gvfile)
	{
		FILE *gvf;
		bool gvf_error = false;
		static bool wordgraph_unlink_xtmpfile_needed = true;

		concatfn(gvf_name, TMPDIR, DOT_FILENAME);
		gvf = fopen(gvf_name, "w");
		if (NULL == gvf)
		{
			prt_error("Error: %s(): fopen() of %s failed: %s\n",
						 __func__, gvf_name, strerror(errno));
			gvf_error = true;
		}
		else
		{
			if (fputs(wgds, gvf) == EOF)
			{
				gvf_error = true;
				prt_error("Error: %s(): fputs() to %s failed: %s\n",
							 __func__, gvf_name, strerror(errno));
			}
			if (fclose(gvf) == EOF)
			{
				gvf_error = true;
				prt_error("Error: %s(): fclose() of %s failed: %s\n",
							  __func__, gvf_name, strerror(errno));
			}
		}
		if (gvf_error && gvfile) /* we need it - cannot continue */
		{
			rc = false;
			goto finish;
		}

		if (wordgraph_unlink_xtmpfile_needed)
		{
			/* The filename is fixed - removal needed only once. */
			wordgraph_unlink_xtmpfile_needed = false;
			atexit(wordgraph_unlink_xtmpfile);
		}
	}

#ifdef _WIN32
#define EXITKEY "ALT-F4"
#elif __APPLE__
#define EXITKEY "⌘-Q"
#endif

#ifdef EXITKEY
	prt_error("Press "EXITKEY" in the graphical display window to continue\n");
#endif

#if !defined HAVE_FORK || defined POPEN_DOT
	rc = x_popen((mode & WGR_X11)? POPEN_DOT_CMD : POPEN_DOT_CMD_NATIVE, wgds);
#else
	{
		assert(NULL != gvf_name, "DOT filename not initialized (#define mess?)");
		const char *const args[] = { DOT_COMMAND, DOT_DRIVER, gvf_name, NULL };
		const char notfound[] =
			" (command not in PATH; \"graphviz\" package not installed?).";
		rc = x_forkexec(args, &pid, notfound);
	}
#endif

finish:
	free(wgds);
	return rc;
}
#else
bool sentence_display_wordgraph(Sentence sent, const char *modestr)
{
	prt_error("Error: Library not configured with wordgraph-display\n");
	return false;
}
#endif /* USE_WORDGRAPH_DISPLAY */
