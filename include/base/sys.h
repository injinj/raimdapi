/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_base__sys_h__
#define __rai_base__sys_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

namespace rai {
class InputStream;
class OutputStream;

namespace Sys {
  extern RAIBASE_DLL_EXP
         InputStream  * in;             /* stdin */
  extern RAIBASE_DLL_EXP
         OutputStream * out,            /* stdout */
                      * err;            /* stderr */

  extern RAIBASE_DLL_EXP
         char           versionString[ 32 ]; /* runtime version info */
  RAIBASE_DLL_EXP
  void initialize( const char *vers = "rai/" __DATE__ ) throw( Error );
  RAIBASE_DLL_EXP
  void terminate( void );
}
} // namespace rai
#endif
