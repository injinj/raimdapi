/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_base__file_h__
#define __rai_base__file_h__

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

#ifndef __rai_base__time_h__
#include "base/time.h"
#endif

namespace rai {

typedef ullong FileOffset;
typedef llong  SeekOffset; /* signed */

#define STDIN_SPECIAL_FILE  "<"
#define STDOUT_SPECIAL_FILE ">"
#define STDERR_SPECIAL_FILE ">&"

#if defined( _WIN32 ) || defined( _WIN64 )
typedef void *fildes_t;
#else
typedef int fildes_t;
#endif 
class RAIBASE_DLL_EXP File {
  public:
    virtual ~File() {};

    virtual void close( void )                              throw( Error ) = 0;

    virtual unsigned int read( void *ptr,  unsigned int nBytes )
                                                            throw( Error ) = 0;
    virtual unsigned int write( const void *ptr,  unsigned int nBytes )
                                                            throw( Error ) = 0;
    virtual unsigned int readAt( void *ptr,  unsigned int nBytes,
                                 FileOffset seekPos )       throw( Error ) = 0;
    virtual unsigned int writeAt( const void *ptr,  unsigned int nBytes,
                                  FileOffset seekPos )      throw( Error ) = 0;
    virtual FileOffset seekSet( SeekOffset seekPos,  int whence )
                                                            throw( Error ) = 0;
    virtual FileOffset length( void )                       throw( Error ) = 0;

    virtual TimeMSecs modifiedTime( void )                  throw( Error ) = 0;

    virtual void truncate( FileOffset eofPos )              throw( Error ) = 0;

    virtual void sync( void )                               throw( Error ) = 0;

    virtual void *map( FileOffset mapSize,  int fileMode,
                       FileOffset alignment )               throw( Error ) = 0;
    virtual void unmap( void *addr,  FileOffset mapSize,
                        FileOffset alignment )              throw( Error ) = 0;

    virtual void getFildes( void *fdptr )                   throw( Error ) = 0;

    virtual void setAsync( bool mode )                      throw( Error ) = 0;
    // Copy file from oldPath to newPath
    static void copyFile( const char *srcPath,  const char *dstPath )
                                                                throw( Error );

    static bool fileExists( const char *path )                  throw( Error );

    static FileOffset fileLength( const char *path )            throw( Error );

    static TimeMSecs fileModifiedTime( const char *path )       throw( Error );

    static void setModifiedTime( const char *path,  TimeMSecs mtime = 0 )
                                                                throw( Error );
    static File *openFile( const char *path,  int fileMode )    throw( Error );

    static File *openFD( fildes_t fd )                          throw( Error );

    static File *openTempFile( const char *tmpDir,  const char *prefix )
                                                                throw( Error );

    // Move file by renaming it. If that fails, try copy and remove original
    static void moveFile( const char *oldPath,  const char *newPath )
                                                                throw( Error );

    static void removeFile( const char *path )                  throw( Error );

    static void renameFile( const char *oldPath,  const char *newPath )
                                                                throw( Error );
    enum Whence {
      IO_SEEK_SET = 0, /* used with File and Input/Output Streams */
      IO_SEEK_CUR = 1,
      IO_SEEK_END = 2
    };

    enum FileMode {
      FILE_RDONLY =    00, /* File open() modes, same function as fcntl.h */
      FILE_WRONLY =    01, /* O_modes */
      FILE_RDWR   =    02,
      FILE_CREAT  =  0100,
      FILE_EXCL   =  0200,
      FILE_TRUNC  = 01000,
      FILE_APPEND = 02000, /* O_APPEND doesn't seem to be well supported in NT*/
      FILE_SHARED = 0x80000 /* causes shm_open() */
    };
};


namespace FileErr {
  enum {
    STAT_FAILED        = 0,
    GETCWD_FAILED      = 1,
    PATH_LEN_TOO_SMALL = 2,
    CREATE_DIR_FAILED  = 3,
    REMOVE_FAILED      = 4,
    RENAME_FAILED      = 5,
    OPEN_WRITE_FAILED  = 6,
    OPEN_READ_FAILED   = 7,
    TEMP_FAILED        = 8,
    CLOSE_FAILED       = 9,
    READ_FAILED        = 10,
    WRITE_FAILED       = 11,
    WRITE_TRUNCATED    = 12,
    SEEK_FAILED        = 13,
    TRUNC_FAILED       = 14,
    SYNC_FAILED        = 15,
    MAPFILE_FAILED     = 16,
    UNMAPFILE_FAILED   = 17,
    MTIME_FAILED       = 18,
    SHM_OPEN_FAILED    = 19,
    MAPVIEW_FAILED     = 20,
    WOULD_BLOCK        = 21,
    NOT_OPEN           = 22,
    FCNTL_FAILED       = 23
  };
  RAIBASE_DLL_EXP
  Error getErr( unsigned int status );
}

} // namespace rai

#endif
