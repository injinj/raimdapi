/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__str_util_h__
#define __rai_util__str_util_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#include <string.h>

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

namespace rai {

enum Units {
  U_NUMBER    = 0,
  U_FRACTION  = 1,
  U_OCTAL     = 8,
  U_DECIMAL   = 10,
  U_HEX       = 16,
  U_MASK      = 31, /* for above formats which can be ored with below */
  U_SECONDS   = 32,
  U_MILLISECS = 64,
  U_MEMORY    = 128,
  U_NANOSECS  = 256,
  U_BITS      = 512,
  U_PERCENT   = 1024
};

static const unsigned int SECS_IN_MINUTE  = 60;
static const unsigned int SECS_IN_HOUR    = 60 * SECS_IN_MINUTE;
static const unsigned int SECS_IN_DAY     = 24 * SECS_IN_HOUR;
static const unsigned int SECS_IN_WEEK    = 7 * SECS_IN_DAY;
static const unsigned int MSECS_IN_SEC    = 1000;
static const unsigned int MSECS_IN_MINUTE = MSECS_IN_SEC * SECS_IN_MINUTE;
static const unsigned int MSECS_IN_HOUR   = MSECS_IN_SEC * SECS_IN_HOUR;
static const unsigned int MSECS_IN_DAY    = MSECS_IN_SEC * SECS_IN_DAY;
static const unsigned int MSECS_IN_WEEK   = MSECS_IN_SEC * SECS_IN_WEEK;
static const unsigned int BYTES_IN_KILO   = 1024;
static const unsigned int BYTES_IN_MEGA   = 1024 * BYTES_IN_KILO;
static const unsigned int BYTES_IN_GIGA   = 1024 * BYTES_IN_MEGA;
static const unsigned int BITS_IN_KILO    = 1000;
static const unsigned int BITS_IN_MEGA    = 1000 * BITS_IN_KILO;
static const unsigned int BITS_IN_GIGA    = 1000 * BITS_IN_MEGA;
static const unsigned int BITS_IN_BYTE    = 8;
static const ullong USECS_IN_MSEC   = 1000;
static const ullong USECS_IN_SEC    = USECS_IN_MSEC * 1000;
static const ullong USECS_IN_MINUTE = USECS_IN_SEC * (ullong) SECS_IN_MINUTE;
static const ullong USECS_IN_HOUR   = USECS_IN_SEC * (ullong) SECS_IN_HOUR;
static const ullong USECS_IN_DAY    = USECS_IN_SEC * (ullong) SECS_IN_DAY;
static const ullong USECS_IN_WEEK   = USECS_IN_SEC * (ullong) SECS_IN_WEEK;
static const ullong NSECS_IN_USEC   = 1000;
static const ullong NSECS_IN_MSEC   = NSECS_IN_USEC * 1000;
static const ullong NSECS_IN_SEC    = NSECS_IN_MSEC * 1000;
static const ullong NSECS_IN_MINUTE = NSECS_IN_SEC * (ullong) SECS_IN_MINUTE;
static const ullong NSECS_IN_HOUR   = NSECS_IN_SEC * (ullong) SECS_IN_HOUR;
static const ullong NSECS_IN_DAY    = NSECS_IN_SEC * (ullong) SECS_IN_DAY;
static const ullong NSECS_IN_WEEK   = NSECS_IN_SEC * (ullong) SECS_IN_WEEK;

namespace StrUtil {
  RAIBASE_DLL_EXP
  const char * asciiToXmlEntity( char c,  bool aposOk = false );

  RAIBASE_DLL_EXP
  unsigned int escapeXmlStringLen( const char *s,  bool aposOk = false );

  RAIBASE_DLL_EXP
  char * escapeXmlStringCopy( char *buf,  const char *s,  bool aposOk = false );

  RAIBASE_DLL_EXP
  char * escapeXmlString( char *buf,  unsigned int bufLen,  const char *s,
                          bool aposOk = false );
  RAIBASE_DLL_EXP
  char * escapeXmlString2( char *buf,  unsigned int bufLen,  const char *s,
                           unsigned int sLen,  bool aposOk = false );
  RAIBASE_DLL_EXP
  unsigned int escapeXmlBuf( char *buf,  const char *s,  unsigned int len,
                             bool aposOk = false );
  RAIBASE_DLL_EXP
  unsigned int stripXmlEntities( char *buf,  unsigned int len,
                            bool plusIsSpace = false, bool skipHexCvt = false );
  RAIBASE_DLL_EXP
  char * escapeJsonString( char *buf,  unsigned int bufLen,  const char *s );

  RAIBASE_DLL_EXP
  char * stripNewline( char *line,  unsigned int *lineLen );

  RAIBASE_DLL_EXP
  char * stripNewline( char *line );

  RAIBASE_DLL_EXP
  char * trimWhitespace( char *line,  unsigned int *lineLen );

  RAIBASE_DLL_EXP
  void parseInt( const char *inString,  int *iVal,
                 const char **end = NULL,  unsigned int base = U_NUMBER )
;
  RAIBASE_DLL_EXP
  void parseInt( const char *inString,  short *iVal,
                 const char **end = NULL,  unsigned int base = U_NUMBER )
;
  RAIBASE_DLL_EXP
  void parseInt( const char *inString,  long *iVal,
                 const char **end = NULL,  unsigned int base = U_NUMBER )
;
  RAIBASE_DLL_EXP
  void parseInt( const char *inString,  llong *iVal,
                 const char **end = NULL,  unsigned int base = U_NUMBER )
;
  RAIBASE_DLL_EXP
  void parseInt( const char *inString,  unsigned int *iVal,
                 const char **end = NULL,  unsigned int base = U_NUMBER )
;
  RAIBASE_DLL_EXP
  void parseInt( const char *inString,  unsigned long *iVal,
                 const char **end = NULL,  unsigned int base = U_NUMBER )
;
  RAIBASE_DLL_EXP
  void parseInt( const char *inString,  ullong *iVal,
                 const char **end = NULL,  unsigned int base = U_NUMBER )
;
  RAIBASE_DLL_EXP
  void parseInt( const char *inString,  unsigned short *iVal,
                 const char **end = NULL,  unsigned int base = U_NUMBER )
;
  RAIBASE_DLL_EXP
  void parseInt( const char *inString,  byte *iVal,
                 const char **end = NULL,  unsigned int base = U_NUMBER )
;
  RAIBASE_DLL_EXP
  bool parseBoolean( const char *inString );

  RAIBASE_DLL_EXP
  char *intToString( byte i,  char *buf,  unsigned int bufLen,
                     unsigned int base = U_DECIMAL,  bool abrev = true,
                     char **end = NULL );
  RAIBASE_DLL_EXP
  char *intToString( char i,  char *buf,  unsigned int bufLen,
                     unsigned int base = U_DECIMAL,  bool abrev = true,
                     char **end = NULL );
  RAIBASE_DLL_EXP
  char *intToString( unsigned short i,  char *buf,  unsigned int bufLen,
                     unsigned int base = U_DECIMAL,  bool abrev = true,
                     char **end = NULL );
  RAIBASE_DLL_EXP
  char *intToString( short i,  char *buf,  unsigned int bufLen,
                     unsigned int base = U_DECIMAL,  bool abrev = true,
                     char **end = NULL );
  RAIBASE_DLL_EXP
  char *intToString( unsigned int i,  char *buf,  unsigned int bufLen,
                     unsigned int base = U_DECIMAL,  bool abrev = true,
                     char **end = NULL );
  RAIBASE_DLL_EXP
  char *intToString( int i,  char *buf,  unsigned int bufLen,
                     unsigned int base = U_DECIMAL,  bool abrev = true,
                     char **end = NULL );
  RAIBASE_DLL_EXP
  char *intToString( ullong i,  char *buf,  unsigned int bufLen,
                     unsigned int base = U_DECIMAL,  bool abrev = true,
                     char **end = NULL );
  RAIBASE_DLL_EXP
  char *intToString( llong i,  char *buf,  unsigned int bufLen,
                     unsigned int base = U_DECIMAL,  bool abrev = true,
                     char **end = NULL );
  /* if decimalPlaces == 255, then float will be truncated to the last
   * non-zero digit up to 8 places, so 1.100 == 1.1 and 2.000 = 2.0 */
  static const unsigned int UNTIL_ZERO = 255;
  RAIBASE_DLL_EXP
  char *floatToString( double f,  char *buf,  unsigned int bufLen,
                       unsigned int decimalPlaces = 2,
                       unsigned int base = U_DECIMAL,
                       bool abrev = true, char **end = NULL )
;
  RAIBASE_DLL_EXP
  char *floatToString( float f,  char *buf,  unsigned int bufLen,
                       unsigned int decimalPlaces = 2,
                       unsigned int base = U_DECIMAL,
                       bool abrev = true, char **end = NULL )
;
  RAIBASE_DLL_EXP
  void parseFloat( const char *inString,  float *fVal,
                   const char **end = NULL,  unsigned int base = U_DECIMAL )
;
  RAIBASE_DLL_EXP
  void parseFloat( const char *inString,  double *fVal,
                   const char **end = NULL,  unsigned int base = U_DECIMAL )
;
  RAIBASE_DLL_EXP
  void parseFloat2( const char *inString,  double *fVal,
                    const char **end,  unsigned int base,
                    unsigned int &precision,  unsigned int &denom,
                    bool &novalue );
  RAIBASE_DLL_EXP
  void parseFloat2( const char *inString,  float *fVal,
                    const char **end,  unsigned int base,
                    unsigned int &precision,  unsigned int &denom,
                    bool &novalue );
  RAIBASE_DLL_EXP
  bool isValidInt( const char *inString );

  RAIBASE_DLL_EXP
  bool isValidFloat( const char *inString );

  RAIBASE_DLL_EXP
  int strncasecmp( const char *s1,  const char *s2,  unsigned int len );

  RAIBASE_DLL_EXP
  int strcasecmp( const char *s1,  const char *s2 );

  inline size_t strnlen( const char *s,  size_t len ) {
#if ! defined( __GLIBC__ ) || defined( __SUNPRO_CC )
    size_t len2 = 0;
    while ( len > 0 && *s != '\0' ) {
      len2++; --len; s++;
    }
    return len2;
#else
    return ::strnlen( s, len );
#endif
  }
  RAIBASE_DLL_EXP
  unsigned int base64encode( const char *in,  unsigned int inLen,
                             char *out );
  RAIBASE_DLL_EXP
  unsigned int base64decode( const char *in,  unsigned int inLen,
                             char *out ); /* may trim 0s from input, null out
                             output to length of input */
  RAIBASE_DLL_EXP
  char *catstrings( char *buf,  unsigned int bufLen,  const char *s, ... );
}


namespace StrUtilErr {
  enum {
    IS_NULL      = 0,
    NOT_INTEGER  = 1,
    NOT_OCTAL    = 2,
    NOT_HEX      = 3,
    BUF_OVERFLOW = 4,
    NOT_BOOLEAN  = 5,
    NOT_BASE64   = 6,
    NOT_TIME     = 7,
    NOT_MEMORY   = 8,
    NOT_FLOAT    = 9,
    DIV_ZERO     = 10
  };
  RAIBASE_DLL_EXP
  Error getErr( unsigned int status );
}
} // namespace rai

#endif
