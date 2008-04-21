/*
 * Java JNI interfaces.
 *
 * This implements a very simple, low-brow, non-OOP interface.
 * It could be improved.
 */

#include <jni.h>
#include <stdio.h>
#include <utilities.h>

#include <link-grammar/api.h>
#include "jni-client.h"

static Dictionary    dict;
static Parse_Options opts, panic_parse_opts;
static Sentence      sent=NULL;
static Linkage       linkage=NULL;
static int           num_linkages, cur_linkage;
#if DO_PHRASE_TREE
CNode*        tree;
#endif

static void setup_panic_parse_options(Parse_Options opts)
{
	parse_options_set_disjunct_cost(opts, 3);
	parse_options_set_min_null_count(opts, 1);
	parse_options_set_max_null_count(opts, MAX_SENTENCE);
	parse_options_set_max_parse_time(opts, 60);
	parse_options_set_islands_ok(opts, 1);
	parse_options_set_short_length(opts, 6);
	parse_options_set_all_short_connectors(opts, 1);
	parse_options_set_linkage_limit(opts, 100);
	parse_options_set_verbosity(opts,0);
}

static void test(void)
{
#ifdef DEBUG
	printf("%d\n",word_contains("said",PAST_TENSE_FORM_MARKER,dict));
	printf("%d\n",word_contains("gave.v",PAST_TENSE_FORM_MARKER,dict));
	printf("%d\n",word_contains("have",PAST_TENSE_FORM_MARKER,dict));
	printf("%d\n",word_contains("had",PAST_TENSE_FORM_MARKER,dict));
#endif
}

static void init(void)
{
	panic_parse_opts = parse_options_create();
	opts	= parse_options_create();
	setup_panic_parse_options(panic_parse_opts);
	parse_options_set_disjunct_cost(opts, 3);
	parse_options_set_max_sentence_length(opts, 70);
	parse_options_set_panic_mode(opts, TRUE);
	parse_options_set_max_parse_time(opts, 30);
	parse_options_set_linkage_limit(opts, 1000);
	parse_options_set_short_length(opts, 10);
	parse_options_set_verbosity(opts,0);

	/* Default to the english language; will need to fix 
	 * this if/when more languages are supported.
	 */
	dict = dictionary_create_lang("en");
	test();
}

#ifdef DEBUG_DO_PHRASE_TREE
static void r_printTree(CNode* cn, int level)
{
	int i;
	CNode* c;

	if (cn==NULL) return;

	/* print label */
	if (cn->label != NULL) {
		printf("(%s ", cn->label);
	} else {
		printf("NULL\n");
	}

	/* Recurse on children. */
	for (c = cn->child; c!=NULL; c=c->next) {
		if (c->child)
			r_printTree(c, level+1);
		else
			printf("%s ", c->label);
	}
	printf(")\n");
	for (i=0; i<=level; i++)
		printf ("   ");
}

static void printTree(CNode* cn)
{
	r_printTree(cn,0);
	printf("\n");
}
#endif /* DEBUG */

static void jParse(char* inputString)
{
	int jverbosity = parse_options_get_verbosity(opts);
	if (sent)
		sentence_delete(sent);

	sent = sentence_create(inputString, dict);
	num_linkages=0;

	if (sent == NULL)
		return;

	if (sentence_length(sent) > parse_options_get_max_sentence_length(opts))
	{
		sentence_delete(sent);
		sent = NULL;
		if (jverbosity > 0) {
			fprintf(stdout,
				"Sentence length (%d words) exceeds maximum allowable (%d words)\n",
				sentence_length(sent), parse_options_get_max_sentence_length(opts));
		}
		return;
	}

	/* First parse with cost 0 or 1 and no null links */
	parse_options_set_disjunct_cost(opts, 2);
	parse_options_set_min_null_count(opts, 0);
	parse_options_set_max_null_count(opts, 0);
	parse_options_reset_resources(opts);

	num_linkages = sentence_parse(sent, opts);

	/* Now parse with null links */
	if ((num_linkages == 0) && (!parse_options_get_batch_mode(opts))) {
		if (jverbosity > 0) fprintf(stdout, "No complete linkages found.\n");
		if (parse_options_get_allow_null(opts)) {
			parse_options_set_min_null_count(opts, 1);
			parse_options_set_max_null_count(opts, sentence_length(sent));
			num_linkages = sentence_parse(sent, opts);
		}
	}

	if (parse_options_timer_expired(opts)) {
		if (jverbosity > 0) fprintf(stdout, "Timer is expired!\n");
	}
	if (parse_options_memory_exhausted(opts)) {
		if (jverbosity > 0) fprintf(stdout, "Memory is exhausted!\n");
	}

	if ((num_linkages == 0) &&
			parse_options_resources_exhausted(opts) &&
			parse_options_get_panic_mode(opts))
	{
		print_total_time(opts);
		if (jverbosity > 0) fprintf(stdout, "Entering \"panic\" mode...\n");
		parse_options_reset_resources(panic_parse_opts);
		parse_options_set_verbosity(panic_parse_opts, jverbosity);
		num_linkages = sentence_parse(sent, panic_parse_opts);
		if (parse_options_timer_expired(panic_parse_opts)) {
			if (jverbosity > 0) fprintf(stdout, "Timer is expired!\n");
		}
	}
}

static void makeLinkage(int i)
{
	if (i<num_linkages) {
		if (linkage)
			linkage_delete(linkage);
		linkage = linkage_create(i,sent,opts);
		linkage_compute_union(linkage);
		linkage_set_current_sublinkage(linkage,linkage_get_num_sublinkages(linkage)-1);

#if DO_PHRASE_TREE
		if (tree)
			linkage_free_constituent_tree(tree);
		tree = linkage_constituent_tree(linkage);
		printTree(tree);
#endif
	}
}

static void finish(void)
{
	if (sent)
		sentence_delete(sent);
	sent = NULL;

#if DO_PHRASE_TREE
	if (tree)
		linkage_free_constituent_tree(tree);
	tree = NULL;
#endif

	if (linkage)
		linkage_delete(linkage);
	linkage = NULL;

	dictionary_delete(dict);
	dict = NULL;

	parse_options_delete(opts);
	opts = NULL;

	parse_options_delete(panic_parse_opts);
	panic_parse_opts = NULL;
}

/* =========================================================================== */
/* Java JNI wrappers */

JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_setMaxParseSeconds(JNIEnv *env, jclass cls, jint maxParseSeconds)
{
	parse_options_set_max_parse_time(opts, maxParseSeconds);
}

JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_setMaxCost(JNIEnv *env, jclass cls, jint maxCost)
{
	parse_options_set_disjunct_cost(opts, maxCost);
}

JNIEXPORT void JNICALL 
Java_org_linkgrammar_LinkGrammar_setDictionariesPath
  (JNIEnv *env, jclass cls, jstring path)
{
    const char *nativePath = (*env)->GetStringUTFChars(env,path, 0);
    set_data_dir(nativePath);
    (*env)->ReleaseStringUTFChars(env,path, nativePath);
}

/*
 * Class:		 LinkGrammar
 * Method:		init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_init(JNIEnv *env, jclass cls)
{
	init();
}

/*
 * Class:		 LinkGrammar
 * Method:		parse
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_parse(JNIEnv *env, jclass cls, jstring str)
{
	const char *cStr = (*env)->GetStringUTFChars(env,str,0);
	char* tmp = strdup(cStr);
	jParse(tmp);
	free(tmp);
	(*env)->ReleaseStringUTFChars(env,str,cStr);
}

/*
 * Class:		 LinkGrammar
 * Method:		close
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_close(JNIEnv *env, jclass cls)
{
	finish();
}

/*
 * Class:		 LinkGrammar
 * Method:		numWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getNumWords(JNIEnv *env, jclass cls)
{
	return linkage_get_num_words(linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		getWord
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_linkgrammar_LinkGrammar_getWord(JNIEnv *env, jclass cls, jint i)
{
	char* w = sentence_get_word(sent, i); /* does not need to be freed, points into sentence */
	jstring j = (*env)->NewStringUTF(env, w);
	return j;
}

/*
 * Class:		 LinkGrammar
 * Method:		numSkippedWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getNumSkippedWords(JNIEnv *env, jclass cls)
{
	return sentence_null_count(sent);
}

/*
 * Class:		 LinkGrammar
 * Method:		numLinkages
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getNumLinkages(JNIEnv *env, jclass cls)
{
	return sentence_num_valid_linkages(sent);
}

/*
 * Class:		 LinkGrammar
 * Method:		makeLinkage
 * Signature: (I)I
 */
JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_makeLinkage(JNIEnv *env, jclass cls, jint i)
{
	cur_linkage = i;
	makeLinkage(cur_linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		linkageNumViolations
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkageNumViolations(JNIEnv *env, jclass cls)
{
	return sentence_num_violations(sent, cur_linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		linkageAndCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkageAndCost(JNIEnv *env, jclass cls)
{
	return sentence_and_cost(sent, cur_linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		linkageDisjunctCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkageDisjunctCost(JNIEnv *env, jclass cls)
{
	return sentence_disjunct_cost(sent, cur_linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		linkageLinkCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkageLinkCost(JNIEnv *env, jclass cls)
{
	return sentence_link_cost(sent, cur_linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		getNumLinks
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getNumLinks(JNIEnv *env, jclass cls)
{
	return linkage_get_num_links(linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		getLinkLWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkLWord(JNIEnv *env, jclass cls, jint i)
{
	return linkage_get_link_lword(linkage, i);
}

/*
 * Class:		 LinkGrammar
 * Method:		getLinkRWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkRWord(JNIEnv *env, jclass cls, jint i)
{
	return linkage_get_link_rword(linkage, i);
}

/*
 * Class:		 LinkGrammar
 * Method:		getLinkLLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkLLabel(JNIEnv *env, jclass cls, jint i)
{
 	/* Does not need to be freed, points into linkage */
	const char *s = linkage_get_link_llabel(linkage, i);
	jstring j = (*env)->NewStringUTF(env, s);
	return j;
}

/*
 * Class:		 LinkGrammar
 * Method:		getLinkRLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkRLabel(JNIEnv *env, jclass cls, jint i)
{
 	/* Does not need to be freed, points into linkage */
	const char *s = linkage_get_link_rlabel(linkage, i);
	jstring j = (*env)->NewStringUTF(env, s);
	return j;
}

/*
 * Class:		 LinkGrammar
 * Method:		getLinkLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkLabel(JNIEnv *env, jclass cls, jint i)
{
 	/* Does not need to be freed, points into linkage */
	const char *s = linkage_get_link_label(linkage, i);
	jstring j = (*env)->NewStringUTF(env, s);
	return j;
}

/*
 * Class:		 LinkGrammar
 * Method:		constituentString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_linkgrammar_LinkGrammar_getConstituentString(JNIEnv *env, jclass cls)
{
	/* mode 1 prints a lisp-style string, nicely indented.
	 * mode 2 prints a lisp-style string, but with square brackets.
	 * mode 3 prints a lisp-style string, one one single line.
	 */
	/* char *s = linkage_print_constituent_tree(linkage, 1); */
	char *s = linkage_print_constituent_tree(linkage, 3);
	jstring j = (*env)->NewStringUTF(env, s);
	linkage_free_constituent_tree_str(s);
	return j;
}

/*
 * Class:		 LinkGrammar
 * Method:		linkString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkString(JNIEnv *env, jclass cls)
{
	char *s = linkage_print_diagram(linkage);
	jstring j = (*env)->NewStringUTF(env, s);
	linkage_free_diagram(s);
	return j;
}

/*
 * Class:		 LinkParser
 * Method:		isPastTenseForm
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
Java_org_linkgrammar_LinkGrammar_isPastTenseForm(JNIEnv *env, jclass cls, jstring str)
{
	const char *cStr = (*env)->GetStringUTFChars(env,str,0);
	if (is_past_tense_form(cStr,dict) == 1)
		return TRUE;
	return FALSE;
}

/*
 * Class:		 LinkParser
 * Method:		isEntity
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
Java_org_linkgrammar_LinkGrammar_isEntity(JNIEnv *env, jclass cls, jstring str)
{
	const char *cStr = (*env)->GetStringUTFChars(env,str,0);
	if (is_entity(cStr,dict) == 1)
		return TRUE;
	return FALSE;
}
