#ifndef __rai_util__strptime_h__
#define __rai_util__strptime_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct tm;

RAIBASE_DLL_EXP
const char * rai_strptime( const char *buf,  const char *fmt,  struct tm *tm );

#ifdef __cplusplus
}
#endif

#endif
