/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if !defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "base/sys.h"
#include "stream/stdio_stream.h"
#include "base/thread.h"
/*#include "net/sock.h"*/
#include "base/log.h"
#include "base/time.h"
#include "util/atomic.h"
#include "util/hash_util.h"

using namespace rai;

InputStream  * Sys::in;          /* stdin */
OutputStream * Sys::out,         /* stdout */
             * Sys::err;         /* stderr */
char           Sys::versionString[ 32 ];
static AtomicUInt initLck, spinLck;

#define STDIN_BUF_LEN  512U
#define STDOUT_BUF_LEN 512U
#define STDERR_BUF_LEN 256U


void
Sys::initialize( const char *vers ) throw( Error )
{
  while ( spinLck.xchg( 1 ) == 1 )
    Thread::sleep( 1 );
  if ( initLck.add( 1 ) == 0 ) {
    Log::LogLevel levelSave;

    int hashTest = Hash32::selftest();

    /* turn off debug logging for now */
    size_t len = ::strlen( vers );
    if ( len >= sizeof( Sys::versionString ) )
      len = sizeof( Sys::versionString ) - 1;
    ::memcpy( Sys::versionString, vers, len );
    Sys::versionString[ len ] = '\0';
    levelSave = Log::minLevel;
    if ( levelSave == Log::LVL_DEBUG )
      Log::minLevel = Log::LVL_NORMAL;

    try {
      // always thread safe
      //Thread::isThreadSafe = ( acc == IS_THREAD_SAFE ? true : false );
      //Thread::isThreadSafe = true;
      Mem::initialize();
      Time::initialize(); /* initialize time -> hires calc */
      Sys::err = StdioOutputStream::createStderr( STDERR_BUF_LEN );
      //Log::log = Sys::err;
      Sys::out = StdioOutputStream::createStdout( STDOUT_BUF_LEN );
      Sys::in  = StdioInputStream::createStdin( STDIN_BUF_LEN );
      Log::minLevel = levelSave;
      /*Socket::sockInit();*/

      //if ( acc == IS_THREAD_SAFE ) {
        Sys::err->initThreadAccess( true );
        Sys::out->initThreadAccess( true );
        Sys::in->initThreadAccess( true );
        /* log functions already lock out multiple threads */
      //}
      if ( hashTest != 0 ) {
        Sys::err->printf( "Hash32::selftest() failed, code=%d\n", hashTest );
        exit( 1 );
      }
    } catch( ... ) {
      Log::minLevel = levelSave;
      Sys::terminate();
      initLck.add( -1 );
      spinLck.xchg( 0 );
      throw;
    }
  }
  spinLck.xchg( 0 );
}


void
Sys::terminate( void )
{
  while ( spinLck.xchg( 1 ) == 1 )
    Thread::sleep( 1 );
  if ( initLck.add( -1 ) == 0 ) {
    OutputStream * out;
    InputStream  * in;
    Log::LogLevel  levelSave;

    /* turn off debug logging for now */
    levelSave = Log::minLevel;
    if ( levelSave == Log::LVL_DEBUG )
      Log::minLevel = Log::LVL_NORMAL;

  #if 0
    if ( Log::log != NULL && Log::log != Sys::err ) {
      out = Log::log;
      Log::log = NULL;
      try {
        out->close();
      } catch( Error ) {
      }
      delete out;
    }
  #endif

    if ( Sys::out != NULL ) {
      out = Sys::out;
      Sys::out = NULL;
      try {
        out->close();
      } catch( Error ) {
      }
      delete out;
    }

    if ( Sys::err != NULL ) {
      out = Sys::err;
      Sys::err = NULL;
      try {
        out->close();
      } catch( Error ) {
      }
      delete out;
    }

    if ( Sys::in != NULL ) {
      in = Sys::in;
      Sys::in = NULL;
      in->close();
      delete in;
    }

    /*Socket::sockTerm();*/
    Mem::terminate();
    Log::minLevel = levelSave;
  }
  spinLck.xchg( 0 );
}

