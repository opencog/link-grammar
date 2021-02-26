/*
 * link-generator.c
 *
 * Generate random corpora from dictionaries.
 * February 2021
 */

#include <stddef.h>

#include "../link-grammar/link-includes.h"

int main (int argc, char* argv[])
{
	const char     *language = NULL;
	Dictionary      dict;

	if ((argc > 1) && (argv[1][0] != '-')) {
		/* The dictionary is the first argument if it doesn't begin with "-" */
		language = argv[1];
	}

	return 0;
}
