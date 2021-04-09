/* link-grammar Vala Bindings
 * Copyright 2021 Miles Wallio <mwallio@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

[CCode (cheader_filename = "link-grammar/link-includes.h")]
namespace LinkGrammar {

	[CCode (cname = "Dictionary", cprefix = "dictionary_", free_function = "dictionary_delete")]
	[Compact]
	public class Dictionary {
		[CCode (cname = "dictionary_create_lang")]
		public Dictionary (string language);
		[CCode (cname = "dictionary_create_lang")]
		public Dictionary.create_lang (string language);
		[CCode (cname = "dictionary_get_lang")]
		public string get_lang ();
		[CCode (cname = "dictionary_set_data_dir")]
		public void set_data_dir (string path);
		[CCode (cname = "dictionary_get_data_dir")]
		public unowned string get_data_dir ();
	}

	[CCode (cname = "Sentence", cprefix = "sentence_", free_function = "sentence_delete")]
	[Compact]
	public class Sentence {
		[CCode (cname = "sentence_create")]
		public Sentence (string input_string, Dictionary dict);
		[CCode (cname = "sentence_create")]
		public Sentence.create (string input_string, Dictionary dict);
		[CCode (cname = "sentence_split")]
		public int split (ParseOptions opts);
		[CCode (cname = "sentence_parse")]
		public int parse (ParseOptions opts);
		[CCode (cname = "sentence_length")]
		public int length ();
		[CCode (cname = "sentence_null_count")]
		public int null_count ();
		[CCode (cname = "sentence_num_linkages_found")]
		public int num_linkages_found ();
		[CCode (cname = "sentence_num_valid_linkages")]
		public int num_valid_linkages ();
		[CCode (cname = "sentence_num_linkages_post_processed")]
		public int num_linkages_post_processed ();
		[CCode (cname = "sentence_num_violations")]
		public int num_violations (LinkageIdx linkage_num);
		[CCode (cname = "sentence_disjunct_cost")]
		public double disjunct_cost (LinkageIdx linkage_num);
		[CCode (cname = "sentence_link_cost")]
		public int link_cost (LinkageIdx linkage_num);
		[CCode (cname = "sentence_display_wordgraph")]
		public bool display_wordgraph (string modestr);
	}

	[SimpleType]
	[CCode (cname = "WordIdx", has_type_id = false)]
	public struct WordIdx : size_t {
	}

	[SimpleType]
	[CCode (cname = "LinkIdx", has_type_id = false)]
	public struct LinkIdx : size_t {
	}

	[SimpleType]
	[CCode (cname = "LinkageIdx", has_type_id = false)]
	public struct LinkageIdx : size_t {
	}

	[CCode (cname = "Linkage", cprefix = "linkage_", free_function = "linkage_delete")]
	[Compact]
	public class Linkage {
		[CCode (cname = "linkage_create")]
		public Linkage (LinkageIdx linkage_num, Sentence sent, ParseOptions opts);
		[CCode (cname = "linkage_create")]
		public Linkage.create (LinkageIdx linkage_num, Sentence sent, ParseOptions opts);
		[CCode (cname = "linkage_get_num_words")]
		public size_t get_num_words ();
		[CCode (cname = "linkage_get_num_links")]
		public size_t get_num_links ();
		[CCode (cname = "linkage_get_link_lword")]
		public WordIdx get_link_lword (LinkIdx index);
		[CCode (cname = "linkage_get_link_rword")]
		public WordIdx get_link_rword (LinkIdx index);
		[CCode (cname = "linkage_get_link_length")]
		public int get_link_length (LinkIdx index);
		[CCode (cname = "linkage_get_link_label")]
		public unowned string get_link_label (LinkIdx index);
		[CCode (cname = "linkage_get_link_llabel")]
		public unowned string get_link_llabel (LinkIdx index);
		[CCode (cname = "linkage_get_link_rlabel")]
		public unowned string get_link_rlabel (LinkIdx index);
		[CCode (cname = "linkage_get_link_num_domains")]
		public int get_link_num_domains (LinkIdx index);
		[CCode (cname = "linkage_get_link_domain_names", array_length = false, array_null_terminated = true)]
		public unowned string[] get_link_domain_names (LinkIdx index);
		[CCode (cname = "linkage_get_words", array_length = false, array_null_terminated = true)]
		public unowned string[] get_words ();
		[CCode (cname = "linkage_get_disjunct_str")]
		public string get_disjunct_str (WordIdx word_num);
		[CCode (cname = "linkage_get_disjunct_cost")]
		public double get_disjunct_cost (WordIdx word_num);
		[CCode (cname = "linkage_get_word")]
		public unowned string get_word (WordIdx word_num);
		[CCode (cname = "linkage_print_constituent_tree")]
		public string print_constituent_tree (ConstituentDisplayStyle mode);
		// public static void free_constituent_tree_str (string str);
		[CCode (cname = "linkage_print_diagram")]
		public string print_diagram (bool display_walls, size_t screen_width);
		// public static void free_diagram (string str);
		[CCode (cname = "linkage_print_postscript")]
		public string print_postscript (bool display_walls, bool print_ps_header);
		// public static void free_postscript (string str);
		[CCode (cname = "linkage_print_disjuncts")]
		public string print_disjuncts ();
		// public static void free_disjuncts (string str);
		[CCode (cname = "linkage_print_links_and_domains")]
		public string print_links_and_domains ();
		// public static void free_links_and_domains (string str);
		[CCode (cname = "linkage_print_pp_msgs")]
		public string print_pp_msgs ();
		// public static void free_pp_msgs (string str);
		[CCode (cname = "linkage_unused_word_cost")]
		public int unused_word_cost ();
		[CCode (cname = "linkage_disjunct_cost")]
		public double disjunct_cost ();
		[CCode (cname = "linkage_link_cost")]
		public int link_cost ();
		[CCode (cname = "linkage_get_violation_name")]
		public string get_violation_name ();
	}

	[CCode (cname = "Parse_Options", cprefix = "parse_options_", free_function = "parse_options_delete")]
	[Compact]
	public class ParseOptions {
		[CCode (cname = "parse_options_create")]
		public ParseOptions ();
		[CCode (cname = "parse_options_create")]
		public ParseOptions.create ();
		[CCode (cname = "parse_options_set_verbosity")]
		public void set_verbosity (int verbosity);
		[CCode (cname = "parse_options_get_verbosity")]
		public int get_verbosity ();
		[CCode (cname = "parse_options_set_debug")]
		public void set_debug (string debug);
		[CCode (cname = "parse_options_get_debug")]
		public string get_debug ();
		[CCode (cname = "parse_options_set_test")]
		public void set_test (string test);
		[CCode (cname = "parse_options_get_test")]
		public string get_test ();
		[CCode (cname = "parse_options_set_linkage_limit")]
		public void set_linkage_limit (int linkage_limit);
		[CCode (cname = "parse_options_get_linkage_limit")]
		public int get_linkage_limit ();
		[CCode (cname = "parse_options_set_disjunct_cost")]
		public void set_disjunct_cost (double disjunct_cost);
		[CCode (cname = "parse_options_get_disjunct_cost")]
		public double get_disjunct_cost ();
		[CCode (cname = "parse_options_set_min_null_count")]
		public void set_min_null_count (int null_count);
		[CCode (cname = "parse_options_get_min_null_count")]
		public int get_min_null_count ();
		[CCode (cname = "parse_options_set_max_null_count")]
		public void set_max_null_count (int null_count);
		[CCode (cname = "parse_options_get_max_null_count")]
		public int get_max_null_count ();
		[CCode (cname = "parse_options_set_islands_ok")]
		public void set_islands_ok (bool islands_ok);
		[CCode (cname = "parse_options_get_islands_ok")]
		public bool get_islands_ok ();
		[CCode (cname = "parse_options_set_spell_guess")]
		public void set_spell_guess (int spell_guess);
		[CCode (cname = "parse_options_get_spell_guess")]
		public int get_spell_guess ();
		[CCode (cname = "parse_options_set_short_length")]
		public void set_short_length (int short_length);
		[CCode (cname = "parse_options_get_short_length")]
		public int get_short_length ();
		[CCode (cname = "parse_options_set_max_memory")]
		public void set_max_memory (int mem);
		[CCode (cname = "parse_options_get_max_memory")]
		public int get_max_memory ();
		[CCode (cname = "parse_options_set_max_parse_time")]
		public void set_max_parse_time (int secs);
		[CCode (cname = "parse_options_get_max_parse_time")]
		public int get_max_parse_time ();
		[CCode (cname = "parse_options_set_cost_model_type")]
		public void set_cost_model_type (CostModelType cm);
		[CCode (cname = "parse_options_get_cost_model_type")]
		public CostModelType get_cost_model_type ();
		[CCode (cname = "parse_options_set_perform_pp_prune")]
		public void set_perform_pp_prune (bool pp_prune);
		[CCode (cname = "parse_options_get_perform_pp_prune")]
		public bool get_perform_pp_prune ();
		[CCode (cname = "parse_options_set_use_sat_parser")]
		public void set_use_sat_parser (bool use_sat_solver);
		[CCode (cname = "parse_options_get_use_sat_parser")]
		public bool get_use_sat_parser ();
		[CCode (cname = "parse_options_timer_expired")]
		public bool timer_expired ();
		[CCode (cname = "parse_options_memory_exhausted")]
		public bool memory_exhausted ();
		[CCode (cname = "parse_options_resources_exhausted")]
		public bool resources_exhausted ();
		[CCode (cname = "parse_options_set_all_short_connectors")]
		public void set_all_short_connectors (bool val);
		[CCode (cname = "parse_options_get_all_short_connectors")]
		public bool get_all_short_connectors ();
		[CCode (cname = "parse_options_set_repeatable_rand")]
		public void set_repeatable_rand (bool val);
		[CCode (cname = "parse_options_get_repeatable_rand")]
		public bool get_repeatable_rand ();
		[CCode (cname = "parse_options_parse_options_get_dialect")]
		public string get_dialect ();
		[CCode (cname = "parse_options_parse_options_set_dialect")]
		public void set_dialect (string dialect);
		[CCode (cname = "parse_options_parse_options_get_display_morphology")]
		public string get_display_morphology ();
		[CCode (cname = "parse_options_parse_options_set_display_morphology")]
		public void set_display_morphology (int val);
		[CCode (cname = "parse_options_reset_resources")]
		public void reset_resources ();
	}

	[Flags]
	[CCode (cname = "Cost_Model_type", cprefix = "")]
	public enum CostModelType {
		VDAL
	}

	[Flags]
	[CCode (cname = "lg_error_severity", cprefix = "lg_")]
	public enum ErrorSeverity {
		Fatal,
		Error,
		Warn,
		Info,
		Debug,
		Trace,
		None
	}

	[Flags]
	[CCode (cname = "ConstituentDisplayStyle")]
	public enum ConstituentDisplayStyle {
		NO_DISPLAY,
		MULTILINE,
		BRACKET_TREE,
		SINGLE_LINE,
		MAX_STYLES
	}
}