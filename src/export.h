#ifndef HAMMER_EXPORT__H__
#define HAMMER_EXPORT__H__

#if defined _MSC_VER
# if defined BUILDING_HAMMER_DLL
#   define H_EXPORT __declspec(dllexport)
# else
    /*
     * there is a slight performance gain in using __declspec(dllimport) here,
     * but doing so would break static linking against hammer.
     */
#   define H_EXPORT extern
# endif
#else
# if __GNUC__ >= 4
#   define H_EXPORT __attribute__ ((visibility ("default")))
# else
#   define H_EXPORT
# endif
#endif

#endif // #ifndef HAMMER_EXPORT__H__
