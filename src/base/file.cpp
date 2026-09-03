/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
  #include <windows.h>
  #include <io.h>
  #include <process.h>
  #include <stdlib.h>
  #include <time.h>

  #if ! defined( INVALID_SET_FILE_POINTER )
    #define INVALID_SET_FILE_POINTER 0xFFFFFFFF
  #endif

#else
  #if defined( __linux ) || defined( __sun__ ) || defined( __SUNPRO_CC )
    #ifndef  _XOPEN_SOURCE
      #define _XOPEN_SOURCE 500 /* pread, pwrite */
    #endif
  #endif

  #if defined( __sun__ ) || defined( __SUNPRO_CC )
    /* for shm_open */
    #define _POSIX_C_SOURCE 199506L
  #endif

  #include <stdio.h>
  #include <string.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <stdlib.h>
  #include <errno.h>
  #include <utime.h>
  #include <sys/time.h>

  #if defined( __GLIBC__ )
    #if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
      #define USE_PREAD
    #endif
  #elif defined( __sun__ ) || defined( __SUNPRO_CC )
    #define USE_PREAD
    #define USE_CADDR_T
  #endif
#endif

#include "base/mem.h"
#include "base/file.h"

#if defined( _WIN32 ) || defined( _WIN64 )
static void fileMode_to_windozeMode( int mode,  DWORD *dwDesiredAccess,
                                     DWORD *dwCreationDisposition );
#else
static void fileMode_to_unixMode( int mode,  unsigned int *unixMode );
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
  #define STDIN_FD       GetStdHandle( STD_INPUT_HANDLE )
  #define STDOUT_FD      GetStdHandle( STD_OUTPUT_HANDLE )
  #define STDERR_FD      GetStdHandle( STD_ERROR_HANDLE )
  #define INVALID_FILDES INVALID_HANDLE_VALUE
#else
  #define STDIN_FD       STDIN_FILENO
  #define STDOUT_FD      STDOUT_FILENO
  #define STDERR_FD      STDERR_FILENO
  #define INVALID_FILDES -1
#endif

using namespace rai;

namespace rai {
class SysFile : public File {
  protected:
    fildes_t fd;

  public:
    SYS_OPS( SysFile );

    SysFile( fildes_t fild = INVALID_FILDES ) {
      this->setFD( fild );
    };

    virtual ~SysFile( void );

    void setFD( fildes_t fild ) { this->fd = fild; }

    virtual void close( void );

    virtual unsigned int read( void *ptr,  unsigned int nBytes )
;
    virtual unsigned int write( const void *ptr,  unsigned int nBytes )
;
    virtual unsigned int readAt( void *ptr,  unsigned int nBytes,
                                 FileOffset seekPos );
    virtual unsigned int writeAt( const void *ptr,  unsigned int nBytes,
                                  FileOffset seekPos );
    virtual FileOffset seekSet( SeekOffset seekPos,  int whence )
;
    virtual FileOffset length( void );

    virtual TimeMSecs modifiedTime( void );

    virtual void truncate( FileOffset eofPos );

    virtual void sync( void );

    virtual void *map( FileOffset mapSize,  int fileMode,
                       FileOffset alignment );
    virtual void unmap( void *addr,  FileOffset mapSize,  FileOffset alignment )
;
    virtual void getFildes( void *fdptr );

    virtual void setAsync( bool mode );

    bool setFlags( int fl,  bool mode );
};

class TmpSysFile : public SysFile {
  private:
    char *path;

  public:
    SYS_OPS( TmpSysFile );

    TmpSysFile( char *p,  fildes_t fild ) : SysFile( fild ) {
      this->path = p;
    }

    virtual void close( void );
};
} // namespace rai

/**
 * File handling routines
 */
//#include "stream/io_stream.h"

void 
File::copyFile( const char *srcPath,  const char *dstPath )
{
  File * srcFile = NULL,
       * dstFile = NULL;
  Error  e2 = NULL;
  FileOffset srcLen, readLen, writtenLen, off, writeLen;
  byte buff[ 64 * 1024 ];

  try {
    srcFile = File::openFile( srcPath, FILE_RDONLY );
    srcLen = srcFile->length();
    dstFile = File::openFile( dstPath, FILE_RDWR | FILE_CREAT | FILE_TRUNC );
    writtenLen = 0;
    readLen = srcFile->read( buff, sizeof( buff ) );
    while( readLen > 0 && writtenLen < srcLen ) {
      for ( off = 0; off < readLen; off += writeLen )
        writeLen = dstFile->write( &buff[ off ], readLen - off );
      writtenLen += readLen;
      readLen = srcFile->read( buff, sizeof( buff ) );
    }
  } catch ( Error e ) {
    e2 = e;
  }
  if ( srcFile != NULL ) {
    srcFile->close();
    delete srcFile;
  }
  if ( dstFile != NULL ) {
    dstFile->close();
    delete dstFile;
  }
  if ( e2 != NULL )
    throw e2;
}

bool
File::fileExists( const char *path )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  DWORD attributes;

  attributes = ::GetFileAttributes( path );
  if ( attributes == (DWORD) -1 ) {
    /*DWORD err = ::GetLastError();
    if ( err == ERROR_BAD_NET_NAME         || err == ERROR_BAD_NETPATH ||
         err == ERROR_BAD_PATHNAME         || err == ERROR_FILE_NOT_FOUND ||
         err == ERROR_FILENAME_EXCED_RANGE || err == ERROR_INVALID_DRIVE ||
         err == ERROR_NO_MORE_FILES        || err == ERROR_PATH_NOT_FOUND )*/
      return false;
    //throw FileErr::getErr( FileErr::STAT_FAILED );
  }
  if ( (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 )
    return false;
  return true;
#else
  struct stat st;

  if ( ::stat( path, &st ) != 0 ) {
    if ( errno == ENOENT )
      return false;
    throw FileErr::getErr( FileErr::STAT_FAILED );
  }
  if ( ! S_ISDIR( st.st_mode ) )
    return true;
  return false;
#endif
}


FileOffset
File::fileLength( const char *path )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  DWORD  fileSizeLo,
         fileSizeHi;
  HANDLE fd;

  fd = ::CreateFile( path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  if ( fd == INVALID_HANDLE_VALUE )
    throw FileErr::getErr( FileErr::STAT_FAILED );

  fileSizeLo = ::GetFileSize( fd, &fileSizeHi );
  CloseHandle( fd );

  if ( fileSizeLo == INVALID_FILE_SIZE && ::GetLastError() != NO_ERROR )
    throw FileErr::getErr( FileErr::STAT_FAILED );

  return ( (FileOffset) fileSizeHi << 32 ) | (FileOffset) fileSizeLo;
#else
  struct stat st;

  if ( ::stat( path, &st ) != 0 )
    throw FileErr::getErr( FileErr::STAT_FAILED );

  return (FileOffset) st.st_size;
#endif
}


TimeMSecs
File::fileModifiedTime( const char *path )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  HANDLE                     fd;
  BY_HANDLE_FILE_INFORMATION fileInfo;
  SYSTEMTIME                 sysTime;
  FILETIME                   fileTime;
  struct tm                  tm;

  fd = ::CreateFile( path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  if ( fd == INVALID_HANDLE_VALUE )
    throw FileErr::getErr( FileErr::STAT_FAILED );

  if ( GetFileInformationByHandle( fd, &fileInfo ) == 0 ) {
    CloseHandle( fd );
    throw FileErr::getErr( FileErr::STAT_FAILED );
  }
  CloseHandle( fd );

  FileTimeToLocalFileTime( &fileInfo.ftLastWriteTime, &fileTime );
  FileTimeToSystemTime( &fileTime, &sysTime );
  ::memset( &tm, 0, sizeof( tm ) );
  tm.tm_year = sysTime.wYear - 1900;
  tm.tm_mon  = sysTime.wMonth - 1;
  tm.tm_wday = sysTime.wDayOfWeek;
  tm.tm_mday = sysTime.wDay;
  tm.tm_hour = sysTime.wHour;
  tm.tm_min  = sysTime.wMinute;
  tm.tm_sec  = sysTime.wSecond;

  return (TimeMSecs) (unsigned long) ::mktime( &tm ) * (TimeMSecs) 1000U;
#else
  struct stat st;

  if ( ::stat( path, &st ) != 0 )
    throw FileErr::getErr( FileErr::STAT_FAILED );

  return (TimeMSecs) (unsigned long) st.st_mtim.tv_sec * (TimeMSecs) 1000U + ( st.st_mtim.tv_nsec / (TimeMSecs) 1000000U );
#endif
}


void
File::setModifiedTime( const char *path,  TimeMSecs mtime )
{
#if defined( _WIN32 ) || defined( _WIN64 )
#else
  timeval utim[ 2 ], *utptr = NULL;
  if ( mtime != 0 ) {
    utim[ 0 ].tv_sec  = mtime / 1000;
    utim[ 0 ].tv_usec = ( mtime % 1000 ) * 1000;
    utim[ 1 ].tv_sec  = utim[ 0 ].tv_sec;
    utim[ 1 ].tv_usec = utim[ 0 ].tv_usec;
    utptr = utim;
  }
#if 0
  utimbuf utim, *utptr = NULL;
  if ( mtime != 0 ) {
    utim.actime  = mtime / 1000;
    utim.modtime = utim.actime;
    utptr = &utim;
  }
#endif
  if ( ::utimes( path, utptr ) != 0 )
    throw FileErr::getErr( FileErr::MTIME_FAILED );
#endif
}

// Move file by renaming it. If that fails, try copy and remove original
void 
File::moveFile( const char *oldPath,  const char *newPath )
{
  // Rename can fail when renaming across file systems.
  try {
    File::renameFile( oldPath, newPath );
  } catch( Error e ) {
    if( e == FileErr::getErr( FileErr::RENAME_FAILED ) ) {
      File::copyFile( oldPath, newPath );
      File::removeFile( oldPath );
    } else {
      throw( e );
    }
  }
}

void
File::removeFile( const char *path )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( ! ::DeleteFile( path ) )
    throw FileErr::getErr( FileErr::REMOVE_FAILED );
#else
  if ( ::remove( path ) != 0 )
    throw FileErr::getErr( FileErr::REMOVE_FAILED );
#endif
}


void
File::renameFile( const char *oldPath,  const char *newPath )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( ! ::MoveFile( oldPath, newPath ) )
    throw FileErr::getErr( FileErr::RENAME_FAILED );
#else
  if ( ::rename( oldPath, newPath ) != 0 )
    throw FileErr::getErr( FileErr::RENAME_FAILED );
#endif
}


File *
File::openFile( const char *path,  int fileMode )
{
  SysFile    * sysFile;
  fildes_t     fd;
#if defined( _WIN32 ) || defined( _WIN64 )
  DWORD        dwDesiredAccess,
               dwCreationDisposition;
#else
  unsigned int unixMode;
#endif

  /* special files "<" = stdin, ">" = stdout, ">&" = stderr */
  if ( ::strcmp( path, STDIN_SPECIAL_FILE ) == 0 )
    return NEW SysFile( STDIN_FD );
  if ( ::strcmp( path, STDOUT_SPECIAL_FILE ) == 0 )
    return NEW SysFile( STDOUT_FD );
  if ( ::strcmp( path, STDERR_SPECIAL_FILE ) == 0 )
    return NEW SysFile( STDERR_FD );

  sysFile = NEW SysFile();

#if defined( _WIN32 ) || defined( _WIN64 )
  fileMode_to_windozeMode( fileMode, &dwDesiredAccess, &dwCreationDisposition );

  fd = ::CreateFile( path, dwDesiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL );

  if ( fd == INVALID_HANDLE_VALUE ) {
    delete sysFile;

    if ( ( dwDesiredAccess & GENERIC_WRITE ) == GENERIC_WRITE )
      throw FileErr::getErr( FileErr::OPEN_WRITE_FAILED );
    else
      throw FileErr::getErr( FileErr::OPEN_READ_FAILED );
  }

  if ( ( fileMode & File::FILE_APPEND ) == File::FILE_APPEND )
    ::SetFilePointer( fd, 0, NULL, FILE_END ); /* ick */

  sysFile->setFD( fd );
#else
  fileMode_to_unixMode( fileMode, &unixMode );

  if ( ( fileMode & FILE_SHARED ) != 0 ) {
    int flags;
    if ( ( fileMode & FILE_RDWR ) != 0 )
      flags = S_IREAD | S_IWRITE;
    else if ( ( fileMode & FILE_WRONLY ) != 0 )
      flags = S_IWRITE;
    else
      flags = S_IREAD;
    fd = ::shm_open( path, unixMode, flags );
    if ( fd < 0 ) {
      delete sysFile;
      throw FileErr::getErr( FileErr::SHM_OPEN_FAILED );
    }
    ::fcntl( fd, F_SETFD, FD_CLOEXEC );
  }
  else {
    fd = ::open( path, unixMode, 0666 );
    if ( fd < 0 ) {
      delete sysFile;

      if ( (unixMode & ( O_WRONLY | O_RDWR | O_APPEND )) != 0 )
        throw FileErr::getErr( FileErr::OPEN_WRITE_FAILED );
      else
        throw FileErr::getErr( FileErr::OPEN_READ_FAILED );
    }
    ::fcntl( fd, F_SETFD, FD_CLOEXEC );
  }
  sysFile->setFD( fd );
#endif

  return sysFile;
}


File *
File::openFD( fildes_t fd )
{
  SysFile    * sysFile;

  sysFile = NEW SysFile();
  sysFile->setFD( fd );
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  ::fcntl( fd, F_SETFD, FD_CLOEXEC );
#endif

  return sysFile;
}


void
SysFile::getFildes( void *fdptr )
{
  if ( this->fd == INVALID_FILDES )
    throw FileErr::getErr( FileErr::NOT_OPEN );
  *(fildes_t *) fdptr = this->fd;
}


void
SysFile::setAsync( bool mode )
{
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  if ( ! this->setFlags( O_NONBLOCK, mode ) )
    throw FileErr::getErr( FileErr::FCNTL_FAILED );
#endif
}


bool
SysFile::setFlags( int fl,  bool mode )
{
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  int flags = ::fcntl( fd, F_GETFD, 0 );
  if ( flags != -1 ) {
    if ( mode )
      flags |= fl;
    else
      flags &= ~fl;
    flags = ::fcntl( this->fd, F_SETFL, flags );
  }
  if ( flags == -1 )
    return false;
#endif
  return true;
}


File *
File::openTempFile( const char *tmpdir,  const char *prefix )
{
  static unsigned int tmpCounter;

  TmpSysFile  * tmpFile;
  char          path[ 1024 ];
  unsigned int  len,
                off,
                tries,
                num;
  fildes_t      fd;

  if ( tmpdir == NULL &&
       (tmpdir = ::getenv( "TEMPDIR" )) == NULL &&
       (tmpdir = ::getenv( "TMPDIR" )) == NULL )
    tmpdir = ".";

  len = ::strlen( tmpdir );
  if ( len + 20 > sizeof( path ) )
    throw FileErr::getErr( FileErr::PATH_LEN_TOO_SMALL );

  /* append slash to tmpdir */
  ::strcpy( path, tmpdir );
  path[ len++ ] = '/';

  /* append prefix to tmpdir */
  if ( prefix != NULL ) {
    if ( ::strlen( prefix ) + len + 20 > sizeof( path ) )
      throw FileErr::getErr( FileErr::PATH_LEN_TOO_SMALL );

    ::strcpy( &path[ len ], prefix );
    len += ::strlen( prefix );
  }

  /* append pid (up to 99999) to tmpdir/prefix */
#if defined( _WIN32 ) || defined( _WIN64 )
  num = ::GetCurrentProcessId();
#else
  num = (unsigned int) ::getpid();
#endif
  if ( num > 10000 ) path[ len++ ] = ( ( num % 100000 ) / 10000 ) + '0';
  if ( num > 1000 )  path[ len++ ] = ( ( num % 10000 ) / 1000 ) + '0';
  if ( num > 100 )   path[ len++ ] = ( ( num % 1000 ) / 100 ) + '0';
  if ( num > 10 )    path[ len++ ] = ( ( num % 100 ) / 10 ) + '0';
  path[ len++ ] = ( num % 10 ) + '0';

#define MAX_TEMP_FILE_COUNT 1000
  for ( tries = 0; tries < MAX_TEMP_FILE_COUNT; tries++ ) {
    /* append tmpCounter to tmpdir/prefixPid */
    num = tmpCounter++ % MAX_TEMP_FILE_COUNT;
    off = len;
    do {
      if ( off + 6 > sizeof( path ) )
        throw FileErr::getErr( FileErr::PATH_LEN_TOO_SMALL );

      path[ off++ ] = ( num % 26 ) + 'a';
      num /= 26;
    } while ( num != 0 );

    ::strcpy( &path[ off ], ".tmp" );

#if defined( _WIN32 ) || defined( _WIN64 )
    fd = ::CreateFile( path, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL, NULL );
    if ( fd != INVALID_HANDLE_VALUE )
      break;

    if ( ::GetLastError() != ERROR_ALREADY_EXISTS )
      throw FileErr::getErr( FileErr::TEMP_FAILED );
#else
    /* try to open, mode is set to user read/write access */
    fd = ::open( path, O_RDWR | O_CREAT | O_EXCL, 0600 );
    if ( fd >= 0 ) {
      ::fcntl( fd, F_SETFD, FD_CLOEXEC );
      break;
    }

    /* EEXIST is set by open() when O_EXCL & file exists */
    if ( errno != EEXIST )
      throw FileErr::getErr( FileErr::TEMP_FAILED );
#endif
  }

  if ( tries == MAX_TEMP_FILE_COUNT )
    throw FileErr::getErr( FileErr::TEMP_FAILED );

  try {
    char * tmpPath;
    MALLOC( ::strlen( path ) + 1, &tmpPath );
    ::strcpy( tmpPath, path );
    tmpFile = NEW TmpSysFile( tmpPath, fd );
  } catch( Error e ) {
#if defined( _WIN32 ) || defined( _WIN64 )
    ::CloseHandle( fd );
    ::DeleteFile( path );
#else
    ::close( fd );
    ::remove( path );
#endif
    throw e;
  }

  return tmpFile;
}


#if defined( _WIN32 ) || defined( _WIN64 )
static void
fileMode_to_windozeMode( int mode,  DWORD *dwDesiredAccess,
                         DWORD *dwCreationDisposition )
{
  *dwDesiredAccess       = 0;
  *dwCreationDisposition = 0;

  if ( (mode & File::FILE_WRONLY) == File::FILE_WRONLY )
    *dwDesiredAccess |= GENERIC_WRITE;
  else if ( (mode & File::FILE_RDWR) == File::FILE_RDWR )
    *dwDesiredAccess |= GENERIC_READ | GENERIC_WRITE;
  else
    *dwDesiredAccess |= GENERIC_READ;

/*  if ( (mode & FILE_APPEND) == FILE_APPEND )
    *dwCreationDisposition |= O_APPEND;*/

  if ( (mode & File::FILE_CREAT) == File::FILE_CREAT ) {

    if ( (mode & File::FILE_EXCL) == File::FILE_EXCL )
      *dwCreationDisposition |= CREATE_NEW;    /* fail if file exists */
    else if ( (mode & File::FILE_TRUNC) == File::FILE_TRUNC )
      *dwCreationDisposition |= CREATE_ALWAYS; /* create and truncate always */
    else
      *dwCreationDisposition |= OPEN_ALWAYS;   /* create if doesn't exist */
  }
  else {
    *dwCreationDisposition |= OPEN_EXISTING;
  }
}

#else
static void
fileMode_to_unixMode( int mode,  unsigned int *unixMode )
{
  *unixMode = 0;

  if ( (mode & File::FILE_WRONLY) == File::FILE_WRONLY )
    *unixMode |= O_WRONLY;
  else if ( (mode & File::FILE_RDWR) == File::FILE_RDWR )
    *unixMode |= O_RDWR;
  else
    *unixMode |= O_RDONLY;

  if ( (mode & File::FILE_APPEND) == File::FILE_APPEND )
    *unixMode |= O_APPEND;

  if ( (mode & File::FILE_CREAT) == File::FILE_CREAT )
    *unixMode |= O_CREAT;

  if ( (mode & File::FILE_TRUNC) == File::FILE_TRUNC )
    *unixMode |= O_TRUNC;

  if ( (mode & File::FILE_EXCL) == File::FILE_EXCL )
    *unixMode |= O_EXCL;
}
#endif


SysFile::~SysFile()
{
  if ( this->fd != INVALID_FILDES ) {
    try {
      this->close();
    } catch( Error ) {
    }
  }
}


void
SysFile::close( void )
{
  int status;

  if ( this->fd != INVALID_FILDES ) {
    status = 0;
    if ( this->fd != STDIN_FD && this->fd != STDOUT_FD &&
         this->fd != STDERR_FD ) {
    #if defined( _WIN32 ) || defined( _WIN64 )
     if ( ! ::CloseHandle( this->fd ) )
       status = ::GetLastError();
    #else
      status = ::close( this->fd );
    #endif
    }

    if ( status != 0 )
      throw FileErr::getErr( FileErr::CLOSE_FAILED );
    else
      this->fd = INVALID_FILDES;
  }
}


void
TmpSysFile::close( void )
{
  Error e2;

  if ( this->fd != INVALID_FILDES ) {
    e2 = NULL;
    try {
      this->SysFile::close();
    } catch( Error e ) {
      e2 = e;
    }

    if ( this->path != NULL ) {
      try {
        ::remove( this->path );
      } catch( Error e ) {
        if ( e2 == NULL )
          e2 = e;
      }
      try {
        FREE( this->path );
      } catch( Error e ) {
        if ( e2 == NULL )
          e2 = e;
      }
      this->path = NULL;
    }

    if ( e2 != NULL )
      throw e2;
  }
}

static inline bool
rai_file_test_would_block( int e )
{
#ifdef EAGAIN
  if ( e == EAGAIN )
    return true;
#endif
#ifdef EWOULDBLOCK
  if ( e == EWOULDBLOCK )
    return true;
#endif
  return false;
}

unsigned int
SysFile::read( void *ptr,  unsigned int nBytes )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  DWORD size;

  size = 0;
  if ( ! ::ReadFile( this->fd, ptr, (DWORD) nBytes, &size, NULL ) ) {
    if ( ::GetLastError() != ERROR_HANDLE_EOF )
      throw FileErr::getErr( FileErr::READ_FAILED );
  }
#else
  ssize_t size;

  size = ::read( this->fd, ptr, (size_t) nBytes );
  if ( size < 0 ) {
    int e = errno;
    if ( rai_file_test_would_block( e ) )
      throw FileErr::getErr( FileErr::WOULD_BLOCK );
    throw FileErr::getErr( FileErr::READ_FAILED );
  }
#endif

  return (unsigned int) size;
}


unsigned int
SysFile::readAt( void *ptr,  unsigned int nBytes,  FileOffset seekPos )

{
#if defined( _WIN32 ) || defined( _WIN64 )
  #if defined( USE_OVERLAPPED )
    DWORD      size;
    OVERLAPPED overlapped;

    /* overlapped doesn't work on win 98 */
    overlapped.OffsetHigh = (DWORD) ( seekPos >> 32 );
    overlapped.Offset     = (DWORD) ( seekPos & 0xffffffffU );
    overlapped.hEvent     = NULL;

    size = 0;
    if ( ! ::ReadFile( this->fd, ptr, (DWORD) nBytes, &size, &overlapped ) ) {
      if ( ::GetLastError() != ERROR_HANDLE_EOF )
        throw FileErr::getErr( FileErr::READ_FAILED );
    }

    return (unsigned int) size;
  #else
    DWORD size;
    LONG  offsetHi,
          offsetLo;

    offsetHi = (LONG) (DWORD) ( seekPos >> 32 );
    offsetLo = (LONG) (DWORD) ( seekPos & 0xffffffffU );

    if ( ::SetFilePointer( this->fd, offsetLo, &offsetHi,
                           FILE_BEGIN ) == INVALID_SET_FILE_POINTER &&
         ::GetLastError() != NO_ERROR ) {
      throw FileErr::getErr( FileErr::SEEK_FAILED );
    }

    size = 0;
    if ( ! ::ReadFile( this->fd, ptr, (DWORD) nBytes, &size, NULL ) ) {
      if ( ::GetLastError() != ERROR_HANDLE_EOF )
        throw FileErr::getErr( FileErr::READ_FAILED );
    }

    return (unsigned int) size;
  #endif
#elif defined( USE_PREAD )
  ssize_t size;

  size = ::pread( this->fd, ptr, (size_t) nBytes, (off_t) seekPos );
  if ( size < 0 ) {
    int e = errno;
    if ( rai_file_test_would_block( e ) )
      throw FileErr::getErr( FileErr::WOULD_BLOCK );
    throw FileErr::getErr( FileErr::READ_FAILED );
  }
  return (unsigned int) size;
#else /* good ol' lseek */
  #warning "using lseek()/read() not pread(), file.readAt() is not thread safe"
  ssize_t size;

  if ( ::lseek( this->fd, (off_t) seekPos, SEEK_SET ) == -1 )
    throw FileErr::getErr( FileErr::SEEK_FAILED );

  size = ::read( this->fd, ptr, (size_t) nBytes );
  if ( size < 0 ) {
    int e = errno;
    if ( rai_file_test_would_block( e ) )
      throw FileErr::getErr( FileErr::WOULD_BLOCK );
    throw FileErr::getErr( FileErr::READ_FAILED );
  }
  return (unsigned int) size;
#endif
}


unsigned int
SysFile::write( const void *ptr,  unsigned int nBytes )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  DWORD size;

  size = 0;
  if ( ! ::WriteFile( this->fd, ptr, (DWORD) nBytes, &size, NULL ) )
    throw FileErr::getErr( FileErr::WRITE_FAILED );
  if ( size == 0 && nBytes > 0 )
    throw FileErr::getErr( FileErr::WRITE_TRUNCATED );

  return (unsigned int) size;
#else
  ssize_t size;

  size = ::write( this->fd, ptr, (size_t) nBytes );
  if ( size < 0 ) {
    int e = errno;
    if ( rai_file_test_would_block( e ) )
      throw FileErr::getErr( FileErr::WOULD_BLOCK );
    throw FileErr::getErr( FileErr::WRITE_FAILED );
  }
  /* size could be < nBytes if fd is a pipe or if there is no space on device */
  if ( size == 0 && nBytes > 0 )
    throw FileErr::getErr( FileErr::WRITE_TRUNCATED );

  return (unsigned int) size;
#endif
}


unsigned int
SysFile::writeAt( const void *ptr,  unsigned int nBytes,  FileOffset seekPos )

{
#if defined( _WIN32 ) || defined( _WIN64 )
  #if defined( USE_OVERLAPPED )
    DWORD      size;
    OVERLAPPED overlapped;

    /* overlapped doesn't work on win 98 */
    overlapped.OffsetHigh = (DWORD) ( seekPos >> 32 );
    overlapped.Offset     = (DWORD) ( seekPos & 0xffffffffU );
    overlapped.hEvent     = NULL;

    size = 0;
    if ( ! ::WriteFile( this->fd, ptr, (DWORD) nBytes, &size, &overlapped ) )
      throw FileErr::getErr( FileErr::WRITE_FAILED );
    if ( size == 0 && nBytes > 0 )
      throw FileErr::getErr( FileErr::WRITE_TRUNCATED );

    return (unsigned int) size;
  #else
    DWORD size;
    LONG  offsetHi,
          offsetLo;

    offsetHi = (LONG) (DWORD) ( seekPos >> 32 );
    offsetLo = (LONG) (DWORD) ( seekPos & 0xffffffffU );

    if ( ::SetFilePointer( this->fd, offsetLo, &offsetHi,
                           FILE_BEGIN ) == INVALID_SET_FILE_POINTER &&
         ::GetLastError() != NO_ERROR ) {
      throw FileErr::getErr( FileErr::SEEK_FAILED );
    }

    size = 0;
    if ( ! ::WriteFile( this->fd, ptr, (DWORD) nBytes, &size, NULL ) )
      throw FileErr::getErr( FileErr::WRITE_FAILED );
    if ( size == 0 && nBytes > 0 )
      throw FileErr::getErr( FileErr::WRITE_TRUNCATED );

    return (unsigned int) size;
  #endif
#elif defined( USE_PREAD )
  ssize_t size;

  size = ::pwrite( this->fd, ptr, (size_t) nBytes, (off_t) seekPos );
  if ( size < 0 ) {
    int e = errno;
    if ( rai_file_test_would_block( e ) )
      throw FileErr::getErr( FileErr::WOULD_BLOCK );
    throw FileErr::getErr( FileErr::WRITE_FAILED );
  }
  if ( size == 0 && nBytes > 0 )
    throw FileErr::getErr( FileErr::WRITE_TRUNCATED );

  return (unsigned int) size;
#else /* good ol' lseek */
  #warning "using lseek()/read() not pread(), file.readAt() is not thread safe"
  ssize_t size;

  if ( ::lseek( this->fd, (off_t) seekPos, SEEK_SET ) == -1 )
    throw FileErr::getErr( FileErr::SEEK_FAILED );

  size = ::write( this->fd, ptr, (size_t) nBytes );
  if ( size < 0 ) {
    int e = errno;
    if ( rai_file_test_would_block( e ) )
      throw FileErr::getErr( FileErr::WOULD_BLOCK );
    throw FileErr::getErr( FileErr::WRITE_FAILED );
  }
  if ( size == 0 && nBytes > 0 )
    throw FileErr::getErr( FileErr::WRITE_TRUNCATED );

  return (unsigned int) size;
#endif
}


FileOffset
SysFile::seekSet( SeekOffset seekPos,  int whence )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  LONG  posLo,
        posHi;
  DWORD dwMoveMethod;

  if ( whence == File::IO_SEEK_SET )
    dwMoveMethod = FILE_CURRENT;
  else if ( whence == File::IO_SEEK_END )
    dwMoveMethod = FILE_END;
  else
    dwMoveMethod = FILE_BEGIN;

  posHi = (LONG) (DWORD) ( seekPos >> 32 );
  posLo = (LONG) (DWORD) ( seekPos & 0xffffffffU );

  if ( (posLo = ::SetFilePointer( this->fd, posLo, &posHi,
                                  dwMoveMethod )) == INVALID_SET_FILE_POINTER &&
       ::GetLastError() != NO_ERROR )
    throw FileErr::getErr( FileErr::SEEK_FAILED );

  return ( (FileOffset) posHi << 32 ) | (FileOffset) posLo;
#else
  off_t pos;

  if ( whence == File::IO_SEEK_SET )
    whence = SEEK_SET;
  else if ( whence == File::IO_SEEK_END )
    whence = SEEK_END;
  else
    whence = SEEK_CUR;

  if ( (pos = ::lseek( this->fd, (off_t) seekPos, whence )) == -1 )
    throw FileErr::getErr( FileErr::SEEK_FAILED );

  return (FileOffset) pos;
#endif
}


FileOffset
SysFile::length( void )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  DWORD fileSizeLo,
        fileSizeHi;

  if ( (fileSizeLo = ::GetFileSize( this->fd,
                                    &fileSizeHi )) == INVALID_FILE_SIZE &&
       ::GetLastError() != NO_ERROR )
    throw FileErr::getErr( FileErr::STAT_FAILED );

  return ( (FileOffset) fileSizeHi << 32 ) | (FileOffset) fileSizeLo;
#else
  struct stat st;

  if ( ::fstat( this->fd, &st ) != 0 )
    throw FileErr::getErr( FileErr::STAT_FAILED );

  return (FileOffset) st.st_size;
#endif
}


TimeMSecs
SysFile::modifiedTime( void )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  BY_HANDLE_FILE_INFORMATION fileInfo;
  SYSTEMTIME                 sysTime;
  FILETIME                   fileTime;
  struct tm                  tm;

  if ( GetFileInformationByHandle( this->fd, &fileInfo ) == 0 )
    throw FileErr::getErr( FileErr::STAT_FAILED );

  FileTimeToLocalFileTime( &fileInfo.ftLastWriteTime, &fileTime );
  FileTimeToSystemTime( &fileTime, &sysTime );
  ::memset( &tm, 0, sizeof( tm ) );
  tm.tm_year = sysTime.wYear - 1900;
  tm.tm_mon  = sysTime.wMonth - 1;
  tm.tm_wday = sysTime.wDayOfWeek;
  tm.tm_mday = sysTime.wDay;
  tm.tm_hour = sysTime.wHour;
  tm.tm_min  = sysTime.wMinute;
  tm.tm_sec  = sysTime.wSecond;

  return (TimeMSecs) (unsigned long) ::mktime( &tm ) * (TimeMSecs) 1000U;
#else
  struct stat st;

  if ( ::fstat( this->fd, &st ) != 0 )
    throw FileErr::getErr( FileErr::STAT_FAILED );

  return (TimeMSecs) (unsigned long) st.st_mtime * (TimeMSecs) 1000U;
#endif
}


void
SysFile::truncate( FileOffset eofPos )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  LONG sizeLo,
       sizeHi;

  sizeHi = (LONG) (DWORD) ( eofPos >> 32 );
  sizeLo = (LONG) (DWORD) ( eofPos & 0xffffffffU );

  /* not be thread safe */
  if ( ::SetFilePointer( this->fd, sizeLo, &sizeHi,
                         FILE_BEGIN ) == INVALID_SET_FILE_POINTER &&
       ::GetLastError() != NO_ERROR )
    throw FileErr::getErr( FileErr::SEEK_FAILED );

  if ( ! ::SetEndOfFile( fd ) )
    throw FileErr::getErr( FileErr::TRUNC_FAILED );
#else
  if ( ::ftruncate( this->fd, (off_t) eofPos ) != 0 )
    throw FileErr::getErr( FileErr::TRUNC_FAILED );
#endif
}


void
SysFile::sync( void )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( ! ::FlushFileBuffers( this->fd ) )
    throw FileErr::getErr( FileErr::SYNC_FAILED );
#else
  if ( ::fsync( this->fd ) != 0 )
    throw FileErr::getErr( FileErr::SYNC_FAILED );
#endif
}


void *
SysFile::map( FileOffset mapSize,  int fileMode,
              FileOffset alignment )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  HANDLE  fm;
  void  * addr;
  int     mode;

  if ( (fileMode & File::FILE_WRONLY) == File::FILE_WRONLY ||
       (fileMode & File::FILE_RDWR) == File::FILE_RDWR )
    mode = PAGE_READWRITE;
  else 
    mode = PAGE_READONLY;

  fm = ::CreateFileMapping( this->fd, NULL, mode, 0, 0, NULL );

  if ( fm == (HANDLE) NULL )
    throw FileErr::getErr( FileErr::MAPFILE_FAILED );

  DWORD off_hi  = 0, off_lo  = 0;
  /*mapSize = 0; ( mapSize + alignment - 1 ) & ~( alignment - 1 );*/

  if ( (fileMode & File::FILE_WRONLY) == File::FILE_WRONLY )
    mode = FILE_MAP_WRITE;
  else if ( (fileMode & File::FILE_RDWR) == File::FILE_RDWR )
    mode = FILE_MAP_READ | FILE_MAP_WRITE;
  else
    mode = FILE_MAP_READ;

  addr = ::MapViewOfFile( fm, mode, off_hi, off_lo, mapSize );
  ::CloseHandle( fm );
  if ( addr == NULL )
    throw FileErr::getErr( FileErr::MAPVIEW_FAILED );

  return addr;
#else
  void * p;
  int    mode;

  if ( (fileMode & File::FILE_WRONLY) == File::FILE_WRONLY )
    mode = PROT_WRITE;
  else if ( (fileMode & File::FILE_RDWR) == File::FILE_RDWR )
    mode = PROT_READ | PROT_WRITE;
  else
    mode = PROT_READ;

  mapSize = ( mapSize + alignment - 1 ) & ~( alignment - 1 );
  p = (void *) ::mmap( 0, mapSize, mode, MAP_SHARED, this->fd, 0 );
  if ( p != MAP_FAILED )
    return p;
  throw FileErr::getErr( FileErr::MAPFILE_FAILED );
#endif
}


void
SysFile::unmap( void *addr,  FileOffset mapSize,  FileOffset alignment )

{
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( ::UnmapViewOfFile( addr ) == 0 )
    throw FileErr::getErr( FileErr::UNMAPFILE_FAILED );
#else
  mapSize = ( mapSize + alignment - 1 ) & ~( alignment - 1 );
#ifdef USE_CADDR_T
  if ( ::munmap( (caddr_t) addr, mapSize ) != 0 )
    throw FileErr::getErr( FileErr::UNMAPFILE_FAILED );
#else
  if ( ::munmap( addr, mapSize ) != 0 )
    throw FileErr::getErr( FileErr::UNMAPFILE_FAILED );
#endif
#endif
}


Error
FileErr::getErr( unsigned int status )
{
  static const char     mod[] = "File";
  static const ErrorRec err[] = {
  /*  0 */ { STAT_FAILED,        "Unable to stat file", mod },
  /*  1 */ { GETCWD_FAILED,      "Unable to get current working directory",
                                 mod },
  /*  2 */ { PATH_LEN_TOO_SMALL, "Buffer too small for path", mod },
  /*  3 */ { CREATE_DIR_FAILED,  "Unable to create directory", mod },
  /*  4 */ { REMOVE_FAILED,      "Unable to remove file", mod },
  /*  5 */ { RENAME_FAILED,      "Unable to rename file", mod },
  /*  6 */ { OPEN_WRITE_FAILED,  "Unable to open file for writing", mod },
  /*  7 */ { OPEN_READ_FAILED,   "Unable to open file for reading", mod },
  /*  8 */ { TEMP_FAILED,        "Unable to create temporary file", mod },
  /*  9 */ { CLOSE_FAILED,       "Unable to close file", mod },
  /* 10 */ { READ_FAILED,        "Unable to read file", mod },
  /* 11 */ { WRITE_FAILED,       "Unable to write file", mod },
  /* 12 */ { WRITE_TRUNCATED,    "Unable to write all data to file, truncated",
                                 mod },
  /* 13 */ { SEEK_FAILED,        "Unable to seek file", mod },
  /* 14 */ { TRUNC_FAILED,       "Unable to truncate file", mod },
  /* 15 */ { SYNC_FAILED,        "Unable to sync file", mod },
  /* 16 */ { MAPFILE_FAILED,     "Unable to memory map file", mod },
  /* 17 */ { UNMAPFILE_FAILED,   "Unable to memory unmap file", mod },
  /* 18 */ { MTIME_FAILED,       "Unable to update file time", mod },
  /* 19 */ { SHM_OPEN_FAILED,    "Shared memory open failed", mod },
  /* 20 */ { MAPVIEW_FAILED,     "Unable to map view of file", mod },
  /* 21 */ { WOULD_BLOCK,        "File read/write would block", mod },
  /* 22 */ { NOT_OPEN,           "File not open", mod },
  /* 23 */ { FCNTL_FAILED,       "Unable to set fcntl flags", mod },
  /* 24 */ { 24,                 "Unknown file error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}

