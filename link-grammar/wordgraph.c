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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>    /* fork() and execl() */
#ifdef HAVE_PRCTL
#include <sys/prctl.h> /* prctl() */
#endif
#include <sys/wait.h>  /* waitpid() */
#include <signal.h>    /* SIG* */

#include "error.h"
#include "externs.h"
#include "string-set.h"
#include "structures.h"
#include "print-util.h"
#include "wordgraph.h"

/* === Gword utilities === */
/* Many more Gword utilities, that are used only in particular files,
 * are define in these files staticly. */

Gword *gword_new(Sentence sent, const char *s)
{
	Gword * const word = malloc(sizeof(*word));

	/* FIXME? NULL can be used if the word is yet unknown. Is it needed? */
	if (NULL== s) lgdebug(+0, "Null-string subword\n");
	word->subword = (NULL == s) ? NULL : string_set_add(s, sent->string_set);

	if (NULL != sent->last_word) sent->last_word->chain_next = word;
	sent->last_word = word;
	word->unsplit_word = NULL;
	word->chain_next = NULL;
	word->prev = word->next = NULL;
	word->issued_unsplit = false;
	word->tokenizing_step = 0;
	word->status = 0;
	word->morpheme_type = MT_INVALID;
	word->label = NULL;
	word->regex_name = NULL;
	word->alternative_id = NULL;
	word->hier_position = NULL;
	word->node_num = sent->gword_node_num++;
	word->split_counter = 0;
	word->null_subwords = NULL;

	return word;
}

Gword *empty_word(void)
{
#ifndef MSC_VER
	static Gword e = {
		.subword = EMPTY_WORD_MARK,
		.unsplit_word = &e,
		.morpheme_type = MT_EMPTY,
		.alternative_id = &e,
		.status = WS_INDICT,
	};
#else
	static Gword e;

	e.subword = EMPTY_WORD_MARK;
	e.unsplit_word = &e;
	e.morpheme_type = MT_EMPTY;
	e.alternative_id = &e;
	e.status = WS_INDICT;
#endif

	return &e;
}

Gword **wordlist_resize(Gword **arr, size_t len)
{
	arr = realloc(arr, (len+2) * sizeof(Gword *));
	arr[len+1] = NULL;
	return arr;
}

size_t wordlist_len(const Gword **arr)
{
	size_t len = 0;
	if (arr)
		while (arr[len] != NULL) len++;
	return len;
}

void wordlist_append(Gword ***arrp, Gword *p)
{
	size_t n = wordlist_len((const Gword **)*arrp);

	*arrp = wordlist_resize(*arrp, n);
	(*arrp)[n] = p;
}

#if 0
/**
 * Replace "count" words from the position "start" by word "wnew".
 */
static void wordlist_replace(Gword ***arrp, size_t start, size_t count,
                             const Gword *wnew)
{
	size_t n = wordlist_len((const Gword **)(*arrp+start+count));

	memmove(*arrp+start+1, *arrp+start+count, (n+1) * sizeof(Gword *));
	(*arrp)[start] = (Gword *)wnew;
}
#endif

#if defined(USE_WORDGRAPH_DISPLAY) || defined(DEBUG)
/**
 * Create a short form of flags summary for displaying in a word node.
 */
GNUC_UNUSED const char *gword_flags(Sentence sent, const Gword *w)
{
	dyn_str *s = dyn_str_new();
	const char *r;
	size_t len;

	if (w->status & WS_UNKNOWN)
		dyn_strcat(s, "UNK|");
	if (w->status & WS_INDICT)
		dyn_strcat(s, "IN|");
	if (w->status & WS_REGEX)
		dyn_strcat(s, "RE|");
	if (w->status & WS_SPELL)
		dyn_strcat(s, "SP|");
	if (w->status & WS_RUNON)
		dyn_strcat(s, "RU|");
	if (w->status & WS_HASALT)
		dyn_strcat(s, "HA|");
	if (w->status & WS_UNSPLIT)
		dyn_strcat(s, "UNS|");
	if (w->status & WS_PL)
		dyn_strcat(s, "PL|");

	len = strlen(s->str);
	if (len > 0) s->str[len-1] = '\0';
	r = string_set_add(s->str, sent->string_set);
	dyn_str_delete(s);
	return r;
}
#endif /* USE_WORDGRAPH_DISPLAY || DEBUG) */

#if USE_WORDGRAPH_DISPLAY
/* === Wordgraph graphical representation === */

/**
 * Graph node name: Add "Sentence:" for the main node; Convert SUBSCRIPT_MARK.
 * Also escape " and \ with a \.
 */
static const char *wlabel(Sentence sent, const Gword *w)
{
	const char *s;
	const char sentence_label[] = "Sentence:\\n";
	dyn_str *l = dyn_str_new();
	char c0[] = "\0\0";

	assert((NULL != w) && (NULL != w->subword) && ('\0' != *w->subword),
	       "Word and subword should exist");
	if (w == sent->wordgraph) dyn_strcat(l, sentence_label);

	for (s = w->subword; *s; s++)
	{
		switch (*s)
		{
			case SUBSCRIPT_MARK:
				dyn_strcat(l, ".");
				break;
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

	s = string_set_add(l->str, sent->string_set);
	dyn_str_delete(l);
	return s;
}

/**
 *  Generate the wordgraph in dot(1) format, for debug.
 */
static String *wordgraph2dot(Sentence sent, int mode, const char *modestr)
{
	const Gword *w;
	Gword	**wp;
	String *wgd = string_new(); /* the wordgraph in dot representation */
	char nn[2*sizeof(char *) + 2 + 2 + 1]; /* \"%p\" node name: "0x..."+NUL*/

	append_string(wgd, "# Mode: %s\n", modestr);
	append_string(wgd, "digraph G {\nsize =\"30,20\";\nrankdir=LR;\n");
	append_string(wgd, "\"%p\" [shape=box,style=filled,color=\".7 .3 1.0\"];\n",
	              sent->wordgraph);

	for (w = sent->wordgraph; w; w = w->chain_next)
	{
		bool show_node;

		if ((mode & WGR_NOUNSPLIT) && (MT_INFRASTRUCTURE != w->morpheme_type))
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

		sprintf(nn, "\"%p\"", w);

		/* Subword node - its name is the token and (label) */
		{
			char dl[128] = "";

			if (!(mode & WGR_NODBGLABEL))
			{
				/* FIXME: HTML labels */
				const size_t l = snprintf(dl, sizeof(dl), "\\n(%s)\\n%s",
				                          w->label, gword_flags(sent, w));

				/* "Cannot happen" */
				if (l >= sizeof(dl))
				{
					char ellipsis[] = "...";

					prt_error("Warning: Debug label truncated for word '%s'\n",
					          w->unsplit_word->subword);
					strcpy(dl+sizeof(dl)-sizeof(ellipsis)-1, ellipsis);
				}

			}
			append_string(wgd, "%s [label=\"%s%s\"];\n",
			              nn, wlabel(sent, w), dl);
			append_string(wgd, "%s [xlabel=\"%zu\"];\n", nn, w->node_num);
			/* For debugging this function: display also hex node names. */
			//append_string(wgd, "%s" [xlabel=\"%zu %p\"];\n", nn,w->node_num, nn);
		}

		if (NULL != w->next)
		{
			for (wp = w->next; *wp; wp++)
			{
				append_string(wgd, "%s->\"%p\" [label=next color=red];\n",
				              nn, *wp);
			}
		}
		if (!(mode & WGR_NOPREV))
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
		if (!(mode & WGR_NOUNSPLIT))
		{
			if ((mode & WGR_REG) && (NULL != w->unsplit_word))
			{
				append_string(wgd, "%s->\"%p\" [label=unsplit];\n",
				              nn, w->unsplit_word);
			}
		}
	}

	if (mode & WGR_REG)
	{
#ifdef WGR_SHOW_TERMINATOR_AT_LHS /* not defined - not useful */
		const Gword *terminating_node = NULL;
#endif

		append_string(wgd, "{rank=same; ");
		for (w = sent->wordgraph->chain_next; w; w = w->chain_next)
		{
			sprintf(nn, "\"%p\"", w);
			if ((w->unsplit_word == sent->wordgraph) &&
			    (!(mode & WGR_NOUNSPLIT) || strstr(string_value(wgd), nn)))
			{
				append_string(wgd, "%s; ", nn);
			}

#ifdef WGR_SHOW_TERMINATOR_AT_LHS
			if (NULL == w->next) terminating_node = w;
#endif
		}
		append_string(wgd, "}\n");

#ifdef WGR_SHOW_TERMINATOR_AT_LHS
		if (terminating_node)
			append_string(wgd, "{rank=sink; \"%p\"}\n", terminating_node);
#endif
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
					if (NULL != old_unsplit) append_string(wgd, "}\n");
					append_string(wgd, "subgraph \"cluster-%p\" {", w->unsplit_word);
					append_string(wgd, "label=\"%s\"; \n",
					              wlabel(sent, w->unsplit_word));

					old_unsplit = w->unsplit_word;
				}
				append_string(wgd, "\"%p\"; ", w);
			}
		}
		append_string(wgd, "}\n");
	}

	append_string(wgd, "\n}\n");

	return wgd;
}

static pid_t pid[WGR_MAX*2]; /* XXX not reentrant */

#ifndef HAVE_PRCTL
/**
 * Cancel the wordgraph viewers, to be used if there is no prctl().
 */
static void wordgraph_show_cancel(void)
{
	size_t i;

	for (i = 0; i < sizeof(pid)/sizeof(pid_t); i++)
		if (0 != pid[i]) kill(pid[i], SIGTERM);
}
#endif

/* FIXME: use the graphviz library instead of invoking dot, as hinted in:
 * http://www.graphviz.org/content/how-use-graphviz-library-c-project */

/*
 * The files that are generated by wordgraph_show() can be viewed using
 * "dot -Txlib /tmp/wgXX.gv" (XX is the hexcode of the mode parameter).
 * XXX Generating fixed filesnames is not reentrant. On the other hand,
 * this is currently only for debugging.
 * FIXME If needed, even this can be fixed.
 */
#ifndef DOT_PATH
#define DOT_PATH "/usr/bin/dot"
#endif
#ifndef DOT_DRIVER
#define DOT_DRIVER "-Txlib"
#endif
#ifndef POPEN_DOT_CMD
#define POPEN_DOT_CMD "dot -Tx11"
#endif

/**
 * Save the wordgraph in a file and also display it.
 * This is for debug. It is not reentrant due to tyhe static pid[].  Files in
 * the format /tmp/wgXX.gv are created, when XX is the mode in hex.
 * A "dot -Txlib" program is automatically launched on them. The xlib driver
 * knows to refresh the graph on file change, and thus additional sentences are
 * shown in the same windows.
 * There can be several viewer active at once.
 * The viewers exit automatically on program end (see the comments in the code).
 *
 * Invocation: wordgraph_show(sent, WGR_x1|WGR_x2|...) - see the macro below.
 *
 * FIXME: Should be renamed, wordgraph-specific functionality moved to the
 * caller, and made more general.
 * FIXME? "dot" 2.34.0 often gets a SEGV due to memory corruptions in it (a
 * known problem). This can be worked-around by trying it again several times
 * until it succeeds (but the window size, if changed by the user, will not be
 * preserved.). However, this code doesn't do that yet.
 */
void (wordgraph_show)(Sentence sent, size_t mode, const char *modestr)
{
	String *const wgd = wordgraph2dot(sent, mode, modestr);
	char psf_template[] = "/tmp/wg%0.*x.gv"; /* XXX not reentrant */
	char psf_name[sizeof psf_template + WGR_MAXDIGITS];
	FILE *psf;
	char *wgds;

	assert(mode > 0);

	wgds = string_copy(wgd);
	string_delete(wgd);

	sprintf(psf_name, psf_template, WGR_MAXDIGITS, mode);
	psf = fopen(psf_name, "w");
	if (NULL == psf)
	{
		/* Hopefully errno is getting set in case of an error. */
		prt_error("wordgraph_show: open %s failed: %s\n",
		          psf_name, strerror(errno));
	}
	else
	{
		if (fprintf(psf, "%s", wgds) < 0)
			prt_error("wordgraph_show: print to %s failed: %s\n",
		             psf_name, strerror(errno));
		if (fclose(psf) == EOF)
			prt_error("wordgraph_show: close %s failed: %s\n",
			           psf_name, strerror(errno));
	}

	/* If the system doesn't have fork(), popen() is used to launch "dot".
	 * This is an inferior implementation than the one below that uses fork(),
	 * because pclose() is blocked until the user closes the "dot" window. */
#if !defined HAVE_FORK || defined POPEN_DOT
	{
		const char *cmd = POPEN_DOT_CMD;
		FILE *const cmdf = popen(cmd, "w");

		if (NULL == cmdf)
		{
			/* Hopefully errno is getting set in case of an error. */
			prt_error("wordgraph_show: popen of '%s' failed: %s\n",
			           cmd, strerror(errno));
		}
		else
	free(wgds);
		{
			if (fprintf(cmdf, "%s", wgds) < 0)
				prt_error("wordgraph_show: print to display command failed: %s\n",
				          strerror(errno));
			if (pclose(cmdf) == -1)
				prt_error("wordgraph_show: pclose of display command failed: %s\n",
			             strerror(errno));
		}

		free(wgds);
	}
#else
	free(wgds);
	{
		/* Fork/exec a graph viwer, and leave it in the background until we exit.
		 * On exit, send SIGHUP. If prctl() is not available and the program
		 * crashes, then it is left to the user to exit the viwer. */
		char cmdpath[] = DOT_PATH;

		assert(mode <= sizeof(pid)/sizeof(pid_t));
		mode--;

		if (0 < pid[mode])
		{
			pid_t rpid = waitpid(pid[mode], NULL, WNOHANG);

			if (0 == rpid) return; /* viewer still active */
			if (-1 == rpid)
			{
				/* FIXME? Respawn in case of a viewer crash (can be found by the
				 * wait status, which is not retrieved for now). */
				prt_error("Error: waitpid(%d): %s (will not respawn viewer)\n",
				          pid[mode], strerror(errno));
				return;
			}
		}

		pid[mode] = fork();

		switch (pid[mode])
		{
			case -1:
				prt_error("Error: wordgraph_show: fork() failed: %s\n",
				          strerror(errno));
				break;
			case 0:
#ifdef HAVE_PRCTL
				if (-1 == prctl(PR_SET_PDEATHSIG, SIGHUP))
				{
						prt_error("Error: prctl(PR_SET_PDEATHSIG, SIGHUP): %s\n",
						          strerror(errno));
				}
#endif
				close(0);
				/* Not closing stdout/stderr, to allow stdout/stderr messages from
				 * the "dot" program. Regretfully what we sometimes get is a crash
				 * message due to memory corruption in "dot" (version 2.34.0, known
				 * problem). */
				execl(cmdpath, "dot", DOT_DRIVER, psf_name, NULL);
				prt_error("Error: debug execl of %s failed: %s\n",
				          cmdpath, strerror(errno));
				/* prt_error() writes to stderr, which doesn't need flush, and also
				 * flushes it anyway, so _exit() can be safely used here. */
				_exit(1);
			default:
#ifndef HAVE_PRCTL
				if (0 != atexit(wordgraph_show_cancel))
					 prt_error("Warning: atexit(wordgraph_show_cancel) failed.\n");
#endif
				break;
		}
	}
#endif
	/* Note: there may be a previous return. */
}
#endif /* USE_WORDGRAPH_DISPLAY */
