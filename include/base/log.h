/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_base__log_h__
#define __rai_base__log_h__

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

#include <stdarg.h> /* va_list */

namespace rai {

class OutputStream;
class InputStream;
class Mutex;

namespace Log {
  enum LogLevel {
    LVL_DEVEL  = 0,
    LVL_FTRACE = 1,
    LVL_TRACE  = 2,
    LVL_DEBUG  = 3, /* turns on debug trace output */
    LVL_MINOR  = 4, /* minor program events */
    LVL_NORMAL = 5, /* non fatal errors */
    LVL_ERROR  = 6  /* fatal errors */
  };

  /* <timestamp> <severity>: <number>+<reason>; <description> (<trace>) */
  enum LogVerbosity {
    VERB_NONE = 0, /* nothing */
    VERB_0   = 1, /* nothing */
    VERB_1   = 2, /* number + descr */
    VERB_2   = 3, /* timestamp + number + reason + descr */
    VERB_3   = 4, /* timestamp + severity + number + reason + descr */
    VERB_4   = 5, /* timestamp + severity + number + reason + descr + trace */

    VERB_TIMESTAMP      = 1 << 4, /* masks instead of levels */
    VERB_SEVERITY       = 1 << 5,
    VERB_NUMBER         = 1 << 6,
    VERB_REASON         = 1 << 7,
    VERB_DESCR          = 1 << 8,
    VERB_TRACE          = 1 << 9,
    VERB_MILLISECS      = 1 << 10
  };

  enum LogRolloverType {
    ROLLOVER_DATE = 0, /* Use closing date and time. logfile.2019-01-12_14:01:34 */
    ROLLOVER_NUM  = 1, /* Use numbers, 1 to n. 1 is most recent. logfile.1 */
    ROLLOVER_UNK  = 2  /* Unknown value */
  };
  
  extern RAIBASE_DLL_EXP
         LogLevel       minLevel;        /* logLevel <? syslogLevel */

  extern RAIBASE_DLL_EXP
         int            rolloverCnt;	 /* how many log versions to keep. 0 mean keep all */

  extern RAIBASE_DLL_EXP
         LogRolloverType rolloverType;	 /* How to rename logfiles with doing rollover. */
  
  /* log functions */
  RAIBASE_DLL_EXP
  LogLevel parseLogLevel( const char *levelName );

  RAIBASE_DLL_EXP
  const char *levelToString( LogLevel level );

  RAIBASE_DLL_EXP
  LogRolloverType parseLogRolloverType( const char *rolloverType ) throw( Error );

  RAIBASE_DLL_EXP
  const char *logRolloverTypeToString( LogRolloverType logRolloverType ) throw( Error );

  // Enable crash logging, by default disabled
  RAIBASE_DLL_EXP
  void enableCrashLogging( bool enable );

  // Open log to output stream - turns off auto rotate
  RAIBASE_DLL_EXP
  void openLog( OutputStream *os,  LogLevel level,  unsigned int verbosity,
                bool useXml = false )                  throw( Error );
  RAIBASE_DLL_EXP
  void openLog( const char *path,  LogLevel level,  unsigned int verbosity,
                bool useXml = false )                  throw( Error );
  /* call with facility == NULL, closes all syslogs */
  RAIBASE_DLL_EXP
  void *openSyslog( const char *facility,  LogLevel syslogLevel = LVL_ERROR,
                   const char *host = NULL,  const char *prog = NULL,
                   const void *sock = NULL,  const void *sockAddr = NULL );
  RAIBASE_DLL_EXP
  void setSyslogLevel( void *hndl,  LogLevel logLevel,
                       unsigned int verbosity );
  RAIBASE_DLL_EXP
  void closeSyslog( void *hndl ); /* close handle returned by openSyslog() */

  RAIBASE_DLL_EXP
  void setLevel( LogLevel logLevel,  bool logXml,  unsigned int verbosity );

  // return logLevel
  RAIBASE_DLL_EXP
  LogLevel getLevel( void );

  RAIBASE_DLL_EXP
  void setLevelPrefix( const char *s );

  RAIBASE_DLL_EXP
  void setVerbosity( unsigned int verbosity );

  RAIBASE_DLL_EXP
  unsigned int getVerbosity( void );

  RAIBASE_DLL_EXP
  bool flipSyslog( bool on );

  RAIBASE_DLL_EXP
  void setLogTimeStampFmt( const char * logTimeStampFmt );

  RAIBASE_DLL_EXP
  void setRolloverCnt( unsigned int rolloverCnt );

  RAIBASE_DLL_EXP
  void setRolloverType( LogRolloverType rolloverType );
  
  RAIBASE_DLL_EXP
  void setSizeLimit( ullong sizeLimit );

  RAIBASE_DLL_EXP
  bool setLogRotateTime( const char *timeSpec, /* Mon, 23:00 */
                 TimeRotate::DayOrWeek rotDorW = TimeRotate::ROTATE_UNSPECIFIED,
                         TimeMSecs rotTime = 0 );
  RAIBASE_DLL_EXP
  bool setLogRotatePeriod( const char *periodSpec,  TimeMSecs rotPeriod = 0 );

  // Set callback to call after rotating log file
  RAIBASE_DLL_EXP
  void setLogRotateCallback( void (*callback)(void *), void * closure );

  RAIBASE_DLL_EXP
  void allocLogBuf( unsigned int bufLen,  LogLevel level );

  RAIBASE_DLL_EXP
  unsigned int getLogBuf( byte *buf,  unsigned int bufLen,  unsigned int &cnt );

  RAIBASE_DLL_EXP
  bool checkRotate( void ); /* check if log needs rotating */

  RAIBASE_DLL_EXP
  TimeMSecs nextLogRotate( TimeMSecs *diffTime = NULL );

  RAIBASE_DLL_EXP
  void updateLogModifiedTime( TimeMSecs mtime = 0 )    throw( Error );

  RAIBASE_DLL_EXP
  void setAutoRotate( bool turnOn ); /* auto-rotate is default */

  RAIBASE_DLL_EXP
  void closeLog( void )                                throw( Error );

  // Lock log and return log OutputStream
  RAIBASE_DLL_EXP
  OutputStream * lock( void );

  // Unlock log
  RAIBASE_DLL_EXP
  void unlock( void );

  // Flush log
  RAIBASE_DLL_EXP
  void flush( void )                                   throw( Error );

  // Copy Input stream to log verbatum
  RAIBASE_DLL_EXP
  void printLog( InputStream * is );

  RAIBASE_DLL_EXP
  void printLog( LogLevel level,  const char *where,  int lineno,
                 Error err,  const char *fmt,  ... )
#if defined( __GNUC__ )
    __attribute__((format(printf,5,6)));
#else
    ;
#endif
  RAIBASE_DLL_EXP
  void vprintLog( LogLevel level,  const char *where,  int lineno,
                  Error err,  const char *fmt,  va_list ap );
  RAIBASE_DLL_EXP
  void vprintSyslog( LogLevel level,  const char *where,  int lineno,
                     Error err,  const char *fmt,  va_list ap );
  RAIBASE_DLL_EXP
  void vprintLog2( TimeMSecs stamp,  LogLevel level,  const char *where,
                   int lineno,  Error e,  const char *instance,
                   const char *fmt,  va_list ap );
  RAIBASE_DLL_EXP
  void vprintSyslog2( TimeMSecs stamp,  LogLevel level,  const char *where,
                      int lineno,  Error e,  const char *instance,
                      const char *fmt,  va_list ap );
  RAIBASE_DLL_EXP
  void vprintLogOut( OutputStream *log,  unsigned int verb,  TimeMSecs stamp,
                     LogLevel level,  const char *file,  int lineno,  Error e,
                     const char *inst,  const char *fmt,  va_list ap )
                                                                throw( Error );
  RAIBASE_DLL_EXP
  void vprintLogXml( OutputStream *log,  unsigned int verb,  TimeMSecs stamp,
                     LogLevel level,  const char *file,  int lineno,  Error e,
                     const char *inst,  const char *fmt,  va_list ap )
                                                                throw( Error );
  RAIBASE_DLL_EXP
  void printLogHex( LogLevel level,  const char *where,  int lineno,
                    Error err,  const char *what, const byte *msg,
                    unsigned int msgSize );

#ifdef VALGRIND
#if defined( __GNUC__ )
#define NOINLINE __attribute__((noinline));
#else
#define NOINLINE
#endif

  bool dologDevel() NOINLINE;
  bool dologFTrace() NOINLINE;
  bool dologTrace() NOINLINE;
  bool dologDebug() NOINLINE;
  bool dologMinor() NOINLINE;
  bool dologNormal() NOINLINE;
  bool dovlogDevel() NOINLINE;
  bool dovlogFTrace() NOINLINE;
  bool dovlogTrace() NOINLINE;
  bool dovlogDebug() NOINLINE;
  bool dovlogMinor() NOINLINE;
  bool dovlogNormal() NOINLINE;

#define logDevel   if ( rai::Log::dologDevel() )  rai::Log::printLog 
#define logFTrace  if ( rai::Log::dologFTrace() ) rai::Log::printLog 
#define logTrace   if ( rai::Log::dologTrace() )  rai::Log::printLog 
#define logDebug   if ( rai::Log::dologDebug() )  rai::Log::printLog
#define logMinor   if ( rai::Log::dologMinor() )  rai::Log::printLog
#define logNormal  if ( rai::Log::dologNormal() ) rai::Log::printLog
#define vlogDevel  if ( rai::Log::dovlogDevel() )  rai::Log::vprintLog 
#define vlogFTrace if ( rai::Log::dovlogFTrace() ) rai::Log::vprintLog 
#define vlogTrace  if ( rai::Log::dovlogTrace() )  rai::Log::vprintLog 
#define vlogDebug  if ( rai::Log::dovlogDebug() )  rai::Log::vprintLog
#define vlogMinor  if ( rai::Log::dovlogMinor() )  rai::Log::vprintLog
#define vlogNormal if ( rai::Log::dovlogNormal() ) rai::Log::vprintLog
#define logError   rai::Log::printLog
#define vlogError  rai::Log::vprintLog
#define logDebugHex if ( rai::Log::dologDebug() )  rai::Log::printLogHex

#undef NOLINE

#else /* not VALGRIND */

#if defined( __GNUC__ )
/* use for log level minor and above likely */
#define RAILOG_YE(x) (__builtin_expect((x),1))
#define RAILOG_NO(x) (__builtin_expect((x),0))
#else
#define RAILOG_YE(x) (x)
#define RAILOG_NO(x) (x)
#endif

/* shorthand for printLog and arguments */
#define logDevel   if RAILOG_NO( rai::Log::minLevel == rai::Log::LVL_DEVEL )  \
                     rai::Log::printLog 
#define logFTrace  if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_FTRACE ) \
                     rai::Log::printLog 
#define logTrace   if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_TRACE )  \
                     rai::Log::printLog 
#define logDebug   if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_DEBUG )  \
                     rai::Log::printLog
#define logMinor   if RAILOG_YE( rai::Log::minLevel <= rai::Log::LVL_MINOR )  \
                     rai::Log::printLog
#define logNormal  if RAILOG_YE( rai::Log::minLevel <= rai::Log::LVL_NORMAL ) \
                     rai::Log::printLog
#define vlogDevel  if RAILOG_NO( rai::Log::minLevel == rai::Log::LVL_DEVEL )  \
                     rai::Log::vprintLog 
#define vlogFTrace if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_FTRACE ) \
                     rai::Log::vprintLog 
#define vlogTrace  if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_TRACE )  \
                     rai::Log::vprintLog 
#define vlogDebug  if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_DEBUG )  \
                     rai::Log::vprintLog
#define vlogMinor  if RAILOG_YE( rai::Log::minLevel <= rai::Log::LVL_MINOR )  \
                     rai::Log::vprintLog
#define vlogNormal if RAILOG_YE( rai::Log::minLevel <= rai::Log::LVL_NORMAL ) \
                     rai::Log::vprintLog
#define logError   rai::Log::printLog
#define vlogError  rai::Log::vprintLog
#define logDebugHex if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_DEBUG ) \
                     rai::Log::printLogHex

#endif /* VALGRIND */

#define LDEVEL  rai::Log::LVL_DEVEL, __FILE__, __LINE__, NULL
#define LFTRACE rai::Log::LVL_FTRACE, __FILE__, __LINE__, NULL
#define LTRACE  rai::Log::LVL_TRACE, __FILE__, __LINE__, NULL
#define LDEBUG  rai::Log::LVL_DEBUG, __FILE__, __LINE__, NULL
#define LMINOR  rai::Log::LVL_MINOR, __FILE__, __LINE__, NULL
#define LNORMAL rai::Log::LVL_NORMAL, __FILE__, __LINE__
#define LERROR  rai::Log::LVL_ERROR, __FILE__, __LINE__
}

/* object to pass around to filter on vprintLog() */
class RAIBASE_DLL_EXP LogV {
  public:
    virtual ~LogV() {};

/* use debugPrint( log, LDEBUG, ... ) 
   expands to if ( minLevel <= level ) log.printLog() */
#define develLog  if RAILOG_NO( rai::Log::minLevel == rai::Log::LVL_DEVEL )  \
                    rai::LogV::printLog 
#define ftraceLog if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_FTRACE ) \
                    rai::LogV::printLog 
#define traceLog  if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_TRACE )  \
                    rai::LogV::printLog 
#define debugLog  if RAILOG_NO( rai::Log::minLevel <= rai::Log::LVL_DEBUG )  \
                    rai::LogV::printLog
#define minorLog  if RAILOG_YE( rai::Log::minLevel <= rai::Log::LVL_MINOR )  \
                    rai::LogV::printLog
#define normalLog if RAILOG_YE( rai::Log::minLevel <= rai::Log::LVL_NORMAL ) \
                    rai::LogV::printLog
#define errorLog  rai::LogV::printLog

    static void printLog( LogV &log,  Log::LogLevel level,  const char *where,
                          int line,  Error err,  const char *fmt,  ... )
    #if defined( __GNUC__ )
        __attribute__((format(printf,6,7)));
    #else
        ;
    #endif
    static void printLog( LogV *log,  Log::LogLevel level,  const char *where,
                          int line,  Error err,  const char *fmt,  ... )
    #if defined( __GNUC__ )
        __attribute__((format(printf,6,7)));
    #else
        ;
    #endif
    static void vprintLog( LogV *log,  Log::LogLevel level,  const char *where, 
                           int lineno,  Error e,  const char *fmt,
                           va_list ap );
    virtual void vprintLog( Log::LogLevel level,  const char *where, 
                            int lineno,  Error e,  const char *fmt,
                            va_list ap ) = 0;
};

namespace LogErr {
  enum {
    OK                 = 0,
    NULL_PATH          = 1,
    BAD_PATH           = 2,
    BAD_ROLL_TYPE      = 3,
    UNKNOWN            = 4,
  };
  RAIBASE_DLL_EXP
  Error getErr( unsigned int status );
}

} // namespace Rai

#endif
