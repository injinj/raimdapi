/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined( _WIN32 ) || defined( _WIN64 )
#include <float.h>
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
#endif

#include "util/str_util.h"

using namespace rai;

static inline const char *
asciiToXmlEntity( char c,  bool aposOk )
{
  static const char *ents[] = {"&quot;",0,0,0,"&amp;","&apos;",0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0,0,0,0,0,"&lt;",0,"&gt;"};
  if ( (byte) c >= '\"' && (byte) c <= '>' ) {
    if ( c == '\'' && aposOk )
      return NULL;
    return ents[ (byte) c - (byte) '\"' ];
  }
  return NULL;
}


const char *
StrUtil::asciiToXmlEntity( char c,  bool aposOk )
{
  return ::asciiToXmlEntity( c, aposOk );
}


unsigned int
StrUtil::escapeXmlStringLen( const char *s,  bool aposOk )
{
  unsigned int i,
               j;
  const char * ent;

  for ( i = 0, j = 0; s[ i ] != '\0'; i++ ) {
    if ( (ent = asciiToXmlEntity( s[ i ], aposOk )) != NULL )
      j += ::strlen( ent );
    else
      j++;
  }
  return j;
}


char *
StrUtil::escapeXmlStringCopy( char *buf,  const char *s,  bool aposOk )
{
  unsigned int i,
               j;
  const char * ent;

  for ( i = 0, j = 0; s[ i ] != '\0'; i++ ) {
    if ( (ent = asciiToXmlEntity( s[ i ], aposOk )) != NULL ) {
      while ( *ent != '\0' )
        buf[ j++ ] = *ent++;
    }
    else
      buf[ j++ ] = s[ i ];
  }
  buf[ j ] = '\0';

  return buf;
}


char *
StrUtil::escapeXmlString( char *buf,  unsigned int bufLen,  const char *s,
                          bool aposOk )
{
  unsigned int i,
               j;
  const char * ent;

  for ( i = 0, j = 0; j < bufLen - 1 && s[ i ] != '\0'; i++ ) {
    if ( (ent = asciiToXmlEntity( s[ i ], aposOk )) != NULL ) {
      while ( j < bufLen - 1 && *ent != '\0' )
        buf[ j++ ] = *ent++;
    }
    else
      buf[ j++ ] = s[ i ];
  }
  buf[ j ] = '\0';

  return buf;
}


char *
StrUtil::escapeXmlString2( char *buf,  unsigned int bufLen,  const char *s,
                           unsigned int sLen,  bool aposOk )
{
  unsigned int i,
               j;
  const char * ent;

  for ( i = 0, j = 0; j < bufLen - 1 && i < sLen; i++ ) {
    if ( (ent = asciiToXmlEntity( s[ i ], aposOk )) != NULL ) {
      while ( j < bufLen - 1 && *ent != '\0' )
        buf[ j++ ] = *ent++;
    }
    else
      buf[ j++ ] = s[ i ];
  }
  buf[ j ] = '\0';

  return buf;
}

static const char hexChars[] = "0123456789abcdef";

unsigned int
StrUtil::escapeXmlBuf( char *buf,  const char *s,  unsigned int len,
                       bool aposOk )
{
  unsigned int i,
               j;
  const char * ent;

  for ( i = 0, j = 0; i < len; i++ ) {
    if ( (byte) s[ i ] < 128 ) {
      if ( (ent = asciiToXmlEntity( s[ i ], aposOk )) != NULL ) {
        while ( *ent != '\0' )
          buf[ j++ ] = *ent++;
      }
      else if ( s[ i ] < ' ' && s[ i ] != '\n' && s[ i ] != '\r' &&
                s[ i ] != '\t' ) {
        buf[ j++ ] = '&';
        buf[ j++ ] = '#';
        buf[ j++ ] = 'x';
        buf[ j++ ] = hexChars[ ( s[ i ] >> 4 ) & 0xf ];
        buf[ j++ ] = hexChars[ s[ i ] & 0xf ];
        buf[ j++ ] = ';';
      }
      else {
        buf[ j++ ] = s[ i ];
      }
    }
    else {
      buf[ j++ ] = '&';
      buf[ j++ ] = '#';
      buf[ j++ ] = 'x';
      buf[ j++ ] = hexChars[ ( s[ i ] >> 4 ) & 0xf ];
      buf[ j++ ] = hexChars[ s[ i ] & 0xf ];
      buf[ j++ ] = ';';
    }
  }
  return j;
}


unsigned int
StrUtil::stripXmlEntities( char *buf,  unsigned int len,  bool plusIsSpace,
                           bool skipHexCvt )
{
  char * s,
       * t,
         c;
  byte   hi,
         lo;

  for ( s = buf, t = buf; s < &buf[ len ]; *t++ = c ) {
    switch ( s[ 0 ] ) {
      case '%':
        /* check if the two chars following % are hex chars */
        if ( ! skipHexCvt && &s[ 3 ] <= &buf[ len ] && isxdigit( s[ 1 ] ) &&
                                                       isxdigit( s[ 2 ] ) ) {
          hi = (byte) s[ 1 ];
          lo = (byte) s[ 2 ];

          hi = (byte) ( isdigit( s[ 1 ] ) ? s[ 1 ] - '0' :
                 ( s[ 1 ] >= 'A' ? s[ 1 ] - 'A' : s[ 1 ] - 'a' ) + 10 );
          lo = (byte) ( isdigit( s[ 2 ] ) ? s[ 2 ] - '0' :
                 ( s[ 2 ] >= 'A' ? s[ 2 ] - 'A' : s[ 2 ] - 'a' ) + 10 );

          c = (char) (byte) ( ( hi << 4 ) | lo );
          s = &s[ 3 ];
        }
        else {
          c = '%'; s++;
        }
        break;

      case '&':
        switch ( &buf[ len ] - s ) {
          default: /* &apos; &quot; */
            if ( s[ 5 ] == ';' ) {
              if ( tolower( s[ 1 ] ) == 'a' && tolower( s[ 2 ] ) == 'p' &&
                   tolower( s[ 3 ] ) == 'o' && tolower( s[ 4 ] ) == 's' ) {
                c = '\''; s = &s[ 6 ];
                break;
              }
              if ( tolower( s[ 1 ] ) == 'q' && tolower( s[ 2 ] ) == 'u' &&
                   tolower( s[ 3 ] ) == 'o' && tolower( s[ 4 ] ) == 't' ) {
                c = '"'; s = &s[ 6 ];
                break;
              }
            }
          /* FALLTHRU */
          case 5: /* &amp; */
            if ( s[ 4 ] == ';' &&
                 tolower( s[ 1 ] ) == 'a' && tolower( s[ 2 ] ) == 'm' &&
                 tolower( s[ 3 ] ) == 'p' ) {
              c = '&'; s = &s[ 5 ];
              break;
            }
          /* FALLTHRU */
          case 4: /* &lt; &gt; */
            if ( s[ 3 ] == ';' ) {
              if ( tolower( s[ 1 ] ) == 'l' && tolower( s[ 2 ] ) == 't' ) {
                c = '<'; s = &s[ 4 ];
                break;
              }
              if ( tolower( s[ 1 ] ) == 'g' && tolower( s[ 2 ] ) == 't' ) {
                c = '>'; s = &s[ 4 ];
                break;
              }
            }
          /* FALLTHRU */
          case 3: case 2: case 1:
            c = '&'; s++;
            break;
        }
        break;

      case '+':
        c = ( plusIsSpace ) ? ' ' : '+';
        s++;
        break;
      default:
        c = *s++;
        break;
    }
  }
  *t = '\0';

  return t - buf;
}


char *
StrUtil::escapeJsonString( char *buf,  unsigned int bufLen,  const char *s )
{
  unsigned int i, j = 0;
  for ( i = 0; ; i++ ) {
    if ( j + 2 >= bufLen ) {
      if ( s[ i ] >= ' ' && s[ i ] <= '~' )
        buf[ j++ ] = s[ i ];
      buf[ j ] = '\0';
      return buf;
    }
    switch ( s[ i ] ) {
      case '\b': buf[ j++ ] = '\\'; buf[ j++ ] = 'b'; break;
      case '\f': buf[ j++ ] = '\\'; buf[ j++ ] = 'f'; break;
      case '\n': buf[ j++ ] = '\\'; buf[ j++ ] = 'n'; break;
      case '\r': buf[ j++ ] = '\\'; buf[ j++ ] = 'r'; break;
      case '\t': buf[ j++ ] = '\\'; buf[ j++ ] = 't'; break;
      case '\"': buf[ j++ ] = '\\'; buf[ j++ ] = '\"'; break;
      case '\\': buf[ j++ ] = '\\'; buf[ j++ ] = '\\'; break;
      case 0:
        buf[ j ] = '\0';
        return buf;
        /*buf[ j++ ] = '\\'; buf[ j++ ] = '0'; break;*/
      default:
        if ( s[ i ] >= ' ' && s[ i ] <= '~' )
          buf[ j++ ] = s[ i ];
        else {
          if ( j + 7 >= bufLen ) {
            buf[ j ] = '\0';
            return buf;
          }
          buf[ j++ ] = '\\';
          buf[ j++ ] = 'u';
          buf[ j++ ] = '0';
          buf[ j++ ] = '0';
          buf[ j++ ] = hexChars[ ( s[ i ] >> 4 ) & 0xfU ];
          buf[ j++ ] = hexChars[ s[ i ] & 0xfU ];
        }
        break;
    }
  }
}


char *
StrUtil::stripNewline( char *line )
{
  unsigned int len = ::strlen( line );
  return StrUtil::stripNewline( line, &len );
}


char *
StrUtil::stripNewline( char *line,  unsigned int *lineLen )
{
  if ( *lineLen > 1 ) {
    if ( line[ *lineLen - 1 ] == '\n' ) {
      line[ --(*lineLen) ] = '\0';
      if ( line[ *lineLen - 1 ] == '\r' )
        line[ --(*lineLen) ] = '\0';
    }
  }
  else if ( *lineLen > 0 && line[ *lineLen - 1 ] == '\n' ) {
    line[ --(*lineLen) ] = '\0';
  }
  return line;
}


char *
StrUtil::trimWhitespace( char *line,  unsigned int *lineLen )
{
  unsigned int i;

  while ( *lineLen > 0 && ( isspace( line[ *lineLen - 1 ] ) ||
                            iscntrl( line[ *lineLen - 1 ] ) ) ) {
      line[ --(*lineLen) ] = '\0';
  }

  for ( i = 0; i < *lineLen; i++ ) {
    if ( ! isspace( line[ i ] ) && ! iscntrl( line[ i ] ) )
      break;
  }

  if ( i > 0 ) {
    ::memmove( line, &line[ i ], *lineLen - i + 1 );
    *lineLen -= i;
  }
  return line;
}


template<class ITYPE>
static void
parse_ITYPE( const char *inString,  ITYPE *iVal,  const char **end,
             unsigned int base )
{
  const char * start,
             * units;
  unsigned int len;

  if ( inString == NULL ) {
    if ( end != NULL )
      *end = NULL;
    throw StrUtilErr::getErr( StrUtilErr::IS_NULL );
  }

  start = inString;

  if ( ( base & U_MASK ) == U_NUMBER || ( base & U_MASK ) == U_HEX ) {
    if ( *inString == '0' &&
          ( inString[ 1 ] == 'x' || inString[ 1 ] == 'X' ) ) {
      base = ( base & ~U_MASK ) | U_HEX;
      inString += 2;
    }
  }
  if ( ( base & U_MASK ) == U_NUMBER || ( base & U_MASK ) == U_OCTAL ) {
    if ( *inString == '0' &&
              ( inString[ 1 ] >= '0' && inString[ 1 ] <= '7' ) ) {
      base = ( base & ~U_MASK ) | U_OCTAL;
      inString++;
    }
  }

  *iVal = 0;
  if ( ( base & U_MASK ) == U_HEX ) {
    while ( isxdigit( *inString ) ) {
      *iVal = *iVal * (ITYPE) 16 +
                      (ITYPE) ( *inString <= '9' ? *inString - '0' :
                              ( *inString <= 'F' ? *inString - 'A' :
                                                   *inString - 'a' ) + 10 );
      inString++;
    }

    if ( start == inString ) {
      if ( ( base & ~U_MASK ) == 0 || *inString == '\0' ) {
        if ( end != NULL )
          goto no_digits;
        throw StrUtilErr::getErr( StrUtilErr::NOT_HEX );
      }
      *iVal = 1; /* just have units, no numbers */
    }
  }
  else if ( ( base & U_MASK ) == U_OCTAL ) {
    while ( *inString >= '0' && *inString <= '7' )
      *iVal = *iVal * (ITYPE) 8 + (ITYPE) ( *inString++ - '0' );

    if ( start == inString ) {
      if ( ( base & ~U_MASK ) == 0 || *inString == '\0' ) {
        if ( end != NULL )
          goto no_digits;
        throw StrUtilErr::getErr( StrUtilErr::NOT_OCTAL );
      }
      *iVal = 1;
    }
  }
  else {
    while ( *inString >= '0' && *inString <= '9' )
      *iVal = *iVal * (ITYPE) 10 + (ITYPE) ( *inString++ - '0' );

    if ( start == inString ) {
      if ( ( base & ~U_MASK ) == 0 || *inString == '\0' ) {
        if ( end != NULL )
          goto no_digits;
        throw StrUtilErr::getErr( StrUtilErr::NOT_INTEGER );
      }
      *iVal = 1;
    }
  }

  if ( ( base & ( U_SECONDS | U_MILLISECS | U_NANOSECS |
                  U_MEMORY | U_BITS ) ) != 0 ) {
    for ( units = inString; isspace( *units ); units++ )
      ;
    for ( len = 0; units[ len ] != '\0' && ! isspace( units[ len ] ); len++ )
      ;
    if ( len > 2 && ( units[ len - 1 ] == 's' || units[ len - 1 ] == 'S' ) )
      len--;
      
    if ( len > 0 ) {
      if ( ( base & ( U_SECONDS | U_MILLISECS | U_NANOSECS ) ) != 0 ) {
        if ( ( len == 2 && StrUtil::strncasecmp( units, "ms", 2 ) == 0 ) ||
             ( len == 4 && StrUtil::strncasecmp( units, "msec", 4 ) == 0 ) ||
             ( len == 8 && StrUtil::strncasecmp( units, "millisec", 8 ) == 0 ) ||
             ( len == 11 && StrUtil::strncasecmp( units, "millisecond", 11 ) == 0 ) ) {
          if ( ( base & U_SECONDS ) != 0 )
            *iVal /= (ITYPE) MSECS_IN_SEC;
          else if ( ( base & U_NANOSECS ) != 0 )
            *iVal *= (ITYPE) NSECS_IN_MSEC;
        }
        else if ( ( len == 2 && StrUtil::strncasecmp( units, "wk", 2 ) == 0 ) ||
		  ( len == 4 && StrUtil::strncasecmp( units, "week", 4 ) == 0 ) ) {
          if ( ( base & U_SECONDS ) != 0 )
            *iVal *= (ITYPE) SECS_IN_WEEK;
          else if ( ( base & U_NANOSECS ) != 0 )
            *iVal *= (ITYPE) NSECS_IN_WEEK;
          else
            *iVal *= (ITYPE) MSECS_IN_WEEK;
        }
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "d", 1 ) == 0 ) ||
                  ( len == 3 && StrUtil::strncasecmp( units, "day", 3 ) == 0 ) ) {
          if ( ( base & U_SECONDS ) != 0 )
            *iVal *= (ITYPE) SECS_IN_DAY;
          else if ( ( base & U_NANOSECS ) != 0 )
            *iVal *= (ITYPE) NSECS_IN_DAY;
          else
            *iVal *= (ITYPE) MSECS_IN_DAY;
        }
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "h", 1 ) == 0 ) ||
                  ( len == 2 && StrUtil::strncasecmp( units, "hr", 2 ) == 0 ) ||
                  ( len == 4 && StrUtil::strncasecmp( units, "hour", 4 ) == 0 ) ) {
          if ( ( base & U_SECONDS ) != 0 )
            *iVal *= (ITYPE) SECS_IN_HOUR;
          else if ( ( base & U_NANOSECS ) != 0 )
            *iVal *= (ITYPE) NSECS_IN_HOUR;
          else
            *iVal *= (ITYPE) MSECS_IN_HOUR;
        }
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "m", 1 ) == 0 ) ||
                  ( len == 3 && StrUtil::strncasecmp( units, "min", 3 ) == 0 ) ||
                  ( len == 6 && StrUtil::strncasecmp( units, "minute", 6 ) == 0 ) ) {
          if ( ( base & U_SECONDS ) != 0 )
            *iVal *= (ITYPE) SECS_IN_MINUTE;
          else if ( ( base & U_NANOSECS ) != 0 )
            *iVal *= (ITYPE) NSECS_IN_MINUTE;
          else
            *iVal *= (ITYPE) MSECS_IN_MINUTE;
        }
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "s", 1 ) == 0 ) ||
                  ( len == 3 && StrUtil::strncasecmp( units, "sec", 3 ) == 0 ) ||
		  ( len == 6 && StrUtil::strncasecmp( units, "second", 6 ) == 0 ) ) {
          if ( ( base & U_SECONDS ) == 0 )
            *iVal = (ITYPE) ( *iVal * MSECS_IN_SEC );
          else if ( ( base & U_NANOSECS ) != 0 )
            *iVal = (ITYPE) ( *iVal * NSECS_IN_SEC );
        }
        else if ( ( len == 2 && StrUtil::strncasecmp( units, "ns", 2 ) == 0 ) ||
                  ( len == 4 && StrUtil::strncasecmp( units, "nsec", 4 ) == 0 ) ||
                  ( len == 7 && StrUtil::strncasecmp( units, "nanosec", 7 ) == 0 ) ||
                  ( len == 10 && StrUtil::strncasecmp( units, "nanosecond", 10 ) == 0 ) ) {
          if ( ( base & U_SECONDS ) != 0 )
            *iVal = (ITYPE) ( (ullong) *iVal / NSECS_IN_SEC );
          else if ( ( base & U_MILLISECS ) != 0 )
            *iVal = (ITYPE) ( (ullong) *iVal / NSECS_IN_MSEC );
        }
        else if ( ( len == 2 && StrUtil::strncasecmp( units, "us", 2 ) == 0 ) ||
                  ( len == 4 && StrUtil::strncasecmp( units, "usec", 4 ) == 0 ) ||
                  ( len == 8 && StrUtil::strncasecmp( units, "microsec", 8 ) == 0 ) ||
                  ( len == 11 && StrUtil::strncasecmp( units, "microsecond", 11 ) == 0 ) ) {
          if ( ( base & U_SECONDS ) != 0 )
            *iVal = (ITYPE) ( (ullong) *iVal / USECS_IN_SEC );
          else if ( ( base & U_MILLISECS ) != 0 )
            *iVal = (ITYPE) ( (ullong) *iVal / USECS_IN_MSEC );
          else if ( ( base & U_NANOSECS ) != 0 )
            *iVal = (ITYPE) ( (ullong) *iVal * NSECS_IN_USEC );
        }
        else
          throw StrUtilErr::getErr( StrUtilErr::NOT_TIME );
      }
      else if ( ( base & U_MEMORY ) != 0 ) {
        if ( ( len == 1 && StrUtil::strncasecmp( units, "b", 1 ) == 0 ) ||
             ( len == 4 && StrUtil::strncasecmp( units, "byte", 4 ) == 0 ) )
          ;
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "k", 1 ) == 0 ) ||
                  ( len == 2 && StrUtil::strncasecmp( units, "kb", 2 ) == 0 ) ||
                  ( len == 5 && StrUtil::strncasecmp( units, "kbyte", 5 ) == 0 ) ||
                  ( len == 8 && StrUtil::strncasecmp( units, "kilobyte", 8 ) == 0 ) )
          *iVal *= (ITYPE) BYTES_IN_KILO;
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "m", 1 ) == 0 ) ||
                  ( len == 2 && StrUtil::strncasecmp( units, "mb", 2 ) == 0 ) ||
                  ( len == 5 && StrUtil::strncasecmp( units, "mbyte", 5 ) == 0 )||
                  ( len == 8 && StrUtil::strncasecmp( units, "megabyte", 8 ) == 0 ) )
          *iVal *= (ITYPE) BYTES_IN_MEGA;
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "g", 1 ) == 0 ) ||
                  ( len == 2 && StrUtil::strncasecmp( units, "gb", 2 ) == 0 ) ||
                  ( len == 5 && StrUtil::strncasecmp( units, "gbyte", 5 ) == 0 ) ||
                  ( len == 8 && StrUtil::strncasecmp( units, "gigabyte", 8 ) == 0 ) )
          *iVal *= (ITYPE) BYTES_IN_GIGA;
        else if ( len == 3 && StrUtil::strncasecmp( units, "bit", 3 ) == 0 )
          *iVal /= (ITYPE) BITS_IN_BYTE;
        else if ( ( len == 4 && StrUtil::strncasecmp( units, "kbit", 4 ) == 0 ) ||
                  ( len == 7 && StrUtil::strncasecmp( units, "kilobit", 7 ) == 0 ) )
          *iVal *= (ITYPE) ( BITS_IN_KILO / BITS_IN_BYTE );
        else if ( ( len == 4 && StrUtil::strncasecmp( units, "mbit", 4 ) == 0 )||
                  ( len == 7 && StrUtil::strncasecmp( units, "megabit", 7 ) == 0 ) )
          *iVal *= (ITYPE) ( BITS_IN_MEGA / BITS_IN_BYTE );
        else if ( ( len == 4 && StrUtil::strncasecmp( units, "gbit", 4 ) == 0 ) ||
                  ( len == 7 && StrUtil::strncasecmp( units, "gigabit", 7 ) == 0 ) )
          *iVal *= (ITYPE) ( BITS_IN_GIGA / BITS_IN_BYTE );
        else
          throw StrUtilErr::getErr( StrUtilErr::NOT_MEMORY );
      }
      else { /* base & U_BITS */
        if ( ( len == 1 && StrUtil::strncasecmp( units, "b", 1 ) == 0 ) ||
             ( len == 3 && StrUtil::strncasecmp( units, "bit", 3 ) == 0 ) )
          ;
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "k", 1 ) == 0 ) ||
                  ( len == 2 && StrUtil::strncasecmp( units, "kb", 2 ) == 0 ) ||
                  ( len == 4 && StrUtil::strncasecmp( units, "kbit", 4 ) == 0 ) ||
                  ( len == 7 && StrUtil::strncasecmp( units, "kilobit", 7 ) == 0 ) )
          *iVal *= (ITYPE) BITS_IN_KILO;
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "m", 1 ) == 0 ) ||
                  ( len == 2 && StrUtil::strncasecmp( units, "mb", 2 ) == 0 ) ||
                  ( len == 4 && StrUtil::strncasecmp( units, "mbit", 4 ) == 0 )||
                  ( len == 7 && StrUtil::strncasecmp( units, "megabit", 7 ) == 0 ) )
          *iVal *= (ITYPE) BITS_IN_MEGA;
        else if ( ( len == 1 && StrUtil::strncasecmp( units, "g", 1 ) == 0 ) ||
                  ( len == 2 && StrUtil::strncasecmp( units, "gb", 2 ) == 0 ) ||
                  ( len == 4 && StrUtil::strncasecmp( units, "gbit", 4 ) == 0 ) ||
                  ( len == 7 && StrUtil::strncasecmp( units, "gigabit", 7 ) == 0 ) )
          *iVal *= (ITYPE) BITS_IN_GIGA;
        else if ( len == 4 && StrUtil::strncasecmp( units, "byte", 4 ) == 0 )
          *iVal *= (ITYPE) BITS_IN_BYTE;
        else if ( ( len == 5 && StrUtil::strncasecmp( units, "kbyte", 5 ) == 0 ) ||
                  ( len == 8 && StrUtil::strncasecmp( units, "kilobyte", 8 ) == 0 ) )
          *iVal *= (ITYPE) BYTES_IN_KILO * (ITYPE) BITS_IN_BYTE;
        else if ( ( len == 5 && StrUtil::strncasecmp( units, "mbyte", 5 ) == 0 )||
                  ( len == 8 && StrUtil::strncasecmp( units, "megabyte", 8 ) == 0 ) )
          *iVal *= (ITYPE) BYTES_IN_MEGA * (ITYPE) BITS_IN_BYTE;
        else if ( ( len == 5 && StrUtil::strncasecmp( units, "gbyte", 5 ) == 0 ) ||
                  ( len == 8 && StrUtil::strncasecmp( units, "gigabyte", 8 ) == 0 ) )
          *iVal *= (ITYPE) BYTES_IN_GIGA * (ITYPE) BITS_IN_BYTE;
        else
          throw StrUtilErr::getErr( StrUtilErr::NOT_MEMORY );
      }

      inString = &units[ len ];
    }
  }
no_digits:;
  if ( end != NULL )
    *end = inString;
}


bool
StrUtil::isValidInt( const char *inString )
{
  if ( inString == NULL )
    return false;
  if ( *inString == '-' )
    inString++;
  if ( *inString < '0' || *inString > '9' )
    return false;
  return true;
}


void
StrUtil::parseInt( const char *inString,  short *iVal,
                   const char **end,  unsigned int base )
{
  bool           isNegative;
  unsigned short uVal;

  if ( inString == NULL ) {
    if ( end != NULL )
      *end = NULL;
    throw StrUtilErr::getErr( StrUtilErr::IS_NULL );
  }

  if ( *inString == '-' ) {
    isNegative = true;
    inString++;
  }
  else {
    if ( *inString == '+' )
      inString++;
    isNegative = false;
  }

  parse_ITYPE( inString, &uVal, end, base );

  if ( isNegative )
    *iVal = -(short) uVal;
  else
    *iVal = (short) uVal;
}


void
StrUtil::parseInt( const char *inString,  int *iVal,
                   const char **end,  unsigned int base )
{
  bool         isNegative;
  unsigned int uVal;

  if ( inString == NULL ) {
    if ( end != NULL )
      *end = NULL;
    throw StrUtilErr::getErr( StrUtilErr::IS_NULL );
  }

  if ( *inString == '-' ) {
    isNegative = true;
    inString++;
  }
  else {
    if ( *inString == '+' )
      inString++;
    isNegative = false;
  }

  parse_ITYPE( inString, &uVal, end, base );

  if ( isNegative )
    *iVal = -(int) uVal;
  else
    *iVal = (int) uVal;
}


void
StrUtil::parseInt( const char *inString,  long *iVal,
                   const char **end,  unsigned int base )
{
  bool          isNegative;
  unsigned long uVal;

  if ( inString == NULL ) {
    if ( end != NULL )
      *end = NULL;
    throw StrUtilErr::getErr( StrUtilErr::IS_NULL );
  }

  if ( *inString == '-' ) {
    isNegative = true;
    inString++;
  }
  else {
    if ( *inString == '+' )
      inString++;
    isNegative = false;
  }

  parse_ITYPE( inString, &uVal, end, base );

  if ( isNegative )
    *iVal = -(long) uVal;
  else
    *iVal = (long) uVal;
}


void
StrUtil::parseInt( const char *inString,  llong *iVal,
                   const char **end,  unsigned int base )
{
  bool   isNegative;
  ullong uVal;

  if ( inString == NULL ) {
    if ( end != NULL )
      *end = NULL;
    throw StrUtilErr::getErr( StrUtilErr::IS_NULL );
  }

  if ( *inString == '-' ) {
    isNegative = true;
    inString++;
  }
  else {
    if ( *inString == '+' )
      inString++;
    isNegative = false;
  }

  parse_ITYPE( inString, &uVal, end, base );

  if ( isNegative )
    *iVal = -(llong) uVal;
  else
    *iVal = (llong) uVal;
}


void
StrUtil::parseInt( const char *inString,  unsigned int *iVal,
                   const char **end,  unsigned int base )
{
  parse_ITYPE( inString, iVal, end, base );
}


void
StrUtil::parseInt( const char *inString,  unsigned long *iVal,
                   const char **end,  unsigned int base )
{
  parse_ITYPE( inString, iVal, end, base );
}


void
StrUtil::parseInt( const char *inString,  ullong *iVal,
                   const char **end,  unsigned int base )
{
  parse_ITYPE( inString, iVal, end, base );
}


void
StrUtil::parseInt( const char *inString,  unsigned short *iVal,
                   const char **end,  unsigned int base )
{
  parse_ITYPE( inString, iVal, end, base );
}


void
StrUtil::parseInt( const char *inString,  byte *iVal,
                   const char **end,  unsigned int base )
{
  parse_ITYPE( inString, iVal, end, base );
}


bool
StrUtil::parseBoolean( const char *inString )
{
  static const char *true_vals[]  = { "true",  "t", "yes", "1", "on", "+" };
  static const char *false_vals[] = { "false", "f", "no",  "0", "off", "-" };
  unsigned int i;

  if ( inString == NULL )
    throw StrUtilErr::getErr( StrUtilErr::IS_NULL );

  for ( i = 0; i < sizeof( true_vals ) / sizeof( true_vals[ 0 ] ); i++ )
    if ( StrUtil::strcasecmp( inString, true_vals[ i ] ) == 0 )
      return true;
  for ( i = 0; i < sizeof( false_vals ) / sizeof( false_vals[ 0 ] ); i++ )
    if ( StrUtil::strcasecmp( inString, false_vals[ i ] ) == 0 )
      return false;

  throw StrUtilErr::getErr( StrUtilErr::NOT_BOOLEAN );
}


template<class ITYPE>
static char *
ITYPE_toString( ITYPE i,  char *outBuf,  unsigned int bufLen,
                unsigned int base,  bool abrev,  char **end )
{
  char         buf[ 64 ];
  const char * units;
  char         nibble;
  unsigned int off,
               len;

  off = sizeof( buf ) - 1;
  buf[ sizeof( buf ) - 1 ] = '\0';

  if ( ( base & ( U_SECONDS | U_MILLISECS | U_NANOSECS | U_MEMORY |
                  U_BITS ) ) != 0 ) {
    if ( ( base & U_SECONDS ) != 0 ) {
      if ( i == 0 ) {
        units = abrev ? "s" : "seconds";
      }
      else if ( i % (ITYPE) SECS_IN_WEEK == 0 ) {
        i /= (ITYPE) SECS_IN_WEEK;
        units = abrev ? "wk" : "weeks";
      }
      else if ( i % (ITYPE) SECS_IN_DAY == 0 ) {
        i /= (ITYPE) SECS_IN_DAY;
        units = abrev ? "d" : "days";
      }
      else if ( i % (ITYPE) SECS_IN_HOUR == 0 ) {
        i /= (ITYPE) SECS_IN_HOUR;
        units = abrev ? "h" : "hours";
      }
      else if ( i % (ITYPE) SECS_IN_MINUTE == 0 ) {
        i /= (ITYPE) SECS_IN_MINUTE;
        units = abrev ? "m" : "minutes";
      }
      else {
        units = abrev ? "s" : "seconds";
      }
    }
    else if ( ( base & U_MILLISECS ) != 0 ) {
      if ( i == 0 ) {
        units = abrev ? "ms" : "millisecs";
      }
      else if ( i % (ITYPE) MSECS_IN_WEEK == 0 ) {
        i /= (ITYPE) MSECS_IN_WEEK;
        units = abrev ? "wk" : "weeks";
      }
      else if ( i % (ITYPE) MSECS_IN_DAY == 0 ) {
        i /= (ITYPE) MSECS_IN_DAY;
        units = abrev ? "d" : "days";
      }
      else if ( i % (ITYPE) MSECS_IN_HOUR == 0 ) {
        i /= (ITYPE) MSECS_IN_HOUR;
        units = abrev ? "h" : "hours";
      }
      else if ( i % (ITYPE) MSECS_IN_MINUTE == 0 ) {
        i /= (ITYPE) MSECS_IN_MINUTE;
        units = abrev ? "m" : "minutes";
      }
      else if ( i % (ITYPE) MSECS_IN_SEC == 0 ) {
        i /= (ITYPE) MSECS_IN_SEC;
        units = abrev ? "s" : "seconds";
      }
      else {
        units = abrev ? "ms" : "millisecs";
      }
    }
    else if ( ( base & U_NANOSECS ) != 0 ) {
      if ( i == 0 ) {
        units = abrev ? "ns" : "nanosecs";
      }
      else if ( i % (ITYPE) NSECS_IN_DAY == 0 ) {
        i /= (ITYPE) NSECS_IN_DAY;
        units = abrev ? "d" : "days";
      }
      else if ( i % (ITYPE) NSECS_IN_HOUR == 0 ) {
        i /= (ITYPE) NSECS_IN_HOUR;
        units = abrev ? "h" : "hours";
      }
      else if ( i % (ITYPE) NSECS_IN_MINUTE == 0 ) {
        i /= (ITYPE) NSECS_IN_MINUTE;
        units = abrev ? "m" : "minutes";
      }
      else if ( i % (ITYPE) NSECS_IN_SEC == 0 ) {
        i /= (ITYPE) NSECS_IN_SEC;
        units = abrev ? "s" : "seconds";
      }
      else if ( i % (ITYPE) NSECS_IN_MSEC == 0 ) {
        i /= (ITYPE) NSECS_IN_MSEC;
        units = abrev ? "ms" : "millisecs";
      }
      else if ( i % (ITYPE) NSECS_IN_USEC == 0 ) {
        i /= (ITYPE) NSECS_IN_USEC;
        units = abrev ? "us" : "microsecs";
      }
      else {
        units = abrev ? "ns" : "nanosecs";
      }
    }
    else if ( ( base & U_MEMORY ) != 0 ) {
      if ( i == 0 ) {
        units = abrev ? "b" : "bytes";
      }
      else if ( i % (ITYPE) BYTES_IN_GIGA  == 0 ) {
        i /= (ITYPE) BYTES_IN_GIGA;
        units = abrev ? "gb" : "gigabytes";
      }
      else if ( i % (ITYPE) BYTES_IN_MEGA == 0 ) {
        i /= (ITYPE) BYTES_IN_MEGA;
        units = abrev ? "mb" : "megabytes";
      }
      else if ( i % (ITYPE) BYTES_IN_KILO == 0 ) {
        i /= (ITYPE) BYTES_IN_KILO;
        units = abrev ? "kb" : "kilobytes";
      }
      else {
        units = abrev ? "b" : "bytes";
      }
    }
    else { /* base & U_BITS */
      if ( i == 0 ) {
        units = abrev ? "bit" : "bits";
      }
      else if ( i % (ITYPE) BITS_IN_GIGA  == 0 ) {
        i /= (ITYPE) BITS_IN_GIGA;
        units = abrev ? "gbit" : "gigabits";
      }
      else if ( i % (ITYPE) BITS_IN_MEGA == 0 ) {
        i /= (ITYPE) BITS_IN_MEGA;
        units = abrev ? "mbit" : "megabits";
      }
      else if ( i % (ITYPE) BITS_IN_KILO == 0 ) {
        i /= (ITYPE) BITS_IN_KILO;
        units = abrev ? "kbit" : "kilobits";
      }
      else {
        units = abrev ? "bit" : "bits";
      }
    }
    len = ::strlen( units );
    if ( ! abrev && i == (ITYPE) 1 )
      len -= 1;
    ::memcpy( &buf[ off -= len ], units, len );
    if ( ! abrev ) {
      buf[ --off ] = ' ';
    }
  }

  if ( ( base & U_MASK ) == U_HEX ) {
    do {
      nibble       = (char) ( i & 15 );
      buf[ --off ] = ( nibble < 10 ? nibble + '0' : nibble - 10 + 'a' );
      i >>= 4;
    } while ( i != 0 );
  }
  else if ( ( base & U_MASK ) == U_OCTAL ) {
    do {
      buf[ --off ] = (char) ( i & 7 ) + '0';
      i >>= 3;
    } while ( i != 0 );
  }
  else {
    do {
      buf[ --off ] = (char) ( i % 10 ) + '0';
      i /= 10;
    } while ( i != 0 );
  }

  if ( sizeof( buf ) - off > bufLen ) {
    ::memcpy( outBuf, &buf[ off ], bufLen );
    if ( end != NULL )
      *end = &outBuf[ bufLen ];
    throw StrUtilErr::getErr( StrUtilErr::BUF_OVERFLOW );
  }
  ::memcpy( outBuf, &buf[ off ], sizeof( buf ) - off );
  if ( end != NULL )
    *end = &outBuf[ sizeof( buf ) - 1 - off ];

  return outBuf;
}


char *
StrUtil::intToString( byte i,  char *buf,  unsigned int bufLen,
                      unsigned int base,  bool abrev,
                      char **end )
{
  return StrUtil::intToString( (unsigned int) i, buf, bufLen, base, abrev,
                               end );
}


char *
StrUtil::intToString( char i,  char *buf,  unsigned int bufLen,
                      unsigned int base,  bool abrev,
                      char **end )
{
  char *p = buf;
  if ( i < 0 && bufLen > 0 ) {
    i = -i; *p++ = '-'; bufLen--;
  }
  StrUtil::intToString( (byte) i, p, bufLen, base, abrev, end );
  return buf;
}


char *
StrUtil::intToString( unsigned short i,  char *buf,  unsigned int bufLen,
                      unsigned int base,  bool abrev,
                      char **end )
{
  return StrUtil::intToString( (unsigned int) i, buf, bufLen, base, abrev,
                               end );
}


char *
StrUtil::intToString( short i,  char *buf,  unsigned int bufLen,
                      unsigned int base,  bool abrev,
                      char **end )
{
  char *p = buf;
  if ( i < 0 && bufLen > 0 ) {
    i = -i; *p++ = '-'; bufLen--;
  }
  StrUtil::intToString( (unsigned short) i, p, bufLen, base, abrev, end );
  return buf;
}


char *
StrUtil::intToString( unsigned int i,  char *buf,  unsigned int bufLen,
                      unsigned int base,  bool abrev,
                      char **end )
{
  return ITYPE_toString( i, buf, bufLen, base, abrev, end );
}


char *
StrUtil::intToString( int i,  char *buf,  unsigned int bufLen,
                      unsigned int base,  bool abrev,
                      char **end )
{
  char *p = buf;
  if ( i < 0 && bufLen > 0 ) {
    i = -i; *p++ = '-'; bufLen--;
  }
  StrUtil::intToString( (unsigned int) i, p, bufLen, base, abrev, end );
  return buf;
}


char *
StrUtil::intToString( ullong i,  char *buf,  unsigned int bufLen,
                      unsigned int base,  bool abrev,
                      char **end )
{
  return ITYPE_toString( i, buf, bufLen, base, abrev, end );
}


char *
StrUtil::intToString( llong i,  char *buf,  unsigned int bufLen,
                      unsigned int base,  bool abrev,
                      char **end )
{
  char *p = buf;
  if ( i < 0 && bufLen > 0 ) {
    i = -i; *p++ = '-'; bufLen--;
  }
  StrUtil::intToString( (ullong) i, p, bufLen, base, abrev, end );
  return buf;
}


#if 0
char *
StrUtil::floatToString( double f,  char *buf,  unsigned int bufLen,
                        unsigned int decimalPlaces,  char **end )
{
  char * p,
       * p2;
  double tmp,
         p10,
         integral,
         decimal;

  if ( bufLen == 0 )
    throw StrUtilErr::getErr( StrUtilErr::BUF_OVERFLOW );

  p = buf;
  if ( f < 0 ) {
    f = -f;
    *p++ = '-';
    bufLen--;
  }

  tmp = modf( f, &integral );
  p10 = pow( (double) 10.0, (double) decimalPlaces );
  tmp = ( tmp + 1.0 ) * p10;
  tmp = modf( tmp, &decimal );

  if ( tmp >= 0.5 ) {
    decimal += 1.0;
    if ( decimal >= p10 * 2.0 )
      integral += 1.0;
  }

  StrUtil::intToString( (ullong) integral, p, bufLen, U_DECIMAL, false, &p2 );
  bufLen -= ( p2 - p );

  StrUtil::intToString( (ullong) decimal, p2, bufLen, U_DECIMAL, false, end );
  *p2 = '.';

  return buf;
}
#endif


template<class FTYPE>
static char *
FTYPE_toString( FTYPE f,  char *buf,  unsigned int bufLen,
                unsigned int decimalPlaces,  unsigned int base,
                bool abrev,  char **end )
{
  char         tmpBuf[ 32 ];
  const char * units;
  char       * p,
             * p2,
             * p3,
             * p4;
  unsigned int i,
               off,
               len;
  double       tmp,
               p10,
               integral,
               decimal;

  if ( bufLen == 0 )
    throw StrUtilErr::getErr( StrUtilErr::BUF_OVERFLOW );

  p = buf;
  if ( f < 0 ) {
    f = -f;
    *p++ = '-';
    bufLen--;
  }

  off = bufLen - 1;
  p[ off ] = '\0';

  if ( ( base & ( U_SECONDS | U_MILLISECS | U_NANOSECS |
                  U_MEMORY | U_BITS ) ) != 0 ) {
    if ( ( base & U_SECONDS ) != 0 ) {
      if ( f == 0 ) {
        units = abrev ? "s" : "seconds";
      }
      else if ( f / (FTYPE) SECS_IN_WEEK >= 1.0 ) {
        f /= (FTYPE) SECS_IN_WEEK;
        units = abrev ? "wk" : "weeks";
      }
      else if ( f / (FTYPE) SECS_IN_DAY >= 1.0 ) {
        f /= (FTYPE) SECS_IN_DAY;
        units = abrev ? "d" : "days";
      }
      else if ( f / (FTYPE) SECS_IN_HOUR >= 1.0 ) {
        f /= (FTYPE) SECS_IN_HOUR;
        units = abrev ? "h" : "hours";
      }
      else if ( f / (FTYPE) SECS_IN_MINUTE >= 1.0 ) {
        f /= (FTYPE) SECS_IN_MINUTE;
        units = abrev ? "m" : "minutes";
      }
      else {
        units = abrev ? "s" : "seconds";
      }
    }
    else if ( ( base & U_MILLISECS ) != 0 ) {
      if ( f == 0 ) {
        units = abrev ? "ms" : "millisecs";
      }
      else if ( f / (FTYPE) MSECS_IN_WEEK >= 1.0 ) {
        f /= (FTYPE) MSECS_IN_WEEK;
        units = abrev ? "wk" : "weeks";
      }
      else if ( f / (FTYPE) MSECS_IN_DAY >= 1.0 ) {
        f /= (FTYPE) MSECS_IN_DAY;
        units = abrev ? "d" : "days";
      }
      else if ( f / (FTYPE) MSECS_IN_HOUR >= 1.0 ) {
        f /= (FTYPE) MSECS_IN_HOUR;
        units = abrev ? "h" : "hours";
      }
      else if ( f / (FTYPE) MSECS_IN_MINUTE >= 1.0 ) {
        f /= (FTYPE) MSECS_IN_MINUTE;
        units = abrev ? "m" : "minutes";
      }
      else if ( f / (FTYPE) MSECS_IN_SEC >= 1.0 ) {
        f /= (FTYPE) MSECS_IN_SEC;
        units = abrev ? "s" : "seconds";
      }
      else {
        units = abrev ? "ms" : "millisecs";
      }
    }
    else if ( ( base & U_NANOSECS ) != 0 ) {
      if ( f == 0 ) {
        units = abrev ? "ns" : "nanosecs";
      }
      else if ( f / (FTYPE) NSECS_IN_DAY >= 1.0 ) {
        f /= (FTYPE) NSECS_IN_DAY;
        units = abrev ? "d" : "days";
      }
      else if ( f / (FTYPE) NSECS_IN_HOUR >= 1.0 ) {
        f /= (FTYPE) NSECS_IN_HOUR;
        units = abrev ? "h" : "hours";
      }
      else if ( f / (FTYPE) NSECS_IN_MINUTE >= 1.0 ) {
        f /= (FTYPE) NSECS_IN_MINUTE;
        units = abrev ? "m" : "minutes";
      }
      else if ( f / (FTYPE) NSECS_IN_SEC >= 1.0 ) {
        f /= (FTYPE) NSECS_IN_SEC;
        units = abrev ? "s" : "seconds";
      }
      else if ( f / (FTYPE) NSECS_IN_MSEC >= 1.0 ) {
        f /= (FTYPE) NSECS_IN_MSEC;
        units = abrev ? "ms" : "millisecs";
      }
      else if ( f / (FTYPE) NSECS_IN_USEC >= 1.0 ) {
        f /= (FTYPE) NSECS_IN_USEC;
        units = abrev ? "us" : "microsecs";
      }
      else {
        units = abrev ? "ns" : "nanosecs";
      }
    }
    else if ( ( base & U_MEMORY ) != 0 ) {
      if ( f == 0 ) {
        units = abrev ? "b" : "bytes";
      }
      else if ( f / (FTYPE) BYTES_IN_GIGA >= 1.0 ) {
        f /= (FTYPE) BYTES_IN_GIGA;
        units = abrev ? "gb" : "gigabytes";
      }
      else if ( f / (FTYPE) BYTES_IN_MEGA >= 1.0 ) {
        f /= (FTYPE) BYTES_IN_MEGA;
        units = abrev ? "mb" : "megabytes";
      }
      else if ( f / (FTYPE) BYTES_IN_KILO >= 1.0 ) {
        f /= (FTYPE) BYTES_IN_KILO;
        units = abrev ? "kb" : "kilobytes";
      }
      else {
        units = abrev ? "b" : "bytes";
      }
    }
    else { /* base & U_BITS */
      if ( f == 0 ) {
        units = abrev ? "bit" : "bits";
      }
      else if ( f / (FTYPE) BITS_IN_GIGA >= 1.0 ) {
        f /= (FTYPE) BITS_IN_GIGA;
        units = abrev ? "gbit" : "gigabits";
      }
      else if ( f / (FTYPE) BITS_IN_MEGA >= 1.0 ) {
        f /= (FTYPE) BITS_IN_MEGA;
        units = abrev ? "mbit" : "megabits";
      }
      else if ( f / (FTYPE) BITS_IN_KILO >= 1.0 ) {
        f /= (FTYPE) BITS_IN_KILO;
        units = abrev ? "kbit" : "kilobits";
      }
      else {
        units = abrev ? "bit" : "bits";
      }
    }

    len = ::strlen( units );
    if ( ! abrev && f == (FTYPE) 1.0 )
      len -= 1;
    if ( off < len )
      throw StrUtilErr::getErr( StrUtilErr::BUF_OVERFLOW );
    ::memcpy( &p[ off -= len ], units, len );
    if ( ! abrev ) {
      if ( off == 0 )
        throw StrUtilErr::getErr( StrUtilErr::BUF_OVERFLOW );
      p[ --off ] = ' ';
    }
  }

  if ( isnan( f ) ) {
    units = "NaN";
  copy_chars:;
    if ( off >= 3 ) {
      p[ --off ] = *units++;
      p[ --off ] = *units++;
      p[ --off ] = *units;
      p3 = p;
    }
    else {
      throw StrUtilErr::getErr( StrUtilErr::BUF_OVERFLOW );
    }
  }
  else if ( isinf( f ) ) {
    units = "fnI";
    goto copy_chars;
  }
  else {
    static const double powtab[] = {
      1.0, 10.0, 100.0, 1000.0, 10000.0,
      100000.0, 1000000.0, 10000000.0,
      100000000.0, 1000000000.0, 10000000000.0,
      100000000000.0, 1000000000000.0, 10000000000000.0,
      100000000000000.0
      };
    /* find the integer and the fraction + 1.0 */
    const double fraction = modf( (double) f, &integral );
    unsigned int places = ( decimalPlaces < 255 ? decimalPlaces : 14 );
    ullong integral_ival = (ullong) integral;
    if ( decimalPlaces >= 255 ) {
      for ( ullong ival_places = integral_ival;
            ival_places >= 100 && places > 1; ival_places /= 10 ) {
        places--;
      }
    }
    if ( places > 0 ) {
      if ( places < sizeof( powtab ) / sizeof( powtab[ 0 ] ) )
        p10 = powtab[ places ];
      else
        p10 = pow( (double) 10.0, (double) places );
      /* multiply fraction + 1 * places wanted (.25 + 1.0) * 1000.0 = 1250 */
      tmp = modf( ( fraction + 1.0 ) * p10, &decimal );
      /* round up, if fraction of decimal places >= 0.5 */
      if ( tmp >= 0.5 ) {
        decimal += 1.0;
        if ( decimal >= p10 * 2.0 )
          integral_ival++;
      }
      else if ( decimal >= p10 * 2.0 ) {
        decimal--;
      }
    }
    StrUtil::intToString( integral_ival, p, off, U_DECIMAL, false, &p2 );
    if ( places > 0 ) {
      /* convert the decimal to 1ddddd, the 1 is replaced with a '.' below */
      if ( decimalPlaces >= 255 ) {
        ullong decimal_ival = (ullong) decimal;

        while ( decimal_ival > 1 && decimal_ival % 10 == 0 )
          decimal_ival /= 10;
        /* at least one zero */
        if ( decimal_ival == 1 || decimal_ival == 2 ) {
          tmpBuf[ 1 ] = '0';
          p4 = &tmpBuf[ 2 ];
        }
        else {
          StrUtil::intToString( decimal_ival, tmpBuf, sizeof( tmpBuf ), U_DECIMAL,
                                false, &p4 );
        }
        /* eat the zeros at the end
        while ( p4 > &tmpBuf[ 2 ] && *(p4-1) == '0' )
          --p4; */
        p3 = &p2[ 1 ];
        for ( i = 1; &tmpBuf[ i ] < p4; i++ ) {
          if ( p3 < &p[ off ] )
            *p3++ = tmpBuf[ i ];
        }
      }
      else {
        StrUtil::intToString( (ullong) decimal, p2, off - ( p2 - p ),
                              U_DECIMAL, false, &p3 );
      }
      *p2 = '.';
    }
    else {
      p3 = p2;
    }
  }
  if ( off > 0 ) {
    while ( off < bufLen )
      *p3++ = p[ off++ ];
    p3--; /* null char */
  }
  if ( end != NULL )
    *end = p3;

  return buf;
}


char *
StrUtil::floatToString( double f,  char *buf,  unsigned int bufLen,
                        unsigned int decimalPlaces,  unsigned int base,
                        bool abrev, char **end )
{
  return FTYPE_toString( f,  buf,  bufLen, decimalPlaces,  base,
                         abrev,  end );
}


char *
StrUtil::floatToString( float f,  char *buf,  unsigned int bufLen,
                        unsigned int decimalPlaces,  unsigned int base,
                        bool abrev, char **end )
{
  return FTYPE_toString( f,  buf,  bufLen, decimalPlaces,  base,
                         abrev,  end );
}

struct FTypeExtra {
  unsigned int denom,
               precision;
  bool         novalue;
};

static void
parse_float_precision( const char *inString,  const char *s,  FTypeExtra &ex )
{
  unsigned int i, j, exp = 0;
  bool negexp = false;

  /* looking for '.' and/or exponent 'e' in number */
  for ( i = 0; ; i++ ) {
    if ( &inString[ i ] == s )
      return;

    if ( inString[ i ] == 'e' || inString[ i ] == 'E' ) {
    parse_exponent:;
      for ( j = i + 1; ; j++ ) {
        if ( &inString[ j ] == s )
          break;
        if ( inString[ j ] == '-' )
          negexp = true;
        else if ( inString[ j ] >= '0' && inString[ j ] <= '9' )
          exp = ( exp * 10 ) + ( inString[ j ] - '0' );
        else
          break;
      }
      if ( negexp )
        ex.precision += exp;
      /* maybe add a counter for significant digits when ! negexp */
      return;
    }

    if ( inString[ i ] == '.' ) {
      for ( j = i + 1; ; j++ ) {
        if ( &inString[ j ] == s )
          return;
        if ( inString[ j ] < '0' || inString[ j ] > '9' ) {
          if ( inString[ j ] == 'e' || inString[ j ] == 'E' ) {
            i = j;
            goto parse_exponent;
          }
          return; /* end of numberA*/
        }
        ex.precision++;
      }
    }
  }
}

template<class FTYPE>
static void
parse_FTYPE( const char *inString,  FTYPE &fVal,  const char **end,
             unsigned int base,  FTypeExtra *ex )
{
  const char * s,
             * units;
  unsigned int len;
  bool         isNeg, noFv;

  if ( inString == NULL ) {
    if ( end != NULL )
      *end = NULL;
    throw StrUtilErr::getErr( StrUtilErr::IS_NULL );
  }

  fVal  = (FTYPE) ::strtod( inString, (char **) &s );
  isNeg = false;
  noFv  = ( s == inString );
  if ( noFv ) {
    if ( ( base & ~U_MASK ) == 0 || *inString == '\0' ) {
      if ( end != NULL )
        goto no_digits;
      throw StrUtilErr::getErr( StrUtilErr::NOT_FLOAT );
    }
  }
  else {
    if ( ex != NULL )
      parse_float_precision( inString, s, *ex );
    if ( fVal < 0.0 )
      isNeg = true;
    else if ( fVal == 0.0 ) {
      do {
        if ( ! isspace( *inString ) ) {
          if ( *inString == '-' )
            isNeg = true;
          break;
        }
      } while ( ++inString < s );
    }
  }
  inString = s;

  if ( ( base & ( U_SECONDS | U_MILLISECS | U_NANOSECS | U_MEMORY | U_BITS |
                  U_FRACTION | U_PERCENT ) ) != 0 ) {
    for ( units = inString; isspace( *units ); units++ )
      ;
    for ( len = 0; units[ len ] != '\0' && ! isspace( units[ len ] ); len++ )
      ;
    if ( ( base & U_FRACTION ) != 0 ) {
      if ( len > 0 ) {
        const char * end2 = units;
        if ( *units == '/' ) {
          FTYPE den = (FTYPE) ::strtod( ++units, (char **) &end2 );
          if ( end2 > units ) {
            if ( den == 0.0 )
              throw StrUtilErr::getErr( StrUtilErr::DIV_ZERO );
            fVal /= den;
            units = end2;
            inString = end2;
            len = 0;
            noFv = false;
            if ( ex != NULL )
              ex->denom = (unsigned int) den;
          }
        }
        else {
          FTYPE num = (FTYPE) ::strtod( units, (char **) &end2 );
          if ( end2 > units && *end2++ == '/' ) {
            const char * end3 = end2;
            FTYPE den = (FTYPE) ::strtod( end2, (char **) &end3 );
            if ( end3 > end2 ) {
              if ( den == 0.0 )
                throw StrUtilErr::getErr( StrUtilErr::DIV_ZERO );
              if ( isNeg )
                fVal -= num / den;
              else
                fVal += num / den;
              units = end3;
              inString = end3;
              len = 0;
              noFv = false;
              if ( ex != NULL )
                ex->denom = (unsigned int) den;
            }
          }
        }
        if ( len == 0 &&
             ( base & ( U_SECONDS | U_MILLISECS | U_NANOSECS | U_MEMORY |
                        U_BITS | U_PERCENT ) ) != 0 ) {
          while ( isspace( *units ) )
            units++;
          len = 0;
          while ( units[ len ] != '\0' && ! isspace( units[ len ] ) )
            len++;
        }
      }
    }
    if ( ( base & U_PERCENT ) != 0 ) {
      if ( *units == '%' ) {
        fVal /= 100.0;
        if ( ex != NULL )
          ex->precision += 2;
        for ( units++; isspace( *units ); units++ )
          ;
        len = 0;
        while ( units[ len ] != '\0' && ! isspace( units[ len ] ) )
          len++;
      }
    }
    if ( ( base & ( U_SECONDS | U_MILLISECS | U_NANOSECS |
                    U_MEMORY | U_BITS ) ) != 0 ) {
      if ( len > 2 && ( units[ len - 1 ] == 's' || units[ len - 1 ] == 'S' ) )
        len--;
        
      if ( len > 0 ) {
        if ( noFv )
          fVal = 1.0;
        if ( ( base & ( U_SECONDS | U_MILLISECS | U_NANOSECS ) ) != 0 ) {
          if ( ( len == 2 && StrUtil::strncasecmp( units, "ms", 2 ) == 0 ) ||
	       ( len == 4 && StrUtil::strncasecmp( units, "msec", 4 ) == 0 ) ||
	       ( len == 8 && StrUtil::strncasecmp( units, "millisec", 8 ) == 0 ) ||
	       ( len == 11 && StrUtil::strncasecmp( units, "millisecond", 11 ) == 0 ) ) {
            if ( ( base & U_SECONDS ) != 0 )
              fVal /= (FTYPE) MSECS_IN_SEC;
            else if ( ( base & U_NANOSECS ) != 0 )
              fVal *= (FTYPE) NSECS_IN_MSEC;
          }
          else if ( ( len == 2 && StrUtil::strncasecmp( units, "wk", 2 ) == 0 ) ||
		    ( len == 4 && StrUtil::strncasecmp( units, "week", 4 ) == 0 ) ) {
            if ( ( base & U_SECONDS ) != 0 )
              fVal *= (FTYPE) SECS_IN_WEEK;
            else if ( ( base & U_NANOSECS ) != 0 )
              fVal *= (FTYPE) NSECS_IN_WEEK;
            else
              fVal *= (FTYPE) MSECS_IN_WEEK;
          }
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "d", 1 ) == 0 ) ||
                    ( len == 3 && StrUtil::strncasecmp( units, "day", 3 ) == 0 ) ) {
            if ( ( base & U_SECONDS ) != 0 )
              fVal *= (FTYPE) SECS_IN_DAY;
            else if ( ( base & U_NANOSECS ) != 0 )
              fVal *= (FTYPE) NSECS_IN_DAY;
            else
              fVal *= (FTYPE) MSECS_IN_DAY;
          }
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "h", 1 ) == 0 ) ||
		    ( len == 2 && StrUtil::strncasecmp( units, "hr", 2 ) == 0 ) ||
		    ( len == 4 && StrUtil::strncasecmp( units, "hour", 4 ) == 0 ) ) {
            if ( ( base & U_SECONDS ) != 0 )
              fVal *= (FTYPE) SECS_IN_HOUR;
            else if ( ( base & U_NANOSECS ) != 0 )
              fVal *= (FTYPE) NSECS_IN_HOUR;
            else
              fVal *= (FTYPE) MSECS_IN_HOUR;
          }
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "m", 1 ) == 0 ) ||
		    ( len == 3 && StrUtil::strncasecmp( units, "min", 3 ) == 0 ) ||
		    ( len == 6 && StrUtil::strncasecmp( units, "minute", 6 ) == 0 ) ) {
            if ( ( base & U_SECONDS ) != 0 )
              fVal *= (FTYPE) SECS_IN_MINUTE;
            else if ( ( base & U_NANOSECS ) != 0 )
              fVal *= (FTYPE) NSECS_IN_MINUTE;
            else
              fVal *= (FTYPE) MSECS_IN_MINUTE;
          }
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "s", 1 ) == 0 ) ||
		    ( len == 3 && StrUtil::strncasecmp( units, "sec", 3 ) == 0 ) ||
		    ( len == 6 && StrUtil::strncasecmp( units, "second", 6 ) == 0 ) ) {
            if ( ( base & U_SECONDS ) == 0 )
              fVal *= (FTYPE) MSECS_IN_SEC;
            else if ( ( base & U_NANOSECS ) != 0 )
              fVal *= (FTYPE) NSECS_IN_SEC;
          }
          else if ( ( len == 2 && StrUtil::strncasecmp( units, "ns", 2 ) == 0 ) ||
                    ( len == 4 && StrUtil::strncasecmp( units, "nsec", 4 ) == 0 ) ||
                    ( len == 7 && StrUtil::strncasecmp( units, "nanosec", 7 ) == 0 ) ||
                    ( len == 10 && StrUtil::strncasecmp( units, "nanosecond", 10 ) == 0 ) ) {
            if ( ( base & U_SECONDS ) != 0 )
              fVal /= (FTYPE) NSECS_IN_SEC;
            else if ( ( base & U_MILLISECS ) != 0 )
              fVal /= (FTYPE) NSECS_IN_MSEC;
          }
          else if ( ( len == 2 && StrUtil::strncasecmp( units, "us", 2 ) == 0 ) ||
                    ( len == 4 && StrUtil::strncasecmp( units, "usec", 4 ) == 0 ) ||
                    ( len == 8 && StrUtil::strncasecmp( units, "microsec", 8 ) == 0 ) ||
                    ( len == 11 && StrUtil::strncasecmp( units, "microsecond", 11 ) == 0 ) ) {
            if ( ( base & U_SECONDS ) != 0 )
              fVal /= (FTYPE) USECS_IN_SEC;
            else if ( ( base & U_MILLISECS ) != 0 )
              fVal /= (FTYPE) USECS_IN_MSEC;
            else if ( ( base & U_NANOSECS ) != 0 )
              fVal *= (FTYPE) NSECS_IN_USEC;
          }
          else
            throw StrUtilErr::getErr( StrUtilErr::NOT_TIME );
        }
        else if ( ( base & U_MEMORY ) != 0 ) {
          if ( ( len == 1 && StrUtil::strncasecmp( units, "b", 1 ) == 0 ) ||
               ( len == 4 && StrUtil::strncasecmp( units, "byte", 4 ) == 0 ) )
            ;
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "k", 1 ) == 0 ) ||
		    ( len == 2 && StrUtil::strncasecmp( units, "kb", 2 ) == 0 ) ||
		    ( len == 5 && StrUtil::strncasecmp( units, "kbyte", 5 ) == 0 ) ||
		    ( len == 8 && StrUtil::strncasecmp( units, "kilobyte", 8 ) == 0 ) )
            fVal *= (FTYPE) BYTES_IN_KILO;
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "m", 1 ) == 0 ) ||
		    ( len == 2 && StrUtil::strncasecmp( units, "mb", 2 ) == 0 ) ||
		    ( len == 5 && StrUtil::strncasecmp( units, "mbyte", 5 ) == 0 ) ||
		    ( len == 8 && StrUtil::strncasecmp( units, "megabyte", 8 ) == 0 ) )
            fVal *= (FTYPE) BYTES_IN_MEGA;
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "g", 1 ) == 0 ) ||
		    ( len == 2 && StrUtil::strncasecmp( units, "gb", 2 ) == 0 ) ||
		    ( len == 5 && StrUtil::strncasecmp( units, "gbyte", 5 ) == 0 ) ||
		    ( len == 8 && StrUtil::strncasecmp( units, "gigabyte", 8 ) == 0 ) )
            fVal *= (FTYPE) BYTES_IN_GIGA;
          else if ( len == 3 && StrUtil::strncasecmp( units, "bit", 3 ) == 0 )
            fVal /= (FTYPE) BITS_IN_BYTE;
          else if ( ( len == 4 && StrUtil::strncasecmp( units, "kbit", 4 ) == 0 ) ||
                    ( len == 7 && StrUtil::strncasecmp( units, "kilobit", 7 ) == 0 ) )
            fVal *= (FTYPE) ( BITS_IN_KILO / BITS_IN_BYTE );
          else if ( ( len == 4 && StrUtil::strncasecmp( units, "mbit", 4 ) == 0 )||
                    ( len == 7 && StrUtil::strncasecmp( units, "megabit", 7 ) == 0 ) )
            fVal *= (FTYPE) ( BITS_IN_MEGA / BITS_IN_BYTE );
          else if ( ( len == 4 && StrUtil::strncasecmp( units, "gbit", 4 ) == 0 ) ||
                    ( len == 7 && StrUtil::strncasecmp( units, "gigabit", 7 ) == 0 ) )
            fVal *= (FTYPE) ( BITS_IN_GIGA / BITS_IN_BYTE );
          else
            throw StrUtilErr::getErr( StrUtilErr::NOT_MEMORY );
        }
        else { /* base & U_BITS */
          if ( ( len == 1 && StrUtil::strncasecmp( units, "b", 1 ) == 0 ) ||
               ( len == 3 && StrUtil::strncasecmp( units, "bit", 3 ) == 0 ) )
            ;
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "k", 1 ) == 0 ) ||
                    ( len == 2 && StrUtil::strncasecmp( units, "kb", 2 ) == 0 ) ||
                    ( len == 4 && StrUtil::strncasecmp( units, "kbit", 4 ) == 0 ) ||
                    ( len == 7 && StrUtil::strncasecmp( units, "kilobit", 7 ) == 0 ) )
            fVal *= (FTYPE) BITS_IN_KILO;
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "m", 1 ) == 0 ) ||
                    ( len == 2 && StrUtil::strncasecmp( units, "mb", 2 ) == 0 ) ||
                    ( len == 4 && StrUtil::strncasecmp( units, "mbit", 4 ) == 0 )||
                    ( len == 7 && StrUtil::strncasecmp( units, "megabit", 7 ) == 0 ) )
            fVal *= (FTYPE) BITS_IN_MEGA;
          else if ( ( len == 1 && StrUtil::strncasecmp( units, "g", 1 ) == 0 ) ||
                    ( len == 2 && StrUtil::strncasecmp( units, "gb", 2 ) == 0 ) ||
                    ( len == 4 && StrUtil::strncasecmp( units, "gbit", 4 ) == 0 ) ||
                    ( len == 7 && StrUtil::strncasecmp( units, "gigabit", 7 ) == 0 ) )
            fVal *= (FTYPE) BITS_IN_GIGA;
          else if ( len == 4 && StrUtil::strncasecmp( units, "byte", 4 ) == 0 )
            fVal *= (FTYPE) BITS_IN_BYTE;
          else if ( ( len == 5 && StrUtil::strncasecmp( units, "kbyte", 5 ) == 0 ) ||
		    ( len == 8 && StrUtil::strncasecmp( units, "kilobyte", 8 ) == 0 ) )
            fVal *= (FTYPE) BYTES_IN_KILO * (FTYPE) BITS_IN_BYTE;
          else if ( ( len == 5 && StrUtil::strncasecmp( units, "mbyte", 5 ) == 0 ) ||
		    ( len == 8 && StrUtil::strncasecmp( units, "megabyte", 8 ) == 0 ) )
            fVal *= (FTYPE) BYTES_IN_MEGA * (FTYPE) BITS_IN_BYTE;
          else if ( ( len == 5 && StrUtil::strncasecmp( units, "gbyte", 5 ) == 0 ) ||
		    ( len == 8 && StrUtil::strncasecmp( units, "gigabyte", 8 ) == 0 ) )
            fVal *= (FTYPE) BYTES_IN_GIGA * (FTYPE) BITS_IN_BYTE;
          else
            throw StrUtilErr::getErr( StrUtilErr::NOT_MEMORY );
        }

        inString = &units[ len ];
      }
    }
  }
no_digits:;
  if ( end != NULL )
    *end = inString;
  if ( noFv && ex != NULL )
    ex->novalue = true;
}


bool
StrUtil::isValidFloat( const char *inString )
{
  char * s;
  if ( StrUtil::isValidInt( inString ) )
    return true;
  if ( inString == NULL )
    return false;
  ::strtod( inString, (char **) &s );
  if ( s == inString )
    return false;
  return true;
}


void
StrUtil::parseFloat( const char *inString,  float *fVal,
                     const char **end,  unsigned int base )
{
  parse_FTYPE( inString, *fVal, end, base, NULL );
}


void
StrUtil::parseFloat( const char *inString,  double *fVal,
                     const char **end,  unsigned int base )
{
  parse_FTYPE( inString, *fVal, end, base, NULL );
}


void
StrUtil::parseFloat2( const char *inString,  double *fVal,
                      const char **end,  unsigned int base,
                      unsigned int &precision,  unsigned int &denom,
                      bool &novalue )
{
  FTypeExtra ex;
  ::memset( &ex, 0, sizeof( ex ) );
  parse_FTYPE( inString, *fVal, end, base, &ex );
  precision = ex.precision;
  denom     = ex.denom;
  novalue   = ex.novalue;
}


void
StrUtil::parseFloat2( const char *inString,  float *fVal,
                      const char **end,  unsigned int base,
                      unsigned int &precision,  unsigned int &denom,
                      bool &novalue )
{
  FTypeExtra ex;
  ::memset( &ex, 0, sizeof( ex ) );
  parse_FTYPE( inString, *fVal, end, base, &ex );
  precision = ex.precision;
  denom     = ex.denom;
  novalue   = ex.novalue;
}


int
StrUtil::strncasecmp( const char *s1,  const char *s2,  unsigned int len )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  char c1, c2;

  while ( len > 0 ) {
    c1 = *s1;
    c2 = *s2;

    if ( c1 != c2 ) {
      if ( islower( c1 ) )
        c1 = toupper( c1 );
      if ( islower( c2 ) )
        c2 = toupper( c2 );

      if ( c1 != c2 )
        return c1 - c2;
    }
    if ( c1 == '\0' )
      break;

    s1++; s2++;
    len--;
  }

  return 0;
#else
  return ::strncasecmp( s1, s2, len );
#endif
}


int
StrUtil::strcasecmp( const char *s1,  const char *s2 )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  char c1, c2;

  for (;;) {
    c1 = *s1;
    c2 = *s2;

    if ( c1 != c2 ) {
      if ( islower( c1 ) )
        c1 = toupper( c1 );
      if ( islower( c2 ) )
        c2 = toupper( c2 );

      if ( c1 != c2 )
        return c1 - c2;
    }
    if ( c1 == '\0' )
      break;

    s1++; s2++;
  }

  return 0;
#else
  return ::strcasecmp( s1, s2 );
#endif
}


unsigned int
StrUtil::base64encode( const char *in,  unsigned int inLen,  char *out )
{
  static const char b64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned int val, outLen = 0;
  while ( inLen >= 3 ) {
    val = ( (unsigned int) (byte) in[ 0 ] << 16 ) |
          ( (unsigned int) (byte) in[ 1 ] << 8 ) |
          (unsigned int) (byte) in[ 2 ];
    out[ outLen ]     = b64[ ( val >> 18 ) & 63U ];
    out[ outLen + 1 ] = b64[ ( val >> 12 ) & 63U ];
    out[ outLen + 2 ] = b64[ ( val >> 6 ) & 63U ];
    out[ outLen + 3 ] = b64[ val & 63U ];
    inLen -= 3; in = &in[ 3 ];
    outLen += 4;
  }
  if ( inLen > 0 ) {
    val = (unsigned int) ( (byte) in[ 0 ] << 16 );
    if ( inLen > 1 )
      val |= (unsigned int) ( (byte) in[ 1 ] << 8 );
    out[ outLen++ ] = b64[ ( val >> 18 ) & 63U ];
    out[ outLen++ ] = b64[ ( val >> 12 ) & 63U ];
    if ( inLen > 1 )
      out[ outLen++ ] = b64[ ( val >> 6 ) & 63U ];
  }
  return outLen;
}


unsigned int
StrUtil::base64decode( const char *in,  unsigned int inLen,  char *out )
{
  static const byte u64[ 256 ] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63, 52,
     53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5,
     6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
     0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
     40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
  };
  unsigned int val, outLen = 0;
  while ( inLen >= 4 ) {
    val = ( (unsigned int) u64[ (byte) in[ 0 ] ] << 18 ) |
          ( (unsigned int) u64[ (byte) in[ 1 ] ] << 12 ) |
          ( (unsigned int) u64[ (byte) in[ 2 ] ] << 6 ) |
          ( (unsigned int) u64[ (byte) in[ 3 ] ] );
    out[ outLen ]     = (char) ( val >> 16 );
    out[ outLen + 1 ] = (char) ( ( val >> 8 ) & 0xffU );
    out[ outLen + 2 ] = (char) ( val & 0xffU );
    inLen -= 4; in = &in[ 4 ];
    outLen += 3;
  }
  /* this may trim zeros from input, need to null out up to length output */
  if ( inLen > 0 ) {
    val = ( (unsigned int) u64[ (byte) in[ 0 ] ] << 18 );
    if ( inLen > 1 )
      val |= ( (unsigned int) u64[ (byte) in[ 1 ] ] << 12 );
    if ( inLen > 2 )
      val |= ( (unsigned int) u64[ (byte) in[ 2 ] ] << 6 );
    out[ outLen++ ] = (char) ( val >> 16 );
    if ( ( val & 0xffffU ) != 0 ) {
      out[ outLen++ ] = (char) ( ( val >> 8 ) & 0xffU );
      if ( ( val & 0xffU ) != 0 )
        out[ outLen++ ] = (char) ( val & 0xffU );
    }
  }
  return outLen;
}


char *
StrUtil::catstrings( char *buf,  unsigned int bufLen,  const char *s, ... )
{
  char  * b = buf,
        * e = &buf[ bufLen ];
  va_list ap;

  if ( bufLen == 0 )
    return buf;
  if ( s == NULL ) {
    *b = '\0';
    return b;
  }
  va_start( ap, s );
  while ( b < e ) {
    /* if s overlaps buf, then only copy the current length of s */
    if ( s >= buf && s <= b ) {
      unsigned int i = ::strlen( s );
      if ( s == b ) {
        b = &b[ i ];
        if ( b > e )
          b = e;
      }
      else {
        while ( b < e ) {
          if ( i-- == 0 ) {
            *b = '\0';
            break;
          }
          *b++ = *s++;
        }
      }
      s = va_arg( ap, const char * );
      if ( s == NULL )
        goto done;
    }
    else {
      /* copy until the end */
      while ( b < e ) {
        if ( (*b++ = *s++) == '\0' ) {
          s = va_arg( ap, const char * );
          if ( s == NULL )
            goto done;
          --b;
          break;
        }
      }
    }
  }
done:;
  va_end( ap );

  if ( b == e )
    *--b = '\0';

  return buf;
}


Error
StrUtilErr::getErr( unsigned int status )
{
  static const char     mod[] = "StrUtil";
  static const ErrorRec err[] = {
  /*  0 */ { IS_NULL,      "Null parameter", mod },
  /*  1 */ { NOT_INTEGER,  "Can't convert string to integer", mod },
  /*  2 */ { NOT_OCTAL,    "Can't convert octal string to integer", mod },
  /*  3 */ { NOT_HEX,      "Can't convert hex string to integer", mod },
  /*  4 */ { BUF_OVERFLOW, "Converting to string overflows buffer", mod },
  /*  5 */ { NOT_BOOLEAN,  "Can't convert boolean string", mod },
  /*  6 */ { NOT_BASE64,   "Can't convert base64 string", mod },
  /*  7 */ { NOT_TIME,     "Time units not recognized", mod },
  /*  8 */ { NOT_MEMORY,   "Memory units not recognized", mod },
  /*  9 */ { NOT_FLOAT,    "Can't convert string to float", mod },
  /* 10 */ { DIV_ZERO,     "Denominator zero, divide by zero", mod },
  /* 11 */ { 11,           "Unknown string util error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}
