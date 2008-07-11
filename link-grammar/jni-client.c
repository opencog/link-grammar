/*
 * Java JNI interfaces.
 *
 * This implements a very simple, low-brow, non-OOP interface.
 * It could be improved.
 */

#include <jni.h>
#include <locale.h>
#include <stdio.h>

#include "api.h"
#include "jni-client.h"
#include "utilities.h"

typedef struct
{
	Dictionary    dict;
	Parse_Options opts, panic_parse_opts;
	Sentence      sent;
	Linkage       linkage;
	int           num_linkages, cur_linkage;
#if DO_PHRASE_TREE
	CNode*        tree;
#endif
} per_thread_data;

/* XXX FIXME
 * The per_thread_data struct should ideally be somehow 
 * fetched from JNIEnv, or as an opaque pointer in the class.
 * Not clear how to do this .. perhaps use NewDirectByteBuffer()
 * and the java.nio.ByteBuffer class ?
 */
static per_thread_data * global_ptd = NULL;

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

/* message: The string is encoded in modified UTF-8, per JNI 1.5 spec. */
static void throwException(JNIEnv *env, const char* message)
{
	char *msg;
	jclass exceptionClazz;
	if ((*env)->ExceptionOccurred(env) != NULL) return;

	
	msg = (char *) malloc(50+strlen(message));
	strcpy(msg, "link-parser:\n");
	strcat(msg, message);
	exceptionClazz = (*env)->FindClass(env, "java/lang/RuntimeException");
	if ((*env)->ThrowNew(env, exceptionClazz, msg) != 0)
		(*env)->FatalError(env, "Cannot throw");
}

static per_thread_data * init(JNIEnv *env)
{
	per_thread_data *ptd;
	/* Get the locale from the environment...
	 * perhaps we should someday get it from the dictionary ??
	 */
	setlocale(LC_ALL, "");

	ptd = (per_thread_data *) malloc(sizeof(per_thread_data));
	memset(ptd, 0, sizeof(per_thread_data));

	lperror_clear();
	ptd->panic_parse_opts = parse_options_create();
	setup_panic_parse_options(ptd->panic_parse_opts);

	ptd->opts = parse_options_create();
	parse_options_set_disjunct_cost(ptd->opts, 3);
	parse_options_set_max_sentence_length(ptd->opts, 170);
	parse_options_set_panic_mode(ptd->opts, TRUE);
	parse_options_set_max_parse_time(ptd->opts, 30);
	parse_options_set_linkage_limit(ptd->opts, 1000);
	parse_options_set_short_length(ptd->opts, 10);
	parse_options_set_verbosity(ptd->opts,0);

	/* Default to the english language; will need to fix 
	 * this if/when more languages are supported.
	 */
	ptd->dict = dictionary_create_lang("en");
	if (!ptd->dict) throwException(env, lperrmsg);
	else test();

	return ptd;
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

static void jParse(JNIEnv *env, per_thread_data *ptd, char* inputString)
{
	int maxlen;
	Parse_Options opts = ptd->opts;
	int jverbosity = parse_options_get_verbosity(opts);

	if (ptd->sent)
		sentence_delete(ptd->sent);

	if (ptd->dict == NULL) throwException(env, "jParse: dictionary not open\n");
	if (inputString == NULL) throwException(env, "jParse: no input sentence!\n");
	ptd->sent = sentence_create(inputString, ptd->dict);
	ptd->num_linkages = 0;

	if (ptd->sent == NULL)
		return;

	maxlen = parse_options_get_max_sentence_length(ptd->opts);
	if (maxlen < sentence_length(ptd->sent))
	{
		sentence_delete(ptd->sent);
		ptd->sent = NULL;
		if (jverbosity > 0) {
			fprintf(stderr,
				"Error: Sentence length (%d words) exceeds maximum allowable (%d words)\n",
				sentence_length(ptd->sent), maxlen);
		}
		return;
	}

	/* First parse with cost 0 or 1 and no null links */
	parse_options_set_disjunct_cost(opts, 2);
	parse_options_set_min_null_count(opts, 0);
	parse_options_set_max_null_count(opts, 0);
	parse_options_reset_resources(opts);

	ptd->num_linkages = sentence_parse(ptd->sent, ptd->opts);

	/* Now parse with null links */
	if ((ptd->num_linkages == 0) && (!parse_options_get_batch_mode(ptd->opts)))
	{
		if (jverbosity > 0) fprintf(stderr, "Warning: No complete linkages found.\n");
		if (parse_options_get_allow_null(opts)) {
			parse_options_set_min_null_count(opts, 1);
			parse_options_set_max_null_count(opts, sentence_length(ptd->sent));
			ptd->num_linkages = sentence_parse(ptd->sent, opts);
		}
	}

	if (parse_options_timer_expired(opts)) {
		if (jverbosity > 0) fprintf(stderr, "Warning: Timer is expired!\n");
	}
	if (parse_options_memory_exhausted(opts)) {
		if (jverbosity > 0) fprintf(stderr, "Warning: Memory is exhausted!\n");
	}

	if ((ptd->num_linkages == 0) &&
			parse_options_resources_exhausted(opts) &&
			parse_options_get_panic_mode(opts))
	{
		print_total_time(opts);
		if (jverbosity > 0) fprintf(stderr, "Warning: Entering \"panic\" mode...\n");
		parse_options_reset_resources(ptd->panic_parse_opts);
		parse_options_set_verbosity(ptd->panic_parse_opts, jverbosity);
		ptd->num_linkages = sentence_parse(ptd->sent, ptd->panic_parse_opts);
		if (parse_options_timer_expired(ptd->panic_parse_opts)) {
			if (jverbosity > 0) fprintf(stderr, "Error: Timer is expired!\n");
		}
	}
}

static void makeLinkage(per_thread_data *ptd)
{
	if (ptd->cur_linkage < ptd->num_linkages)
	{
		if (ptd->linkage)
			linkage_delete(ptd->linkage);

		ptd->linkage = linkage_create(ptd->cur_linkage,ptd->sent,ptd->opts);
		linkage_compute_union(ptd->linkage);
		linkage_set_current_sublinkage(ptd->linkage,
		                        linkage_get_num_sublinkages(ptd->linkage)-1);

#if DO_PHRASE_TREE
		if (tree)
			linkage_free_constituent_tree(tree);
		tree = linkage_constituent_tree(linkage);
		printTree(tree);
#endif
	}
}

static void finish(per_thread_data *ptd)
{
	if (ptd->sent)
		sentence_delete(ptd->sent);
	ptd->sent = NULL;

#if DO_PHRASE_TREE
	if (tree)
		linkage_free_constituent_tree(tree);
	tree = NULL;
#endif

	if (ptd->linkage)
		linkage_delete(ptd->linkage);
	ptd->linkage = NULL;

	dictionary_delete(ptd->dict);
	ptd->dict = NULL;

	parse_options_delete(ptd->opts);
	ptd->opts = NULL;

	parse_options_delete(ptd->panic_parse_opts);
	ptd->panic_parse_opts = NULL;
}

/* =========================================================================== */
/* Java JNI wrappers */

/*
 * Class:		 LinkGrammar
 * Method:		getVersion
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_linkgrammar_LinkGrammar_getVersion(JNIEnv *env, jclass cls)
{
	const char *s = "link-grammar-" LINK_VERSION_STRING;
	jstring j = (*env)->NewStringUTF(env, s);
	return j;
}

JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_setMaxParseSeconds(JNIEnv *env, jclass cls, jint maxParseSeconds)
{
	per_thread_data *ptd = global_ptd;
	parse_options_set_max_parse_time(ptd->opts, maxParseSeconds);
}

JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_setMaxCost(JNIEnv *env, jclass cls, jint maxCost)
{
	per_thread_data *ptd = global_ptd;
	parse_options_set_disjunct_cost(ptd->opts, maxCost);
}

JNIEXPORT void JNICALL 
Java_org_linkgrammar_LinkGrammar_setDictionariesPath(JNIEnv *env,
                                              jclass cls, jstring path)
{
	const char *nativePath = (*env)->GetStringUTFChars(env,path, 0);

	// Java passes null pointers as the string "null"
	if (nativePath && strcmp(nativePath, "null")) 
	{
		set_data_dir(nativePath);
	}
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
	global_ptd = init(env);
}

/*
 * Class:		 LinkGrammar
 * Method:		parse
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_parse(JNIEnv *env, jclass cls, jstring str)
{
	per_thread_data *ptd = global_ptd;
	const char *cStr;
	char* tmp;
	cStr = (*env)->GetStringUTFChars(env,str,0);
	tmp = strdup(cStr);
	jParse(env, ptd, tmp);
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
	per_thread_data *ptd = global_ptd;
	finish(ptd);
}

/*
 * Class:		 LinkGrammar
 * Method:		numWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getNumWords(JNIEnv *env, jclass cls)
{
	per_thread_data *ptd = global_ptd;
	return linkage_get_num_words(ptd->linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		getWord
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_linkgrammar_LinkGrammar_getWord(JNIEnv *env, jclass cls, jint i)
{
	per_thread_data *ptd = global_ptd;

	/* does not need to be freed, points into sentence */
	char* w = sentence_get_word(ptd->sent, i);
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
	per_thread_data *ptd = global_ptd;
	return sentence_null_count(ptd->sent);
}

/*
 * Class:		 LinkGrammar
 * Method:		numLinkages
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getNumLinkages(JNIEnv *env, jclass cls)
{
	per_thread_data *ptd = global_ptd;
	return sentence_num_valid_linkages(ptd->sent);
}

/*
 * Class:		 LinkGrammar
 * Method:		makeLinkage
 * Signature: (I)I
 */
JNIEXPORT void JNICALL
Java_org_linkgrammar_LinkGrammar_makeLinkage(JNIEnv *env, jclass cls, jint i)
{
	per_thread_data *ptd = global_ptd;
	ptd->cur_linkage = i;
	makeLinkage(ptd);
}

/*
 * Class:		 LinkGrammar
 * Method:		linkageNumViolations
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkageNumViolations(JNIEnv *env, jclass cls)
{
	per_thread_data *ptd = global_ptd;
	return sentence_num_violations(ptd->sent, ptd->cur_linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		linkageAndCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkageAndCost(JNIEnv *env, jclass cls)
{
	per_thread_data *ptd = global_ptd;
	return sentence_and_cost(ptd->sent, ptd->cur_linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		linkageDisjunctCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkageDisjunctCost(JNIEnv *env, jclass cls)
{
	per_thread_data *ptd = global_ptd;
	return sentence_disjunct_cost(ptd->sent, ptd->cur_linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		linkageLinkCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkageLinkCost(JNIEnv *env, jclass cls)
{
	per_thread_data *ptd = global_ptd;
	return sentence_link_cost(ptd->sent, ptd->cur_linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		getNumLinks
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getNumLinks(JNIEnv *env, jclass cls)
{
	per_thread_data *ptd = global_ptd;
	return linkage_get_num_links(ptd->linkage);
}

/*
 * Class:		 LinkGrammar
 * Method:		getLinkLWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkLWord(JNIEnv *env, jclass cls, jint i)
{
	per_thread_data *ptd = global_ptd;
	return linkage_get_link_lword(ptd->linkage, i);
}

/*
 * Class:		 LinkGrammar
 * Method:		getLinkRWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkRWord(JNIEnv *env, jclass cls, jint i)
{
	per_thread_data *ptd = global_ptd;
	return linkage_get_link_rword(ptd->linkage, i);
}

/*
 * Class:		 LinkGrammar
 * Method:		getLinkLLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_linkgrammar_LinkGrammar_getLinkLLabel(JNIEnv *env, jclass cls, jint i)
{
	per_thread_data *ptd = global_ptd;
 	/* Does not need to be freed, points into linkage */
	const char *s = linkage_get_link_llabel(ptd->linkage, i);
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
	per_thread_data *ptd = global_ptd;
 	/* Does not need to be freed, points into linkage */
	const char *s = linkage_get_link_rlabel(ptd->linkage, i);
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
	per_thread_data *ptd = global_ptd;
 	/* Does not need to be freed, points into linkage */
	const char *s = linkage_get_link_label(ptd->linkage, i);
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
	per_thread_data *ptd = global_ptd;
	/* mode 1 prints a lisp-style string, nicely indented.
	 * mode 2 prints a lisp-style string, but with square brackets.
	 * mode 3 prints a lisp-style string, one one single line.
	 */
	/* char *s = linkage_print_constituent_tree(linkage, 1); */
	char *s = linkage_print_constituent_tree(ptd->linkage, 3);
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
	per_thread_data *ptd = global_ptd;
	char *s = linkage_print_diagram(ptd->linkage);
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
	per_thread_data *ptd = global_ptd;
	const char *cStr = (*env)->GetStringUTFChars(env,str,0);
	if (is_past_tense_form(cStr,ptd->dict) == 1)
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
	per_thread_data *ptd = global_ptd;
	const char *cStr = (*env)->GetStringUTFChars(env,str,0);
	if (is_entity(cStr,ptd->dict) == 1)
		return TRUE;
	return FALSE;
}
