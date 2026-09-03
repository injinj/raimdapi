/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_base__types_h__
#define __rai_base__types_h__

#if defined( __x86_64__ ) || defined( __amd64 )
  #if ! defined( __amd64__ )
    #define __amd64__
  #endif
#endif

#if defined( __SUNPRO_CC ) || defined( __SUNPRO_C )
  #if ! defined( __sun__ ) && ! defined( __linux__ )
    #define __sun__
  #endif
#endif


#if defined( __ICC ) || defined( __GNUC__ )
  __extension__ typedef long long          llong;
  __extension__ typedef unsigned long long ullong;
  typedef unsigned char byte;
  typedef unsigned long ulongptr;
  #define RAI_DLL_EXPORT
#elif defined( __ICL )
  typedef __int64            llong;
  typedef unsigned __int64   ullong;
  typedef unsigned __int64   ulongptr;
  #define byte unsigned char
  #if defined( _MT ) && defined( _DLL )
  #pragma comment(linker,"/NODEFAULTLIB:libmmd")
  #pragma comment(lib,"libmmds.lib")
  #endif
  #if defined( RAI_DLL )
    #define RAI_DLL_EXPORT __declspec(dllexport)
  #else
    #define RAI_DLL_EXPORT
  #endif
#elif defined( _MSC_VER )
  typedef __int64            llong;
  typedef unsigned __int64   ullong;
  typedef unsigned __int64   ulongptr;
  #define byte unsigned char
  #if defined( RAI_DLL )
    #define RAI_DLL_EXPORT __declspec(dllexport)
  #else
    #define RAI_DLL_EXPORT
  #endif
#else
  typedef long long          llong;
  typedef unsigned long long ullong;
  typedef unsigned char      byte;
  typedef unsigned long      ulongptr;
  #define RAI_DLL_EXPORT
#endif

namespace rai {
struct ErrorRec {
  unsigned int status; /* error status code, it's domain is the module */
  const char * reason, /* the static error string that status maps to */
             * module; /* the static module name where the error is defined */
};

typedef const struct ErrorRec *Error;
}
typedef const struct rai::ErrorRec *RaiException;

#if ! defined( NULL )
  #if defined( __GNUC__ )
    #define NULL __null
  #else
    #define NULL 0
  #endif
#endif

#endif
