#include "jhammer.h"
#include "com_upstandinghackers_hammer_Hammer.h"
#include <stdlib.h>

JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_parse
  (JNIEnv *env, jclass class, jobject obj, jbyteArray input_, jint length_)
{
    HParser *parser;
    uint8_t* input;
    size_t length;
    HParseResult *result;
    jclass resultClass;
    jobject retVal;

    parser = UNWRAP(env, obj);
    
    input = (uint8_t *) ((*env)->GetByteArrayElements(env, input_, NULL));
    length = (size_t) length_;

    result = h_parse(parser, input, length);
    
    if(result==NULL)
        return NULL;

    FIND_CLASS(resultClass, env, "com/upstandinghackers/hammer/ParseResult");
    
    NEW_INSTANCE(retVal, env, resultClass, result);
 
    return retVal;
}

JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_token
  (JNIEnv *env, jclass class, jbyteArray str, jint len)
{
    RETURNWRAP(env, h_token((uint8_t *) ((*env)->GetByteArrayElements(env, str, NULL)), (size_t) len));
}

JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_ch
  (JNIEnv *env, jclass class, jbyte c)
{
    RETURNWRAP(env, h_ch((uint8_t) c));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_chRange
  (JNIEnv *env, jclass class, jbyte lower, jbyte upper)
{
    
    RETURNWRAP(env, h_ch_range((uint8_t) lower, (uint8_t) upper));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_intRange
  (JNIEnv *env, jclass class, jobject obj, jlong lower, jlong upper)
{
    HParser *parser;
    parser = UNWRAP(env, obj); 
    RETURNWRAP(env, h_int_range(parser, (int64_t) lower, (int64_t) upper));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_bits
  (JNIEnv *env, jclass class, jint len, jboolean sign)
{
    RETURNWRAP(env, h_bits((size_t) len, (bool)(sign & JNI_TRUE)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_int64
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_int64());
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_int32
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_int32());
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_int16
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_int16()); 
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_int8
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_int8());
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_uInt64
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_uint64());
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_uInt32
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_uint32()); 
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_uInt16
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_uint16());
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_uInt8
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_uint8());
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_whitespace
  (JNIEnv *env, jclass class, jobject parser)
{
    RETURNWRAP(env, h_whitespace(UNWRAP(env, parser)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_left
  (JNIEnv *env, jclass class, jobject p, jobject q)
{
    RETURNWRAP(env, h_left(UNWRAP(env, p), UNWRAP(env, q)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_right
  (JNIEnv *env, jclass class, jobject p, jobject q)
{
    RETURNWRAP(env, h_right(UNWRAP(env, p), UNWRAP(env, q)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_middle
  (JNIEnv *env, jclass class, jobject p, jobject x, jobject q)
{
    RETURNWRAP(env, h_middle(UNWRAP(env, p), UNWRAP(env, x), UNWRAP(env, q)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_in
  (JNIEnv *env, jclass class, jbyteArray charset, jint length)
{
    RETURNWRAP(env, h_in((uint8_t *) ((*env)->GetByteArrayElements(env, charset, NULL)), (size_t)length));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_endP
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_end_p());
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_nothingP
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_nothing_p());
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_sequence
  (JNIEnv *env, jclass class, jobjectArray sequence)
{
    jsize length;
    void **parsers;
    int i;
    jobject current;
    const HParser *result;

    length = (*env)->GetArrayLength(env, sequence);
    parsers = malloc(sizeof(void *)*(length+1));
    if(NULL==parsers)
    {
        return NULL;
    }

    for(i=0; i<length; i++)
    {
        current = (*env)->GetObjectArrayElement(env, sequence, (jsize)i);
        parsers[i] = UNWRAP(env, current);
    }
    parsers[length] = NULL;
    
    result = h_sequence__a(parsers);
    RETURNWRAP(env, result);
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_choice
  (JNIEnv *env, jclass class, jobjectArray choices)
{
    jsize length;
    void **parsers;
    int i;
    jobject current;
    const HParser *result;

    length = (*env)->GetArrayLength(env, choices);
    parsers = malloc(sizeof(HParser *)*(length+1));
    if(NULL==parsers)
    {
        return NULL;
    }

    for(i=0; i<length; i++)
    {
        current = (*env)->GetObjectArrayElement(env, choices, (jsize)i);
        parsers[i] = UNWRAP(env, current);
    }
    parsers[length] = NULL;

    result = h_choice__a(parsers);
    RETURNWRAP(env, result);
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_butNot
  (JNIEnv *env, jclass class, jobject p, jobject q)
{
    RETURNWRAP(env, h_butnot(UNWRAP(env, p), UNWRAP(env, q)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_difference
  (JNIEnv *env, jclass class, jobject p, jobject q)
{
    RETURNWRAP(env, h_difference(UNWRAP(env, p), UNWRAP(env, q)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_xor
  (JNIEnv *env, jclass class, jobject p, jobject q)
{
    RETURNWRAP(env, h_xor(UNWRAP(env, p), UNWRAP(env, q)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_many
  (JNIEnv *env, jclass class, jobject p)
{
    RETURNWRAP(env, h_many(UNWRAP(env, p)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_many1
  (JNIEnv *env, jclass class, jobject p)
{
    RETURNWRAP(env, h_many1(UNWRAP(env, p)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_repeatN
  (JNIEnv *env, jclass class, jobject p, jint n)
{
    RETURNWRAP(env, h_repeat_n(UNWRAP(env, p), (size_t)n));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_optional
  (JNIEnv *env, jclass class, jobject p)
{
    RETURNWRAP(env, h_optional(UNWRAP(env, p)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_ignore
  (JNIEnv *env, jclass class, jobject p)
{
    RETURNWRAP(env, h_ignore(UNWRAP(env, p)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_sepBy
  (JNIEnv *env, jclass class, jobject p, jobject sep)
{
    RETURNWRAP(env, h_sepBy(UNWRAP(env, p), UNWRAP(env, sep)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_sepBy1
  (JNIEnv *env, jclass class, jobject p, jobject sep)
{
    RETURNWRAP(env, h_sepBy1(UNWRAP(env, p), UNWRAP(env, sep)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_epsilonP
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_epsilon_p());
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_lengthValue
  (JNIEnv *env, jclass class, jobject length, jobject value)
{
    RETURNWRAP(env, h_length_value(UNWRAP(env, length), UNWRAP(env, value)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_and
  (JNIEnv *env, jclass class, jobject p)
{
    RETURNWRAP(env, h_and(UNWRAP(env, p)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_not
  (JNIEnv *env, jclass class, jobject p)
{
    RETURNWRAP(env, h_not(UNWRAP(env, p)));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_Hammer_indirect
  (JNIEnv *env, jclass class)
{
    RETURNWRAP(env, h_indirect());
}



