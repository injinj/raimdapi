/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "util/args.h"
#include "base/file.h"
#include "base/mem.h"
#include "util/str_util.h"
#include "stream/io_stream.h"
#include "stream/file_stream.h"

using namespace rai;

static const char COMMAND_LINE[] = "Command line";
static const char ENVIRONMENT[]  = "Environment";


struct ArgsDefaultOutputStream : public OutputStream {
  SYS_OPS( ArgsDefaultOutputStream );
  ArgsDefaultOutputStream() : OutputStream() {}

  virtual unsigned int emptyBuf( const byte *buf,
                                unsigned int bufLen );
};

unsigned int
ArgsDefaultOutputStream::emptyBuf( const byte *buf,
                                  unsigned int bufLen )
{
  ::fwrite( buf, 1, bufLen, stderr );
  return bufLen;
}

RAIBASE_DLL_EXP const StringArg * Args::log;
RAIBASE_DLL_EXP const StringArg * Args::logLevel;
RAIBASE_DLL_EXP const UIntArg   * Args::logRollCnt;
RAIBASE_DLL_EXP const StringArg * Args::logRollDateFmt;
RAIBASE_DLL_EXP const StringArg * Args::logRollType;
RAIBASE_DLL_EXP const DoubleArg * Args::logSizeLimit;
RAIBASE_DLL_EXP const UIntArg   * Args::logVerb;
RAIBASE_DLL_EXP const BoolArg   * Args::logXml;
RAIBASE_DLL_EXP const BoolArg   * Args::help;
RAIBASE_DLL_EXP const BoolArg   * Args::version;
RAIBASE_DLL_EXP const BoolArg   * Args::printRc;
static char rcFileName[ 256 ] = "args.ini";
RAIBASE_DLL_EXP const StringArg * Args::rcFile;

Args::Args()
{
  static const StringArg xlog(      "log", "-", "<file>",
                                  "Log filename, use '-' for stderr" );
  static const StringArg xlogLevel( "logLevel", "minor", "<dbg|min|norm|err>",
                                  "Error level of logging" );
  static const UIntArg   xlogRolloverCnt(  "logRollCnt", 0, "<0-n>",
                                           "How many log files to keep when rolling over logs. Zero means keep all logs." );
  static const StringArg xlogRolloverType( "logRollType", "date", "num",
                                           "The form of the suffix for rolled over file. "
                                           "\'date\' means use a data-time stamp. "
                                           "\'num\' means use a number. Most recent file gets lowest number, starting at 1" );
#if defined( _WIN32 ) || defined( _WIN64 )
  #define LOGFMTSTR ".%Y-%m-%d_%H-%M-%S"
#else
  #define LOGFMTSTR ".%Y-%m-%d_%H:%M:%S"
#endif
  static const StringArg xlogRolloverDateFmt( "logRollDateFmt", LOGFMTSTR, LOGFMTSTR,
                                              "This is the format of the suffix for rolled over file when "
                                              "the logRollType is \'date\'. See man page for strftime" );
  static const DoubleArg   xlogSizeLimit(  "logSizeLimit", 0, "<size>",     
                                         "Maximum size of log file, in bytes");
  static const UIntArg   xlogVerb(  "logVerb", 4, "<1-5>",     
                                  "Verbosity level of logging"
                      /*"1=nothing "
                      "2=\"errno; descr\" "
                      "3=\"time errno+reason; descr\" "
                      "4=\"time severity: errno+reason; descr\" "
                      "5=\"time severity: errno+reason; descr (file:lineno)\""*/);
  static const BoolArg   xlogXml(   "logXml",   false, NULL,
                                  "Use XML log format" );
  static const BoolArg   xhelp(     "help",     false, NULL,
                                  "Display help and exit" );
  static const BoolArg   xversion(  "version",  false, NULL,
                                  "Display version and exit" );
  static const BoolArg   xprintRc(  "printRC",  false, NULL,
                                  "Print arguments in resource format and exit" );
  static const StringArg xrcFile(   "rcFile",   rcFileName, "<file>",
                                    "Load arguments from resource file" );
  static ArgsDefaultOutputStream defOut;

  if ( Args::rcFile == NULL ) {
    Args::log            = &xlog;
    Args::logLevel       = &xlogLevel;
    Args::logRollCnt     = &xlogRolloverCnt;
    Args::logRollDateFmt = &xlogRolloverDateFmt;
    Args::logRollType    = &xlogRolloverType;
    Args::logSizeLimit   = &xlogSizeLimit;
    Args::logVerb        = &xlogVerb;
    Args::logXml         = &xlogXml;
    Args::help           = &xhelp;
    Args::version        = &xversion;
    Args::printRc        = &xprintRc;
    Args::rcFile         = &xrcFile;
  }
  this->argCount    = 0;
  this->versionInfo = "No version set";
  this->envPrefix   = "P_";
  this->out         = &defOut;
  this->copyArgs    = NULL;
}


bool
Args::processArgs( unsigned int argc,  char *argv[] )
{
  const char *name;

  if ( argc > 1 )
    this->addArgs( argc, argv );

  /* check if -help */
  if ( argc > 1 && (name = this->getName( HELP_ARG )) != NULL &&
       this->getBoolean( name ) ) {
    this->printHelp();
    this->out->flush();
  }
  /* check if -version */
  else if ( argc > 1 && (name = this->getName( VERSION_ARG )) != NULL &&
            this->getBoolean( name ) ) {
    this->printVersion();
    this->out->flush();
  }
  /* check if -printRC */
  else {
    const char * rcFile;

    if ( (name = this->getName( RCFILE_ARG )) != NULL &&
         (rcFile = this->getString( name )) != NULL ) {

      /* add .ini files, env and args overrides .ini file */
      if ( File::fileExists( rcFile ) )
        this->addRCFile( rcFile );
      this->addEnv();
      if ( argc > 1 )
        this->addArgs( argc, argv );
    }
    else {
      /* add env, args override */
      this->addEnv();
      if ( argc > 1 )
        this->addArgs( argc, argv );
    }

    if ( argc > 1 && (name = this->getName( PRINTRC_ARG )) != NULL &&
         this->getBoolean( name ) ) {
      this->printRC();
      this->out->flush();
    }
    else {
      /* expand ${arg} style values */
      this->expandArgs();

      return true;
    }
  }

  return false;
}


const char *
Args::getName( ArgFlags flags )
{
  unsigned int i;

  for ( i = 0; i < this->argCount; i++ ) {
    if ( ( this->args[ i ].flags & flags ) != 0 )
      return this->args[ i ].name;
  }

  return NULL;
}


void 
Args::clear( void )
{
  unsigned int i;

  for ( i = 0; i < this->argCount; i++ ) {
    if ( this->args[ i ].type == STRING_ARG ) {
      if ( this->args[ i ].val.s != NULL ) {
        FREE( this->args[ i ].val.s );
        this->args[ i ].val.s = NULL;
      }
      if ( this->args[ i ].vals != NULL ) {
        FREE( this->args[ i ].vals );
        this->args[ i ].vals = NULL;
      }
    }
  }
  while ( this->copyArgs != NULL ) {
    TmpArgList *next = this->copyArgs->next;
    FREE( this->copyArgs );
    this->copyArgs = next;
  }

  this->argCount = 0;
}


void
Args::printHelp( OutputStream *o ) const
{
  if ( o == NULL ) o = this->out;
  o->puts( "Usage:\n" );
  this->printHelp2( '-', ' ', o );
}


void
Args::printOptions( OutputStream *o ) const
{
  this->printHelp2( 0, '=', o );
}


void
Args::printHelp2( char optChar1,  char optChar2,
                  OutputStream *o ) const
{
  static unsigned int termWidth;
  const Arg  * p;
  unsigned int len,
               col,
               start,
               space,
               i,
               maxCol;
  char         line[ 80 * 5 ],
               num[ 128 ],
               descr[ 2 * 1024 ];

  if ( o == NULL ) o = this->out;
  if ( termWidth == 0 ) {
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
    FILE * fp;
    char   buf[ 16 ];
    size_t n;
    if ( (fp = ::popen( "tput cols", "r" )) != NULL ) {
      n = ::fread( buf, 1, 16, fp );
      ::pclose( fp );
      if ( n > 0 ) {
        for ( unsigned int i = 0; i < n; i++ )
          if ( isdigit( buf[ i ] ) )
            termWidth = termWidth * 10 + ( buf[ i ] - '0' );
        if ( termWidth < 50 )
          termWidth = 50;
        if ( termWidth > sizeof( line ) )
          termWidth = sizeof( line );
      }
    }
#endif
    if ( termWidth == 0 )
      termWidth = 80;
  }
  maxCol = termWidth - 1;
  line[ maxCol ] = '\0';

  for ( i = 0; i < this->argCount; i++ ) {

    p = &this->args[ i ];
    if ( ( p->flags & ( COMMAND_ARG | HELP_ARG | RCFILE_ARG |
                        VERSION_ARG | PRINTRC_ARG ) ) == 0 )
      continue;

    ::memset( line, ' ', maxCol );

    col = 2;
    if ( optChar1 != '\0' )
      line[ col++ ] = optChar1;
    len = ::strlen( p->name );
    if ( len > maxCol - col )
      len = maxCol - col;
    ::memcpy( &line[ col ], p->name, len );
    col += len;
    if ( optChar2 != '\0' && p->example != NULL )
      line[ col ] = optChar2;
    col++;

    if ( p->example != NULL ) {
      len = ::strlen( p->example );
      if ( len > maxCol - col )
        len = maxCol - col;
      ::memcpy( &line[ col ], p->example, len );
      col += len + 1;
    }

    if ( p->description != NULL ) {
      ::strcpy( descr, p->description );

      if ( (p->flags & ( HELP_ARG | VERSION_ARG | PRINTRC_ARG )) == 0 ) {
        if ( ( p->flags & NO_DEFAULT_VAL ) == 0 ) {
          switch ( p->type ) {
            case STRING_ARG:
              if ( p->defVal.s != NULL ) {
                ::strcat( descr, " (default: " );
                ::strcat( descr, p->defVal.s );
                ::strcat( descr, ")" );
              }
              break;

            case UINT_ARG:
              ::strcat( descr, " (default: " );
              if ( ( p->flags & TIME_SEC_ARG ) != 0 )
                StrUtil::intToString( p->defVal.i, num, sizeof( num ),
                                      U_SECONDS );
              else if ( ( p->flags & TIME_MS_ARG ) != 0 )
                StrUtil::intToString( p->defVal.i, num, sizeof( num ),
                                      U_MILLISECS );
              else if ( ( p->flags & MEM_ARG ) != 0 )
                StrUtil::intToString( p->defVal.i, num, sizeof( num ),
                                      U_MEMORY );
              else if ( ( p->flags & BITS_ARG ) != 0 )
                StrUtil::intToString( p->defVal.i, num, sizeof( num ),
                                      U_BITS );
              else
                StrUtil::intToString( p->defVal.i, num, sizeof( num ) );
              ::strcat( descr, num );
              ::strcat( descr, ")" );
              break;

            case ULLONG_ARG:
              ::strcat( descr, " (default: " );
              if ( ( p->flags & TIME_SEC_ARG ) != 0 )
                StrUtil::intToString( p->defVal.l, num, sizeof( num ),
                                      U_SECONDS );
              else if ( ( p->flags & TIME_MS_ARG ) != 0 )
                StrUtil::intToString( p->defVal.l, num, sizeof( num ),
                                      U_MILLISECS );
              else if ( ( p->flags & MEM_ARG ) != 0 )
                StrUtil::intToString( p->defVal.l, num, sizeof( num ),
                                      U_MEMORY );
              else if ( ( p->flags & BITS_ARG ) != 0 )
                StrUtil::intToString( p->defVal.l, num, sizeof( num ),
                                      U_BITS );
              else
                StrUtil::intToString( p->defVal.l, num, sizeof( num ) );
              ::strcat( descr, num );
              ::strcat( descr, ")" );
              break;

            case BOOL_ARG:
              if ( p->defVal.b )
                ::strcat( descr, " (default: yes)" );
              else
                ::strcat( descr, " (default: no)" );
              break;

            case DOUBLE_ARG:
              ::strcat( descr, " (default: " );
              if ( ( p->flags & TIME_SEC_ARG ) != 0 )
                StrUtil::floatToString( p->defVal.d, num, sizeof( num ),
                                        3, U_SECONDS );
              else if ( ( p->flags & TIME_MS_ARG ) != 0 )
                StrUtil::floatToString( p->defVal.d, num, sizeof( num ),
                                        3, U_MILLISECS );
              else if ( ( p->flags & MEM_ARG ) != 0 )
                StrUtil::floatToString( p->defVal.d, num, sizeof( num ),
                                        3, U_MEMORY );
              else if ( ( p->flags & BITS_ARG ) != 0 )
                StrUtil::floatToString( p->defVal.d, num, sizeof( num ),
                                        3, U_BITS );
              else
                StrUtil::floatToString( p->defVal.d, num, sizeof( num ),
                                        3 );
              ::strcat( descr, num );
              ::strcat( descr, ")" );
              break;
          }
        }
      }
      len = ::strlen( descr );

      start = ( col > 33 ) ? col : 33;
      for ( space = 0; space < len; ) {

        while ( descr[ space ] == ' ' )
          space++;
        if ( space >= len )
          break;

        for ( col = start; col < maxCol && space < len; )
          line[ col++ ] = descr[ space++ ];

        if ( space < len ) {
          while ( col > start && line[ col - 1 ] != ' ' ) {
            col--;
            space--;
          }
          while ( col > start && line[ col - 1 ] == ' ' ) {
            col--;
            space--;
          }

          if ( col == start ) {
            space += maxCol - start;
            col    = maxCol;
          }
        }
        start = 33;

        line[ col ] = '\0';
        o->printf( "%s\n", line );
        ::memset( line, ' ', maxCol );
      }
    }
    else {
      for ( col = maxCol - 1; col > 0 && line[ col ] == ' '; col-- )
        ;
      line[ col + 1 ] = '\0';
      o->printf( "%s\n", line );
    }
  }
}


bool
Args::Arg::defaultToString( char *buf,  unsigned int bufLen ) const

{
  const char * s;
  char         num[ 128 ];

  s = NULL;
  if ( ( this->flags & NO_DEFAULT_VAL ) == 0 ) {
    switch ( this->type ) {
      case STRING_ARG:
        s = this->defVal.s;
        break;

      case UINT_ARG:
        if ( ( this->flags & TIME_SEC_ARG ) != 0 )
          StrUtil::intToString( this->defVal.i, num, sizeof( num ),
                                U_SECONDS, false );
        else if ( ( this->flags & TIME_MS_ARG ) != 0 )
          StrUtil::intToString( this->defVal.i, num, sizeof( num ),
                                U_MILLISECS, false );
        else if ( ( this->flags & MEM_ARG ) != 0 )
          StrUtil::intToString( this->defVal.i, num, sizeof( num ),
                                U_MEMORY, false );
        else if ( ( this->flags & BITS_ARG ) != 0 )
          StrUtil::intToString( this->defVal.i, num, sizeof( num ),
                                U_BITS, false );
        else
          StrUtil::intToString( this->defVal.i, num, sizeof( num ),
                                U_DECIMAL, false );
        s = num;
        break;

      case ULLONG_ARG:
        if ( ( this->flags & TIME_SEC_ARG ) != 0 )
          StrUtil::intToString( this->defVal.l, num, sizeof( num ),
                                U_SECONDS, false );
        else if ( ( this->flags & TIME_MS_ARG ) != 0 )
          StrUtil::intToString( this->defVal.l, num, sizeof( num ),
                                U_MILLISECS, false );
        else if ( ( this->flags & MEM_ARG ) != 0 )
          StrUtil::intToString( this->defVal.l, num, sizeof( num ),
                                U_MEMORY, false );
        else if ( ( this->flags & BITS_ARG ) != 0 )
          StrUtil::intToString( this->defVal.l, num, sizeof( num ),
                                U_BITS, false );
        else
          StrUtil::intToString( this->defVal.l, num, sizeof( num ),
                                U_DECIMAL, false );
        s = num;
        break;

      case BOOL_ARG:
        if ( this->defVal.b )
          s = "true";
        else
          s = "false";
        break;

      case DOUBLE_ARG:
        if ( ( this->flags & TIME_SEC_ARG ) != 0 )
          StrUtil::floatToString( this->defVal.d, num, sizeof( num ),
                                  StrUtil::UNTIL_ZERO, U_SECONDS, false );
        else if ( ( this->flags & TIME_MS_ARG ) != 0 )
          StrUtil::floatToString( this->defVal.d, num, sizeof( num ),
                                  StrUtil::UNTIL_ZERO, U_MILLISECS, false );
        else if ( ( this->flags & MEM_ARG ) != 0 )
          StrUtil::floatToString( this->defVal.d, num, sizeof( num ),
                                  StrUtil::UNTIL_ZERO, U_MEMORY, false );
        else if ( ( this->flags & BITS_ARG ) != 0 )
          StrUtil::floatToString( this->defVal.d, num, sizeof( num ),
                                  StrUtil::UNTIL_ZERO, U_BITS, false );
        else
          StrUtil::floatToString( this->defVal.d, num, sizeof( num ),
                                  StrUtil::UNTIL_ZERO, U_DECIMAL, false );
        s = num;
        break;
    }
  }
  if ( s == NULL )
    return false;

  while ( *s != '\0' && bufLen > 1 ) {
    *buf++ = *s++;
    bufLen--;
  }
  *buf = '\0';
  return true;
}


void
Args::printVersion( OutputStream *o ) const
{
  if ( o == NULL ) o = this->out;
  o->puts( this->versionInfo );
  o->puts( "\n" );
}


void
Args::printRC( OutputStream *rcOut ) const
{
  const Arg    * p;
  unsigned int   len,
                 col,
                 space,
                 i,
                 iVal;
  ullong         lVal;
  double         dVal;
  char           line[ 64 * 1024 ],
                 num[ 128 ];
  const char   * arg;

  if ( rcOut == NULL )
    rcOut = this->out;
  rcOut->printf( "# Configuration generated for %s\n\n",
                  this->versionInfo );

  for ( i = 0; i < this->argCount; i++ ) {
    p = &this->args[ i ];

    if ( ( p->flags & RESOURCE_ARG ) == 0 )
      continue;

    if ( p->description != NULL ) {
      len = ::strlen( p->description );

      for ( space = 0; space < len; ) {

        ::strcpy( line, "# " );
        while ( p->description[ space ] == ' ' )
          space++;
        if ( space >= len )
          break;

        for ( col = 2; col < 79 && space < len; )
          line[ col++ ] = p->description[ space++ ];

        if ( space < len ) {
          while ( col > 2 && line[ col - 1 ] != ' ' ) {
            col--;
            space--;
          }
          while ( col > 2 && line[ col - 1 ] == ' ' ) {
            col--;
            space--;
          }

          if ( col == 2 ) {
            space += 79 - 2;
            col    = 79;
          }
        }

        line[ col ] = '\0';
        rcOut->printf( "%s\n", line );
      }
    }

    len = ::strlen( p->name );
    ::memcpy( line, p->name, len );
    line[ len ] = '=';
    col = len + 1;

    arg = NULL;
    switch ( p->type ) {
      case STRING_ARG:
        if ( p->numValues > 0 )
          arg = p->val.s;
        else if ( p->defVal.s != NULL )
          arg = p->defVal.s;
        break;

      case UINT_ARG:
        if ( p->numValues > 0 )
          iVal = p->val.i;
        else
          iVal = p->defVal.i;
        if ( ( p->flags & TIME_SEC_ARG ) != 0 )
          StrUtil::intToString( iVal, num, sizeof( num ), U_SECONDS );
        else if ( ( p->flags & TIME_MS_ARG ) != 0 )
          StrUtil::intToString( iVal, num, sizeof( num ), U_MILLISECS );
        else if ( ( p->flags & MEM_ARG ) != 0 )
          StrUtil::intToString( iVal, num, sizeof( num ), U_MEMORY );
        else if ( ( p->flags & BITS_ARG ) != 0 )
          StrUtil::intToString( iVal, num, sizeof( num ), U_BITS );
        else
          StrUtil::intToString( iVal, num, sizeof( num ) );
        arg = num;
        break;

      case ULLONG_ARG:
        if ( p->numValues > 0 )
          lVal = p->val.l;
        else
          lVal = p->defVal.l;
        if ( ( p->flags & TIME_SEC_ARG ) != 0 )
          StrUtil::intToString( lVal, num, sizeof( num ), U_SECONDS );
        else if ( ( p->flags & TIME_MS_ARG ) != 0 )
          StrUtil::intToString( lVal, num, sizeof( num ), U_MILLISECS );
        else if ( ( p->flags & MEM_ARG ) != 0 )
          StrUtil::intToString( lVal, num, sizeof( num ), U_MEMORY );
        else if ( ( p->flags & BITS_ARG ) != 0 )
          StrUtil::intToString( lVal, num, sizeof( num ), U_BITS );
        else
          StrUtil::intToString( lVal, num, sizeof( num ) );
        arg = num;
        break;

      case BOOL_ARG:
        if ( p->defVal.b )
          ::strcpy( num, "yes" );
        else
          ::strcpy( num, "no" );
        arg = num;
        break;

      case DOUBLE_ARG:
        if ( p->numValues > 0 )
          dVal = p->val.d;
        else
          dVal = p->defVal.d;
        if ( ( p->flags & TIME_SEC_ARG ) != 0 )
          StrUtil::floatToString( dVal, num, sizeof( num ), 3, U_SECONDS );
        else if ( ( p->flags & TIME_MS_ARG ) != 0 )
          StrUtil::floatToString( dVal, num, sizeof( num ), 3, U_MILLISECS );
        else if ( ( p->flags & MEM_ARG ) != 0 )
          StrUtil::floatToString( dVal, num, sizeof( num ), 3, U_MEMORY );
        else if ( ( p->flags & BITS_ARG ) != 0 )
          StrUtil::floatToString( dVal, num, sizeof( num ), 3, U_BITS );
        else
          StrUtil::floatToString( dVal, num, sizeof( num ), 3 );
        arg = num;
        break;
    }

    if ( arg != NULL ) {
      len = ::strlen( arg );
      if( len > sizeof( line ) - col ) {
        len = sizeof( line ) - col - 1;
        rcOut->printf( "# ERROR: Truncating line to %d characters\n", len );
      }
      ::memcpy( &line[ col ], arg, len );
      col += len;
    }
    if ( arg == NULL || ( p->flags & NO_DEFAULT_VAL ) != 0 ) {
      ::memmove( &line[ 2 ], line, col );
      line[ 0 ] = '#';
      line[ 1 ] = ' ';
      col += 2;
    }

/*    if ( p->example != NULL ) {
      ::memcpy( &line[ col ], "  # ", 4 );
      len = ::strlen( p->example );
      ::memcpy( &line[ col ], p->example, len );
      col += len;
    }*/

    line[ col ] = '\0';
    rcOut->printf( "%s\n\n", line );
  }
}


void
Args::expandArgs( void )
{
  const char * oldVal,
             * s;
  char       * ptr,
             * end,
             * exp,
             * expEnd,
               var[ 64 ],
               varVal[ 1024 ],
               expansion[ 1024 ];
  unsigned int i,
               j;

  expEnd = &expansion[ sizeof( expansion ) - 1 ];

  for ( i = 0; i < this->argCount; i++ ) {
    if ( this->args[ i ].type == STRING_ARG ) {
      if ( this->args[ i ].numValues > 0 ) {
        oldVal = this->args[ i ].val.s;
      }
      else {
        oldVal = this->args[ i ].defVal.s;
      }
      if ( oldVal == NULL )
        continue;

      ptr = (char *) ::strstr( oldVal, "${" );
      for ( j = 0; ptr != NULL && j < 20; j++ ) {
        if ( (end = ::strchr( &ptr[ 2 ], '}' )) != NULL &&
              (unsigned int) ( end - &ptr[ 2 ] ) < sizeof( var ) ) {
          ::strncpy( var, &ptr[ 2 ], end - &ptr[ 2 ] );
          var[ end - &ptr[ 2 ] ] = '\0';

          if ( this->getExpansion( var, varVal, sizeof( varVal ) ) ) {
            exp = expansion;
            for ( s = oldVal; exp < expEnd && s < ptr; s++ )
              *exp++ = *s;
            for ( s = varVal; exp < expEnd && *s != '\0'; s++ )
              *exp++ = *s;
            for ( s = &end[ 1 ]; exp < expEnd && *s != '\0'; s++ )
              *exp++ = *s;
            *exp++ = '\0';

            REALLOC( exp - expansion, &this->args[ i ].val.s );
            ::memcpy( this->args[ i ].val.s, expansion, exp - expansion );

            this->args[ i ].numValues = 1;
            oldVal = this->args[ i ].val.s;
            ptr    = this->args[ i ].val.s;
          }
        }

        ptr = ::strstr( &ptr[ 2 ], "${" );
      }
    }
  }
}


bool
Args::getExpansion( const char *name,  char *buf,  unsigned int bufLen )

{
  Arg        * p;
  const char * value;

  if ( (p = this->getArg( name )) != NULL ) {
    switch ( p->type ) {
      case STRING_ARG:
        buf[ bufLen - 1 ] = '\0';
        if ( p->numValues > 0 )
          ::strncpy( buf, p->val.s, bufLen - 1 );
        else if ( p->defVal.s != NULL )
          ::strncpy( buf, p->defVal.s, bufLen - 1 );
        else
          return false;
        return true;

      case UINT_ARG:
        if ( p->numValues > 0 )
          StrUtil::intToString( p->val.i, buf, bufLen );
        else
          StrUtil::intToString( p->defVal.i, buf, bufLen );
        return true;

      case ULLONG_ARG:
        if ( p->numValues > 0 )
          StrUtil::intToString( p->val.l, buf, bufLen );
        else
          StrUtil::intToString( p->defVal.l, buf, bufLen );
        return true;

      default:
        break;
    }
  }
  else {
    if ( (value = ::getenv( name )) != NULL ) {
      ::strncpy( buf, value, bufLen - 1 );
      buf[ bufLen - 1 ] = '\0';
      return true;
    }
  }

  return false;
}


void
Args::matchArgs( const char *name,  unsigned int argc,  char **argv,
                 unsigned int *numMatched,  const char *source )

{
  Arg        * p;
  char       * ptr,
             * buf,
            ** argPtr;
  unsigned int i,
               size,
               len,
               argCount;

  *numMatched = 0;

  if ( (p = this->getArg( name )) == NULL ) {
    this->out->printf( "%s: Unknown argument: %s\n", source, name );
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  }

  if ( argc == 0 && p->type != BOOL_ARG ) {
    this->out->printf( "%s: Expecting argument of type %s after %s\n",
                       source, p->typeString(), name );
    throw ArgsErr::getErr( ArgsErr::MISSING_ARG );
  }

  buf    = NULL;
  argPtr = NULL;

  try {
    unsigned int matchFirst = 1;
    if ( ( p->flags & LIST_ARG ) != 0 && source != COMMAND_LINE && argc == 1 ) {
      for ( ptr = argv[ 0 ]; *ptr != '\0'; ptr++ )
        if ( *ptr <= ' ' )
          break;
      if ( *ptr != '\0' ) {
        len = ::strlen( argv[ 0 ] );
        MALLOC( len + 1, &buf );
        MALLOC( sizeof( argPtr[ 0 ] ), &argPtr );
        ::strcpy( buf, argv[ 0 ] );
        argPtr[ 0 ] = buf;
        argCount = 1;
        ptr = &buf[ ptr - argv[ 0 ] ];
        for (;;) {
          do {
            *ptr++ = '\0';
          } while ( ptr < &buf[ len ] && *ptr <= ' ' );
          if ( ptr == &buf[ len ] )
            break;
          REALLOC( sizeof( argPtr[ 0 ] ) * ( argCount + 1 ), &argPtr );
          argPtr[ argCount++ ] = ptr;
          do {
            ptr++;
          } while ( ptr < &buf[ len ] && *ptr > ' ' );
          if ( ptr == &buf[ len ] )
            break;
        }
        argv = argPtr;
        argc = argCount;
      }
    }
    else if ( ( p->flags & ( TIME_SEC_ARG | TIME_MS_ARG | MEM_ARG |
                    BITS_ARG ) ) != 0 && source == COMMAND_LINE && argc == 2 ) {
      unsigned int len1 = ::strlen( argv[ 0 ] );
      len = len1 + 1 + ::strlen( argv[ 1 ] );
      MALLOC( len + 1, &buf );
      MALLOC( sizeof( argPtr[ 0 ] ), &argPtr );
      ::strcpy( buf, argv[ 0 ] );
      buf[ len1++ ] = ' ';
      ::strcpy( &buf[ len1 ], argv[ 1 ] );
      argPtr[ 0 ] = buf;
      argCount = 1;
      argv = argPtr;
      argc = argCount;
      matchFirst = 2;
    }

    switch ( p->type ) {
      case STRING_ARG:
        if ( argv[ 0 ] == NULL ) {
          if ( p->val.s != NULL ) {
            FREE( p->val.s );
            p->val.s = NULL;
          }
          p->numValues = 0;
          *numMatched = 1;
          break;
        }
        REALLOC( ::strlen( argv[ 0 ] ) + 1, &p->val.s );
        ::strcpy( p->val.s, argv[ 0 ] );
        p->numValues = 1;
        *numMatched = matchFirst;

        if ( ( p->flags & LIST_ARG ) != 0 && argc > 1 ) {
          size = ( argc - 1 ) * sizeof( p->vals[ 0 ] );
          for ( i = 1; i < argc; i++ )
            size += ::strlen( argv[ i ] ) + 1;
          REALLOC( size, &p->vals );
          ptr = (char *) &p->vals[ argc - 1 ];
          for ( i = 1; i < argc; i++ ) {
            p->vals[ i - 1 ].s = ptr;
            len = ::strlen( argv[ i ] );
            ::memcpy( ptr, argv[ i ], len );
            ptr = &ptr[ len ];
            *ptr++ = '\0';
          }
          p->numValues = argc;
          *numMatched = argc;
        }
        break;

      case UINT_ARG:
      case ULLONG_ARG:
      case DOUBLE_ARG:
        try {
          i = 0;
          if ( p->type == UINT_ARG ) {
            p->val.i = p->parseUInt( argv[ 0 ] );
            p->numValues = 1;
            *numMatched = matchFirst;

            if ( ( p->flags & LIST_ARG ) != 0 && argc > 1 ) {
              size = ( argc - 1 ) * sizeof( p->vals[ 0 ] );
              REALLOC( size, &p->vals );
              for ( i = 1; i < argc; i++ ) {
                p->vals[ i - 1 ].i = p->parseUInt( argv[ i ] );
              }
              p->numValues = argc;
              *numMatched = argc;
            }
          }
          else if ( p->type == ULLONG_ARG ) {
            p->val.l = p->parseULLong( argv[ 0 ] );
            p->numValues = 1;
            *numMatched = matchFirst;

            if ( ( p->flags & LIST_ARG ) != 0 && argc > 1 ) {
              size = ( argc - 1 ) * sizeof( p->vals[ 0 ] );
              REALLOC( size, &p->vals );
              for ( i = 1; i < argc; i++ ) {
                p->vals[ i - 1 ].l = p->parseULLong( argv[ i ] );
              }
              p->numValues = argc;
              *numMatched = argc;
            }
          }
          else {
            p->val.d = p->parseDouble( argv[ 0 ] );
            p->numValues = 1;
            *numMatched = matchFirst;

            if ( ( p->flags & LIST_ARG ) != 0 && argc > 1 ) {
              size = ( argc - 1 ) * sizeof( p->vals[ 0 ] );
              REALLOC( size, &p->vals );
              for ( i = 1; i < argc; i++ ) {
                p->vals[ i - 1 ].d = p->parseDouble( argv[ i ] );
              }
              p->numValues = argc;
              *numMatched = argc;
            }
          }
        } catch ( Error e ) {
          this->out->printf(
                         "%s: Arg \"%s\" after option %s should be type %s%s\n",
                         source, argv[ i ], p->name, p->typeString(),
                         ( p->flags & TIME_SEC_ARG ) != 0 ? ", in seconds":
                         ( p->flags & TIME_MS_ARG ) != 0 ? ", in millisecs" :
                         ( p->flags & MEM_ARG ) != 0 ? ", in bytes" : 
                         ( p->flags & BITS_ARG ) != 0 ? ", in bits" : "" );
          throw e;
        }
        break;

      case BOOL_ARG:
        p->numValues = 1;
        if ( argc > 0 ) {
          p->val.b = StrUtil::parseBoolean( argv[ 0 ] );
          *numMatched = 1;
        }
        else {
          p->val.b = true;
        }
        break;
    }
  } catch ( Error e ) {
    if ( buf != NULL )
      FREE( buf );
    if ( argPtr != NULL )
      FREE( argPtr );
    throw e;
  }

  if ( buf != NULL )
    FREE( buf );
  if ( argPtr != NULL )
    FREE( argPtr );
}


unsigned int
Args::Arg::parseUInt( const char *arg )
{
  unsigned int i;

  if ( ( this->flags & TIME_SEC_ARG ) != 0 )
    StrUtil::parseInt( arg, &i, NULL, U_SECONDS );
  else if ( ( this->flags & TIME_MS_ARG ) != 0 )
    StrUtil::parseInt( arg, &i, NULL, U_MILLISECS );
  else if ( ( this->flags & MEM_ARG ) != 0 )
    StrUtil::parseInt( arg, &i, NULL, U_MEMORY );
  else if ( ( this->flags & BITS_ARG ) != 0 )
    StrUtil::parseInt( arg, &i, NULL, U_BITS );
  else
    StrUtil::parseInt( arg, &i );
  return i;
}


ullong
Args::Arg::parseULLong( const char *arg )
{
  ullong i;

  if ( ( this->flags & TIME_SEC_ARG ) != 0 )
    StrUtil::parseInt( arg, &i, NULL, U_SECONDS );
  else if ( ( this->flags & TIME_MS_ARG ) != 0 )
    StrUtil::parseInt( arg, &i, NULL, U_MILLISECS );
  else if ( ( this->flags & MEM_ARG ) != 0 )
    StrUtil::parseInt( arg, &i, NULL, U_MEMORY );
  else if ( ( this->flags & BITS_ARG ) != 0 )
    StrUtil::parseInt( arg, &i, NULL, U_BITS );
  else
    StrUtil::parseInt( arg, &i );
  return i;
}


double
Args::Arg::parseDouble( const char *arg )
{
  double d;

  if ( ( this->flags & TIME_SEC_ARG ) != 0 )
    StrUtil::parseFloat( arg, &d, NULL, U_SECONDS );
  else if ( ( this->flags & TIME_MS_ARG ) != 0 )
    StrUtil::parseFloat( arg, &d, NULL, U_MILLISECS );
  else if ( ( this->flags & MEM_ARG ) != 0 )
    StrUtil::parseFloat( arg, &d, NULL, U_MEMORY );
  else if ( ( this->flags & BITS_ARG ) != 0 )
    StrUtil::parseFloat( arg, &d, NULL, U_BITS );
  else
    StrUtil::parseFloat( arg, &d );
  return d;
}


void
Args::addArgs( unsigned int argc,  char **argv )
{
  const char * name;
  unsigned int i,
               j,
               numMatched;

  for ( i = 1; i < argc; ) {

    if ( argv[ i ][ 0 ] != '-' ) {
      this->out->printf( "%s: Missing argument: %s\n", COMMAND_LINE,
                         argv[ i ] );
      throw ArgsErr::getErr( ArgsErr::MISSING_ARG );
    }
    else {
      name = &argv[ i ][ 1 ];
      if ( name[ 0 ] == '-' && name[ 1 ] != '\0' )
        name++;
      i++;
      for ( j = 0; j + i < argc; j++ ) {
        if ( argv[ j + i ][ 0 ] == '-' ) {
          if ( argv[ j + i ][ 1 ] != '\0' ) {
            /* if not a negative number */
            if ( ! isdigit( argv[ j + i ][ 1 ] ) &&
                 argv[ j + i ][ 1 ] != '.' )
              break;
          }
        }
      }

      this->matchArgs( name, j, &argv[ i ], &numMatched, COMMAND_LINE );

      i += numMatched;
    }
  }
}


void
Args::processParms( const char * const parms[],  char *parmVals[],
                    unsigned int numParms,  const char *source )
{
  unsigned int i,
               numMatched;

  for ( i = 0; i < numParms; i++ ) {
    this->matchArgs( parms[ i ], 1, &parmVals[ i ], &numMatched, source );
  }
}


void
Args::addRCFile( const char *path )
{
  InputStream * in;

  if ( path == NULL )
    return;

  in = FileInputStream::open( path );

  try {
    this->addRCInput( in, path );
    in->close();
  } catch ( ... ) {
    delete in;
    throw;
  }
  delete in;
}


void
Args::addRCInput( InputStream *in,  const char *src )
{
  char          line[ 64 * 1024 ],
                source[ 64 * 1024 ],
              * srcLinePtr,
              * name,
              * val,
              * equalPtr;
  unsigned int  numMatched,
                len,
                lineCount,
                nameLen,
                valLen;

  source[ sizeof( source ) - 5 ] = '\0';
  ::strncpy( source, src, sizeof( source ) - 5 );
  srcLinePtr = &source[ ::strlen( source ) ];
  *srcLinePtr++ = ':';

  for ( lineCount = 1; ; lineCount++ ) {
    if ( (len = in->gets( line, sizeof( line ) ) ) == 0 )
      break;

    StrUtil::stripNewline( line, &len );

    /* if not a comment */
    if ( len > 0 && line[ 0 ] != '#' && line[ 0 ] != ';' &&
                    line[ 0 ] != '*' && line[ 0 ] != '[' ) {

      StrUtil::intToString( lineCount, srcLinePtr,
                            &source[ sizeof( source ) ] - srcLinePtr );

      if ( (equalPtr = ::strchr( line, '=' )) != NULL ) {
        name      = line;
        nameLen   = equalPtr - line;
        *equalPtr = '\0';
        StrUtil::trimWhitespace( name, &nameLen );

        val    = &equalPtr[ 1 ];
        valLen = &line[ len ] - &equalPtr[ 1 ];
        StrUtil::trimWhitespace( val, &valLen );


        if ( name[ 0 ] != '\0' && val[ 0 ] != '\0' ) {
          this->matchArgs( name, 1, &val, &numMatched, source );
          if ( numMatched < 1 ) {
            this->out->printf( "%s: Arg to %s didn't match \"%s\"\n",
                               source, name, val );
            throw ArgsErr::getErr( ArgsErr::ARG_UNMATCHED );
          }
        }
      }
      else {
        this->out->printf( "%s: Arg not formatted \"arg=value\"\n",
                           source );
        throw ArgsErr::getErr( ArgsErr::BAD_RESOURCE );
      }
    }
  }
}


void
Args::addEnv( void )
{
  Arg        * p;
  char         envVar[ 80 ],
             * envPtr,
             * value;
  unsigned int i,
               varLen,
               numMatched;

  if ( this->envPrefix == NULL )
    return;

  ::strncpy( envVar, this->envPrefix, sizeof( envVar ) - 1 );
  envVar[ sizeof( envVar ) - 1 ] = '\0';
  envPtr = &envVar[ ::strlen( envVar ) ];
  varLen = &envVar[ sizeof( envVar ) - 1 ] - envPtr;

  for ( i = 0; i < this->argCount; i++ ) {

    p = &this->args[ i ];

    if ( ( p->flags & RESOURCE_ARG ) == 0 )
      continue;

    ::strncpy( envPtr, p->name, varLen );
    if ( ( value = ::getenv( envVar )) != NULL )
      this->matchArgs( p->name, 1, &value, &numMatched, ENVIRONMENT );
  }
}


bool
Args::exists( const char *name ) const
{
  if ( this->getArgByName( name ) == NULL )
    return false;
  return true;
}


const Args::Arg *
Args::getArgByName( const char *name ) const
{
  unsigned int i;
  for ( i = 0; i < this->argCount; i++ ) {
    if ( ::strcmp( this->args[ i ].name, name ) == 0 )
      return &this->args[ i ];
  }
  return NULL;
}


unsigned int
Args::getArgFlags( const char *name ) const
{
  const Arg * p = this->getArgByName( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );

  return p->flags;
}


Args::ArgType
Args::getArgType( const char *name ) const
{
  const Arg * p = this->getArgByName( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  return p->type;
}


unsigned int
Args::getNumValues( const char *name ) const
{
  const Arg * p = this->getArgByName( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  //if ( ( p->flags & LIST_ARG ) == 0 )
  //  throw ArgsErr::getErr( ArgsErr::NOT_A_LIST );
  return p->numValues;
}


const char *
Args::getString( const char *name,  unsigned int n ) const
{
  const Arg * p = this->getArgByName( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type != STRING_ARG )
    throw ArgsErr::getErr( ArgsErr::EXPECTING_STRING );
  if ( n > 0 && ( p->flags & LIST_ARG ) == 0 )
    throw ArgsErr::getErr( ArgsErr::NOT_A_LIST );

  if ( n < p->numValues ) {
    if ( n == 0 )
      return p->val.s;
    return p->vals[ n - 1 ].s;
  }
  return p->defVal.s;
}


void
Args::setString( const char *name,  const char *val )
{
  const char *p[ 1 ] = { name };
  char *v[ 1 ] = { (char *) val };
  this->processParms( p, v, 1, "setString()" );
#if 0
  Arg * p = this->getArg( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type == STRING_ARG )
    STRDUP( p->val.s, val );
  else if ( p->type == UINT_ARG )
    p->val.i = p->parseUInt( val );
  else if ( p->type == ULLONG_ARG )
    p->val.l = p->parseULLong( val );
  else if ( p->type == BOOL_ARG )
    p->val.b = StrUtil::parseBoolean( val );
  else
    p->val.d = p->parseDouble( val );
  if ( p->numValues == 0 )
    p->numValues = 1;
#endif
}


unsigned int
Args::getUInt( const char *name,  unsigned int n ) const
{
  const Arg * p = this->getArgByName( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type != UINT_ARG )
    throw ArgsErr::getErr( ArgsErr::EXPECTING_UINT );
  if ( n > 0 && ( p->flags & LIST_ARG ) == 0 )
    throw ArgsErr::getErr( ArgsErr::NOT_A_LIST );

  if ( n < p->numValues ) {
    if ( n == 0 )
      return p->val.i;
    return p->vals[ n - 1 ].i;
  }
  return p->defVal.i;
}


void
Args::setUInt( const char *name,  unsigned int val )
{
  Arg * p = this->getArg( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type != UINT_ARG )
    throw ArgsErr::getErr( ArgsErr::EXPECTING_UINT );

  p->val.i = val;
  if ( p->numValues == 0 )
    p->numValues = 1;
}


ullong
Args::getULLong( const char *name,  unsigned int n ) const
{
  const Arg * p = this->getArgByName( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type != ULLONG_ARG )
    throw ArgsErr::getErr( ArgsErr::EXPECTING_ULLONG );
  if ( n > 0 && ( p->flags & LIST_ARG ) == 0 )
    throw ArgsErr::getErr( ArgsErr::NOT_A_LIST );

  if ( n < p->numValues ) {
    if ( n == 0 )
      return p->val.l;
    return p->vals[ n - 1 ].l;
  }
  return p->defVal.l;
}


void
Args::setULLong( const char *name,  ullong val )
{
  Arg * p = this->getArg( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type != ULLONG_ARG )
    throw ArgsErr::getErr( ArgsErr::EXPECTING_ULLONG );

  p->val.l = val;
  if ( p->numValues == 0 )
    p->numValues = 1;
}


bool
Args::getBoolean( const char *name,  unsigned int n ) const
{
  const Arg * p = this->getArgByName( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type != BOOL_ARG )
    throw ArgsErr::getErr( ArgsErr::EXPECTING_BOOL );
  if ( n > 0 && ( p->flags & LIST_ARG ) == 0 )
    throw ArgsErr::getErr( ArgsErr::NOT_A_LIST );

  if ( n < p->numValues ) {
    if ( n == 0 )
      return p->val.b;
    return p->vals[ n - 1 ].b;
  }
  return p->defVal.b;
}


void
Args::setBoolean( const char *name,  bool val )
{
  Arg * p = this->getArg( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type != BOOL_ARG )
    throw ArgsErr::getErr( ArgsErr::EXPECTING_BOOL );

  p->val.b = val;
  if ( p->numValues == 0 )
    p->numValues = 1;
}


double
Args::getDouble( const char *name,  unsigned int n ) const
{
  const Arg * p = this->getArgByName( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type != DOUBLE_ARG )
    throw ArgsErr::getErr( ArgsErr::EXPECTING_DOUBLE );
  if ( n > 0 && ( p->flags & LIST_ARG ) == 0 )
    throw ArgsErr::getErr( ArgsErr::NOT_A_LIST );

  if ( n < p->numValues ) {
    if ( n == 0 )
      return p->val.d;
    return p->vals[ n - 1 ].d;
  }
  return p->defVal.d;
}


void
Args::setDouble( const char *name,  double val )
{
  Arg * p = this->getArg( name );
  if ( p == NULL )
    throw ArgsErr::getErr( ArgsErr::UNKNOWN_ARG );
  if ( p->type != DOUBLE_ARG )
    throw ArgsErr::getErr( ArgsErr::EXPECTING_DOUBLE );

  p->val.d = val;
  if ( p->numValues == 0 )
    p->numValues = 1;
}


bool
Args::isSet( const char *name,  bool orHasDefVal ) const
{
  const Arg * p = this->getArgByName( name );
  if ( p == NULL )
    return false;
  if ( p->numValues > 0 )
    return true;
  if ( orHasDefVal && ( p->flags & NO_DEFAULT_VAL ) == 0 &&
       ( p->type != STRING_ARG || p->defVal.s != NULL ) )
    return true;
  return false;
}


Args::Arg *
Args::getArg( const char *name )
{
  unsigned int i;
  for ( i = 0; i < this->argCount; i++ ) {
    if ( ::strcmp( this->args[ i ].name, name ) == 0 )
      return &this->args[ i ];
  }
  return NULL;
}


bool
Args::removeArg( const char *name )
{
  Arg * p;
  if ( (p = this->getArg( name )) != NULL ) {
    unsigned int off = &p[ 1 ] - this->args;
    ::memmove( &p[ 0 ], &p[ 1 ], ( this->argCount - off ) * sizeof ( p[ 0 ] ) );
    this->argCount--;
    return true;
  }
  return false;
}


Args::Arg *
Args::addType( ArgType type,  const char *name,  const char *example,
               const char *description,  unsigned int flags )
{
  Arg * p;

  if ( this->getArg( name ) != NULL )
    throw ArgsErr::getErr( ArgsErr::ARG_EXISTS );

  if ( this->argCount == MAX_ARG_COUNT )
    throw ArgsErr::getErr( ArgsErr::TOO_MANY_ARGS );

  p = &this->args[ this->argCount++ ];
  ::memset( p, 0, sizeof( p[ 0 ] ) );

  p->name        = name;
  p->example     = example;
  p->description = description;
  p->type        = type;
  p->flags       = flags;

  return p;
}


void
Args::add( const StringArg *arg,  unsigned int flags )
{
  Arg * p;

  p = this->addType( STRING_ARG, arg->name, arg->example, arg->description,
                     flags );
  p->defVal.s = arg->defVal;
}


void
Args::add( const UIntArg *arg,  unsigned int flags )
{
  Arg * p;

  p = this->addType( UINT_ARG, arg->name, arg->example, arg->description,
                     flags );
  p->defVal.i = arg->defVal;
}


void
Args::add( const ULLongArg *arg,  unsigned int flags )
{
  Arg * p;

  p = this->addType( ULLONG_ARG, arg->name, arg->example, arg->description,
                     flags );
  p->defVal.l = arg->defVal;
}


void
Args::add( const BoolArg *arg,  unsigned int flags )
{
  Arg * p;

  p = this->addType( BOOL_ARG, arg->name, arg->example, arg->description,
                     flags );
  p->defVal.b = arg->defVal;
}


void
Args::add( const DoubleArg *arg,  unsigned int flags )
{
  Arg * p;

  p = this->addType( DOUBLE_ARG, arg->name, arg->example, arg->description,
                     flags );
  p->defVal.d = arg->defVal;
}


Args::TmpArgList *
Args::allocArg( ArgType t,  const char *name,  const char *example,
                const char *descr,  const char *defVal )
{
  TmpArgList * tmp;
  char       * p, * n, * e, * d, * v;
  unsigned int size = sizeof( TmpArgList ),
               nlen = ( name != NULL ? ::strlen( name ) + 1 : 0 ),
               elen = ( example != NULL ? ::strlen( example ) + 1 : 0 ),
               dlen = ( descr != NULL ? ::strlen( descr ) + 1 : 0 ),
               vlen = ( defVal != NULL ? ::strlen( defVal ) + 1 : 0 );

  switch ( t ) {
    case STRING_ARG:  size += sizeof( StringArg ); break;
    case UINT_ARG:    size += sizeof( UIntArg );   break;
    case BOOL_ARG:    size += sizeof( BoolArg );   break;
    case ULLONG_ARG:  size += sizeof( ULLongArg ); break;
    default:          size += sizeof( DoubleArg ); break;
  }
  MALLOC( size + nlen + elen + dlen + vlen, &tmp );
  ::memset( tmp, 0, size + nlen + elen + dlen + vlen );
  p = &((char *) tmp)[ sizeof( TmpArgList ) ];
  n = ( nlen == 0 ? NULL : &((char *) tmp)[ size ] );
  e = ( elen == 0 ? NULL : &((char *) tmp)[ size + nlen ] );
  d = ( dlen == 0 ? NULL : &((char *) tmp)[ size + nlen + elen ] );
  v = ( vlen == 0 ? NULL : &((char *) tmp)[ size + nlen + elen + dlen ] );
  switch ( t ) {
    case STRING_ARG:  tmp->u.sa = (StringArg *) p;
                      tmp->u.sa->name        = n;
                      tmp->u.sa->example     = e;
                      tmp->u.sa->description = d;
                      tmp->u.sa->defVal      = v; break;
    case UINT_ARG:    tmp->u.ia = (UIntArg *) p;
                      tmp->u.ia->name        = n;
                      tmp->u.ia->example     = e;
                      tmp->u.ia->description = d; break;
    case BOOL_ARG:    tmp->u.ba = (BoolArg *) p;
                      tmp->u.ba->name        = n;
                      tmp->u.ba->example     = e;
                      tmp->u.ba->description = d; break;
    case ULLONG_ARG:  tmp->u.la = (ULLongArg *) p;
                      tmp->u.la->name        = n;
                      tmp->u.la->example     = e;
                      tmp->u.la->description = d; break;
    default:          tmp->u.da = (DoubleArg *) p;
                      tmp->u.da->name        = n;
                      tmp->u.da->example     = e;
                      tmp->u.da->description = d; break;
  }
  if ( name ) ::strcpy( n, name );
  if ( example ) ::strcpy( e, example );
  if ( descr ) ::strcpy( d, descr );
  if ( defVal ) ::strcpy( v, defVal );

  return tmp;
}


void
Args::copy( const StringArg &arg,  unsigned int flags )
{
  TmpArgList * tmp = this->allocArg( STRING_ARG, arg.name, arg.example,
                                     arg.description, arg.defVal );
  this->add( tmp->u.sa, flags );
}


void
Args::copy( const UIntArg &arg,  unsigned int flags )
{
  TmpArgList * tmp = this->allocArg( UINT_ARG, arg.name, arg.example,
                                     arg.description, NULL );
  tmp->u.ia->defVal = arg.defVal;
  this->add( tmp->u.ia, flags );
}


void
Args::copy( const ULLongArg &arg,  unsigned int flags )
{
  TmpArgList * tmp = this->allocArg( ULLONG_ARG, arg.name, arg.example,
                                     arg.description, NULL );
  tmp->u.la->defVal = arg.defVal;
  this->add( tmp->u.la, flags );
}


void
Args::copy( const BoolArg &arg,  unsigned int flags )
{
  TmpArgList * tmp = this->allocArg( BOOL_ARG, arg.name, arg.example,
                                     arg.description, NULL );
  tmp->u.ba->defVal = arg.defVal;
  this->add( tmp->u.ba, flags );
}


void
Args::copy( const DoubleArg &arg,  unsigned int flags )
{
  TmpArgList * tmp = this->allocArg( DOUBLE_ARG, arg.name, arg.example,
                                     arg.description, NULL );
  tmp->u.da->defVal = arg.defVal;
  this->add( tmp->u.da, flags );
}


void
Args::addDefaults( const char *vers,  const char *pref,  OutputStream *out,
                   const char *argv0 )
{
  this->addDefaults( vers, pref, out, argv0, false );
}

void
Args::addDefaults( const char *vers,  const char *pref,  OutputStream *out,
                   const char *argv0,  bool addLogRoll )
{
  const char * ptr;
  unsigned int i;

  this->add( log,             COMMAND_ARG | RESOURCE_ARG );
  this->add( logLevel,        COMMAND_ARG | RESOURCE_ARG );
  if ( addLogRoll ) {
    this->add( logRollCnt,    COMMAND_ARG | RESOURCE_ARG );
    this->add( logRollDateFmt,COMMAND_ARG | RESOURCE_ARG );
    this->add( logRollType,   COMMAND_ARG | RESOURCE_ARG );
    this->add( logSizeLimit,  COMMAND_ARG | RESOURCE_ARG | MEM_ARG );
  }
  this->add( logVerb,         COMMAND_ARG | RESOURCE_ARG );
  this->add( logXml,          COMMAND_ARG | RESOURCE_ARG );
  this->add( help,            HELP_ARG );
  this->add( version,         VERSION_ARG );
  this->add( printRc,         PRINTRC_ARG );
  this->add( rcFile,          RCFILE_ARG );

  if ( vers != NULL )
    this->setVersion( vers );
  if ( pref != NULL )
    this->setEnvPrefix( pref );
  if ( out != NULL )
    this->setOutputStream( out );
  if ( argv0 != NULL ) {
    if ( (ptr = ::strrchr( argv0, '/' )) == NULL )
      if ( (ptr = ::strrchr( argv0, '\\' )) == NULL )
        ptr = argv0 - 1;
    ptr++;
    for ( i = 0; i < sizeof( rcFileName ) - 1 && *ptr != '\0' && *ptr != '.'; )
      rcFileName[ i++ ] = *ptr++;
    for ( ptr = ".ini"; i < sizeof( rcFileName ) - 1 && *ptr != '\0'; )
      rcFileName[ i++ ] = *ptr++;
    rcFileName[ i ] = '\0';
  }
}


Error
ArgsErr::getErr( unsigned int status )
{
  static const char     mod[] = "Args";
  static const ErrorRec err[] = {
  /*  0 */{ UNKNOWN_ARG,      "Arg undefined", mod },
  /*  1 */{ MISSING_ARG,      "Missing argument", mod },
  /*  2 */{ ARG_UNMATCHED,    "Unmatched argument", mod },
  /*  3 */{ BAD_RESOURCE,     "Resource file line not formatted correctly "
                              "(name = val)", mod },
  /*  4 */{ EXPECTING_STRING, "Arg type mismatch, expecting string", mod },
  /*  5 */{ EXPECTING_UINT,   "Arg type mismatch, expecting uint", mod },
  /*  6 */{ EXPECTING_ULLONG, "Arg type mismatch, expecting ulong", mod },
  /*  7 */{ EXPECTING_BOOL,   "Arg type mismatch, expecting bool", mod },
  /*  8 */{ EXPECTING_DOUBLE, "Arg type mismatch, expecting double", mod },
  /*  9 */{ ARG_EXISTS,       "Arg already exists, trying to redefine", mod },
  /* 10 */{ TOO_MANY_ARGS,    "Too many args, can't add another", mod },
  /* 11 */{ NOT_A_LIST,       "Arg not a list, only has one element", mod },
  /* 12 */{ 12,               "Unknown args error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}

