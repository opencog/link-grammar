
#include <jni.h>
/* Header for class LinkGrammar */

#ifndef _LinkGrammar_H_
#define _LinkGrammar_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     LinkGrammar
 * Method:    setMaxParseSeconds
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_setMaxParseSeconds
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    setMaxCost
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_setMaxCost
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    init
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_init
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    parse
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_parse
	(JNIEnv *, jclass, jstring);

/*
 * Class:     LinkGrammar
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_close
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    numWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_numWords
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    getWord
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_getWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    numSkippedWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_numSkippedWords
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    numLinkages
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_numLinkages
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    makeLinkage
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_linkgrammar_LinkGrammar_makeLinkage
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    linkageNumViolations
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_linkageNumViolations
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    linkageDisjunctCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_linkageDisjunctCost
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    numLinks
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_numLinks
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    linkLWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_linkLWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    linkRWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_linkgrammar_LinkGrammar_linkRWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    linkLLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_linkLLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    linkRLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_linkRLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    linkLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_linkLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkGrammar
 * Method:    constituentString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_constituentString
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    linkString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linkgrammar_LinkGrammar_linkString
	(JNIEnv *, jclass);

/*
 * Class:     LinkGrammar
 * Method:    isPastTenseForm
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_linkgrammar_LinkGrammar_isPastTenseForm
	(JNIEnv *, jclass, jstring);

/*
 * Class:     LinkGrammar
 * Method:    isEntity
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_linkgrammar_LinkGrammar_isEntity
	(JNIEnv *, jclass, jstring);

#ifdef __cplusplus
}
#endif

#endif /*_LinkGrammar_H_ */
