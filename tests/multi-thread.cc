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
#include <atomic>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include "link-grammar/link-includes.h"

static std::atomic_int parse_count; // Just to validate that it can parse.

static void parse_one_sent(Dictionary dict, Parse_Options opts, const char *sent_str)
{
	Sentence sent = sentence_create(sent_str, dict);
	if (!sent) {
		fprintf (stderr, "Fatal error: Unable to create parser\n");
		exit(2);
	}
	sentence_split(sent, opts);
	int num_linkages = sentence_parse(sent, opts);
#if 0
	if (num_linkages <= 0) {
		fprintf (stderr, "Fatal error: Unable to parse sentence\n");
		exit(3);
	}
#endif
	if (0 < num_linkages)
	{
		parse_count++;
		if (10 < num_linkages) num_linkages = 10;

		for (int li = 0; li<num_linkages; li++)
		{
			Linkage linkage = linkage_create(li, sent, opts);

			// Short diagram, it wraps.
			char * str = linkage_print_diagram(linkage, true, 50);
			linkage_free_diagram(str);
			str = linkage_print_links_and_domains(linkage);
			linkage_free_links_and_domains(str);
			str = linkage_print_disjuncts(linkage);
			linkage_free_disjuncts(str);
			str = linkage_print_constituent_tree(linkage, SINGLE_LINE);
			linkage_free_constituent_tree_str(str);
			linkage_delete(linkage);
		}
	}
	sentence_delete(sent);
}

static void parse_sents(Dictionary dict, Parse_Options opts, int thread_id, int niter)
{
	const char *sents[] = {
		"Frank felt vindicated when his long time friend Bill revealed that he was the winner of the competition.",
		"Logorrhea, or excessive and often incoherent talkativeness or wordiness, is a social disease.",
		"It was covered with bites.",
		"I have no idea what that is.",
		"His shout had been involuntary, something anybody might have done.",
		"Trump, Ryan and McConnell are using the budget process to pay for the GOP’s $1.5 trillion tax scam.",
		"We ate popcorn and watched movies on TV for three days.",
		"Sweat stood on his brow, fury was bright in his one good eye.",
		"One of the things you do when you stop your bicycle is apply the brake.",
		"The line extends 10 miles offshore.",

		// The English parser will choke on this.
		"под броню боевого робота устремились потоки энергии.",
		"через четверть часа здесь будет полно полицейских.",

		// Rabindranath Tagore
		"習近平: 堅守 實體經濟落實高品 質發展",
		"文在寅希望半島對話氛 圍在平昌冬奧會後能延續",
		"土耳其出兵 搶先機 美土俄棋局怎麼走?",
		"默克爾努力獲突破 德社民黨同意開展組閣談判"

		// Thai test sentences from corpus.batch
		"ตำรวจ กิน ข้าว",
		"ตำรวจ นาย หนึ่ง ซื้อ เสื้อ ตัว หนึ่ง",
		"ฉัน ไป ตลาด ซื้อ ข้าว มา",
		"ฉัน เดิน ไป ตลาด ซื้อ ข้าว มา กิน"
	};

	int nsents = sizeof(sents) / sizeof(const char *);

#define WID 120
#define LIN 4
	char junk[LIN*WID];
	for (int ln=0; ln<LIN; ln++)
	{
		char *line = &junk[ln*WID];
		for (int w = 0; w < WID; w++)
		{
			// Ugly junk including stuff that should be
			// bad/invalid UTF-8 character sequences.
			line[w] = ((thread_id+1)*w+1)%30 + (31*ln ^ 0x66);
			if (30<w) line[w] += 0x7f;
			if (60<w) line[w] += 0x50;
			if (90<w) line[w] = line[w-90] ^ line[w-30];
			if (0 == w%25) line[w] = ' ';
			if (0 == w%23) line[w] = ' ';
			if (0 == w%15) line[w] = ' ';
			if (0 == w%11) line[w] = ' ';
		}
		line[WID-1] = 0x0;
	}

	for (int j=0; j<niter; j += (nsents+LIN))
	{
		for (int i=0; i < nsents; ++i)
		{
			parse_one_sent(dict, opts, sents[i]);
		}
		for (int ln=0; ln<LIN; ln++)
		{
			char *line = &junk[ln*WID];
			parse_one_sent(dict, opts, line);
		}
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "en_US.UTF-8");

	dictionary_set_data_dir(DICTIONARY_DIR "/data");
	Dictionary dicte = dictionary_create_lang("en");
	Dictionary dictr = dictionary_create_lang("ru");
	Dictionary dicth = dictionary_create_lang("th");
	if (!dicte or !dictr) {
		fprintf (stderr, "Fatal error: Unable to open the dictionary\n");
		exit(1);
	}

	const int n_threads = 10;
	const int niter = 500;
	Parse_Options opts[n_threads];

	printf("Creating %d threads, each parsing %d sentences\n",
		 n_threads, niter);
	std::vector<std::thread> thread_pool;
	for (int i=0; i < n_threads; i++)
	{
		Dictionary dict = dicte;
		opts[i] = parse_options_create();
		if (1 == i%3)
		{
			dict = dictr; // Russian
			parse_options_set_spell_guess(opts[i], 0);
		}

		if (2 == i%3)
		{
			dict = dicth;  // Thai
			parse_options_set_spell_guess(opts[i], 0);
		}

		thread_pool.push_back(std::thread(parse_sents, dict, opts[i], i, niter));
	}

	// Wait for all threads to complete
	for (std::thread& t : thread_pool) t.join();
	const int pcnt = parse_count;
	if (0 == pcnt)
	{
		printf("Fatal error: Nothing got parsed\n");
		exit(2);
	}
	printf("Done with multi-threaded parsing (stat: %d full parses)\n", pcnt);


	for (int i=0; i < n_threads; i++)
		parse_options_delete(opts[i]);

	dictionary_delete(dicte);
	dictionary_delete(dictr);
	dictionary_delete(dicth);
	return 0;
}
