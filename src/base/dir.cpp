/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#if defined( _WIN32 ) || defined( _WIN64 )
  #include <windows.h>
  #include <io.h>
  #include <process.h>
  #include <stdlib.h>
#else
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <dirent.h>
  #include <errno.h>
#endif

#include "base/mem.h"
#include "base/dir.h"
#include "util/str_util.h"

using namespace rai;

#if defined( _WIN32 ) || defined( _WIN64 )
  #define INVALID_DIRPTR INVALID_HANDLE_VALUE

  typedef HANDLE dirptr_t;
#else
  #define INVALID_DIRPTR NULL

  typedef DIR *dirptr_t;
#endif


bool
Dir::dirExists( const char *path )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  DWORD attributes;

  attributes = ::GetFileAttributes( path );
  if ( attributes == INVALID_FILE_ATTRIBUTES )
    throw DirErr::getErr( DirErr::STAT_FAILED );
  if ( (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
    return false;
  return true;
#else
  struct stat st;

  if ( ::stat( path, &st ) != 0 ) {
    if ( errno == ENOENT )
      return false;
    throw DirErr::getErr( DirErr::STAT_FAILED );
  }
  if ( S_ISDIR( st.st_mode ) )
    return true;
  return false;
#endif
}


void
Dir::workingDirectory( char *path,  unsigned int pathLen )
{
  unsigned int len;

#if defined( _WIN32 ) || defined( _WIN64 )
  if ( ! ::GetCurrentDirectory( (DWORD) pathLen, path ) )
    throw DirErr::getErr( DirErr::GETCWD_FAILED );
#else
  if ( ::getcwd( path, pathLen ) == NULL )
    throw DirErr::getErr( DirErr::GETCWD_FAILED );
#endif

  len = ::strlen( path );

  if ( len > 0 && path[ len - 1 ] != '/' ) {
    if ( len + 2 >= pathLen )
      throw DirErr::getErr( DirErr::PATH_LEN_TOO_SMALL );
    path[ len++ ] = '/';
    path[ len ] = '\0';
  }

#if defined( _WIN32 ) || defined( _WIN64 )
  while ( len > 0 ) {
    if ( path[ --len ] == '\\' )
      path[ len ] = '/';
  }
#endif
}


void
Dir::changeDirectory( const char *path )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( ! ::SetCurrentDirectory( path ) )
    throw DirErr::getErr( DirErr::CHANGE_DIR_FAILED );
#else
  if ( ::chdir( path ) != 0 )
    throw DirErr::getErr( DirErr::CHANGE_DIR_FAILED );
#endif
}


void
Dir::makeDir( const char *path )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( ! ::CreateDirectory( path, NULL ) )
    throw DirErr::getErr( DirErr::CREATE_DIR_FAILED );
#else
  if ( ::mkdir( path, 0777 ) != 0 )
    throw DirErr::getErr( DirErr::CREATE_DIR_FAILED );
#endif
}


namespace rai {
class SysDir : public Dir {
  public:
    dirptr_t dir;
#if defined( _WIN32 ) || defined( _WIN64 )
    WIN32_FIND_DATA data;
    char path[ MAX_PATH ];
#endif
    SYS_OPS( SysDir );

    SysDir( dirptr_t d = INVALID_DIRPTR ) {
      this->dir = d;
    }

    virtual ~SysDir( void );

    virtual void close( void );

    virtual bool read( char *path,  unsigned int pathLen,
                       unsigned int *usedLen );
    virtual void rewind( void );
};
} // namespace rai

SysDir::~SysDir()
{
  if ( this->dir != INVALID_DIRPTR ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


Dir *
Dir::openDir( const char *path )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  dirptr_t        h;
  WIN32_FIND_DATA data;
  char            path2[ 1024 ];
  SysDir        * dir;
  unsigned int    len;
  char            sep;

  len = ::strlen( path );
  if ( len == 0 || len >= sizeof( path2 ) - 3 )
    throw DirErr::getErr( DirErr::OPEN_FAILED );

  ::strncpy( path2, path, len );
  sep = '/';
  if ( ::strchr( path2, '/' ) == NULL && ::strchr( path2, '\\' ) != NULL )
    sep = '\\';
  if ( path2[ len - 1 ] != sep )
    path2[ len++ ] = sep;
  path2[ len++ ] = '*';
  path2[ len++ ] = '\0';

  h = ::FindFirstFile( path2, &data );
  if ( h == INVALID_DIRPTR )
    throw DirErr::getErr( DirErr::OPEN_FAILED );

  try {
    dir = NEW SysDir( h );
    dir->data = data;
    ::strncpy( dir->path, path2, sizeof( dir->path ) );
    return dir;
  } catch ( ... ) {
    ::FindClose( h );
    throw;
  }

#else
  DIR * d;

  if ( (d = opendir( path )) == NULL )
    throw DirErr::getErr( DirErr::OPEN_FAILED );

  try {
    return NEW SysDir( d );
  } catch ( ... ) {
    closedir( d );
    throw;
  }
#endif
}


void
SysDir::close( void )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( this->dir != INVALID_DIRPTR ) {
    BOOL b = ::FindClose( this->dir );
    this->dir = INVALID_DIRPTR;
    ::memset( &this->data, 0, sizeof( this->data ) );
    if ( ! b )
      throw DirErr::getErr( DirErr::CLOSE_FAILED );
  }
#else
  int status;

  if ( this->dir != INVALID_DIRPTR ) {
    status = closedir( this->dir );
    this->dir = INVALID_DIRPTR;
    if ( status != 0 )
      throw DirErr::getErr( DirErr::CLOSE_FAILED );
  }
#endif
}


bool
SysDir::read( char *path,  unsigned int pathLen,  unsigned int *usedLen )

{
#if defined( _WIN32 ) || defined( _WIN64 )
  unsigned int len;

  if ( this->data.cFileName[ 0 ] == '\0' )
    return false;

  len = ::strlen( this->data.cFileName );
  if ( len >= pathLen )
    ::strncpy( path, this->data.cFileName, pathLen );
  else
    ::strcpy( path, this->data.cFileName );

  if ( usedLen != NULL )
    *usedLen = len;

  if ( ! ::FindNextFile( this->dir, &this->data ) )
    ::memset( &this->data, 0, sizeof( this->data ) );

  return true;
#else
  struct dirent * ent;
  unsigned int    len;

  errno = 0;
  if ( (ent = ::readdir( this->dir )) == NULL ) {
    if ( errno != 0 )
      throw DirErr::getErr( DirErr::READ_FAILED );
    return false;
  }

  len = str_copy( path, ent->d_name, pathLen );

  if ( usedLen != NULL )
    *usedLen = len;

  return true;
#endif
}


void
SysDir::rewind( void )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( this->dir != INVALID_DIRPTR )
    ::FindClose( this->dir );
  this->dir = ::FindFirstFile( this->path, &this->data );
  if ( this->dir == INVALID_DIRPTR ) {
    ::memset( &this->data, 0, sizeof( this->data ) );
    throw DirErr::getErr( DirErr::REWIND_FAILED );
  }
#else
  rewinddir( this->dir );
#endif
}


Error
DirErr::getErr( unsigned int status )
{
  static const char     mod[] = "Dir";
  static const ErrorRec err[] = {
  /*  0 */ { OPEN_FAILED,        "Unable to open directory", mod },
  /*  1 */ { READ_FAILED,        "Unable to read directory", mod },
  /*  2 */ { CLOSE_FAILED,       "Unable to close directory", mod },
  /*  3 */ { CREATE_DIR_FAILED,  "Unable to create directory", mod },
  /*  4 */ { STAT_FAILED,        "Unable to stat directory", mod },
  /*  5 */ { GETCWD_FAILED,      "Unable to get current working directory",
                                 mod },
  /*  6 */ { PATH_LEN_TOO_SMALL, "Buffer too small for path", mod },
  /*  7 */ { REWIND_FAILED,      "Unable to rewind directory", mod },
  /*  8 */ { CHANGE_DIR_FAILED,  "Unable to change working directory", mod },
  /*  9 */ { 9,                  "Unknown dir error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}


