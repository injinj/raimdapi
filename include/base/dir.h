/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_base__dir_h__
#define __rai_base__dir_h__

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

class RAIBASE_DLL_EXP Dir {
  public:
    virtual ~Dir() {};

    virtual void close( void ) = 0;

    virtual bool read( char *path,  unsigned int pathLen,
                       unsigned int *usedLen = NULL ) = 0;
    virtual void rewind( void ) = 0;

    static Dir *openDir( const char *path );

    static bool dirExists( const char *path );

    static void makeDir( const char *path );

    static void workingDirectory( char *path,  unsigned int pathLen )
;
    static void changeDirectory( const char *path );
};


namespace DirErr {
  enum {
    OPEN_FAILED        = 0,
    READ_FAILED        = 1,
    CLOSE_FAILED       = 2,
    CREATE_DIR_FAILED  = 3,
    STAT_FAILED        = 4,
    GETCWD_FAILED      = 5,
    PATH_LEN_TOO_SMALL = 6,
    REWIND_FAILED      = 7,
    CHANGE_DIR_FAILED  = 8
  };
  RAIBASE_DLL_EXP
  Error getErr( unsigned int status );
}
} // namespace rai

#endif
