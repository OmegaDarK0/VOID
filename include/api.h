#ifndef VOID_API_H
#define VOID_API_H

#ifdef _WIN32
    #ifdef VOID_BUILD_DLL
        #define VOID_API __declspec(dllexport)
    #else
        #define VOID_API __declspec(dllimport)
    #endif
#else
    #define VOID_API
#endif

#endif //VOID_API_H
