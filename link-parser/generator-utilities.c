#include <stdlib.h>
#include <string.h>

#include "link-grammar/dict-common/dict-api.h"
#include "link-grammar/error.h"

#include "generator-utilities.h"

#define SUBSCRIPT_MARK '\3'
#define SUBSCRIPT_DOT '.'
#define MAX_WORD 180

const char *select_word(const Category *catlist,
                        const Category_cost *cc,
                        WordIdx w,
                        unsigned int disjunct_category_idx)
{
	/* Select a disjunct category. */
	unsigned int category_num = cc[disjunct_category_idx].num;
	lgdebug(5, "Word %zu: category %d \"%u\";", w,
	        disjunct_category_idx, category_num);
	category_num--; /* Categories starts with 1 but Category is 0 based. */
	unsigned int num_words = catlist[category_num].num_words;

	/* Select a dictionary word from the selected disjunct category. */
	unsigned int r = (unsigned int)rand();
	unsigned int dict_word_idx = r % num_words;
	const char *word = catlist[category_num].word[dict_word_idx];
	lgdebug(5, " r=%08x word %d/%u \"%s\"\n",
	        r, dict_word_idx, num_words, word);

	return word;
}

static const char *cond_subscript(const char *ow, bool leave_subscript)
{
	const char *sm = strchr(ow, SUBSCRIPT_MARK);
	if (sm == NULL) return ow;

	static char w[MAX_WORD + 1]; // XXX FIXME this is not thread-safe!
	strcpy(w, ow);
	w[sm - ow] = leave_subscript ? SUBSCRIPT_DOT : '\0';
	return w;
}

static void print_sent(const Category* catlist,
                       Linkage linkage, size_t nwords, const char** words,
                       bool subscript,
                       const Category_cost** cclist,
                       unsigned int* cc_select)
{
	for(WordIdx w = 0; w < nwords; w++)
	{
		const Category_cost *cc = cclist[w];
		if (cc == NULL)
		{
			// When is cc NULL? When does this ever happen?
			printf("%s", cond_subscript(words[w], subscript));
			if (w < nwords-1) printf(" ");
			continue;
		}

		unsigned int dj_cat_idx = cc_select[w];
		const char *word = select_word(catlist, cc, w, dj_cat_idx);
		printf("%s", cond_subscript(word, subscript));
		if (w < nwords-1) printf(" ");
	}

	printf("\n");
}

static void print_sent_all(const Category* catlist,
                           Linkage linkage, size_t nwords, const char** words,
                           bool subscript)
{
	const Category_cost* cclist[nwords];
	unsigned int cclen[nwords];
	unsigned int cc_select[nwords];

	for(WordIdx w = 0; w < nwords; w++)
	{
		const Category_cost* cc = linkage_get_categories(linkage, w);
		cclist[w] = cc;

		unsigned int dj_num_cats = 0;
		cclen[w] = 0;
		cc_select[w] = 0;
		if (NULL != cclist[w])
		{
			while (cc[dj_num_cats].num != 0) dj_num_cats++;
			assert(dj_num_cats != 0, "Bad disjunct!");

			cclen[w] = dj_num_cats;
		}
	}

	print_sent(catlist, linkage, nwords, words, subscript,
	           cclist, cc_select);
}

void print_sentence(const Category* catlist,
                    Linkage linkage, size_t nwords, const char** words,
                    bool subscript, bool explode)
{
	if (explode)
	{
		print_sent_all(catlist, linkage, nwords, words, subscript);
		return;
	}

	/* Otherwise, just select one word at random */
	const Category_cost* cclist[nwords];
	unsigned int cclen[nwords];
	unsigned int cc_select[nwords];

	for(WordIdx w = 0; w < nwords; w++)
	{
		const Category_cost* cc = linkage_get_categories(linkage, w);
		cclist[w] = cc;

		unsigned int dj_num_cats = 0;
		cclen[w] = 0;
		cc_select[w] = 0;
		if (NULL != cc)
		{
			while (cc[dj_num_cats].num != 0) dj_num_cats++;
			assert(dj_num_cats != 0, "Bad disjunct!");

			cclen[w] = dj_num_cats;

			/* Pick one at random. */
			unsigned int r = (unsigned int)rand();
			cc_select[w] = r % dj_num_cats;
		}
	}

	print_sent(catlist, linkage, nwords, words, subscript,
	           cclist, cc_select);
}

/* ------------------------------------------------------------ */

void dump_categories(Dictionary dict, const Category *catlist)
{
#define TAB "   "
	const int maxcol = 60; /* Account for initial formatting tabs. */

	printf(TAB"\"categories\": [\n");
	for (unsigned int n = 0; catlist[n].num_words != 0; n++)
	{
		printf(TAB"\{\n");
		if (catlist[n].name[0] != '\0')
			printf(TAB TAB"\"name\": %s,\n", catlist[n].name);
		printf(TAB TAB"\"category_num\": %u,\n", n + 1);
		printf(TAB TAB"\"exp\": %s,\n", lg_exp_stringify(catlist[n].exp));
		printf(TAB TAB"\"num_words\": %u,\n", catlist[n].num_words);
		printf(TAB TAB"\"words\": [\n");
		printf(TAB TAB TAB);

		int currcol = 0;
		for (unsigned int wi = 0; wi < catlist[n].num_words; wi++)
		{
			const char *word = catlist[n].word[wi];
			if ((currcol > 0) && currcol + (int)strlen(word) > maxcol)
			{
				if (wi != catlist[n].num_words - 1) printf("\n"TAB TAB TAB);
				currcol = 0;
			}
			currcol += printf("\"%s\"", word); /* No crash if printf() fails. */
			if (wi != catlist[n].num_words - 1)
			{
				printf(",");
				currcol++;
			}
			else
			{
				printf("\n");
			}
		}
		printf(TAB TAB"]\n");
		printf(TAB"}");
		if (catlist[n+1].num_words != 0)
			printf(",");
		printf("\n");
	}
	printf(TAB"]\n");
	printf("}\n");
}

/* =========================== END OF FILE =====================*/
