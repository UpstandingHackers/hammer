#include "jhammer.h"
#include "com_upstandinghackers_hammer_Parser.h"

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_Parser_bindIndirect
  (JNIEnv *env, jobject this, jobject parser)
{
    h_bind_indirect(UNWRAP(env, this), UNWRAP(env, parser));
}

JNIEXPORT void JNICALL Java_com_upstandinghackers_hammer_Parser_free
  (JNIEnv *env, jobject this)
{
    //XXX NOT IMPLEMENTED
    //h_free(UNWRAP(env, this));
}
