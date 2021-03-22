#include <stdlib.h>
#include <string.h>

#include "link-grammar/dict-common/dict-api.h"
#include "link-grammar/error.h"

#include "generator-utilities.h"

#define SUBSCRIPT_MARK '\3'
#define SUBSCRIPT_DOT '.'
#define MAX_WORD 180

const char *select_word(const Category *category, const Category_cost *cc,
								WordIdx w)
{
	unsigned int disjunct_num_categories;
	for (disjunct_num_categories = 0; cc[disjunct_num_categories].num != 0;
	   disjunct_num_categories++)
		;
	if (disjunct_num_categories == 0) return "BAD_DISJUNCT";

	/* Select a disjunct category. */
	unsigned int r = (unsigned int)rand();
	unsigned int disjunct_category_idx = r % disjunct_num_categories;
	unsigned int category_num = cc[disjunct_category_idx].num;
	lgdebug(5, "Word %zu: r=%08x category %d/%u \"%u\";", w, r,
	        disjunct_category_idx, disjunct_num_categories, category_num);
	category_num--; /* Categories starts with 1 but Category is 0 based. */
	unsigned int num_words = category[category_num].num_words;

	/* Select a dictionary word from the selected disjunct category. */
	r = (unsigned int)rand();
	unsigned int dict_word_idx = r % num_words;
	const char *word = category[category_num].word[dict_word_idx];
	lgdebug(5, " r=%08x word %d/%u \"%s\"\n",
	        r, dict_word_idx, num_words, word);

	return word;
}

const char *cond_subscript(const char *ow, bool leave_subscript)
{
	const char *sm = strchr(ow, SUBSCRIPT_MARK);
	if (sm == NULL) return ow;

	static char w[MAX_WORD + 1]; // XXX this is not thread-safe!
	strcpy(w, ow);
	w[sm - ow] = leave_subscript ? SUBSCRIPT_DOT : '\0';
	return w;
}

void dump_categories(Dictionary dict, const Category *category)
{
#define TAB "   "
	const int maxcol = 60; /* Account for initial formatting tabs. */

	printf(TAB"\"categories\": [\n");
	for (unsigned int n = 0; category[n].num_words != 0; n++)
	{
		printf(TAB"\{\n");
		if (category[n].name[0] != '\0')
			printf(TAB TAB"\"name\": %s,\n", category[n].name);
		printf(TAB TAB"\"category_num\": %u,\n", n + 1);
		printf(TAB TAB"\"exp\": %s,\n", lg_exp_stringify(category[n].exp));
		printf(TAB TAB"\"num_words\": %u,\n", category[n].num_words);
		printf(TAB TAB"\"words\": [\n");
		printf(TAB TAB TAB);

		int currcol = 0;
		for (unsigned int wi = 0; wi < category[n].num_words; wi++)
		{
			const char *word = category[n].word[wi];
			if ((currcol > 0) && currcol + (int)strlen(word) > maxcol)
			{
				if (wi != category[n].num_words - 1) printf("\n"TAB TAB TAB);
				currcol = 0;
			}
			currcol += printf("\"%s\"", word); /* No crash if printf() fails. */
			if (wi != category[n].num_words - 1)
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
		if (category[n+1].num_words != 0)
			printf(",");
		printf("\n");
	}
	printf(TAB"]\n");
	printf("}\n");
}
