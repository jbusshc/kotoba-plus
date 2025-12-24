#ifndef KOTOBA_H
#define KOTOBA_H


#if defined(_WIN32)
  #if defined(KOTOBA_BUILD_DLL)
    #define KOTOBA_API __declspec(dllexport)
  #else
    #define KOTOBA_API __declspec(dllimport)
  #endif
#else
  #define KOTOBA_API
#endif

#endif // KOTOBA_H
