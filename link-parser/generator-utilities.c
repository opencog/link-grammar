#include <stdlib.h>
#include <string.h>

#include "link-grammar/dict-common/dict-api.h"
#include "link-grammar/error.h"

#include "generator-utilities.h"

#define SUBSCRIPT_MARK '\3'
#define SUBSCRIPT_DOT '.'
#define MAX_WORD 180

static const char *cond_subscript(const char *ow, bool leave_subscript)
{
	const char *sm = strchr(ow, SUBSCRIPT_MARK);
	if (sm == NULL) return ow;

	static char w[MAX_WORD + 1]; // XXX FIXME this is not thread-safe!
	strcpy(w, ow);
	w[sm - ow] = leave_subscript ? SUBSCRIPT_DOT : '\0';
	return w;
}

static void print_sent(size_t nwords, const char** words,
                       bool subscript)
{
	for(WordIdx w = 0; w < nwords; w++)
	{
		assert(NULL != words[w], "Failed to select word!");
		printf("%s", cond_subscript(words[w], subscript));
		if (w < nwords-1) printf(" ");
	}

	printf("\n");
}

/* Recursive sentence odometer */
static void sent_odom(const Category* catlist,
                      Linkage linkage, size_t nwords, const char** words,
                      bool subscript,
                      const Category_cost** cclist,
                      unsigned int* cclen,
                      const char** selected_words,
                      WordIdx cur_word)
{
	if (cur_word >= nwords)
	{
		print_sent(nwords, selected_words, subscript);
		return;
	}

	const Category_cost *cc = cclist[cur_word];
	if (cc == NULL)
	{
		selected_words[cur_word] = words[cur_word];
		sent_odom(catlist, linkage, nwords, words, subscript,
		          cclist, cclen, selected_words, cur_word+1);
	}
	else
	{
		/* Loop over all categories at the current odometer position. */
		for (unsigned int catidx = 0; catidx < cclen[cur_word]; catidx++)
		{
			/* Subtract 1 because there isn't any "category zero" */
			unsigned int catno = cc[catidx].num - 1;
			unsigned int num_words = catlist[catno].num_words;

			/* Loop over all words for the selected category */
			for (unsigned int widx = 0; widx < num_words; widx++)
			{
				selected_words[cur_word] = catlist[catno].word[widx];

				sent_odom(catlist, linkage, nwords, words, subscript,
				          cclist, cclen, selected_words, cur_word+1);
			}
		}
	}
}

static void print_sent_all(const Category* catlist,
                           Linkage linkage, size_t nwords, const char** words,
                           bool subscript)
{
	const Category_cost* cclist[nwords];
	unsigned int cclen[nwords];
	const char* selected_words[nwords];

	for(WordIdx w = 0; w < nwords; w++)
	{
		const Category_cost* cc = linkage_get_categories(linkage, w);
		cclist[w] = cc;

		unsigned int dj_num_cats = 0;
		cclen[w] = 1;
		selected_words[w] = NULL;
		if (NULL != cclist[w])
		{
			/* Count the number of categories for this disjunct */
			while (cc[dj_num_cats].num != 0) dj_num_cats++;
			assert(dj_num_cats != 0, "Bad disjunct!");

			cclen[w] = dj_num_cats;
		}
	}

	sent_odom(catlist, linkage, nwords, words, subscript,
	          cclist, cclen, selected_words, 0);
}

static const char *select_random_word(const Category *catlist,
                                      const Category_cost *cc,
                                      WordIdx w)
{
	/* Count the number of categories for this disjunct */
	unsigned int dj_num_cats = 0;
	while (cc[dj_num_cats].num != 0)
	   dj_num_cats++;

	assert(dj_num_cats != 0, "Bad disjunct!");

	/* Select a category on this disjunct. */
	unsigned int r = (unsigned int)rand();
	unsigned int catidx = r % dj_num_cats;

	/* Subtract 1 because Category 0 is undefined. */
	unsigned int catnum = cc[catidx].num - 1;
	lgdebug(5, "Word %zu: r=%08x category %d/%u \"%u\";", w, r,
	        catidx, dj_num_cats, catnum);
	unsigned int num_words = catlist[catnum].num_words;

	/* Select a dictionary word from the selected disjunct category. */
	r = (unsigned int)rand();
	unsigned int dict_word_idx = r % num_words;
	const char *word = catlist[catnum].word[dict_word_idx];
	lgdebug(5, " r=%08x word %d/%u \"%s\"\n",
	        r, dict_word_idx, num_words, word);

	return word;
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
	const char* selected_words[nwords];

	for(WordIdx w = 0; w < nwords; w++)
	{
		const Category_cost* cc = linkage_get_categories(linkage, w);
		if (cc == NULL)
		{
			printf("%s", cond_subscript(words[w], subscript));
		}
		else
		{
			const char *word = select_random_word(catlist, cc, w);
			printf("%s", cond_subscript(word, subscript));
		}
		if (w < nwords-1) printf(" ");
	}
	printf("\n");
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
