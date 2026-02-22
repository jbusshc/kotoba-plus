#ifndef KOTOBA_H
#define KOTOBA_H

#if defined(_WIN32)
  #if defined(KOTOBA_BUILD_DLL)
    #define KOTOBA_API __declspec(dllexport)
  #elif defined(KOTOBA_USE_DLL)
    #define KOTOBA_API __declspec(dllimport)
  #else
    #define KOTOBA_API
  #endif
#else
  #define KOTOBA_API
#endif

#ifdef DEBUG
    #define LOG_DEBUG(...) printf(__VA_ARGS__)
#else
    #define LOG_DEBUG(...)
#endif


#endif // KOTOBA_H
