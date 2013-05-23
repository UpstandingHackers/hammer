#include "jhammer.h"
#include "com_upstandinghackers_hammer_ParsedToken.h"

#define HPT_UNWRAP(env, this) HParsedToken *inner = unwrap_parsed_token(env, this); assert(inner!=NULL)

HParsedToken *unwrap_parsed_token(JNIEnv *env, jobject obj)
{
    jclass parsedTokenClass;
    jfieldID parsedTokenInner;
    FIND_CLASS(parsedTokenClass, env, "com/upstandinghackers/hammer/ParsedToken");
    parsedTokenInner =  (*env)->GetFieldID(env, parsedTokenClass, "inner", "J");
    return (HParsedToken *)((*env)->GetLongField(env, obj, parsedTokenInner));
}


JNIEXPORT jint JNICALL Java_com_upstandinghackers_hammer_ParsedToken_getTokenTypeInternal
  (JNIEnv *env, jobject this)
{
    HPT_UNWRAP(env, this);
    if(inner==NULL)
        return (jint)0;
    return (jint)(inner->token_type);
}

JNIEXPORT jint JNICALL Java_com_upstandinghackers_hammer_ParsedToken_getIndex
  (JNIEnv *env, jobject this)
{
    HPT_UNWRAP(env, this);
    return (jint) (inner->index);
}

JNIEXPORT jbyte JNICALL Java_com_upstandinghackers_hammer_ParsedToken_getBitOffset
  (JNIEnv *env, jobject this)
{
    HPT_UNWRAP(env, this);
    return (jbyte) (inner->bit_offset);
}

JNIEXPORT jbyteArray JNICALL Java_com_upstandinghackers_hammer_ParsedToken_getBytesValue
  (JNIEnv *env, jobject this)
{
    jbyteArray outArray;
    HPT_UNWRAP(env, this);
    outArray = (*env)->NewByteArray(env, (jsize)inner->bytes.len);
    (*env)->SetByteArrayRegion(env, outArray, (jsize) 0, (jsize)(inner->bytes.len), (jbyte *)(inner->bytes.token));
    return outArray; 
}

JNIEXPORT jlong JNICALL Java_com_upstandinghackers_hammer_ParsedToken_getSIntValue
  (JNIEnv *env, jobject this)
{
    HPT_UNWRAP(env, this);
    return (jlong) (inner->sint);
}

JNIEXPORT jlong JNICALL Java_com_upstandinghackers_hammer_ParsedToken_getUIntValue
  (JNIEnv *env, jobject this)
{
    HPT_UNWRAP(env, this);
    return (jlong) (inner->uint);
}

JNIEXPORT jdouble JNICALL Java_com_upstandinghackers_hammer_ParsedToken_getDoubleValue
  (JNIEnv *env, jobject this)
{
    HPT_UNWRAP(env, this);
    return (jdouble) (inner->dbl);
}

JNIEXPORT jfloat JNICALL Java_com_upstandinghackers_hammer_ParsedToken_getFloatValue
  (JNIEnv *env, jobject this)
{
    HPT_UNWRAP(env, this);
    return (jfloat) (inner->flt);
}

JNIEXPORT jobjectArray JNICALL Java_com_upstandinghackers_hammer_ParsedToken_getSeqValue
  (JNIEnv *env, jobject this)
{
    jsize i;
    HPT_UNWRAP(env, this);
    jsize returnSize = inner->seq->used;
    jobject currentObject;
    jclass returnClass;
    FIND_CLASS(returnClass, env, "com/upstandinghackers/hammer/ParsedToken");
    jobjectArray retVal = (*env)->NewObjectArray(env, returnSize, returnClass, NULL);
    for(i = 0; i<returnSize; i++)
    {
        NEW_INSTANCE(currentObject, env, returnClass, inner->seq->elements[i]);
        (*env)->SetObjectArrayElement(env, retVal, i, currentObject);
    }
    return retVal;
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParsedToken_setTokenType
  (JNIEnv *env, jobject this, jobject tokenType)
{
    jclass tokenTypeClass;
    jmethodID getValue;
    jint typeVal;
    HPT_UNWRAP(env, this);
    
    FIND_CLASS(tokenTypeClass, env, "com/upstandinghackers/hammer/Hammer$TokenType");
    getValue = (*env)->GetMethodID(env, tokenTypeClass, "getValue", "()I");
    typeVal = (*env)->CallIntMethod(env, tokenType, getValue);

    inner->token_type = (int32_t) typeVal; // unsafe cast, but enums should be of type int
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParsedToken_setIndex
  (JNIEnv *env, jobject this, jint index)
{
    HPT_UNWRAP(env, this);
    inner->index = (size_t)index;
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParsedToken_setBitOffset
  (JNIEnv *env, jobject this, jbyte bit_offset)
{
    HPT_UNWRAP(env, this);
    inner->bit_offset = (char)bit_offset;
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParsedToken_setBytesValue
  (JNIEnv *env, jobject this, jbyteArray bytes_)
{
    HBytes bytes;
    HPT_UNWRAP(env, this);
    
    bytes.token = (uint8_t *) ((*env)->GetByteArrayElements(env, bytes_, NULL));
    bytes.len = (size_t) (*env)->GetArrayLength(env, bytes_);

    inner->bytes = bytes;
    inner->token_type = TT_BYTES;
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParsedToken_setSIntValue
  (JNIEnv *env, jobject this, jlong sint)
{
    HPT_UNWRAP(env, this);
    inner->token_type = TT_SINT;
    inner->sint = (int64_t)sint;
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParsedToken_setUIntValue
  (JNIEnv *env, jobject this, jlong uint)
{
    HPT_UNWRAP(env, this);
    inner->token_type = TT_UINT;
    inner->uint = (uint64_t)uint;
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParsedToken_setDoubleValue
  (JNIEnv *env, jobject this, jdouble dbl)
{
    HPT_UNWRAP(env, this);
    //token_type?
    inner->dbl = (double)dbl;
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParsedToken_setFloatValue
  (JNIEnv *env, jobject this, jfloat flt)
{
    HPT_UNWRAP(env, this);
    //token_type?
    inner->flt = (float)flt;
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_ParsedToken_setSeqValue
  (JNIEnv *env, jobject this, jobjectArray values)
{
    HArena *arena;
    size_t len, i;
    jobject currentValue;
    HParsedToken *currentValueInner;
    HCountedArray *seq;
    HPT_UNWRAP(env, this);
    len = (size_t) (*env)->GetArrayLength(env, values); 
    arena = h_new_arena(&system_allocator, 0);
    seq = h_carray_new_sized(arena, len);
    
    // unwrap each value and append it to the new HCountedArray
    for(i = 0; i<len; i++)
    {
        currentValue = (*env)->GetObjectArrayElement(env, values, (jsize)i);
        if(NULL == currentValue) 
            continue;
        currentValueInner = unwrap_parsed_token(env, currentValue);
        if(currentValueInner)
            h_carray_append(seq, (void *)currentValueInner);
    }
    
    inner->token_type = TT_SEQUENCE; 
    inner->seq = seq;
}
