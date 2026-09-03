#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <signal.h>
#else
#include <windows.h>
#include <wincon.h>
#endif

#include "base/sys.h"
#include "base/mem.h"
#include "base/log.h"
#include "stream/io_stream.h"
#include "util/term.h"

using namespace rai;

namespace rai {
struct MyTermOutput : public TermOutput {
  char         termClear[ 16 ],
               termHome[ 16 ];
  unsigned int termClearSize,
               termHomeSize;
  bool         useStdio,
               winChg;
#if defined( _WIN32 ) || defined( _WIN64 )
  COORD  pos;
  HANDLE sb;
  CONSOLE_SCREEN_BUFFER_INFO info;
#endif

  SYS_OPS( MyTermOutput );
  MyTermOutput() : TermOutput() {};
  virtual ~MyTermOutput();

  virtual void init( void ) throw( Error );
  
  virtual void clear( void ) throw( Error );
  
  virtual void home( void ) throw( Error );
  
  virtual void writeBytes( const byte *buf,  unsigned int n ) throw( Error );
  
  virtual void flush( void ) throw( Error );

  virtual bool geomChanged( void );

  void getGeom( void );
};
}


static MyTermOutput *MyTerm;

extern "C" void
rai_MyTermOutput_sigWinChg( int s )
{
  logDebug( LDEBUG, "sigWinChg" );
  if ( MyTerm != NULL )
    MyTerm->winChg = true;
}


MyTermOutput::~MyTermOutput()
{
  if ( MyTerm == this )
    MyTerm = NULL;
}


TermOutput *
TermOutput::create( void ) throw( Error )
{
  MyTermOutput *mt = NEW MyTermOutput();
  mt->init();
  MyTerm = mt;
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  ::signal( SIGWINCH, rai_MyTermOutput_sigWinChg );
#endif
  return mt;
}

  
bool
MyTermOutput::geomChanged( void )
{
  if ( this->winChg ) {
    this->winChg = false;
    this->getGeom();
    return true;
  }
  return false;
}


void
MyTermOutput::getGeom( void )
{
  unsigned int ln = this->termLines,
               co = this->termCols;
  const char * l, * c;

  this->termLines = 0;
  this->termCols  = 0;

  if ( (l = ::getenv( "LINES" )) != NULL &&
       (c = ::getenv( "COLUMNS" )) != NULL ) {
    this->termLines = atoi( l );
    this->termCols  = atoi( c );
  }
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  if ( this->termLines == 0 || this->termCols == 0 ) {
    FILE * fp;
    char   buf[ 16 ];
    size_t n;
    if ( (fp = ::popen( "tput lines", "r" )) != NULL ) {
      n = ::fread( buf, 1, 16, fp );
      ::pclose( fp );
      if ( n > 0 )
        this->termLines = atoi( buf );
    }
    if ( (fp = ::popen( "tput cols", "r" )) != NULL ) {
      n = ::fread( buf, 1, 16, fp );
      ::pclose( fp );
      if ( n > 0 )
        this->termCols = atoi( buf );
    }
  }
#endif
  logDebug( LDEBUG, "getGeom [co:%u,ln:%u] old [co:%u,ln:%u]",
                  this->termCols, this->termLines, co, ln );
}


void
MyTermOutput::init( void ) throw( Error )
{
  ::memset( this->termClear, 0, sizeof( this->termClear ) );
  ::memset( this->termHome, 0, sizeof( this->termHome ) );
  this->termClearSize = 0;
  this->termHomeSize  = 0;
  this->termLines     = 0;
  this->termCols      = 0;

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  FILE * fp;
  size_t n;

  if ( (fp = ::popen( "tput clear", "r" )) != NULL ) {
    n = ::fread( this->termClear, 1, 16, fp );
    ::pclose( fp );
    if ( n > 0 )
      this->termClearSize = n;
  }
  if ( (fp = ::popen( "tput home", "r" )) != NULL ) {
    n = ::fread( this->termHome, 1, 16, fp );
    ::pclose( fp );
    if ( n > 0 )
      this->termHomeSize = n;
  }
  this->getGeom();
#endif
  this->useStdio = false;
  if ( ::getenv( "TERM" ) != NULL ) {
#if defined( _WIN32 ) || defined( _WIN64 )
    logMinor( LMINOR, "TERM=%s is set, not using windows console screen, "
                      "using terminal control sequences instead",
              getenv( "TERM" ) );
#endif
    this->useStdio = true;
  }
#if defined( _WIN32 ) || defined( _WIN64 )
  else {
    this->pos.X = 0; this->pos.Y = 0;
    this->sb = ::CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE, 0,
                                        NULL, CONSOLE_TEXTMODE_BUFFER,
                                        NULL );
    if ( this->sb == INVALID_HANDLE_VALUE ||
         ! ::GetConsoleScreenBufferInfo( this->sb, &this->info ) ||
         ! ::SetConsoleActiveScreenBuffer( this->sb ) ||
         ! ::SetConsoleMode( this->sb, ENABLE_PROCESSED_OUTPUT |
                                   ENABLE_WRAP_AT_EOL_OUTPUT ) )
      this->useStdio = true;
    else {
      this->termLines = this->info.dwSize.Y;
      this->termCols  = this->info.dwSize.X;
    }
  }
#endif
  if ( this->useStdio ) {
    if ( this->termClearSize == 0 ) {
      ::strcpy( this->termClear, "\033[H\033[J" );
      this->termClearSize = ::strlen( this->termClear );
    }
    if ( this->termHomeSize == 0 ) {
      ::strcpy( this->termHome, "\033[H" );
      this->termHomeSize = ::strlen( this->termHome );
    }
    if ( this->termLines == 0 ) {
      const char *s;
      if ( (s = ::getenv( "LINES" )) != NULL && atoi( s ) != 0 )
        this->termLines = atoi( s );
      else
        this->termLines = 24;
    }
    if ( this->termCols == 0 ) {
      const char *s;
      if ( (s = ::getenv( "COLUMNS" )) != NULL && atoi( s ) != 0 )
        this->termCols = atoi( s );
      else
        this->termCols = 80;
    }
  }
}


void
MyTermOutput::clear( void ) throw( Error )
{
  if ( this->useStdio ) {
    Sys::out->writeBytes( (byte *) this->termClear, this->termClearSize );
    Sys::out->flush();
  }
#if defined( _WIN32 ) || defined( _WIN64 )
  else {
    unsigned int sz = this->termLines * this->termCols;

    this->pos.X = 0;
    this->pos.Y = 0;
    ::FillConsoleOutputCharacter( this->sb, (TCHAR) ' ', sz, this->pos,
                                  NULL );
    ::SetConsoleCursorPosition( this->sb, this->pos );
  }
#endif
}


void
MyTermOutput::home( void ) throw( Error )
{
  if ( this->useStdio ) {
    Sys::out->writeBytes( (byte *) this->termHome, this->termHomeSize );
  }
#if defined( _WIN32 ) || defined( _WIN64 )
  else {
    this->pos.X = 0;
    this->pos.Y = 0;
    ::SetConsoleCursorPosition( this->sb, this->pos );
  }
#endif
}


void
MyTermOutput::writeBytes( const byte *buf,  unsigned int n ) throw( Error )
{
  if ( this->useStdio ) {
    Sys::out->writeBytes( buf, n );
  }
#if defined( _WIN32 ) || defined( _WIN64 )
  else {
    char line[ 256 ];
    unsigned int i, j = 0;
    for ( i = 0; i < n; i++ ) {
      if ( buf[ i ] == '\n' ) {
        line[ j++ ] = '\n';
        ::SetConsoleCursorPosition( this->sb, this->pos );
        ::WriteConsole( this->sb, line, j, NULL, NULL );
        this->pos.Y++;
        this->pos.X = 0;
        j = 0;
      }
      else {
        if ( j < 256 )
          line[ j++ ] = (char) buf[ i ];
      }
    }
    if ( j > 0 ) {
      ::SetConsoleCursorPosition( this->sb, this->pos );
      ::WriteConsole( this->sb, line, j, NULL, NULL );
      this->pos.X += j;
    }
  }
#endif
}


void
MyTermOutput::flush( void ) throw( Error )
{
  if ( this->useStdio ) {
    Sys::out->flush();
  }
}
