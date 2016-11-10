/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"
#include "structures.h"
#include "api-structures.h"
#include "print-util.h"

static TLS lg_error *lasterror;
static TLS void *error_handler_data;
static void default_error_handler(lg_error *, void *);
static TLS lg_error_handler error_handler = default_error_handler;

/* This list should match enum lg_error_severity. */
#define MAX_SEVERITY_LABEL_SIZE 64 /* In bytes. */
const char *severity_label_by_level[] =
{
	"Fatal error", "Error", "Warning", "Info", "Debug", "Trace", /*lg_None*/"",
	NULL
};

/* Name to prepend to messages. */
static const char libname[] = "link-grammar";

/* === Error queue utilities ======================================== */
static lg_error *error_queue_resize(lg_error *lge, int len)
{
	lge = realloc(lge, (len+2) * sizeof(lg_error));
	lge[len+1].msg = NULL;
	return lge;
}

static int error_queue_len(lg_error *lge)
{
	size_t len = 0;
	if (lge)
		while (NULL != lge[len].msg) len++;
	return len;
}

static void error_queue_append(lg_error **lge, lg_error *current_error)
{
	int n = error_queue_len(*lge);

	*lge = error_queue_resize(*lge, n);
	current_error->msg = strdup(current_error->msg);
	(*lge)[n] = *current_error;
}
/* ==================================================================*/

/**
 * Return the error severity according to the start of the error string.
 * If an error severity is not found - return None.
 */
static lg_error_severity message_error_severity(const char *msg)
{
	for (const char **llp = severity_label_by_level; NULL != *llp; llp++)
	{
		for (const char *s = *llp, *t = msg; ; s++, t++)
		{
			if ((':' == *t) && (t > msg))
			{
				return (int)(llp - severity_label_by_level + 1);
			}
			if ((*s != *t) || ('\0' == *s)) break;
		}
	}

	return None;
}

static void lg_error_msg_free(lg_error *lge)
{
		free((void *)lge->msg);
		free((void *)lge->severity_label);
}

/* === API functions ================================================*/
/**
 * Set the error handler function to the given one.
 * @param lg_error_handler New error handler function
 * @param data Argument for the error handler function
 */
lg_error_handler lg_error_set_handler(lg_error_handler f, void *data)
{
	const lg_error_handler oldf = error_handler;
	error_handler = f;
	error_handler_data = data;
	return oldf;
}

const void *lg_error_set_handler_data(void * data)
{
	const char *old_data = error_handler_data;

	error_handler_data = data;
	return old_data;
}

/**
 * Print the error queue and free it.
 * @param f Error handler function
 * @param data Argument for the error handler function
 * @return Number of errors
 */
int lg_error_printall(lg_error_handler f, void *data)
{
	int n = error_queue_len(lasterror);
	if (0 == n) return 0;

	for (lg_error *lge = &lasterror[n-1]; lge >= lasterror; lge--)
	{
		if (NULL == f)
			default_error_handler(lge, data);
		else
			f(lasterror, data);
		lg_error_msg_free(lge);
	}
	free(lasterror);
	lasterror = NULL;

	return n;
}

/**
 * Clear the error queue. Free all of its memory.
 * @return Number of errors
 */
int lg_error_clearall(void)
{
	if (NULL == lasterror) return 0;
	int nerrors = 0;

	for (lg_error *lge = lasterror; NULL != lge->msg; lge++)
	{
		nerrors++;
		lg_error_msg_free(lge);
	}
	free(lasterror);
	lasterror = NULL;

	return nerrors;
}

/**
 * Format the given raw error message.
 * Create a complete error message, ready to be printed.
 * If the severity is not None, add the library name.
 * Also add the severity label.
 * @param lge The raw error message.
 * @return The complete error message. The caller needs to free the memory.
 */
char *lg_error_formatmsg(lg_error *lge)
{
	char *formated_error_message;
	//printf("DEBUG-lg_format_error: msg=%s sev=%d\n", lge->msg, lge->severity);

	String *s = string_new();

	/* Prepend libname to messages with higher severity than Debug. */
	if (lge->severity < Debug)
		append_string(s, "%s: ", libname);

	/* Prepend severity label to messages which don't already have it. */
	if (None == message_error_severity(lge->msg))
		append_string(s, "%s", lge->severity_label);

	append_string(s, "%s", lge->msg);

	formated_error_message = string_copy(s);
	string_delete(s);

	return formated_error_message;
}
/* ================================================================== */

/**
 * The default error handler callback function.
 * @param lge The raw error message.
 */
static void default_error_handler(lg_error *lge, void *data)
{
	FILE *outfile = stdout;

	if (((NULL == data) && (lge->severity <= Debug)) ||
	    ((NULL != data) && (lge->severity <= *(lg_error_severity *)data) &&
	     (None !=  lge->severity)))
	if (((NULL == data) && (lge->severity < Debug)) ||
	    ((NULL != data) && (lge->severity < *(lg_error_severity *)data) &&
	     (None !=  lge->severity)))
	{
		fflush(stdout); /* Make sure that stdout has been written out first. */
		outfile = stderr;
	}

	char *msg = lg_error_formatmsg(lge);
	fprintf(outfile, "%s", msg);
	free(msg);

	fflush(outfile); /* Also stderr, in case some OS does some strange thing */
}

/**
 * Convert a numerical severity level to its corresponding string.
 */
static const char *error_severity_label(lg_error_severity sev)
{
	char *sevlabel;
	sevlabel = alloca(MAX_SEVERITY_LABEL_SIZE);

	if (None == sev)
	{
		sevlabel[0] = '\0';
	}
	else if ((sev < 1) || (sev > None))
	{
		snprintf(sevlabel, MAX_SEVERITY_LABEL_SIZE, "Message severity %d: ", sev);
	}
	else
	{
		snprintf(sevlabel, MAX_SEVERITY_LABEL_SIZE, "%s: ",
		         severity_label_by_level[sev-1]);
	}

	return strdup(sevlabel);
}

static void verr_msg(err_ctxt *ec, lg_error_severity sev, const char *fmt, va_list args)
	GNUC_PRINTF(3,0);

static void verr_msg(err_ctxt *ec, lg_error_severity sev, const char *fmt, va_list args)
{
	String *outbuf = string_new();

	vappend_string(outbuf, fmt, args);

	if ((Info != sev) && ec->sent != NULL)
	{
		size_t i, j;
		const char **a, **b;

#if 0
		/* Previous code. Documenting its problem:
		 * In the current library version (using Wordgraph) it may print a
		 * nonsense sequence of morphemes if the words have been split to
		 * morphemes in various ways, because the "alternatives" array doesn't
		 * hold real alternatives any more (see example in the comments of
		 * print_sentence_word_alternatives()).
		 *
		 * We could print the first path in the Wordgraph, analogous to what we
		 * did here, but (same problem as printing alternatives[0] only) it may
		 * not contain all the words, including those that failed (because they
		 * are in another path). */

		fprintf(stderr, "\tFailing sentence was:\n\t");
		for (i=0; i<ec->sent->length; i++)
		{
			fprintf(stderr, "%s ", ec->sent->word[i].alternatives[0]);
		}
#else
		/* The solution is just to print all the sentence tokenized subwords in
		 * their order in the sentence, without duplications. */

		append_string(outbuf,
		        "\tFailing sentence contains the following words/morphemes:\n\t");
		for (i=0; i<ec->sent->length; i++)
		{
			for (a = ec->sent->word[i].alternatives; NULL != *a; a++)
			{
				bool next_word = false;

				if (0 == strcmp(*a, EMPTY_WORD_MARK)) continue;
				for (j=0; j<ec->sent->length; j++)
				{
					for (b = ec->sent->word[j].alternatives; NULL != *b; b++)
					{
						/* print only the first occurrence. */
						if (0 == strcmp(*a, *b))
						{
							next_word = true;
							if (a != b) break;
							append_string(outbuf, "%s ", *a);
							break;
						}
					}
					if (next_word) break;
				}
			}
		}
#endif
	}
	append_string(outbuf, "\n");

	lg_error current_error;
	/* current_error.ec = *ec; */
	current_error.msg = string_value(outbuf);
	lg_error_severity msg_sev = message_error_severity(current_error.msg);
	current_error.severity = ((None == msg_sev) && (0 != sev)) ? sev : msg_sev;
	current_error.severity_label = error_severity_label(current_error.severity);

	if (NULL == error_handler)
	{
		error_queue_append(&lasterror, &current_error);
	}
	else
	{
		error_handler(&current_error, error_handler_data);
		free((void *)current_error.severity_label);
	}

	string_delete(outbuf);
}

void err_msg(err_ctxt *ec, lg_error_severity sev, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	verr_msg(ec, sev, fmt, args);
	va_end(args);
}

void prt_error(const char *fmt, ...)
{
	err_ctxt ec;
	va_list args;

	ec.sent = NULL;
	va_start(args, fmt);
	verr_msg(&ec, 0, fmt, args);
	va_end(args);
}

/**
 * Check whether the given feature is enabled. It is considered
 * enabled if it is found in the comma delimited list of features.
 * This list, if not empty, has a leading and a trailing commas.
 * Return NULL if not enabled, else ",". If the feature appears
 * as "feature:param", return a pointer to param.
 * @param    list Comma delimited list of features (start/end commas too).
 * @param    ... List of features to check.
 * @return   If not enabled - NULL; Else "," or the feature param if exists.
 */
const char *feature_enabled(const char * list, ...)
{

	const char *feature;
	va_list given_features;
	va_start(given_features, list);

	while (NULL != (feature = va_arg(given_features, char *)))
	{
		size_t len = strlen(feature);
		char *buff = alloca(len + 2 + 1); /* leading comma + comma/colon + NUL */

		/* The "feature" variable may contain a full/relative file path.
		 * If so, extract the file name from it. On Windows first try the
		 * native separator \, but also try /. */
		const char *dir_sep = NULL;
#ifdef _WIN32
		dir_sep = strrchr(feature, '\\');
#endif
		if (NULL == dir_sep) dir_sep = strrchr(feature, '/');
		if (NULL != dir_sep) feature = dir_sep + 1;

		buff[0] = ',';
		strcpy(buff+1, feature);
		strcat(buff, ",");

		if (NULL != strstr(list, buff)) return ",";
		buff[len+1] = ':'; /* check for "feature:param" */
		if (NULL != strstr(list, buff)) return strstr(list, buff) + len + 1;
	}
	va_end(given_features);

	return NULL;
}
