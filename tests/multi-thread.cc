

#include <thread>
#include <vector>

#include <locale.h>
#include <stdio.h>
#include "link-grammar/link-includes.h"

static void parse_sents(Dictionary dict, Parse_Options opts, int thread_id, int niter)
{
	const char *sents[] = {
		"Frank felt vindicated when his long time friend Bill revealed that he was the winner of the competition.",
		"Logorrhea, or excessive and often incoherent talkativeness or wordiness, is a social disease.",
		"He obtained the lease of the manor of Great Burstead Grange (near East Horndon) from the Abbey of Stratford Langthorne, and purchased the manor of Bayhouse in West Thurrock.",
		"We ate popcorn and watched movies on TV for three days.",
		"Sweat stood on his brow, fury was bright in his one good eye.",
		"One of the things you do when you stop your bicycle is apply the brake."
// "под броню боевого робота устремились потоки энергии.",
// "через четверть часа здесь будет полно полицейских."
	};

	int nsents = sizeof(sents) / sizeof(const char *);
	for (int i=0; i < nsents; ++i)
	{
		Sentence sent = sentence_create(sents[i], dict);
		sentence_split(sent, opts);
		int num_linkages = sentence_parse(sent, opts);
		if (num_linkages > 0) {
			Linkage linkage = linkage_create(0, sent, opts);
			char * diagram = linkage_print_diagram(linkage, true, 800);
			linkage_free_diagram(diagram);
			linkage_delete(linkage);
		}
		sentence_delete(sent);
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "en_US.UTF-8");
	Parse_Options opts = parse_options_create();
	// Dictionary dict = dictionary_create_lang("ru");
	Dictionary dict = dictionary_create_lang("en");
	if (!dict) {
		printf ("Fatal error: Unable to open the dictionary\n");
		return 1;
	}

	int n_threads = 10;
	int niter = 100;
	std::vector<std::thread> thread_pool;
	for (int i=0; i < n_threads; i++) {
		thread_pool.push_back(std::thread(parse_sents, dict, opts, i, niter));
	}

	// Wait for all threads to complete
	for (std::thread& t : thread_pool) t.join();

	dictionary_delete(dict);
	parse_options_delete(opts);
	return 0;
}
