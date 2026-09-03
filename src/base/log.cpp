/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>
#include "util/array.h"
#include "base/dir.h"
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <syslog.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
typedef int socket_t;
typedef const void * send_arg_t;
typedef socklen_t    addrlen_t;
inline static int closeSocket( socket_t s ) { return ::close( s ); }
#else
#include <time.h>
#include <io.h>
#undef byte
#define FD_SETSIZE MAX_DESCRIPTORS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#define LOG_USER    (1<<3)
#define LOG_DAEMON  (3<<3)
#define LOG_LOCAL0  (16<<3)
#define LOG_LOCAL1  (17<<3)
#define LOG_LOCAL2  (18<<3)
#define LOG_LOCAL3  (19<<3)
#define LOG_LOCAL4  (20<<3)
#define LOG_LOCAL5  (21<<3)
#define LOG_LOCAL6  (22<<3)
#define LOG_LOCAL7  (23<<3)

#define LOG_WINDOWS 256

#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7
typedef SOCKET socket_t;
typedef const char * send_arg_t;
typedef int          addrlen_t;
inline static int closeSocket( socket_t s ) { return ::closesocket( s ); }
#endif

static const struct Facility {
  int          facility;
  const char * name;
} facilities[] = {
#if defined( _WIN32 ) || defined( _WIN64 )
  { LOG_WINDOWS, "LOG_WIN" },
#endif
  { LOG_USER,   "LOG_USER" },
  { LOG_DAEMON, "LOG_DAEMON" },
  { LOG_LOCAL0, "LOG_LOCAL0" },
  { LOG_LOCAL1, "LOG_LOCAL1" },
  { LOG_LOCAL2, "LOG_LOCAL2" },
  { LOG_LOCAL3, "LOG_LOCAL3" },
  { LOG_LOCAL4, "LOG_LOCAL4" },
  { LOG_LOCAL5, "LOG_LOCAL5" },
  { LOG_LOCAL6, "LOG_LOCAL6" },
  { LOG_LOCAL7, "LOG_LOCAL7" },
};

/*#include "net/sock_types.h"*/
#include "base/sys.h"
#include "base/log.h"
#include "base/thread.h"
#include "base/file.h"
#include "util/str_util.h"
#include "stream/file_stream.h"
#include "stream/byte_array_stream.h"
#include "stream/cycle_stream.h"
#include "stream/stdio_stream.h"

using namespace rai;

static const socket_t INVALID_SOCKET = -1;
static const unsigned int defaultVerb = ( Log::VERB_SEVERITY |
                                          Log::VERB_NUMBER |
                                          Log::VERB_REASON |
                                          Log::VERB_DESCR |
                                          Log::VERB_TIMESTAMP ),
                          syslogVerb  = ( Log::VERB_NUMBER |
                                          Log::VERB_REASON |
                                          Log::VERB_DESCR );
static const Log::LogLevel defaultLvl = Log::LVL_NORMAL;
static const unsigned int LOG_BUF_LINE_LEN = 80;

#ifdef __sun__
#define log logOut
#endif
static OutputStream       * log                  = NULL;
static Mutex              * logMutex             = NULL;
static Log::LogLevel        logLevel             = defaultLvl;
Log::LogLevel               Log::minLevel        = defaultLvl;
static unsigned int         logVerbosity         = defaultVerb;
static bool                 logXml               = false,
                            suppressTimestamp    = false,
                            needStartupTimestamp = false,
                            autoRotateCheck      = true;
static ullong               logSizeLimit         = 0;
static TimeRotate           logRotate            = { 0, 0, 0,
                                               TimeRotate::ROTATE_UNSPECIFIED };
static char                 logFileName[ 1024 ];
static CycleOutputStream  * logBufOut            = NULL;
static byte               * logBuf               = NULL;
static unsigned int         logBufLen            = 0,
                            logCount             = 0;
static Log::LogLevel        logBufLevel          = Log::LVL_ERROR;
static void (*rotateCallback)(void *)            = NULL;
static void               * rotateClosure        = 0;

struct SyslogList {
  SyslogList         * next;
  int                  syslogFacility;
  Log::LogLevel        syslogLevel;
  struct sockaddr_in * syslogAddr;
  struct sockaddr_in   syslog_addr_buf;
  unsigned int         syslogVerbosity;
  socket_t             syslogSock;
  char                 syslogHost[ 256 ],
                       syslogProg[ 32 ];

  SYS_OPS( SyslogList );
  SyslogList() {
    this->next            = NULL;
    this->syslogFacility  = 0;
    this->syslogLevel     = Log::LVL_ERROR;
    this->syslogAddr      = NULL;
    ::memset( &this->syslog_addr_buf, 0, sizeof( this->syslog_addr_buf ) );
    this->syslogVerbosity = syslogVerb;
    this->syslogSock      = INVALID_SOCKET;
    this->syslogHost[ 0 ] = '\0';
    this->syslogProg[ 0 ] = '\0';
  }
  ~SyslogList() {
    if ( this->syslogSock != INVALID_SOCKET )
      closeSocket( this->syslogSock );
  }
};

class FileTime  {
    public:
    char      * fileName;
    TimeMSecs   modTime;

    FileTime( const char * fileName, TimeMSecs modTime ) {
      this->fileName = NULL;
      STRDUP( this->fileName, fileName );
      this->modTime = modTime;
    };

    ~FileTime() {
      if ( fileName ) {
        FREE( fileName );
        fileName = NULL;
      }
    };
};

// List of log files ordered by time for rollover
class FileList : public HeapSortArray<FileTime *>
{
    public:
    SYS_OPS( FileList );

    FileList() : HeapSortArray<FileTime *>( 32 ) { };
    virtual int compare( FileTime * ft1, FileTime * ft2 ) {
      return ( ft1->modTime == ft2->modTime ? 0 : ft1->modTime < ft2->modTime ? -1 : 1 );
    };
    FileTime * addFile( const char * directory, const char * fileName ) {
      char path[1024];
      TimeMSecs modTime;
      ::strncpy( path, directory, sizeof(path) );
      ::strncat( path, fileName, sizeof(path) );
      modTime = File::fileModifiedTime( path );
      FileTime * fileTime = NEW FileTime( fileName, modTime );
      push( fileTime );
      return fileTime;
    };
    void removeAll( void ) {
      FileTime * fileTime;
      while( length() > 0 ) {
        fileTime = pop();
        delete fileTime;
      }
    };
    ~FileList() {
      removeAll();
    };
};

// Class to rename log files and remove them as required.
// File names may be eiher based on a time stamp or a number
// Files are storted by their modified time. This assumes they
// are not modified manualy.

class LogRenamer {
    FileList            files;
    char                logFileDir[1024];       // Directory containing logFilePath
    char                logFilePath[1024];      // Full path to log file.
    char              * logFileName;            // Just the file name part of path
    unsigned int        logFileNameLen;
    Log::LogRolloverType rolloverType;
    unsigned int        rolloverCnt;            // 
    char              * logTimeStampFmt;

    unsigned int findLogFiles() {
      char              file[1024];
      unsigned int      usedLen;
      Dir             * dir;
      
      dir = Dir::openDir( logFileDir );

      while( dir->read(file, sizeof(file), &usedLen ) ) {
        if( strncmp( logFileName, file, logFileNameLen) == 0 ) {
          files.addFile( logFileDir, file );
        }
      }
      return files.length();
    }      

    void renameWithTime() {
      TimeMSecs         currTime;
      char              logFileName2[1024];
      int               len;
      
      findLogFiles();

      // printf("Date File Cnt %d\n", files.length() );
      if ( rolloverCnt ) {
        while ( files.length() > rolloverCnt ) {
          FileTime * fileTime = files.pop();
          str_copy( logFileName2, logFileDir, sizeof( logFileName2 ) );
          str_cat( logFileName2, fileTime->fileName, sizeof( logFileName2 ) );
          // printf("Date Remove file %s\n", logFileName2 );
          File::removeFile( logFileName2 );
          delete fileTime;
        }
      }
      currTime = Time::currentTimeMillisecs();
      
      ::strcpy( logFileName2, logFilePath );
      len = ::strlen( logFileName2 );
      Time::strftime( Time::TZ_LOCAL_TIME, currTime, logTimeStampFmt,
                      &logFileName2[ len ], sizeof( logFileName2 ) - len );
      File::renameFile( logFilePath, logFileName2 );
      // printf("Date Renamed %s to %s\n", logFilePath, logFileName2 );
      files.removeAll();
      // printf("Rename done\n");
    }
    
    void renameWithNum() {
      char srcpath[1024], dstpath[1024];
      unsigned int fileCnt;
      
      findLogFiles(); // Just need count of files
      // printf("File Cnt %d\n", files.length() );      
      fileCnt = files.length();
      while( fileCnt > 1 ) {
        if( rolloverCnt && fileCnt >= rolloverCnt ) {
          if( snprintf( dstpath, sizeof(dstpath), "%s%s.%d", logFileDir, logFileName, fileCnt) > 0 ) {
            if( File::fileExists( dstpath ) ) { 
              // printf("Removed %s \n", srcpath );
              File::removeFile( dstpath );
            }
          }
        } else {
          if( snprintf( srcpath, sizeof(dstpath), "%s%s.%d", logFileDir, logFileName, fileCnt-1) > 0 &&
              snprintf( dstpath, sizeof(dstpath), "%s%s.%d", logFileDir, logFileName, fileCnt) > 0 ) {
            if( File::fileExists( srcpath ) ) {
              // printf("Rename %s to %s\n", srcpath, dstpath );
              TimeMSecs modTime = File::fileModifiedTime( srcpath );
              File::renameFile( srcpath, dstpath ); // file.n-1 -> file.n
              File::setModifiedTime( dstpath, modTime );
            }
          }
        }
        fileCnt--; 
      }
      // filename.log -> filename.log.1
      if( snprintf( srcpath, sizeof(dstpath), "%s%s", logFileDir, logFileName) > 0 &&
          snprintf( dstpath, sizeof(dstpath), "%s%s.%d", logFileDir, logFileName, fileCnt) > 0 ) {
        if( File::fileExists( srcpath ) ) {
          // printf("Rename %s to %s\n", srcpath, dstpath );
          TimeMSecs modTime = File::fileModifiedTime( srcpath );
          File::renameFile( srcpath, dstpath ); // file -> file.1
          File::setModifiedTime( dstpath, modTime );
        }
      }
      // printf("Rename done\n");
      files.removeAll();
    }

    void setPath( const char * path ) {
      unsigned int      len;
      char            * p;
      if ( ! path )
        throw LogErr::getErr( LogErr::NULL_PATH );
      len = ::strlen( path );
      if ( len == 0 || len >= sizeof( logFileDir ) - 3 )
        throw LogErr::getErr( LogErr::NULL_PATH );
      ::memcpy( logFilePath, path, len ); logFilePath[ len ] = '\0';
      ::memcpy( logFileDir, path, len ); logFileDir[ len ] = '\0';

#if defined( _WIN32 ) || defined( _WIN64 )
      // Windows paths suck. https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats
      // https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstfilea
      // Find desired separator
      char sep = '/';
      if ( ::strchr( logFileDir, '/' ) == NULL && ::strchr( logFileDir, '\\' ) != NULL ) {
        sep = '\\';
        ::strcpy( logFileDir, "\\*" ); // path didn't include directory, so search in CWD
      } else {
        p = strrchr( logFileDir, sep );
        *++p = '*';
        *++p ='\0';
      }
#else
      /* File paths:
         somepath/logfile
         /somepath/logfile
         ./somepath/logfile
         ../somepath/logfile
         ./logfile
         ../logfile
         /logfile
         logfile
       */
      if ( ( p = ::strrchr( logFileDir, '/' )) != NULL ) {
        logFileName = logFilePath + (++p - logFileDir);
        *p = '\0';            // lop off logFileName from directory
      } else {
        logFileName = logFilePath;
        logFileDir[0] = '.';
        logFileDir[1] = '/';
        logFileDir[2] = '\0';   // logFileName contained logfile
      } 
#endif
      logFileNameLen = strlen( logFileName);
    }

    public:
    LogRenamer() {
      this->rolloverType        = Log::ROLLOVER_DATE;
      this->rolloverCnt         = 0;
      this->logFileName         = NULL;
      this->logFileNameLen      = 0;
      this->logFileDir[0]       = 0;
      this->logFilePath[0]      = 0;
      this->logTimeStampFmt     = NULL;
#if defined( _WIN32 ) || defined( _WIN64 )
      STRDUP( this->logTimeStampFmt, (char *)".%Y-%m-%d_%H-%M-%S" );
#else
      STRDUP( this->logTimeStampFmt, (char *)".%Y-%m-%d_%H:%M:%S" );
#endif
    }

    void setRolloverCnt( unsigned int rolloverCnt ) {
      this->rolloverCnt = rolloverCnt;
    }
    
    void setRolloverType( Log::LogRolloverType rolloverType ) {
      this->rolloverType = rolloverType;
    }

    void setLogTimeStampFmt( const char * logTimeStampFmt ) {
      if( this->logTimeStampFmt ) {
        FREE( this->logTimeStampFmt );
        this->logTimeStampFmt = NULL;
      }
      STRDUP( this->logTimeStampFmt, logTimeStampFmt );
    }
    
    void rename(const char * path) {
      setPath( path );

      switch( rolloverType ) {
        case Log::ROLLOVER_DATE:
          renameWithTime();
          break;
        case Log::ROLLOVER_NUM:
          renameWithNum();
          break;
        case Log::ROLLOVER_UNK:
        default:
          throw( LogErr::getErr( LogErr::BAD_ROLL_TYPE ) );
      }
    }
};

static LogRenamer   renamer;
static SyslogList * syslogs         = NULL;
static bool         syslogDisabled  = false;

#ifdef VALGRIND
  bool Log::dologDevel() { return Log::minLevel == Log::LVL_DEVEL; };
  bool Log::dologFTrace() { return Log::minLevel <= Log::LVL_FTRACE; };
  bool Log::dologTrace() { return Log::minLevel <= Log::LVL_TRACE; };
  bool Log::dologDebug() { return Log::minLevel <= Log::LVL_DEBUG; };
  bool Log::dologMinor() { return Log::minLevel <= Log::LVL_MINOR; };
  bool Log::dologNormal() { return Log::minLevel <= Log::LVL_NORMAL; };
  bool Log::dovlogDevel() { return Log::minLevel == Log::LVL_DEVEL; };
  bool Log::dovlogFTrace() { return Log::minLevel <= Log::LVL_FTRACE; };
  bool Log::dovlogTrace() { return Log::minLevel <= Log::LVL_TRACE; };
  bool Log::dovlogDebug() { return Log::minLevel <= Log::LVL_DEBUG; };
  bool Log::dovlogMinor() { return Log::minLevel <= Log::LVL_MINOR; };
  bool Log::dovlogNormal() { return Log::minLevel <= Log::LVL_NORMAL; };
#endif

static void
rai_log_lock( void )
{
  if ( logMutex == NULL )
    logMutex = Mutex::create( Mutex::RECURSIVE_LOCK );
  /* disable pthread_cancel while locked */
  Thread::disableCancelState();
  logMutex->lock();
}

static void
rai_log_unlock( void )
{
  if ( logMutex != NULL ) {
    logMutex->unlock();
    /* enable thread_cancel while unlocked */
    Thread::enableCancelState();
  }
}

Log::LogLevel
Log::parseLogLevel( const char *levelName )
{
  if ( StrUtil::strcasecmp( levelName, "devel" ) == 0 ||
       StrUtil::strcasecmp( levelName, "dev" ) == 0 )
    return Log::LVL_DEVEL;
  if ( StrUtil::strcasecmp( levelName, "ftrace" ) == 0 ||
       StrUtil::strcasecmp( levelName, "ftr" ) == 0 )
    return Log::LVL_FTRACE;
  if ( StrUtil::strcasecmp( levelName, "trace" ) == 0 ||
       StrUtil::strcasecmp( levelName, "tr" ) == 0 )
    return Log::LVL_TRACE;
  if ( StrUtil::strcasecmp( levelName, "debug" ) == 0 ||
       StrUtil::strcasecmp( levelName, "dbg" ) == 0 )
    return Log::LVL_DEBUG;
  if ( StrUtil::strcasecmp( levelName, "minor" ) == 0 ||
       StrUtil::strcasecmp( levelName, "min" ) == 0 )
    return Log::LVL_MINOR;
  if ( StrUtil::strcasecmp( levelName, "normal" ) == 0 ||
       StrUtil::strcasecmp( levelName, "norm" ) == 0 )
    return Log::LVL_NORMAL;
  return Log::LVL_ERROR;
}

const char *
Log::levelToString( LogLevel level )
{
  switch ( level ) {
    default:
    case LVL_DEBUG: return "debug";
    case LVL_DEVEL: return "devel";
    case LVL_FTRACE: return "ftrace";
    case LVL_TRACE: return "trace";
    case LVL_MINOR: return "minor";
    case LVL_NORMAL: return "normal";
    case LVL_ERROR: return "error";
  }
}

Log::LogRolloverType
Log::parseLogRolloverType( const char *rolloverType )
{
  if ( StrUtil::strcasecmp( rolloverType, "date" ) == 0 ||
       StrUtil::strcasecmp( rolloverType, "time" ) == 0 )
    return Log::ROLLOVER_DATE;
  if ( StrUtil::strcasecmp( rolloverType, "num" ) == 0 ||
       StrUtil::strcasecmp( rolloverType, "number" ) == 0 )
    return Log::ROLLOVER_NUM;
  throw( LogErr::getErr( LogErr::BAD_ROLL_TYPE ) );
}

const char *
Log::logRolloverTypeToString( Log::LogRolloverType logRolloverType )
{
  switch ( logRolloverType ) {
    case Log::ROLLOVER_DATE: return "date";
    case Log::ROLLOVER_NUM:  return "num";
    default:
      throw( LogErr::getErr( LogErr::BAD_ROLL_TYPE ) );  
  }
}

static void
updateMinLevel( void )
{
  Log::LogLevel tmpLevel = logLevel;

  for ( SyslogList *el = syslogs; el != NULL; el = el->next ) {
    if ( el->syslogLevel < tmpLevel )
      tmpLevel = el->syslogLevel;
  }
  if ( logBufLevel < tmpLevel )
    tmpLevel = logBufLevel;

  Log::minLevel = tmpLevel;
}


// Log to output stream
void
Log::openLog( OutputStream *os,  LogLevel level,  unsigned int verbosity,
              bool useXml )
{
  OutputStream * out;
  Error          e2;

  e2 = NULL;
  if ( log != Sys::err ) {
    out = log;
    /*log = Sys::err;*/
    log = NULL;
    if ( out != NULL )
      delete out;
  }

  if ( logMutex == NULL )
    logMutex = Mutex::create( Mutex::RECURSIVE_LOCK );

  logLevel = Log::LVL_NORMAL;
  logXml   = useXml;
  Log::setVerbosity( verbosity );

  /* still open log if verbosity is 0, it may change during program execution */
  try {
    setAutoRotate( false );
    logRotate.setLastTime( 0 );
    log = os;
    const char *logName = "_internal_";
    ::strncpy( logFileName, logName, sizeof( logFileName ) );
    suppressTimestamp    = false;
    needStartupTimestamp = true;
  } catch ( Error e ) {
    e2 = e;
  }

  logLevel = level;
  updateMinLevel();

  if ( e2 != NULL )
    throw e2;
}


static int rai_enable_crash_logging;

static const char hc[ 16 ] =
    {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

static void
log_write_stack( byte *tos, char *buf,
                 void (*write_bytes)( const char *, unsigned int ) )
{
  unsigned int i, j, k, l, m;
  unsigned int stkSize = 0, zcnt = 0;
  bool endofstack = false;
  byte b;
  while ( ( ( (ptrdiff_t) (void *) tos ) & 0xf ) != 0 )
    tos -= sizeof( void * );
  ::strcpy( buf, "     0:  " );
  unsigned int offset = (unsigned int) (ptrdiff_t) (void *) tos;
  for ( j = 5, k = offset; k > 0; ) {
    buf[ j ] = hc[ k & 0xf ];
    if ( j-- == 0 )
      break;
    k >>= 4;
  }
  for ( i = 0; ! endofstack; ) {
    for ( k = 0; k < 16; k++ ) {
      if ( stkSize >= 8 && tos[ stkSize - 8 ] == 'T' &&
           ::strcmp( (const char *) &tos[ stkSize - 8 ], "THRBASE" ) == 0 ) {
        endofstack = true;
        break;
      }
      stkSize++;
    }
    k = 9;
    l = 61;
    m = i;
    b = 0;
    for ( j = 0; j < 16 && m < stkSize; m++ ) {
      b |= tos[ m ];
      buf[ k++ ] = hc[ tos[ m ] >> 4 ];
      buf[ k++ ] = hc[ tos[ m ] & 0xf ];
      buf[ k++ ] = ' ';
      buf[ l++ ] = ( tos[ m ] >= ' ' && tos[ m ] < 127 ) ? tos[ m ] : '.';
      if ( ( ++j & 0x3 ) == 0 )
        buf[ k++ ] = ' ';
    }
    while ( k < 61 )
      buf[ k++ ] = ' ';
    buf[ l++ ] = '\n';
    if ( b == 0 )
      zcnt++;
    else
      zcnt = 0;
    if ( zcnt == 0 )
      (*write_bytes)( buf, l );
    else if ( zcnt == 1 )
      (*write_bytes)( "...\n", 4 );
    ::strcpy( buf, "     0:  " );
    i += 16;
    k  = i + offset;
    for ( j = 5; k > 0; ) {
      buf[ j ] = hc[ k & 0xf ];
      if ( j-- == 0 )
        break;
      k >>= 4;
    }
  }
  if ( stkSize > 0 ) {
    ::strcpy( buf, "     0:\n" );
    stkSize += offset;
    for ( j = 5; stkSize > 0; ) {
      buf[ j ] = hc[ stkSize & 0xf ];
      if ( j-- == 0 )
        break;
      stkSize >>= 4;
    }
    (*write_bytes)( buf, 8 );
  }
}


#if defined( __linux ) || defined( __sun )
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <execinfo.h>
#include <stdlib.h>
#include <ucontext.h>

static int rai_log_fd = 2;
static bool rai_handler_installed;

static void log_write_seperator( char *buf ) {
  ::memset( buf, '-', 78 ); buf[ 78 ] = '\n';
  (void ) ::write( rai_log_fd, buf, 79 );
}

static void log_write_string( const char *buf ) {
  (void ) ::write( rai_log_fd, buf, ::strlen( buf ) );
}

static void log_write_bytes( const char *buf,  unsigned int len ) {
  (void ) ::write( rai_log_fd, buf, len );
}

static void
rai_log_crash_handler( int sig,  siginfo_t *nfo,  void *uctx )
{
#if defined( __amd64__ )
  static const char *regnm[] = {
   "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rdi",
   "rsi", "rbp", "rbx", "rdx", "rax", "rcx", "rsp", "rip", "efl",
   "csgsfs", "err", "trapno", "oldmask", "cr2", NULL
  };
#elif defined( __i386 )
  static const char *regnm[] = {
   "gs", "fs", "es", "ds", "edi", "esi", "ebp", "esp", "ebx", "edx",
   "ecx", "eax", "trapno", "err", "eip", "cs", "efl", "uesp", "ss", NULL
  };
#elif defined( __arm__ )
  static const char *regnm[] = {
   "trap", "code", "oldmask", "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
   "r8", "r9", "r10", "fp", "ip", "sp", "lr", "pc", "cpsr", "fault", NULL
  };
#endif
#define PTR_BITS  ( sizeof( void * ) * 8 )
#define PTR_BYTES ( sizeof( void * ) )
  unsigned int j, k;
#define write_register( nm, x ) \
  k = ::strlen( nm ); ::strcpy( buf, nm ); \
  buf[ k++ ] = '='; buf[ k++ ] = '0'; buf[ k++ ] = 'x'; \
  for ( j = 0; j < PTR_BYTES-1; j++ ) { \
    if ( ( (((unsigned long) (void *) x) >> (PTR_BITS - (j*8+4))) & 0xf ) != 0 || \
       ( ( (((unsigned long) (void *) x) >> (PTR_BITS - (j*8+8))) & 0xf ) != 0 ) ) \
      break; \
  } \
  for ( ; j < PTR_BYTES; j++ ) { \
    buf[ k++ ] = hc[ (((unsigned long) (void *) x) >> (PTR_BITS - (j*8+4))) & 0xf ]; \
    buf[ k++ ] = hc[ (((unsigned long) (void *) x) >> (PTR_BITS - (j*8+8))) & 0xf ]; \
  } \
  (void) ::write( rai_log_fd, buf, k )

  static volatile int already_crashed;
  void *array[ 40 ];
  char buf[ 256 ];
  size_t size;
  int fd, n;
  byte * tos;
  ::strcpy( buf, "Error: Signal " );
  if ( sig > 9 )
    buf[ 14 ] = ( ( sig / 10 ) % 10 ) + '0';
  else
    buf[ 14 ] = ' ';
  buf[ 15 ] = ( sig % 10 ) + '0';
  if ( sig == SIGSEGV )
    ::strcpy( &buf[ 16 ], " (segmentation fault) caught:\n" );
  else if ( sig == SIGILL )
    ::strcpy( &buf[ 16 ], " (illegal instruction) caught:\n" );
  else if ( sig == SIGABRT )
    ::strcpy( &buf[ 16 ], " (abort) caught:\n" );
  else if ( sig == SIGFPE )
    ::strcpy( &buf[ 16 ], " (floating point exception) caught:\n" );
  else
    ::strcpy( &buf[ 16 ], " caught.\n" );
  log_write_string( buf );
  if ( already_crashed++ != 0 )
    exit( 1 );
  log_write_seperator( buf );
  if ( sig == SIGSEGV ) {
    log_write_string( "Fault address: " );
    write_register( "ptr", nfo->si_addr );
    (void) ::write( rai_log_fd, "\n", 1 );
  }
#if defined( __amd64__ ) || defined( __i386 )
  ucontext_t *uc = (ucontext_t *) uctx;
  log_write_string( "Regs: " );
  write_register( regnm[ 0 ], uc->uc_mcontext.gregs[ 0 ] );
  for ( unsigned int r = 1; r < sizeof( uc->uc_mcontext.gregs ) /
                                sizeof( uc->uc_mcontext.gregs[ 0 ] ); r++ ) {
    if ( regnm[ r ] == NULL ) break;
    (void) ::write( rai_log_fd, " ", 1 );
    write_register( regnm[ r ], uc->uc_mcontext.gregs[ r ] );
  }
  (void) ::write( rai_log_fd, "\n", 1 );
  log_write_seperator( buf );
#elif defined( __arm__ )
  ucontext_t *uc = (ucontext_t *) uctx;
  log_write_string( "Regs: " );
  write_register( regnm[ 0 ], uc->uc_mcontext.trap_no );
  for ( unsigned int r = 1; r < sizeof( uc->uc_mcontext ) /
                                sizeof( uc->uc_mcontext.trap_no ); r++ ) {
    if ( regnm[ r ] == NULL ) break;
    (void) ::write( rai_log_fd, " ", 1 );
    write_register( regnm[ r ], (&uc->uc_mcontext.trap_no)[ r ] );
  }
  (void) ::write( rai_log_fd, "\n", 1 );
  log_write_seperator( buf );
#endif

  log_write_string( "Process map:\n" );
  log_write_seperator( buf );
  if ( (fd = ::open( "/proc/self/maps", O_RDONLY )) >= 0 ) {
    while ( ( n = ::read( fd, buf, sizeof( buf ) ) ) > 0 )
      (void) ::write( rai_log_fd, buf, n );
  }
  log_write_seperator( buf );

  log_write_string( "Stack trace:\n" );
  log_write_seperator( buf );
  size = ::backtrace( array, 40 );
  ::backtrace_symbols_fd( array, size, rai_log_fd );
  log_write_seperator( buf );
  log_write_string( "Thread stack:\n" );
  log_write_seperator( buf );

  log_write_stack( (byte *) &tos, buf, log_write_bytes );

  exit( 1 );
}
#elif defined( _WIN32 ) || defined( _WIN64 )
#include <eh.h>
#include <psapi.h>
#include <dbghelp.h>

#include "stream/stdio_stream.h"

static inline DWORD int_to_buf( DWORD64 x,  char *p,  DWORD64 base ) {
  DWORD64 i;
  DWORD   len = 0;
  for ( i = base; ; i *= base ) {
    if ( x / i == 0 )
      break;
  }
  if ( base == 16 ) {
    *p++ = '0';
    *p++ = 'x';
    len = 2;
  }
  for (;;) {
    i /= base;
    *p++ = hc[ ( x / i ) ];
    len++;
    if ( i == 1 )
      break;
    x = x % i;
  }
  *p = '\0';
  return len;
}

static HANDLE rai_log_fd;
static bool rai_handler_installed;

static void log_write_seperator( char *buf ) {
  ::memset( buf, '-', 78 ); buf[ 78 ] = '\n';
  DWORD sz = 0; ::WriteFile( rai_log_fd, buf, 79, &sz, NULL );
}

static void log_write_string( const char *buf ) {
  DWORD sz = 0; ::WriteFile( rai_log_fd, buf, ::strlen( buf ), &sz, NULL );
}

static void log_write_bytes( const char *buf,  unsigned int len ) {
  DWORD sz = 0; ::WriteFile( rai_log_fd, buf, len, &sz, NULL );
}
#pragma optimize("", off)

static LONG WINAPI
rai_log_crash_handler( EXCEPTION_POINTERS *r )
{
  unsigned int i, frames;
  void       * stack[ 32 ];
  char         buf[ 256 ];
  struct {
   IMAGEHLP_SYMBOL64 symbol;
   char buf[ 256 ];
  } sbuf;
  HANDLE  process;
  HMODULE dbgh, mods[ 256 ];
  DWORD   n, sz;
  //MINIDUMP_EXCEPTION_INFORMATION mdei; 
  typedef BOOL (*TsymInit)(HANDLE, PCSTR, BOOL);
  typedef BOOL (*TgetSymA)(HANDLE, DWORD64, DWORD64, PIMAGEHLP_SYMBOL64);
  typedef BOOL (*TminiDump)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
                            PMINIDUMP_EXCEPTION_INFORMATION,
                            PMINIDUMP_USER_STREAM_INFORMATION,
                            PMINIDUMP_CALLBACK_INFORMATION );
  typedef BOOL (*TstackWalk)( DWORD, HANDLE, HANDLE, LPSTACKFRAME64,
                              PVOID, PREAD_PROCESS_MEMORY_ROUTINE64,
                              PFUNCTION_TABLE_ACCESS_ROUTINE64,
                              PGET_MODULE_BASE_ROUTINE64,
                              PTRANSLATE_ADDRESS_ROUTINE64 );
  TsymInit   symInit = NULL;
  TgetSymA   getSymA = NULL;
  TminiDump  writeDump = NULL;
  TstackWalk stackWalk = NULL;
  PFUNCTION_TABLE_ACCESS_ROUTINE64 funcTab = NULL;
  PGET_MODULE_BASE_ROUTINE64 getMod = NULL;
  STACKFRAME64 stack_frame;
  byte       * tos;

  log_write_seperator( buf );
  log_write_string( "Exception\n" );
  log_write_seperator( buf );
  process = ::GetCurrentProcess();

  dbgh = ::LoadLibrary( "dbghelp.dll" );
  if ( dbgh != NULL ) {
    symInit = (TsymInit) ::GetProcAddress( dbgh, "SymInitialize" );
    getSymA = (TgetSymA) ::GetProcAddress( dbgh, "SymGetSymFromAddr64" );
    writeDump = (TminiDump) ::GetProcAddress( dbgh, "MiniDumpWriteDump" );
    stackWalk = (TstackWalk) ::GetProcAddress( dbgh, "StackWalk64" );
    funcTab   = (PFUNCTION_TABLE_ACCESS_ROUTINE64)
                ::GetProcAddress( dbgh, "SymFunctionTableAccess64" );
    getMod    = (PGET_MODULE_BASE_ROUTINE64)
                ::GetProcAddress( dbgh, "SymGetModuleBase64" );
  }
  if ( dbgh == NULL || symInit == NULL || getSymA == NULL ||
       stackWalk == NULL ) {
    log_write_string( "no DbgHelp dll\n" );
  }
  else {
    (*symInit)( process, NULL, TRUE );

    if ( r == NULL ) {
      frames = ::CaptureStackBackTrace( 0, sizeof( stack ) / sizeof( stack[0] ),
                                        stack, NULL );
    }
    else {
      frames = 0;
      // Initialize stack walking.
      memset( &stack_frame, 0, sizeof( stack_frame ) );
    #if defined( _WIN64 )
      int machine_type = IMAGE_FILE_MACHINE_AMD64;
      stack_frame.AddrPC.Offset = r->ContextRecord->Rip;
      stack_frame.AddrFrame.Offset = r->ContextRecord->Rbp;
      stack_frame.AddrStack.Offset = r->ContextRecord->Rsp;

      stack_frame.AddrPC.Mode = AddrModeFlat;
      stack_frame.AddrFrame.Mode = AddrModeFlat;
      stack_frame.AddrStack.Mode = AddrModeFlat;
      while ( (*stackWalk)( machine_type, process, GetCurrentThread(),
                            &stack_frame, r->ContextRecord, NULL,
                            funcTab, getMod, NULL ) &&
             frames < sizeof( stack ) / sizeof( stack[ 0 ] ) ) {
        stack[ frames++ ] = (void *) ( stack_frame.AddrPC.Offset );
      }
    #else
    /*  machine_type = IMAGE_FILE_MACHINE_I386;
      stack_frame.AddrPC.Offset = r->ContextRecord->Eip;
      stack_frame.AddrFrame.Offset = r->ContextRecord->Ebp;
      stack_frame.AddrStack.Offset = r->ContextRecord->Esp;*/
    #endif
    }
    log_write_seperator( buf );
    log_write_string( "Stack trace:\n" );
    log_write_seperator( buf );
    for( i = 0; i < frames; i++ ) {
      memset( &sbuf, 0, sizeof( sbuf ) );
      sbuf.symbol.MaxNameLength = sizeof( sbuf.buf ) - 1;
      sbuf.symbol.SizeOfStruct  = sizeof( IMAGEHLP_SYMBOL64 );

      (*getSymA)( process, ( DWORD64 )( stack[ i ] ), 0, &sbuf.symbol );
      n = int_to_buf( (DWORD64) ( frames - i - 1 ), buf, 10 );
      sz = 0; ::WriteFile( rai_log_fd, buf, n, &sz, NULL );
      sz = 0; ::WriteFile( rai_log_fd, " ", 1, &sz, NULL );
      n = strlen( sbuf.symbol.Name );
      if ( n > 0 ) {
        sz = 0; ::WriteFile( rai_log_fd, sbuf.symbol.Name, n, &sz, NULL );
      }
      sz = 0; ::WriteFile( rai_log_fd, " - ", 3, &sz, NULL );
      n = int_to_buf( sbuf.symbol.Address, buf, 16 );
      sz = 0; ::WriteFile( rai_log_fd, buf, n, &sz, NULL );
      sz = 0; ::WriteFile( rai_log_fd, "\n", 1, &sz, NULL );
    }
  }

  if ( r != NULL ) {
    #define write_int_label( S, N ) \
      sz = 0; ::WriteFile( rai_log_fd, S, strlen( S ), &sz, NULL ); \
      n = int_to_buf( (DWORD64) N, buf, 16 ); \
      buf[ n++ ] = '\n'; \
      sz = 0; ::WriteFile( rai_log_fd, buf, n, &sz, NULL );
    log_write_seperator( buf );
    log_write_string( "Exception Record:\n" );
    log_write_seperator( buf );
    if ( r->ExceptionRecord != NULL ) {
      write_int_label( "Code  =", r->ExceptionRecord->ExceptionCode );
      write_int_label( "Flags =", r->ExceptionRecord->ExceptionFlags );
      write_int_label( "Addr  =", r->ExceptionRecord->ExceptionAddress );
      write_int_label( "Parm  =", r->ExceptionRecord->NumberParameters );
    }
    log_write_seperator( buf );
    log_write_string( "Context Record:\n" );
    log_write_seperator( buf );
    if ( r->ContextRecord != NULL ) {
      write_int_label( "eflags=", r->ContextRecord->EFlags );
#if defined( _WIN64 )
      write_int_label( "rax   =", r->ContextRecord->Rax );
      write_int_label( "rcx   =", r->ContextRecord->Rcx );
      write_int_label( "rdx   =", r->ContextRecord->Rdx );
      write_int_label( "rbx   =", r->ContextRecord->Rbx );
      write_int_label( "rsp   =", r->ContextRecord->Rsp );
      write_int_label( "rbp   =", r->ContextRecord->Rbp );
      write_int_label( "rsi   =", r->ContextRecord->Rsi );
      write_int_label( "rdi   =", r->ContextRecord->Rdi );
      write_int_label( "r8    =", r->ContextRecord->R8 );
      write_int_label( "r9    =", r->ContextRecord->R9 );
      write_int_label( "r10   =", r->ContextRecord->R10 );
      write_int_label( "r11   =", r->ContextRecord->R11 );
      write_int_label( "r12   =", r->ContextRecord->R12 );
      write_int_label( "r13   =", r->ContextRecord->R13 );
      write_int_label( "r14   =", r->ContextRecord->R14 );
      write_int_label( "r15   =", r->ContextRecord->R15 );
      write_int_label( "rip   =", r->ContextRecord->Rip );
#else
      write_int_label( "eax   =", r->ContextRecord->Eax );
      write_int_label( "ecx   =", r->ContextRecord->Ecx );
      write_int_label( "edx   =", r->ContextRecord->Edx );
      write_int_label( "ebx   =", r->ContextRecord->Ebx );
      write_int_label( "esp   =", r->ContextRecord->Esp );
      write_int_label( "ebp   =", r->ContextRecord->Ebp );
      write_int_label( "eip   =", r->ContextRecord->Eip );
#endif
    }
  }

  n = sizeof( mods );
  if ( ::EnumProcessModulesEx( process, mods, sizeof( mods ),
                               &n, LIST_MODULES_ALL ) ) {
    log_write_seperator( buf );
    log_write_string( "Process Modules:\n" );
    log_write_seperator( buf );

    n /= sizeof( mods[ 0 ] );
    for ( unsigned int i = 0; i < n; i++ ) {
      MODULEINFO info;
      if ( ::GetModuleInformation( process, mods[ i ], &info,
                                   sizeof( info ) ) ) {
        sz = int_to_buf( (DWORD64) info.lpBaseOfDll, buf, 16 );
        buf[ sz++ ] = '-';
        log_write_bytes( buf, sz );
        sz = int_to_buf( (DWORD64) info.lpBaseOfDll +
                         (DWORD64) info.SizeOfImage, buf, 16 );
        buf[ sz++ ] = ' ';
        log_write_bytes( buf, sz );
      }
      if ( (sz = ::GetModuleFileNameEx( process, mods[ i ],
                                        buf, sizeof( buf ) - 1 )) > 0 ) {
        buf[ sz++ ] = '\n';
        log_write_bytes( buf, sz );
      }
    }
  }

  log_write_seperator( buf );
  log_write_string( "Thread stack:\n" );
  log_write_seperator( buf );

  log_write_stack( (byte *) &tos, buf, log_write_bytes );

/*  mdei.ThreadId           = GetCurrentThreadId(); 
  mdei.ExceptionPointers  = r; 
  mdei.ClientPointers     = FALSE; 

  if ( writeDump != NULL )
    (*writeDump)( process, GetCurrentProcessId(), 
                  rai_log_fd, MiniDumpNormal, (r != NULL) ? &mdei : 0, 0, 0 ); 
*/

  ::ExitProcess( 1 );
  return 1;
}
#pragma optimize("", on)
#endif

static OutputStream *
rai_open_log_file( const char *path )
{
  OutputStream * out  = NULL;
  File         * file = NULL;

  /* 0 = check env var, 1 = not set, 2 = is set */
  if ( rai_enable_crash_logging < 2 ) {
    if ( rai_enable_crash_logging == 0 )
      rai_enable_crash_logging =
        ( ::getenv( "RAI_CRASH_HANDLER" ) != NULL ? 2 : 1 );
    if ( rai_enable_crash_logging < 2 )
      return FileOutputStream::append( path, 8 * 1024, true );
  }
#if defined( _WIN32 ) || defined( _WIN64 )
  HANDLE fd;

  if ( ::strcmp( path, STDOUT_SPECIAL_FILE ) == 0 )
    fd = ::GetStdHandle( STD_OUTPUT_HANDLE );
  else if ( ::strcmp( path, STDERR_SPECIAL_FILE ) == 0 )
    fd = ::GetStdHandle( STD_ERROR_HANDLE );
  else {
    fd = ::CreateFile( path, GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL );
    if ( fd == INVALID_HANDLE_VALUE )
      throw FileErr::getErr( FileErr::OPEN_WRITE_FAILED );
    ::SetFilePointer( fd, 0, NULL, FILE_END );
    file = File::openFD( fd );
  }
  rai_log_fd = fd;
#elif defined( __linux )
  int fd;

  if ( ::strcmp( path, STDOUT_SPECIAL_FILE ) == 0 )
    fd = 0;
  else if ( ::strcmp( path, STDERR_SPECIAL_FILE ) == 0 )
    fd = 2;
  else {
    const int mode = O_WRONLY | O_APPEND | O_CREAT;
    if ( (fd = ::open( path, mode, 0666 )) < 0 )
      throw FileErr::getErr( FileErr::OPEN_WRITE_FAILED );
    ::fcntl( fd, F_SETFD, FD_CLOEXEC );
    file = File::openFD( fd );
  }
  rai_log_fd = fd;
#endif
  if ( file == NULL )
    file = File::openFile( path, File::FILE_WRONLY | File::FILE_APPEND |
                                 File::FILE_CREAT );

  if ( ::strcmp( path, STDOUT_SPECIAL_FILE ) == 0 ||
       ::strcmp( path, STDERR_SPECIAL_FILE ) == 0 )
    out = NEW StdioOutputStream( file, 8 * 1024, true );
  else
    out = FileOutputStream::create( file, 8 * 1024, true, true,
                                    file->length() );
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( ! rai_handler_installed ) {
    rai_handler_installed = true;
    ::SetUnhandledExceptionFilter( rai_log_crash_handler );
  }
#elif defined( __linux ) || defined( __sun )
  if ( ! rai_handler_installed ) {
    struct sigaction sa;
    rai_handler_installed = true;

    sa.sa_handler = (void (*)(int)) rai_log_crash_handler;
    ::sigemptyset( &sa.sa_mask );
    sa.sa_flags = SA_RESTART | SA_SIGINFO;

    ::sigaction( SIGSEGV, &sa, NULL );
    ::sigaction( SIGILL, &sa, NULL );
    ::sigaction( SIGABRT, &sa, NULL );
    ::sigaction( SIGFPE, &sa, NULL );
  }
#endif
  return out;
}


void
Log::enableCrashLogging( bool enable )
{
  rai_enable_crash_logging = ( enable ? 2 : 1 );
}


void
Log::openLog( const char *path,  LogLevel level,  unsigned int verbosity,
              bool useXml )
{
  OutputStream * out;
  Error          e2;

  e2 = NULL;
  if ( log != Sys::err ) {
    out = log;
    /*log = Sys::err;*/
    log = NULL;
    if ( out != NULL )
      delete out;
  }
  rai_log_lock();

  logLevel = Log::LVL_NORMAL;
  logXml   = useXml;
  Log::setVerbosity( verbosity );

  /* still open log if verbosity is 0, it may change during program execution */
  if ( path != NULL ) {
    if ( ::strcmp( path, "-" ) != 0 ) {
      try {
        TimeMSecs t;
        if ( File::fileExists( path ) )
          t = File::fileModifiedTime( path );
        else
          t = Time::currentTimeMillisecs();
        logRotate.setLastTime( t );
        out = rai_open_log_file( path );
        log = out;
        str_copy( logFileName, path, sizeof( logFileName ) );
        suppressTimestamp    = false;
        needStartupTimestamp = true;
      } catch ( Error e ) {
        e2 = e;
      }
    }
    else {
      if ( ! logXml ) {
        suppressTimestamp    = true;
        needStartupTimestamp = false;
      }
      else {
        suppressTimestamp    = false;
        needStartupTimestamp = true;
      }
      //log = Sys::err;
      log = rai_open_log_file( STDERR_SPECIAL_FILE );
      logFileName[ 0 ] = '\0';
      logRotate.setLastTime( 0 );
    }
  }

  logLevel = level;
  updateMinLevel();

  rai_log_unlock();

  if ( e2 != NULL )
    throw e2;
}


void *
Log::openSyslog( const char *facility,  LogLevel level,  const char *host,
                 const char *prog,  const void *sock,  const void *sockAddr )
{
  SyslogList *el;
  unsigned int i;

  if ( facility != NULL )
    el = NEW SyslogList();
  else
    el = NULL;

  rai_log_lock();

  /* facility == NULL closes all existing syslogs */
  if ( facility == NULL ) {
    SyslogList * el_next;
    for ( el = syslogs; el != NULL; el = el_next ) {
      el_next = el->next;
      delete el;
    }
    level = Log::LVL_ERROR;
  }
  else {
    if ( facility[ 0 ] >= '0' && facility[ 0 ] <= '9' ) {
      el->syslogFacility = atoi( facility ) * 8;
    }
    else {
      for ( i = 0; i < sizeof( facilities ) / sizeof( facilities[ 0 ] ); i++ ) {
        const char *name = facilities[ i ].name;
        if ( StrUtil::strcasecmp( facility, name ) == 0 ||
             StrUtil::strcasecmp( facility, &name[ 4 ] ) == 0 ) {
          el->syslogFacility = facilities[ i ].facility;
          break;
        }
      }
      if ( i == sizeof( facilities ) / sizeof( facilities[ 0 ] ) ) {
        el->syslogFacility = facilities[ 0 ].facility;
        /*if ( Sys::err != NULL )
          Sys::err->printf( "Unknown system log facility: %s, using %s\n",
                            syslogFacility, syslogFacility );*/
      }
    }
    if ( host == NULL )
      el->syslogHost[ 0 ] = '\0';
    else {
      ::strncpy( el->syslogHost, host, sizeof( el->syslogHost ) );
      el->syslogHost[ sizeof( el->syslogHost ) - 1 ] = '\0';
    }
    if ( prog == NULL )
      el->syslogProg[ 0 ] = '\0';
    else {
      ByteArrayOutputStream bout( (byte *) el->syslogProg,
                                  sizeof( el->syslogProg ) - 1 );
      bout.printf( "%s[%u]", prog, Thread::getProcessId() );
      el->syslogProg[ bout.length() ] = '\0';
    }
    if ( sock != NULL )
      ::memcpy( &el->syslogSock, sock, sizeof( el->syslogSock ) );
    if ( sockAddr != NULL ) {
      if ( el->syslogAddr == NULL )
        el->syslogAddr = &el->syslog_addr_buf;
      ::memcpy( el->syslogAddr, sockAddr, sizeof( *el->syslogAddr ) );
    }
    el->syslogLevel = level;
    el->next = syslogs;
    syslogs = el;
  }

  updateMinLevel();

  rai_log_unlock();
  return (void *) el;
}

static void
setVerb( unsigned int verbosity,  unsigned int &verb )
{
  if ( verbosity <= Log::VERB_4 ) {
    verb = 0;

    switch ( verbosity ) {
      case Log::VERB_4:
        verb |= (unsigned int) ( Log::VERB_TRACE );
      case Log::VERB_3:
        verb |= (unsigned int) ( Log::VERB_SEVERITY );
      case Log::VERB_2:
        verb |= (unsigned int) ( Log::VERB_TIMESTAMP |
                                 Log::VERB_MILLISECS |
                                 Log::VERB_REASON );
      case Log::VERB_1:
        verb |= (unsigned int) ( Log::VERB_NUMBER | Log::VERB_DESCR );
      case Log::VERB_0:
      case Log::VERB_NONE:
      default:
        break;
    }
  }
  else {
    verb = ( verbosity & (unsigned int) ( Log::VERB_TIMESTAMP |
                                          Log::VERB_SEVERITY |
                                          Log::VERB_NUMBER |
                                          Log::VERB_REASON |
                                          Log::VERB_DESCR |
                                          Log::VERB_TRACE |
                                          Log::VERB_MILLISECS ) );
    if ( verbosity != verb ) {
      if ( Sys::err != NULL )
        Sys::err->printf( "Warning: Verbosity level not valid: 0x%x\n",
                          verbosity );
    }
  }
}

void
Log::setSyslogLevel( void *hndl,  Log::LogLevel logLevel,
                     unsigned int logVerbosity )
{
  SyslogList * el = (SyslogList *) hndl;
  el->syslogLevel     = logLevel;
  setVerb( logVerbosity,  el->syslogVerbosity );
}

void
Log::closeSyslog( void *hndl )
{
  rai_log_lock();

  SyslogList * el, * el_next, * el_prev = NULL;
  for ( el = syslogs; el != NULL; el = el_next ) {
    el_next = el->next;
    if ( (void *) el == hndl ) {
      if ( el_prev == NULL )
        syslogs = el_next;
      else
        el_prev->next = el_next;
      delete el;
      break;
    }
    el_prev = el;
  }

  rai_log_unlock();
}

void
Log::setLevel( LogLevel level,  bool useXml,  unsigned int verbosity )
{
  logLevel = level;
  updateMinLevel();

  logXml = useXml;
  Log::setVerbosity( verbosity );
}

// Set callback to call after rotating log file
void 
Log::setLogRotateCallback( void (*callback)(void *), void * closure )
{
  rotateCallback = callback;
  rotateClosure  = closure;
}


Log::LogLevel 
Log::getLevel( void )
{
  return logLevel;
}

void
Log::setVerbosity( unsigned int verbosity )
{
  setVerb( verbosity, logVerbosity );
}


unsigned int
Log::getVerbosity( void )
{
  return logVerbosity;
}


bool
Log::flipSyslog( bool on )
{
  bool last = ! syslogDisabled;
  if ( on )
    syslogDisabled = false;
  else
    syslogDisabled = true;
  return last;
}

void
Log::setLogTimeStampFmt( const char * logTimeStampFmt )
{
  renamer.setLogTimeStampFmt( logTimeStampFmt );
}

void
Log::setRolloverCnt(  unsigned int rolloverCnt )
{
  renamer.setRolloverCnt( rolloverCnt );
}

void
Log::setRolloverType(  LogRolloverType rolloverType )
{
  renamer.setRolloverType( rolloverType );
}

void
Log::setSizeLimit( ullong sizeLimit )
{
  logSizeLimit = sizeLimit;
}


bool
Log::setLogRotateTime( const char *timeSpec,  TimeRotate::DayOrWeek rotDorW,
                       TimeMSecs rotTime )
{
  return logRotate.setRotateTime( timeSpec, rotDorW, rotTime );
}


bool
Log::setLogRotatePeriod( const char *periodSpec,  TimeMSecs rotatePeriod )
{
  return logRotate.setRotatePeriod( periodSpec, rotatePeriod );
}


void
Log::allocLogBuf( unsigned int bufLen,  LogLevel level )
{
  byte              * tmpBuf = NULL,
                    * oldBuf = NULL;
  CycleOutputStream * tmpOut = NULL,
                    * oldOut = NULL;

  try {
    if ( bufLen > 0 ) {
      MALLOC( bufLen, &tmpBuf );
      tmpOut = NEW CycleOutputStream( tmpBuf, bufLen );
    }
  } catch ( ... ) {
    return;
  }

  rai_log_lock();

  oldBuf      = logBuf;
  oldOut      = logBufOut;
  logBufLen   = bufLen;
  logBuf      = tmpBuf;
  logBufOut   = tmpOut;
  logBufLevel = level;
  logCount    = 0;

  updateMinLevel();

  rai_log_unlock();

  if ( oldOut != NULL )
    delete oldOut;
  if ( oldBuf != NULL )
    FREE( oldBuf );
}


unsigned int
Log::getLogBuf( byte *buf,  unsigned int bufLen,  unsigned int &cnt )
{
  rai_log_lock();

  if ( logBufOut != NULL )
    bufLen = logBufOut->copyTo( buf, bufLen );
  else
    bufLen = 0;
  cnt = logCount;

  rai_log_unlock();

  return bufLen;
}


static void
printStartupTimestamp( TimeMSecs stamp )
{
  const char *fmt;
  char dateBuf[ 80 ];

  if ( logXml )
    fmt = "<Log><Start tm=\"%s\" />\n";
  else
    fmt = "### log start %s ###\n";

  log->printf( fmt, Time::timestamp( stamp, dateBuf, sizeof( dateBuf ) ) );
  needStartupTimestamp = false;
}


static void
printShutdownTimestamp( void )
{
  const char *fmt;
  char dateBuf[ 80 ];

  if ( logXml )
    fmt = "<Shutdown tm=\"%s\" /></Log>\n";
  else
    fmt = "### log shutdown %s ###\n";

  log->printf( fmt, Time::timestamp( dateBuf, sizeof( dateBuf ) ) );
}


static void
printRotateTimestamp( void )
{
  const char *fmt;
  char dateBuf[ 80 ];

  if ( logXml )
    fmt = "<Rotate tm=\"%s\" /></Log>\n";
  else
    fmt = "### log rotate %s ###\n";

  log->printf( fmt, Time::timestamp( dateBuf, sizeof( dateBuf ) ) );
}


static bool
checkLogRotate( void )
{
  OutputStream * out;
  unsigned int   logVerb;
  bool           sysDisab;
  Error          e2;
  bool           doRotate;

  if ( log == NULL || log == Sys::err || logFileName[ 0 ] == '\0' ||
       ::strcmp( logFileName, STDERR_SPECIAL_FILE ) == 0 ||
       ::strcmp( logFileName, STDOUT_SPECIAL_FILE ) == 0 )
    return false;

  e2       = NULL;
  logVerb  = logVerbosity;
  sysDisab = syslogDisabled;

  /* so we don't get stuck in an infinite loop, set verbs to zero,
     preventing Log::vprintLog() from calling us again */
  logVerbosity   = 0;
  syslogDisabled = true;
  doRotate       = false;

  try {
    if ( logSizeLimit != 0 && log->getStreamOffset() >= logSizeLimit )
      doRotate = true;

    if ( logRotate.checkRotate() )
      doRotate = true;

    if ( doRotate ) {
      if ( ! suppressTimestamp ) {
        printRotateTimestamp();
        needStartupTimestamp = true;
      }

      out = log;
      log = NULL;
      out->close();
      delete out;

      renamer.rename( logFileName );
    }
  } catch ( Error e ) {
    e2 = e;
  }
  if ( log == NULL ) {
    try {
      log = rai_open_log_file( logFileName );
    } catch ( Error e ) {
      if ( e2 == NULL )
        e2 = e;
      if ( Sys::err != NULL )
        log = Sys::err;
    }
  }
  logVerbosity   = logVerb;
  syslogDisabled = sysDisab;

  if ( e2 != NULL )
    throw e2;
  return doRotate;
}


TimeMSecs
Log::nextLogRotate( TimeMSecs *diffTime )
{
  if ( log == NULL || log == Sys::err || logFileName[ 0 ] == '\0' ||
       logRotate.time == 0 )
    return 0;
  return logRotate.nextRotate( diffTime );
}


bool
Log::checkRotate( void )
{
  bool isRotated;

  if ( log == NULL || log == Sys::err || logFileName[ 0 ] == '\0' ||
       ( logSizeLimit == 0 && logRotate.time == 0 ) )
    return false;

  isRotated = false;
  rai_log_lock();

  try {
    isRotated = checkLogRotate();
    if ( isRotated && needStartupTimestamp )
      printStartupTimestamp( Time::currentTimeMillisecs() );
    if( rotateCallback ) {
      (*rotateCallback)( rotateClosure );
    }
    rai_log_unlock();
  } catch ( Error e3 ) {
    /* prevent rotation */
    logSizeLimit   = 0;
    logRotate.time = 0;
    rai_log_unlock();
    logError( LERROR, e3, "Rotate log file \"%s\" failed", logFileName );
  }

  return isRotated;
}


void
Log::setAutoRotate( bool turnOn )
{
  autoRotateCheck = turnOn;
}


void
Log::updateLogModifiedTime( TimeMSecs mtime )
{
  Error e2 = NULL;
  rai_log_lock();
  try {
    if ( log != NULL && logFileName[ 0 ] != '\0' )
      File::setModifiedTime( logFileName, mtime );
  } catch ( Error e ) {
    e2 = e;
  }
  rai_log_unlock();
  if ( e2 != NULL )
    throw e2;
}


void
Log::closeLog( void )
{
  OutputStream * out;
  Log::LogLevel  levelSave;

  rai_log_lock();

  if ( logVerbosity != 0 && ! suppressTimestamp && log != NULL )
    printShutdownTimestamp();

  if ( log != Sys::err ) {
    levelSave = Log::minLevel;
    if ( levelSave == Log::LVL_DEBUG )
      Log::minLevel = Log::LVL_NORMAL;

    try {
      out = log;
      /*log = Sys::err;*/
      log = NULL;
      if ( out != NULL ) {
        out->close();
        delete out;
      }
    } catch( ... ) {
      Log::minLevel = levelSave;
      rai_log_unlock();
      throw;
    }
    Log::minLevel = levelSave;
  }

  rai_log_unlock();
  if ( logMutex != NULL ) {
    delete logMutex;
    logMutex = NULL;
  }
}


OutputStream * 
Log::lock( void )
{
  rai_log_lock();
  return log;
}


void 
Log::unlock( void )
{
  rai_log_unlock();
}

void
Log::flush( void )
{
  rai_log_lock();
  try {
    if( log ) {
      log->flush();
    }
  } catch( ... ) {
    // unlikely we can log there error here. Just catch and ignore so we can unlock
  }
  rai_log_unlock();
}

// Copy input stream to log as is
void
Log::printLog( InputStream * is )
{
  unsigned int bytesRead;
  byte buff[ 256 * 1024 ];

  rai_log_lock();
  try {
    while( ( bytesRead = is->readBytes( buff, sizeof( buff ) ) ) > 0 ) {
      unsigned int bytesWritten = 0;
      while( bytesWritten < bytesRead ) {
        bytesWritten += log->writeBytes( buff + bytesWritten, bytesRead - bytesWritten );
      }
    }
  } catch( Error ) {
  }
  rai_log_unlock();
}

void
Log::printLog( LogLevel level,  const char *file,  int lineno,  Error e,
               const char *fmt,  ... )
{
  va_list ap;

  va_start( ap, fmt );
  Log::vprintLog( level, file, lineno, e, fmt, ap );
  va_end( ap );
}


static char lvlString[ 8 ][ 16 ] = {
  "*****",
  "Debug",
  "Devel",
  "FTrce",
  "Trace",
  "Minor",
  "Norml",
  "Error",
};

void
Log::setLevelPrefix( const char *s )
{
  unsigned int i, len = ::strlen( lvlString[ 0 ] );
  if ( len > 5 )
    for ( i = 0; i < 8; i++ )
      ::memmove( &lvlString[ i ][ 0 ], &lvlString[ i ][ len - 5 ], 6 );
  if ( (len = ::strlen( s )) > 0 )
    for ( i = 0; i < 8; i++ ) {
      ::memmove( &lvlString[ i ][ len ], &lvlString[ i ][ 0 ], 6 );
      ::memcpy( &lvlString[ i ][ 0 ], s, len );
    }
}

static const char *
getLevelString( Log::LogLevel level )
{
  switch ( level ) {
    default:              return lvlString[ 0 ];
    case Log::LVL_DEBUG:  return lvlString[ 1 ];
    case Log::LVL_DEVEL:  return lvlString[ 2 ];
    case Log::LVL_FTRACE: return lvlString[ 3 ];
    case Log::LVL_TRACE:  return lvlString[ 4 ];
    case Log::LVL_MINOR:  return lvlString[ 5 ];
    case Log::LVL_NORMAL: return lvlString[ 6 ];
    case Log::LVL_ERROR:  return lvlString[ 7 ];
  }
}


void
Log::vprintLog( LogLevel level,  const char *file,  int lineno,  Error e,
                const char *fmt,  va_list ap )
{
  TimeMSecs stamp = Time::currentTimeMillisecs();
  Log::vprintLog2( stamp, level, file, lineno, e, NULL, fmt, ap );
}


void
Log::vprintSyslog( LogLevel level,  const char *file,  int lineno,  Error e,
                   const char *fmt,  va_list ap )
{
  TimeMSecs stamp = Time::currentTimeMillisecs();
  Log::vprintSyslog2( stamp, level, file, lineno, e, NULL, fmt, ap );
}


void
Log::vprintLog2( TimeMSecs stamp,  LogLevel level,  const char *file,
                 int lineno,  Error e,  const char *inst,  const char *fmt,
                 va_list ap )
{
  if ( ! syslogDisabled && syslogs != NULL ) {
    Log::vprintSyslog2( stamp, level, file, lineno, e, inst, fmt, ap );
  }

  rai_log_lock();

  if ( logFileName[ 0 ] != '\0' && autoRotateCheck &&
       ( logSizeLimit != 0 || logRotate.time != 0 ) ) {
    try {
      if( checkLogRotate() )
        if( rotateCallback ) {
          (*rotateCallback)( rotateClosure );
        }
    } catch ( Error e3 ) {
      rai_log_unlock();
      /* prevent rotation */
      logSizeLimit   = 0;
      logRotate.time = 0;
      logError( LERROR, e3, "Rotate log file \"%s\" failed", logFileName );

      rai_log_lock();
    }
  }

  try {
    if ( ! suppressTimestamp && needStartupTimestamp )
      printStartupTimestamp( stamp );

    if ( logVerbosity != 0 ) {
      if ( level >= logLevel && log != NULL ) {
        if ( logXml ) {
          Log::vprintLogXml( log, logVerbosity, stamp, level, file, lineno, e,
                             inst, fmt, ap );
        }
        else {
          Log::vprintLogOut( log, logVerbosity, stamp, level, file, lineno, e,
                             inst, fmt, ap );
        }
      }
      if ( level >= logBufLevel && logBufOut != NULL ) {
        if ( logXml ) {
          Log::vprintLogXml( logBufOut, logVerbosity, stamp, level, file,
                             lineno, e, inst, fmt, ap );
        }
        else {
          Log::vprintLogOut( logBufOut, logVerbosity, stamp, level, file,
                             lineno, e, inst, fmt, ap );
        }
        logCount++;
      }
    }
  } catch( Error e2 ) {
    try {
      if ( Sys::err != NULL && Sys::err != log ) {
        Sys::err->printf( "Logger unable to print: %s.%d+%s\n", e2->module,
                          e2->status, e2->reason );
        if ( e != NULL )
          Sys::err->printf( "+ Original errcode was: %s.%d+%s\n", e->module,
                            e->status, e->reason );
      }
    } catch( Error ) {
    }
  }
  rai_log_unlock();
}


void
Log::vprintLogOut( OutputStream *log,  unsigned int verb,  TimeMSecs stamp,
                   LogLevel level,  const char *file,  int lineno,  Error e,
                   const char *inst,  const char *fmt,  va_list ap )

{
  /* verbosity level produces:
   * 1: Nothing
   * 2: 101+Can't start services
   *  = VERB_NUMBER | VERB_DESCR
   * 3: [timestamp] 101+Bind system call failed; Can't start services
   *  = VERB_TIMESTAMP | VERB_REASON | VERB_NUMBER |
   *    VERB_DESCR
   * 4: [timestamp] Error: 101+Bind system call failed; Can't start services
   *  = VERB_SEVERITY | VERB_TIMESTAMP | VERB_REASON |
   *    VERB_NUMBER | VERB_DESCR
   * 5: [timestamp] Error: 101+Bind system call failed; Can't start \
   * services (node/node.c:310)
   *  = VERB_TRACE | VERB_SEVERITY | VERB_TIMESTAMP |
   *    VERB_REASON | VERB_NUMBER | VERB_DESCR
   */
  if ( (verb & Log::VERB_TIMESTAMP) != 0 ) {
    char timeString[ 40 ];
    if ( (verb & Log::VERB_MILLISECS) == 0 )
      Time::timestamp( stamp, timeString, sizeof( timeString ) );
    else
      Time::timestamp( Time::millisecsToNanosecs( 0, stamp ), 3,
                       timeString, sizeof( timeString ) );
    log->printf( "%s ", timeString );
  }

  if ( (verb & Log::VERB_SEVERITY) != 0 ) {
    log->printf( "%s: ", getLevelString( level ) );
  }

  if ( e != NULL ) {
    switch ( (verb & ( Log::VERB_REASON | Log::VERB_NUMBER ) ) ) {
      case Log::VERB_REASON:
        log->printf( "%s;", e->reason );
        break;
      case Log::VERB_NUMBER:
        log->printf( "%s.%u;", e->module, e->status );
        break;
      case ( Log::VERB_REASON | Log::VERB_NUMBER ):
        log->printf( "%s.%d+%s;", e->module, e->status, e->reason );
        break;
      default:
        break;
    }
  }

  if ( (verb & Log::VERB_DESCR ) != 0 ) {
    if ( inst != NULL && inst[ 0 ] != '\0' ) {
      log->puts( " " );
      log->puts( inst );
      log->puts( ":" );
    }
    if ( fmt != NULL ) {
      log->puts( " " );
#if defined( _WIN32 ) || defined( _WIN64 )
      log->vprintf( fmt, ap );
#else
      va_list aq;
      va_copy( aq, ap );
      log->vprintf( fmt, aq );
      va_end( aq );
#endif
    }
  }

  if ( (verb & Log::VERB_TRACE) != 0 ) {
    if ( file != NULL )
      log->printf( " (%s:%d)", file, lineno );
  }

  log->puts( "\n" );
}


void
Log::vprintLogXml( OutputStream *log,  unsigned int verb,  TimeMSecs stamp,
                   LogLevel level,  const char *file,  int lineno,  Error e,
                   const char *inst,  const char *fmt,  va_list ap )

{
  log->puts( "<X " );

  if ( (verb & Log::VERB_SEVERITY) != 0 ) {
    log->printf( "sv=\"%s\" ", getLevelString( level ) );
  }

  if ( (verb & Log::VERB_TIMESTAMP) != 0 ) {
    char timeString[ 40 ];
    if ( (verb & Log::VERB_MILLISECS) == 0 )
      Time::timestamp( stamp, timeString, sizeof( timeString ) );
    else
      Time::timestamp( Time::millisecsToNanosecs( 0, stamp ), 3,
                       timeString, sizeof( timeString ) );
    log->printf( "tm=\"%s\" ", timeString );
  }

  if ( e != NULL ) {
    if ( (verb & Log::VERB_NUMBER) != 0 )
      log->printf( "no=\"%s.%u\" ", e->module, e->status );
    if ( (verb & Log::VERB_REASON) != 0 )
      log->printf( "re=\"%s\" ", e->reason );
  }

  if ( fmt != NULL ) {
    if ( (verb & Log::VERB_DESCR ) != 0 ) {
      byte                  tmpBuf[ 1024 ];
      ByteArrayOutputStream tmpOut( tmpBuf, sizeof( tmpBuf ) );
      const char          * replace;
      unsigned int          i,
                            len,
                            rlen;

      try {
        if ( inst != NULL && inst[ 0 ] != '\0' ) {
          tmpOut.puts( inst );
          tmpOut.puts( ": " );
        }
#if defined( _WIN32 ) || defined( _WIN64 )
        tmpOut.vprintf( fmt, ap );
#else
        va_list aq;
        va_copy( aq, ap );
        tmpOut.vprintf( fmt, aq );
        va_end( aq );
#endif
      } catch ( ... ) {
        /* if overflow of tmpBuf */
      }

      len     = tmpOut.length();
      i       = 0;
      replace = NULL;
      rlen    = 0;
      while ( i < len ) {
        /* replace quotes with &quot; */
        if ( tmpBuf[ i ] == '"' ) {
          replace = "&quot;";
          rlen    = 6;
        }
        /* replace > with &gt; */
        else if ( tmpBuf[ i ] == '>' ) {
          replace = "&gt;";
          rlen    = 4;
        }
        /* replace < with &lt; */
        else if ( tmpBuf[ i ] == '<' ) {
          replace = "&lt;";
          rlen    = 4;
        }
        /* replace & with &amp; */
        else if ( tmpBuf[ i ] == '&' ) {
          replace = "&amp;";
          rlen    = 5;
        }
        /* replace newline with &#A; */
        else if ( tmpBuf[ i ] == '\n' ) {
          replace = "&#A;";
          rlen    = 4;
        }
        /* replace newline with &#D; */
        else if ( tmpBuf[ i ] == '\r' ) {
          replace = "&#D;";
          rlen    = 4;
        }
        else {
          i++;
          continue;
        }

        if ( len + rlen >= sizeof( tmpBuf ) )
          tmpBuf[ i ] = ' ';
        else {
          ::memmove( &tmpBuf[ i + rlen ], &tmpBuf[ i + 1 ],
                     len - ( i + 1 ) );
          ::memcpy( &tmpBuf[ i ], replace, rlen );
          i   += rlen;
          len += rlen - 1;
        }
      }
      log->printf( "de=\"%.*s\" ", len, (char *) tmpBuf );
    }
  }

  if ( (verb & Log::VERB_TRACE) != 0 ) {
    if ( file != NULL )
      log->printf( "tr=\"%s:%d\" ", file, lineno );
  }
  log->puts( "/>\n" );
}


static bool
rai_log_lookup_ip4_host( const char *hostname,  byte *quad )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  struct hostent * h = ::gethostbyname( hostname );

  if ( h == NULL || h->h_addrtype != AF_INET )
    return false;

  ::memcpy( quad, h->h_addr, 4 );
#else
  struct addrinfo * h,
                  * res;
  if ( ::getaddrinfo( hostname, NULL, NULL, &h ) != 0 )
    return false;
  for ( res = h ; res != NULL; res = res->ai_next ) {
    if ( res->ai_family == AF_INET &&
         res->ai_addrlen >= sizeof( struct sockaddr_in ) ) {
       struct sockaddr_in * in = (struct sockaddr_in *) res->ai_addr;
       ::memcpy( quad, &in->sin_addr.s_addr, 4 );
       break;
     }
   }
   ::freeaddrinfo( h );
   if ( res == NULL )
     return false;
#endif
  return true;
}


void
Log::vprintSyslog2( TimeMSecs stamp,  LogLevel level,  const char *file,
                    int lineno,  Error e,  const char *inst,  const char *fmt,
                    va_list ap )
{
  static const unsigned int OFF = 5 + 32 + 2;
  if ( ! syslogDisabled && syslogs != NULL ) {
    for ( SyslogList *el = syslogs; el != NULL; el = el->next ) {
      try {
        if ( level >= el->syslogLevel && el->syslogVerbosity != 0 ) {
          byte                  tmpBuf[ 1024 + OFF ];
          ByteArrayOutputStream tmpOut( &tmpBuf[ OFF ],
                                        sizeof( tmpBuf ) - (OFF+1));
          unsigned int          len;

          try {
            Log::vprintLogOut( &tmpOut, el->syslogVerbosity, stamp, level,
                               file, lineno, e, inst, fmt, ap );
          } catch ( ... ) {
          }

          len = tmpOut.length();
          tmpBuf[ len + OFF ] = '\0';

#if defined( _WIN32 ) || defined( _WIN64 )
         if ( el->syslogFacility == LOG_WINDOWS ||
              el->syslogHost[ 0 ] == '\0' ) {
            static HANDLE fac = INVALID_HANDLE_VALUE;
            const char  * s   = (char *) &tmpBuf[ OFF ];
            WORD          prio;

            if ( fac == INVALID_HANDLE_VALUE )
              fac = ::RegisterEventSource( NULL, "RAI" );
            if ( level == Log::LVL_DEBUG )
              prio = EVENTLOG_SUCCESS;
            else if ( level == Log::LVL_MINOR )
              prio = EVENTLOG_INFORMATION_TYPE;
            else if ( level == Log::LVL_NORMAL )
              prio = EVENTLOG_WARNING_TYPE;
            else
              prio = EVENTLOG_ERROR_TYPE;
            if ( fac != INVALID_HANDLE_VALUE )
              ::ReportEvent( fac, prio, 0, 0, NULL, 1, 0, &s, NULL );
            return;
          }
#endif
          int prio = el->syslogFacility;
          if ( level == Log::LVL_DEBUG )
            prio |= LOG_DEBUG;
          else if ( level == Log::LVL_MINOR )
            prio |= LOG_INFO;
          else if ( level == Log::LVL_NORMAL )
            prio |= LOG_WARNING;
          else
            prio |= LOG_ALERT;

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
          if ( el->syslogHost[ 0 ] == '\0' ) {
            static int fac = 9999;
            if ( fac != ( prio & ~( ( 1 << 3 ) - 1 ) ) ) {
              if ( fac != 9999 )
                ::closelog();
              fac = prio & ~( ( 1 << 3 ) - 1 );
              ::openlog( "RAI", LOG_PID, fac );
            }
            ::syslog( prio, "%s", (char *) &tmpBuf[ OFF ] );
          }
          else
#endif
          {
            //static socket_t           s = INVALID_SOCKET;
            //static struct sockaddr_in log_addr;
            unsigned int plen = ::strlen( el->syslogProg );
            unsigned int off  = OFF - ( 5 + plen + ( plen == 0 ? 0 : 2 ) );

            tmpBuf[ off ]   = '<';
            tmpBuf[ off+1 ] = '0' + (byte) ( ( prio / 100 ) % 10 );
            tmpBuf[ off+2 ] = '0' + (byte) ( ( prio / 10 ) % 10 );
            tmpBuf[ off+3 ] = '0' + (byte) ( prio % 10 );
            tmpBuf[ off+4 ] = '>';
            if ( el->syslogProg[ 0 ] != '\0' ) {
              ::memcpy( (char *) &tmpBuf[ off + 5 ], el->syslogProg, plen );
              tmpBuf[ off + plen + 5 ] = ':';
              tmpBuf[ off + plen + 6 ] = ' ';
            }

            if ( el->syslogSock == INVALID_SOCKET ) {
              char         buf[ sizeof( el->syslogHost ) ];
              unsigned int off = ::strlen( el->syslogHost ),
                           ar[ 5 ] = {0,0,0,0,0},
                           i = 4,
                           ten = 1;
              ::memcpy( buf, el->syslogHost, sizeof( el->syslogHost ) );
              while ( off > 0 ) {
                --off;
                if ( isdigit( buf[ off ] ) ) {
                  ar[ i ] += (unsigned int) (byte) ( buf[ off ] - '0' ) * ten;
                  ten *= 10;
                }
                else if ( buf[ off ] == ':' || buf[ off ] == '.' ) {
                  if ( buf[ off ] == '.' && i == 4 ) {
                    ar[ 3 ] = ar[ 4 ];
                    ar[ 4 ] = 514;
                    --i;
                  }
                  buf[ off ] = '\0';
                  if ( i == 0 )
                    break;
                  --i;
                  ten = 1;
                }
                else {
                  if ( i == 4 )
                    ar[ 4 ] = 514;
                  if ( i > 0 ) {
                    byte quad[ 4 ];
                    if ( rai_log_lookup_ip4_host( buf, quad ) ) {
                      for ( i = 0; i < 4; i++ )
                        ar[ i ] = quad[ i ];
                      i = 0;
                    }
                  }
                  break;
                }
              }
              if ( i > 0 ) {
                ar[ 0 ] = 127; ar[ 1 ] = ar[ 2 ] = 0; ar[ 3 ] = 1;
                if ( i == 4 )
                  ar[ 4 ] = 514;
              }

              el->syslogSock = ::socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
              if ( el->syslogSock != INVALID_SOCKET ) {
            #if ! defined( _WIN32 ) && ! defined( _WIN64 )
                ::fcntl( el->syslogSock, F_SETFD, FD_CLOEXEC );
            #endif
                if ( el->syslogAddr == NULL )
                  el->syslogAddr = &el->syslog_addr_buf;
                el->syslogAddr->sin_port   = htons( ar[ 4 ] );
                el->syslogAddr->sin_family = AF_INET;
                byte quad[ 4 ];
                for ( i = 0; i < 4; i++ )
                  quad[ i ] = (byte) ar[ i ];
                ::memcpy( &el->syslogAddr->sin_addr.s_addr, quad, 4 );
              }
            }
            if ( el->syslogSock != INVALID_SOCKET ) {
              if ( el->syslogAddr != NULL )
                ::sendto( el->syslogSock, (send_arg_t) &tmpBuf[ off ],
                          len + OFF - off, 0,
                          (struct sockaddr *) el->syslogAddr,
                          (addrlen_t) sizeof( struct sockaddr_in ) );
              else {
            #if ! defined( _WIN32 ) && ! defined( _WIN64 )
                ::write( el->syslogSock, (send_arg_t) &tmpBuf[ off ],
                         len + OFF - off );
	    #else
                _write( el->syslogSock, (send_arg_t) &tmpBuf[ off ],
                        len + OFF - off );
	    #endif
              }
            }
          }
        }
      } catch ( ... ) {
      }
    }
  }
}


void
Log::printLogHex( LogLevel level,  const char *where,  int lineno,
	          Error err,  const char *what,  const byte *msg,
                  unsigned int msgSize )
{
  static const char hexChars[] = "0123456789abcdef";
  unsigned int i, j, k, l, m;
  char         line[ 80 ];
  unsigned int verb;

  rai_log_lock();
  if( what != NULL ) {
    printLog( level, where, lineno, err, "%s", what );
  }

  // don't need file and line number, so turn off tracing if its on
  verb = logVerbosity;
  logVerbosity = Log::VERB_DESCR;
  
  if ( msgSize > 16 * 1024 ) {
    printLog( level, where, lineno, err,
              "Hex dump of message too big: %u, truncating to 16k", msgSize );
    msgSize = 16 * 1024;
  }
  for ( i = 0; i < msgSize; ) {
    k = 0;
    l = 52;
    m = i;
    for ( j = 0; j < 16 && m < msgSize; m++ ) {
      line[ k++ ] = hexChars[ msg[ m ] >> 4 ];
      line[ k++ ] = hexChars[ msg[ m ] & 0xf ];
      line[ k++ ] = ' ';
      line[ l++ ] = ( msg[ m ] >= ' ' && msg[ m ] <= 127 ) ?
	msg[ m ] : '.';
      if ( ( ++j & 0x3 ) == 0 )
	line[ k++ ] = ' ';
    }
    while ( k < 52 )
      line[ k++ ] = ' ';
    
    line[ l ] = '\0';

    printLog( level, where, lineno, err, "%04x: %s", i, line );
    i += 16;
  }
  logVerbosity = verb;
  rai_log_unlock();
}


void
LogV::printLog( LogV &log,  Log::LogLevel level,  const char *where,  int line,
                Error err,  const char *fmt,  ... )
{
  va_list ap;
  va_start( ap, fmt );
  log.vprintLog( level, where, line, err, fmt, ap );
  va_end( ap );
}

void
LogV::printLog( LogV *log,  Log::LogLevel level,  const char *where,  int line,
                Error err,  const char *fmt,  ... )
{
  va_list ap;
  va_start( ap, fmt );
  if ( log == NULL )
    Log::vprintLog( level, where, line, err, fmt, ap );
  else
    log->vprintLog( level, where, line, err, fmt, ap );
  va_end( ap );
}

void
LogV::vprintLog( LogV *log,  Log::LogLevel level,  const char *where, 
                 int line,  Error err,  const char *fmt,  va_list ap )
{
  if ( log == NULL )
    Log::vprintLog( level, where, line, err, fmt, ap );
  else
    log->vprintLog( level, where, line, err, fmt, ap );
}

Error
LogErr::getErr( unsigned int status )
{
  static const char mod[]     = "LogErr";
  static const ErrorRec err[] = {
  /*  0 */ { OK,                    "OK", mod },
  /*  1 */ { NULL_PATH,             "Path not specified", mod },
  /*  2 */ { BAD_PATH,              "Invalid file path", mod },
  /*  3 */ { BAD_ROLL_TYPE,         "Invalid log rollover type. Expect \'date\' or \'num\'", mod },
  /*  4 */ { 4,                     "Unknown Log error", mod },
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}
