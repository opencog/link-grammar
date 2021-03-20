#include <string.h>

#include "link-grammar/dict-common/dict-api.h"

#include "generator-utilities.h"

void dump_categories(Dictionary dict, const Category *category)
{
#define TAB "   "
	const int maxcol = 60; /* Account for initial formatting tabs. */

	printf(TAB"\"categories\": [\n");
	for (unsigned int n = 0; category[n].num_words != 0; n++)
	{
		printf(TAB"\{\n");
		if (category[n].category_name != NULL)
			printf(TAB TAB"\"category_name\": %s,\n", category[n].category_name);
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
