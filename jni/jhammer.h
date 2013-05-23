#ifndef JHAMMER_H
#define JHAMMER_H
#include <jni.h>
#include "internal.h"
#include <assert.h>

// Unsafe (non-asserting) helpers
#define FIND_CLASS_(env, class) (*env)->FindClass(env, class)
#define REFCONSTRUCTOR_(env, class) (*env)->GetMethodID(env, class, "<init>", "(J)V")
#define NEW_INSTANCE_(env, class, inner) (*env)->NewObject(env, class, REFCONSTRUCTOR_(env, class), (jlong)inner)

// Safer versions, assert that the result is not NULL
// If one of those asserts fails, it most likely means that there's a typo (wrong class name or method signature) or big trouble (OOM)
#define FIND_CLASS(target, env, class) target = FIND_CLASS_(env, class); assert(target != NULL)
#define REFCONSTRUCTOR(target, env, class) target = REFCONSTRUCTOR_(env, class); assert(target != NULL)
#define NEW_INSTANCE(target, env, class, inner) target = NEW_INSTANCE_(env, class, inner); assert(target != NULL)


// Since there's a LOT of wrapping/unwrapping HParsers, these macros make it a bit more readable
#define PARSER_CLASS "com/upstandinghackers/hammer/Parser"
#define PARSER_REF(env) (*env)->GetFieldID(env, FIND_CLASS_(env, PARSER_CLASS), "inner", "J")

#define RETURNWRAP(env, inner) jclass __cls=FIND_CLASS_(env, PARSER_CLASS); \
                                assert(__cls != NULL); \
                                jmethodID __constructor = REFCONSTRUCTOR_(env, __cls); \
                                assert(__constructor != NULL); \
                                return (*env)->NewObject(env, __cls, __constructor, (jlong)inner)

#define UNWRAP(env, object) (HParser *)((*env)->GetLongField(env, object, PARSER_REF(env)))

#endif
