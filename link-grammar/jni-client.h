
#include <jni.h>
/* Header for class LinkParserJNIClient */

#ifndef _LinkParserJNIClient
#define _LinkParserJNIClient

#ifdef __cplusplus
extern "C" {
#endif
#undef LinkParserJNIClient_verbosity
#define LinkParserJNIClient_verbosity 1L
/*
 * Class:     LinkParserJNIClient
 * Method:    cSetMaxParseSeconds
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_LinkParserJNIClient_cSetMaxParseSeconds
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkParserJNIClient
 * Method:    cSetMaxCost
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_LinkParserJNIClient_cSetMaxCost
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkParserJNIClient
 * Method:    cInit
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_LinkParserJNIClient_cInit
	(JNIEnv *, jclass, jstring);

/*
 * Class:     LinkParserJNIClient
 * Method:    cTest
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_LinkParserJNIClient_cTest
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cParse
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_LinkParserJNIClient_cParse
	(JNIEnv *, jclass, jstring);

/*
 * Class:     LinkParserJNIClient
 * Method:    cClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_LinkParserJNIClient_cClose
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cNumWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_LinkParserJNIClient_cNumWords
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cGetWord
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_LinkParserJNIClient_cGetWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkParserJNIClient
 * Method:    cNumSkippedWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_LinkParserJNIClient_cNumSkippedWords
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cNumLinkages
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_LinkParserJNIClient_cNumLinkages
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cMakeLinkage
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_LinkParserJNIClient_cMakeLinkage
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkParserJNIClient
 * Method:    cLinkageNumViolations
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_LinkParserJNIClient_cLinkageNumViolations
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cLinkageDisjunctCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_LinkParserJNIClient_cLinkageDisjunctCost
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cNumLinks
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_LinkParserJNIClient_cNumLinks
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cLinkLWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_LinkParserJNIClient_cLinkLWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkParserJNIClient
 * Method:    cLinkRWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_LinkParserJNIClient_cLinkRWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkParserJNIClient
 * Method:    cLinkLLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_LinkParserJNIClient_cLinkLLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkParserJNIClient
 * Method:    cLinkRLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_LinkParserJNIClient_cLinkRLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkParserJNIClient
 * Method:    cLinkLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_LinkParserJNIClient_cLinkLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     LinkParserJNIClient
 * Method:    cConstituentString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_LinkParserJNIClient_cConstituentString
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cLinkString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_LinkParserJNIClient_cLinkString
	(JNIEnv *, jclass);

/*
 * Class:     LinkParserJNIClient
 * Method:    cIsPastTenseForm
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_LinkParserJNIClient_cIsPastTenseForm
	(JNIEnv *, jclass, jstring);

/*
 * Class:     LinkParserJNIClient
 * Method:    cIsEntity
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_LinkParserJNIClient_cIsEntity
	(JNIEnv *, jclass, jstring);

#ifdef __cplusplus
}
#endif

#endif
