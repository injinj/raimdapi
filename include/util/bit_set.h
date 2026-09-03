/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__bit_set_h__
#define __rai_util__bit_set_h__

#ifndef __rai_base__types_h__
#include "base/types.h" /* for CONST_DEF */
#endif

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
#include <intrin.h>
#endif

namespace rai {
template <class WORD>
struct VecOps {
  static const unsigned int WD_SHFT = sizeof( WORD ) == 4 ? 5 :
                                      sizeof( WORD ) == 8 ? 6 :
                                      sizeof( WORD ) == 2 ? 4 : 3;
  static const unsigned int WD_BITS = 8 * sizeof( WORD );
  static const unsigned int WD_MASK = ( 1U << WD_SHFT ) - 1;
  static const WORD         WD_SET  = ~((WORD) 0 );

#if defined( _WIN32 ) || defined( _WIN64 )
  static inline int ctzl( ullong val ) {
    unsigned long z;
    if ( _BitScanForward64( &z, val ) )
      return (int) z;
    return 64;
  }
  static inline int ctzw( unsigned int val ) {
    unsigned long z;
    if ( _BitScanForward( &z, val ) )
      return (int) z;
    return 32;
  }
  static inline int ffsl( ullong val ) { return val == 0 ? 0 : ( ctzl( val ) + 1 ); }
  static inline int ffsw( unsigned int val ) { return val == 0 ? 0 : ( ctzw( val ) + 1 ); }
  static inline int popcountl( ullong val ) { return __popcnt64( val ); }
  static inline int popcountw( unsigned int val ) { return __popcnt( val ); }
#else
  static inline int ffsl( ullong val ) { return __builtin_ffsl( val ); }
  static inline int ffsw( unsigned int val ) { return __builtin_ffs( val ); }
  static inline int popcountl( ullong val ) { return __builtin_popcountl( val ); }
  static inline int popcountw( unsigned int val ) { return __builtin_popcount( val ); }
#endif

  static void setAll( WORD *val,  unsigned int nbits ) {
    for ( unsigned int i = 0; ; i++ ) {
      WORD v = WD_SET;
      if ( nbits < WD_BITS ) {
        if ( nbits == 0 )
          break;
        v &= ( (WORD) 1U << nbits ) - 1;
      }
      val[ i ] = v;
      if ( nbits < WD_BITS )
        break;
      nbits -= WD_BITS;
    }
  }
  static void clearAll( WORD *val,  unsigned int sz ) {
    if ( sz != 0 ) {
      *val++ = 0;
      while ( --sz ) *val++ = 0;
    }
  }
  static void flipAll( WORD *val,  unsigned int nbits ) {
    for ( unsigned int i = 0; ; i++ ) {
      WORD v = WD_SET;
      if ( nbits < WD_BITS ) {
        if ( nbits == 0 )
          break;
        v &= ( (WORD) 1U << nbits ) - 1;
      }
      val[ i ] ^= v;
      if ( nbits < WD_BITS )
        break;
      nbits -= WD_BITS;
    }
  }
  static void set( WORD *val,  unsigned int nbits,  unsigned int shft ) {
    if ( shft < nbits )
      val[ shft >> WD_SHFT ] |= (WORD) 1U << ( shft & WD_MASK );
  }
  static void unSet( WORD *val,  unsigned int nbits,  unsigned int shft ) {
    if ( shft < nbits )
      val[ shft >> WD_SHFT ] &= ~( (WORD) 1U << ( shft & WD_MASK ) );
  }
  static bool isSet( const WORD *val,  unsigned int nbits, unsigned int shft ) {
    if ( shft < nbits )
      return ( val[ shft >> WD_SHFT ] &
               ( (WORD) 1U << ( shft & WD_MASK ) ) ) != 0;
    return false;
  }
  static bool testSet( WORD *val,  unsigned int nbits,  unsigned int shft ) {
    bool b = false;
    if ( shft < nbits ) {
      unsigned int i = shft >> WD_SHFT;
      WORD         w = (WORD) 1U << ( shft & WD_MASK );
      b = ( ( val[ i ] & w ) != 0 );
      val[ i ] |= w;
    }
    return b;
  }
  static bool testunSet( WORD *val,  unsigned int nbits,  unsigned int shft ) {
    bool b = false;
    if ( shft < nbits ) {
      unsigned int i = shft >> WD_SHFT;
      WORD         w = (WORD) 1U << ( shft & WD_MASK );
      b = ( ( val[ i ] & w ) != 0 );
      val[ i ] &= ~w;
    }
    return b;
  }
  static inline unsigned int do_ffs( unsigned int w ) {
    return ffsw( w );
  }
  static inline unsigned int do_ffs( ullong w ) {
    return ffsl( w );
  }
  static inline unsigned int do_ffs( unsigned short w ) {
    return ffsw( (unsigned int) w );
  }
  static inline unsigned int do_ffs( byte w ) {
    return ffsw( (unsigned int) w );
  }
  static bool nextSet( const WORD *val,  unsigned int nbits, unsigned int &j ) {
    if ( j < nbits ) {
      unsigned int k = j >> WD_SHFT;
      for (;;) {
        WORD w = val[ k ] >> ( j & WD_MASK );
        unsigned int x = do_ffs( w );
        if ( x == 0 ) {
          j = ++k << WD_SHFT;
          if ( j >= nbits )
            return false;
        }
        else {
          j += x - 1;
          if ( j >= nbits )
            return false;
          return true;
        }
      }
    }
    return false;
  }
  static bool flipNext( WORD *val,  unsigned int nbits,  unsigned int &j ) {
    if ( nextSet( val, nbits, j ) ) {
      val[ j >> WD_SHFT ] ^= ( (WORD) 1 << ( j & WD_MASK ) );
      return true;
    }
    return false;
  }
  static unsigned int bitCount( ullong v ) {
    return popcountl( v );
#if 0
    /* better if fewer bits set */
    for ( unsigned int cnt = 0; ; ) {
      if ( v == 0 )
        return cnt;
      cnt++;
      v &= v - 1;
    }
#endif
#if 0
    ullong w, x;
    w = v - ( ( v >> 1 ) & 0x5555555555555555ULL );
    x = ( w & 0x3333333333333333ULL ) +
        ( ( w >> 2 ) & 0x3333333333333333ULL );
    x = ( x + ( x >> 4 ) ) & 0x0f0f0f0f0f0f0f0fULL;
    return ( ( (unsigned int) x +
               (unsigned int) ( x >> 32 ) ) * 0x01010101U ) >> 24;
#endif
  }
  static unsigned int bitCount( const ullong *val,  unsigned int shft ) {
    unsigned int count = 0;
    ullong       v;
    for ( unsigned int i = 0; ; i++ ) {
      v = val[ i ];
      if ( shft < 64 )
        v &= ( (ullong) 1U << shft ) - 1;
      count += bitCount( v );
      if ( shft < 64 )
        break;
      shft -= 64;
    }
    return count;
  }
  static unsigned int bitCount( unsigned int v ) {
    return popcountw( v );
#if 0
    for ( unsigned int cnt = 0; ; ) {
      if ( v == 0 )
        return cnt;
      cnt++;
      v &= v - 1;
    }
#endif
  }
  static unsigned int bitCount( const unsigned int *val,  unsigned int shft ) {
    unsigned int count = 0;
    unsigned int v;
    for ( unsigned int i = 0; ; i++ ) {
      v = val[ i ];
      if ( shft < 32 )
        v &= ( 1U << shft ) - 1;
      count += bitCount( v );
      if ( shft < 32 )
        break;
      shft -= 32;
    }
    return count;
  }
  static void rightShift( WORD *val,  unsigned int sz,  unsigned int shft ) {
    unsigned int i, lshft, rshft, end;
    rshft  = shft & WD_MASK;
    shft >>= WD_SHFT;
    if ( shft >= sz ) {
      for ( i = 0; i < sz; i++ )
        val[ i ] = 0;
    }
    else {
      end = sz - shft - 1;
      if ( rshft == 0 ) {
        for ( i = 0; i < end; i++ )
          val[ i ] = val[ i + shft ];
        val[ i ] = val[ i + shft ];
      }
      else {
        lshft = WD_BITS - rshft;
        for ( i = 0; i < end; i++ )
          val[ i ] = ( val[ i + shft ] >> rshft ) |
                           ( val[ i + shft + 1 ] << lshft );
        val[ i ] = val[ i + shft ] >> rshft;
      }
      while ( ++i < sz )
        val[ i ] = 0;
    }
  }
  static void leftShift( WORD *val,  unsigned int sz,  unsigned int nbits,
                         unsigned int shft ) {
    unsigned int i, lshft, rshft, end;
    lshft  = shft & WD_MASK;
    shft >>= WD_SHFT;
    if ( shft >= sz ) {
      for ( i = 0; i < sz; i++ )
        val[ i ] = 0;
    }
    else {
      end = sz - shft;
      if ( lshft == 0 ) {
        for ( i = 1; i < end; i++ )
          val[ sz - i ] = val[ sz - ( i + shft ) ];
        val[ sz - i ] = val[ 0 ];
      }
      else {
        rshft = WD_BITS - lshft;
        for ( i = 1; i < end; i++ )
          val[ sz - i ] = ( val[ sz-( i+shft ) ] << lshft ) |
                                   ( val[ sz - ( i+shft+1 ) ] >> rshft );
        val[ sz - i ] = val[ 0 ] << lshft;
      }
      if ( (nbits &= WD_MASK) != 0 )
        val[ sz - 1 ] &= ( (WORD) 1U << nbits ) - 1;
      while ( ++i <= sz )
        val[ sz - i ] = 0;
    }
  }
  static bool equals( const WORD *v1,  const WORD *v2,  unsigned int sz ) {
    for ( unsigned int i = 0; i < sz; i++ )
      if ( v1[ i ] != v2[ i ] )
        return false;
    return true;
  }
  static bool equals( const WORD *v1,  unsigned int v2,  unsigned int sz ) {
    if ( v1[ 0 ] != (WORD) v2 )
      return false;
    for ( unsigned int i = 1; i < sz; i++ )
      if ( v1[ i ] != 0 )
        return false;
    return true;
  }
  static void copy( WORD *v1,  const WORD *v2,  unsigned int sz ) {
    while ( sz != 0 ) { --sz; *v1++ = *v2++; }
  }
  static void copy( WORD *v1,  unsigned int v2,  unsigned int sz ) {
    *v1++ = (WORD) v2;
    while ( --sz != 0 ) *v1++ = 0;
  }
  static void orBits( WORD *v1,  const WORD *v2,  unsigned int sz ) {
    while ( sz != 0 ) { --sz; *v1++ |= *v2++; }
  }
  static void orBits( WORD *v1,  unsigned int v2 ) {
    v1[ 0 ] |= (WORD) v2;
  }
  static void andBits( WORD *v1,  const WORD *v2,  unsigned int sz ) {
    while ( sz != 0 ) { --sz; *v1++ &= *v2++; }
  }
  static void andBits( WORD *v1,  unsigned int v2,  unsigned int sz ) {
    *v1++ &= (WORD) v2;
    while ( --sz != 0 ) *v1++ = 0;
  }
  static void xorBits( WORD *v1,  const WORD *v2,  unsigned int sz ) {
    while ( sz != 0 ) { --sz; *v1++ ^= *v2++; }
  }
  static void xorBits( WORD *v1,  unsigned int v2,  unsigned int sz ) {
    *v1++ ^= (WORD) v2;
    while ( --sz != 0 ) *v1++ ^= 0;
  }
  static void notBits( WORD *v1,  const WORD *v2,  unsigned int sz ) {
    while ( sz != 0 ) { --sz; *v1 &= ~*v2; v1++; v2++; }
  }
  static void notBits( WORD *v1,  unsigned int sz ) {
    while ( sz != 0 ) { --sz; *v1 = ~*v1; v1++; }
  }
};


template <class WORD>
struct TBitSetV {
  unsigned int valsz,
               nbits;
  WORD         val[ 1 ];

  void setAll( void ) {
    VecOps<WORD>::setAll( this->val, this->nbits );
  }
  void clearAll( void ) {
    VecOps<WORD>::clearAll( this->val, this->valsz );
  }
  void set( WORD *v ) {
    VecOps<WORD>::orBits( this->val, v, this->valsz );
  }
  void set( unsigned int shft ) {
    VecOps<WORD>::set( this->val, this->nbits, shft );
  }
  void unSet( unsigned int shft ) {
    VecOps<WORD>::unSet( this->val, this->nbits, shft );
  }
  bool isSet( unsigned int shft ) const {
    return VecOps<WORD>::isSet( this->val, this->nbits, shft );
  }
  bool testSet( unsigned int shft ) {
    return VecOps<WORD>::testSet( this->val, this->nbits, shft );
  }
  bool nextSet( unsigned int &j ) const {
    return VecOps<WORD>::nextSet( this->val, this->nbits, j );
  }
  bool nextSet( unsigned int &j,  unsigned int maxbit ) const {
    return VecOps<WORD>::nextSet( this->val, maxbit, j );
  }
  bool flipNext( unsigned int &j ) {
    return VecOps<WORD>::flipNext( this->val, this->nbits, j );
  }
  bool flipNext( unsigned int &j,  unsigned int maxbit ) {
    return VecOps<WORD>::flipNext( this->val, maxbit, j );
  }
  unsigned int bitCount( void ) const {
    return VecOps<WORD>::bitCount( this->val, this->nbits );
  }
  TBitSetV& operator>>=( unsigned int shft ) {
    VecOps<WORD>::rightShift( this->val, this->valsz, shft );
    return *this;
  }
  TBitSetV& operator<<=( unsigned int shft ) {
    VecOps<WORD>::leftShift( this->val, this->valsz, this->nbits, shft );
    return *this;
  }
  bool operator==( const TBitSetV &b ) const {
    return VecOps<WORD>::equals( this->val, b.val, this->valsz );
  }
  bool operator!=( const TBitSetV &b ) const {
    return ! ( *this == b );
  }
  bool operator==( unsigned int v ) const {
    return VecOps<WORD>::equals( this->val, v, this->valsz );
  }
  bool operator!=( unsigned int v ) const {
    return ! ( *this == v );
  }
  TBitSetV& operator=( const TBitSetV &b ) {
    VecOps<WORD>::copy( this->val, b.val, this->valsz );
    return *this;
  }
  TBitSetV& operator=( unsigned int v ) {
    VecOps<WORD>::copy( this->val, v, this->valsz );
    return *this;
  }
  TBitSetV& operator|=( const TBitSetV &b ) {
    VecOps<WORD>::orBits( this->val, b.val, this->valsz );
    return *this;
  }
  TBitSetV& operator|=( unsigned int v ) {
    VecOps<WORD>::orBits( this->val, v );
    return *this;
  }
  TBitSetV& operator&=( const TBitSetV &b ) {
    VecOps<WORD>::andBits( this->val, b.val, this->valsz );
    return *this;
  }
  TBitSetV& operator&=( unsigned int v ) {
    VecOps<WORD>::andBits( this->val, v, this->valsz );
    return *this;
  }
};


template <class WORD, unsigned int NBITS>
struct TBitSet {
  static const unsigned int VALSZ = ( NBITS + VecOps<WORD>::WD_MASK ) /
                                    VecOps<WORD>::WD_BITS;
  WORD val[ VALSZ ];

  TBitSet() {}
  TBitSet( unsigned int v ) {
    *this = v;
  }
  TBitSet( const TBitSet &b ) {
    *this = b;
  }
  const WORD *getVal( void ) const {
    return this->val;
  }
  unsigned int valByteCount( void ) const {
    return sizeof( this->val );
  }
  void setAll( void ) {
    VecOps<WORD>::setAll( this->val, NBITS );
  }
  void clearAll( void ) {
    VecOps<WORD>::clearAll( this->val, VALSZ );
  }
  void set( WORD *v ) {
    VecOps<WORD>::orBits( this->val, v, VALSZ );
  }
  void clear( WORD *v ) {
    VecOps<WORD>::clearAll( v, VALSZ );
  }
  void copyFrom( WORD *v ) {
    VecOps<WORD>::copy( this->val, v, VALSZ );
  }
  void copyTo( WORD *v ) {
    VecOps<WORD>::copy( v, this->val, VALSZ );
  }
  void set( unsigned int shft ) {
    VecOps<WORD>::set( this->val, NBITS, shft );
  }
  void unSet( unsigned int shft ) {
    VecOps<WORD>::unSet( this->val, NBITS, shft );
  }
  bool isSet( unsigned int shft ) const {
    return VecOps<WORD>::isSet( this->val, NBITS, shft );
  }
  bool testSet( unsigned int shft ) {
    return VecOps<WORD>::testSet( this->val, NBITS, shft );
  }
  bool nextSet( unsigned int &j ) const {
    return VecOps<WORD>::nextSet( this->val, NBITS, j );
  }
  bool nextSet( unsigned int &j,  unsigned int maxbit ) const {
    return VecOps<WORD>::nextSet( this->val, maxbit, j );
  }
  bool flipNext( unsigned int &j ) {
    return VecOps<WORD>::flipNext( this->val, NBITS, j );
  }
  bool flipNext( unsigned int &j,  unsigned int maxbit ) {
    return VecOps<WORD>::flipNext( this->val, maxbit, j );
  }
  unsigned int bitCount( unsigned int shft =
                         VALSZ * VecOps<WORD>::WD_BITS ) const {
    if ( shft == VALSZ * VecOps<WORD>::WD_BITS )
      shft = NBITS;
    return VecOps<WORD>::bitCount( this->val, shft );
  }
  TBitSet& operator>>=( unsigned int shft ) {
    VecOps<WORD>::rightShift( this->val, VALSZ, shft );
    return *this;
  }
  TBitSet& operator<<=( unsigned int shft ) {
    VecOps<WORD>::leftShift( this->val, VALSZ, NBITS, shft );
    return *this;
  }
  bool operator==( const TBitSet &b ) const {
    return VecOps<WORD>::equals( this->val, b.val, VALSZ );
  }
  bool operator!=( const TBitSet &b ) const {
    return ! ( *this == b );
  }
  bool operator==( unsigned int v ) const {
    return VecOps<WORD>::equals( this->val, v, VALSZ );
  }
  bool operator!=( unsigned int v ) const {
    return ! ( *this == v );
  }
  TBitSet& operator=( const TBitSet &b ) {
    VecOps<WORD>::copy( this->val, b.val, VALSZ );
    return *this;
  }
  TBitSet& operator=( unsigned int v ) {
    VecOps<WORD>::copy( this->val, v, VALSZ );
    return *this;
  }
  TBitSet& operator|=( const TBitSet &b ) {
    VecOps<WORD>::orBits( this->val, b.val, VALSZ );
    return *this;
  }
  TBitSet& operator|=( unsigned int v ) {
    VecOps<WORD>::orBits( this->val, v );
    return *this;
  }
  TBitSet& operator&=( const TBitSet &b ) {
    VecOps<WORD>::andBits( this->val, b.val, VALSZ );
    return *this;
  }
  TBitSet& operator&=( unsigned int v ) {
    VecOps<WORD>::andBits( this->val, v, VALSZ );
    return *this;
  }
  TBitSet operator~( void ) {
    TBitSet b( *this );
    VecOps<WORD>::flipAll( b.val, NBITS );
    return b;
  }
  TBitSet operator>>( unsigned int shft ) {
    TBitSet b( *this );
    b >>= shft;
    return b;
  }
  TBitSet operator<<( unsigned int shft ) {
    TBitSet b( *this );
    b <<= shft;
    return b;
  }
  TBitSet operator&( const TBitSet &b ) {
    TBitSet c( *this );
    c &= b;
    return c;
  }
  TBitSet operator&( unsigned int v ) {
    return TBitSet( this->val[ 0 ] & v );
  }
  TBitSet operator|( const TBitSet &b ) {
    TBitSet c( *this );
    c |= b;
    return c;
  }
  TBitSet operator|( unsigned int v ) {
    TBitSet b( *this );
    b |= v;
    return b;
  }
};

template <class WORD> /* maximum size is 256 * 64 + 56 = 16440 bits */
struct TBitSetDynamic {
  static const unsigned int MAX_HI_BITS = sizeof( WORD ) * 8,
                            MAX_LO_SIZE = sizeof( WORD ) - 1,
                            MAX_LO_BITS = sizeof( byte ) * MAX_LO_SIZE * 8;
  union {
    WORD   hi; /* if arsz == 0, then no allocation and size is 64 + 56 bits */
    WORD * ar; /* for more than 120 bits, ar is allocated and contains a vec */
  } u;
  byte arsz, /* arsz == 0 then u.hi, otherwise u.ar[] array size is arsz + 1 */
       lo[ MAX_LO_SIZE ]; /* 1 + sizeof( lo ) == sizeof( WORD ) */
  /* could increase max size by using short instead of byte here */

  TBitSetDynamic() {
    ::memset( this, 0, sizeof( *this ) );
  }
  TBitSetDynamic( unsigned int x ) {
    ::memset( this, 0, sizeof( *this ) );
    if ( x != 0 )
      this->orBits( x );
  }
  ~TBitSetDynamic() {
    if ( this->arsz > 0 )
      FREE( this->u.ar );
  }
  TBitSetDynamic( const TBitSetDynamic<WORD> &val ) : arsz( 0 ) {
    copyBits( *this, val );
  }
  TBitSetDynamic& operator=( const TBitSetDynamic<WORD> &val ) {
    copyBits( *this, val );
    return *this;
  }
  TBitSetDynamic& operator=( unsigned int x ) {
    if ( this->arsz != 0 )
      this->resize( 1 );
    ::memset( this, 0, sizeof( *this ) );
    if ( x != 0 )
      this->orBits( x );
    return *this;
  }
  bool operator==( const TBitSetDynamic<WORD> &val ) const {
    return equals( *this, val );
  }
  bool operator!=( const TBitSetDynamic<WORD> &val ) const {
    return ! equals( *this, val );
  }
  TBitSetDynamic& operator|=( const TBitSetDynamic<WORD> &val ) {
    orBits( *this, val );
    return *this;
  }
  TBitSetDynamic& operator|=( unsigned int val ) {
    this->orBits( val );
    return *this;
  }
  TBitSetDynamic& operator&=( const TBitSetDynamic<WORD> &val ) {
    andBits( *this, val );
    return *this;
  }
  TBitSetDynamic& operator^=( const TBitSetDynamic<WORD> &val ) {
    xorBits( *this, val );
    return *this;
  }

  unsigned int hiWords( void ) const {
    return (unsigned int) this->arsz + 1;
  }
  unsigned int hiBits( void ) const {
    return MAX_HI_BITS * this->hiWords();
  }
  unsigned int bitSize( void ) const {
    return MAX_LO_BITS + this->hiBits();
  }
  WORD *hiPtr( void ) {
    return ( this->arsz == 0 ? &this->u.hi : this->u.ar );
  }
  const WORD *hiPtrConst( void ) const {
    return ( this->arsz == 0 ? &this->u.hi : this->u.ar );
  }

  void setAll( void ) {
    VecOps<byte>::setAll( this->lo, MAX_LO_BITS );
    VecOps<WORD>::setAll( this->hiPtr(), this->hiBits() );
  }

  void clearAll( void ) {
    ::memset( this->lo, 0, sizeof( this->lo ) );
    VecOps<WORD>::clearAll( this->hiPtr(), this->hiWords() );
  }

  void set( unsigned int shft ) {
    if ( shft < MAX_LO_BITS )
      VecOps<byte>::set( this->lo, MAX_LO_BITS, shft );
    if ( shft >= this->bitSize() )
      this->extend( shft );
    shft -= MAX_LO_BITS;
    VecOps<WORD>::set( this->hiPtr(), this->hiBits(), shft );
  }

  void unSet( unsigned int shft ) {
    if ( shft < MAX_LO_BITS )
      VecOps<byte>::unSet( this->lo, MAX_LO_BITS, shft );
    if ( shft >= this->bitSize() )
      this->extend( shft );
    shft -= MAX_LO_BITS;
    VecOps<WORD>::unSet( this->hiPtr(), this->hiBits(), shft );
  }

  bool isSet( unsigned int shft ) {
    if ( shft < MAX_LO_BITS )
      return VecOps<byte>::isSet( this->lo, MAX_LO_BITS, shft );
    if ( shft >= this->bitSize() )
      return false;
    shft -= MAX_LO_BITS;
    return VecOps<WORD>::isSet( this->hiPtr(), this->hiBits(), shft );
  }

  bool testSet( unsigned int shft ) {
    if ( shft < MAX_LO_BITS )
      return VecOps<byte>::testSet( this->lo, MAX_LO_BITS, shft );
    if ( shft >= this->bitSize() )
      this->extend( shft );
    shft -= MAX_LO_BITS;
    return VecOps<WORD>::testSet( this->hiPtr(), this->hiBits(), shft );
  }

  bool testunSet( unsigned int shft ) {
    if ( shft < MAX_LO_BITS )
      return VecOps<byte>::testunSet( this->lo, MAX_LO_BITS, shft );
    if ( shft >= this->bitSize() )
      return false;
    shft -= MAX_LO_BITS;
    return VecOps<WORD>::testunSet( this->hiPtr(), this->hiBits(), shft );
  }

  static bool equals( const TBitSetDynamic<WORD> &lft,
                      const TBitSetDynamic<WORD> &rht ) {
    if ( ::memcmp( lft.lo, rht.lo, MAX_LO_SIZE ) != 0 )
      return false;
    const WORD   * lp  = lft.hiPtrConst(),
                 * rp  = rht.hiPtrConst();
    if ( lp[ 0 ] != rp[ 0 ] )
      return false;
    unsigned int   lsz = lft.hiWords(),
                   rsz = rht.hiWords(),
                   min = ( lsz < rsz ? lsz : rsz );
    if ( min == 1 )
      return true;
    if ( ! VecOps<WORD>::equals( &lp[ 1 ], &rp[ 1 ], min - 1 ) )
      return false;
    for ( unsigned int i = min; ; i++ ) {
      if ( i >= lsz && i >= rsz )
        return true;
      if ( ( i < lsz && lp[ i ] != 0 ) || ( i < rsz && rp[ i ] != 0 ) )
        return false;
    }
  }

  bool equals( const TBitSetDynamic<WORD> &val ) const {
    return equals( *this, val );
  }

  static void copyBits( TBitSetDynamic<WORD> &lft,
                        const TBitSetDynamic<WORD> &rht ) {
    ::memcpy( lft.lo, rht.lo, MAX_LO_SIZE );
    unsigned int lsz = lft.hiWords(),
                 rsz = rht.hiWords();
    if ( lsz != rsz )
      lft.resize( rsz );
    WORD       * lp  = lft.hiPtr();
    const WORD * rp  = rht.hiPtrConst();
    VecOps<WORD>::copy( lp, rp, rsz );
  }

  void copyBits( const TBitSetDynamic<WORD> &val ) {
    return copyBits( *this, val );
  }

  static void orBits( TBitSetDynamic<WORD> &lft,
                      const TBitSetDynamic<WORD> &rht ) {
    VecOps<byte>::orBits( lft.lo, rht.lo, MAX_LO_SIZE );
    unsigned int lsz = lft.hiWords(),
                 rsz = rht.hiWords();
    if ( lsz < rsz )
      lft.resize( rsz );
    WORD       * lp  = lft.hiPtr();
    const WORD * rp  = rht.hiPtrConst();
    VecOps<WORD>::orBits( lp, rp, rsz );
  }

  void orBits( const TBitSetDynamic<WORD> &val ) {
    orBits( *this, val );
  }

  void orBits( unsigned int val ) {
    unsigned int i = 0;
    while ( val != 0 ) {
      byte b = ( val & 0xffU );
      val >>= 8;
      if ( i < MAX_LO_SIZE )
        this->lo[ i++ ] |= b;
      else {
        WORD *p = this->hiPtr();
        p[ i ] |= b;
      }
    }
  }

  static void andBits( TBitSetDynamic<WORD> &lft,
                       const TBitSetDynamic<WORD> &rht ) {
    VecOps<byte>::andBits( lft.lo, rht.lo, MAX_LO_SIZE );
    unsigned int   lsz = lft.hiWords(),
                   rsz = rht.hiWords(),
                   min = ( lsz < rsz ? lsz : rsz );
    WORD         * lp  = lft.hiPtr();
    const WORD   * rp  = rht.hiPtrConst();
    VecOps<WORD>::andBits( lp, rp, min );
  }

  void andBits( const TBitSetDynamic<WORD> &val ) {
    andBits( *this, val );
  }
  /* operator: lft &= ~rht */
  static void notBits( TBitSetDynamic<WORD> &lft,
                       const TBitSetDynamic<WORD> &rht ) {
    VecOps<byte>::notBits( lft.lo, rht.lo, MAX_LO_SIZE );
    unsigned int   lsz = lft.hiWords(),
                   rsz = rht.hiWords(),
                   min = ( lsz < rsz ? lsz : rsz );
    WORD         * lp  = lft.hiPtr();
    const WORD   * rp  = rht.hiPtrConst();
    VecOps<WORD>::notBits( lp, rp, min );
  }

  void notBits( const TBitSetDynamic<WORD> &val ) {
    notBits( *this, val );
  }
  /* operator: this = ~this */
  void notBits( void ) {
    for ( unsigned int i = 0; i < MAX_LO_SIZE; i++ )
      this->lo[ i ] = ~this->lo[ i ];
    VecOps<WORD>::notBits( this->hiPtr(), this->hiWords() );
  }

  static void xorBits( TBitSetDynamic<WORD> &lft,
                       const TBitSetDynamic<WORD> &rht ) {
    VecOps<byte>::xorBits( lft.lo, rht.lo, MAX_LO_SIZE );
    unsigned int   lsz = lft.hiWords(),
                   rsz = rht.hiWords();
    if ( lsz < rsz )
      lft.resize( rsz );
    WORD       * lp  = lft.hiPtr();
    const WORD * rp  = rht.hiPtrConst();
    VecOps<WORD>::xorBits( lp, rp, rsz );
    if ( lsz > rsz )
      VecOps<WORD>::xorBits( &lp[ rsz ], (unsigned int) 0, lsz - rsz );
  }

  void xorBits( const TBitSetDynamic<WORD> &val ) {
    xorBits( *this, val );
  }

  bool isEmpty( void ) const {
    unsigned int i;
    for ( i = 0; i < MAX_LO_SIZE; i++ )
      if ( this->lo[ i ] != 0 )
        return false;
    const WORD * p  = this->hiPtrConst();
    unsigned int sz = this->hiWords();
    for ( i = 0; i < sz; i++ )
      if ( p[ i ] != 0 )
        return false;
    return true;
  }

  unsigned int bitCount( void ) const {
    WORD lob = 0;
    ::memcpy( &lob, this->lo, MAX_LO_SIZE );
    unsigned int cnt = VecOps<WORD>::bitCount( lob ),
                 sz  = this->hiWords();
    const WORD * p   = this->hiPtrConst();
    for ( unsigned int i = 0; i < sz; i++ )
      cnt += VecOps<WORD>::bitCount( p[ i ] );
    return cnt;
  }

  bool nextSet( unsigned int &j ) {
    if ( j < MAX_LO_BITS ) {
      if ( VecOps<byte>::nextSet( this->lo, MAX_LO_BITS, j ) )
        return true;
      j = MAX_LO_BITS;
    }
    unsigned int k = j - MAX_LO_BITS;
    if ( VecOps<WORD>::nextSet( this->hiPtr(), this->hiBits(), k ) ) {
      j = k + MAX_LO_BITS;
      return true;
    }
    return false;
  }

  unsigned int encode( byte *buf ) const {
    unsigned int i,
                 off  = 0,
                 prev = 0,
                 j    = 1,
                 k    = 0,
                 mask = 1,
                 sz   = this->hiWords();
    const WORD * p    = this->hiPtrConst();

    for ( i = 0; i < MAX_LO_SIZE; i++ ) {
      if ( this->lo[ i ] != 0 ) {
        k |= mask;
        buf[ j++ ] = this->lo[ i ];
      }
      mask <<= 1;
    }
    if ( sz > 1 || *p != 0 ) {
      i = 0;
      do {
        WORD v = p[ i ];
        for ( unsigned int n = 0; n < sizeof( WORD ); n++ ) {
          if ( mask == 0x80 ) {
            buf[ off ] = (byte) ( k | 0x80 );
            prev = off;
            off  = j++;
            mask = 1;
            k    = 0;
          }
          byte b = (byte) ( v & 0xffU );
          v >>= 8;
          if ( b != 0 ) {
            k |= mask;
            buf[ j++ ] = b;
          }
          mask <<= 1;
        }
      } while ( ++i != sz );
    }
    if ( k != 0 || j == 1 ) {
      buf[ off ] = (byte) k;
      return j;
    }
    buf[ prev ] &= 0x7fU;
    if ( j == off + 1 )
      j--;
    return j;
  }

  unsigned int maxEncodeSize( void ) const {
    return this->bitSize() / 7 + 1;
  }

  unsigned int decode( const byte *buf ) {
    static const unsigned int CHUNK_SZ = 32;
    unsigned int i,
                 off  = 0,
                 sz   = 0,
                 j    = 1,
                 k    = 0,
                 mask = 1;
    WORD         v[ CHUNK_SZ ];

    k = buf[ 0 ];
    for ( i = 0; i < MAX_LO_SIZE; i++ ) {
      if ( ( k & mask ) != 0 )
        this->lo[ i ] = buf[ j++ ];
      else
        this->lo[ i ] = 0;
      mask <<= 1;
    }
    for (;;) {
      v[ off ] = 0;
      for ( unsigned int n = 0; n < sizeof( WORD ); n++ ) {
        if ( mask == 0x80 ) {
          if ( ( k & 0x80 ) == 0 )
            goto break_loop;
          k    = buf[ j++ ];
          mask = 1;
        }
        if ( ( k & mask ) != 0 )
          v[ off ] |= (WORD) buf[ j++ ] << ( n * 8 );
        mask <<= 1;
      }
      if ( ++off == CHUNK_SZ ) {
        this->resize( sz + CHUNK_SZ );
        WORD *p = this->hiPtr();
        ::memcpy( &p[ sz ], v, sizeof( v ) );
        off = 0;
        sz += CHUNK_SZ;
      }
    }
  break_loop:;
    if ( v[ off ] != 0 )
      off++;
    if ( off > 0 ) {
      this->resize( sz + off );
      WORD *p = this->hiPtr();
      ::memcpy( &p[ sz ], v, sizeof( v[ 0 ] ) * off );
    }
    else if ( sz == 0 ) {
      this->resize( 1 );
      this->u.hi = 0;
    }
    return j;
  }

  static unsigned int decodeSize( const byte *buf,  unsigned int maxLen ) {
    for ( unsigned int i = 0; ; ) {
      if ( i >= maxLen )
        return 0;
      if ( ( buf[ i ] & 0x80 ) == 0 ) {
        i += 1 + VecOps<byte>::bitCount( (unsigned int) buf[ i ] );
        if ( i > maxLen )
          return 0;
        return i;
      }
      i += 1 + VecOps<byte>::bitCount( (unsigned int) buf[ i ] & 0x7fU );
    }
  }

  void resize( unsigned int newsz ) throw( Error ) {
    WORD * p = NULL;
    if ( newsz > 256 ) {
      static const ErrorRec e = { 0, "Too many bits (257*8)", "BitSetDynamic" };
      throw &e;
    }
    /* hiWords() size range should be 1 -> 256 */
    unsigned int cursz = this->hiWords();
    if ( newsz <= 1 ) {
      if ( cursz > 1 ) {
        p = this->u.ar;
        this->u.hi = p[ 0 ];
        FREE( p );
      }
      this->arsz = 0;
      return;
    }
    MALLOC( newsz * sizeof( WORD ), &p );
    if ( newsz > cursz ) {
      ::memcpy( p, this->hiPtr(), cursz * sizeof( WORD ) );
      ::memset( &p[ cursz ], 0, ( newsz - cursz ) * sizeof( WORD ) );
    }
    else {
      ::memcpy( p, this->hiPtr(), newsz * sizeof( WORD ) );
    }
    if ( this->arsz != 0 )
      FREE( this->u.ar );
    this->u.ar = p;
    this->arsz = (byte) ( newsz - 1 );
  }

  void extend( unsigned int shft ) throw( Error ) {
    this->resize( ( ( shft - MAX_LO_BITS ) + MAX_HI_BITS - 1 ) / MAX_HI_BITS );
  }
};

#ifndef BIT_SET_MACHINE_WORD
#if defined( __amd64__ ) || defined( __sparcv9 )
#define BIT_SET_MACHINE_WORD ullong
#else
#define BIT_SET_MACHINE_WORD unsigned int
#endif
#endif

template <unsigned int NBITS>
struct BitSet : public TBitSet<BIT_SET_MACHINE_WORD, NBITS> {
  BitSet() : TBitSet<BIT_SET_MACHINE_WORD, NBITS>() {}
  BitSet( unsigned int v ) : TBitSet<BIT_SET_MACHINE_WORD, NBITS>( v ) {}
  BitSet( const BitSet &b ) : TBitSet<BIT_SET_MACHINE_WORD, NBITS>( b ) {}
  void init( void ) { *this = 0; }
};

#if 0 && __GNUC__ >= 4
/* gcc4 builtin SSE vector ops, doesn't look worthwhile yet */
typedef BIT_SET_MACHINE_WORD MYVEC __attribute ((vector_size(256/8)))
template <unsigned int NBITS, class VEC>
struct BitSet2 : public TBitSet<BIT_SET_MACHINE_WORD, NBITS> {
  BitSet2() : TBitSet<BIT_SET_MACHINE_WORD, NBITS>() {}
  BitSet2( unsigned int v ) : TBitSet<BIT_SET_MACHINE_WORD, NBITS>( v ) {}
  BitSet2( const BitSet2 &b ) : TBitSet<BIT_SET_MACHINE_WORD, NBITS>( b ) {}

  BitSet2& operator|=( const BitSet2 &b ) {
    *(VEC *) this->val |= *(VEC *) b.val;
    return *this;
  }
};
#endif

struct BitSetV : public TBitSetV<BIT_SET_MACHINE_WORD> {
  private: BitSetV(); /* can't declare static */ public:

  void operator delete( void *p ) {
    FREE( p );
  }
  static BitSetV *create( unsigned int nbits ) {
    BitSetV    * set;
    unsigned int valsz = ( nbits + VecOps<BIT_SET_MACHINE_WORD>::WD_MASK ) /
                           VecOps<BIT_SET_MACHINE_WORD>::WD_BITS;
    MALLOC( valsz * sizeof( BIT_SET_MACHINE_WORD ) +
            sizeof( BitSetV ) - sizeof( BIT_SET_MACHINE_WORD ), &set );
    set->valsz = valsz;
    set->nbits = nbits;
    VecOps<BIT_SET_MACHINE_WORD>::clearAll( set->val, set->valsz );
    return set;
  }
};

struct BitSetB : public TBitSetV<byte> {
  private: BitSetB(); /* can't declare static */ public:

  void operator delete( void *p ) {
    FREE( p );
  }
  static BitSetB *create( unsigned int nbits ) {
    BitSetB    * set;
    unsigned int valsz = ( nbits + VecOps<byte>::WD_MASK ) /
                           VecOps<byte>::WD_BITS;
    MALLOC( valsz * sizeof( byte ) + sizeof( BitSetB ), &set );
    set->valsz = valsz;
    set->nbits = nbits;
    VecOps<byte>::clearAll( set->val, set->valsz );
    return set;
  }
};

typedef struct TBitSetDynamic<BIT_SET_MACHINE_WORD> BitSetD;

#if 0
#include <string.h>
#include "base/sys.h"
#include "stream/io_stream.h"
#include "base/mem.h"
#include "util/bit_set.h"

using namespace rai;

int
main( int argc, char *argv[] )
{
  BitSetD bs( 0 ), bs2( 0 ), bs3( 0 ), bs4( 0 ), bs5( 0 );
  unsigned int i, j;
  byte tmp[ 32 ];

  Sys::initialize();

  bs.set( 0 ); bs.set( 3 ); bs.set( 4 ); bs.set( 8 );
  Sys::out->puts( "bs : " );
  for ( i = 0; i < bs.bitSize(); i++ )
    Sys::out->puts( bs.isSet( i ) ? "1" : "0" );
  Sys::out->puts( "\n" );
  Sys::out->printf( "count: %u\n", bs.bitCount() ); Sys::out->flush();

  bs2.set( 15 ); bs2.set( 22 ); bs2.set( 64 ); bs2.set( 128 ); bs2.set( 144 );
  Sys::out->puts( "bs2: " );
  for ( i = 0; i < bs2.bitSize(); i++ )
    Sys::out->puts( bs2.isSet( i ) ? "1" : "0" );
  Sys::out->puts( "\n" );
  Sys::out->printf( "count: %u\n", bs2.bitCount() ); Sys::out->flush();

  bs3 = bs;
  bs3 |= bs2;

  Sys::out->puts( "bs3: " );
  for ( i = 0; i < bs3.bitSize(); i++ )
    Sys::out->puts( bs3.isSet( i ) ? "1" : "0" );
  Sys::out->puts( "\n" );
  Sys::out->printf( "count: %u\n", bs3.bitCount() ); Sys::out->flush();

  Sys::out->puts( "bs3: " );
  for ( i = 0; bs3.nextSet( i ); i++ )
    Sys::out->printf( "%u ", i );
  Sys::out->puts( "\n" ); Sys::out->flush();

  j = bs3.encode( tmp );
  for ( i = 0; i < j; i++ )
    Sys::out->printf( "%02x ", tmp[ i ] );
  Sys::out->puts( "\n" );
  Sys::out->printf( "bs3 sz %u, bits %u\n", j, bs3.bitSize() );
  Sys::out->flush();

  j = bs4.decode( tmp );
  Sys::out->printf( "bs4 sz %u, bits %u\n", j, bs4.bitSize() );
  Sys::out->printf( "bs4 %s bs3\n", ( bs4 == bs3 ) ? "==" : "!=" );
  Sys::out->flush();

  bs.clearAll();
  bs |= 0xf004408;
  Sys::out->puts( "bs : " );
  for ( i = 0; i < bs.bitSize(); i++ )
    Sys::out->puts( bs.isSet( i ) ? "1" : "0" );
  Sys::out->puts( "\n" );
  bs &= bs3;
  Sys::out->puts( "bs : " );
  for ( i = 0; i < bs.bitSize(); i++ )
    Sys::out->puts( bs.isSet( i ) ? "1" : "0" );
  Sys::out->puts( "\n" );

  j = bs.encode( tmp );
  for ( i = 0; i < j; i++ )
    Sys::out->printf( "%02x ", tmp[ i ] );
  Sys::out->puts( "\n" );
  Sys::out->printf( "bs  sz %u, bits %u\n", j, bs.bitSize() );
  j = bs5.decode( tmp );
  Sys::out->printf( "bs5 sz %u, bits %u\n", j, bs5.bitSize() );
  Sys::out->printf( "bs5 %s bs\n", ( bs5 == bs ) ? "==" : "!=" );

  Sys::out->puts( "bs5: " );
  for ( i = 0; i < bs5.bitSize(); i++ )
    Sys::out->puts( bs5.isSet( i ) ? "1" : "0" );
  Sys::out->puts( "\n" );
  Sys::out->flush();

  Sys::out->puts( "bs5: " );
  for ( i = 0; bs5.nextSet( i ); i++ )
    Sys::out->printf( "%u ", i );
  Sys::out->puts( "\n" ); Sys::out->flush();

  return 0;
}
#endif
} // namespace rai

#endif
