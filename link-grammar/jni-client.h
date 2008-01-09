
#include <jni.h>
/* Header for class LinkGrammar */

#ifndef _LinkGrammar
#define _LinkGrammar

#ifdef __cplusplus
extern "C" {
#endif
#undef LinkGrammar_verbosity
#define LinkGrammar_verbosity 1L
/*
 * Class:     LinkGrammar
 * Method:    cSetMaxParseSeconds
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_cSetMaxParseSeconds
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    cSetMaxCost
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_cSetMaxCost
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    cInit
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_cInit
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cTest
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_cTest
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cParse
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_cParse
	(JNIEnv *, jclass, jstring);

/*
 * Class:     LinkGrammar
 * Method:    cClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_cClose
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cNumWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_cNumWords
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cGetWord
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_cGetWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    cNumSkippedWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_cNumSkippedWords
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cNumLinkages
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_cNumLinkages
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cMakeLinkage
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_cMakeLinkage
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    cLinkageNumViolations
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_cLinkageNumViolations
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cLinkageDisjunctCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_cLinkageDisjunctCost
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cNumLinks
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_cNumLinks
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cLinkLWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_cLinkLWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    cLinkRWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_cLinkRWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    cLinkLLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_cLinkLLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    cLinkRLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_cLinkRLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    cLinkLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_cLinkLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    cConstituentString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_cConstituentString
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cLinkString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_cLinkString
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    cIsPastTenseForm
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_linkgrammar_LinkGrammar_cIsPastTenseForm
	(JNIEnv *, jclass, jstring);

/*
 * Class:     LinkGrammar
 * Method:    cIsEntity
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_linkgrammar_LinkGrammar_cIsEntity
	(JNIEnv *, jclass, jstring);

#ifdef __cplusplus
}
#endif

#endif
