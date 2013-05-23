#include "jhammer.h"
#include "com_upstandinghackers_hammer_ParseResult.h"

HParseResult *unwrap_parse_result(JNIEnv *env, jobject obj)
{
    jclass parseResultClass;
    jfieldID parseResultInner;
    FIND_CLASS(parseResultClass, env, "com/upstandinghackers/hammer/ParseResult");
    parseResultInner =  (*env)->GetFieldID(env, parseResultClass, "inner", "J");
    return (HParseResult *)((*env)->GetLongField(env, obj, parseResultInner));
}


JNIEXPORT jobject JNICALL Java_com_upstandinghackers_hammer_ParseResult_getAst
  (JNIEnv *env, jobject this)
{
    HParseResult *inner;
    jclass parsedTokenClass;
    jobject retVal;

    if(this == NULL)
        return NULL; // parse unsuccessful
    inner = unwrap_parse_result(env, this);
    if(inner->ast == NULL)
        return NULL; // parse successful, but empty

    FIND_CLASS(parsedTokenClass, env, "com/upstandinghackers/hammer/ParsedToken");
    NEW_INSTANCE(retVal, env, parsedTokenClass, inner->ast);
    return retVal;
    
}

JNIEXPORT jlong JNICALL Java_com_upstandinghackers_hammer_ParseResult_getBitLength
  (JNIEnv *env, jobject this)
{
    HParseResult *inner = unwrap_parse_result(env, this);
    return (jlong) (inner->bit_length);
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParseResult_free
  (JNIEnv *env, jobject this)
{
    //XXX: NOT IMPLEMENTED
}

