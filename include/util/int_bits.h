/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__int_bits_h__
#define __rai_util__int_bits_h__

#include <string.h>

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif


#if defined( __ICC ) && __ICC == 600
  /* disable: controlling expression is constant */
  #pragma warning(disable:279)
#endif

#define RAI_FLIP_ENDIAN( i ) \
      ( ( ( ( i ) << 24 ) | ( ( i ) >> 8 ) ) & 0xff00ff00U ) | \
        ( ( ( ( i ) << 8 ) | ( ( i ) >> 24 ) ) & 0x00ff00ffU )
namespace rai {
namespace Aligned {
#if defined( _WIN32 ) || defined( _WIN64 ) || \
    defined( __i386 ) || defined( __amd64__ )
  static const bool isLittleEndian = true;
#elif defined( __sparc )
  static const bool isLittleEndian = false;
#elif defined( __arm__ )
  static const bool isLittleEndian = ( __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ );
#else
  static const int endianTest = 1;
  static const bool isLittleEndian = ( *(byte *) &endianTest == 1 );
#endif

  static inline void swap( unsigned int &i ) {
#if ( defined( __i386 ) || defined( __amd64__ ) ) && \
    ( defined( __GNUC__ ) || defined( __ICC ) )
    register unsigned int j = i;
    __asm__ __volatile__ ( "bswap %0" : "=r" (j) : "0" (j) );
    i = j;
#elif defined( _MSC_VER ) && ! defined( _WIN64 )
    register unsigned int j = i;
    __asm {
          mov   eax, j
          bswap	eax
          mov   j, eax
    }
    i = j;
#else
    i = RAI_FLIP_ENDIAN( i );
#endif
  }
  static inline void swap( ullong &i ) {
#if ( defined( __amd64__ ) && ( defined( __GNUC__ ) || defined( __ICC ) ) )
    register ullong j = i;
    __asm__ __volatile__ ( "bswap %0" : "=r" (j) : "0" (j) );
    i = j;
#else
    unsigned int hi = (unsigned int) ( i >> 32 ),
                 lo = (unsigned int) i;
    swap( hi );
    swap( lo );
    i = ( (ullong) lo << 32 ) | (ullong) hi;
#endif
  }
  static inline void swap( double &f ) {
    ullong u;
    ::memcpy( &u, &f, sizeof( f ) );
    Aligned::swap( u );
    ::memcpy( &f, &u, sizeof( f ) );
  }
  static inline void swap( float &f ) {
    unsigned int u;
    ::memcpy( &u, &f, sizeof( f ) );
    Aligned::swap( u );
    ::memcpy( &f, &u, sizeof( f ) );
  }
  static inline void swap( unsigned short &i ) {
    i = (unsigned short) ( i << 8 ) | (unsigned short) ( i >> 8 );
  }
  static inline void swap( byte & ) {}
  static inline void swap( int &i ) {
    Aligned::swap( *(unsigned int *) &i );
  }
  static inline void swap( llong &i ) {
    Aligned::swap( *(ullong *) &i );
  }
  static inline void swap( short &i ) {
    Aligned::swap( *(unsigned short *) &i );
  }
  static inline void swap( char & ) {}
  static inline void swap( bool & ) {}

  static inline void endianSwap( unsigned int &i ) {
    if ( isLittleEndian )
      Aligned::swap( i );
  }
  static inline void endianSwap( ullong &i ) {
    if ( isLittleEndian )
      Aligned::swap( i );
  }
  static inline void endianSwap( double &f ) {
    if ( isLittleEndian )
      Aligned::swap( f );
  }
  static inline void endianSwap( float &f ) {
    if ( isLittleEndian )
      Aligned::swap( f );
  }
  static inline void endianSwap( unsigned short &i ) {
    if ( isLittleEndian )
      Aligned::swap( i );
  }
  static inline void endianSwap( byte & ) {}
  static inline void endianSwap( int &i ) {
    if ( isLittleEndian )
      Aligned::swap( i );
  }
  static inline void endianSwap( llong &i ) {
    if ( isLittleEndian )
      Aligned::swap( i );
  }
  static inline void endianSwap( short &i ) {
    if ( isLittleEndian )
      Aligned::swap( i );
  }
  static inline void endianSwap( char & ) {}
  static inline void endianSwap( bool & ) {}

  static inline void endianGetInt( const byte *b,  ullong &i ) {
    i = *(ullong *) b;
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  unsigned int &i ) {
    i = *(unsigned int *) b;
    Aligned::endianSwap( i );
  }
  static inline void endianGetFloat( const byte *b,  double &f ) {
    f = *(double *) b;
    Aligned::endianSwap( f );
  }
  static inline void endianGetFloat( const byte *b,  float &f ) {
    f = *(float *) b;
    Aligned::endianSwap( f );
  }
  static inline void endianGetInt( const byte *b,  unsigned short &i ) {
    i = *(unsigned short *) b;
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  byte &i ) {
    i = b[ 0 ];
  }
  static inline void endianGetInt( const byte *b,  llong &i ) {
    i = *(llong *) b;
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  int &i ) {
    i = *(int *) b;
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  short &i ) {
    i = *(short *) b;
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  char &i ) {
    i = (char) b[ 0 ];
  }
  static inline void endianGetInt( const byte *b,  bool &i ) {
    i = ( b[ 0 ] ? true : false );
  }

  static inline void endianPutInt( ullong i,  byte *b ) {
    Aligned::endianSwap( i );
    *(ullong *) b = i;
  }
  static inline void endianPutInt( unsigned int i,  byte *b ) {
    Aligned::endianSwap( i );
    *(unsigned int *) b = i;
  }
  static inline void endianPutFloat( double f,  byte *b ) {
    Aligned::endianSwap( f );
    *(double *) b = f;
  }
  static inline void endianPutFloat( float f,  byte *b ) {
    Aligned::endianSwap( f );
    *(float *) b = f;
  }
  static inline void endianPutInt( unsigned short i,  byte *b ) {
    Aligned::endianSwap( i );
    *(unsigned short *) b = i;
  }
  static inline void endianPutInt( byte i,  byte *b ) {
    b[ 0 ] = i;
  }
  static inline void endianPutInt( llong i,  byte *b ) {
    Aligned::endianSwap( i );
    *(llong *) b = i;
  }
  static inline void endianPutInt( int i,  byte *b ) {
    Aligned::endianSwap( i );
    *(int *) b = i;
  }
  static inline void endianPutInt( short i,  byte *b ) {
    Aligned::endianSwap( i );
    *(short *) b = i;
  }
  static inline void endianPutInt( char i,  byte *b ) {
    b[ 0 ] = (byte) i;
  }
  static inline void endianPutInt( bool i,  byte *b ) {
    b[ 0 ] = (byte) ( i ? 1 : 0 );
  }

  static inline void getInt( const byte *b,  ullong &i ) {
    i = *(ullong *) b;
  }
  static inline void getInt( const byte *b,  unsigned int &i ) {
    i = *(unsigned int *) b;
  }
  static inline void getFloat( const byte *b,  double &f ) {
    f = *(double *) b;
  }
  static inline void getFloat( const byte *b,  float &f ) {
    f = *(float *) b;
  }
  static inline void getInt( const byte *b,  unsigned short &i ) {
    i = *(unsigned short *) b;
  }
  static inline void getInt( const byte *b,  byte &i ) {
    i = b[ 0 ];
  }
  static inline void getInt( const byte *b,  llong &i ) {
    i = *(llong *) b;
  }
  static inline void getInt( const byte *b,  int &i ) {
    i = *(int *) b;
  }
  static inline void getInt( const byte *b,  short &i ) {
    i = *(short *) b;
  }
  static inline void getInt( const byte *b,  char &i ) {
    i = (char) b[ 0 ];
  }
  static inline void getInt( const byte *b,  bool &i ) {
    i = ( b[ 0 ] ? true : false );
  }

  static inline void putInt( ullong i,  byte *b ) {
    *(ullong *) b = i;
  }
  static inline void putInt( unsigned int i,  byte *b ) {
    *(unsigned int *) b = i;
  }
  static inline void putFloat( double f,  byte *b ) {
    *(double *) b = f;
  }
  static inline void putFloat( float f,  byte *b ) {
    *(float *) b = f;
  }
  static inline void putInt( unsigned short i,  byte *b ) {
    *(unsigned short *) b = i;
  }
  static inline void putInt( byte i,  byte *b ) {
    b[ 0 ] = i;
  }
  static inline void putInt( llong i,  byte *b ) {
    *(llong *) b = i;
  }
  static inline void putInt( int i,  byte *b ) {
    *(int *) b = i;
  }
  static inline void putInt( short i,  byte *b ) {
    *(short *) b = i;
  }
  static inline void putInt( char i,  byte *b ) {
    b[ 0 ] = (byte) i;
  }
  static inline void putInt( bool i,  byte *b ) {
    b[ 0 ] = (byte) ( i ? 1 : 0 );
  }
}


namespace Unaligned {
  static inline void endianGetInt( const byte *b,  unsigned int &i ) {
    ::memcpy( &i, b, sizeof( unsigned int ) );
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  ullong &i ) {
    ::memcpy( &i, b, sizeof( ullong ) );
    Aligned::endianSwap( i );
  }
  static inline void endianGetFloat( const byte *b,  float &f ) {
    ::memcpy( &f, b, sizeof( float ) );
    Aligned::endianSwap( f );
  }
  static inline void endianGetFloat( const byte *b,  double &f ) {
    ::memcpy( &f, b, sizeof( double ) );
    Aligned::endianSwap( f );
  }
  static inline void endianGetInt( const byte *b,  unsigned short &i ) {
    ::memcpy( &i, b, sizeof( unsigned short ) );
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  byte &i ) {
    i = b[ 0 ];
  }
  static inline void endianGetInt( const byte *b,  int &i ) {
    ::memcpy( &i, b, sizeof( int ) );
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  llong &i ) {
    ::memcpy( &i, b, sizeof( llong ) );
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  short &i ) {
    ::memcpy( &i, b, sizeof( short ) );
    Aligned::endianSwap( i );
  }
  static inline void endianGetInt( const byte *b,  char &i ) {
    i = (char) b[ 0 ];
  }
  static inline void endianGetInt( const byte *b,  bool &i ) {
    i = ( b[ 0 ] ? true : false );
  }

  static inline void endianPutInt( ullong i,  byte *b ) {
    Aligned::endianSwap( i );
    ::memcpy( b, &i, sizeof( ullong ) );
  }
  static inline void endianPutInt( unsigned int i,  byte *b ) {
    Aligned::endianSwap( i );
    ::memcpy( b, &i, sizeof( unsigned int ) );
  }
  static inline void endianPutFloat( double f,  byte *b ) {
    Aligned::endianSwap( f );
    ::memcpy( b, &f, sizeof( double ) );
  }
  static inline void endianPutFloat( float f,  byte *b ) {
    Aligned::endianSwap( f );
    ::memcpy( b, &f, sizeof( float ) );
  }
  static inline void endianPutInt( unsigned short i,  byte *b ) {
    Aligned::endianSwap( i );
    ::memcpy( b, &i, sizeof( unsigned short ) );
  }
  static inline void endianPutInt( byte i,  byte *b ) {
    b[ 0 ] = i;
  }
  static inline void endianPutInt( llong i,  byte *b ) {
    Aligned::endianSwap( i );
    ::memcpy( b, &i, sizeof( llong ) );
  }
  static inline void endianPutInt( int i,  byte *b ) {
    Aligned::endianSwap( i );
    ::memcpy( b, &i, sizeof( int ) );
  }
  static inline void endianPutInt( short i,  byte *b ) {
    Aligned::endianSwap( i );
    ::memcpy( b, &i, sizeof( short ) );
  }
  static inline void endianPutInt( char i,  byte *b ) {
    b[ 0 ] = (byte) i;
  }
  static inline void endianPutInt( bool i,  byte *b ) {
    b[ 0 ] = (byte) ( i ? 1 : 0 );
  }

  static inline void getInt( const byte *b,  unsigned int &i ) {
    ::memcpy( &i, b, sizeof( unsigned int ) );
  }
  static inline void getInt( const byte *b,  ullong &i ) {
    ::memcpy( &i, b, sizeof( ullong ) );
  }
  static inline void getFloat( const byte *b,  float &f ) {
    ::memcpy( &f, b, sizeof( float ) );
  }
  static inline void getFloat( const byte *b,  double &f ) {
    ::memcpy( &f, b, sizeof( double ) );
  }
  static inline void getInt( const byte *b,  unsigned short &i ) {
    ::memcpy( &i, b, sizeof( unsigned short ) );
  }
  static inline void getInt( const byte *b,  byte &i ) {
    i = b[ 0 ];
  }
  static inline void getInt( const byte *b,  int &i ) {
    ::memcpy( &i, b, sizeof( int ) );
  }
  static inline void getInt( const byte *b,  llong &i ) {
    ::memcpy( &i, b, sizeof( llong ) );
  }
  static inline void getInt( const byte *b,  short &i ) {
    ::memcpy( &i, b, sizeof( short ) );
  }
  static inline void getInt( const byte *b,  char &i ) {
    i = (char) b[ 0 ];
  }
  static inline void getInt( const byte *b,  bool &i ) {
    i = ( b[ 0 ] ? true : false );
  }

  static inline void putInt( ullong i,  byte *b ) {
    ::memcpy( b, &i, sizeof( ullong ) );
  }
  static inline void putInt( unsigned int i,  byte *b ) {
    ::memcpy( b, &i, sizeof( unsigned int ) );
  }
  static inline void putFloat( double f,  byte *b ) {
    ::memcpy( b, &f, sizeof( double ) );
  }
  static inline void putFloat( float f,  byte *b ) {
    ::memcpy( b, &f, sizeof( float ) );
  }
  static inline void putInt( unsigned short i,  byte *b ) {
    ::memcpy( b, &i, sizeof( unsigned short ) );
  }
  static inline void putInt( byte i,  byte *b ) {
    b[ 0 ] = i;
  }
  static inline void putInt( llong i,  byte *b ) {
    ::memcpy( b, &i, sizeof( llong ) );
  }
  static inline void putInt( int i,  byte *b ) {
    ::memcpy( b, &i, sizeof( int ) );
  }
  static inline void putInt( short i,  byte *b ) {
    ::memcpy( b, &i, sizeof( short ) );
  }
  static inline void putInt( char i,  byte *b ) {
    b[ 0 ] = (byte) i;
  }
  static inline void putInt( bool i,  byte *b ) {
    b[ 0 ] = ( i ? 1 : 0 );
  }

  static inline unsigned int encode( byte *b,  byte i ) {
    unsigned int len = 0;
    if ( i > 0x7fU ) {
      b[ len++ ] = (byte) ( i & 0x7fU );
      i >>= 7;
    }
    b[ len++ ] = (byte) ( i | 0x80U );
    return len;
  }
  static inline unsigned int encode( byte *b,  unsigned short i ) {
    unsigned int len = 0;
    do {
      b[ len++ ] = (byte) ( i & 0x7fU );
      i >>= 7;
    } while ( i != 0 );
    b[ len - 1 ] |= 0x80U;
    return len;
  }
  static inline unsigned int encode( byte *b,  unsigned int i ) {
    unsigned int len = 0;
    do {
      b[ len++ ] = (byte) ( i & 0x7fU );
      i >>= 7;
    } while ( i != 0 );
    b[ len - 1 ] |= 0x80U;
    return len;
  }
  static inline unsigned int encode( byte *b,  unsigned long i ) {
    unsigned int len = 0;
    do {
      b[ len++ ] = (byte) ( i & 0x7fU );
      i >>= 7;
    } while ( i != 0 );
    b[ len - 1 ] |= 0x80U;
    return len;
  }
  static inline unsigned int encode( byte *b,  ullong i ) {
    unsigned int len;
    for ( len = 0; i > (ullong) 0xffffffffU; i >>= 7 )
      b[ len++ ] = (byte) ( i & 0x7fU );
    return len + encode( &b[ len ], (unsigned int) i );
  }
  static inline unsigned int encode( byte *b,  short i ) {
    if ( i < 0 )
      return encode( b, (unsigned short) ( ( ( (unsigned short) -i ) << 1 ) |
                                               (unsigned short) 1U ) );
    return encode( b, (unsigned short) ( ( ( (unsigned short) i ) << 1 ) |
                                             (unsigned short) 0U ) );
  }
  static inline unsigned int encode( byte *b,  int i ) {
    if ( i < 0 )
      return encode( b, (unsigned int) ( ( ( (unsigned int) -i ) << 1 )| 1U ) );
    return encode( b, (unsigned int) ( ( ( (unsigned int) i ) << 1 ) | 0U ) );
  }
  static inline unsigned int encode( byte *b,  llong i ) {
    if ( i < 0 )
      return encode( b, (ullong) ( ( ( (ullong) -i ) << 1 ) | (ullong) 1U ) );
    return encode( b, (ullong) ( ( ( (ullong) i ) << 1 ) | (ullong) 0U ) );
  }
  static inline unsigned int encode( byte *b,  char i ) {
    if ( i < 0 )
      return encode( b, (byte) ( ( ( (byte) -i ) << 1 ) | (byte) 1U ) );
    return encode( b, (byte) ( ( ( (byte) i ) << 1 ) | (byte) 0U ) );
  }
  static inline unsigned int encode( byte *b,  double d ) {
    ullong x;
    ::memcpy( &x, &d, sizeof( x ) );
    Aligned::swap( x );
    return encode( b, x );
  }

  static inline unsigned int decode( const byte *b,  unsigned short &i ) {
    i = (unsigned short) b[ 0 ];
    if ( ( i & 0x80U ) != 0 ) {
      i &= 0x7fU;
      return 1;
    }
    i |= (unsigned short) b[ 1 ] << 7;
    if ( ( i & 0x4000U ) != 0 ) {
      i &= 0x3fffU;
      return 2;
    }
    i |= ( (unsigned short) b[ 2 ] & 0x7f ) << 14;
    return 3;
  }
  static inline unsigned int decode( const byte *b,  unsigned int &i ) {
    unsigned int len   = 0,
                 shift = 0;
    i = (unsigned int) ( b[ 0 ] & 0x7fU );
    while ( ( b[ len++ ] & 0x80U ) == 0 ) {
      shift += 7;
      i     |= (unsigned int) ( b[ len ] & 0x7fU ) << shift;
    }
    return len;
  }
  static inline unsigned int decode( const byte *b,  unsigned long &i ) {
    unsigned int len   = 0,
                 shift = 0;
    i = (unsigned long) ( b[ 0 ] & 0x7fU );
    while ( ( b[ len++ ] & 0x80U ) == 0 ) {
      shift += 7;
      i     |= (unsigned long) ( b[ len ] & 0x7fU ) << shift;
    }
    return len;
  }
  static inline unsigned int decode( const byte *b,  ullong &i ) {
    unsigned int len   = 0,
                 shift = 0;
    i = (ullong) ( b[ 0 ] & 0x7fU );
    while ( ( b[ len++ ] & 0x80U ) == 0 ) {
      shift += 7;
      i     |= (ullong) ( b[ len ] & 0x7fU ) << shift;
    }
    return len;
  }
  static inline unsigned int decode( const byte *b,  byte &i ) {
    if ( b[ 0 ] <= 0x7fU ) {
      i = b[ 0 ] | (byte) ( ( b[ 1 ] & 0x7fU ) << 7 );
      return 2;
    }
    i = (byte) ( b[ 0 ] & 0x7fU );
    return 1;
  }
  static inline unsigned int decode( const byte *b,  short &i ) {
    unsigned int   len;
    unsigned short j;
    len = decode( b, j );
    i   = ( j & 1 ) ? -(short) ( j >> 1 ) : (short) ( j >> 1 );
    return len;
  }
  static inline unsigned int decode( const byte *b,  int &i ) {
    unsigned int len;
    unsigned int j;
    len = decode( b, j );
    i   = ( j & 1 ) ? -(int) ( j >> 1 ) : (int) ( j >> 1 );
    return len;
  }
  static inline unsigned int decode( const byte *b,  llong &i ) {
    unsigned int len;
    ullong       j;
    len = decode( b, j );
    i   = ( j & 1 ) ? -(llong) ( j >> 1 ) : (llong) ( j >> 1 );
    return len;
  }
  static inline unsigned int decode( const byte *b,  char &i ) {
    unsigned int len;
    byte         j;
    len = decode( b, j );
    i   = ( j & 1 ) ? -(char) ( j >> 1 ) : (char) ( j >> 1 );
    return len;
  }
  static inline unsigned int decode( byte *b,  double &d ) {
    ullong x;
    unsigned int len = decode( b, x );
    Aligned::swap( x );
    ::memcpy( &d, &x, sizeof( d ) );
    return len;
  }

  static inline unsigned int codeLen( unsigned short i ) {
    return ( i <= 0x7fU ) ? 1 : ( ( i <= 0x3fffU ) ? 2 : 3 );
  }
  static inline unsigned int codeLen( unsigned int i ) {
    return ( i <= 0x7fU ) ? 1 : ( ( i <= 0x3fffU ) ? 2 :
             ( ( i <= 0x1fffffU ) ? 3 : ( ( i <= 0xfffffffU ) ? 4 : 5 ) ) );
  }
  static inline unsigned int codeLen( unsigned long i ) {
    if ( sizeof( unsigned int ) == sizeof( unsigned long ) )
      return codeLen( (unsigned int) i );
    else {
      for ( unsigned int len = 0; ; len++, i >>= 7 )
        if ( i <= (unsigned long) 0xffffffffU )
          return len + codeLen( (unsigned int) i );
    }
  }
  static inline unsigned int codeLen( ullong i ) {
    for ( unsigned int len = 0; ; len++, i >>= 7 )
      if ( i <= (ullong) 0xffffffffU )
        return len + codeLen( (unsigned int) i );
  }
  static inline unsigned int codeLen( byte i ) {
    return ( i <= 0x7fU ) ? 1 : 2;
  }
  static inline unsigned int codeLen( short i ) {
    if ( i < 0 )
      i = -i;
    return codeLen( (unsigned short) ( ( (unsigned short) i ) << 1 ) );
  }
  static inline unsigned int codeLen( int i ) {
    if ( i < 0 )
      i = -i;
    return codeLen( (unsigned int) ( ( (unsigned int) i ) << 1 ) );
  }
  static inline unsigned int codeLen( llong i ) {
    if ( i < 0 )
      i = -i;
    return codeLen( (ullong) ( ( (ullong) i ) << 1 ) );
  }
  static inline unsigned int codeLen( char i ) {
    if ( i < 0 )
      i = -i;
    return codeLen( (byte) ( ( (byte) i ) << 1 ) );
  }
  static inline unsigned int codeLen( double d ) {
    ullong x;
    ::memcpy( &x, &d, sizeof( x ) );
    Aligned::swap( x );
    return codeLen( x );
  }

  /* for coding bits with lots of zeros */
  static inline unsigned int codeLen( const byte *bits,  unsigned int bitLen ) {
    unsigned int i, k = 0;

    while ( bitLen >= 64 ) {
      k++;
      for ( i = 0; i < 8; i++ ) {
        if ( bits[ i ] != 0 )
          k++;
      }
      bits = &bits[ 8 ];
      bitLen -= 64;
    }
    if ( bitLen > 0 ) {
      k++;
      for ( i = 0; bitLen >= 8; i++ ) {
        if ( bits[ i ] != 0 )
          k++;
        bitLen -= 8;
      }
      if ( bitLen > 0 ) {
        byte mask = ( 1U << bitLen ) - 1;
        if ( ( bits[ i ] & mask ) != 0 )
          k++;
      }
    }
    return k;
  }
  static inline unsigned int encode( const byte *bits,  unsigned int bitLen,
                                     byte *out ) {
    byte * o = out;
    unsigned int i, j;
    /* if a byte is non-zero, set a bit and output the byte */
    while ( bitLen >= 64 ) {
      out[ 0 ] = 0;
      j = 1;
      for ( i = 0; i < 8; i++ ) {
        if ( bits[ i ] != 0 ) {
          out[ 0 ] |= 1 << i;
          out[ j++ ] = bits[ i ];
        }
      }
      bits = &bits[ 8 ];
      out  = &out[ j ];
      bitLen -= 64;
    }
    j = 0;
    if ( bitLen > 0 ) {
      out[ 0 ] = 0;
      j = 1;
      for ( i = 0; bitLen >= 8; i++ ) {
        if ( bits[ i ] != 0 ) {
          out[ 0 ] |= 1 << i;
          out[ j++ ] = bits[ i ];
        }
        bitLen -= 8;
      }
      if ( bitLen > 0 ) {
        byte mask = ( 1U << bitLen ) - 1;
        if ( ( bits[ i ] & mask ) != 0 ) {
          out[ 0 ] |= 1 << i;
          out[ j++ ] = ( bits[ i ] & mask );
        }
      }
    }
    return (unsigned int) ( &out[ j ] - o );
  }
  static inline unsigned int decode( const byte *buf,  unsigned int bitLen,
                                     byte *out ) {
    const byte * b = buf;
    unsigned int i, j, k;
    /* if a bit is set, output the byte otherwise output zero */
    while ( bitLen >= 64 ) {
      j = 1;
      for ( i = 0; i < 8; i++ ) {
        if ( ( buf[ 0 ] & ( 1 << i ) ) != 0 )
          out[ i ] = buf[ j++ ];
        else
          out[ i ] = 0;
        k++;
      }
      buf  = &buf[ j ];
      out  = &out[ 8 ];
      bitLen -= 64;
    }
    j = 0;
    if ( bitLen > 0 ) {
      j = 1;
      k = 0;
      for ( i = 0; bitLen >= 8; i++ ) {
        if ( ( buf[ 0 ] & ( 1 << i ) ) != 0 )
          out[ k ] = buf[ j++ ];
        else
          out[ k ] = 0;
        k++;
        bitLen -= 8;
      }
      if ( bitLen > 0 ) {
        if ( ( buf[ 0 ] & ( 1 << i ) ) != 0 )
          out[ k ] = buf[ j++ ];
        else
          out[ k ] = 0;
        k++;
      }
    }
    return (unsigned int) ( &buf[ j ] - b );
  }
}
} // namespace rai

#if 0
int
main( int argc, char *argv[] )
{
  byte c[ 16 ], d[ 16 ];
  byte x[] = { 0, 80, 128, 0, 0, 0, 0, 255, 0, 0, 0, 15 };
  unsigned int i, j, n;

  n = sizeof( x ) * 8 - 4;
  printf( "codeLen = %u\n", Unaligned::codeLen( x, n ) );
  for ( i = 0; i < sizeof( x ); i++ )
    printf( "%02x ", x[ i ] );
  printf( "\nencode = %u\n", j = Unaligned::encode( x, n, c ) );
  for ( i = 0; i < j; i++ )
    printf( "%02x ", c[ i ] );
  printf( "\ndecode = %u\n", j = Unaligned::decode( c, n, d ) );
  for ( i = 0; i < sizeof( x ); i++ )
    printf( "%02x ", d[ i ] );
  printf( "\n" );

  return 0;
}
#endif

#endif
