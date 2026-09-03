/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIMSG_DLL_EXP ) && defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "msg/subject.h"
#include "base/mem.h"
#include "util/hash_util.h"

using namespace rai;
/*
 * MsaSubject binary format (SS.A.B.C):
 *
 *    length
 *    of each
 * |- segment -|- 1 -|2-|3-|4-|
 * +--+--+--+--+--+--+--+--+--+
 * |02|01|01|01| S| S| A| B| C|
 * +--+--+--+--+--+--+--+--+--+
 * 0  1  2  3  4  5  6  7  8  9
 *
 * SassSubject binary format (SS.A.B.C):
 *
 * # of  length
 * segs  of each
 * |--|- segment -|- seg1 -|- 2 -|- 3 -|- 4 -|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |04|02|01|01|01| S| S|\0| A|\0| B|\0| C|\0|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14
 *
 * RaiSubject binary format (SS.A.B.C):
 *
 * # of  length of
 * segs  seg1 + 1
 * |--|--|- seg1 -|--|- 2 -|--|- 3 -|--|- 4 -|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |04|04| S| S|\0|03| A|\0|03| B|\0|03| C|\0|\0|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
 */

static const unsigned int MSA_SEG_0_BITS = 2;
static const unsigned int MSA_SEG_1_BITS = 14;
static const unsigned int MSA_SEG_2_BITS = 3;
static const unsigned int MSA_SEG_3_BITS = 4;

static const unsigned int MSA_SEG_0_SHIFT = 21;
static const unsigned int MSA_SEG_1_SHIFT = 0;
static const unsigned int MSA_SEG_2_SHIFT = 18;
static const unsigned int MSA_SEG_3_SHIFT = 14;

static const unsigned int SASS_SEG_0_BITS = 3;
static const unsigned int SASS_SEG_1_BITS = 2;
static const unsigned int SASS_SEG_2_BITS = 14;
static const unsigned int SASS_SEG_3_BITS = 3;

static const unsigned int SASS_SEG_0_SHIFT = 19;
static const unsigned int SASS_SEG_1_SHIFT = 17;
static const unsigned int SASS_SEG_2_SHIFT = 0;
static const unsigned int SASS_SEG_3_SHIFT = 14;

/*const unsigned int MSA_WILDCARD_ALL  = ( 1U << 23 ) - 1U;
const unsigned int SASS_WILDCARD_ALL = ( 1U << 22 ) - 1U;*/

static const struct SegBits {
  unsigned int bits[ 4 ],
               shift[ 4 ],
               mask[ 4 ];
}  msaBits = {
  { MSA_SEG_0_BITS, MSA_SEG_1_BITS, MSA_SEG_2_BITS, MSA_SEG_3_BITS },
  { MSA_SEG_0_SHIFT, MSA_SEG_1_SHIFT, MSA_SEG_2_SHIFT, MSA_SEG_3_SHIFT },
  { ( 1U << MSA_SEG_0_BITS ) - 1, ( 1U << MSA_SEG_1_BITS ) - 1,
    ( 1U << MSA_SEG_2_BITS ) - 1, ( 1U << MSA_SEG_3_BITS ) - 1 }
}, sassBits = {
  { SASS_SEG_0_BITS, SASS_SEG_1_BITS, SASS_SEG_2_BITS, SASS_SEG_3_BITS },
  { SASS_SEG_0_SHIFT, SASS_SEG_1_SHIFT, SASS_SEG_2_SHIFT, SASS_SEG_3_SHIFT },
  { ( 1U << SASS_SEG_0_BITS ) - 1, ( 1U << SASS_SEG_1_BITS ) - 1,
    ( 1U << SASS_SEG_2_BITS ) - 1, ( 1U << SASS_SEG_3_BITS ) - 1 }
};
static const byte QUERY[] = { 'Q','U','E','R','Y','\0' };


void
Subject::clear( void )
{
  if ( this->buf != NULL ) {
    if ( this->allocFlag )
      FREE( this->buf );
  }
  this->buf       = NULL;
  this->hashVal   = 0;
  this->allocFlag = false;
  this->bufLen    = 0;
}


void
Subject::nullReference( void )
{
  throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );
}


unsigned int
Subject::computeHash( const byte *buf,  unsigned int len )
{
  return Hash32::crc_c( buf, len );
}


unsigned int
MsaSubject::layout( const char *segments[ 4 ],  unsigned int lens[ 4 ],
                     byte *toBuf )
{
  unsigned int i,
               len;

  toBuf[ 0 ] = (byte) lens[ 0 ] + 1;
  toBuf[ 1 ] = (byte) lens[ 1 ] + 1;
  toBuf[ 2 ] = (byte) lens[ 2 ] + 1;
  toBuf[ 3 ] = (byte) lens[ 3 ] + 1;
  len = 4;

  for ( i = 0; i < 4; i++ ) {
    if( lens[ i ] ) {
      ::memcpy( &toBuf[ len ], segments[ i ], lens[ i ] );
      len += lens[ i ];
      toBuf[ len++ ] = '\0';
    }
  }

  return len;
}

void
MsaSubject::set( byte *buf,  unsigned int bufLen,  bool isAlloced )

{
  if ( bufLen >= 4 ) {
    unsigned int len = (unsigned int) buf[ 0 ] + (unsigned int) buf[ 1 ] +
                       (unsigned int) buf[ 2 ] + (unsigned int) buf[ 3 ] + 4;
    if ( len <= bufLen ) {
      this->buf       = buf;
      this->hashVal   = 0;
      this->bufLen    = len;
      this->allocFlag = isAlloced;
      return;
    }
  }
  throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
}


void
SassSubject::set( byte *buf,  unsigned int bufLen,  bool isAlloced )

{
  if ( bufLen >= 5 && buf[ 0 ] == 4 ) {
    unsigned int len = (unsigned int) buf[ 1 ] + (unsigned int) buf[ 2 ] +
                       (unsigned int) buf[ 3 ] + (unsigned int) buf[ 4 ] + 9;
    if ( len <= bufLen ) {
      this->buf       = buf;
      this->hashVal   = 0;
      this->bufLen    = len;
      this->allocFlag = isAlloced;
      return;
    }
  }
  throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
}


void
RaiSubject::set( byte *buf,  unsigned int bufLen,  bool isAlloced )

{
  unsigned int len,
               segLen;
  byte         n;

  if ( bufLen > 0 ) {
    len = 1;
    for ( n = 0; n < buf[ 0 ]; n++ ) {
      segLen = (unsigned int) buf[ len ];
      if ( segLen < 3 )
        throw SubjectErr::getErr( SubjectErr::RV_EMPTY_SEG );
      len += segLen;
      if ( len > bufLen )
        throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
    }
    this->buf       = buf;
    this->hashVal   = 0;
    this->bufLen    = len;
    this->allocFlag = isAlloced;
    return;
  }
  throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
}


void
Subject::copy( const Subject &subj )
{
  const byte * buf     = subj.buf; /* so that subj.copy( subj ) works */
  unsigned int bufLen  = subj.bufLen,
               hashVal = subj.hashVal;

  if ( this->isStatic() )
    this->clear();

  if ( bufLen == 0 )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  REALLOC( bufLen, &this->buf );
  ::memcpy( this->buf, buf, bufLen );
  this->hashVal   = hashVal;
  this->bufLen    = bufLen;
  this->allocFlag = true;
}


void
SassSubject::copy( const SassSubject &subj,  byte *toBuf,
                   unsigned int toBufLen )
{
  if ( subj.bufLen > toBufLen )
    throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
  ::memcpy( toBuf, subj.buf, subj.bufLen );
  if ( this->isAlloced() )
    this->clear();
  this->buf     = toBuf;
  this->hashVal = subj.hashVal;
  this->bufLen  = subj.bufLen;
}


void
RaiSubject::copy( const RaiSubject &subj,  byte *toBuf,
                 unsigned int toBufLen )
{
  if ( subj.bufLen > toBufLen )
    throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
  ::memcpy( toBuf, subj.buf, subj.bufLen );
  if ( this->isAlloced() )
    this->clear();
  this->buf     = toBuf;
  this->hashVal = subj.hashVal;
  this->bufLen  = subj.bufLen;
}


void
MsaSubject::move( MsaSubject &subj )
{
  this->Subject::move( subj );
}


void
SassSubject::move( SassSubject &subj )
{
  this->Subject::move( subj );
}


void
RaiSubject::move( RaiSubject &subj )
{
  this->Subject::move( subj );
}


bool
MsaSubject::isWildcard( void ) const
{
  if ( this->buf != NULL && 
       ( this->buf[ 0 ] == 0 || this->buf[ 1 ] == 0 ||
         this->buf[ 2 ] == 0 || this->buf[ 3 ] == 0 ) )
    return true;
  return false;
}


bool
SassSubject::isWildcard( void ) const
{
  if ( this->buf != NULL && this->buf[ 0 ] == 4 &&
       ( this->buf[ 1 ] == 0 || this->buf[ 2 ] == 0 ||
         this->buf[ 3 ] == 0 || this->buf[ 4 ] == 0 ) )
    return true;
  return false;
}


bool
RaiSubject::isWildcard( void ) const
{
  unsigned int len,
               segLen;
  byte         n;

  if ( this->buf != NULL ) {
    len = 1;
    for ( n = 0; n < this->buf[ 0 ]; n++ ) {
      segLen = (unsigned int) this->buf[ len ];
      if ( segLen == 3 ) {
        if ( this->buf[ len + 1 ] == '*' )
          return true;
        if ( this->buf[ len + 1 ] == '>' && n + 1 == this->buf[ 0 ] )
          return true;
      }
      len += segLen;
    }
  }
  return false;
}


unsigned int
MsaSubject::decode( char *subject,  unsigned int maxSubjLen ) const

{
  unsigned int off,
               count,
               len,
               labelLen;
  const byte * ptr,
             * codedName;

  if ( (codedName = this->buf) == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  if ( maxSubjLen == 0 )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );

  for ( count = 4; count > 0 && codedName[ count - 1 ] == 0; count-- )
    ;

  len = 0;
  ptr = &codedName[ 4 ];

  for ( off = 0; off < count; off++ ) {
    labelLen = (unsigned int) codedName[ off ];
    if ( labelLen > 0 ) {
      if ( len + labelLen >= maxSubjLen )
        throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );
      ::memcpy( &subject[ len ], ptr, labelLen - 1 );
      ptr  = &ptr[ labelLen ];
      len += labelLen - 1;
    }
    if ( off + 1 < count ) {
      if ( len + 2 >= maxSubjLen )
        throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );
      subject[ len++ ] = '.';
    }
  }
  subject[ len ] = '\0';

  return len;
}


unsigned int
SassSubject::decode( char *subject,  unsigned int maxSubjLen ) const

{
  unsigned int off,
               count,
               len,
               labelLen;
  const byte * ptr,
             * codedName;

  if ( (codedName = this->buf) == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  if ( maxSubjLen == 0 )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );

  for ( count = 5; count > 1 && codedName[ count - 1 ] == 0; count-- )
    ;

  len = 0;
  ptr = &codedName[ 5 ];

  for ( off = 1; off < count; off++ ) {
    labelLen = (unsigned int) codedName[ off ];
    if ( labelLen > 0 ) {
      if ( len + labelLen + 1 >= maxSubjLen )
        throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );
      ::memcpy( &subject[ len ], ptr, labelLen );
      ptr  = &ptr[ labelLen + 1 ];
      len += labelLen;
    }
    else {
      ptr++;
    }
    if ( off + 1 < count ) {
      if ( len + 2 >= maxSubjLen )
        throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );
      subject[ len++ ] = '.';
    }
  }
  subject[ len ] = '\0';

  return len;
}


unsigned int
RaiSubject::decode( char *subject,  unsigned int maxSubjLen,
                    const char sep ) const
{
  const byte * ptr = this->buf;
  unsigned int off;
  byte         n;

  if ( ptr == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  if ( maxSubjLen == 0 )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );

  if ( ptr[ 0 ] == 0 ) {
    subject[ 0 ] = '\0';
    return 0;
  }
  const byte segCount = *ptr++;;
  off = 0;
  n   = 0;
  do {
    if ( off + (unsigned int) *ptr++ > maxSubjLen )
      throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );
    do {
      subject[ off++ ] = *ptr++;
    } while ( *ptr != '\0' );
    subject[ off++ ] = sep;
    ptr++;
  } while ( ++n < segCount );
  subject[ off - 1 ] = '\0';

  return off - 1;
}

unsigned int
MsaSubject::encode( const char *subject )
{
  const char * ptr,
             * label,
             * tmp;
  unsigned int len,
               labelLen,
               subLen,
               off;

  if ( subject == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  if ( this->isStatic() )
    this->clear();

  subLen = ::strlen( subject );
  REALLOC( 5 + subLen, &this->buf );
  ::memset( this->buf, 0, 4 );
  this->hashVal   = 0;
  this->allocFlag = true;

  off = 0;
  len = 4;
  ptr = subject;

  for ( label = ptr; off < 4; label = ++ptr ) {
    if ( (tmp = ::strchr( ptr, '.' )) != NULL )
      ptr = tmp;
    else
      ptr = &subject[ subLen ];

    if ( (labelLen = ptr - label) == 0 )
      off++;
    else {
      if ( labelLen + 1 > 0xffU )
        throw SubjectErr::getErr( SubjectErr::LABEL_TOO_BIG );
      this->buf[ off++ ] = labelLen + 1;
      ::memcpy( &this->buf[ len ], label, labelLen );
      len += labelLen;
      this->buf[ len++ ] = 0;
    }

    if ( *ptr == '\0' )
      break;
  }

  this->bufLen = len;
  return len;
}

unsigned int
MsaSubject::encode( const char *segments[ 4 ],  unsigned int lens[ 4 ],
                     byte *toBuf, unsigned int bufSize )
{
  unsigned int len;

  len = MsaSubject::validate( lens );

  if ( toBuf == NULL || len > bufSize ) {
    if ( this->isStatic() )
      this->clear();

    REALLOC( len, &this->buf );
    this->allocFlag = true;
    toBuf = this->buf;
  }
  else {
    if ( this->isAlloced() )
      this->clear();
    this->buf = toBuf;
  }
  this->hashVal = 0;
  this->bufLen  = len;

  return MsaSubject::layout( segments, lens, toBuf );
}

unsigned int
MsaSubject::validate( unsigned int lens[ 4 ] )
{
  unsigned int len,
               i;
  len = 8;      // segment lens (4) + segment null ( 4 ) 
  for ( i = 0; i < 4; i++ ) {
    if ( lens[ i ] > 255 )
      throw SubjectErr::getErr( SubjectErr::LABEL_TOO_BIG );
    len += lens[ i ];
  }

  return len;
}

unsigned int
MsaSubject::convert( const RaiSubject &subj,  bool isWild,
                      byte *toBuf,  unsigned int bufSize )
{
  const char * segs[ SassConst::MAX_RV_SEGMENTS ];
  unsigned int lens[ SassConst::MAX_RV_SEGMENTS ],
               i,
               count;

  count = subj.getSegments( segs, lens, SassConst::MAX_RV_SEGMENTS );
  if ( count > 4 )
    count = 4;
  for ( i = 0; i < count; i++ ) {
    if ( isWild ) {
      /* translate MSA wildcards into RV wildcards */
      if ( ( segs[ i ][ 0 ] == '*' && segs[ i ][ 1 ] == '\0' ) ||
           ( segs[ i ][ 0 ] == '>' && segs[ i ][ 1 ] == '\0' &&
             i + 1 == count ) ) {
        segs[ i ] = "";
        lens[ i ] = 0;
      }
    }
    else {
      if ( segs[ i ][ 0 ] == '=' && segs[ i ][ 1 ] == '0' &&
           segs[ i ][ 2 ] == '\0' ) {
        segs[ i ] = "";
        lens[ i ] = 0;
      }
    }
  }
  for ( ; i < 4; i++ ) {
    segs[ i ] = "";
    lens[ i ] = 0;
  }

  return this->encode( segs, lens, toBuf, bufSize );
}

unsigned int
SassSubject::validate( const char *subject )
{
  const char * ptr,
             * label,
             * tmp;
  unsigned int len,
               labelLen,
               off;

  if ( subject == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  off = 1;
  len = 5;
  ptr = subject;

  for ( label = ptr; off < 5; label = ++ptr ) {
    if ( (tmp = ::strchr( ptr, '.' )) != NULL )
      ptr = tmp;
    else
      ptr = &ptr[ ::strlen( ptr ) ];

    labelLen = ptr - label;
    if ( labelLen > 255 )
      throw SubjectErr::getErr( SubjectErr::LABEL_TOO_BIG );
    off++;
    len += labelLen + 1;

    if ( *ptr == '\0' ) {
      len += 5 - off;
      break;
    }
  }

  return len;
}


unsigned int
SassSubject::encode( const char *subject,  byte *toBuf,  unsigned int bufSize )

{
  unsigned int len;

  len = SassSubject::validate( subject );

  if ( toBuf == NULL || len > bufSize ) {
    if ( this->isStatic() )
      this->clear();

    REALLOC( len, &this->buf );
    this->allocFlag = true;
    toBuf = this->buf;
  }
  else {
    if ( this->isAlloced() )
      this->clear();
    this->buf = toBuf;
  }
  this->hashVal = 0;
  this->bufLen  = len;

  return SassSubject::layout( subject, toBuf );
}


unsigned int
SassSubject::layout( const char *subject,  byte *toBuf )
{
  const char * ptr,
             * label,
             * tmp;
  unsigned int len,
               labelLen,
               off;

  toBuf[ 0 ] = 4;
  ::memset( &toBuf[ 1 ], 0, 4 );

  off = 1;
  len = 5;
  ptr = subject;

  for ( label = ptr; off < 5; label = ++ptr ) {
    if ( (tmp = ::strchr( ptr, '.' )) != NULL )
      ptr = tmp;
    else
      ptr = &ptr[ ::strlen( ptr ) ];

    labelLen = ptr - label;
    toBuf[ off++ ] = labelLen;
    ::memcpy( &toBuf[ len ], label, labelLen );
    len += labelLen;
    toBuf[ len++ ] = 0;

    if ( *ptr == '\0' ) {
      while ( off < 5 ) {
        toBuf[ len++ ] = 0;
        off++;
      }
      break;
    }
  }

  return len;
}


unsigned int
SassSubject::validate( const char *segments[ 4 ] )
{
  unsigned int segLen,
               len,
               i;

  if ( segments == NULL || segments[ 0 ] == NULL || segments[ 1 ] == NULL ||
                           segments[ 2 ] == NULL || segments[ 3 ] == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  len = 9;
  for ( i = 0; i < 4; i++ ) {
    segLen = ::strlen( segments[ i ] );
    if ( segLen > 255 )
      throw SubjectErr::getErr( SubjectErr::LABEL_TOO_BIG );
    len += segLen;
  }

  return len;
}


unsigned int
SassSubject::validate( unsigned int lens[ 4 ] )
{
  unsigned int len,
               i;
  len = 9;
  for ( i = 0; i < 4; i++ ) {
    if ( lens[ i ] > 255 )
      throw SubjectErr::getErr( SubjectErr::LABEL_TOO_BIG );
    len += lens[ i ];
  }

  return len;
}


unsigned int
SassSubject::encode( const char *segments[ 4 ],  byte *toBuf,
                     unsigned int bufSize )
{
  unsigned int len;

  len = SassSubject::validate( segments );

  if ( toBuf == NULL || len > bufSize ) {
    if ( this->isStatic() )
      this->clear();

    REALLOC( len, &this->buf );
    this->allocFlag = true;
    toBuf = this->buf;
  }
  else {
    if ( this->isAlloced() )
      this->clear();
    this->buf = toBuf;
  }
  this->hashVal = 0;
  this->bufLen  = len;

  return SassSubject::layout( segments, toBuf );
}


unsigned int
SassSubject::encode( const char *segments[ 4 ],  unsigned int lens[ 4 ],
                     byte *toBuf, unsigned int bufSize )
{
  unsigned int len;

  len = SassSubject::validate( lens );

  if ( toBuf == NULL || len > bufSize ) {
    if ( this->isStatic() )
      this->clear();

    REALLOC( len, &this->buf );
    this->allocFlag = true;
    toBuf = this->buf;
  }
  else {
    if ( this->isAlloced() )
      this->clear();
    this->buf = toBuf;
  }
  this->hashVal = 0;
  this->bufLen  = len;

  return SassSubject::layout( segments, lens, toBuf );
}


unsigned int
SassSubject::layout( const char *segments[ 4 ],  byte *toBuf )
{
  const char * ptr;
  unsigned int i,
               off,
               len,
               labelLen;

  toBuf[ 0 ] = 4;
  ::memset( &toBuf[ 1 ], 0, 4 );
  off = 1;
  len = 5;

  for ( i = 0; i < 4; i++ ) {
    for ( ptr = segments[ i ]; ; ptr++ ) {
      if ( (toBuf[ len++ ] = (byte) *ptr) == 0 ) {
        labelLen = ptr - segments[ i ];
        toBuf[ off++ ] = labelLen;
        break;
      }
    }
  }

  return len;
}


unsigned int
SassSubject::layout( const char *segments[ 4 ],  unsigned int lens[ 4 ],
                     byte *toBuf )
{
  unsigned int i,
               len;

  toBuf[ 0 ] = 4;
  toBuf[ 1 ] = (byte) lens[ 0 ];
  toBuf[ 2 ] = (byte) lens[ 1 ];
  toBuf[ 3 ] = (byte) lens[ 2 ];
  toBuf[ 4 ] = (byte) lens[ 3 ];
  len = 5;

  for ( i = 0; i < 4; i++ ) {
    ::memcpy( &toBuf[ len ], segments[ i ], lens[ i ] );
    len += lens[ i ];
    toBuf[ len++ ] = '\0';
  }

  return len;
}


unsigned int
RaiSubject::validate( const char *subject,  unsigned int &numSegs,
                      const char sep )
{
  register const char * ptr, * last;
  register unsigned int segCount, err;

  if ( subject == NULL )
    err = SubjectErr::SUBJECT_IS_NULL;
  else {
    last = subject;
    segCount = 1;
    err = 0;
    for ( ptr = subject; ; ptr++ ) {
      if ( *ptr == sep || *ptr == '\0' ) {
        if ( ptr == last ) {
          err = SubjectErr::RV_EMPTY_SEG;
          break;
        }
        if ( ptr - last > 253 ) { /* can't be bigger than a byte */
          err = SubjectErr::LABEL_TOO_BIG;
          break;
        }
        if ( *ptr == '\0' )
          break;
        last = ptr + 1;
        segCount++;
      }
    }
    /* must fit in a byte */
    if ( segCount > 255 )
      err = SubjectErr::RV_TOO_MANY_SEGS;
    numSegs = segCount;
  }
  if ( err != 0 )
    throw SubjectErr::getErr( err );
  return (unsigned int) ( ptr - subject ) +    /* length of the string */
                        ( segCount * 2 ) + 1 - /* 2 for each segment + 1 segs */
                        ( segCount - 1 );      /* subtract the '.' count */
}

unsigned int
RaiSubject::validate2( const char *subject,  unsigned int maxLen,
                       unsigned int &numSegs,  const char sep )
{
  register const char * ptr, * last;
  register unsigned int segCount, err;

  if ( subject == NULL || maxLen == 0 )
    err = SubjectErr::SUBJECT_IS_NULL;
  else {
    last = subject;
    segCount = 1;
    err = 0;
    maxLen++;
    for ( ptr = subject; ; ptr++ ) {
      if ( --maxLen == 0 || *ptr == sep || *ptr == '\0' ) {
        if ( ptr == last ) {
          err = SubjectErr::RV_EMPTY_SEG;
          break;
        }
        if ( ptr - last > 253 ) { /* can't be bigger than a byte */
          err = SubjectErr::LABEL_TOO_BIG;
          break;
        }
        if ( maxLen == 0 || *ptr == '\0' )
          break;
        last = ptr + 1;
        segCount++;
      }
    }
    /* must fit in a byte */
    if ( segCount > 255 )
      err = SubjectErr::RV_TOO_MANY_SEGS;
    numSegs = segCount;
  }
  if ( err != 0 )
    throw SubjectErr::getErr( err );
  return (unsigned int) ( ptr - subject ) +    /* length of the string */
                        ( segCount * 2 ) + 1 - /* 2 for each segment + 1 segs */
                        ( segCount - 1 );      /* subtract the '.' count */
}


unsigned int
RaiSubject::encode( const char *subject,  byte *toBuf,  unsigned int bufSize,
                    const char sep )
{
  unsigned int len,
               segCount;

  if ( bufSize != 0 ) {
    if ( (len = this->staticLayout( subject, toBuf, bufSize, sep )) != 0 )
      return len;
  }
  len = RaiSubject::validate( subject, segCount, sep );

  if ( toBuf == NULL || len > bufSize ) {
    if ( this->isStatic() )
      this->clear();

    if ( len > this->bufLen )
      REALLOC( len, &this->buf );
    this->allocFlag = true;
    toBuf = this->buf;
  }
  else {
    if ( this->isAlloced() )
      this->clear();
    this->buf = toBuf;
  }
  this->hashVal = 0;
  this->bufLen  = len;

  return RaiSubject::layout( subject, segCount, toBuf, sep );
}


unsigned int
RaiSubject::encode2( const char *subject,  unsigned int maxLen,  byte *toBuf,
                     unsigned int bufSize,  const char sep )
{
  unsigned int len,
               segCount;

  if ( bufSize != 0 ) {
    if ( (len = this->staticLayout2( subject, maxLen, toBuf, bufSize,
                                     sep )) != 0 )
      return len;
  }
  len = RaiSubject::validate2( subject, maxLen, segCount, sep );

  if ( toBuf == NULL || len > bufSize ) {
    if ( this->isStatic() )
      this->clear();

    if ( len > this->bufLen )
      REALLOC( len, &this->buf );
    this->allocFlag = true;
    toBuf = this->buf;
  }
  else {
    if ( this->isAlloced() )
      this->clear();
    this->buf = toBuf;
  }
  this->hashVal = 0;
  this->bufLen  = len;

  return RaiSubject::layout2( subject, maxLen, segCount, toBuf, sep );
}


unsigned int
RaiSubject::layout( const char *subject,  unsigned int segCount,
                    byte *toBuf,  const char sep )
{
  register const char * ptr, * last;
  register unsigned int off;

  toBuf[ 0 ] = (byte) segCount;
  last = subject;
  off  = 1;
  for ( ptr = subject; ; ptr++ ) {
    if ( *ptr == sep || *ptr == '\0' ) {
      toBuf[ off++ ] = (byte)( ptr - last + 2 );
      while ( last < ptr )
        toBuf[ off++ ] = *last++;
      toBuf[ off++ ] = '\0';
      if ( *ptr == '\0' )
        break;
      last++;
    }
  }
  return off;
}


unsigned int
RaiSubject::layout2( const char *subject,  unsigned int maxLen,
                     unsigned int segCount,  byte *toBuf,  const char sep )
{
  register const char * ptr, * last;
  register unsigned int off;

  toBuf[ 0 ] = (byte) segCount;
  last = subject;
  off  = 1;
  maxLen++;
  for ( ptr = subject; ; ptr++ ) {
    if ( --maxLen == 0 || *ptr == sep || *ptr == '\0' ) {
      toBuf[ off++ ] = (byte)( ptr - last + 2 );
      while ( last < ptr )
        toBuf[ off++ ] = *last++;
      toBuf[ off++ ] = '\0';
      if ( maxLen == 0 || *ptr == '\0' )
        break;
      last++;
    }
  }
  return off;
}


unsigned int
RaiSubject::staticLayout( const char *subject,  byte *toBuf,
                  unsigned int toBufLen,  const char sep )
{
  register const char * ptr, * last;
  register unsigned int off;
  register unsigned long len;
  unsigned int err = 0;

  off = 1;
  if ( off >= toBufLen )
    return 0;
  toBuf[ 0 ] = 0;
  last = subject;

  for ( ptr = subject; ; ptr++ ) {
    if ( *ptr == sep || *ptr == '\0' ) {
      if ( ++toBuf[ 0 ] == 0 ) {
        err = SubjectErr::RV_TOO_MANY_SEGS;
        break;
      }
      len = ( ptr - last + 2 );
      if ( len == 2 ) {
        err = SubjectErr::RV_EMPTY_SEG;
        break;
      }
      if ( len > 253 ) {
        err = SubjectErr::LABEL_TOO_BIG;
        break;
      }
      if ( len + off + 2 > toBufLen )
        return 0;

      toBuf[ off++ ] = (byte) len;
      while ( last < ptr )
        toBuf[ off++ ] = *last++;
      toBuf[ off++ ] = '\0';
      if ( *ptr == '\0' )
        break;
      last++;
    }
  }

  if ( err != 0 )
    throw SubjectErr::getErr( err );

  if ( this->isAlloced() )
    this->clear();

  this->buf     = toBuf;
  this->hashVal = 0;
  this->bufLen  = off;

  return off;
}


unsigned int
RaiSubject::staticLayout2( const char *subject,  unsigned int maxLen,
                           byte *toBuf,  unsigned int toBufLen,
                           const char sep )
{
  register const char * ptr, * last;
  register unsigned int off;
  register unsigned long len;
  unsigned int err = 0;

  off = 1;
  if ( off >= toBufLen )
    return 0;
  toBuf[ 0 ] = 0;
  last = subject;

  maxLen++;
  for ( ptr = subject; ; ptr++ ) {
    if ( --maxLen == 0 || *ptr == sep || *ptr == '\0' ) {
      if ( ++toBuf[ 0 ] == 0 ) {
        err = SubjectErr::RV_TOO_MANY_SEGS;
        break;
      }
      len = ( ptr - last + 2 );
      if ( len == 2 ) {
        err = SubjectErr::RV_EMPTY_SEG;
        break;
      }
      if ( len > 253 ) {
        err = SubjectErr::LABEL_TOO_BIG;
        break;
      }
      if ( len + off + 2 > toBufLen )
        return 0;

      toBuf[ off++ ] = (byte) len;
      while ( last < ptr )
        toBuf[ off++ ] = *last++;
      toBuf[ off++ ] = '\0';
      if ( maxLen == 0 || *ptr == '\0' )
        break;
      last++;
    }
  }

  if ( err != 0 )
    throw SubjectErr::getErr( err );

  if ( this->isAlloced() )
    this->clear();

  this->buf     = toBuf;
  this->hashVal = 0;
  this->bufLen  = off;

  return off;
}


unsigned int
RaiSubject::toRIC( char *ricBuf,  unsigned int maxLen,
                   const char *sector,  bool dontMapCaretToDot,
                   bool dontAppendNaE )
{
  const char * seg[ SassConst::MAX_RV_SEGMENTS ];
  unsigned int lens[ SassConst::MAX_RV_SEGMENTS ],
               i, nSegs;

  nSegs = this->getSegments( seg, lens, sizeof( seg ) / sizeof( seg[ 0 ] ) );
  if ( nSegs == 0 )
    throw SubjectErr::getErr( SubjectErr::BAD_RIC );

  /* find <SYMBOL>.<EXCH> part of the subject */
  for ( i = 0; ; i++ ) {
    if ( ::strcmp( seg[ i ], sector ) == 0 )
      goto break_loop;
    if ( i == nSegs - 1 ) { /* didn't find REC, LINK, PAGE */
      if ( nSegs >= 2 )
        i = nSegs - 2;
      else
        i = nSegs - 1;
      goto break_loop2;
    }
  }
break_loop:;
  if ( ++i >= nSegs )
    throw SubjectErr::getErr( SubjectErr::BAD_RIC );
break_loop2:;
  return this->toRIC2( &seg[ i ], &lens[ i ], nSegs - i, ricBuf, 0, maxLen,
                       dontMapCaretToDot, dontAppendNaE );
}


unsigned int
RaiSubject::toRIC( char *ricBuf,  unsigned int maxLen,  unsigned int prefixSegs,
                   bool dontMapCaretToDot,  bool dontAppendNaE )
{
  const char * seg[ SassConst::MAX_RV_SEGMENTS ];
  unsigned int lens[ SassConst::MAX_RV_SEGMENTS ],
               i = prefixSegs, nSegs;

  nSegs = this->getSegments( seg, lens, sizeof( seg ) / sizeof( seg[ 0 ] ) );
  if ( nSegs == 0 || i >= nSegs )
    throw SubjectErr::getErr( SubjectErr::BAD_RIC );
  return this->toRIC2( &seg[ i ], &lens[ i ], nSegs - i, ricBuf, 0, maxLen,
                       dontMapCaretToDot, dontAppendNaE );
}


unsigned int
RaiSubject::toRIC( char *ricBuf,  unsigned int maxLen,
                   bool dontMapCaretToDot,  bool dontAppendNaE )
{
  const char * seg[ SassConst::MAX_RV_SEGMENTS ];
  unsigned int lens[ SassConst::MAX_RV_SEGMENTS ],
               i, nSegs, off;

  nSegs = this->getSegments( seg, lens, sizeof( seg ) / sizeof( seg[ 0 ] ) );
  if ( nSegs == 0 )
    throw SubjectErr::getErr( SubjectErr::BAD_RIC );

  /* find <SYMBOL>.<EXCH> part of the subject */
  off = 0;
  for ( i = 0; ; i++ ) {
    switch ( seg[ i ][ 0 ] ) {
      case 'A':
        if ( ::strcmp( seg[ i ], "ANY" ) == 0 )
          goto break_loop;
        break;
      case 'R':
        if ( ::strcmp( seg[ i ], "REC" ) == 0 )
          goto break_loop;
        break;
      case 'M':
        if ( ::strcmp( seg[ i ], "MBO" ) == 0 )
          goto break_loop;
        if ( ::strcmp( seg[ i ], "MBP" ) == 0 )
          goto break_loop;
        break;
      case 'L':
        if ( ::strcmp( seg[ i ], "LINK" ) == 0 ) {
          if ( off == maxLen )
            throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
          ricBuf[ off++ ] = 'j';
          goto break_loop;
        }
        break;
      case 'P':
        if ( ::strcmp( seg[ i ], "PAGE" ) == 0 ) {
          if ( off == maxLen )
            throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
          ricBuf[ off++ ] = 'p';
          goto break_loop;
        }
        break;
      default:
        break;
    }
    if ( i == nSegs - 1 ) { /* didn't find REC, LINK, PAGE */
      if ( nSegs >= 2 )
        i = nSegs - 2;
      else
        i = nSegs - 1;
      goto break_loop2;
    }
  }

break_loop:;
  if ( ++i >= nSegs )
    throw SubjectErr::getErr( SubjectErr::BAD_RIC );
break_loop2:;
  return this->toRIC2( &seg[ i ], &lens[ i ], nSegs - i, &ricBuf[ off ],
                       off, maxLen - off, dontMapCaretToDot, dontAppendNaE );
}


unsigned int
RaiSubject::toRIC2( const char **seg,  unsigned int *lens,  unsigned int nSegs,
                    char *ricBuf,  unsigned int off,  unsigned int maxLen,
                    bool dontMapCaretToDot,  bool dontAppendNaE )
{
  unsigned int i = 0, startOff = off;
  const char * p;
  bool isNaE;

  if ( ! dontMapCaretToDot ) {
    if ( lens[ i ] + off >= maxLen )
      throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
    for ( p = seg[ i++ ]; ; off++ ) {
      if ( (ricBuf[ off ] = *p++) == '\0' )
        break;
      if ( ricBuf[ off ] == '^' )
        ricBuf[ off ] = '.';
    }
  }
  for ( ; i < nSegs; i++ ) {
    p = seg[ i ];
    if ( i == nSegs - 1 && ! dontAppendNaE && ::strcmp( p, "NaE" ) == 0 )
      isNaE = true;
    else
      isNaE = false;
    if ( i + 1 < nSegs || ! isNaE ) {
      if ( lens[ i ] + 1 + off >= maxLen )
        throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );

      if ( off > startOff )
        ricBuf[ off++ ] = '.';
      for ( ; ; off++ ) {
        if ( (ricBuf[ off ] = *p++) == '\0' )
          break;
      }
    }
  }
  return off;
}


unsigned int
RaiSubject::encodeRIC( const char *prefix,  const char *ric,
                       byte *toBuf,  unsigned int bufLen,
                       bool dontMapDotToCaret,
                       bool dontAppendNaE )
{
  const char * sector = "REC";
  if ( ric[ 0 ] == 'p' || ric[ 0 ] == 'j' ) {
    if ( ric[ 0 ] == 'p' )
      sector = "PAGE";
    else
      sector = "LINK";
    ric++;
  }
  return this->encodeRIC( prefix, sector, ric, toBuf, bufLen,
                          dontMapDotToCaret, dontAppendNaE );
}


unsigned int
RaiSubject::encodeRIC( const char *prefix,  const char *sector,
                       const char *ric,  byte *toBuf,  unsigned int bufLen,
                       bool dontMapDotToCaret,
                       bool dontAppendNaE )
{
  unsigned int segLen[ SassConst::MAX_RV_SEGMENTS ];
  const char * seg[ SassConst::MAX_RV_SEGMENTS ],
             * suffix,
             * endRic,
             * p;
  char         subj[ SassConst::MAX_SUBJECT_LEN ];
  unsigned int off, lastOff, ricLen, segCount, slen;

  slen   = ( sector == NULL || sector[ 0 ] == '\0' ? 0 : ::strlen( sector ) );
  ricLen = ::strlen( ric );
  off = 0;
  if ( prefix != NULL && prefix[ 0 ] != '\0' ) {
    if ( ::strlen( prefix ) + ricLen + 10 >= SassConst::MAX_SUBJECT_LEN )
      throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );
    segCount = 0;
    for ( lastOff = 0; *prefix != '\0'; off++ ) {
      if ( (subj[ off ] = *prefix++) == '.' ) {
        if ( segCount == SassConst::MAX_RV_SEGMENTS )
          throw SubjectErr::getErr( SubjectErr::MAX_SEG_OVERFLOW );
        seg[ segCount ] = &subj[ lastOff ];
        segLen[ segCount ] = off - lastOff;
        segCount++;
        subj[ off ] = '\0';
        lastOff = off + 1;
      }
    }
    if ( segCount + 3 == SassConst::MAX_RV_SEGMENTS )
      throw SubjectErr::getErr( SubjectErr::MAX_SEG_OVERFLOW );
    seg[ segCount ] = &subj[ lastOff ];
    segLen[ segCount ] = off - lastOff;
    segCount++;
    if ( slen > 0 ) {
      seg[ segCount ] = sector;
      segLen[ segCount ] = slen;
      segCount++;
    }
    subj[ off++ ] = '\0';
  }
  else {
    if ( ricLen + 10 >= SassConst::MAX_SUBJECT_LEN )
      throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );
    if ( slen > 0 ) {
      seg[ 0 ] = sector;
      segLen[ 0 ] = slen;
      segCount = 1;
    }
    else
      segCount = 0;
  }

  endRic = &ric[ ricLen ];
  if ( dontAppendNaE )
    suffix = NULL;
  else
    suffix = "NaE";
  for ( p = endRic - 1; p > ric; p-- ) {
    if ( *p == '.' ) {
      if ( p == ric || ( p == ric + 1 && ric[ 0 ] == 'd' ) ||
           p == endRic - 1 ) /* [.][.*]* | d[.][.*] | [.*]*[.] */
        break;
      if ( p[ 1 ] >= '0' && p[ 1 ] <= '9' ) /* match [.][0-9] */
        break;
      if ( *(p - 1) == '#' ) /* match #[.] */
        break;
      suffix = p + 1;
      endRic = p;
      break;
    }
  }
  if ( ! dontMapDotToCaret ) {
    for ( lastOff = off; ric < endRic; off++ ) {
      if ( (subj[ off ] = *ric++) == '.' )
        subj[ off ] = '^';
      else if ( subj[ off ] == '^' )
        throw SubjectErr::getErr( SubjectErr::CARET_IN_RIC );
    }
  }
  else {
    for ( lastOff = off; ric < endRic; off++ ) {
      if ( (subj[ off ] = *ric++) == '.' ) {
        if ( segCount + 2 == SassConst::MAX_RV_SEGMENTS )
          throw SubjectErr::getErr( SubjectErr::MAX_SEG_OVERFLOW );
        seg[ segCount ] = &subj[ lastOff ];
        segLen[ segCount ] = off - lastOff;
        lastOff = off + 1;
        segCount++;
        subj[ off ] = '\0';
      }
    }
  }
  seg[ segCount ] = &subj[ lastOff ];
  segLen[ segCount ] = off - lastOff;
  segCount++;
  subj[ off++ ] = '\0';

  if ( suffix != NULL ) {
    for ( lastOff = off; *suffix != '\0'; off++ )
      subj[ off ] = *suffix++;
    seg[ segCount ] = &subj[ lastOff ];
    segLen[ segCount ] = off - lastOff;
    segCount++;
    subj[ off ] = '\0';
  }
  return this->encode( seg, segLen, segCount, toBuf, bufLen );
}


RaiSubject *
RaiSubject::create( const char *subject )
{
  RaiSubject * s;
  unsigned int len,
               segCount;

  len = RaiSubject::validate( subject, segCount );

  MALLOC( sizeof( RaiSubject ) + len, &s );
  s->buf       = (byte *) &s[ 1 ];
  s->hashVal   = 0;
  s->bufLen    = len;
  s->allocFlag = false;

  s->layout( subject, segCount, s->buf );
  return s;
}


unsigned int
RaiSubject::validate( const char **segments,  unsigned int segCount )

{
  unsigned int i,
               len,
               segLen;

  if ( segCount == 0 )
    throw SubjectErr::getErr( SubjectErr::RV_EMPTY_SEG );

  len = 0;
  for ( i = 0; i < segCount; i++ ) {
    if ( segments[ i ] == NULL || segments[ i ][ 0 ] == '\0' )
      throw SubjectErr::getErr( SubjectErr::RV_EMPTY_SEG );
    segLen = ::strlen( segments[ i ] );
    if ( segLen > 253 )
      throw SubjectErr::getErr( SubjectErr::LABEL_TOO_BIG );
    len += segLen;
  }
  len += segCount * 2 + 1; /* 2 for each segment + 1 for segs */

  return len;
}


unsigned int
RaiSubject::validate( unsigned int *lens,  unsigned int segCount )

{
  unsigned int i,
               len,
               segLen;

  if ( segCount == 0 )
    throw SubjectErr::getErr( SubjectErr::RV_EMPTY_SEG );

  len = 0;
  for ( i = 0; i < segCount; i++ ) {
    segLen = lens[ i ];
    if ( segLen == 0 )
      throw SubjectErr::getErr( SubjectErr::RV_EMPTY_SEG );
    if ( segLen > 253 )
      throw SubjectErr::getErr( SubjectErr::LABEL_TOO_BIG );
    len += segLen;
  }
  len += segCount * 2 + 1; /* 2 for each segment + 1 for segs */

  return len;
}


unsigned int
RaiSubject::encode( const char **segments,  unsigned int segCount,
                    byte *toBuf,  unsigned int bufSize )
{
  unsigned int len;

  len = RaiSubject::validate( segments, segCount );

  if ( toBuf == NULL || len > bufSize ) {
    if ( this->isStatic() )
      this->clear();

    REALLOC( len, &this->buf );
    this->allocFlag = true;
    toBuf = this->buf;
  }
  else {
    if ( this->isAlloced() )
      this->clear();
    this->buf = toBuf;
  }
  this->hashVal = 0;
  this->bufLen  = len;

  return RaiSubject::layout( segments, segCount, toBuf );
}


unsigned int
RaiSubject::encode( const char **segments,  unsigned int *lens,
                    unsigned int segCount,  byte *toBuf,
                    unsigned int bufSize )
{
  unsigned int len;

  len = RaiSubject::validate( lens, segCount );

  if ( toBuf == NULL || len > bufSize ) {
    if ( this->isStatic() )
      this->clear();

    REALLOC( len, &this->buf );
    this->allocFlag = true;
    toBuf = this->buf;
  }
  else {
    if ( this->isAlloced() )
      this->clear();
    this->buf = toBuf;
  }
  this->hashVal = 0;
  this->bufLen  = len;

  return RaiSubject::layout( segments, lens, segCount, toBuf );
}


unsigned int
RaiSubject::layout( const char **segments,  unsigned int segCount,
                    byte *toBuf )
{
  const char * ptr;
  unsigned int i,
               off,
               segOff;

  toBuf[ 0 ] = (byte) segCount;
  off        = 1;
  for ( i = 0; i < segCount; i++ ) {
    ptr    = segments[ i ];
    segOff = off++;
    do {
      toBuf[ off++ ] = *ptr++;
    } while ( *ptr != '\0' );
    toBuf[ off++ ]  = '\0';
    toBuf[ segOff ] = (byte)( off - segOff );
  }

  return off;
}


unsigned int
RaiSubject::layout( const char **segments,  unsigned int *lens,
                    unsigned int segCount,  byte *toBuf )
{
  unsigned int i, j,
               off,
               len;

  toBuf[ 0 ] = (byte) segCount;
  off        = 1;
  for ( i = 0; i < segCount; i++ ) {
    len = lens[ i ];
    toBuf[ off++ ] = (byte) ( len + 2 );
    j = 0;
    do {
      toBuf[ off++ ] = segments[ i ][ j++ ];
    } while ( --len != 0 );
    toBuf[ off++ ] = '\0';
  }

  return off;
}


unsigned int
RaiSubject::concat( RaiSubject &subj1,  RaiSubject &subj2,  byte *toBuf,
                   unsigned int bufSize )
{
  const byte * buf1     = NULL,
             * buf2     = NULL;
  unsigned int segCount = 0,
               len1     = 0,
               len2     = 0,
               len,
               n1,
               n2;
  if ( (n1 = subj1.segmentCount()) > 0 ) {
    segCount += n1;
    len1      = subj1.length();
    buf1      = subj1.getBuf();
  }
  if ( (n2 = subj2.segmentCount()) > 0 ) {
    segCount += n2;
    len2      = subj2.length();
    buf2      = subj2.getBuf();
  }

  if ( n1 > 0 && n2 > 0 )
    len = len1 + len2 - 1;
  else if ( n1 > 0 )
    len = len1;
  else if ( n2 > 0 )
    len = len2;
  else {
    this->clear();
    return 0;
  }
  if ( toBuf == NULL || len > bufSize ) {
    if ( this->isStatic() )
      this->clear();

    REALLOC( len, &this->buf );
    this->allocFlag = true;
    toBuf = this->buf;
  }
  else {
    if ( this->isAlloced() )
      this->clear();
    this->buf = toBuf;
  }
  this->hashVal = 0;
  this->bufLen  = len;

  if ( n1 > 0 && n2 > 0 ) {
    toBuf[ 0 ] = (byte) segCount;
    ::memcpy( &toBuf[ 1 ], &buf1[ 1 ], len1 - 1 );
    ::memcpy( &toBuf[ len1 ], &buf2[ 1 ], len2 - 1 );
  }
  else if ( n1 > 0 )
    ::memcpy( toBuf, buf1, len1 );
  else
    ::memcpy( toBuf, buf2, len2 );
  return len;
}


unsigned int
SassSubject::convert( const RaiSubject &subj,  bool isWild,
                      byte *toBuf,  unsigned int bufSize )
{
  const char * segs[ SassConst::MAX_RV_SEGMENTS ];
  unsigned int lens[ SassConst::MAX_RV_SEGMENTS ],
               i,
               count;

  count = subj.getSegments( segs, lens, SassConst::MAX_RV_SEGMENTS );
  if ( count > 4 )
    count = 4;
  for ( i = 0; i < count; i++ ) {
    if ( isWild ) {
      /* translate sass wildcards into RV wildcards */
      if ( ( segs[ i ][ 0 ] == '*' && segs[ i ][ 1 ] == '\0' ) ||
           ( segs[ i ][ 0 ] == '>' && segs[ i ][ 1 ] == '\0' &&
             i + 1 == count ) ) {
        segs[ i ] = "";
        lens[ i ] = 0;
      }
    }
    else {
      if ( segs[ i ][ 0 ] == '=' && segs[ i ][ 1 ] == '0' &&
           segs[ i ][ 2 ] == '\0' ) {
        segs[ i ] = "";
        lens[ i ] = 0;
      }
    }
  }
  for ( ; i < 4; i++ ) {
    segs[ i ] = "";
    lens[ i ] = 0;
  }

  return this->encode( segs, lens, toBuf, bufSize );
}


unsigned int
RaiSubject::convert( const SassSubject &subj,  bool isWild,  byte *toBuf,
                    unsigned int bufSize )
{
  const char * segs[ 4 ];
  unsigned int i,
               count;

  for ( i = 0; i < 4; i++ )
    segs[ i ] = subj.segment( i );

  for ( count = 4; count > 0; ) {
    if ( segs[ count - 1 ][ 0 ] != '\0' )
      break;
    count--;
  }
  if ( count > 0 ) {
    for ( i = 0; i < count - 1; i++ ) {
      /* sass can have empty segments for subjects, RV cannot */
      if ( segs[ i ][ 0 ] == '\0' ) {
        if ( isWild )
          segs[ i ] = "*";
        else
          segs[ i ] = "=0";
      }
    }
  }
  if ( isWild && count < 4 )
    segs[ count++ ] = ">";

  return this->encode( segs, count, toBuf, bufSize );
}

unsigned int
RaiSubject::convert( const MsaSubject &subj,  bool isWild,  byte *toBuf,
                    unsigned int bufSize )
{
  const char * segs[ 4 ];
  unsigned int i,
               count;

  for ( i = 0; i < 4; i++ )
    segs[ i ] = subj.segment( i );

  for ( count = 4; count > 0; ) {
    if ( segs[ count - 1 ][ 0 ] != '\0' )
      break;
    count--;
  }
  if ( count > 0 ) {
    for ( i = 0; i < count - 1; i++ ) {
      /* MSA can have empty segments for subjects, RV cannot */
      if ( segs[ i ][ 0 ] == '\0' ) {
        if ( isWild )
          segs[ i ] = "*";
        else
          segs[ i ] = "=0";
      }
    }
  }
  if ( isWild && count < 4 )
    segs[ count++ ] = ">";

  return this->encode( segs, count, toBuf, bufSize );
}


unsigned int
RaiSubject::convert2( const SassSubject &subj,  const char * prefix, bool isWild,  byte *toBuf,
                    unsigned int bufSize )
{
  const char * segs[ 5 ];
  unsigned int i,
               count;

  segs[ 0 ] = prefix;
  for ( i = 1; i < 5; i++ )
    segs[ i ] = subj.segment( i - 1 );

  for ( count = 5; count > 1; ) {
    if ( segs[ count - 1 ][ 0 ] != '\0' )
      break;
    count--;
  }
  if ( count > 1 ) {
    for ( i = 1; i < count - 1; i++ ) {
      /* sass can have empty segments for subjects, RV cannot */
      if ( segs[ i ][ 0 ] == '\0' ) {
        if ( isWild )
          segs[ i ] = "*";
        else
          segs[ i ] = "=0";
      }
    }
  }
  if ( isWild && count < 5 )
    segs[ count++ ] = ">";

  return this->encode( segs, count, toBuf, bufSize );
}


unsigned int
MsaSubject::queryLength( void ) const
{
  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  return 4 + (unsigned int) this->buf[ 0 ] + (unsigned int) this->buf[ 1 ] +
             (unsigned int) this->buf[ 2 ] + sizeof( QUERY );
}


unsigned int
SassSubject::queryLength( void ) const
{
  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  return 9 + (unsigned int) this->buf[ 1 ] + sizeof( QUERY ) - 1;
}


unsigned int
MsaSubject::copyTo( byte *toBuf,  unsigned int toBufLen ) const
{
  unsigned int len;

  len = this->length();
  if ( toBufLen < len )
    throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
  ::memcpy( toBuf, this->buf, len );

  return len;
}


unsigned int
SassSubject::copyTo( byte *toBuf,  unsigned int toBufLen ) const
{
  unsigned int len;

  len = this->length();
  if ( toBufLen < len )
    throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
  ::memcpy( toBuf, this->buf, len );

  return len;
}


unsigned int
RaiSubject::copyTo( byte *toBuf,  unsigned int toBufLen ) const
{
  unsigned int len;

  len = this->length();
  if ( toBufLen < len )
    throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );
  ::memcpy( toBuf, this->buf, len );

  return len;
}


unsigned int
MsaSubject::copyQueryTo( byte *toBuf,  unsigned int toBufLen ) const

{
  unsigned int len;

  len = this->queryLength();
  if ( toBufLen < len )
    throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );

  ::memcpy( toBuf, this->buf, len - sizeof( QUERY ) );
  ::memcpy( &toBuf[ len - sizeof( QUERY ) ], QUERY, sizeof( QUERY ) );
  toBuf[ 3 ] = sizeof( QUERY );

  return len;
}


unsigned int
SassSubject::copyQueryTo( byte *toBuf,  unsigned int toBufLen ) const

{
  unsigned int len;

  len = this->queryLength();
  if ( toBufLen < len )
    throw SubjectErr::getErr( SubjectErr::BUF_TOO_SMALL );

  len = (unsigned int) this->buf[ 1 ];
  toBuf[ 0 ] = 4;
  toBuf[ 1 ] = (byte) len;
  toBuf[ 2 ] = 0;
  toBuf[ 3 ] = 0;
  toBuf[ 4 ] = sizeof( QUERY ) - 1;

  ::memcpy( &toBuf[ 5 ], &this->buf[ 5 ], len + 1 );
  toBuf[ len + 6 ] = 0;
  toBuf[ len + 7 ] = 0;
  ::memcpy( &toBuf[ len + 8 ], QUERY, sizeof( QUERY ) );

  return len + 8 + sizeof( QUERY );
}


unsigned int
SassSubject::decodeQuery( char *subject,  unsigned int maxSubjLen ) const

{
  unsigned int len;
  const byte * codedName;

  if ( (codedName = this->buf) == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  len = (unsigned int) codedName[ 1 ];
  if ( len + 3 + sizeof( QUERY ) > maxSubjLen )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_TOO_LONG );

  ::memcpy( subject, &codedName[ 5 ], len );
  subject[ len++ ] = '.';
  subject[ len++ ] = '.';
  subject[ len++ ] = '.';
  ::memcpy( &subject[ len ], QUERY, sizeof( QUERY ) );
  len += sizeof( QUERY );

  return len;
}


bool
MsaSubject::equals( const MsaSubject &subj ) const
{
  if ( subj.buf == NULL || this->buf == NULL )
    return false;
  if ( this->bufLen == subj.bufLen &&
       ::memcmp( this->buf, subj.buf, this->bufLen ) == 0 )
    return true;
  return false;
}


bool
SassSubject::equals( const SassSubject &subj ) const
{
  if ( subj.buf == NULL || this->buf == NULL )
    return false;
  if ( this->bufLen == subj.bufLen &&
       ::memcmp( this->buf, subj.buf, this->bufLen ) == 0 )
    return true;
  return false;
}


bool
RaiSubject::equals( const RaiSubject &subj ) const
{
  if ( subj.buf == NULL || this->buf == NULL )
    return false;
  if ( this->bufLen == subj.bufLen &&
       ::memcmp( this->buf, subj.buf, this->bufLen ) == 0 )
    return true;
  return false;
}


bool
MsaSubject::matches( const MsaSubject &subj ) const
{
  unsigned int len,
               len2,
               off,
               off2,
               i;
  if ( subj.buf == NULL || this->buf == NULL )
    return false;
  off  = 4;
  off2 = 4;
  for ( i = 0; i < 4; i++ ) {
    len  = (unsigned int) this->buf[ i ];
    len2 = (unsigned int) subj.buf[ i ];
    if ( ( len != len2 ||
           ::memcmp( &this->buf[ off ], &subj.buf[ off2 ], len ) != 0 ) &&
         len != 0 )
      return false;
    off  += len;
    off2 += len2;
  }
  return true;
}


bool
SassSubject::matches( const SassSubject &subj ) const
{
  unsigned int len,
               len2,
               off,
               off2,
               i;

  if ( subj.buf == NULL || this->buf == NULL )
    return false;

  off  = 5;
  off2 = 5;
  for ( i = 1; i < 5; i++ ) {
    len  = (unsigned int) this->buf[ i ];
    len2 = (unsigned int) subj.buf[ i ];
    if ( ( len != len2 ||
           ::memcmp( &this->buf[ off ], &subj.buf[ off2 ], len ) != 0 ) &&
         len != 0 )
      return false;
    off  += len + 1;
    off2 += len2 + 1;
  }
  return true;
}


bool
RaiSubject::matches( const RaiSubject &subj ) const
{
  const byte * ptr,
             * ptr2;
  byte         n,
               len,
               len2;

  if ( subj.buf == NULL || this->buf == NULL )
    return false;

  /* this is the wildcard pattern, subj can be a wild or a normal subject */
  ptr  = &this->buf[ 1 ];
  ptr2 = &subj.buf[ 1 ];;

  for ( n = 0; ; n++ ) {
    /* if at the end of the pattern */
    if ( n == this->buf[ 0 ] ) {
      /* if all segments matched */
      if ( n == subj.buf[ 0 ] )
        return true;
    }
    /* if at the end of subject, didn't match all of pattern */
    if ( n == subj.buf[ 0 ] )
      return false;

    /* the lengths of the current segment */
    len  = *ptr++;
    len2 = *ptr2++;
    /* check that segment matches literally or matches a wildcard */
    if ( len == len2 ) {
      do {
        if ( *ptr != *ptr2++ ) {
          /* check case that '*' or '>' matches 1 char */
          if ( len != 3 )
            return false;
          if ( *ptr != '*' ) {
            /* if wildcard is '>' and it is at the end of the pattern */
            if ( *ptr == '>' && n + 1 == this->buf[ 0 ] )
              return true;
            return false;
          }
        }
      } while ( *++ptr != '\0' );
      /* skip over nul char */
      ptr++;
      ptr2++;
    }
    else {
      /* check case that '*' or '>' matches segment */
      if ( len != 3 )
        return false;
      if ( *ptr != '*' ) {
        /* if wildcard is '>' and it is at the end of the pattern */
        if ( *ptr == '>' && n + 1 == this->buf[ 0 ] )
          return true;
        return false;
      }
      /* skip to next segment */
      ptr  = &ptr[ 2 ];
      ptr2 = &ptr2[ len2 - 1 ];
    }
  }

  return false;
}


const char *
MsaSubject::segment( unsigned int segNum ) const
{
  unsigned int i,
               off,
               len;
  static char  nullSeg = 0;

  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  off = 4;
  for ( i = 0; i < 4; i++ ) {
    if ( segNum == i ) {
      if ( this->buf[ i ] == 0 ) {
        return &nullSeg;    // zero length seg has 0 bytes in MSA subject
      } else {
        return (const char *) &this->buf[ off ];
      }
    }
    len  = (unsigned int) this->buf[ i ];
    off += len;
  }
  throw SubjectErr::getErr( SubjectErr::BAD_SEGMENT_NUM );
}

const char *
SassSubject::segment( unsigned int segNum ) const
{
  unsigned int i,
               off,
               len;

  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  off = 5;
  for ( i = 1; i < 5; i++ ) {
    if ( segNum + 1 == i )
      return (const char *) &this->buf[ off ];
    len  = (unsigned int) this->buf[ i ];
    off += len + 1;
  }
  throw SubjectErr::getErr( SubjectErr::BAD_SEGMENT_NUM );
}


const char *
RaiSubject::segment( unsigned int segNum ) const
{
  unsigned int len;
  byte         n;

  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  len = 1;
  for ( n = 0; n < this->buf[ 0 ]; n++ ) {
    if ( segNum-- == 0 )
      return (const char *) &this->buf[ len + 1 ];
    len += (unsigned int) this->buf[ len ];
  }

  throw SubjectErr::getErr( SubjectErr::BAD_SEGMENT_NUM );
}


unsigned int
SassSubject::segmentCount( void ) const
{
  if ( this->buf == NULL )
    return 0;
  return 4;
}


unsigned int
RaiSubject::segmentCount( void ) const
{
  if ( this->buf == NULL )
    return 0;
  return (unsigned int) this->buf[ 0 ];
}


unsigned int
SassSubject::getSegments( const char **segs,  unsigned int maxSegs ) const

{
  unsigned int off;
  byte         n;

  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );
  if ( maxSegs < 4 )
    throw SubjectErr::getErr( SubjectErr::MAX_SEG_OVERFLOW );

  off = 5;
  for ( n = 0; ; ) {
    segs[ n ] = (const char *) &this->buf[ off ];
    if ( ++n == 4 )
      break;
    off += (unsigned int) this->buf[ n ] + 1;
  }

  return 4;
}


unsigned int
SassSubject::getSegments( const char **segs,  unsigned int *lens,
                          unsigned int maxSegs ) const
{
  unsigned int off,
               len;
  byte         n;

  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );
  if ( maxSegs < 4 )
    throw SubjectErr::getErr( SubjectErr::MAX_SEG_OVERFLOW );

  off = 5;
  for ( n = 0; ; ) {
    segs[ n ] = (const char *) &this->buf[ off ];
    len       = (unsigned int) this->buf[ n + 1 ];
    lens[ n ] = len;
    if ( ++n == 4 )
      break;
    off += len + 1;
  }

  return 4;
}


unsigned int
RaiSubject::getSegments( const char **segs,  unsigned int maxSegs ) const

{
  unsigned int len,
               n;

  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );
  if ( maxSegs < (unsigned int) this->buf[ 0 ] )
    throw SubjectErr::getErr( SubjectErr::MAX_SEG_OVERFLOW );

  maxSegs = (unsigned int) this->buf[ 0 ];
  len     = 1;
  for ( n = 0; ; ) {
    segs[ n ] = (const char *) &this->buf[ len + 1 ];
    if ( ++n == maxSegs )
      break;
    len += (unsigned int) this->buf[ len ];
  }

  return n;
}


unsigned int
RaiSubject::getSegments( const char **segs,  unsigned int *lens,
                        unsigned int maxSegs ) const
{
  unsigned int len,
               n;

  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );
  if ( maxSegs < (unsigned int) this->buf[ 0 ] )
    throw SubjectErr::getErr( SubjectErr::MAX_SEG_OVERFLOW );

  maxSegs = (unsigned int) this->buf[ 0 ];
  len     = 1;
  for ( n = 0; ; ) {
    segs[ n ] = (const char *) &this->buf[ len + 1 ];
    lens[ n ] = (unsigned int) this->buf[ len ] - 2;
    if ( ++n == maxSegs )
      break;
    len += (unsigned int) this->buf[ len ];
  }

  return n;
}


unsigned int
Subject::hashSegment( unsigned int mask,  unsigned int numHashBits,
                      const byte *segment,  unsigned int segmentLen )
{
  unsigned int shift,
               hash,
               bitsPerChar,
               charMask;

  if ( segmentLen == 0 ) /* wildcard */
    return mask;

  hash        = 0;
  bitsPerChar = ( numHashBits + segmentLen - 1 ) / segmentLen;
  charMask    = ( 1 << bitsPerChar ) - 1;

  shift = 0;
  do {
    hash  |= ( *segment++ & charMask ) << shift;
    shift += bitsPerChar;
  } while ( --segmentLen > 0 && shift < numHashBits );

  if ( (hash &= mask ) == mask ) /* can't use all bits, that's a wildcard */
    hash = 0;
  return hash;
}


unsigned int
MsaSubject::hash( void ) const
{
  unsigned int seg,
               hash,
               len,
               off,
               i;

  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  off  = 4;
  hash = 0;
  for ( i = 0; i < 4; i++ ) {
    len   = (unsigned int) this->buf[ i ];
    seg   = Subject::hashSegment( msaBits.mask[ i ], msaBits.bits[ i ],
                                  &this->buf[ off ], len > 0 ? len - 1 : 0 );
    hash |= seg << msaBits.shift[ i ];
    off  += len;
  }
  return hash;
}


unsigned int
SassSubject::hash( void ) const
{
  unsigned int seg,
               hash,
               len,
               off,
               i;

  if ( this->buf == NULL )
    throw SubjectErr::getErr( SubjectErr::SUBJECT_IS_NULL );

  off  = 5;
  hash = 0;
  for ( i = 1; i < 5; i++ ) {
    len   = (unsigned int) this->buf[ i ];
    seg   = Subject::hashSegment( sassBits.mask[ i-1 ], sassBits.bits[ i-1 ],
                                  &this->buf[ off ], len );
    hash |= seg << sassBits.shift[ i-1 ];
    off  += len + 1;
  }
  return hash;
}


unsigned int
MsaSubject::hashQuery( unsigned int hash )
{
  static unsigned int msaQuerySeg;
  unsigned int        seg;

  if ( msaQuerySeg == 0 ) {
    seg = Subject::hashSegment( msaBits.mask[ 3 ], msaBits.bits[ 3 ],
                                QUERY, sizeof( QUERY ) - 1 );
    seg <<= msaBits.shift[ 3 ];
    msaQuerySeg = seg;
  }
  hash &= ~( msaBits.mask[ 3 ] << msaBits.shift[ 3 ] );
  hash |= msaQuerySeg;

  return hash;
}


unsigned int
SassSubject::hashQuery( unsigned int hash )
{
  static unsigned int sassQuerySeg;
  unsigned int        seg;

  if ( sassQuerySeg == 0 ) {
    seg   = Subject::hashSegment( sassBits.mask[ 3 ], sassBits.bits[ 3 ],
                                  QUERY, sizeof( QUERY ) - 1 );
    seg <<= sassBits.shift[ 3 ];
    sassQuerySeg = seg;
  }
  hash &= ~( sassBits.mask[ 3 ] << sassBits.shift[ 3 ] );
  hash |= sassQuerySeg | ( sassBits.mask[ 1 ] << sassBits.shift[ 1 ] ) |
                         ( sassBits.mask[ 2 ] << sassBits.shift[ 2 ] );
  return hash;
}

bool
MsaSubject::hashMatches( unsigned int hash1, unsigned int hash2 )
{
  unsigned int mask;

  if( hash1 == hash2 ) {
    return true;
  }

  for( int i = 0; i < 4; i++ ) {
    mask = msaBits.mask[ i ] << msaBits.shift[ i ];
    if( ( mask & hash1 ) == mask ) {
      continue; // this segment is a wild card
    }
    if( ( hash1 & mask ) != ( hash2 & mask ) ) {
      return false; // segments don't match
    }
  }
  return true;  // hash1 contains hash2
}

int
MsaSubject::hashWildParts( unsigned int hash ) 
{
  int           numWildParts = 0;
  unsigned int  mask;

  for( int i = 0; i < 4; i++ ) {
    mask = msaBits.mask[ i ] << msaBits.shift[ i ];
    if( ( mask & hash ) == mask ) {
      numWildParts++; // this segment is a wild card
    }
  }
  return numWildParts;
}

unsigned int
MsaSubject::hashSegMask( int seg ) 
{
  return msaBits.mask[ seg ] << msaBits.shift[ seg ];
}

Error
SubjectErr::getErr( unsigned int status )
{
  static const char     mod[] = "Subject";
  static const ErrorRec err[] = {
  /*  0 */ { SUBJECT_TOO_LONG, "Subject name too long for buffer space", mod },
  /*  1 */ { BUF_TOO_SMALL,    "Buf too small for subject copy", mod },
  /*  2 */ { SUBJECT_IS_NULL,  "Null subject buffer dereferenced", mod },
  /*  3 */ { BAD_SEGMENT_NUM,  "Segment number is out of range", mod },
  /*  4 */ { LABEL_TOO_BIG,    "Segment is longer than 255 bytes", mod },
  /*  5 */ { RV_EMPTY_SEG,     "RaiSubject segment cannot be empty (ex: ..)", mod },
  /*  6 */ { RV_TOO_MANY_SEGS, "RaiSubject segment count is more than 255", mod },
  /*  7 */ { MAX_SEG_OVERFLOW, "Segment count larger than buffer space", mod },
  /*  8 */ { BAD_RIC,          "Can't convert subject to RIC", mod },
  /*  9 */ { CARET_IN_RIC,     "Can't convert RIC to subject, "
                               "caret (^) not allowed in RIC", mod },
  /* 10 */ { 10,               "Unknown subject error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}
