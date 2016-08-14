/***************************************************************************/
/* Copyright (c) 2014 Linas Vepstas                                        */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software.      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

// This implements a very simple-minded multi-threaded unit test.
// All it does is to make sure the system doesn't crash e.g. due to
// memory allocation conflicts.

#include <thread>
#include <vector>

#include <locale.h>
#include <stdio.h>
#include <fcntl.h>
#include "link-grammar/link-includes.h"

static void parse_one_sent(Dictionary dict, Parse_Options opts, const char *sent_str)
{
	Sentence sent = sentence_create(sent_str, dict);
	sentence_split(sent, opts);
	int num_linkages = sentence_parse(sent, opts);
	if (0 < num_linkages)
	{
		if (10 < num_linkages) num_linkages = 10;

		for (int li = 0; li<num_linkages; li++)
		{
			Linkage linkage = linkage_create(li, sent, opts);
			char * str = linkage_print_diagram(linkage, true, 80);
			printf("%s: %s\n%s\n", dictionary_get_lang(dict), sent_str, str);
			linkage_free_diagram(str);
			str = linkage_print_links_and_domains(linkage);
			linkage_free_links_and_domains(str);
			str = linkage_print_disjuncts(linkage);
			linkage_free_disjuncts(str);
			str = linkage_print_constituent_tree(linkage, SINGLE_LINE);
			linkage_free_constituent_tree_str(str);
			linkage_delete(linkage);
			fflush(stdout);
		}
	}
	sentence_delete(sent);
}

static void parse_sents(Dictionary dict, int dictnum, Parse_Options opts, int thread_id)
{
	const char *sent_en[] = {
		"Frank felt vindicated when his long time friend Bill revealed that he was the winner of the competition.",
		"Logorrhea, or excessive and often incoherent talkativeness or wordiness, is a social disease.",
		"Is it fine?",
		"It was covered with bites.",
		"I have no idea what that is.",
		"His shout had been involuntary, something anybody might have done.",
		"Is it fine?",
		"He obtained the lease of the manor of Great Burstead Grange (near East Horndon) from the Abbey of Stratford Langthorne, and purchased the manor of Bayhouse in West Thurrock.",
		"We ate popcorn and watched movies on TV for three days.",
		"Is it fine?",
		"Sweat stood on his brow, fury was bright in his one good eye.",
		"One of the things you do when you stop your bicycle is apply the brake.",
		"The line extends 10 miles offshore.",
		"Is it fine?",
		NULL
	};
	const char *sent_ru[] = {
		"под броню боевого робота устремились потоки энергии.",
		"Она ушла",
		"он был хороший доктор",
		"он был доктор",
		"среди собравшихся прокатился ропот удивления",
		"осталось снять две последние группы",
		"Потолок зала поддерживали металлические колонны.",
		"через четверть часа здесь будет полно полицейских.",
		NULL
	};
	const char *sent_tr[] = {
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		"Senin ne istediğini bilmiyorum",
		NULL
	};

	const char **sent = dictnum==1 ? sent_en : (dictnum==2 ? sent_ru :sent_tr);
	printf("DICTNUM %d\n", dictnum);
	fflush(stdout);

	for (int i=0; sent[i] != NULL; ++i)
	{
		parse_one_sent(dict, opts, sent[i]);
	}
}

int main(int argc, char* argv[])
{

	setlocale(LC_ALL, "en_US.UTF-8");
	Parse_Options opts = parse_options_create();
	parse_options_set_verbosity(opts, 3);
	parse_options_set_islands_ok(opts, 1);
	parse_options_set_max_null_count(opts, 1);

	Dictionary dict_en = dictionary_create_lang("en");
	if (!dict_en) {
		printf ("Fatal error: Unable to open the \"en\" dictionary\n");
		return 1;
	}
	Dictionary dict_ru = dictionary_create_lang("ru");
	if (!dict_ru) {
		printf ("Fatal error: Unable to open the \"ru\" dictionary\n");
		return 1;
	}
	Dictionary dict_tr = dictionary_create_lang("tr");
	if (!dict_tr) {
		printf ("Fatal error: Unable to open the \"tr\" dictionary\n");
		return 1;
	}

	//fcntl(1, F_SETFL, O_APPEND);
	std::vector<std::thread> thread_pool;
	thread_pool.push_back(std::thread(parse_sents, dict_en, 1, opts, 1));
	thread_pool.push_back(std::thread(parse_sents, dict_ru, 2, opts, 2));
	thread_pool.push_back(std::thread(parse_sents, dict_tr, 3, opts, 3));
	thread_pool.push_back(std::thread(parse_sents, dict_ru, 2, opts, 2));
	thread_pool.push_back(std::thread(parse_sents, dict_en, 1, opts, 1));

	// Wait for all threads to complete
	for (std::thread& t : thread_pool) t.join();
	printf("Done with multi-threaded parsing\n");

	dictionary_delete(dict_en);
	dictionary_delete(dict_ru);
	dictionary_delete(dict_tr);
	parse_options_delete(opts);
	return 0;
}
