/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "util/hash_util.h"
#include "util/int_bits.h"
#include "base/mem.h"

using namespace rai;

/* no case */
static inline byte nocase( byte c )
{
  return (byte) toupper( (int) (unsigned int) c );
}

/* unrolls hash function to produce:
 * KeyType
 * function( const byte *buf, unsigned int bufLen, bool noCase, KeyType init )
 */
#define HASH_FUNCTION( NAME_SPC, KEY_TYPE, HASH_NAME, ROUND ) \
  KEY_TYPE \
  NAME_SPC::HASH_NAME( const byte *buf,  unsigned int bufLen,  KEY_TYPE key ) { \
    while ( bufLen >= 4 ) { \
      ROUND( key, buf[ 0 ] ); \
      ROUND( key, buf[ 1 ] ); \
      ROUND( key, buf[ 2 ] ); \
      ROUND( key, buf[ 3 ] ); \
      buf = &buf[ 4 ]; bufLen -= 4; \
    } \
    while ( bufLen > 0 ) { \
      ROUND( key, *buf++ ); \
      bufLen--; \
    } \
    return key; \
  }

#define HASH_FUNCTION2( KEY_TYPE, HASH_NAME, ROUND ) \
  static KEY_TYPE \
  HASH_NAME( const byte *buf,  unsigned int bufLen,  KEY_TYPE key ) { \
    register byte c; \
    while ( bufLen >= 4 ) { \
      c = nocase( buf[ 0 ] ); ROUND( key, c ); \
      c = nocase( buf[ 1 ] ); ROUND( key, c ); \
      c = nocase( buf[ 2 ] ); ROUND( key, c ); \
      c = nocase( buf[ 3 ] ); ROUND( key, c ); \
      buf = &buf[ 4 ]; bufLen -= 4; \
    } \
    while ( bufLen > 0 ) { \
      c = nocase( *buf++ ); ROUND( key, c ); \
      bufLen--; \
    } \
    return key; \
  }

/*
 * djb: comp.lang.c post credited to Dan Bernstein
 */
static inline void
djb_round( unsigned int &key,  byte c )
{
  key = (unsigned int) c ^ ( ( key << 5 ) + key );
}
/* define Hash32::djb() */
HASH_FUNCTION( Hash32, unsigned int, djb, djb_round )
HASH_FUNCTION2( unsigned int, djb_nocase, djb_round )

unsigned int
Hash32::djbs( const char *buf,  bool noCase,  unsigned int keyInit )
{
  if ( noCase )
    return djb_nocase( (const byte *) buf, ::strlen( buf ), keyInit );
  return Hash32::djb( (const byte *) buf, ::strlen( buf ), keyInit );
}


/*
 * fnv32 & fnv64: sleepycat dbm hash credited to Glenn Fowler,
 * Landon Curt Noll, Phong Vo; see: http://www.isthe.com/chongo/index.html
 */
static inline void
fnv32_round( unsigned int &key,  byte c )
{
  key = (unsigned int) c ^ ( key * ( ( 1U << 24 ) + 403U ) );
}
/* define Hash32::fnv() */
HASH_FUNCTION( Hash32, unsigned int, fnv, fnv32_round )
HASH_FUNCTION2( unsigned int, fnv_nocase, fnv32_round )

unsigned int
Hash32::fnvs( const char *buf,  bool noCase,  unsigned int keyInit )
{
  if ( noCase )
    return fnv_nocase( (const byte *) buf, ::strlen( buf ), keyInit );
  return Hash32::fnv( (const byte *) buf, ::strlen( buf ), keyInit );
}


static inline void
fnv64_round( ullong &key,  byte c )
{
  key = (ullong) c ^ ( key * ( ( (ullong) 1U << 40 ) + (ullong) 435U ) );
}
/* define Hash64::fnv() */
HASH_FUNCTION( Hash64, ullong, fnv, fnv64_round )

#undef HASH_FUNCTION


unsigned short
Hash16::djb( const byte *buf,  unsigned int bufLen, unsigned int key )
{
  key = Hash32::djb( buf, bufLen, key );
  return (unsigned short) ( key >> 16 ) ^ (unsigned short) key;
}


unsigned short
Hash16::djbs( const char *buf,  bool noCase,  unsigned int key )
{
  if ( noCase )
    key = Hash32::djb( (const byte *) buf, ::strlen( buf ), key );
  else
    key = djb_nocase( (const byte *) buf, ::strlen( buf ), key );
  return (unsigned short) ( key >> 16 ) ^ (unsigned short) key;
}

/*
 * newhash32 & newhash64:
 * Bob Jenkins newhash http://burtleburtle.net/bob/hash/evahash.html
 */

static inline unsigned int
le_word32_nocase( const byte *w )
{
  register unsigned int wd;
  
  wd  = (unsigned int) nocase( w[ 0 ] );
  wd |= (unsigned int) nocase( w[ 1 ] ) << 8;
  wd |= (unsigned int) nocase( w[ 2 ] ) << 16;
  wd |= (unsigned int) nocase( w[ 3 ] ) << 24;

  return wd;
}

static const unsigned int newhashMagic = 0x9e3779b9U;

static inline void
newhash_mix32( unsigned int &a,  unsigned int &b,  unsigned int &c )
{
  a -= b;  a -= c;  a ^= c >> 13;
  b -= c;  b -= a;  b ^= a << 8;
  c -= a;  c -= b;  c ^= b >> 13;
  a -= b;  a -= c;  a ^= c >> 12;
  b -= c;  b -= a;  b ^= a << 16;
  c -= a;  c -= b;  c ^= b >> 5;
  a -= b;  a -= c;  a ^= c >> 3;
  b -= c;  b -= a;  b ^= a << 10;
  c -= a;  c -= b;  c ^= b >> 15;
}

unsigned int
Hash32::bjHashInt( unsigned int a )
{
  a -= (a<<6);
  a ^= (a>>17);
  a -= (a<<9);
  a ^= (a<<4);
  a -= (a<<3);
  a ^= (a<<10);
  a ^= (a>>15);
  return a;
}

unsigned int
Hash32::newhash( const byte *buf,  unsigned int bufLen,  unsigned int c )
{
  register unsigned int i;
  unsigned int j, k, a, b, x, y, z;

  i = 0;
  a = b = newhashMagic;

  if ( bufLen >= 12 ) {
    if ( Aligned::isLittleEndian ) {
      do {
        Unaligned::getInt( &buf[ i ], x ); a += x;
        Unaligned::getInt( &buf[ i + 4 ], y ); b += y;
        Unaligned::getInt( &buf[ i + 8 ], z ); c += z;
        i += 12;
        newhash_mix32( a, b, c );
      } while ( i + 12 <= bufLen );
    }
    else {
      do {
        Unaligned::getInt( &buf[ i ], x ); Aligned::swap( x ); a += x;
        Unaligned::getInt( &buf[ i + 4 ], y ); Aligned::swap( y ); b += y;
        Unaligned::getInt( &buf[ i + 8 ], z ); Aligned::swap( x ); c += z;
        i += 12;
        newhash_mix32( a, b, c );
      } while ( i + 12 <= bufLen );
    }
  }

  c += bufLen;
  j  = bufLen;
  k  = bufLen - i;
  while ( k > 8 )
    c += (unsigned int) buf[ --j ] << ( ( k-- * 8 ) & 31U );
  while ( k > 4 )
    b += (unsigned int) buf[ --j ] << ( ( --k * 8 ) & 31U );
  while ( k > 0 )
    a += (unsigned int) buf[ --j ] << ( ( --k * 8 ) & 31U );
  newhash_mix32( a, b, c );

  return c;
}


static unsigned int
newhash_nocase( const byte *buf,  unsigned int bufLen,  unsigned int c )
{
  unsigned int i, j, k, a, b;

  i = 0;
  a = b = newhashMagic;

  if ( bufLen >= 12 ) {
    do {
      a += le_word32_nocase( &buf[ i ] ); i += 4;
      b += le_word32_nocase( &buf[ i ] ); i += 4;
      c += le_word32_nocase( &buf[ i ] ); i += 4;
      newhash_mix32( a, b, c );
    } while ( i + 12 <= bufLen );
  }

  c += bufLen;
  j  = bufLen;
  k  = bufLen - i;
  while ( k > 8 )
    c += (unsigned int) nocase( buf[ --j ] ) << ( ( k-- * 8 ) & 31U );
  while ( k > 4 )
    b += (unsigned int) nocase( buf[ --j ] ) << ( ( --k * 8 ) & 31U );
  while ( k > 0 )
    a += (unsigned int) nocase( buf[ --j ] ) << ( ( --k * 8 ) & 31U );
  newhash_mix32( a, b, c );

  return c;
}


unsigned int
Hash32::newhashs( const char *buf,  bool noCase,  unsigned int keyInit )
{
  if ( noCase )
    return newhash_nocase( (const byte *) buf, ::strlen( buf ), keyInit );
  return Hash32::newhash( (const byte *) buf, ::strlen( buf ), keyInit );
}


static inline ullong
le_word64_nocase( const byte *w )
{
  unsigned int wlo,
               whi;

  wlo  = (unsigned int) nocase( w[ 0 ] );
  wlo |= (unsigned int) nocase( w[ 1 ] ) << 8;
  wlo |= (unsigned int) nocase( w[ 2 ] ) << 16;
  wlo |= (unsigned int) nocase( w[ 3 ] ) << 24;
  whi  = (unsigned int) nocase( w[ 4 ] );
  whi |= (unsigned int) nocase( w[ 5 ] ) << 8;
  whi |= (unsigned int) nocase( w[ 6 ] ) << 16;
  whi |= (unsigned int) nocase( w[ 7 ] ) << 24;

  return (ullong) wlo | ( (ullong) whi << 32 );
}


static inline ullong
le_word64( const byte *w )
{
  unsigned int wlo,
               whi;

  wlo  = (unsigned int) w[ 0 ];
  wlo |= (unsigned int) w[ 1 ] << 8;
  wlo |= (unsigned int) w[ 2 ] << 16;
  wlo |= (unsigned int) w[ 3 ] << 24;
  whi  = (unsigned int) w[ 4 ];
  whi |= (unsigned int) w[ 5 ] << 8;
  whi |= (unsigned int) w[ 6 ] << 16;
  whi |= (unsigned int) w[ 7 ] << 24;

  return (ullong) wlo | ( (ullong) whi << 32 );
}


ullong
Hash64::newhash( const byte *buf,  unsigned int bufLen,  ullong c )
{
  unsigned int i, j, k;
  ullong       a, b, x, y, z;

  i = 0;
  a = b = Hash64::newhashMagic;

  if ( bufLen >= 24 ) {
    if ( Aligned::isLittleEndian ) {
      do {
        Unaligned::getInt( &buf[ i ], x ); a += x;
        Unaligned::getInt( &buf[ i + 8 ], y ); b += y;
        Unaligned::getInt( &buf[ i + 16 ], z ); c += z;
        i += 24;
        Hash64::newhash_mix( a, b, c );
      } while ( i + 24 <= bufLen );
    }
    else {
      do {
        Unaligned::getInt( &buf[ i ], x ); Aligned::swap( x ); a += x;
        Unaligned::getInt( &buf[ i + 8 ], y ); Aligned::swap( y ); b += y;
        Unaligned::getInt( &buf[ i + 16 ], z ); Aligned::swap( z ); c += z;
        i += 24;
        Hash64::newhash_mix( a, b, c );
      } while ( i + 24 <= bufLen );
    }
  }

  c += bufLen;
  j  = bufLen;
  k  = bufLen - i;
  while ( k > 16 )
    c += (ullong) buf[ --j ] << ( ( k-- * 8 ) & 63U );
  while ( k > 8 )
    b += (ullong) buf[ --j ] << ( ( --k * 8 ) & 63U );
  while ( k > 0 )
    a += (ullong) buf[ --j ] << ( ( --k * 8 ) & 63U );
  Hash64::newhash_mix( a, b, c );

  return c;
}


static ullong
newhash_nocase( const byte *buf,  unsigned int bufLen,  ullong c )
{
  unsigned int i, j, k;
  ullong       a, b;

  i = 0;
  a = b = Hash64::newhashMagic;

  if ( bufLen >= 24 ) {
    do {
      a += le_word64_nocase( &buf[ i ] ); i += 8;
      b += le_word64_nocase( &buf[ i ] ); i += 8;
      c += le_word64_nocase( &buf[ i ] ); i += 8;
      Hash64::newhash_mix( a, b, c );
    } while ( i + 24 <= bufLen );
  }

  c += bufLen;
  j  = bufLen;
  k  = bufLen - i;
  while ( k > 16 )
    c += (ullong) nocase( buf[ --j ] ) << ( ( k-- * 8 ) & 63U );
  while ( k > 8 )
    b += (ullong) nocase( buf[ --j ] ) << ( ( --k * 8 ) & 63U );
  while ( k > 0 )
    a += (ullong) nocase( buf[ --j ] ) << ( ( --k * 8 ) & 63U );
  Hash64::newhash_mix( a, b, c );

  return c;
}


ullong
Hash64::newhashs( const char *buf,  bool noCase,  ullong keyInit )
{
  if ( noCase )
    return newhash_nocase( (const byte *) buf, ::strlen( buf ), keyInit );
  return Hash64::newhash( (const byte *) buf, ::strlen( buf ), keyInit );
}


/* Originally:
 *
 * crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * Thanks to Rodney Brown <rbrown64@csc.com.au> for his contribution of faster
 * CRC methods: exclusive-oring 32 bits of data at a time, and pre-computing
 * tables for updating the shift register in one step with three exclusive-ors
 * instead of four steps with four exclusive-ors.  This results in more than a
 * factor-of-two increase in speed on a Power PC using gcc -O3.
 */

static const unsigned int crc32tab_le[4][256] = /* little endian */
{
  {
    0x00000000U, 0x77073096U, 0xee0e612cU, 0x990951baU, 0x076dc419U,
    0x706af48fU, 0xe963a535U, 0x9e6495a3U, 0x0edb8832U, 0x79dcb8a4U,
    0xe0d5e91eU, 0x97d2d988U, 0x09b64c2bU, 0x7eb17cbdU, 0xe7b82d07U,
    0x90bf1d91U, 0x1db71064U, 0x6ab020f2U, 0xf3b97148U, 0x84be41deU,
    0x1adad47dU, 0x6ddde4ebU, 0xf4d4b551U, 0x83d385c7U, 0x136c9856U,
    0x646ba8c0U, 0xfd62f97aU, 0x8a65c9ecU, 0x14015c4fU, 0x63066cd9U,
    0xfa0f3d63U, 0x8d080df5U, 0x3b6e20c8U, 0x4c69105eU, 0xd56041e4U,
    0xa2677172U, 0x3c03e4d1U, 0x4b04d447U, 0xd20d85fdU, 0xa50ab56bU,
    0x35b5a8faU, 0x42b2986cU, 0xdbbbc9d6U, 0xacbcf940U, 0x32d86ce3U,
    0x45df5c75U, 0xdcd60dcfU, 0xabd13d59U, 0x26d930acU, 0x51de003aU,
    0xc8d75180U, 0xbfd06116U, 0x21b4f4b5U, 0x56b3c423U, 0xcfba9599U,
    0xb8bda50fU, 0x2802b89eU, 0x5f058808U, 0xc60cd9b2U, 0xb10be924U,
    0x2f6f7c87U, 0x58684c11U, 0xc1611dabU, 0xb6662d3dU, 0x76dc4190U,
    0x01db7106U, 0x98d220bcU, 0xefd5102aU, 0x71b18589U, 0x06b6b51fU,
    0x9fbfe4a5U, 0xe8b8d433U, 0x7807c9a2U, 0x0f00f934U, 0x9609a88eU,
    0xe10e9818U, 0x7f6a0dbbU, 0x086d3d2dU, 0x91646c97U, 0xe6635c01U,
    0x6b6b51f4U, 0x1c6c6162U, 0x856530d8U, 0xf262004eU, 0x6c0695edU,
    0x1b01a57bU, 0x8208f4c1U, 0xf50fc457U, 0x65b0d9c6U, 0x12b7e950U,
    0x8bbeb8eaU, 0xfcb9887cU, 0x62dd1ddfU, 0x15da2d49U, 0x8cd37cf3U,
    0xfbd44c65U, 0x4db26158U, 0x3ab551ceU, 0xa3bc0074U, 0xd4bb30e2U,
    0x4adfa541U, 0x3dd895d7U, 0xa4d1c46dU, 0xd3d6f4fbU, 0x4369e96aU,
    0x346ed9fcU, 0xad678846U, 0xda60b8d0U, 0x44042d73U, 0x33031de5U,
    0xaa0a4c5fU, 0xdd0d7cc9U, 0x5005713cU, 0x270241aaU, 0xbe0b1010U,
    0xc90c2086U, 0x5768b525U, 0x206f85b3U, 0xb966d409U, 0xce61e49fU,
    0x5edef90eU, 0x29d9c998U, 0xb0d09822U, 0xc7d7a8b4U, 0x59b33d17U,
    0x2eb40d81U, 0xb7bd5c3bU, 0xc0ba6cadU, 0xedb88320U, 0x9abfb3b6U,
    0x03b6e20cU, 0x74b1d29aU, 0xead54739U, 0x9dd277afU, 0x04db2615U,
    0x73dc1683U, 0xe3630b12U, 0x94643b84U, 0x0d6d6a3eU, 0x7a6a5aa8U,
    0xe40ecf0bU, 0x9309ff9dU, 0x0a00ae27U, 0x7d079eb1U, 0xf00f9344U,
    0x8708a3d2U, 0x1e01f268U, 0x6906c2feU, 0xf762575dU, 0x806567cbU,
    0x196c3671U, 0x6e6b06e7U, 0xfed41b76U, 0x89d32be0U, 0x10da7a5aU,
    0x67dd4accU, 0xf9b9df6fU, 0x8ebeeff9U, 0x17b7be43U, 0x60b08ed5U,
    0xd6d6a3e8U, 0xa1d1937eU, 0x38d8c2c4U, 0x4fdff252U, 0xd1bb67f1U,
    0xa6bc5767U, 0x3fb506ddU, 0x48b2364bU, 0xd80d2bdaU, 0xaf0a1b4cU,
    0x36034af6U, 0x41047a60U, 0xdf60efc3U, 0xa867df55U, 0x316e8eefU,
    0x4669be79U, 0xcb61b38cU, 0xbc66831aU, 0x256fd2a0U, 0x5268e236U,
    0xcc0c7795U, 0xbb0b4703U, 0x220216b9U, 0x5505262fU, 0xc5ba3bbeU,
    0xb2bd0b28U, 0x2bb45a92U, 0x5cb36a04U, 0xc2d7ffa7U, 0xb5d0cf31U,
    0x2cd99e8bU, 0x5bdeae1dU, 0x9b64c2b0U, 0xec63f226U, 0x756aa39cU,
    0x026d930aU, 0x9c0906a9U, 0xeb0e363fU, 0x72076785U, 0x05005713U,
    0x95bf4a82U, 0xe2b87a14U, 0x7bb12baeU, 0x0cb61b38U, 0x92d28e9bU,
    0xe5d5be0dU, 0x7cdcefb7U, 0x0bdbdf21U, 0x86d3d2d4U, 0xf1d4e242U,
    0x68ddb3f8U, 0x1fda836eU, 0x81be16cdU, 0xf6b9265bU, 0x6fb077e1U,
    0x18b74777U, 0x88085ae6U, 0xff0f6a70U, 0x66063bcaU, 0x11010b5cU,
    0x8f659effU, 0xf862ae69U, 0x616bffd3U, 0x166ccf45U, 0xa00ae278U,
    0xd70dd2eeU, 0x4e048354U, 0x3903b3c2U, 0xa7672661U, 0xd06016f7U,
    0x4969474dU, 0x3e6e77dbU, 0xaed16a4aU, 0xd9d65adcU, 0x40df0b66U,
    0x37d83bf0U, 0xa9bcae53U, 0xdebb9ec5U, 0x47b2cf7fU, 0x30b5ffe9U,
    0xbdbdf21cU, 0xcabac28aU, 0x53b39330U, 0x24b4a3a6U, 0xbad03605U,
    0xcdd70693U, 0x54de5729U, 0x23d967bfU, 0xb3667a2eU, 0xc4614ab8U,
    0x5d681b02U, 0x2a6f2b94U, 0xb40bbe37U, 0xc30c8ea1U, 0x5a05df1bU,
    0x2d02ef8dU
  },
  {
    0x00000000U, 0x191b3141U, 0x32366282U, 0x2b2d53c3U, 0x646cc504U,
    0x7d77f445U, 0x565aa786U, 0x4f4196c7U, 0xc8d98a08U, 0xd1c2bb49U,
    0xfaefe88aU, 0xe3f4d9cbU, 0xacb54f0cU, 0xb5ae7e4dU, 0x9e832d8eU,
    0x87981ccfU, 0x4ac21251U, 0x53d92310U, 0x78f470d3U, 0x61ef4192U,
    0x2eaed755U, 0x37b5e614U, 0x1c98b5d7U, 0x05838496U, 0x821b9859U,
    0x9b00a918U, 0xb02dfadbU, 0xa936cb9aU, 0xe6775d5dU, 0xff6c6c1cU,
    0xd4413fdfU, 0xcd5a0e9eU, 0x958424a2U, 0x8c9f15e3U, 0xa7b24620U,
    0xbea97761U, 0xf1e8e1a6U, 0xe8f3d0e7U, 0xc3de8324U, 0xdac5b265U,
    0x5d5daeaaU, 0x44469febU, 0x6f6bcc28U, 0x7670fd69U, 0x39316baeU,
    0x202a5aefU, 0x0b07092cU, 0x121c386dU, 0xdf4636f3U, 0xc65d07b2U,
    0xed705471U, 0xf46b6530U, 0xbb2af3f7U, 0xa231c2b6U, 0x891c9175U,
    0x9007a034U, 0x179fbcfbU, 0x0e848dbaU, 0x25a9de79U, 0x3cb2ef38U,
    0x73f379ffU, 0x6ae848beU, 0x41c51b7dU, 0x58de2a3cU, 0xf0794f05U,
    0xe9627e44U, 0xc24f2d87U, 0xdb541cc6U, 0x94158a01U, 0x8d0ebb40U,
    0xa623e883U, 0xbf38d9c2U, 0x38a0c50dU, 0x21bbf44cU, 0x0a96a78fU,
    0x138d96ceU, 0x5ccc0009U, 0x45d73148U, 0x6efa628bU, 0x77e153caU,
    0xbabb5d54U, 0xa3a06c15U, 0x888d3fd6U, 0x91960e97U, 0xded79850U,
    0xc7cca911U, 0xece1fad2U, 0xf5facb93U, 0x7262d75cU, 0x6b79e61dU,
    0x4054b5deU, 0x594f849fU, 0x160e1258U, 0x0f152319U, 0x243870daU,
    0x3d23419bU, 0x65fd6ba7U, 0x7ce65ae6U, 0x57cb0925U, 0x4ed03864U,
    0x0191aea3U, 0x188a9fe2U, 0x33a7cc21U, 0x2abcfd60U, 0xad24e1afU,
    0xb43fd0eeU, 0x9f12832dU, 0x8609b26cU, 0xc94824abU, 0xd05315eaU,
    0xfb7e4629U, 0xe2657768U, 0x2f3f79f6U, 0x362448b7U, 0x1d091b74U,
    0x04122a35U, 0x4b53bcf2U, 0x52488db3U, 0x7965de70U, 0x607eef31U,
    0xe7e6f3feU, 0xfefdc2bfU, 0xd5d0917cU, 0xcccba03dU, 0x838a36faU,
    0x9a9107bbU, 0xb1bc5478U, 0xa8a76539U, 0x3b83984bU, 0x2298a90aU,
    0x09b5fac9U, 0x10aecb88U, 0x5fef5d4fU, 0x46f46c0eU, 0x6dd93fcdU,
    0x74c20e8cU, 0xf35a1243U, 0xea412302U, 0xc16c70c1U, 0xd8774180U,
    0x9736d747U, 0x8e2de606U, 0xa500b5c5U, 0xbc1b8484U, 0x71418a1aU,
    0x685abb5bU, 0x4377e898U, 0x5a6cd9d9U, 0x152d4f1eU, 0x0c367e5fU,
    0x271b2d9cU, 0x3e001cddU, 0xb9980012U, 0xa0833153U, 0x8bae6290U,
    0x92b553d1U, 0xddf4c516U, 0xc4eff457U, 0xefc2a794U, 0xf6d996d5U,
    0xae07bce9U, 0xb71c8da8U, 0x9c31de6bU, 0x852aef2aU, 0xca6b79edU,
    0xd37048acU, 0xf85d1b6fU, 0xe1462a2eU, 0x66de36e1U, 0x7fc507a0U,
    0x54e85463U, 0x4df36522U, 0x02b2f3e5U, 0x1ba9c2a4U, 0x30849167U,
    0x299fa026U, 0xe4c5aeb8U, 0xfdde9ff9U, 0xd6f3cc3aU, 0xcfe8fd7bU,
    0x80a96bbcU, 0x99b25afdU, 0xb29f093eU, 0xab84387fU, 0x2c1c24b0U,
    0x350715f1U, 0x1e2a4632U, 0x07317773U, 0x4870e1b4U, 0x516bd0f5U,
    0x7a468336U, 0x635db277U, 0xcbfad74eU, 0xd2e1e60fU, 0xf9ccb5ccU,
    0xe0d7848dU, 0xaf96124aU, 0xb68d230bU, 0x9da070c8U, 0x84bb4189U,
    0x03235d46U, 0x1a386c07U, 0x31153fc4U, 0x280e0e85U, 0x674f9842U,
    0x7e54a903U, 0x5579fac0U, 0x4c62cb81U, 0x8138c51fU, 0x9823f45eU,
    0xb30ea79dU, 0xaa1596dcU, 0xe554001bU, 0xfc4f315aU, 0xd7626299U,
    0xce7953d8U, 0x49e14f17U, 0x50fa7e56U, 0x7bd72d95U, 0x62cc1cd4U,
    0x2d8d8a13U, 0x3496bb52U, 0x1fbbe891U, 0x06a0d9d0U, 0x5e7ef3ecU,
    0x4765c2adU, 0x6c48916eU, 0x7553a02fU, 0x3a1236e8U, 0x230907a9U,
    0x0824546aU, 0x113f652bU, 0x96a779e4U, 0x8fbc48a5U, 0xa4911b66U,
    0xbd8a2a27U, 0xf2cbbce0U, 0xebd08da1U, 0xc0fdde62U, 0xd9e6ef23U,
    0x14bce1bdU, 0x0da7d0fcU, 0x268a833fU, 0x3f91b27eU, 0x70d024b9U,
    0x69cb15f8U, 0x42e6463bU, 0x5bfd777aU, 0xdc656bb5U, 0xc57e5af4U,
    0xee530937U, 0xf7483876U, 0xb809aeb1U, 0xa1129ff0U, 0x8a3fcc33U,
    0x9324fd72U
  },
  {
    0x00000000U, 0x01c26a37U, 0x0384d46eU, 0x0246be59U, 0x0709a8dcU,
    0x06cbc2ebU, 0x048d7cb2U, 0x054f1685U, 0x0e1351b8U, 0x0fd13b8fU,
    0x0d9785d6U, 0x0c55efe1U, 0x091af964U, 0x08d89353U, 0x0a9e2d0aU,
    0x0b5c473dU, 0x1c26a370U, 0x1de4c947U, 0x1fa2771eU, 0x1e601d29U,
    0x1b2f0bacU, 0x1aed619bU, 0x18abdfc2U, 0x1969b5f5U, 0x1235f2c8U,
    0x13f798ffU, 0x11b126a6U, 0x10734c91U, 0x153c5a14U, 0x14fe3023U,
    0x16b88e7aU, 0x177ae44dU, 0x384d46e0U, 0x398f2cd7U, 0x3bc9928eU,
    0x3a0bf8b9U, 0x3f44ee3cU, 0x3e86840bU, 0x3cc03a52U, 0x3d025065U,
    0x365e1758U, 0x379c7d6fU, 0x35dac336U, 0x3418a901U, 0x3157bf84U,
    0x3095d5b3U, 0x32d36beaU, 0x331101ddU, 0x246be590U, 0x25a98fa7U,
    0x27ef31feU, 0x262d5bc9U, 0x23624d4cU, 0x22a0277bU, 0x20e69922U,
    0x2124f315U, 0x2a78b428U, 0x2bbade1fU, 0x29fc6046U, 0x283e0a71U,
    0x2d711cf4U, 0x2cb376c3U, 0x2ef5c89aU, 0x2f37a2adU, 0x709a8dc0U,
    0x7158e7f7U, 0x731e59aeU, 0x72dc3399U, 0x7793251cU, 0x76514f2bU,
    0x7417f172U, 0x75d59b45U, 0x7e89dc78U, 0x7f4bb64fU, 0x7d0d0816U,
    0x7ccf6221U, 0x798074a4U, 0x78421e93U, 0x7a04a0caU, 0x7bc6cafdU,
    0x6cbc2eb0U, 0x6d7e4487U, 0x6f38fadeU, 0x6efa90e9U, 0x6bb5866cU,
    0x6a77ec5bU, 0x68315202U, 0x69f33835U, 0x62af7f08U, 0x636d153fU,
    0x612bab66U, 0x60e9c151U, 0x65a6d7d4U, 0x6464bde3U, 0x662203baU,
    0x67e0698dU, 0x48d7cb20U, 0x4915a117U, 0x4b531f4eU, 0x4a917579U,
    0x4fde63fcU, 0x4e1c09cbU, 0x4c5ab792U, 0x4d98dda5U, 0x46c49a98U,
    0x4706f0afU, 0x45404ef6U, 0x448224c1U, 0x41cd3244U, 0x400f5873U,
    0x4249e62aU, 0x438b8c1dU, 0x54f16850U, 0x55330267U, 0x5775bc3eU,
    0x56b7d609U, 0x53f8c08cU, 0x523aaabbU, 0x507c14e2U, 0x51be7ed5U,
    0x5ae239e8U, 0x5b2053dfU, 0x5966ed86U, 0x58a487b1U, 0x5deb9134U,
    0x5c29fb03U, 0x5e6f455aU, 0x5fad2f6dU, 0xe1351b80U, 0xe0f771b7U,
    0xe2b1cfeeU, 0xe373a5d9U, 0xe63cb35cU, 0xe7fed96bU, 0xe5b86732U,
    0xe47a0d05U, 0xef264a38U, 0xeee4200fU, 0xeca29e56U, 0xed60f461U,
    0xe82fe2e4U, 0xe9ed88d3U, 0xebab368aU, 0xea695cbdU, 0xfd13b8f0U,
    0xfcd1d2c7U, 0xfe976c9eU, 0xff5506a9U, 0xfa1a102cU, 0xfbd87a1bU,
    0xf99ec442U, 0xf85cae75U, 0xf300e948U, 0xf2c2837fU, 0xf0843d26U,
    0xf1465711U, 0xf4094194U, 0xf5cb2ba3U, 0xf78d95faU, 0xf64fffcdU,
    0xd9785d60U, 0xd8ba3757U, 0xdafc890eU, 0xdb3ee339U, 0xde71f5bcU,
    0xdfb39f8bU, 0xddf521d2U, 0xdc374be5U, 0xd76b0cd8U, 0xd6a966efU,
    0xd4efd8b6U, 0xd52db281U, 0xd062a404U, 0xd1a0ce33U, 0xd3e6706aU,
    0xd2241a5dU, 0xc55efe10U, 0xc49c9427U, 0xc6da2a7eU, 0xc7184049U,
    0xc25756ccU, 0xc3953cfbU, 0xc1d382a2U, 0xc011e895U, 0xcb4dafa8U,
    0xca8fc59fU, 0xc8c97bc6U, 0xc90b11f1U, 0xcc440774U, 0xcd866d43U,
    0xcfc0d31aU, 0xce02b92dU, 0x91af9640U, 0x906dfc77U, 0x922b422eU,
    0x93e92819U, 0x96a63e9cU, 0x976454abU, 0x9522eaf2U, 0x94e080c5U,
    0x9fbcc7f8U, 0x9e7eadcfU, 0x9c381396U, 0x9dfa79a1U, 0x98b56f24U,
    0x99770513U, 0x9b31bb4aU, 0x9af3d17dU, 0x8d893530U, 0x8c4b5f07U,
    0x8e0de15eU, 0x8fcf8b69U, 0x8a809decU, 0x8b42f7dbU, 0x89044982U,
    0x88c623b5U, 0x839a6488U, 0x82580ebfU, 0x801eb0e6U, 0x81dcdad1U,
    0x8493cc54U, 0x8551a663U, 0x8717183aU, 0x86d5720dU, 0xa9e2d0a0U,
    0xa820ba97U, 0xaa6604ceU, 0xaba46ef9U, 0xaeeb787cU, 0xaf29124bU,
    0xad6fac12U, 0xacadc625U, 0xa7f18118U, 0xa633eb2fU, 0xa4755576U,
    0xa5b73f41U, 0xa0f829c4U, 0xa13a43f3U, 0xa37cfdaaU, 0xa2be979dU,
    0xb5c473d0U, 0xb40619e7U, 0xb640a7beU, 0xb782cd89U, 0xb2cddb0cU,
    0xb30fb13bU, 0xb1490f62U, 0xb08b6555U, 0xbbd72268U, 0xba15485fU,
    0xb853f606U, 0xb9919c31U, 0xbcde8ab4U, 0xbd1ce083U, 0xbf5a5edaU,
    0xbe9834edU
  },
  {
    0x00000000U, 0xb8bc6765U, 0xaa09c88bU, 0x12b5afeeU, 0x8f629757U,
    0x37def032U, 0x256b5fdcU, 0x9dd738b9U, 0xc5b428efU, 0x7d084f8aU,
    0x6fbde064U, 0xd7018701U, 0x4ad6bfb8U, 0xf26ad8ddU, 0xe0df7733U,
    0x58631056U, 0x5019579fU, 0xe8a530faU, 0xfa109f14U, 0x42acf871U,
    0xdf7bc0c8U, 0x67c7a7adU, 0x75720843U, 0xcdce6f26U, 0x95ad7f70U,
    0x2d111815U, 0x3fa4b7fbU, 0x8718d09eU, 0x1acfe827U, 0xa2738f42U,
    0xb0c620acU, 0x087a47c9U, 0xa032af3eU, 0x188ec85bU, 0x0a3b67b5U,
    0xb28700d0U, 0x2f503869U, 0x97ec5f0cU, 0x8559f0e2U, 0x3de59787U,
    0x658687d1U, 0xdd3ae0b4U, 0xcf8f4f5aU, 0x7733283fU, 0xeae41086U,
    0x525877e3U, 0x40edd80dU, 0xf851bf68U, 0xf02bf8a1U, 0x48979fc4U,
    0x5a22302aU, 0xe29e574fU, 0x7f496ff6U, 0xc7f50893U, 0xd540a77dU,
    0x6dfcc018U, 0x359fd04eU, 0x8d23b72bU, 0x9f9618c5U, 0x272a7fa0U,
    0xbafd4719U, 0x0241207cU, 0x10f48f92U, 0xa848e8f7U, 0x9b14583dU,
    0x23a83f58U, 0x311d90b6U, 0x89a1f7d3U, 0x1476cf6aU, 0xaccaa80fU,
    0xbe7f07e1U, 0x06c36084U, 0x5ea070d2U, 0xe61c17b7U, 0xf4a9b859U,
    0x4c15df3cU, 0xd1c2e785U, 0x697e80e0U, 0x7bcb2f0eU, 0xc377486bU,
    0xcb0d0fa2U, 0x73b168c7U, 0x6104c729U, 0xd9b8a04cU, 0x446f98f5U,
    0xfcd3ff90U, 0xee66507eU, 0x56da371bU, 0x0eb9274dU, 0xb6054028U,
    0xa4b0efc6U, 0x1c0c88a3U, 0x81dbb01aU, 0x3967d77fU, 0x2bd27891U,
    0x936e1ff4U, 0x3b26f703U, 0x839a9066U, 0x912f3f88U, 0x299358edU,
    0xb4446054U, 0x0cf80731U, 0x1e4da8dfU, 0xa6f1cfbaU, 0xfe92dfecU,
    0x462eb889U, 0x549b1767U, 0xec277002U, 0x71f048bbU, 0xc94c2fdeU,
    0xdbf98030U, 0x6345e755U, 0x6b3fa09cU, 0xd383c7f9U, 0xc1366817U,
    0x798a0f72U, 0xe45d37cbU, 0x5ce150aeU, 0x4e54ff40U, 0xf6e89825U,
    0xae8b8873U, 0x1637ef16U, 0x048240f8U, 0xbc3e279dU, 0x21e91f24U,
    0x99557841U, 0x8be0d7afU, 0x335cb0caU, 0xed59b63bU, 0x55e5d15eU,
    0x47507eb0U, 0xffec19d5U, 0x623b216cU, 0xda874609U, 0xc832e9e7U,
    0x708e8e82U, 0x28ed9ed4U, 0x9051f9b1U, 0x82e4565fU, 0x3a58313aU,
    0xa78f0983U, 0x1f336ee6U, 0x0d86c108U, 0xb53aa66dU, 0xbd40e1a4U,
    0x05fc86c1U, 0x1749292fU, 0xaff54e4aU, 0x322276f3U, 0x8a9e1196U,
    0x982bbe78U, 0x2097d91dU, 0x78f4c94bU, 0xc048ae2eU, 0xd2fd01c0U,
    0x6a4166a5U, 0xf7965e1cU, 0x4f2a3979U, 0x5d9f9697U, 0xe523f1f2U,
    0x4d6b1905U, 0xf5d77e60U, 0xe762d18eU, 0x5fdeb6ebU, 0xc2098e52U,
    0x7ab5e937U, 0x680046d9U, 0xd0bc21bcU, 0x88df31eaU, 0x3063568fU,
    0x22d6f961U, 0x9a6a9e04U, 0x07bda6bdU, 0xbf01c1d8U, 0xadb46e36U,
    0x15080953U, 0x1d724e9aU, 0xa5ce29ffU, 0xb77b8611U, 0x0fc7e174U,
    0x9210d9cdU, 0x2aacbea8U, 0x38191146U, 0x80a57623U, 0xd8c66675U,
    0x607a0110U, 0x72cfaefeU, 0xca73c99bU, 0x57a4f122U, 0xef189647U,
    0xfdad39a9U, 0x45115eccU, 0x764dee06U, 0xcef18963U, 0xdc44268dU,
    0x64f841e8U, 0xf92f7951U, 0x41931e34U, 0x5326b1daU, 0xeb9ad6bfU,
    0xb3f9c6e9U, 0x0b45a18cU, 0x19f00e62U, 0xa14c6907U, 0x3c9b51beU,
    0x842736dbU, 0x96929935U, 0x2e2efe50U, 0x2654b999U, 0x9ee8defcU,
    0x8c5d7112U, 0x34e11677U, 0xa9362eceU, 0x118a49abU, 0x033fe645U,
    0xbb838120U, 0xe3e09176U, 0x5b5cf613U, 0x49e959fdU, 0xf1553e98U,
    0x6c820621U, 0xd43e6144U, 0xc68bceaaU, 0x7e37a9cfU, 0xd67f4138U,
    0x6ec3265dU, 0x7c7689b3U, 0xc4caeed6U, 0x591dd66fU, 0xe1a1b10aU,
    0xf3141ee4U, 0x4ba87981U, 0x13cb69d7U, 0xab770eb2U, 0xb9c2a15cU,
    0x017ec639U, 0x9ca9fe80U, 0x241599e5U, 0x36a0360bU, 0x8e1c516eU,
    0x866616a7U, 0x3eda71c2U, 0x2c6fde2cU, 0x94d3b949U, 0x090481f0U,
    0xb1b8e695U, 0xa30d497bU, 0x1bb12e1eU, 0x43d23e48U, 0xfb6e592dU,
    0xe9dbf6c3U, 0x516791a6U, 0xccb0a91fU, 0x740cce7aU, 0x66b96194U,
    0xde0506f1U
  }
};
static const unsigned int crc32tab_be[4][256] = /* big endian */
{
  {
    0x00000000U, 0x96300777U, 0x2c610eeeU, 0xba510999U, 0x19c46d07U,
    0x8ff46a70U, 0x35a563e9U, 0xa395649eU, 0x3288db0eU, 0xa4b8dc79U,
    0x1ee9d5e0U, 0x88d9d297U, 0x2b4cb609U, 0xbd7cb17eU, 0x072db8e7U,
    0x911dbf90U, 0x6410b71dU, 0xf220b06aU, 0x4871b9f3U, 0xde41be84U,
    0x7dd4da1aU, 0xebe4dd6dU, 0x51b5d4f4U, 0xc785d383U, 0x56986c13U,
    0xc0a86b64U, 0x7af962fdU, 0xecc9658aU, 0x4f5c0114U, 0xd96c0663U,
    0x633d0ffaU, 0xf50d088dU, 0xc8206e3bU, 0x5e10694cU, 0xe44160d5U,
    0x727167a2U, 0xd1e4033cU, 0x47d4044bU, 0xfd850dd2U, 0x6bb50aa5U,
    0xfaa8b535U, 0x6c98b242U, 0xd6c9bbdbU, 0x40f9bcacU, 0xe36cd832U,
    0x755cdf45U, 0xcf0dd6dcU, 0x593dd1abU, 0xac30d926U, 0x3a00de51U,
    0x8051d7c8U, 0x1661d0bfU, 0xb5f4b421U, 0x23c4b356U, 0x9995bacfU,
    0x0fa5bdb8U, 0x9eb80228U, 0x0888055fU, 0xb2d90cc6U, 0x24e90bb1U,
    0x877c6f2fU, 0x114c6858U, 0xab1d61c1U, 0x3d2d66b6U, 0x9041dc76U,
    0x0671db01U, 0xbc20d298U, 0x2a10d5efU, 0x8985b171U, 0x1fb5b606U,
    0xa5e4bf9fU, 0x33d4b8e8U, 0xa2c90778U, 0x34f9000fU, 0x8ea80996U,
    0x18980ee1U, 0xbb0d6a7fU, 0x2d3d6d08U, 0x976c6491U, 0x015c63e6U,
    0xf4516b6bU, 0x62616c1cU, 0xd8306585U, 0x4e0062f2U, 0xed95066cU,
    0x7ba5011bU, 0xc1f40882U, 0x57c40ff5U, 0xc6d9b065U, 0x50e9b712U,
    0xeab8be8bU, 0x7c88b9fcU, 0xdf1ddd62U, 0x492dda15U, 0xf37cd38cU,
    0x654cd4fbU, 0x5861b24dU, 0xce51b53aU, 0x7400bca3U, 0xe230bbd4U,
    0x41a5df4aU, 0xd795d83dU, 0x6dc4d1a4U, 0xfbf4d6d3U, 0x6ae96943U,
    0xfcd96e34U, 0x468867adU, 0xd0b860daU, 0x732d0444U, 0xe51d0333U,
    0x5f4c0aaaU, 0xc97c0dddU, 0x3c710550U, 0xaa410227U, 0x10100bbeU,
    0x86200cc9U, 0x25b56857U, 0xb3856f20U, 0x09d466b9U, 0x9fe461ceU,
    0x0ef9de5eU, 0x98c9d929U, 0x2298d0b0U, 0xb4a8d7c7U, 0x173db359U,
    0x810db42eU, 0x3b5cbdb7U, 0xad6cbac0U, 0x2083b8edU, 0xb6b3bf9aU,
    0x0ce2b603U, 0x9ad2b174U, 0x3947d5eaU, 0xaf77d29dU, 0x1526db04U,
    0x8316dc73U, 0x120b63e3U, 0x843b6494U, 0x3e6a6d0dU, 0xa85a6a7aU,
    0x0bcf0ee4U, 0x9dff0993U, 0x27ae000aU, 0xb19e077dU, 0x44930ff0U,
    0xd2a30887U, 0x68f2011eU, 0xfec20669U, 0x5d5762f7U, 0xcb676580U,
    0x71366c19U, 0xe7066b6eU, 0x761bd4feU, 0xe02bd389U, 0x5a7ada10U,
    0xcc4add67U, 0x6fdfb9f9U, 0xf9efbe8eU, 0x43beb717U, 0xd58eb060U,
    0xe8a3d6d6U, 0x7e93d1a1U, 0xc4c2d838U, 0x52f2df4fU, 0xf167bbd1U,
    0x6757bca6U, 0xdd06b53fU, 0x4b36b248U, 0xda2b0dd8U, 0x4c1b0aafU,
    0xf64a0336U, 0x607a0441U, 0xc3ef60dfU, 0x55df67a8U, 0xef8e6e31U,
    0x79be6946U, 0x8cb361cbU, 0x1a8366bcU, 0xa0d26f25U, 0x36e26852U,
    0x95770cccU, 0x03470bbbU, 0xb9160222U, 0x2f260555U, 0xbe3bbac5U,
    0x280bbdb2U, 0x925ab42bU, 0x046ab35cU, 0xa7ffd7c2U, 0x31cfd0b5U,
    0x8b9ed92cU, 0x1daede5bU, 0xb0c2649bU, 0x26f263ecU, 0x9ca36a75U,
    0x0a936d02U, 0xa906099cU, 0x3f360eebU, 0x85670772U, 0x13570005U,
    0x824abf95U, 0x147ab8e2U, 0xae2bb17bU, 0x381bb60cU, 0x9b8ed292U,
    0x0dbed5e5U, 0xb7efdc7cU, 0x21dfdb0bU, 0xd4d2d386U, 0x42e2d4f1U,
    0xf8b3dd68U, 0x6e83da1fU, 0xcd16be81U, 0x5b26b9f6U, 0xe177b06fU,
    0x7747b718U, 0xe65a0888U, 0x706a0fffU, 0xca3b0666U, 0x5c0b0111U,
    0xff9e658fU, 0x69ae62f8U, 0xd3ff6b61U, 0x45cf6c16U, 0x78e20aa0U,
    0xeed20dd7U, 0x5483044eU, 0xc2b30339U, 0x612667a7U, 0xf71660d0U,
    0x4d476949U, 0xdb776e3eU, 0x4a6ad1aeU, 0xdc5ad6d9U, 0x660bdf40U,
    0xf03bd837U, 0x53aebca9U, 0xc59ebbdeU, 0x7fcfb247U, 0xe9ffb530U,
    0x1cf2bdbdU, 0x8ac2bacaU, 0x3093b353U, 0xa6a3b424U, 0x0536d0baU,
    0x9306d7cdU, 0x2957de54U, 0xbf67d923U, 0x2e7a66b3U, 0xb84a61c4U,
    0x021b685dU, 0x942b6f2aU, 0x37be0bb4U, 0xa18e0cc3U, 0x1bdf055aU,
    0x8def022dU
  },
  {
    0x00000000U, 0x41311b19U, 0x82623632U, 0xc3532d2bU, 0x04c56c64U,
    0x45f4777dU, 0x86a75a56U, 0xc796414fU, 0x088ad9c8U, 0x49bbc2d1U,
    0x8ae8effaU, 0xcbd9f4e3U, 0x0c4fb5acU, 0x4d7eaeb5U, 0x8e2d839eU,
    0xcf1c9887U, 0x5112c24aU, 0x1023d953U, 0xd370f478U, 0x9241ef61U,
    0x55d7ae2eU, 0x14e6b537U, 0xd7b5981cU, 0x96848305U, 0x59981b82U,
    0x18a9009bU, 0xdbfa2db0U, 0x9acb36a9U, 0x5d5d77e6U, 0x1c6c6cffU,
    0xdf3f41d4U, 0x9e0e5acdU, 0xa2248495U, 0xe3159f8cU, 0x2046b2a7U,
    0x6177a9beU, 0xa6e1e8f1U, 0xe7d0f3e8U, 0x2483dec3U, 0x65b2c5daU,
    0xaaae5d5dU, 0xeb9f4644U, 0x28cc6b6fU, 0x69fd7076U, 0xae6b3139U,
    0xef5a2a20U, 0x2c09070bU, 0x6d381c12U, 0xf33646dfU, 0xb2075dc6U,
    0x715470edU, 0x30656bf4U, 0xf7f32abbU, 0xb6c231a2U, 0x75911c89U,
    0x34a00790U, 0xfbbc9f17U, 0xba8d840eU, 0x79dea925U, 0x38efb23cU,
    0xff79f373U, 0xbe48e86aU, 0x7d1bc541U, 0x3c2ade58U, 0x054f79f0U,
    0x447e62e9U, 0x872d4fc2U, 0xc61c54dbU, 0x018a1594U, 0x40bb0e8dU,
    0x83e823a6U, 0xc2d938bfU, 0x0dc5a038U, 0x4cf4bb21U, 0x8fa7960aU,
    0xce968d13U, 0x0900cc5cU, 0x4831d745U, 0x8b62fa6eU, 0xca53e177U,
    0x545dbbbaU, 0x156ca0a3U, 0xd63f8d88U, 0x970e9691U, 0x5098d7deU,
    0x11a9ccc7U, 0xd2fae1ecU, 0x93cbfaf5U, 0x5cd76272U, 0x1de6796bU,
    0xdeb55440U, 0x9f844f59U, 0x58120e16U, 0x1923150fU, 0xda703824U,
    0x9b41233dU, 0xa76bfd65U, 0xe65ae67cU, 0x2509cb57U, 0x6438d04eU,
    0xa3ae9101U, 0xe29f8a18U, 0x21cca733U, 0x60fdbc2aU, 0xafe124adU,
    0xeed03fb4U, 0x2d83129fU, 0x6cb20986U, 0xab2448c9U, 0xea1553d0U,
    0x29467efbU, 0x687765e2U, 0xf6793f2fU, 0xb7482436U, 0x741b091dU,
    0x352a1204U, 0xf2bc534bU, 0xb38d4852U, 0x70de6579U, 0x31ef7e60U,
    0xfef3e6e7U, 0xbfc2fdfeU, 0x7c91d0d5U, 0x3da0cbccU, 0xfa368a83U,
    0xbb07919aU, 0x7854bcb1U, 0x3965a7a8U, 0x4b98833bU, 0x0aa99822U,
    0xc9fab509U, 0x88cbae10U, 0x4f5def5fU, 0x0e6cf446U, 0xcd3fd96dU,
    0x8c0ec274U, 0x43125af3U, 0x022341eaU, 0xc1706cc1U, 0x804177d8U,
    0x47d73697U, 0x06e62d8eU, 0xc5b500a5U, 0x84841bbcU, 0x1a8a4171U,
    0x5bbb5a68U, 0x98e87743U, 0xd9d96c5aU, 0x1e4f2d15U, 0x5f7e360cU,
    0x9c2d1b27U, 0xdd1c003eU, 0x120098b9U, 0x533183a0U, 0x9062ae8bU,
    0xd153b592U, 0x16c5f4ddU, 0x57f4efc4U, 0x94a7c2efU, 0xd596d9f6U,
    0xe9bc07aeU, 0xa88d1cb7U, 0x6bde319cU, 0x2aef2a85U, 0xed796bcaU,
    0xac4870d3U, 0x6f1b5df8U, 0x2e2a46e1U, 0xe136de66U, 0xa007c57fU,
    0x6354e854U, 0x2265f34dU, 0xe5f3b202U, 0xa4c2a91bU, 0x67918430U,
    0x26a09f29U, 0xb8aec5e4U, 0xf99fdefdU, 0x3accf3d6U, 0x7bfde8cfU,
    0xbc6ba980U, 0xfd5ab299U, 0x3e099fb2U, 0x7f3884abU, 0xb0241c2cU,
    0xf1150735U, 0x32462a1eU, 0x73773107U, 0xb4e17048U, 0xf5d06b51U,
    0x3683467aU, 0x77b25d63U, 0x4ed7facbU, 0x0fe6e1d2U, 0xccb5ccf9U,
    0x8d84d7e0U, 0x4a1296afU, 0x0b238db6U, 0xc870a09dU, 0x8941bb84U,
    0x465d2303U, 0x076c381aU, 0xc43f1531U, 0x850e0e28U, 0x42984f67U,
    0x03a9547eU, 0xc0fa7955U, 0x81cb624cU, 0x1fc53881U, 0x5ef42398U,
    0x9da70eb3U, 0xdc9615aaU, 0x1b0054e5U, 0x5a314ffcU, 0x996262d7U,
    0xd85379ceU, 0x174fe149U, 0x567efa50U, 0x952dd77bU, 0xd41ccc62U,
    0x138a8d2dU, 0x52bb9634U, 0x91e8bb1fU, 0xd0d9a006U, 0xecf37e5eU,
    0xadc26547U, 0x6e91486cU, 0x2fa05375U, 0xe836123aU, 0xa9070923U,
    0x6a542408U, 0x2b653f11U, 0xe479a796U, 0xa548bc8fU, 0x661b91a4U,
    0x272a8abdU, 0xe0bccbf2U, 0xa18dd0ebU, 0x62defdc0U, 0x23efe6d9U,
    0xbde1bc14U, 0xfcd0a70dU, 0x3f838a26U, 0x7eb2913fU, 0xb924d070U,
    0xf815cb69U, 0x3b46e642U, 0x7a77fd5bU, 0xb56b65dcU, 0xf45a7ec5U,
    0x370953eeU, 0x763848f7U, 0xb1ae09b8U, 0xf09f12a1U, 0x33cc3f8aU,
    0x72fd2493U
  },
  {
    0x00000000U, 0x376ac201U, 0x6ed48403U, 0x59be4602U, 0xdca80907U,
    0xebc2cb06U, 0xb27c8d04U, 0x85164f05U, 0xb851130eU, 0x8f3bd10fU,
    0xd685970dU, 0xe1ef550cU, 0x64f91a09U, 0x5393d808U, 0x0a2d9e0aU,
    0x3d475c0bU, 0x70a3261cU, 0x47c9e41dU, 0x1e77a21fU, 0x291d601eU,
    0xac0b2f1bU, 0x9b61ed1aU, 0xc2dfab18U, 0xf5b56919U, 0xc8f23512U,
    0xff98f713U, 0xa626b111U, 0x914c7310U, 0x145a3c15U, 0x2330fe14U,
    0x7a8eb816U, 0x4de47a17U, 0xe0464d38U, 0xd72c8f39U, 0x8e92c93bU,
    0xb9f80b3aU, 0x3cee443fU, 0x0b84863eU, 0x523ac03cU, 0x6550023dU,
    0x58175e36U, 0x6f7d9c37U, 0x36c3da35U, 0x01a91834U, 0x84bf5731U,
    0xb3d59530U, 0xea6bd332U, 0xdd011133U, 0x90e56b24U, 0xa78fa925U,
    0xfe31ef27U, 0xc95b2d26U, 0x4c4d6223U, 0x7b27a022U, 0x2299e620U,
    0x15f32421U, 0x28b4782aU, 0x1fdeba2bU, 0x4660fc29U, 0x710a3e28U,
    0xf41c712dU, 0xc376b32cU, 0x9ac8f52eU, 0xada2372fU, 0xc08d9a70U,
    0xf7e75871U, 0xae591e73U, 0x9933dc72U, 0x1c259377U, 0x2b4f5176U,
    0x72f11774U, 0x459bd575U, 0x78dc897eU, 0x4fb64b7fU, 0x16080d7dU,
    0x2162cf7cU, 0xa4748079U, 0x931e4278U, 0xcaa0047aU, 0xfdcac67bU,
    0xb02ebc6cU, 0x87447e6dU, 0xdefa386fU, 0xe990fa6eU, 0x6c86b56bU,
    0x5bec776aU, 0x02523168U, 0x3538f369U, 0x087faf62U, 0x3f156d63U,
    0x66ab2b61U, 0x51c1e960U, 0xd4d7a665U, 0xe3bd6464U, 0xba032266U,
    0x8d69e067U, 0x20cbd748U, 0x17a11549U, 0x4e1f534bU, 0x7975914aU,
    0xfc63de4fU, 0xcb091c4eU, 0x92b75a4cU, 0xa5dd984dU, 0x989ac446U,
    0xaff00647U, 0xf64e4045U, 0xc1248244U, 0x4432cd41U, 0x73580f40U,
    0x2ae64942U, 0x1d8c8b43U, 0x5068f154U, 0x67023355U, 0x3ebc7557U,
    0x09d6b756U, 0x8cc0f853U, 0xbbaa3a52U, 0xe2147c50U, 0xd57ebe51U,
    0xe839e25aU, 0xdf53205bU, 0x86ed6659U, 0xb187a458U, 0x3491eb5dU,
    0x03fb295cU, 0x5a456f5eU, 0x6d2fad5fU, 0x801b35e1U, 0xb771f7e0U,
    0xeecfb1e2U, 0xd9a573e3U, 0x5cb33ce6U, 0x6bd9fee7U, 0x3267b8e5U,
    0x050d7ae4U, 0x384a26efU, 0x0f20e4eeU, 0x569ea2ecU, 0x61f460edU,
    0xe4e22fe8U, 0xd388ede9U, 0x8a36abebU, 0xbd5c69eaU, 0xf0b813fdU,
    0xc7d2d1fcU, 0x9e6c97feU, 0xa90655ffU, 0x2c101afaU, 0x1b7ad8fbU,
    0x42c49ef9U, 0x75ae5cf8U, 0x48e900f3U, 0x7f83c2f2U, 0x263d84f0U,
    0x115746f1U, 0x944109f4U, 0xa32bcbf5U, 0xfa958df7U, 0xcdff4ff6U,
    0x605d78d9U, 0x5737bad8U, 0x0e89fcdaU, 0x39e33edbU, 0xbcf571deU,
    0x8b9fb3dfU, 0xd221f5ddU, 0xe54b37dcU, 0xd80c6bd7U, 0xef66a9d6U,
    0xb6d8efd4U, 0x81b22dd5U, 0x04a462d0U, 0x33cea0d1U, 0x6a70e6d3U,
    0x5d1a24d2U, 0x10fe5ec5U, 0x27949cc4U, 0x7e2adac6U, 0x494018c7U,
    0xcc5657c2U, 0xfb3c95c3U, 0xa282d3c1U, 0x95e811c0U, 0xa8af4dcbU,
    0x9fc58fcaU, 0xc67bc9c8U, 0xf1110bc9U, 0x740744ccU, 0x436d86cdU,
    0x1ad3c0cfU, 0x2db902ceU, 0x4096af91U, 0x77fc6d90U, 0x2e422b92U,
    0x1928e993U, 0x9c3ea696U, 0xab546497U, 0xf2ea2295U, 0xc580e094U,
    0xf8c7bc9fU, 0xcfad7e9eU, 0x9613389cU, 0xa179fa9dU, 0x246fb598U,
    0x13057799U, 0x4abb319bU, 0x7dd1f39aU, 0x3035898dU, 0x075f4b8cU,
    0x5ee10d8eU, 0x698bcf8fU, 0xec9d808aU, 0xdbf7428bU, 0x82490489U,
    0xb523c688U, 0x88649a83U, 0xbf0e5882U, 0xe6b01e80U, 0xd1dadc81U,
    0x54cc9384U, 0x63a65185U, 0x3a181787U, 0x0d72d586U, 0xa0d0e2a9U,
    0x97ba20a8U, 0xce0466aaU, 0xf96ea4abU, 0x7c78ebaeU, 0x4b1229afU,
    0x12ac6fadU, 0x25c6adacU, 0x1881f1a7U, 0x2feb33a6U, 0x765575a4U,
    0x413fb7a5U, 0xc429f8a0U, 0xf3433aa1U, 0xaafd7ca3U, 0x9d97bea2U,
    0xd073c4b5U, 0xe71906b4U, 0xbea740b6U, 0x89cd82b7U, 0x0cdbcdb2U,
    0x3bb10fb3U, 0x620f49b1U, 0x55658bb0U, 0x6822d7bbU, 0x5f4815baU,
    0x06f653b8U, 0x319c91b9U, 0xb48adebcU, 0x83e01cbdU, 0xda5e5abfU,
    0xed3498beU
  },
  {
    0x00000000U, 0x6567bcb8U, 0x8bc809aaU, 0xeeafb512U, 0x5797628fU,
    0x32f0de37U, 0xdc5f6b25U, 0xb938d79dU, 0xef28b4c5U, 0x8a4f087dU,
    0x64e0bd6fU, 0x018701d7U, 0xb8bfd64aU, 0xddd86af2U, 0x3377dfe0U,
    0x56106358U, 0x9f571950U, 0xfa30a5e8U, 0x149f10faU, 0x71f8ac42U,
    0xc8c07bdfU, 0xada7c767U, 0x43087275U, 0x266fcecdU, 0x707fad95U,
    0x1518112dU, 0xfbb7a43fU, 0x9ed01887U, 0x27e8cf1aU, 0x428f73a2U,
    0xac20c6b0U, 0xc9477a08U, 0x3eaf32a0U, 0x5bc88e18U, 0xb5673b0aU,
    0xd00087b2U, 0x6938502fU, 0x0c5fec97U, 0xe2f05985U, 0x8797e53dU,
    0xd1878665U, 0xb4e03addU, 0x5a4f8fcfU, 0x3f283377U, 0x8610e4eaU,
    0xe3775852U, 0x0dd8ed40U, 0x68bf51f8U, 0xa1f82bf0U, 0xc49f9748U,
    0x2a30225aU, 0x4f579ee2U, 0xf66f497fU, 0x9308f5c7U, 0x7da740d5U,
    0x18c0fc6dU, 0x4ed09f35U, 0x2bb7238dU, 0xc518969fU, 0xa07f2a27U,
    0x1947fdbaU, 0x7c204102U, 0x928ff410U, 0xf7e848a8U, 0x3d58149bU,
    0x583fa823U, 0xb6901d31U, 0xd3f7a189U, 0x6acf7614U, 0x0fa8caacU,
    0xe1077fbeU, 0x8460c306U, 0xd270a05eU, 0xb7171ce6U, 0x59b8a9f4U,
    0x3cdf154cU, 0x85e7c2d1U, 0xe0807e69U, 0x0e2fcb7bU, 0x6b4877c3U,
    0xa20f0dcbU, 0xc768b173U, 0x29c70461U, 0x4ca0b8d9U, 0xf5986f44U,
    0x90ffd3fcU, 0x7e5066eeU, 0x1b37da56U, 0x4d27b90eU, 0x284005b6U,
    0xc6efb0a4U, 0xa3880c1cU, 0x1ab0db81U, 0x7fd76739U, 0x9178d22bU,
    0xf41f6e93U, 0x03f7263bU, 0x66909a83U, 0x883f2f91U, 0xed589329U,
    0x546044b4U, 0x3107f80cU, 0xdfa84d1eU, 0xbacff1a6U, 0xecdf92feU,
    0x89b82e46U, 0x67179b54U, 0x027027ecU, 0xbb48f071U, 0xde2f4cc9U,
    0x3080f9dbU, 0x55e74563U, 0x9ca03f6bU, 0xf9c783d3U, 0x176836c1U,
    0x720f8a79U, 0xcb375de4U, 0xae50e15cU, 0x40ff544eU, 0x2598e8f6U,
    0x73888baeU, 0x16ef3716U, 0xf8408204U, 0x9d273ebcU, 0x241fe921U,
    0x41785599U, 0xafd7e08bU, 0xcab05c33U, 0x3bb659edU, 0x5ed1e555U,
    0xb07e5047U, 0xd519ecffU, 0x6c213b62U, 0x094687daU, 0xe7e932c8U,
    0x828e8e70U, 0xd49eed28U, 0xb1f95190U, 0x5f56e482U, 0x3a31583aU,
    0x83098fa7U, 0xe66e331fU, 0x08c1860dU, 0x6da63ab5U, 0xa4e140bdU,
    0xc186fc05U, 0x2f294917U, 0x4a4ef5afU, 0xf3762232U, 0x96119e8aU,
    0x78be2b98U, 0x1dd99720U, 0x4bc9f478U, 0x2eae48c0U, 0xc001fdd2U,
    0xa566416aU, 0x1c5e96f7U, 0x79392a4fU, 0x97969f5dU, 0xf2f123e5U,
    0x05196b4dU, 0x607ed7f5U, 0x8ed162e7U, 0xebb6de5fU, 0x528e09c2U,
    0x37e9b57aU, 0xd9460068U, 0xbc21bcd0U, 0xea31df88U, 0x8f566330U,
    0x61f9d622U, 0x049e6a9aU, 0xbda6bd07U, 0xd8c101bfU, 0x366eb4adU,
    0x53090815U, 0x9a4e721dU, 0xff29cea5U, 0x11867bb7U, 0x74e1c70fU,
    0xcdd91092U, 0xa8beac2aU, 0x46111938U, 0x2376a580U, 0x7566c6d8U,
    0x10017a60U, 0xfeaecf72U, 0x9bc973caU, 0x22f1a457U, 0x479618efU,
    0xa939adfdU, 0xcc5e1145U, 0x06ee4d76U, 0x6389f1ceU, 0x8d2644dcU,
    0xe841f864U, 0x51792ff9U, 0x341e9341U, 0xdab12653U, 0xbfd69aebU,
    0xe9c6f9b3U, 0x8ca1450bU, 0x620ef019U, 0x07694ca1U, 0xbe519b3cU,
    0xdb362784U, 0x35999296U, 0x50fe2e2eU, 0x99b95426U, 0xfcdee89eU,
    0x12715d8cU, 0x7716e134U, 0xce2e36a9U, 0xab498a11U, 0x45e63f03U,
    0x208183bbU, 0x7691e0e3U, 0x13f65c5bU, 0xfd59e949U, 0x983e55f1U,
    0x2106826cU, 0x44613ed4U, 0xaace8bc6U, 0xcfa9377eU, 0x38417fd6U,
    0x5d26c36eU, 0xb389767cU, 0xd6eecac4U, 0x6fd61d59U, 0x0ab1a1e1U,
    0xe41e14f3U, 0x8179a84bU, 0xd769cb13U, 0xb20e77abU, 0x5ca1c2b9U,
    0x39c67e01U, 0x80fea99cU, 0xe5991524U, 0x0b36a036U, 0x6e511c8eU,
    0xa7166686U, 0xc271da3eU, 0x2cde6f2cU, 0x49b9d394U, 0xf0810409U,
    0x95e6b8b1U, 0x7b490da3U, 0x1e2eb11bU, 0x483ed243U, 0x2d596efbU,
    0xc3f6dbe9U, 0xa6916751U, 0x1fa9b0ccU, 0x7ace0c74U, 0x9461b966U,
    0xf10605deU
  }
};


unsigned int
Hash32::crc( const byte *buf,  unsigned int bufLen,  unsigned int key )
{
  register const byte * endBuf = &buf[ bufLen ];
  key = ~key;

  if ( Aligned::isLittleEndian ) {
#if ! defined( __amd64__ ) && ! defined( __i386 ) /* this slows the hash */
    if ( buf == endBuf )
      return ~key;
    /* calc bytes until align on int boundary */
    while ( ( ( (ulongptr) buf ) & ( sizeof( unsigned int ) - 1 ) ) != 0 ) {
      key = crc32tab_le[ 0 ][ (byte) key ^ buf[ 0 ] ] ^ ( key >> 8 );
      if ( ++buf == endBuf )
        break;
    }
#endif
    /* calc ints by 4 */
    while ( &buf[ sizeof( unsigned int ) * 2 ] <= endBuf ) {
      key ^= ((const unsigned int *) buf)[ 0 ];
      key  = crc32tab_le[ 3 ][ (byte) key ]           ^
             crc32tab_le[ 2 ][ (byte) ( key >> 8 ) ]  ^
             crc32tab_le[ 1 ][ (byte) ( key >> 16 ) ] ^
             crc32tab_le[ 0 ][ (byte) ( key >> 24 ) ];
      key ^= ((const unsigned int *) buf)[ 1 ];
      key  = crc32tab_le[ 3 ][ (byte) key ]           ^
             crc32tab_le[ 2 ][ (byte) ( key >> 8 ) ]  ^
             crc32tab_le[ 1 ][ (byte) ( key >> 16 ) ] ^
             crc32tab_le[ 0 ][ (byte) ( key >> 24 ) ];
      buf  = (const byte *) &((const unsigned int *) buf)[ 2 ];
    }
    /* calc ints by 1 */
    while ( &buf[ sizeof( unsigned int ) ] <= endBuf ) {
      key ^= ((const unsigned int *) buf)[ 0 ];
      key  = crc32tab_le[ 3 ][ (byte) key ]           ^
             crc32tab_le[ 2 ][ (byte) ( key >> 8 ) ]  ^
             crc32tab_le[ 1 ][ (byte) ( key >> 16 ) ] ^
             crc32tab_le[ 0 ][ (byte) ( key >> 24 ) ];
      buf  = (const byte *) &((const unsigned int *) buf)[ 1 ];
    }
    /* calc bytes */
    while ( endBuf > buf ) {
      key = crc32tab_le[ 0 ][ (byte) key ^ buf[ 0 ] ] ^ ( key >> 8 );
      buf++;
    }
  }
  else {
    if ( buf == endBuf )
      return ~key;
    /* bswap */
    Aligned::swap( key );

    /* calc bytes until align on int boundary */
    while ( ( ( (ulongptr) buf ) & ( sizeof( unsigned int ) - 1 ) ) != 0 ) {
      key = crc32tab_be[ 0 ][ ( key >> 24 ) ^ buf[ 0 ] ] ^ ( key << 8 );
      if ( ++buf == endBuf )
        break;
    }
    /* calc ints by 4 */
    while ( &buf[ sizeof( unsigned int ) * 2 ] <= endBuf ) {
      key ^= ((const unsigned int *) buf)[ 0 ];
      key  = crc32tab_be[ 0 ][ (byte) key ]           ^
             crc32tab_be[ 1 ][ (byte) ( key >> 8 ) ]  ^
             crc32tab_be[ 2 ][ (byte) ( key >> 16 ) ] ^
             crc32tab_be[ 3 ][ (byte) ( key >> 24 ) ];
      key ^= ((const unsigned int *) buf)[ 1 ];
      key  = crc32tab_be[ 0 ][ (byte) key ]           ^
             crc32tab_be[ 1 ][ (byte) ( key >> 8 ) ]  ^
             crc32tab_be[ 2 ][ (byte) ( key >> 16 ) ] ^
             crc32tab_be[ 3 ][ (byte) ( key >> 24 ) ];
      buf  = (const byte *) &((const unsigned int *) buf)[ 2 ];
    }
    /* calc ints by 1 */
    while ( &buf[ sizeof( unsigned int ) ] <= endBuf ) {
      key ^= ((const unsigned int *) buf)[ 0 ];
      key  = crc32tab_be[ 0 ][ (byte) key ]           ^
             crc32tab_be[ 1 ][ (byte) ( key >> 8 ) ]  ^
             crc32tab_be[ 2 ][ (byte) ( key >> 16 ) ] ^
             crc32tab_be[ 3 ][ (byte) ( key >> 24 ) ];
      buf  = (const byte *) &((const unsigned int *) buf)[ 1 ];
    }
    /* calc bytes */
    while ( endBuf > buf ) {
      key = crc32tab_be[ 0 ][ ( key >> 24 ) ^ buf[ 0 ] ] ^ ( key << 8 );
      buf++;
    }
    /* bswap */
    Aligned::swap( key );
  }

  return ~key;
}

#if 0
static unsigned int
crc_nocase( const byte *buf,  unsigned int bufLen,  unsigned int key )
{
  const byte * endBuf;

  endBuf = &buf[ bufLen ];
  key    = ~key;

  do {
    key = crc32tab_le[ 0 ][ (byte) key ^ nocase( buf[ 0 ] ) ] ^ ( key >> 8 );
  } while ( ++buf < endBuf );

  return ~key;
}
#endif

/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-2003 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
unsigned int
Hash32::adler( const byte *buf,  unsigned int len,  unsigned int s1 )
{
  static const unsigned int BASE = 65521U;/* largest prime smaller than 65536 */
  static const unsigned int NMAX = 5552U; /* NMAX is the largest n such that
                                        255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */
  unsigned int s2 = ( s1 >> 16 ) & 0xffffU, k;
  s1 &= 0xffffU;

  while ( len > 0 ) {
    k = len < NMAX ? (unsigned int) len : NMAX;
    len -= k;
    while ( k >= 4 ) {
      s1 += buf[ 0 ]; s2 += s1;
      s1 += buf[ 1 ]; s2 += s1;
      s1 += buf[ 2 ]; s2 += s1;
      s1 += buf[ 3 ]; s2 += s1;
      buf += 4;
      k   -= 4;
    }
    while ( k > 0 ) {
      s1 += *buf++; s2 += s1; k--;
    }
    s1 %= BASE;
    s2 %= BASE;
  }

  return ( s2 << 16 ) | s1;
}


template <unsigned int NBITS, unsigned int BLKSZ,  bool outputIsLittle,
          class StateInt>
struct RSA_Hash : public HashContext {
  
  /* 256/32 = 64 */
  static const unsigned int STATESZ = NBITS / ( sizeof( StateInt ) * 8 );
  ullong   count;
  StateInt state[ STATESZ ];
  byte     block[ BLKSZ ];

  virtual void init( void ) = 0;

  virtual void transform( StateInt *x ) = 0;

  virtual void update( const byte *input,  unsigned int inputLen ) {
    unsigned int i, j;

    /* Add count to MDp->count */
    i = (unsigned int) ( this->count & ( ( BLKSZ << 3 ) - 1 ) ) >> 3;
    this->count += (ullong) inputLen << 3;

    if ( i > 0 && i + inputLen > BLKSZ ) {
      j = BLKSZ - i;
      ::memcpy( &this->block[ i ], input, j );
      this->transform( (StateInt *) this->block );
      inputLen -= j;
      input = &input[ j ];
      i = 0;
    }

    /* Process data */
    while ( inputLen >= BLKSZ ) {
      if ( ( (ulongptr) (void *) input &
              ( sizeof( unsigned int ) - 1 ) ) != 0 ) {
        ::memcpy( this->block, input, BLKSZ );
        this->transform( (StateInt *) this->block );
      }
      else {
        this->transform( (StateInt *) input );
      }
      input = &input[ BLKSZ ];
      inputLen -= BLKSZ;
    }

    if ( inputLen > 0 )
      ::memcpy( &this->block[ i ], input, inputLen );
  }

  virtual void final( void ) {
    static const unsigned int PADLIMIT = BLKSZ - (BLKSZ / 8 ); /* 56, 112 */
    unsigned int i;
    ullong       n;

    i = (unsigned int) ( this->count & ( ( BLKSZ << 3 ) - 1 ) ) >> 3;

    this->block[ i++ ] = (byte) 0x80U;

    if ( i >= PADLIMIT ) {
      ::memset( &this->block[ i ], 0, BLKSZ - i );
      /* need to do two blocks to finish up */
      this->transform( (StateInt *) this->block );
      ::memset( this->block, 0, PADLIMIT );
    }
    else {
      ::memset( &this->block[ i ], 0, PADLIMIT - i );
    }

    n = this->count;
    if ( outputIsLittle ) {
      for ( i = PADLIMIT; i < BLKSZ; i++ ) {
        this->block[ i ] = (byte) n;
        n >>= 8;
      }
    }
    else {
      for ( i = BLKSZ; i > PADLIMIT; ) {
        this->block[ --i ] = (byte) n;
        n >>= 8;
      }
    }
    this->transform( (StateInt *) this->block );

    if ( ! ( Aligned::isLittleEndian ^ ! outputIsLittle ) ) {
      for ( i = 0; i < STATESZ; i++ )
        Aligned::swap( this->state[ i ] );
    }
  }
  virtual unsigned int digestSize( void ) { /* in bytes */
    return NBITS / 8;
  }
  virtual unsigned int blockSize( void ) { /* in bytes */
    return BLKSZ;
  }
  virtual void digest( void *d ) {
    ::memcpy( d, this->state, sizeof( this->state ) );
  }
  virtual bool selftest( void ) = 0;

  virtual HashContext *dup( void ) = 0;

  virtual HashContext *dup2( void *p,  unsigned int &sz ) = 0;
};


/* Copyright (C) 1990, RSA Data Security, Inc. All rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD4 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 *
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD4 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

struct MD4Ctx : public RSA_Hash<128, 512/8, true, unsigned int> {
  static const unsigned int I0 = 0x67452301U; /* Initial values for MD buffer */
  static const unsigned int I1 = 0xefcdab89U;
  static const unsigned int I2 = 0x98badcfeU;
  static const unsigned int I3 = 0x10325476U;

  /* Compile-time declarations of MD4 ``magic constants'' */
  static const unsigned int C2 = 013240474631; /* round 2 constant = sqrt(2) */
  static const unsigned int C3 = 015666365641; /* round 3 constant = sqrt(3) */

  static const unsigned int fs1 =  3;            /* round 1 shift amounts */
  static const unsigned int fs2 =  7;
  static const unsigned int fs3 = 11;
  static const unsigned int fs4 = 19;
  static const unsigned int gs1 =  3;            /* round 2 shift amounts */
  static const unsigned int gs2 =  5;
  static const unsigned int gs3 =  9;
  static const unsigned int gs4 = 13;
  static const unsigned int hs1 =  3;            /* round 3 shift amounts */
  static const unsigned int hs2 =  9;
  static const unsigned int hs3 = 11;
  static const unsigned int hs4 = 15;

  static inline unsigned int f( unsigned int X,  unsigned int Y,
                                unsigned int Z ) {
    return ((X&Y) | ((~X)&Z));
  }
  static inline unsigned int g( unsigned int X,  unsigned int Y,
                                unsigned int Z ) {
    return ((X&Y) | (X&Z) | (Y&Z));
  }
  static inline unsigned int h( unsigned int X,  unsigned int Y,
                                unsigned int Z ) {
    return (X^Y^Z);
  }
  static inline unsigned int rot( unsigned int X, unsigned int S ) {
    return ((X<<S) | (X>>(32-S)));
  }
  static inline void ff( unsigned int &A,  unsigned int B,
                         unsigned int C,  unsigned int D,
                         unsigned int X,  unsigned int s ) {
    A = rot((A + f(B,C,D) + X),s);
  }
  static inline void gg( unsigned int &A,  unsigned int B,
                         unsigned int C,  unsigned int D,
                         unsigned int X,  unsigned int s ) {
    A = rot((A + g(B,C,D) + X + C2),s);
  }
  static inline void hh( unsigned int &A,  unsigned int B,
                         unsigned int C,  unsigned int D,
                         unsigned int X,  unsigned int s ) {
    A = rot((A + h(B,C,D) + X + C3),s);
  }

  virtual void init( void ) {
    this->state[ 0 ] = I0;  
    this->state[ 1 ] = I1;  
    this->state[ 2 ] = I2;  
    this->state[ 3 ] = I3; 
    this->count = 0;
  }

  virtual void transform( unsigned int *x ) {
    unsigned int A, B, C, D;

    if ( ! Aligned::isLittleEndian ) {
      if ( x != (unsigned int *) this->block ) {
        ::memcpy( this->block, x, 64 );
        x = (unsigned int *) this->block;
      }
      for ( A = 0; A < 64 / 4; A++ )
        Aligned::swap( x[ A ] );
    }
    A = this->state[ 0 ];
    B = this->state[ 1 ];
    C = this->state[ 2 ];
    D = this->state[ 3 ];
    /* Round 1 */
    ff( A, B, C, D, x[ 0 ], fs1 ); ff( D, A, B, C, x[ 1 ], fs2 ); 
    ff( C, D, A, B, x[ 2 ], fs3 ); ff( B, C, D, A, x[ 3 ], fs4 ); 
    ff( A, B, C, D, x[ 4 ], fs1 ); ff( D, A, B, C, x[ 5 ], fs2 ); 
    ff( C, D, A, B, x[ 6 ], fs3 ); ff( B, C, D, A, x[ 7 ], fs4 ); 
    ff( A, B, C, D, x[ 8 ], fs1 ); ff( D, A, B, C, x[ 9 ], fs2 ); 
    ff( C, D, A, B, x[ 10 ], fs3 ); ff( B, C, D, A, x[ 11 ], fs4 ); 
    ff( A, B, C, D, x[ 12 ], fs1 ); ff( D, A, B, C, x[ 13 ], fs2 ); 
    ff( C, D, A, B, x[ 14 ], fs3 ); ff( B, C, D, A, x[ 15 ], fs4 ); 
    /* Round 2 */
    gg( A, B, C, D, x[ 0 ], gs1 ); gg( D, A, B, C, x[ 4 ], gs2 ); 
    gg( C, D, A, B, x[ 8 ], gs3 ); gg( B, C, D, A, x[ 12 ], gs4 ); 
    gg( A, B, C, D, x[ 1 ], gs1 ); gg( D, A, B, C, x[ 5 ], gs2 ); 
    gg( C, D, A, B, x[ 9 ], gs3 ); gg( B, C, D, A, x[ 13 ], gs4 ); 
    gg( A, B, C, D, x[ 2 ], gs1 ); gg( D, A, B, C, x[ 6 ], gs2 ); 
    gg( C, D, A, B, x[ 10 ], gs3 ); gg( B, C, D, A, x[ 14 ], gs4 ); 
    gg( A, B, C, D, x[ 3 ], gs1 ); gg( D, A, B, C, x[ 7 ], gs2 ); 
    gg( C, D, A, B, x[ 11 ], gs3 ); gg( B, C, D, A, x[ 15 ], gs4 );  
    /* Round 3 */
    hh( A, B, C, D, x[ 0 ], hs1 ); hh( D, A, B, C, x[ 8 ], hs2 ); 
    hh( C, D, A, B, x[ 4 ], hs3 ); hh( B, C, D, A, x[ 12 ], hs4 ); 
    hh( A, B, C, D, x[ 2 ], hs1 ); hh( D, A, B, C, x[ 10 ], hs2 ); 
    hh( C, D, A, B, x[ 6 ], hs3 ); hh( B, C, D, A, x[ 14 ], hs4 ); 
    hh( A, B, C, D, x[ 1 ], hs1 ); hh( D, A, B, C, x[ 9 ], hs2 ); 
    hh( C, D, A, B, x[ 5 ], hs3 ); hh( B, C, D, A, x[ 13 ], hs4 ); 
    hh( A, B, C, D, x[ 3 ], hs1 ); hh( D, A, B, C, x[ 11 ], hs2 ); 
    hh( C, D, A, B, x[ 7 ], hs3 ); hh( B, C, D, A, x[ 15 ], hs4 );

    this->state[ 0 ] += A; 
    this->state[ 1 ] += B;
    this->state[ 2 ] += C;
    this->state[ 3 ] += D; 
  }

  virtual bool selftest( void ) {
    static const byte mt[] = {
      0x31, 0xd6, 0xcf, 0xe0, 0xd1, 0x6a, 0xe9, 0x31,
      0xb7, 0x3c, 0x59, 0xd7, 0xe0, 0xc0, 0x89, 0xc0 };
    static const byte a[] = {
      0xbd, 0xe5, 0x2c, 0xb3, 0x1d, 0xe3, 0x3e, 0x46,
      0x24, 0x5e, 0x05, 0xfb, 0xdb, 0xd6, 0xfb, 0x24 };
    static const byte eighty[] = {
      0xe3, 0x3b, 0x4d, 0xdc, 0x9c, 0x38, 0xf2, 0x19,
      0x9c, 0x3e, 0x7b, 0x16, 0x4f, 0xcc, 0x05, 0x36 };
    this->init();
    this->update( (const byte *) "", 0 );
    this->final();
    if ( ::memcmp( this->state, mt, 128 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "a", 1 );
    this->final();
    if ( ::memcmp( this->state, a, 128 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "12345678901234567890123456789012345"
                                 "67890123456789012345678901234567890"
                                 "1234567890", 80 );
    this->final();
    if ( ::memcmp( this->state, eighty, 128 / 8 ) != 0 )
      return false;
    return true;
  }

  void * operator new( size_t sz, void *ptr ) { return ptr; }
  SYS_OPS( MD4Ctx );
  MD4Ctx() {}
  virtual ~MD4Ctx() {}
  virtual HashContext *dup( void ) {
    return NEW MD4Ctx();
  }
  virtual HashContext *dup2( void *p,  unsigned int &sz ) {
    if ( sz >= sizeof( MD4Ctx ) ) {
      sz = sizeof( MD4Ctx );
      return new ( p ) MD4Ctx();
    }
    sz = sizeof( MD4Ctx );
    return NULL;
  }
};


void
Hash128::md4( const byte *buf,  unsigned int bufLen,  void *digest )
{
  MD4Ctx ctx;

  ctx.init();
  ctx.update( buf, bufLen );
  ctx.final();

  ::memcpy( digest, ctx.state, sizeof( ctx.state ) );
}


/* originally:
 * MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
 */

/* Copyright (C) 1991, RSA Data Security, Inc. All rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.  
                                                                    
   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.  
                                                                    
   These notices must be retained in any copies of any part of this
   documentation and/or software.  
 */
struct MD5Ctx : public RSA_Hash<128, 512/8, true, unsigned int> {
  /* Constants for MD5Transform routine.
   */
  static const unsigned int S11 = 7;
  static const unsigned int S12 = 12;
  static const unsigned int S13 = 17;
  static const unsigned int S14 = 22;
  static const unsigned int S21 = 5;
  static const unsigned int S22 = 9;
  static const unsigned int S23 = 14;
  static const unsigned int S24 = 20;
  static const unsigned int S31 = 4;
  static const unsigned int S32 = 11;
  static const unsigned int S33 = 16;
  static const unsigned int S34 = 23;
  static const unsigned int S41 = 6;
  static const unsigned int S42 = 10;
  static const unsigned int S43 = 15;
  static const unsigned int S44 = 21;

  /* F, G, H and I are basic MD5 functions.
   */
  static inline unsigned int F( unsigned int X,  unsigned int Y,
                                unsigned int Z ) {
    return ((X&Y) | ((~X)&Z));
  }
  static inline unsigned int G( unsigned int X,  unsigned int Y,
                                unsigned int Z ) {
    return ((X&Z) | (Y&(~Z)));
  }
  static inline unsigned int H( unsigned int X,  unsigned int Y,
                                unsigned int Z ) {
    return (X^Y^Z);
  }
  static inline unsigned int I( unsigned int X,  unsigned int Y,
                                unsigned int Z ) {
    return (Y^(X|(~Z)));
  }
  /* ROTATE_LEFT rotates x left n bits.
   */
  static inline unsigned int ROTATE_LEFT( unsigned int X, unsigned int S ) {
    return ((X<<S) | (X>>(32-S)));
  }

  /* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
     Rotation is separate from addition to prevent recomputation.
   */
  static inline void FF( unsigned int &a, unsigned int b, unsigned int c,
                         unsigned int d, unsigned int x, unsigned int s,
                         unsigned int ac ) {
    (a) += F ((b), (c), (d)) + (x) + (ac);
    (a) = ROTATE_LEFT ((a), (s));
    (a) += (b);
  }
  static inline void GG( unsigned int &a, unsigned int b, unsigned int c,
                         unsigned int d, unsigned int x, unsigned int s,
                         unsigned int ac ) {
    (a) += G ((b), (c), (d)) + (x) + (ac);
    (a) = ROTATE_LEFT ((a), (s));
    (a) += (b);
  }
  static inline void HH( unsigned int &a, unsigned int b, unsigned int c,
                         unsigned int d, unsigned int x, unsigned int s,
                         unsigned int ac ) {
    (a) += H ((b), (c), (d)) + (x) + (ac);
    (a) = ROTATE_LEFT ((a), (s));
    (a) += (b);
  }
  static inline void II( unsigned int &a, unsigned int b, unsigned int c,
                         unsigned int d, unsigned int x, unsigned int s,
                         unsigned int ac ) {
    (a) += I ((b), (c), (d)) + (x) + (ac);
    (a) = ROTATE_LEFT ((a), (s));
    (a) += (b);
  }

  /* MD5 initialization. Begins an MD5 operation, writing a new context.
   */
  virtual void init( void ) {
    this->state[ 0 ] = 0x67452301U;
    this->state[ 1 ] = 0xefcdab89U;
    this->state[ 2 ] = 0x98badcfeU;
    this->state[ 3 ] = 0x10325476U;
    this->count = 0;
  }

  /* MD5 basic transformation. Transforms state based on block.
   */
  virtual void transform( unsigned int *x ) {
    unsigned int a, b, c, d;
    
    if ( ! Aligned::isLittleEndian ) {
      if ( x != (unsigned int *) this->block ) {
        ::memcpy( this->block, x, 64 );
        x = (unsigned int *) this->block;
      }
      for ( a = 0; a < 64 / 4; a++ )
        Aligned::swap( x[ a ] );
    }
    a = this->state[ 0 ];
    b = this->state[ 1 ];
    c = this->state[ 2 ];
    d = this->state[ 3 ];
    /* Round 1 */
    FF ( a, b, c, d, x[ 0], S11, 0xd76aa478U ); /* 1 */
    FF ( d, a, b, c, x[ 1], S12, 0xe8c7b756U ); /* 2 */
    FF ( c, d, a, b, x[ 2], S13, 0x242070dbU ); /* 3 */
    FF ( b, c, d, a, x[ 3], S14, 0xc1bdceeeU ); /* 4 */
    FF ( a, b, c, d, x[ 4], S11, 0xf57c0fafU ); /* 5 */
    FF ( d, a, b, c, x[ 5], S12, 0x4787c62aU ); /* 6 */
    FF ( c, d, a, b, x[ 6], S13, 0xa8304613U ); /* 7 */
    FF ( b, c, d, a, x[ 7], S14, 0xfd469501U ); /* 8 */
    FF ( a, b, c, d, x[ 8], S11, 0x698098d8U ); /* 9 */
    FF ( d, a, b, c, x[ 9], S12, 0x8b44f7afU ); /* 10 */
    FF ( c, d, a, b, x[10], S13, 0xffff5bb1U ); /* 11 */
    FF ( b, c, d, a, x[11], S14, 0x895cd7beU ); /* 12 */
    FF ( a, b, c, d, x[12], S11, 0x6b901122U ); /* 13 */
    FF ( d, a, b, c, x[13], S12, 0xfd987193U ); /* 14 */
    FF ( c, d, a, b, x[14], S13, 0xa679438eU ); /* 15 */
    FF ( b, c, d, a, x[15], S14, 0x49b40821U ); /* 16 */
    /* Round 2 */
    GG ( a, b, c, d, x[ 1], S21, 0xf61e2562U ); /* 17 */
    GG ( d, a, b, c, x[ 6], S22, 0xc040b340U ); /* 18 */
    GG ( c, d, a, b, x[11], S23, 0x265e5a51U ); /* 19 */
    GG ( b, c, d, a, x[ 0], S24, 0xe9b6c7aaU ); /* 20 */
    GG ( a, b, c, d, x[ 5], S21, 0xd62f105dU ); /* 21 */
    GG ( d, a, b, c, x[10], S22,  0x2441453U ); /* 22 */
    GG ( c, d, a, b, x[15], S23, 0xd8a1e681U ); /* 23 */
    GG ( b, c, d, a, x[ 4], S24, 0xe7d3fbc8U ); /* 24 */
    GG ( a, b, c, d, x[ 9], S21, 0x21e1cde6U ); /* 25 */
    GG ( d, a, b, c, x[14], S22, 0xc33707d6U ); /* 26 */
    GG ( c, d, a, b, x[ 3], S23, 0xf4d50d87U ); /* 27 */
    GG ( b, c, d, a, x[ 8], S24, 0x455a14edU ); /* 28 */
    GG ( a, b, c, d, x[13], S21, 0xa9e3e905U ); /* 29 */
    GG ( d, a, b, c, x[ 2], S22, 0xfcefa3f8U ); /* 30 */
    GG ( c, d, a, b, x[ 7], S23, 0x676f02d9U ); /* 31 */
    GG ( b, c, d, a, x[12], S24, 0x8d2a4c8aU ); /* 32 */
    /* Round 3 */
    HH ( a, b, c, d, x[ 5], S31, 0xfffa3942U ); /* 33 */
    HH ( d, a, b, c, x[ 8], S32, 0x8771f681U ); /* 34 */
    HH ( c, d, a, b, x[11], S33, 0x6d9d6122U ); /* 35 */
    HH ( b, c, d, a, x[14], S34, 0xfde5380cU ); /* 36 */
    HH ( a, b, c, d, x[ 1], S31, 0xa4beea44U ); /* 37 */
    HH ( d, a, b, c, x[ 4], S32, 0x4bdecfa9U ); /* 38 */
    HH ( c, d, a, b, x[ 7], S33, 0xf6bb4b60U ); /* 39 */
    HH ( b, c, d, a, x[10], S34, 0xbebfbc70U ); /* 40 */
    HH ( a, b, c, d, x[13], S31, 0x289b7ec6U ); /* 41 */
    HH ( d, a, b, c, x[ 0], S32, 0xeaa127faU ); /* 42 */
    HH ( c, d, a, b, x[ 3], S33, 0xd4ef3085U ); /* 43 */
    HH ( b, c, d, a, x[ 6], S34,  0x4881d05U ); /* 44 */
    HH ( a, b, c, d, x[ 9], S31, 0xd9d4d039U ); /* 45 */
    HH ( d, a, b, c, x[12], S32, 0xe6db99e5U ); /* 46 */
    HH ( c, d, a, b, x[15], S33, 0x1fa27cf8U ); /* 47 */
    HH ( b, c, d, a, x[ 2], S34, 0xc4ac5665U ); /* 48 */
    /* Round 4 */
    II ( a, b, c, d, x[ 0], S41, 0xf4292244U ); /* 49 */
    II ( d, a, b, c, x[ 7], S42, 0x432aff97U ); /* 50 */
    II ( c, d, a, b, x[14], S43, 0xab9423a7U ); /* 51 */
    II ( b, c, d, a, x[ 5], S44, 0xfc93a039U ); /* 52 */
    II ( a, b, c, d, x[12], S41, 0x655b59c3U ); /* 53 */
    II ( d, a, b, c, x[ 3], S42, 0x8f0ccc92U ); /* 54 */
    II ( c, d, a, b, x[10], S43, 0xffeff47dU ); /* 55 */
    II ( b, c, d, a, x[ 1], S44, 0x85845dd1U ); /* 56 */
    II ( a, b, c, d, x[ 8], S41, 0x6fa87e4fU ); /* 57 */
    II ( d, a, b, c, x[15], S42, 0xfe2ce6e0U ); /* 58 */
    II ( c, d, a, b, x[ 6], S43, 0xa3014314U ); /* 59 */
    II ( b, c, d, a, x[13], S44, 0x4e0811a1U ); /* 60 */
    II ( a, b, c, d, x[ 4], S41, 0xf7537e82U ); /* 61 */
    II ( d, a, b, c, x[11], S42, 0xbd3af235U ); /* 62 */
    II ( c, d, a, b, x[ 2], S43, 0x2ad7d2bbU ); /* 63 */
    II ( b, c, d, a, x[ 9], S44, 0xeb86d391U ); /* 64 */

    this->state[ 0 ] += a;
    this->state[ 1 ] += b;
    this->state[ 2 ] += c;
    this->state[ 3 ] += d;
  }

  virtual bool selftest( void ) {
    static const byte mt[] = {
      0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
      0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e };
    static const byte a[] = {
      0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
      0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61 };
    static const byte eighty[] = {
      0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55,
      0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a };
    this->init();
    this->update( (const byte *) "", 0 );
    this->final();
    if ( ::memcmp( this->state, mt, 128 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "a", 1 );
    this->final();
    if ( ::memcmp( this->state, a, 128 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "12345678901234567890123456789012345"
                                 "67890123456789012345678901234567890"
                                 "1234567890", 80 );
    this->final();
    if ( ::memcmp( this->state, eighty, 128 / 8 ) != 0 )
      return false;
    return true;
  }

  void * operator new( size_t sz, void *ptr ) { return ptr; }
  SYS_OPS( MD5Ctx );
  MD5Ctx() {}
  virtual ~MD5Ctx() {}
  virtual HashContext *dup( void ) {
    return NEW MD5Ctx();
  }
  virtual HashContext *dup2( void *p, unsigned int &sz ) {
    if ( sz >= sizeof( MD5Ctx ) ) {
      sz = sizeof( MD5Ctx );
      return new ( p ) MD5Ctx();
    }
    sz = sizeof( MD5Ctx );
    return NULL;
  }
};


void
Hash128::md5( const byte *buf,  unsigned int bufLen,  void *digest )
{
  MD5Ctx ctx;

  ctx.init();
  ctx.update( buf, bufLen );
  ctx.final();

  ::memcpy( digest, ctx.state, sizeof( ctx.state ) );
}


struct SHA1Ctx : public RSA_Hash<160, 512/8, false, unsigned int> {
  static inline unsigned int ROL( unsigned int v,  unsigned int s ) {
    return (v << s) | (v >> (32 - s));
  }
  static inline unsigned int BLK( unsigned int *block,  unsigned int i ) {
    return block[i&15] = ROL(block[(i+13)&15] ^ block[(i+8)&15] ^
                             block[(i+2)&15] ^ block[i&15], 1 );
  }
  static inline void R0( unsigned int v, unsigned int &w, unsigned int x,
                         unsigned int y, unsigned int &z, unsigned int b ) {
    z += ((w&(x^y))^y) + b + 0x5A827999U + ROL(v,5);
    w  = ROL(w,30);
  }
  static inline void R1( unsigned int v, unsigned int &w, unsigned int x,
                         unsigned int y, unsigned int &z, unsigned int b ) {
    z += ((w&(x^y))^y) + b + 0x5A827999U + ROL(v,5);
    w  = ROL(w,30);
  }
  static inline void R2( unsigned int v, unsigned int &w, unsigned int x,
                         unsigned int y, unsigned int &z, unsigned int b ) {
    z += (w^x^y) + b + 0x6ED9EBA1U + ROL(v,5);
    w  = ROL(w,30);
  }
  static inline void R3( unsigned int v, unsigned int &w, unsigned int x,
                         unsigned int y, unsigned int &z, unsigned int b ) {
    z += (((w|x)&y)|(w&x)) + b + 0x8F1BBCDCU + ROL(v,5);
    w  = ROL(w,30);
  }
  static inline void R4( unsigned int v, unsigned int &w, unsigned int x,
                         unsigned int y, unsigned int &z, unsigned int b ) {
    z += (w^x^y) + b + 0xCA62C1D6U + ROL(v,5);
    w  = ROL(w,30);
  }

  virtual void init( void ) {
    this->state[ 0 ] = 0x67452301U;
    this->state[ 1 ] = 0xEFCDAB89U;
    this->state[ 2 ] = 0x98BADCFEU;
    this->state[ 3 ] = 0x10325476U;
    this->state[ 4 ] = 0xC3D2E1F0U;
    this->count = 0;
  }

  virtual void transform( unsigned int *x ) {
    unsigned int a, b, c, d, e;

    if ( x != (unsigned int *) this->block ) {
      ::memcpy( this->block, x, 64 );
      x = (unsigned int *) this->block;
    }
    if ( Aligned::isLittleEndian ) {
      for ( a = 0; a < 64 / 4; a++ )
        Aligned::swap( x[ a ] );
    }
    a = this->state[ 0 ];
    b = this->state[ 1 ]; 
    c = this->state[ 2 ];
    d = this->state[ 3 ];
    e = this->state[ 4 ];

    R0( a,b,c,d,e, x[ 0] ); R0( e,a,b,c,d, x[ 1] );
    R0( d,e,a,b,c, x[ 2] ); R0( c,d,e,a,b, x[ 3] );
    R0( b,c,d,e,a, x[ 4] ); R0( a,b,c,d,e, x[ 5] );
    R0( e,a,b,c,d, x[ 6] ); R0( d,e,a,b,c, x[ 7] );
    R0( c,d,e,a,b, x[ 8] ); R0( b,c,d,e,a, x[ 9] );
    R0( a,b,c,d,e, x[10] ); R0( e,a,b,c,d, x[11] );
    R0( d,e,a,b,c, x[12] ); R0( c,d,e,a,b, x[13] );
    R0( b,c,d,e,a, x[14] ); R0( a,b,c,d,e, x[15] );

    R1( e,a,b,c,d, BLK( x, 0 ) ); R1( d,e,a,b,c, BLK( x, 1 ) );
    R1( c,d,e,a,b, BLK( x, 2 ) ); R1( b,c,d,e,a, BLK( x, 3 ) );

    R2( a,b,c,d,e, BLK( x, 4 ) ); R2( e,a,b,c,d, BLK( x, 5 ) );
    R2( d,e,a,b,c, BLK( x, 6 ) ); R2( c,d,e,a,b, BLK( x, 7 ) );
    R2( b,c,d,e,a, BLK( x, 8 ) ); R2( a,b,c,d,e, BLK( x, 9 ) );
    R2( e,a,b,c,d, BLK( x,10 ) ); R2( d,e,a,b,c, BLK( x,11 ) );
    R2( c,d,e,a,b, BLK( x,12 ) ); R2( b,c,d,e,a, BLK( x,13 ) );
    R2( a,b,c,d,e, BLK( x,14 ) ); R2( e,a,b,c,d, BLK( x,15 ) );
    R2( d,e,a,b,c, BLK( x, 0 ) ); R2( c,d,e,a,b, BLK( x, 1 ) );
    R2( b,c,d,e,a, BLK( x, 2 ) ); R2( a,b,c,d,e, BLK( x, 3 ) );
    R2( e,a,b,c,d, BLK( x, 4 ) ); R2( d,e,a,b,c, BLK( x, 5 ) );
    R2( c,d,e,a,b, BLK( x, 6 ) ); R2( b,c,d,e,a, BLK( x, 7 ) );

    R3( a,b,c,d,e, BLK( x, 8 ) ); R3( e,a,b,c,d, BLK( x, 9 ) );
    R3( d,e,a,b,c, BLK( x,10 ) ); R3( c,d,e,a,b, BLK( x,11 ) );
    R3( b,c,d,e,a, BLK( x,12 ) ); R3( a,b,c,d,e, BLK( x,13 ) );
    R3( e,a,b,c,d, BLK( x,14 ) ); R3( d,e,a,b,c, BLK( x,15 ) );
    R3( c,d,e,a,b, BLK( x, 0 ) ); R3( b,c,d,e,a, BLK( x, 1 ) );
    R3( a,b,c,d,e, BLK( x, 2 ) ); R3( e,a,b,c,d, BLK( x, 3 ) );
    R3( d,e,a,b,c, BLK( x, 4 ) ); R3( c,d,e,a,b, BLK( x, 5 ) );
    R3( b,c,d,e,a, BLK( x, 6 ) ); R3( a,b,c,d,e, BLK( x, 7 ) );
    R3( e,a,b,c,d, BLK( x, 8 ) ); R3( d,e,a,b,c, BLK( x, 9 ) );
    R3( c,d,e,a,b, BLK( x,10 ) ); R3( b,c,d,e,a, BLK( x,11 ) );

    R4( a,b,c,d,e, BLK( x,12 ) ); R4( e,a,b,c,d, BLK( x,13 ) );
    R4( d,e,a,b,c, BLK( x,14 ) ); R4( c,d,e,a,b, BLK( x,15 ) );
    R4( b,c,d,e,a, BLK( x, 0 ) ); R4( a,b,c,d,e, BLK( x, 1 ) );
    R4( e,a,b,c,d, BLK( x, 2 ) ); R4( d,e,a,b,c, BLK( x, 3 ) );
    R4( c,d,e,a,b, BLK( x, 4 ) ); R4( b,c,d,e,a, BLK( x, 5 ) );
    R4( a,b,c,d,e, BLK( x, 6 ) ); R4( e,a,b,c,d, BLK( x, 7 ) );
    R4( d,e,a,b,c, BLK( x, 8 ) ); R4( c,d,e,a,b, BLK( x, 9 ) );
    R4( b,c,d,e,a, BLK( x,10 ) ); R4( a,b,c,d,e, BLK( x,11 ) );
    R4( e,a,b,c,d, BLK( x,12 ) ); R4( d,e,a,b,c, BLK( x,13 ) );
    R4( c,d,e,a,b, BLK( x,14 ) ); R4( b,c,d,e,a, BLK( x,15 ) );

    this->state[ 0 ] += a;
    this->state[ 1 ] += b; 
    this->state[ 2 ] += c;
    this->state[ 3 ] += d;
    this->state[ 4 ] += e;
  }

  virtual bool selftest( void ) {
    static const byte abc[] = {
      0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e, 0x25, 0x71,
      0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d };
    static const byte abcdbc[] = {
      0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2, 0x6e, 0xba, 0xae, 0x4a, 0xa1,
      0xf9, 0x51, 0x29, 0xe5, 0xe5, 0x46, 0x70, 0xf1 };
    this->init();
    this->update( (const byte *) "abc", 3 );
    this->final();
    if ( ::memcmp( this->state, abc, 160 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "abcdbcdecdefdefgefghfghighi"
                                 "jhijkijkljklmklmnlmnomnopnopq", 56 );
    this->final();
    if ( ::memcmp( this->state, abcdbc, 160 / 8 ) != 0 )
      return false;
    return true;
  }

  void * operator new( size_t sz, void *ptr ) { return ptr; }
  SYS_OPS( SHA1Ctx );
  SHA1Ctx() {}
  virtual ~SHA1Ctx() {}
  virtual HashContext *dup( void ) {
    return NEW SHA1Ctx();
  }
  virtual HashContext *dup2( void *p, unsigned int &sz ) {
    if ( sz >= sizeof( SHA1Ctx ) ) {
      sz = sizeof( SHA1Ctx );
      return new ( p ) SHA1Ctx();
    }
    sz = sizeof( SHA1Ctx );
    return NULL;
  }
};


void
Hash160::sha1( const byte *buf,  unsigned int bufLen,  void *digest )
{
  SHA1Ctx ctx;

  ctx.init();
  ctx.update( buf, bufLen );
  ctx.final();

  ::memcpy( digest, ctx.state, sizeof( ctx.state ) );
}


/********************************************************************\
 *
 *      Copyright (c) Katholieke Universiteit Leuven
 *      1996, All Rights Reserved
 *
 *  Conditions for use of the RIPEMD-160 Software
 &
 *  The RIPEMD-160 software is freely available for use under the terms and
 *  conditions described hereunder, which shall be deemed to be accepted by
 *  any user of the software and applicable on any use of the software:
 * 
 *  1. K.U.Leuven Department of Electrical Engineering-ESAT/COSIC shall for
 *     all purposes be considered the owner of the RIPEMD-160 software and of
 *     all copyright, trade secret, patent or other intellectual property
 *     rights therein.
 *  2. The RIPEMD-160 software is provided on an "as is" basis without
 *     warranty of any sort, express or implied. K.U.Leuven makes no
 *     representation that the use of the software will not infringe any
 *     patent or proprietary right of third parties. User will indemnify
 *     K.U.Leuven and hold K.U.Leuven harmless from any claims or liabilities
 *     which may arise as a result of its use of the software. In no
 *     circumstances K.U.Leuven R&D will be held liable for any deficiency,
 *     fault or other mishappening with regard to the use or performance of
 *     the software.
 *  3. User agrees to give due credit to K.U.Leuven in scientific publications 
 *     or communications in relation with the use of the RIPEMD-160 software 
 *     as follows: RIPEMD-160 software written by Antoon Bosselaers, 
 *     available at http://www.esat.kuleuven.ac.be/~cosicart/ps/AB-9601/.
 *
\********************************************************************/

struct RIPEMD128Ctx : public RSA_Hash<128, 512/8, true, unsigned int> {
  /* ROL(x, n) cyclically rotates x over n bits to the left */
  static inline unsigned int ROL( unsigned int x,  unsigned int n ) {
    return (((x) << (n)) | ((x) >> (32-(n))));
  }

  /* the five basic functions F(), G() and H() */
  static inline unsigned int F( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return ((x) ^ (y) ^ (z));
  }
  static inline unsigned int G( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return (((x) & (y)) | (~(x) & (z)));
  }
  static inline unsigned int H( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return (((x) | ~(y)) ^ (z));
  }
  static inline unsigned int I( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return (((x) & (z)) | ((y) & ~(z)));
  }
  static inline unsigned int J( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return ((x) ^ ((y) | ~(z)));
  }
  
  /* the ten basic operations FF() through III() */
  static inline void FF( unsigned int &a,  unsigned int b,  unsigned int c,
                         unsigned int d,  unsigned int x,  unsigned int s ) {
    a += F((b), (c), (d)) + (x);
    a  = ROL((a), (s));
  }
  static inline void GG( unsigned int &a,  unsigned int b,  unsigned int c,
                         unsigned int d,  unsigned int x,  unsigned int s ) {
    a += G((b), (c), (d)) + (x) + 0x5a827999U;
    a  = ROL((a), (s));
  }
  static inline void HH( unsigned int &a,  unsigned int b,  unsigned int c,
                         unsigned int d,  unsigned int x,  unsigned int s ) {
    a += H((b), (c), (d)) + (x) + 0x6ed9eba1U;
    a  = ROL((a), (s));
  }
  static inline void II( unsigned int &a,  unsigned int b,  unsigned int c,
                         unsigned int d,  unsigned int x,  unsigned int s ) {
    a += I((b), (c), (d)) + (x) + 0x8f1bbcdcU;
    a  = ROL((a), (s));
  }
  static inline void FFF( unsigned int &a,  unsigned int b,  unsigned int c,
                          unsigned int d,  unsigned int x,  unsigned int s ) {
    a += F((b), (c), (d)) + (x);
    a  = ROL((a), (s));
  }
  static inline void GGG( unsigned int &a,  unsigned int b,  unsigned int c,
                          unsigned int d,  unsigned int x,  unsigned int s ) {
    a += G((b), (c), (d)) + (x) + 0x6d703ef3U;
    a  = ROL((a), (s));
  }
  static inline void HHH( unsigned int &a,  unsigned int b,  unsigned int c,
                          unsigned int d,  unsigned int x,  unsigned int s ) {
    a += H((b), (c), (d)) + (x) + 0x5c4dd124U;
    a  = ROL((a), (s));
  }
  static inline void III( unsigned int &a,  unsigned int b,  unsigned int c,
                          unsigned int d,  unsigned int x,  unsigned int s ) {
    a += I((b), (c), (d)) + (x) + 0x50a28be6U;
    a  = ROL((a), (s));
  }

  virtual void init( void ) {
    this->state[ 0 ] = 0x67452301U;
    this->state[ 1 ] = 0xefcdab89U;
    this->state[ 2 ] = 0x98badcfeU;
    this->state[ 3 ] = 0x10325476U;
    this->count = 0;
  }

  virtual void transform( unsigned int *x ) {
   unsigned int aa,  bb,  cc,  dd,
                aaa, bbb, ccc, ddd;

   if ( ! Aligned::isLittleEndian ) {
      if ( x != (unsigned int *) this->block ) {
        ::memcpy( this->block, x, 64 );
        x = (unsigned int *) this->block;
      }
     for ( aa = 0; aa < 64 / 4; aa++ )
       Aligned::swap( x[ aa ] );
   }
   aa = aaa = this->state[ 0 ];
   bb = bbb = this->state[ 1 ];
   cc = ccc = this->state[ 2 ];
   dd = ddd = this->state[ 3 ];

   /* round 1 */
   FF(aa, bb, cc, dd, x[ 0], 11); FF(dd, aa, bb, cc, x[ 1], 14);
   FF(cc, dd, aa, bb, x[ 2], 15); FF(bb, cc, dd, aa, x[ 3], 12);
   FF(aa, bb, cc, dd, x[ 4],  5); FF(dd, aa, bb, cc, x[ 5],  8);
   FF(cc, dd, aa, bb, x[ 6],  7); FF(bb, cc, dd, aa, x[ 7],  9);
   FF(aa, bb, cc, dd, x[ 8], 11); FF(dd, aa, bb, cc, x[ 9], 13);
   FF(cc, dd, aa, bb, x[10], 14); FF(bb, cc, dd, aa, x[11], 15);
   FF(aa, bb, cc, dd, x[12],  6); FF(dd, aa, bb, cc, x[13],  7);
   FF(cc, dd, aa, bb, x[14],  9); FF(bb, cc, dd, aa, x[15],  8);
   /* round 2 */
   GG(aa, bb, cc, dd, x[ 7],  7); GG(dd, aa, bb, cc, x[ 4],  6);
   GG(cc, dd, aa, bb, x[13],  8); GG(bb, cc, dd, aa, x[ 1], 13);
   GG(aa, bb, cc, dd, x[10], 11); GG(dd, aa, bb, cc, x[ 6],  9);
   GG(cc, dd, aa, bb, x[15],  7); GG(bb, cc, dd, aa, x[ 3], 15);
   GG(aa, bb, cc, dd, x[12],  7); GG(dd, aa, bb, cc, x[ 0], 12);
   GG(cc, dd, aa, bb, x[ 9], 15); GG(bb, cc, dd, aa, x[ 5],  9);
   GG(aa, bb, cc, dd, x[ 2], 11); GG(dd, aa, bb, cc, x[14],  7);
   GG(cc, dd, aa, bb, x[11], 13); GG(bb, cc, dd, aa, x[ 8], 12);
   /* round 3 */
   HH(aa, bb, cc, dd, x[ 3], 11); HH(dd, aa, bb, cc, x[10], 13);
   HH(cc, dd, aa, bb, x[14],  6); HH(bb, cc, dd, aa, x[ 4],  7);
   HH(aa, bb, cc, dd, x[ 9], 14); HH(dd, aa, bb, cc, x[15],  9);
   HH(cc, dd, aa, bb, x[ 8], 13); HH(bb, cc, dd, aa, x[ 1], 15);
   HH(aa, bb, cc, dd, x[ 2], 14); HH(dd, aa, bb, cc, x[ 7],  8);
   HH(cc, dd, aa, bb, x[ 0], 13); HH(bb, cc, dd, aa, x[ 6],  6);
   HH(aa, bb, cc, dd, x[13],  5); HH(dd, aa, bb, cc, x[11], 12);
   HH(cc, dd, aa, bb, x[ 5],  7); HH(bb, cc, dd, aa, x[12],  5);
   /* round 4 */
   II(aa, bb, cc, dd, x[ 1], 11); II(dd, aa, bb, cc, x[ 9], 12);
   II(cc, dd, aa, bb, x[11], 14); II(bb, cc, dd, aa, x[10], 15);
   II(aa, bb, cc, dd, x[ 0], 14); II(dd, aa, bb, cc, x[ 8], 15);
   II(cc, dd, aa, bb, x[12],  9); II(bb, cc, dd, aa, x[ 4],  8);
   II(aa, bb, cc, dd, x[13],  9); II(dd, aa, bb, cc, x[ 3], 14);
   II(cc, dd, aa, bb, x[ 7],  5); II(bb, cc, dd, aa, x[15],  6);
   II(aa, bb, cc, dd, x[14],  8); II(dd, aa, bb, cc, x[ 5],  6);
   II(cc, dd, aa, bb, x[ 6],  5); II(bb, cc, dd, aa, x[ 2], 12);
   /* parallel round 1 */
   III(aaa, bbb, ccc, ddd, x[ 5],  8); III(ddd, aaa, bbb, ccc, x[14],  9);
   III(ccc, ddd, aaa, bbb, x[ 7],  9); III(bbb, ccc, ddd, aaa, x[ 0], 11);
   III(aaa, bbb, ccc, ddd, x[ 9], 13); III(ddd, aaa, bbb, ccc, x[ 2], 15);
   III(ccc, ddd, aaa, bbb, x[11], 15); III(bbb, ccc, ddd, aaa, x[ 4],  5);
   III(aaa, bbb, ccc, ddd, x[13],  7); III(ddd, aaa, bbb, ccc, x[ 6],  7);
   III(ccc, ddd, aaa, bbb, x[15],  8); III(bbb, ccc, ddd, aaa, x[ 8], 11);
   III(aaa, bbb, ccc, ddd, x[ 1], 14); III(ddd, aaa, bbb, ccc, x[10], 14);
   III(ccc, ddd, aaa, bbb, x[ 3], 12); III(bbb, ccc, ddd, aaa, x[12],  6);
   /* parallel round 2 */
   HHH(aaa, bbb, ccc, ddd, x[ 6],  9); HHH(ddd, aaa, bbb, ccc, x[11], 13);
   HHH(ccc, ddd, aaa, bbb, x[ 3], 15); HHH(bbb, ccc, ddd, aaa, x[ 7],  7);
   HHH(aaa, bbb, ccc, ddd, x[ 0], 12); HHH(ddd, aaa, bbb, ccc, x[13],  8);
   HHH(ccc, ddd, aaa, bbb, x[ 5],  9); HHH(bbb, ccc, ddd, aaa, x[10], 11);
   HHH(aaa, bbb, ccc, ddd, x[14],  7); HHH(ddd, aaa, bbb, ccc, x[15],  7);
   HHH(ccc, ddd, aaa, bbb, x[ 8], 12); HHH(bbb, ccc, ddd, aaa, x[12],  7);
   HHH(aaa, bbb, ccc, ddd, x[ 4],  6); HHH(ddd, aaa, bbb, ccc, x[ 9], 15);
   HHH(ccc, ddd, aaa, bbb, x[ 1], 13); HHH(bbb, ccc, ddd, aaa, x[ 2], 11);
   /* parallel round 3 */   
   GGG(aaa, bbb, ccc, ddd, x[15],  9); GGG(ddd, aaa, bbb, ccc, x[ 5],  7);
   GGG(ccc, ddd, aaa, bbb, x[ 1], 15); GGG(bbb, ccc, ddd, aaa, x[ 3], 11);
   GGG(aaa, bbb, ccc, ddd, x[ 7],  8); GGG(ddd, aaa, bbb, ccc, x[14],  6);
   GGG(ccc, ddd, aaa, bbb, x[ 6],  6); GGG(bbb, ccc, ddd, aaa, x[ 9], 14);
   GGG(aaa, bbb, ccc, ddd, x[11], 12); GGG(ddd, aaa, bbb, ccc, x[ 8], 13);
   GGG(ccc, ddd, aaa, bbb, x[12],  5); GGG(bbb, ccc, ddd, aaa, x[ 2], 14);
   GGG(aaa, bbb, ccc, ddd, x[10], 13); GGG(ddd, aaa, bbb, ccc, x[ 0], 13);
   GGG(ccc, ddd, aaa, bbb, x[ 4],  7); GGG(bbb, ccc, ddd, aaa, x[13],  5);
   /* parallel round 4 */
   FFF(aaa, bbb, ccc, ddd, x[ 8], 15); FFF(ddd, aaa, bbb, ccc, x[ 6],  5);
   FFF(ccc, ddd, aaa, bbb, x[ 4],  8); FFF(bbb, ccc, ddd, aaa, x[ 1], 11);
   FFF(aaa, bbb, ccc, ddd, x[ 3], 14); FFF(ddd, aaa, bbb, ccc, x[11], 14);
   FFF(ccc, ddd, aaa, bbb, x[15],  6); FFF(bbb, ccc, ddd, aaa, x[ 0], 14);
   FFF(aaa, bbb, ccc, ddd, x[ 5],  6); FFF(ddd, aaa, bbb, ccc, x[12],  9);
   FFF(ccc, ddd, aaa, bbb, x[ 2], 12); FFF(bbb, ccc, ddd, aaa, x[13],  9);
   FFF(aaa, bbb, ccc, ddd, x[ 9], 12); FFF(ddd, aaa, bbb, ccc, x[ 7],  5);
   FFF(ccc, ddd, aaa, bbb, x[10], 15); FFF(bbb, ccc, ddd, aaa, x[14],  8);
   /* combine results */
   ddd += cc + this->state[ 1 ];               /* final result for MDbuf[0] */
   this->state[ 1 ] = this->state[ 2 ] + dd + aaa;
   this->state[ 2 ] = this->state[ 3 ] + aa + bbb;
   this->state[ 3 ] = this->state[ 0 ] + bb + ccc;
   this->state[ 0 ] = ddd;
  }

  virtual bool selftest( void ) {
    static const byte mt[] = {
      0xcd, 0xf2, 0x62, 0x13, 0xa1, 0x50, 0xdc, 0x3e, 0xcb, 0x61, 0x0f, 0x18,
      0xf6, 0xb3, 0x8b, 0x46 };
    static const byte a[] = {
      0x86, 0xbe, 0x7a, 0xfa, 0x33, 0x9d, 0x0f, 0xc7, 0xcf, 0xc7, 0x85, 0xe7,
      0x2f, 0x57, 0x8d, 0x33 };
    static const byte eighty[] = {
      0x3f, 0x45, 0xef, 0x19, 0x47, 0x32, 0xc2, 0xdb, 0xb2, 0xc4, 0xa2, 0xc7,
      0x69, 0x79, 0x5f, 0xa3 };
    this->init();
    this->update( (const byte *) "", 0 );
    this->final();
    if ( ::memcmp( this->state, mt, 128 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "a", 1 );
    this->final();
    if ( ::memcmp( this->state, a, 128 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "12345678901234567890123456789012345"
                                 "67890123456789012345678901234567890"
                                 "1234567890", 80 );
    this->final();
    if ( ::memcmp( this->state, eighty, 128 / 8 ) != 0 )
      return false;
    return true;
  }

  void * operator new( size_t sz, void *ptr ) { return ptr; }
  SYS_OPS( RIPEMD128Ctx );
  RIPEMD128Ctx() {}
  virtual ~RIPEMD128Ctx() {}
  virtual HashContext *dup( void ) {
    return NEW RIPEMD128Ctx();
  }
  virtual HashContext *dup2( void *p, unsigned int &sz ) {
    if ( sz >= sizeof( RIPEMD128Ctx ) ) {
      sz = sizeof( RIPEMD128Ctx );
      return new ( p ) RIPEMD128Ctx();
    }
    sz = sizeof( RIPEMD128Ctx );
    return NULL;
  }
};


void
Hash128::ripemd( const byte *buf,  unsigned int bufLen,  void *digest )
{
  RIPEMD128Ctx ctx;

  ctx.init();
  ctx.update( buf, bufLen );
  ctx.final();

  ::memcpy( digest, ctx.state, sizeof( ctx.state ) );
}


/********************************************************************\
 *
 *      Copyright (c) Katholieke Universiteit Leuven
 *      1996, All Rights Reserved
 *
 *  Conditions for use of the RIPEMD-160 Software
 &
 *  The RIPEMD-160 software is freely available for use under the terms and
 *  conditions described hereunder, which shall be deemed to be accepted by
 *  any user of the software and applicable on any use of the software:
 * 
 *  1. K.U.Leuven Department of Electrical Engineering-ESAT/COSIC shall for
 *     all purposes be considered the owner of the RIPEMD-160 software and of
 *     all copyright, trade secret, patent or other intellectual property
 *     rights therein.
 *  2. The RIPEMD-160 software is provided on an "as is" basis without
 *     warranty of any sort, express or implied. K.U.Leuven makes no
 *     representation that the use of the software will not infringe any
 *     patent or proprietary right of third parties. User will indemnify
 *     K.U.Leuven and hold K.U.Leuven harmless from any claims or liabilities
 *     which may arise as a result of its use of the software. In no
 *     circumstances K.U.Leuven R&D will be held liable for any deficiency,
 *     fault or other mishappening with regard to the use or performance of
 *     the software.
 *  3. User agrees to give due credit to K.U.Leuven in scientific publications 
 *     or communications in relation with the use of the RIPEMD-160 software 
 *     as follows: RIPEMD-160 software written by Antoon Bosselaers, 
 *     available at http://www.esat.kuleuven.ac.be/~cosicart/ps/AB-9601/.
 *
\********************************************************************/

struct RIPEMD160Ctx : public RSA_Hash<160, 512/8, true, unsigned int> {
  /* ROL(x, n) cyclically rotates x over n bits to the left */
  static inline unsigned int ROL( unsigned int x,  unsigned int n ) {
    return (((x) << (n)) | ((x) >> (32-(n))));
  }

  /* the five basic functions F(), G() and H() */
  static inline unsigned int F( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return ((x) ^ (y) ^ (z));
  }
  static inline unsigned int G( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return (((x) & (y)) | (~(x) & (z)));
  }
  static inline unsigned int H( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return (((x) | ~(y)) ^ (z));
  }
  static inline unsigned int I( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return (((x) & (z)) | ((y) & ~(z)));
  }
  static inline unsigned int J( unsigned int x,  unsigned int y,
                                unsigned int z ) {
    return ((x) ^ ((y) | ~(z)));
  }
  
  /* the ten basic operations FF() through III() */
  static inline void FF( unsigned int &a,  unsigned int b,  unsigned int &c,
                         unsigned int d,  unsigned int e,  unsigned int x,
                         unsigned int s ) {
    a += F((b), (c), (d)) + (x);
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }
  static inline void GG( unsigned int &a,  unsigned int b,  unsigned int &c,
                         unsigned int d,  unsigned int e,  unsigned int x,
                         unsigned int s ) {
    a += G((b), (c), (d)) + (x) + 0x5a827999U;
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }
  static inline void HH( unsigned int &a,  unsigned int b,  unsigned int &c,
                         unsigned int d,  unsigned int e,  unsigned int x,
                         unsigned int s ) {
    a += H((b), (c), (d)) + (x) + 0x6ed9eba1U;
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }
  static inline void II( unsigned int &a,  unsigned int b,  unsigned int &c,
                         unsigned int d,  unsigned int e,  unsigned int x,
                         unsigned int s ) {
    a += I((b), (c), (d)) + (x) + 0x8f1bbcdcU;
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }
  static inline void JJ( unsigned int &a,  unsigned int b,  unsigned int &c,
                         unsigned int d,  unsigned int e,  unsigned int x,
                         unsigned int s ) {
    a += J((b), (c), (d)) + (x) + 0xa953fd4eU;
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }
  static inline void FFF( unsigned int &a,  unsigned int b,  unsigned int &c,
                          unsigned int d,  unsigned int e,  unsigned int x,
                          unsigned int s ) {
    a += F((b), (c), (d)) + (x);
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }
  static inline void GGG( unsigned int &a,  unsigned int b,  unsigned int &c,
                          unsigned int d,  unsigned int e,  unsigned int x,
                          unsigned int s  ) {
    a += G((b), (c), (d)) + (x) + 0x7a6d76e9U;
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }
  static inline void HHH( unsigned int &a,  unsigned int b,  unsigned int &c,
                          unsigned int d,  unsigned int e,  unsigned int x,
                          unsigned int s  ) {
    a += H((b), (c), (d)) + (x) + 0x6d703ef3U;
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }
  static inline void III( unsigned int &a,  unsigned int b,  unsigned int &c,
                          unsigned int d,  unsigned int e,  unsigned int x,
                          unsigned int s  ) {
    a += I((b), (c), (d)) + (x) + 0x5c4dd124U;
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }
  static inline void JJJ( unsigned int &a,  unsigned int b,  unsigned int &c,
                          unsigned int d,  unsigned int e,  unsigned int x,
                          unsigned int s  ) {
    a += J((b), (c), (d)) + (x) + 0x50a28be6U;
    a  = ROL((a), (s)) + (e);
    c  = ROL((c), 10);
  }

  virtual void init( void ) {
    this->state[ 0 ] = 0x67452301U;
    this->state[ 1 ] = 0xEFCDAB89U;
    this->state[ 2 ] = 0x98BADCFEU;
    this->state[ 3 ] = 0x10325476U;
    this->state[ 4 ] = 0xC3D2E1F0U;
    this->count = 0;
  }

  virtual void transform( unsigned int *x ) {
    unsigned int aa, bb, cc, dd, ee,
                 aaa, bbb, ccc, ddd, eee;
    
    if ( ! Aligned::isLittleEndian ) {
      if ( x != (unsigned int *) this->block ) {
        ::memcpy( this->block, x, 64 );
        x = (unsigned int *) this->block;
      }
      for ( aa = 0; aa < 64 / 4; aa++ )
        Aligned::swap( x[ aa ] );
    }
    aa = aaa = this->state[ 0 ];
    bb = bbb = this->state[ 1 ];
    cc = ccc = this->state[ 2 ];
    dd = ddd = this->state[ 3 ];
    ee = eee = this->state[ 4 ];

    /* round 1 */
    FF(aa, bb, cc, dd, ee, x[ 0], 11); FF(ee, aa, bb, cc, dd, x[ 1], 14);
    FF(dd, ee, aa, bb, cc, x[ 2], 15); FF(cc, dd, ee, aa, bb, x[ 3], 12);
    FF(bb, cc, dd, ee, aa, x[ 4],  5); FF(aa, bb, cc, dd, ee, x[ 5],  8);
    FF(ee, aa, bb, cc, dd, x[ 6],  7); FF(dd, ee, aa, bb, cc, x[ 7],  9);
    FF(cc, dd, ee, aa, bb, x[ 8], 11); FF(bb, cc, dd, ee, aa, x[ 9], 13);
    FF(aa, bb, cc, dd, ee, x[10], 14); FF(ee, aa, bb, cc, dd, x[11], 15);
    FF(dd, ee, aa, bb, cc, x[12],  6); FF(cc, dd, ee, aa, bb, x[13],  7);
    FF(bb, cc, dd, ee, aa, x[14],  9); FF(aa, bb, cc, dd, ee, x[15],  8);
    /* round 2 */
    GG(ee, aa, bb, cc, dd, x[ 7],  7); GG(dd, ee, aa, bb, cc, x[ 4],  6);
    GG(cc, dd, ee, aa, bb, x[13],  8); GG(bb, cc, dd, ee, aa, x[ 1], 13);
    GG(aa, bb, cc, dd, ee, x[10], 11); GG(ee, aa, bb, cc, dd, x[ 6],  9);
    GG(dd, ee, aa, bb, cc, x[15],  7); GG(cc, dd, ee, aa, bb, x[ 3], 15);
    GG(bb, cc, dd, ee, aa, x[12],  7); GG(aa, bb, cc, dd, ee, x[ 0], 12);
    GG(ee, aa, bb, cc, dd, x[ 9], 15); GG(dd, ee, aa, bb, cc, x[ 5],  9);
    GG(cc, dd, ee, aa, bb, x[ 2], 11); GG(bb, cc, dd, ee, aa, x[14],  7);
    GG(aa, bb, cc, dd, ee, x[11], 13); GG(ee, aa, bb, cc, dd, x[ 8], 12);
    /* round 3 */
    HH(dd, ee, aa, bb, cc, x[ 3], 11); HH(cc, dd, ee, aa, bb, x[10], 13);
    HH(bb, cc, dd, ee, aa, x[14],  6); HH(aa, bb, cc, dd, ee, x[ 4],  7);
    HH(ee, aa, bb, cc, dd, x[ 9], 14); HH(dd, ee, aa, bb, cc, x[15],  9);
    HH(cc, dd, ee, aa, bb, x[ 8], 13); HH(bb, cc, dd, ee, aa, x[ 1], 15);
    HH(aa, bb, cc, dd, ee, x[ 2], 14); HH(ee, aa, bb, cc, dd, x[ 7],  8);
    HH(dd, ee, aa, bb, cc, x[ 0], 13); HH(cc, dd, ee, aa, bb, x[ 6],  6);
    HH(bb, cc, dd, ee, aa, x[13],  5); HH(aa, bb, cc, dd, ee, x[11], 12);
    HH(ee, aa, bb, cc, dd, x[ 5],  7); HH(dd, ee, aa, bb, cc, x[12],  5);
    /* round 4 */
    II(cc, dd, ee, aa, bb, x[ 1], 11); II(bb, cc, dd, ee, aa, x[ 9], 12);
    II(aa, bb, cc, dd, ee, x[11], 14); II(ee, aa, bb, cc, dd, x[10], 15);
    II(dd, ee, aa, bb, cc, x[ 0], 14); II(cc, dd, ee, aa, bb, x[ 8], 15);
    II(bb, cc, dd, ee, aa, x[12],  9); II(aa, bb, cc, dd, ee, x[ 4],  8);
    II(ee, aa, bb, cc, dd, x[13],  9); II(dd, ee, aa, bb, cc, x[ 3], 14);
    II(cc, dd, ee, aa, bb, x[ 7],  5); II(bb, cc, dd, ee, aa, x[15],  6);
    II(aa, bb, cc, dd, ee, x[14],  8); II(ee, aa, bb, cc, dd, x[ 5],  6);
    II(dd, ee, aa, bb, cc, x[ 6],  5); II(cc, dd, ee, aa, bb, x[ 2], 12);
    /* round 5 */
    JJ(bb, cc, dd, ee, aa, x[ 4],  9); JJ(aa, bb, cc, dd, ee, x[ 0], 15);
    JJ(ee, aa, bb, cc, dd, x[ 5],  5); JJ(dd, ee, aa, bb, cc, x[ 9], 11);
    JJ(cc, dd, ee, aa, bb, x[ 7],  6); JJ(bb, cc, dd, ee, aa, x[12],  8);
    JJ(aa, bb, cc, dd, ee, x[ 2], 13); JJ(ee, aa, bb, cc, dd, x[10], 12);
    JJ(dd, ee, aa, bb, cc, x[14],  5); JJ(cc, dd, ee, aa, bb, x[ 1], 12);
    JJ(bb, cc, dd, ee, aa, x[ 3], 13); JJ(aa, bb, cc, dd, ee, x[ 8], 14);
    JJ(ee, aa, bb, cc, dd, x[11], 11); JJ(dd, ee, aa, bb, cc, x[ 6],  8);
    JJ(cc, dd, ee, aa, bb, x[15],  5); JJ(bb, cc, dd, ee, aa, x[13],  6);
    /* parallel round 1 */
    JJJ(aaa, bbb, ccc, ddd, eee, x[ 5],  8);
    JJJ(eee, aaa, bbb, ccc, ddd, x[14],  9);
    JJJ(ddd, eee, aaa, bbb, ccc, x[ 7],  9);
    JJJ(ccc, ddd, eee, aaa, bbb, x[ 0], 11);
    JJJ(bbb, ccc, ddd, eee, aaa, x[ 9], 13);
    JJJ(aaa, bbb, ccc, ddd, eee, x[ 2], 15);
    JJJ(eee, aaa, bbb, ccc, ddd, x[11], 15);
    JJJ(ddd, eee, aaa, bbb, ccc, x[ 4],  5);
    JJJ(ccc, ddd, eee, aaa, bbb, x[13],  7);
    JJJ(bbb, ccc, ddd, eee, aaa, x[ 6],  7);
    JJJ(aaa, bbb, ccc, ddd, eee, x[15],  8);
    JJJ(eee, aaa, bbb, ccc, ddd, x[ 8], 11);
    JJJ(ddd, eee, aaa, bbb, ccc, x[ 1], 14);
    JJJ(ccc, ddd, eee, aaa, bbb, x[10], 14);
    JJJ(bbb, ccc, ddd, eee, aaa, x[ 3], 12);
    JJJ(aaa, bbb, ccc, ddd, eee, x[12],  6);
    /* parallel round 2 */
    III(eee, aaa, bbb, ccc, ddd, x[ 6],  9); 
    III(ddd, eee, aaa, bbb, ccc, x[11], 13);
    III(ccc, ddd, eee, aaa, bbb, x[ 3], 15);
    III(bbb, ccc, ddd, eee, aaa, x[ 7],  7);
    III(aaa, bbb, ccc, ddd, eee, x[ 0], 12);
    III(eee, aaa, bbb, ccc, ddd, x[13],  8);
    III(ddd, eee, aaa, bbb, ccc, x[ 5],  9);
    III(ccc, ddd, eee, aaa, bbb, x[10], 11);
    III(bbb, ccc, ddd, eee, aaa, x[14],  7);
    III(aaa, bbb, ccc, ddd, eee, x[15],  7);
    III(eee, aaa, bbb, ccc, ddd, x[ 8], 12);
    III(ddd, eee, aaa, bbb, ccc, x[12],  7);
    III(ccc, ddd, eee, aaa, bbb, x[ 4],  6);
    III(bbb, ccc, ddd, eee, aaa, x[ 9], 15);
    III(aaa, bbb, ccc, ddd, eee, x[ 1], 13);
    III(eee, aaa, bbb, ccc, ddd, x[ 2], 11);
    /* parallel round 3 */
    HHH(ddd, eee, aaa, bbb, ccc, x[15],  9);
    HHH(ccc, ddd, eee, aaa, bbb, x[ 5],  7);
    HHH(bbb, ccc, ddd, eee, aaa, x[ 1], 15);
    HHH(aaa, bbb, ccc, ddd, eee, x[ 3], 11);
    HHH(eee, aaa, bbb, ccc, ddd, x[ 7],  8);
    HHH(ddd, eee, aaa, bbb, ccc, x[14],  6);
    HHH(ccc, ddd, eee, aaa, bbb, x[ 6],  6);
    HHH(bbb, ccc, ddd, eee, aaa, x[ 9], 14);
    HHH(aaa, bbb, ccc, ddd, eee, x[11], 12);
    HHH(eee, aaa, bbb, ccc, ddd, x[ 8], 13);
    HHH(ddd, eee, aaa, bbb, ccc, x[12],  5);
    HHH(ccc, ddd, eee, aaa, bbb, x[ 2], 14);
    HHH(bbb, ccc, ddd, eee, aaa, x[10], 13);
    HHH(aaa, bbb, ccc, ddd, eee, x[ 0], 13);
    HHH(eee, aaa, bbb, ccc, ddd, x[ 4],  7);
    HHH(ddd, eee, aaa, bbb, ccc, x[13],  5);
    /* parallel round 4 */   
    GGG(ccc, ddd, eee, aaa, bbb, x[ 8], 15);
    GGG(bbb, ccc, ddd, eee, aaa, x[ 6],  5);
    GGG(aaa, bbb, ccc, ddd, eee, x[ 4],  8);
    GGG(eee, aaa, bbb, ccc, ddd, x[ 1], 11);
    GGG(ddd, eee, aaa, bbb, ccc, x[ 3], 14);
    GGG(ccc, ddd, eee, aaa, bbb, x[11], 14);
    GGG(bbb, ccc, ddd, eee, aaa, x[15],  6);
    GGG(aaa, bbb, ccc, ddd, eee, x[ 0], 14);
    GGG(eee, aaa, bbb, ccc, ddd, x[ 5],  6);
    GGG(ddd, eee, aaa, bbb, ccc, x[12],  9);
    GGG(ccc, ddd, eee, aaa, bbb, x[ 2], 12);
    GGG(bbb, ccc, ddd, eee, aaa, x[13],  9);
    GGG(aaa, bbb, ccc, ddd, eee, x[ 9], 12);
    GGG(eee, aaa, bbb, ccc, ddd, x[ 7],  5);
    GGG(ddd, eee, aaa, bbb, ccc, x[10], 15);
    GGG(ccc, ddd, eee, aaa, bbb, x[14],  8);
    /* parallel round 5 */
    FFF(bbb, ccc, ddd, eee, aaa, x[12] ,  8);
    FFF(aaa, bbb, ccc, ddd, eee, x[15] ,  5);
    FFF(eee, aaa, bbb, ccc, ddd, x[10] , 12);
    FFF(ddd, eee, aaa, bbb, ccc, x[ 4] ,  9);
    FFF(ccc, ddd, eee, aaa, bbb, x[ 1] , 12);
    FFF(bbb, ccc, ddd, eee, aaa, x[ 5] ,  5);
    FFF(aaa, bbb, ccc, ddd, eee, x[ 8] , 14);
    FFF(eee, aaa, bbb, ccc, ddd, x[ 7] ,  6);
    FFF(ddd, eee, aaa, bbb, ccc, x[ 6] ,  8);
    FFF(ccc, ddd, eee, aaa, bbb, x[ 2] , 13);
    FFF(bbb, ccc, ddd, eee, aaa, x[13] ,  6);
    FFF(aaa, bbb, ccc, ddd, eee, x[14] ,  5);
    FFF(eee, aaa, bbb, ccc, ddd, x[ 0] , 15);
    FFF(ddd, eee, aaa, bbb, ccc, x[ 3] , 13);
    FFF(ccc, ddd, eee, aaa, bbb, x[ 9] , 11);
    FFF(bbb, ccc, ddd, eee, aaa, x[11] , 11);
    /* combine results */
    ddd += cc + this->state[ 1 ];               /* final result for MDbuf[0] */
    this->state[ 1 ] = this->state[ 2 ] + dd + eee;
    this->state[ 2 ] = this->state[ 3 ] + ee + aaa;
    this->state[ 3 ] = this->state[ 4 ] + aa + bbb;
    this->state[ 4 ] = this->state[ 0 ] + bb + ccc;
    this->state[ 0 ] = ddd;
  }

  virtual bool selftest( void ) {
    static const byte mt[] = {
      0x9c, 0x11, 0x85, 0xa5, 0xc5, 0xe9, 0xfc, 0x54, 0x61, 0x28, 0x08, 0x97,
      0x7e, 0xe8, 0xf5, 0x48, 0xb2, 0x25, 0x8d, 0x31 };
    static const byte a[] = {
      0x0b, 0xdc, 0x9d, 0x2d, 0x25, 0x6b, 0x3e, 0xe9, 0xda, 0xae, 0x34, 0x7b,
      0xe6, 0xf4, 0xdc, 0x83, 0x5a, 0x46, 0x7f, 0xfe };
    static const byte eighty[] = {
      0x9b, 0x75, 0x2e, 0x45, 0x57, 0x3d, 0x4b, 0x39, 0xf4, 0xdb, 0xd3, 0x32,
      0x3c, 0xab, 0x82, 0xbf, 0x63, 0x32, 0x6b, 0xfb };
    this->init();
    this->update( (const byte *) "", 0 );
    this->final();
    if ( ::memcmp( this->state, mt, 160 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "a", 1 );
    this->final();
    if ( ::memcmp( this->state, a, 160 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "12345678901234567890123456789012345"
                                 "67890123456789012345678901234567890"
                                 "1234567890", 80 );
    this->final();
    if ( ::memcmp( this->state, eighty, 160 / 8 ) != 0 )
      return false;
    return true;
  }

  void * operator new( size_t sz, void *ptr ) { return ptr; }
  SYS_OPS( RIPEMD160Ctx );
  RIPEMD160Ctx() {}
  virtual ~RIPEMD160Ctx() {}
  virtual HashContext *dup( void ) {
    return NEW RIPEMD160Ctx();
  }
  virtual HashContext *dup2( void *p, unsigned int &sz ) {
    if ( sz >= sizeof( RIPEMD160Ctx ) ) {
      sz = sizeof( RIPEMD160Ctx );
      return new ( p ) RIPEMD160Ctx();
    }
    sz = sizeof( RIPEMD160Ctx );
    return NULL;
  }
};


void
Hash160::ripemd( const byte *buf,  unsigned int bufLen,  void *digest )
{
  RIPEMD160Ctx ctx;

  ctx.init();
  ctx.update( buf, bufLen );
  ctx.final();

  ::memcpy( digest, ctx.state, sizeof( ctx.state ) );
}


struct SHA256Ctx : public RSA_Hash<256, 512/8, false, unsigned int> {
  unsigned int work[ 64 ];
  static inline unsigned int ROR( unsigned int v,  unsigned int s ) {
    return (v >> s) | (v << (32 - s));
  }
  static inline unsigned int G0( unsigned int v ) {
    return ROR( v, 7 ) ^ ROR( v, 18 ) ^ ( v >> 3 );
  }
  static inline unsigned int G1( unsigned int v ) {
    return ROR( v, 17 ) ^ ROR( v, 19 ) ^ ( v >> 10 );
  }
  static inline unsigned int S0( unsigned int v ) {
    return ROR( v, 2 ) ^ ROR( v, 13 ) ^ ROR( v, 22 );
  }
  static inline unsigned int S1( unsigned int v ) {
    return ROR( v, 6 ) ^ ROR( v, 11 ) ^ ROR( v, 25 );
  }
  inline unsigned int R( unsigned int t ) {
    return this->work[ t ] = G1( this->work[ t - 2 ] ) + this->work[ t - 7 ] +
                             G0( this->work[ t - 15 ] ) + this->work[ t - 16 ];
  }
  static inline unsigned int MAJ( unsigned int x,  unsigned int y,
                                  unsigned int z ) {
    return ( x & y ) | ( z & ( x | y ) );
  }
  static inline unsigned int CH( unsigned int x,  unsigned int y,
                                 unsigned int z ) {
    return z ^ ( x & ( y ^ z ) );
  }
  static inline void P( unsigned int a, unsigned int b, unsigned int c,
                        unsigned int &d, unsigned int e, unsigned int f,
                        unsigned int g, unsigned int &h, unsigned int x,
                        unsigned int K ) {
    register unsigned int t1 = h + S1( e ) + CH( e, f, g ) + K + x,
                          t2 = S0( a ) + MAJ( a, b, c );
    d += t1;
    h  = t1 + t2;
  }

  virtual void init( void ) {
    this->state[ 0 ] = 0x6A09E667U;
    this->state[ 1 ] = 0xBB67AE85U;
    this->state[ 2 ] = 0x3C6EF372U;
    this->state[ 3 ] = 0xA54FF53AU;
    this->state[ 4 ] = 0x510E527FU;
    this->state[ 5 ] = 0x9B05688CU;
    this->state[ 6 ] = 0x1F83D9ABU;
    this->state[ 7 ] = 0x5BE0CD19U;
    this->count = 0;
  }

  virtual void transform( unsigned int *x ) {
    unsigned int a, b, c, d, e, f, g, h;

    ::memcpy( this->work, x, 64 );
    if ( Aligned::isLittleEndian ) {
      for ( a = 0; a < 64 / 4; a++ )
        Aligned::swap( this->work[ a ] );
    }
    a = this->state[ 0 ];
    b = this->state[ 1 ]; 
    c = this->state[ 2 ];
    d = this->state[ 3 ];
    e = this->state[ 4 ];
    f = this->state[ 5 ];
    g = this->state[ 6 ];
    h = this->state[ 7 ];

    P( a, b, c, d, e, f, g, h, this->work[ 0], 0x428A2F98U );
    P( h, a, b, c, d, e, f, g, this->work[ 1], 0x71374491U );
    P( g, h, a, b, c, d, e, f, this->work[ 2], 0xB5C0FBCFU );
    P( f, g, h, a, b, c, d, e, this->work[ 3], 0xE9B5DBA5U );
    P( e, f, g, h, a, b, c, d, this->work[ 4], 0x3956C25BU );
    P( d, e, f, g, h, a, b, c, this->work[ 5], 0x59F111F1U );
    P( c, d, e, f, g, h, a, b, this->work[ 6], 0x923F82A4U );
    P( b, c, d, e, f, g, h, a, this->work[ 7], 0xAB1C5ED5U );
    P( a, b, c, d, e, f, g, h, this->work[ 8], 0xD807AA98U );
    P( h, a, b, c, d, e, f, g, this->work[ 9], 0x12835B01U );
    P( g, h, a, b, c, d, e, f, this->work[10], 0x243185BEU );
    P( f, g, h, a, b, c, d, e, this->work[11], 0x550C7DC3U );
    P( e, f, g, h, a, b, c, d, this->work[12], 0x72BE5D74U );
    P( d, e, f, g, h, a, b, c, this->work[13], 0x80DEB1FEU );
    P( c, d, e, f, g, h, a, b, this->work[14], 0x9BDC06A7U );
    P( b, c, d, e, f, g, h, a, this->work[15], 0xC19BF174U );
    P( a, b, c, d, e, f, g, h, this->R(16), 0xE49B69C1U );
    P( h, a, b, c, d, e, f, g, this->R(17), 0xEFBE4786U );
    P( g, h, a, b, c, d, e, f, this->R(18), 0x0FC19DC6U );
    P( f, g, h, a, b, c, d, e, this->R(19), 0x240CA1CCU );
    P( e, f, g, h, a, b, c, d, this->R(20), 0x2DE92C6FU );
    P( d, e, f, g, h, a, b, c, this->R(21), 0x4A7484AAU );
    P( c, d, e, f, g, h, a, b, this->R(22), 0x5CB0A9DCU );
    P( b, c, d, e, f, g, h, a, this->R(23), 0x76F988DAU );
    P( a, b, c, d, e, f, g, h, this->R(24), 0x983E5152U );
    P( h, a, b, c, d, e, f, g, this->R(25), 0xA831C66DU );
    P( g, h, a, b, c, d, e, f, this->R(26), 0xB00327C8U );
    P( f, g, h, a, b, c, d, e, this->R(27), 0xBF597FC7U );
    P( e, f, g, h, a, b, c, d, this->R(28), 0xC6E00BF3U );
    P( d, e, f, g, h, a, b, c, this->R(29), 0xD5A79147U );
    P( c, d, e, f, g, h, a, b, this->R(30), 0x06CA6351U );
    P( b, c, d, e, f, g, h, a, this->R(31), 0x14292967U );
    P( a, b, c, d, e, f, g, h, this->R(32), 0x27B70A85U );
    P( h, a, b, c, d, e, f, g, this->R(33), 0x2E1B2138U );
    P( g, h, a, b, c, d, e, f, this->R(34), 0x4D2C6DFCU );
    P( f, g, h, a, b, c, d, e, this->R(35), 0x53380D13U );
    P( e, f, g, h, a, b, c, d, this->R(36), 0x650A7354U );
    P( d, e, f, g, h, a, b, c, this->R(37), 0x766A0ABBU );
    P( c, d, e, f, g, h, a, b, this->R(38), 0x81C2C92EU );
    P( b, c, d, e, f, g, h, a, this->R(39), 0x92722C85U );
    P( a, b, c, d, e, f, g, h, this->R(40), 0xA2BFE8A1U );
    P( h, a, b, c, d, e, f, g, this->R(41), 0xA81A664BU );
    P( g, h, a, b, c, d, e, f, this->R(42), 0xC24B8B70U );
    P( f, g, h, a, b, c, d, e, this->R(43), 0xC76C51A3U );
    P( e, f, g, h, a, b, c, d, this->R(44), 0xD192E819U );
    P( d, e, f, g, h, a, b, c, this->R(45), 0xD6990624U );
    P( c, d, e, f, g, h, a, b, this->R(46), 0xF40E3585U );
    P( b, c, d, e, f, g, h, a, this->R(47), 0x106AA070U );
    P( a, b, c, d, e, f, g, h, this->R(48), 0x19A4C116U );
    P( h, a, b, c, d, e, f, g, this->R(49), 0x1E376C08U );
    P( g, h, a, b, c, d, e, f, this->R(50), 0x2748774CU );
    P( f, g, h, a, b, c, d, e, this->R(51), 0x34B0BCB5U );
    P( e, f, g, h, a, b, c, d, this->R(52), 0x391C0CB3U );
    P( d, e, f, g, h, a, b, c, this->R(53), 0x4ED8AA4AU );
    P( c, d, e, f, g, h, a, b, this->R(54), 0x5B9CCA4FU );
    P( b, c, d, e, f, g, h, a, this->R(55), 0x682E6FF3U );
    P( a, b, c, d, e, f, g, h, this->R(56), 0x748F82EEU );
    P( h, a, b, c, d, e, f, g, this->R(57), 0x78A5636FU );
    P( g, h, a, b, c, d, e, f, this->R(58), 0x84C87814U );
    P( f, g, h, a, b, c, d, e, this->R(59), 0x8CC70208U );
    P( e, f, g, h, a, b, c, d, this->R(60), 0x90BEFFFAU );
    P( d, e, f, g, h, a, b, c, this->R(61), 0xA4506CEBU );
    P( c, d, e, f, g, h, a, b, this->R(62), 0xBEF9A3F7U );
    P( b, c, d, e, f, g, h, a, this->R(63), 0xC67178F2U );

    this->state[ 0 ] += a;
    this->state[ 1 ] += b; 
    this->state[ 2 ] += c;
    this->state[ 3 ] += d;
    this->state[ 4 ] += e;
    this->state[ 5 ] += f;
    this->state[ 6 ] += g;
    this->state[ 7 ] += h;
  }

  virtual bool selftest( void ) {
    static const byte abc[] = {
      0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde,
      0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
      0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad };
    static const byte abcdbc[] = {
      0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8, 0xe5, 0xc0, 0x26, 0x93,
      0x0c, 0x3e, 0x60, 0x39, 0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
      0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1 };
    this->init();
    this->update( (const byte *) "abc", 3 );
    this->final();
    if ( ::memcmp( this->state, abc, 256 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "abcdbcdecdefdefgefghfghighi"
                                 "jhijkijkljklmklmnlmnomnopnopq", 56 );
    this->final();
    if ( ::memcmp( this->state, abcdbc, 256 / 8 ) != 0 )
      return false;
    return true;
  }

  void * operator new( size_t sz, void *ptr ) { return ptr; }
  SYS_OPS( SHA256Ctx );
  SHA256Ctx() {}
  virtual ~SHA256Ctx() {}
  virtual HashContext *dup( void ) {
    return NEW SHA256Ctx();
  }
  virtual HashContext *dup2( void *p, unsigned int &sz ) {
    if ( sz >= sizeof( SHA256Ctx ) ) {
      sz = sizeof( SHA256Ctx );
      return new ( p ) SHA256Ctx();
    }
    sz = sizeof( SHA256Ctx );
    return NULL;
  }
};


void
Hash256::sha2( const byte *buf,  unsigned int bufLen,  void *digest )
{
  SHA256Ctx ctx;

  ctx.init();
  ctx.update( buf, bufLen );
  ctx.final();

  ::memcpy( digest, ctx.state, sizeof( ctx.state ) );
}

/* microsoft didn't have 0x1234ULL constants (still doesn't?) */
static inline ullong UL( unsigned int hi,  unsigned int lo ) {
  return ( (ullong) hi << 32 ) | (ullong) lo;
}

struct SHA512Ctx : public RSA_Hash<512, 1024/8, false, ullong> {
  ullong work[ 80 ];
  static inline ullong ROR( ullong v,  unsigned int s ) {
    return (v >> s) | (v << (64 - s));
  }
  static inline ullong G0( ullong v ) {
    return ROR( v, 1 ) ^ ROR( v, 8 ) ^ ( v >> 7 );
  }
  static inline ullong G1( ullong v ) {
    return ROR( v, 19 ) ^ ROR( v, 61 ) ^ ( v >> 6 );
  }
  static inline ullong S0( ullong v ) {
    return ROR( v, 28 ) ^ ROR( v, 34 ) ^ ROR( v, 39 );
  }
  static inline ullong S1( ullong v ) {
    return ROR( v, 14 ) ^ ROR( v, 18 ) ^ ROR( v, 41 );
  }
  inline void BLEND( unsigned int t ) {
    this->work[ t ] = G1( this->work[ t - 2 ] ) + this->work[ t - 7 ] +
                      G0( this->work[ t - 15 ] ) + this->work[ t - 16 ];
  }
  static inline ullong MAJ( ullong x,  ullong y,  ullong z ) {
    return ( x & y ) | ( z & ( x | y ) );
  }
  static inline ullong CH( ullong x,  ullong y,  ullong z ) {
    return z ^ ( x & ( y ^ z ) );
  }

  virtual void init( void ) {
    this->state[ 0 ] = UL( 0x6a09e667U, 0xf3bcc908U );
    this->state[ 1 ] = UL( 0xbb67ae85U, 0x84caa73bU );
    this->state[ 2 ] = UL( 0x3c6ef372U, 0xfe94f82bU );
    this->state[ 3 ] = UL( 0xa54ff53aU, 0x5f1d36f1U );
    this->state[ 4 ] = UL( 0x510e527fU, 0xade682d1U );
    this->state[ 5 ] = UL( 0x9b05688cU, 0x2b3e6c1fU );
    this->state[ 6 ] = UL( 0x1f83d9abU, 0xfb41bd6bU );
    this->state[ 7 ] = UL( 0x5be0cd19U, 0x137e2179U );
    this->count = 0;
  }

  static const ullong K[ 80 ];

  virtual void transform( ullong *x ) {
    ullong a, b, c, d, e, f, g, h, t1, t2;
    unsigned int i;

    ::memcpy( this->work, x, 128 );
    if ( Aligned::isLittleEndian ) {
      for ( i = 0; i < 128 / 8; i++ )
        Aligned::swap( this->work[ i ] );
    }
    for ( i = 16; i < 80; i++ )
      BLEND( i );
    a = this->state[ 0 ];
    b = this->state[ 1 ]; 
    c = this->state[ 2 ];
    d = this->state[ 3 ];
    e = this->state[ 4 ];
    f = this->state[ 5 ];
    g = this->state[ 6 ];
    h = this->state[ 7 ];

    for ( unsigned int i = 0; i < 80; i += 8 ) {
      t1 = h + S1( e ) + CH( e,f,g ) + K[ i   ] + this->work[ i   ];
      t2 = S0( a ) + MAJ( a,b,c );     d += t1;   h = t1 + t2;
      t1 = g + S1( d ) + CH( d,e,f ) + K[ i+1 ] + this->work[ i+1 ];
      t2 = S0( h ) + MAJ( h,a,b );     c += t1;   g = t1 + t2;
      t1 = f + S1( c ) + CH( c,d,e ) + K[ i+2 ] + this->work[ i+2 ];
      t2 = S0( g ) + MAJ( g,h,a );     b += t1;   f = t1 + t2;
      t1 = e + S1( b ) + CH( b,c,d ) + K[ i+3 ] + this->work[ i+3 ];
      t2 = S0( f ) + MAJ( f,g,h );     a += t1;   e = t1 + t2;
      t1 = d + S1( a ) + CH( a,b,c ) + K[ i+4 ] + this->work[ i+4 ];
      t2 = S0( e ) + MAJ( e,f,g );     h += t1;   d = t1 + t2;
      t1 = c + S1( h ) + CH( h,a,b ) + K[ i+5 ] + this->work[ i+5 ];
      t2 = S0( d ) + MAJ( d,e,f );     g += t1;   c = t1 + t2;
      t1 = b + S1( g ) + CH( g,h,a ) + K[ i+6 ] + this->work[ i+6 ];
      t2 = S0( c ) + MAJ( c,d,e );     f += t1;   b = t1 + t2;
      t1 = a + S1( f ) + CH( f,g,h ) + K[ i+7 ] + this->work[ i+7 ];
      t2 = S0( b ) + MAJ( b,c,d );     e += t1;   a = t1 + t2;
    }

    this->state[ 0 ] += a;
    this->state[ 1 ] += b; 
    this->state[ 2 ] += c;
    this->state[ 3 ] += d;
    this->state[ 4 ] += e;
    this->state[ 5 ] += f;
    this->state[ 6 ] += g;
    this->state[ 7 ] += h;
  }

  virtual bool selftest( void ) {
    static const byte abc[] = {
      0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc, 0x41, 0x73, 0x49,
      0xae, 0x20, 0x41, 0x31, 0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
      0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a, 0x21, 0x92, 0x99, 0x2a,
      0x27, 0x4f, 0xc1, 0xa8, 0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
      0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e, 0x2a, 0x9a, 0xc9, 0x4f,
      0xa5, 0x4c, 0xa4, 0x9f };
    static const byte abcdbc[] = {
      0x20, 0x4a, 0x8f, 0xc6, 0xdd, 0xa8, 0x2f, 0x0a, 0x0c, 0xed, 0x7b, 0xeb,
      0x8e, 0x08, 0xa4, 0x16, 0x57, 0xc1, 0x6e, 0xf4, 0x68, 0xb2, 0x28, 0xa8,
      0x27, 0x9b, 0xe3, 0x31, 0xa7, 0x03, 0xc3, 0x35, 0x96, 0xfd, 0x15, 0xc1,
      0x3b, 0x1b, 0x07, 0xf9, 0xaa, 0x1d, 0x3b, 0xea, 0x57, 0x78, 0x9c, 0xa0,
      0x31, 0xad, 0x85, 0xc7, 0xa7, 0x1d, 0xd7, 0x03, 0x54, 0xec, 0x63, 0x12,
      0x38, 0xca, 0x34, 0x45 };
    this->init();
    this->update( (const byte *) "abc", 3 );
    this->final();
    if ( ::memcmp( this->state, abc, 512 / 8 ) != 0 )
      return false;
    this->init();
    this->update( (const byte *) "abcdbcdecdefdefgefghfghighi"
                                 "jhijkijkljklmklmnlmnomnopnopq", 56 );
    this->final();
    if ( ::memcmp( this->state, abcdbc, 512 / 8 ) != 0 )
      return false;
    return true;
  }

  void * operator new( size_t sz, void *ptr ) { return ptr; }
  SYS_OPS( SHA512Ctx );
  SHA512Ctx() {}
  virtual ~SHA512Ctx() {}
  virtual HashContext *dup( void ) {
    return NEW SHA512Ctx();
  }
  virtual HashContext *dup2( void *p, unsigned int &sz ) {
    if ( sz >= sizeof( SHA512Ctx ) ) {
      sz = sizeof( SHA512Ctx );
      return new ( p ) SHA512Ctx();
    }
    sz = sizeof( SHA512Ctx );
    return NULL;
  }
};

const ullong SHA512Ctx::K[] = {
  UL( 0x428a2f98U, 0xd728ae22U ), UL( 0x71374491U, 0x23ef65cdU ),
  UL( 0xb5c0fbcfU, 0xec4d3b2fU ), UL( 0xe9b5dba5U, 0x8189dbbcU ),
  UL( 0x3956c25bU, 0xf348b538U ), UL( 0x59f111f1U, 0xb605d019U ),
  UL( 0x923f82a4U, 0xaf194f9bU ), UL( 0xab1c5ed5U, 0xda6d8118U ),
  UL( 0xd807aa98U, 0xa3030242U ), UL( 0x12835b01U, 0x45706fbeU ),
  UL( 0x243185beU, 0x4ee4b28cU ), UL( 0x550c7dc3U, 0xd5ffb4e2U ),
  UL( 0x72be5d74U, 0xf27b896fU ), UL( 0x80deb1feU, 0x3b1696b1U ),
  UL( 0x9bdc06a7U, 0x25c71235U ), UL( 0xc19bf174U, 0xcf692694U ),
  UL( 0xe49b69c1U, 0x9ef14ad2U ), UL( 0xefbe4786U, 0x384f25e3U ),
  UL( 0x0fc19dc6U, 0x8b8cd5b5U ), UL( 0x240ca1ccU, 0x77ac9c65U ),
  UL( 0x2de92c6fU, 0x592b0275U ), UL( 0x4a7484aaU, 0x6ea6e483U ),
  UL( 0x5cb0a9dcU, 0xbd41fbd4U ), UL( 0x76f988daU, 0x831153b5U ),
  UL( 0x983e5152U, 0xee66dfabU ), UL( 0xa831c66dU, 0x2db43210U ),
  UL( 0xb00327c8U, 0x98fb213fU ), UL( 0xbf597fc7U, 0xbeef0ee4U ),
  UL( 0xc6e00bf3U, 0x3da88fc2U ), UL( 0xd5a79147U, 0x930aa725U ),
  UL( 0x06ca6351U, 0xe003826fU ), UL( 0x14292967U, 0x0a0e6e70U ),
  UL( 0x27b70a85U, 0x46d22ffcU ), UL( 0x2e1b2138U, 0x5c26c926U ),
  UL( 0x4d2c6dfcU, 0x5ac42aedU ), UL( 0x53380d13U, 0x9d95b3dfU ),
  UL( 0x650a7354U, 0x8baf63deU ), UL( 0x766a0abbU, 0x3c77b2a8U ),
  UL( 0x81c2c92eU, 0x47edaee6U ), UL( 0x92722c85U, 0x1482353bU ),
  UL( 0xa2bfe8a1U, 0x4cf10364U ), UL( 0xa81a664bU, 0xbc423001U ),
  UL( 0xc24b8b70U, 0xd0f89791U ), UL( 0xc76c51a3U, 0x0654be30U ),
  UL( 0xd192e819U, 0xd6ef5218U ), UL( 0xd6990624U, 0x5565a910U ),
  UL( 0xf40e3585U, 0x5771202aU ), UL( 0x106aa070U, 0x32bbd1b8U ),
  UL( 0x19a4c116U, 0xb8d2d0c8U ), UL( 0x1e376c08U, 0x5141ab53U ),
  UL( 0x2748774cU, 0xdf8eeb99U ), UL( 0x34b0bcb5U, 0xe19b48a8U ),
  UL( 0x391c0cb3U, 0xc5c95a63U ), UL( 0x4ed8aa4aU, 0xe3418acbU ),
  UL( 0x5b9cca4fU, 0x7763e373U ), UL( 0x682e6ff3U, 0xd6b2b8a3U ),
  UL( 0x748f82eeU, 0x5defb2fcU ), UL( 0x78a5636fU, 0x43172f60U ),
  UL( 0x84c87814U, 0xa1f0ab72U ), UL( 0x8cc70208U, 0x1a6439ecU ),
  UL( 0x90befffaU, 0x23631e28U ), UL( 0xa4506cebU, 0xde82bde9U ),
  UL( 0xbef9a3f7U, 0xb2c67915U ), UL( 0xc67178f2U, 0xe372532bU ),
  UL( 0xca273eceU, 0xea26619cU ), UL( 0xd186b8c7U, 0x21c0c207U ),
  UL( 0xeada7dd6U, 0xcde0eb1eU ), UL( 0xf57d4f7fU, 0xee6ed178U ),
  UL( 0x06f067aaU, 0x72176fbaU ), UL( 0x0a637dc5U, 0xa2c898a6U ),
  UL( 0x113f9804U, 0xbef90daeU ), UL( 0x1b710b35U, 0x131c471bU ),
  UL( 0x28db77f5U, 0x23047d84U ), UL( 0x32caab7bU, 0x40c72493U ),
  UL( 0x3c9ebe0aU, 0x15c9bebcU ), UL( 0x431d67c4U, 0x9c100d4cU ),
  UL( 0x4cc5d4beU, 0xcb3e42b6U ), UL( 0x597f299cU, 0xfc657e2aU ),
  UL( 0x5fcb6fabU, 0x3ad6faecU ), UL( 0x6c44198cU, 0x4a475817U ) };


void
Hash512::sha2( const byte *buf,  unsigned int bufLen,  void *digest )
{
  SHA512Ctx ctx;

  ctx.init();
  ctx.update( buf, bufLen );
  ctx.final();

  ::memcpy( digest, ctx.state, sizeof( ctx.state ) );
}


HashContext *
HashContext::create( HashType type )
{
  if ( type == MD4 )
    return NEW MD4Ctx();
  if ( type == MD5 )
    return NEW MD5Ctx();
  if ( type == SHA1 )
    return NEW SHA1Ctx();
  if ( type == RIPEMD128 )
    return NEW RIPEMD128Ctx();
  if ( type == RIPEMD160 )
    return NEW RIPEMD160Ctx();
  if ( type == SHA256 )
    return NEW SHA256Ctx();
  if ( type == SHA512 )
    return NEW SHA512Ctx();
  return NULL;
}


namespace rai {
/* derived from
 * http://en.wikipedia.org/wiki/Hash-based_message_authentication_code#Implementation */
struct HMACCtx : public HashContext {
  static const unsigned int MAX_BLOCK_SIZE = 256; /* 2 * SHA512 */
  HashContext *ctx;
  byte         iPad[ MAX_BLOCK_SIZE ],
               oPad[ MAX_BLOCK_SIZE ];
  unsigned int blksz;
  bool         isAlloced;

  virtual ~HMACCtx() {
    if ( this->isAlloced && this->ctx != NULL )
      delete this->ctx;
  }

  void * operator new( size_t sz, void *ptr ) { return ptr; }
  SYS_OPS( HMACCtx );
  HMACCtx( HashContext *c,  const byte *key, unsigned int keyLen, bool isAll ) {
    this->blksz = c->blockSize();
    this->ctx   = c;
    this->isAlloced = isAll;

    if ( keyLen > 0 ) {
      byte keyBuf[ MAX_BLOCK_SIZE ];
      if ( keyLen > this->blksz ) {
        c->init();
        c->update( key, keyLen );
        c->final();
        c->digest( keyBuf );
        keyLen = c->digestSize();
      }
      else {
        ::memcpy( keyBuf, key, keyLen );
      }
      if ( keyLen < this->blksz )
        ::memset( &keyBuf[ keyLen ], 0, this->blksz - keyLen );

      for ( unsigned int i = 0; i < this->blksz; i++ ) {
        this->oPad[ i ] = 0x5c ^ keyBuf[ i ];
        this->iPad[ i ] = 0x36 ^ keyBuf[ i ];
      }
    }
  }

  virtual void init( void ) {
    this->ctx->init();
    this->ctx->update( this->iPad, this->blksz );
  }
  virtual void update( const byte *input,  unsigned int inputLen ) {
    this->ctx->update( input, inputLen );
  }

  virtual void final( void ) {
    byte dig[ MAX_BLOCK_SIZE ];

    this->ctx->final();
    this->ctx->digest( dig );
    this->ctx->init();
    this->ctx->update( this->oPad, this->blksz );
    this->ctx->update( dig, this->ctx->digestSize() );
    this->ctx->final();
  }

  virtual unsigned int digestSize( void ) {
    return this->ctx->digestSize();
  }
  virtual unsigned int blockSize( void ) {
    return this->blksz;
  }
  virtual void digest( void *d ) {
    this->ctx->digest( d );
  }
  virtual bool selftest( void ) {
    return true; /* need to know the underlying hash */
  }
  virtual HashContext *dup( void ) {
    HMACCtx *hmac = NEW HMACCtx( this->ctx->dup(), NULL, 0, true );
    ::memcpy( hmac->iPad, this->iPad, this->blksz );
    ::memcpy( hmac->oPad, this->oPad, this->blksz );
    return hmac;
  }
  virtual HashContext *dup2( void *p, unsigned int &sz ) {
    unsigned int sz2 = sz;
    HashContext *ctx2 = this->ctx->dup2( p, sz );
    if ( ctx2 != NULL && sz2 - sz >= sizeof( HMACCtx ) ) {
      p = (void *) &((byte *) p)[ sz ];
      sz += sizeof( HMACCtx );
      HMACCtx *hmac = new ( p ) HMACCtx( ctx2, NULL, 0, false );
      ::memcpy( hmac->iPad, this->iPad, this->blksz );
      ::memcpy( hmac->oPad, this->oPad, this->blksz );
      return hmac;
    }
    sz += sizeof( HMACCtx );
    return NULL;
  }
};
}


HashContext *
HashContext::createHMAC( HashType type,  const byte *key,
                         unsigned int keyLen )
{
  HashContext *ctx = HashContext::create( type );
  if ( ctx == NULL )
    return NULL;
  return NEW HMACCtx( ctx, key, keyLen, true );
}


#define ub4 unsigned int
#define ub1 unsigned char

inline static ub4
ind( ub4 *mm, ub4 x ) {
  return (*(ub4 *)((ub1 *)(mm) + ((x) & ((Random::Isaac::RANDSIZ-1)<<2))));
}


inline static void
rngstep( ub4 mix, ub4 &a, ub4 &b, ub4 *mm, ub4 *&m, ub4 *&m2, ub4 *&r, ub4 &x,
         ub4 &y )
{
  x = *m;
  a = (a^(mix)) + *(m2++);
  *(m++) = y = ind(mm,x) + a + b;
  *(r++) = b = ind(mm,y>>Random::Isaac::RANDSIZL) + x;
}


void
Random::Isaac::fill( void )
{
  ub4 a, b, x, y, *m, *mm, *m2, *r, *mend;

  mm=this->randmem; r=this->u.randrsli; 
  a = this->randa; b = this->randb + (++this->randc);

  for (m = mm, mend = m2 = m+(RANDSIZ/2); m<mend; ) {
    rngstep( a<<13, a, b, mm, m, m2, r, x, y );
    rngstep( a>>6 , a, b, mm, m, m2, r, x, y );
    rngstep( a<<2 , a, b, mm, m, m2, r, x, y );
    rngstep( a>>16, a, b, mm, m, m2, r, x, y );
  }
  for (m2 = mm; m2<mend; ) {
    rngstep( a<<13, a, b, mm, m, m2, r, x, y );
    rngstep( a>>6 , a, b, mm, m, m2, r, x, y );
    rngstep( a<<2 , a, b, mm, m, m2, r, x, y );
    rngstep( a>>16, a, b, mm, m, m2, r, x, y );
  }
  this->randb = b; this->randa = a;
  this->randcnt = sizeof( this->u.randrslb );

  /* make prng the same regardless of endian */
  if ( ! Aligned::isLittleEndian ) {
    for ( a = 0; a < RANDSIZ; a++ )
      Aligned::swap( this->u.randrsli[ a ] );
  }
}


inline static void
mix( ub4 &a, ub4 &b, ub4 &c, ub4 &d, ub4 &e, ub4 &f, ub4 &g, ub4 &h )
{
  a^=b<<11; d+=a; b+=c;
  b^=c>>2;  e+=b; c+=d;
  c^=d<<8;  f+=c; d+=e;
  d^=e>>16; g+=d; e+=f;
  e^=f<<10; h+=e; f+=g;
  f^=g>>4;  a+=f; g+=h;
  g^=h<<8;  b+=g; h+=a;
  h^=a>>9;  c+=h; a+=b;
}  


void
Random::Isaac::init( const byte *seed,  unsigned int seedBytes )
{
  unsigned int i;
  ub4 a, b, c, d, e, f, g, h;
  ub4 *m,*r;

  this->randa = this->randb = this->randc = 0;
  m=this->randmem;
  a=b=c=d=e=f=g=h=0x9e3779b9;  /* the golden ratio */

  for (i=0; i<4; ++i) {        /* scramble it */
    mix(a,b,c,d,e,f,g,h);
  }

  if ( seed != NULL ) {
    if ( seedBytes > sizeof( this->u.randrsli ) )
      seedBytes = sizeof( this->u.randrsli );
    r=this->u.randrsli;
    ::memset( r, 0, sizeof( this->u.randrsli ) );
    ::memcpy( r, seed, seedBytes );
    if ( ! Aligned::isLittleEndian ) /* same seed produces same numbers */
      for (i=0; i<RANDSIZ; i++)
        Aligned::swap( r[i] );
    /* initialize using the contents of r[] as the seed */
    for (i=0; i<RANDSIZ; i+=8) {
      a+=r[i  ]; b+=r[i+1]; c+=r[i+2]; d+=r[i+3];
      e+=r[i+4]; f+=r[i+5]; g+=r[i+6]; h+=r[i+7];
      mix(a,b,c,d,e,f,g,h);
      m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
      m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
    } 
    /* do a second pass to make all of the seed affect all of m */
    for (i=0; i<RANDSIZ; i+=8) {
      a+=m[i  ]; b+=m[i+1]; c+=m[i+2]; d+=m[i+3];
      e+=m[i+4]; f+=m[i+5]; g+=m[i+6]; h+=m[i+7];
      mix(a,b,c,d,e,f,g,h);
      m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
      m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
    }
  }
  else {
    /* fill in mm[] with messy stuff */
    for (i=0; i<RANDSIZ; i+=8) {
      mix(a,b,c,d,e,f,g,h);
      m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
      m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
    }
  }

  this->randcnt = 0;
}

#undef ub4
#undef ub1

ullong
Random::Isaac::nextLong( void )
{
  ullong l;

  if ( this->randcnt < 8 ) {
    if ( this->randcnt == NOT_INITIALIZED )
      this->init();
    this->fill();
  }
  l = this->u.randrsll[ ( ( this->randcnt -= 8 ) >> 3 ) & LONGS_MASK ];
  if ( ! Aligned::isLittleEndian )
    Aligned::swap( l );
  return l;
}


unsigned int
Random::Isaac::nextInt( void )
{
  unsigned int i;

  if ( this->randcnt < 4 ) {
    if ( this->randcnt == NOT_INITIALIZED )
      this->init();
    this->fill();
  }
  i = this->u.randrsli[ ( ( this->randcnt -= 4 ) >> 2 ) & INTS_MASK ];
  if ( ! Aligned::isLittleEndian )
    Aligned::swap( i );
  return i;
}


unsigned short
Random::Isaac::nextShort( void )
{
  unsigned short s;

  if ( this->randcnt < 2 ) {
    if ( this->randcnt == NOT_INITIALIZED )
      this->init();
    this->fill();
  }
  s = this->u.randrsls[ ( ( this->randcnt -= 2 ) >> 1 ) & SHORTS_MASK ];
  if ( ! Aligned::isLittleEndian )
    Aligned::swap( s );
  return s;
}


byte
Random::Isaac::nextByte( void )
{
  if ( this->randcnt < 1 ) {
    if ( this->randcnt == NOT_INITIALIZED )
      this->init();
    this->fill();
  }
  return this->u.randrslb[ ( --this->randcnt ) & BYTES_MASK ];
}


void
Random::Isaac::nextBytes( byte *buf,  unsigned int bufLen )
{
  unsigned int n;

  while ( bufLen != 0 ) {
    if ( this->randcnt < 1 ) {
      if ( this->randcnt == NOT_INITIALIZED )
        this->init();
      this->fill();
    }
    if ( bufLen > (unsigned int) this->randcnt )
      n = (unsigned int) this->randcnt;
    else
      n = bufLen;
    bufLen -= n;
    do {
      *buf++ = this->u.randrslb[ ( --this->randcnt ) & BYTES_MASK ];
    } while ( --n != 0 );
  }
}

/*
 * Copyright (c) 2004-2006 Intel Corporation - All Rights Reserved
 *
 *
 * This software program is licensed subject to the BSD License, available at
 * http://www.opensource.org/licenses/bsd-license.html.
 *
 * Abstract:
 *
 * Tables for software CRC generation
 */

/*
 * The following CRC lookup table was generated automagically using the
 * following model parameters:
 *
 * Generator Polynomial = ................. 0x1EDC6F41
 * Generator Polynomial Length = .......... 32 bits
 * Reflected Bits = ....................... TRUE
 * Table Generation Offset = .............. 32 bits
 * Number of Slices = ..................... 8 slices
 * Slice Lengths = ........................ 8 8 8 8 8 8 8 8
 * Directory Name = ....................... .\
 * File Name = ............................ 8x256_tables.c
 */

static unsigned int sctp_crc_tableil8_o32[256] =
{
	0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4, 0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
	0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B, 0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
	0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B, 0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
	0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54, 0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
	0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A, 0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
	0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5, 0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
	0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45, 0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
	0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A, 0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
	0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x522BEE48, 0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
	0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687, 0x0C38D26C, 0xFE53516F, 0xED03A29B, 0x1F682198,
	0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927, 0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
	0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8, 0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
	0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096, 0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
	0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859, 0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
	0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9, 0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
	0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36, 0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
	0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C, 0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93,
	0x082F63B7, 0xFA44E0B4, 0xE9141340, 0x1B7F9043, 0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
	0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3, 0x55326B08, 0xA759E80B, 0xB4091BFF, 0x466298FC,
	0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C, 0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033,
	0xA24BB5A6, 0x502036A5, 0x4370C551, 0xB11B4652, 0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
	0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D, 0xEF087A76, 0x1D63F975, 0x0E330A81, 0xFC588982,
	0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D, 0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622,
	0x38CC2A06, 0xCAA7A905, 0xD9F75AF1, 0x2B9CD9F2, 0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
	0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530, 0x0417B1DB, 0xF67C32D8, 0xE52CC12C, 0x1747422F,
	0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF, 0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0,
	0xD3D3E1AB, 0x21B862A8, 0x32E8915C, 0xC083125F, 0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
	0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90, 0x9E902E7B, 0x6CFBAD78, 0x7FAB5E8C, 0x8DC0DD8F,
	0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE, 0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1,
	0x69E9F0D5, 0x9B8273D6, 0x88D28022, 0x7AB90321, 0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
	0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81, 0x34F4F86A, 0xC69F7B69, 0xD5CF889D, 0x27A40B9E,
	0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E, 0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351
};

/*
 * end of the CRC lookup table crc_tableil8_o32
 */



/*
 * The following CRC lookup table was generated automagically using the
 * following model parameters:
 *
 * Generator Polynomial = ................. 0x1EDC6F41
 * Generator Polynomial Length = .......... 32 bits
 * Reflected Bits = ....................... TRUE
 * Table Generation Offset = .............. 32 bits
 * Number of Slices = ..................... 8 slices
 * Slice Lengths = ........................ 8 8 8 8 8 8 8 8
 * Directory Name = ....................... .\
 * File Name = ............................ 8x256_tables.c
 */

static unsigned int sctp_crc_tableil8_o40[256] =
{
	0x00000000, 0x13A29877, 0x274530EE, 0x34E7A899, 0x4E8A61DC, 0x5D28F9AB, 0x69CF5132, 0x7A6DC945,
	0x9D14C3B8, 0x8EB65BCF, 0xBA51F356, 0xA9F36B21, 0xD39EA264, 0xC03C3A13, 0xF4DB928A, 0xE7790AFD,
	0x3FC5F181, 0x2C6769F6, 0x1880C16F, 0x0B225918, 0x714F905D, 0x62ED082A, 0x560AA0B3, 0x45A838C4,
	0xA2D13239, 0xB173AA4E, 0x859402D7, 0x96369AA0, 0xEC5B53E5, 0xFFF9CB92, 0xCB1E630B, 0xD8BCFB7C,
	0x7F8BE302, 0x6C297B75, 0x58CED3EC, 0x4B6C4B9B, 0x310182DE, 0x22A31AA9, 0x1644B230, 0x05E62A47,
	0xE29F20BA, 0xF13DB8CD, 0xC5DA1054, 0xD6788823, 0xAC154166, 0xBFB7D911, 0x8B507188, 0x98F2E9FF,
	0x404E1283, 0x53EC8AF4, 0x670B226D, 0x74A9BA1A, 0x0EC4735F, 0x1D66EB28, 0x298143B1, 0x3A23DBC6,
	0xDD5AD13B, 0xCEF8494C, 0xFA1FE1D5, 0xE9BD79A2, 0x93D0B0E7, 0x80722890, 0xB4958009, 0xA737187E,
	0xFF17C604, 0xECB55E73, 0xD852F6EA, 0xCBF06E9D, 0xB19DA7D8, 0xA23F3FAF, 0x96D89736, 0x857A0F41,
	0x620305BC, 0x71A19DCB, 0x45463552, 0x56E4AD25, 0x2C896460, 0x3F2BFC17, 0x0BCC548E, 0x186ECCF9,
	0xC0D23785, 0xD370AFF2, 0xE797076B, 0xF4359F1C, 0x8E585659, 0x9DFACE2E, 0xA91D66B7, 0xBABFFEC0,
	0x5DC6F43D, 0x4E646C4A, 0x7A83C4D3, 0x69215CA4, 0x134C95E1, 0x00EE0D96, 0x3409A50F, 0x27AB3D78,
	0x809C2506, 0x933EBD71, 0xA7D915E8, 0xB47B8D9F, 0xCE1644DA, 0xDDB4DCAD, 0xE9537434, 0xFAF1EC43,
	0x1D88E6BE, 0x0E2A7EC9, 0x3ACDD650, 0x296F4E27, 0x53028762, 0x40A01F15, 0x7447B78C, 0x67E52FFB,
	0xBF59D487, 0xACFB4CF0, 0x981CE469, 0x8BBE7C1E, 0xF1D3B55B, 0xE2712D2C, 0xD69685B5, 0xC5341DC2,
	0x224D173F, 0x31EF8F48, 0x050827D1, 0x16AABFA6, 0x6CC776E3, 0x7F65EE94, 0x4B82460D, 0x5820DE7A,
	0xFBC3FAF9, 0xE861628E, 0xDC86CA17, 0xCF245260, 0xB5499B25, 0xA6EB0352, 0x920CABCB, 0x81AE33BC,
	0x66D73941, 0x7575A136, 0x419209AF, 0x523091D8, 0x285D589D, 0x3BFFC0EA, 0x0F186873, 0x1CBAF004,
	0xC4060B78, 0xD7A4930F, 0xE3433B96, 0xF0E1A3E1, 0x8A8C6AA4, 0x992EF2D3, 0xADC95A4A, 0xBE6BC23D,
	0x5912C8C0, 0x4AB050B7, 0x7E57F82E, 0x6DF56059, 0x1798A91C, 0x043A316B, 0x30DD99F2, 0x237F0185,
	0x844819FB, 0x97EA818C, 0xA30D2915, 0xB0AFB162, 0xCAC27827, 0xD960E050, 0xED8748C9, 0xFE25D0BE,
	0x195CDA43, 0x0AFE4234, 0x3E19EAAD, 0x2DBB72DA, 0x57D6BB9F, 0x447423E8, 0x70938B71, 0x63311306,
	0xBB8DE87A, 0xA82F700D, 0x9CC8D894, 0x8F6A40E3, 0xF50789A6, 0xE6A511D1, 0xD242B948, 0xC1E0213F,
	0x26992BC2, 0x353BB3B5, 0x01DC1B2C, 0x127E835B, 0x68134A1E, 0x7BB1D269, 0x4F567AF0, 0x5CF4E287,
	0x04D43CFD, 0x1776A48A, 0x23910C13, 0x30339464, 0x4A5E5D21, 0x59FCC556, 0x6D1B6DCF, 0x7EB9F5B8,
	0x99C0FF45, 0x8A626732, 0xBE85CFAB, 0xAD2757DC, 0xD74A9E99, 0xC4E806EE, 0xF00FAE77, 0xE3AD3600,
	0x3B11CD7C, 0x28B3550B, 0x1C54FD92, 0x0FF665E5, 0x759BACA0, 0x663934D7, 0x52DE9C4E, 0x417C0439,
	0xA6050EC4, 0xB5A796B3, 0x81403E2A, 0x92E2A65D, 0xE88F6F18, 0xFB2DF76F, 0xCFCA5FF6, 0xDC68C781,
	0x7B5FDFFF, 0x68FD4788, 0x5C1AEF11, 0x4FB87766, 0x35D5BE23, 0x26772654, 0x12908ECD, 0x013216BA,
	0xE64B1C47, 0xF5E98430, 0xC10E2CA9, 0xD2ACB4DE, 0xA8C17D9B, 0xBB63E5EC, 0x8F844D75, 0x9C26D502,
	0x449A2E7E, 0x5738B609, 0x63DF1E90, 0x707D86E7, 0x0A104FA2, 0x19B2D7D5, 0x2D557F4C, 0x3EF7E73B,
	0xD98EEDC6, 0xCA2C75B1, 0xFECBDD28, 0xED69455F, 0x97048C1A, 0x84A6146D, 0xB041BCF4, 0xA3E32483
};

/*
 * end of the CRC lookup table crc_tableil8_o40
 */



/*
 * The following CRC lookup table was generated automagically using the
 * following model parameters:
 *
 * Generator Polynomial = ................. 0x1EDC6F41
 * Generator Polynomial Length = .......... 32 bits
 * Reflected Bits = ....................... TRUE
 * Table Generation Offset = .............. 32 bits
 * Number of Slices = ..................... 8 slices
 * Slice Lengths = ........................ 8 8 8 8 8 8 8 8
 * Directory Name = ....................... .\
 * File Name = ............................ 8x256_tables.c
 */

static unsigned int sctp_crc_tableil8_o48[256] =
{
	0x00000000, 0xA541927E, 0x4F6F520D, 0xEA2EC073, 0x9EDEA41A, 0x3B9F3664, 0xD1B1F617, 0x74F06469,
	0x38513EC5, 0x9D10ACBB, 0x773E6CC8, 0xD27FFEB6, 0xA68F9ADF, 0x03CE08A1, 0xE9E0C8D2, 0x4CA15AAC,
	0x70A27D8A, 0xD5E3EFF4, 0x3FCD2F87, 0x9A8CBDF9, 0xEE7CD990, 0x4B3D4BEE, 0xA1138B9D, 0x045219E3,
	0x48F3434F, 0xEDB2D131, 0x079C1142, 0xA2DD833C, 0xD62DE755, 0x736C752B, 0x9942B558, 0x3C032726,
	0xE144FB14, 0x4405696A, 0xAE2BA919, 0x0B6A3B67, 0x7F9A5F0E, 0xDADBCD70, 0x30F50D03, 0x95B49F7D,
	0xD915C5D1, 0x7C5457AF, 0x967A97DC, 0x333B05A2, 0x47CB61CB, 0xE28AF3B5, 0x08A433C6, 0xADE5A1B8,
	0x91E6869E, 0x34A714E0, 0xDE89D493, 0x7BC846ED, 0x0F382284, 0xAA79B0FA, 0x40577089, 0xE516E2F7,
	0xA9B7B85B, 0x0CF62A25, 0xE6D8EA56, 0x43997828, 0x37691C41, 0x92288E3F, 0x78064E4C, 0xDD47DC32,
	0xC76580D9, 0x622412A7, 0x880AD2D4, 0x2D4B40AA, 0x59BB24C3, 0xFCFAB6BD, 0x16D476CE, 0xB395E4B0,
	0xFF34BE1C, 0x5A752C62, 0xB05BEC11, 0x151A7E6F, 0x61EA1A06, 0xC4AB8878, 0x2E85480B, 0x8BC4DA75,
	0xB7C7FD53, 0x12866F2D, 0xF8A8AF5E, 0x5DE93D20, 0x29195949, 0x8C58CB37, 0x66760B44, 0xC337993A,
	0x8F96C396, 0x2AD751E8, 0xC0F9919B, 0x65B803E5, 0x1148678C, 0xB409F5F2, 0x5E273581, 0xFB66A7FF,
	0x26217BCD, 0x8360E9B3, 0x694E29C0, 0xCC0FBBBE, 0xB8FFDFD7, 0x1DBE4DA9, 0xF7908DDA, 0x52D11FA4,
	0x1E704508, 0xBB31D776, 0x511F1705, 0xF45E857B, 0x80AEE112, 0x25EF736C, 0xCFC1B31F, 0x6A802161,
	0x56830647, 0xF3C29439, 0x19EC544A, 0xBCADC634, 0xC85DA25D, 0x6D1C3023, 0x8732F050, 0x2273622E,
	0x6ED23882, 0xCB93AAFC, 0x21BD6A8F, 0x84FCF8F1, 0xF00C9C98, 0x554D0EE6, 0xBF63CE95, 0x1A225CEB,
	0x8B277743, 0x2E66E53D, 0xC448254E, 0x6109B730, 0x15F9D359, 0xB0B84127, 0x5A968154, 0xFFD7132A,
	0xB3764986, 0x1637DBF8, 0xFC191B8B, 0x595889F5, 0x2DA8ED9C, 0x88E97FE2, 0x62C7BF91, 0xC7862DEF,
	0xFB850AC9, 0x5EC498B7, 0xB4EA58C4, 0x11ABCABA, 0x655BAED3, 0xC01A3CAD, 0x2A34FCDE, 0x8F756EA0,
	0xC3D4340C, 0x6695A672, 0x8CBB6601, 0x29FAF47F, 0x5D0A9016, 0xF84B0268, 0x1265C21B, 0xB7245065,
	0x6A638C57, 0xCF221E29, 0x250CDE5A, 0x804D4C24, 0xF4BD284D, 0x51FCBA33, 0xBBD27A40, 0x1E93E83E,
	0x5232B292, 0xF77320EC, 0x1D5DE09F, 0xB81C72E1, 0xCCEC1688, 0x69AD84F6, 0x83834485, 0x26C2D6FB,
	0x1AC1F1DD, 0xBF8063A3, 0x55AEA3D0, 0xF0EF31AE, 0x841F55C7, 0x215EC7B9, 0xCB7007CA, 0x6E3195B4,
	0x2290CF18, 0x87D15D66, 0x6DFF9D15, 0xC8BE0F6B, 0xBC4E6B02, 0x190FF97C, 0xF321390F, 0x5660AB71,
	0x4C42F79A, 0xE90365E4, 0x032DA597, 0xA66C37E9, 0xD29C5380, 0x77DDC1FE, 0x9DF3018D, 0x38B293F3,
	0x7413C95F, 0xD1525B21, 0x3B7C9B52, 0x9E3D092C, 0xEACD6D45, 0x4F8CFF3B, 0xA5A23F48, 0x00E3AD36,
	0x3CE08A10, 0x99A1186E, 0x738FD81D, 0xD6CE4A63, 0xA23E2E0A, 0x077FBC74, 0xED517C07, 0x4810EE79,
	0x04B1B4D5, 0xA1F026AB, 0x4BDEE6D8, 0xEE9F74A6, 0x9A6F10CF, 0x3F2E82B1, 0xD50042C2, 0x7041D0BC,
	0xAD060C8E, 0x08479EF0, 0xE2695E83, 0x4728CCFD, 0x33D8A894, 0x96993AEA, 0x7CB7FA99, 0xD9F668E7,
	0x9557324B, 0x3016A035, 0xDA386046, 0x7F79F238, 0x0B899651, 0xAEC8042F, 0x44E6C45C, 0xE1A75622,
	0xDDA47104, 0x78E5E37A, 0x92CB2309, 0x378AB177, 0x437AD51E, 0xE63B4760, 0x0C158713, 0xA954156D,
	0xE5F54FC1, 0x40B4DDBF, 0xAA9A1DCC, 0x0FDB8FB2, 0x7B2BEBDB, 0xDE6A79A5, 0x3444B9D6, 0x91052BA8
};

/*
 * end of the CRC lookup table crc_tableil8_o48
 */



/*
 * The following CRC lookup table was generated automagically using the
 * following model parameters:
 *
 * Generator Polynomial = ................. 0x1EDC6F41
 * Generator Polynomial Length = .......... 32 bits
 * Reflected Bits = ....................... TRUE
 * Table Generation Offset = .............. 32 bits
 * Number of Slices = ..................... 8 slices
 * Slice Lengths = ........................ 8 8 8 8 8 8 8 8
 * Directory Name = ....................... .\
 * File Name = ............................ 8x256_tables.c
 */

static unsigned int sctp_crc_tableil8_o56[256] =
{
	0x00000000, 0xDD45AAB8, 0xBF672381, 0x62228939, 0x7B2231F3, 0xA6679B4B, 0xC4451272, 0x1900B8CA,
	0xF64463E6, 0x2B01C95E, 0x49234067, 0x9466EADF, 0x8D665215, 0x5023F8AD, 0x32017194, 0xEF44DB2C,
	0xE964B13D, 0x34211B85, 0x560392BC, 0x8B463804, 0x924680CE, 0x4F032A76, 0x2D21A34F, 0xF06409F7,
	0x1F20D2DB, 0xC2657863, 0xA047F15A, 0x7D025BE2, 0x6402E328, 0xB9474990, 0xDB65C0A9, 0x06206A11,
	0xD725148B, 0x0A60BE33, 0x6842370A, 0xB5079DB2, 0xAC072578, 0x71428FC0, 0x136006F9, 0xCE25AC41,
	0x2161776D, 0xFC24DDD5, 0x9E0654EC, 0x4343FE54, 0x5A43469E, 0x8706EC26, 0xE524651F, 0x3861CFA7,
	0x3E41A5B6, 0xE3040F0E, 0x81268637, 0x5C632C8F, 0x45639445, 0x98263EFD, 0xFA04B7C4, 0x27411D7C,
	0xC805C650, 0x15406CE8, 0x7762E5D1, 0xAA274F69, 0xB327F7A3, 0x6E625D1B, 0x0C40D422, 0xD1057E9A,
	0xABA65FE7, 0x76E3F55F, 0x14C17C66, 0xC984D6DE, 0xD0846E14, 0x0DC1C4AC, 0x6FE34D95, 0xB2A6E72D,
	0x5DE23C01, 0x80A796B9, 0xE2851F80, 0x3FC0B538, 0x26C00DF2, 0xFB85A74A, 0x99A72E73, 0x44E284CB,
	0x42C2EEDA, 0x9F874462, 0xFDA5CD5B, 0x20E067E3, 0x39E0DF29, 0xE4A57591, 0x8687FCA8, 0x5BC25610,
	0xB4868D3C, 0x69C32784, 0x0BE1AEBD, 0xD6A40405, 0xCFA4BCCF, 0x12E11677, 0x70C39F4E, 0xAD8635F6,
	0x7C834B6C, 0xA1C6E1D4, 0xC3E468ED, 0x1EA1C255, 0x07A17A9F, 0xDAE4D027, 0xB8C6591E, 0x6583F3A6,
	0x8AC7288A, 0x57828232, 0x35A00B0B, 0xE8E5A1B3, 0xF1E51979, 0x2CA0B3C1, 0x4E823AF8, 0x93C79040,
	0x95E7FA51, 0x48A250E9, 0x2A80D9D0, 0xF7C57368, 0xEEC5CBA2, 0x3380611A, 0x51A2E823, 0x8CE7429B,
	0x63A399B7, 0xBEE6330F, 0xDCC4BA36, 0x0181108E, 0x1881A844, 0xC5C402FC, 0xA7E68BC5, 0x7AA3217D,
	0x52A0C93F, 0x8FE56387, 0xEDC7EABE, 0x30824006, 0x2982F8CC, 0xF4C75274, 0x96E5DB4D, 0x4BA071F5,
	0xA4E4AAD9, 0x79A10061, 0x1B838958, 0xC6C623E0, 0xDFC69B2A, 0x02833192, 0x60A1B8AB, 0xBDE41213,
	0xBBC47802, 0x6681D2BA, 0x04A35B83, 0xD9E6F13B, 0xC0E649F1, 0x1DA3E349, 0x7F816A70, 0xA2C4C0C8,
	0x4D801BE4, 0x90C5B15C, 0xF2E73865, 0x2FA292DD, 0x36A22A17, 0xEBE780AF, 0x89C50996, 0x5480A32E,
	0x8585DDB4, 0x58C0770C, 0x3AE2FE35, 0xE7A7548D, 0xFEA7EC47, 0x23E246FF, 0x41C0CFC6, 0x9C85657E,
	0x73C1BE52, 0xAE8414EA, 0xCCA69DD3, 0x11E3376B, 0x08E38FA1, 0xD5A62519, 0xB784AC20, 0x6AC10698,
	0x6CE16C89, 0xB1A4C631, 0xD3864F08, 0x0EC3E5B0, 0x17C35D7A, 0xCA86F7C2, 0xA8A47EFB, 0x75E1D443,
	0x9AA50F6F, 0x47E0A5D7, 0x25C22CEE, 0xF8878656, 0xE1873E9C, 0x3CC29424, 0x5EE01D1D, 0x83A5B7A5,
	0xF90696D8, 0x24433C60, 0x4661B559, 0x9B241FE1, 0x8224A72B, 0x5F610D93, 0x3D4384AA, 0xE0062E12,
	0x0F42F53E, 0xD2075F86, 0xB025D6BF, 0x6D607C07, 0x7460C4CD, 0xA9256E75, 0xCB07E74C, 0x16424DF4,
	0x106227E5, 0xCD278D5D, 0xAF050464, 0x7240AEDC, 0x6B401616, 0xB605BCAE, 0xD4273597, 0x09629F2F,
	0xE6264403, 0x3B63EEBB, 0x59416782, 0x8404CD3A, 0x9D0475F0, 0x4041DF48, 0x22635671, 0xFF26FCC9,
	0x2E238253, 0xF36628EB, 0x9144A1D2, 0x4C010B6A, 0x5501B3A0, 0x88441918, 0xEA669021, 0x37233A99,
	0xD867E1B5, 0x05224B0D, 0x6700C234, 0xBA45688C, 0xA345D046, 0x7E007AFE, 0x1C22F3C7, 0xC167597F,
	0xC747336E, 0x1A0299D6, 0x782010EF, 0xA565BA57, 0xBC65029D, 0x6120A825, 0x0302211C, 0xDE478BA4,
	0x31035088, 0xEC46FA30, 0x8E647309, 0x5321D9B1, 0x4A21617B, 0x9764CBC3, 0xF54642FA, 0x2803E842
};

/*
 * end of the CRC lookup table crc_tableil8_o56
 */



/*
 * The following CRC lookup table was generated automagically using the
 * following model parameters:
 *
 * Generator Polynomial = ................. 0x1EDC6F41
 * Generator Polynomial Length = .......... 32 bits
 * Reflected Bits = ....................... TRUE
 * Table Generation Offset = .............. 32 bits
 * Number of Slices = ..................... 8 slices
 * Slice Lengths = ........................ 8 8 8 8 8 8 8 8
 * Directory Name = ....................... .\
 * File Name = ............................ 8x256_tables.c
 */

static unsigned int sctp_crc_tableil8_o64[256] =
{
	0x00000000, 0x38116FAC, 0x7022DF58, 0x4833B0F4, 0xE045BEB0, 0xD854D11C, 0x906761E8, 0xA8760E44,
	0xC5670B91, 0xFD76643D, 0xB545D4C9, 0x8D54BB65, 0x2522B521, 0x1D33DA8D, 0x55006A79, 0x6D1105D5,
	0x8F2261D3, 0xB7330E7F, 0xFF00BE8B, 0xC711D127, 0x6F67DF63, 0x5776B0CF, 0x1F45003B, 0x27546F97,
	0x4A456A42, 0x725405EE, 0x3A67B51A, 0x0276DAB6, 0xAA00D4F2, 0x9211BB5E, 0xDA220BAA, 0xE2336406,
	0x1BA8B557, 0x23B9DAFB, 0x6B8A6A0F, 0x539B05A3, 0xFBED0BE7, 0xC3FC644B, 0x8BCFD4BF, 0xB3DEBB13,
	0xDECFBEC6, 0xE6DED16A, 0xAEED619E, 0x96FC0E32, 0x3E8A0076, 0x069B6FDA, 0x4EA8DF2E, 0x76B9B082,
	0x948AD484, 0xAC9BBB28, 0xE4A80BDC, 0xDCB96470, 0x74CF6A34, 0x4CDE0598, 0x04EDB56C, 0x3CFCDAC0,
	0x51EDDF15, 0x69FCB0B9, 0x21CF004D, 0x19DE6FE1, 0xB1A861A5, 0x89B90E09, 0xC18ABEFD, 0xF99BD151,
	0x37516AAE, 0x0F400502, 0x4773B5F6, 0x7F62DA5A, 0xD714D41E, 0xEF05BBB2, 0xA7360B46, 0x9F2764EA,
	0xF236613F, 0xCA270E93, 0x8214BE67, 0xBA05D1CB, 0x1273DF8F, 0x2A62B023, 0x625100D7, 0x5A406F7B,
	0xB8730B7D, 0x806264D1, 0xC851D425, 0xF040BB89, 0x5836B5CD, 0x6027DA61, 0x28146A95, 0x10050539,
	0x7D1400EC, 0x45056F40, 0x0D36DFB4, 0x3527B018, 0x9D51BE5C, 0xA540D1F0, 0xED736104, 0xD5620EA8,
	0x2CF9DFF9, 0x14E8B055, 0x5CDB00A1, 0x64CA6F0D, 0xCCBC6149, 0xF4AD0EE5, 0xBC9EBE11, 0x848FD1BD,
	0xE99ED468, 0xD18FBBC4, 0x99BC0B30, 0xA1AD649C, 0x09DB6AD8, 0x31CA0574, 0x79F9B580, 0x41E8DA2C,
	0xA3DBBE2A, 0x9BCAD186, 0xD3F96172, 0xEBE80EDE, 0x439E009A, 0x7B8F6F36, 0x33BCDFC2, 0x0BADB06E,
	0x66BCB5BB, 0x5EADDA17, 0x169E6AE3, 0x2E8F054F, 0x86F90B0B, 0xBEE864A7, 0xF6DBD453, 0xCECABBFF,
	0x6EA2D55C, 0x56B3BAF0, 0x1E800A04, 0x269165A8, 0x8EE76BEC, 0xB6F60440, 0xFEC5B4B4, 0xC6D4DB18,
	0xABC5DECD, 0x93D4B161, 0xDBE70195, 0xE3F66E39, 0x4B80607D, 0x73910FD1, 0x3BA2BF25, 0x03B3D089,
	0xE180B48F, 0xD991DB23, 0x91A26BD7, 0xA9B3047B, 0x01C50A3F, 0x39D46593, 0x71E7D567, 0x49F6BACB,
	0x24E7BF1E, 0x1CF6D0B2, 0x54C56046, 0x6CD40FEA, 0xC4A201AE, 0xFCB36E02, 0xB480DEF6, 0x8C91B15A,
	0x750A600B, 0x4D1B0FA7, 0x0528BF53, 0x3D39D0FF, 0x954FDEBB, 0xAD5EB117, 0xE56D01E3, 0xDD7C6E4F,
	0xB06D6B9A, 0x887C0436, 0xC04FB4C2, 0xF85EDB6E, 0x5028D52A, 0x6839BA86, 0x200A0A72, 0x181B65DE,
	0xFA2801D8, 0xC2396E74, 0x8A0ADE80, 0xB21BB12C, 0x1A6DBF68, 0x227CD0C4, 0x6A4F6030, 0x525E0F9C,
	0x3F4F0A49, 0x075E65E5, 0x4F6DD511, 0x777CBABD, 0xDF0AB4F9, 0xE71BDB55, 0xAF286BA1, 0x9739040D,
	0x59F3BFF2, 0x61E2D05E, 0x29D160AA, 0x11C00F06, 0xB9B60142, 0x81A76EEE, 0xC994DE1A, 0xF185B1B6,
	0x9C94B463, 0xA485DBCF, 0xECB66B3B, 0xD4A70497, 0x7CD10AD3, 0x44C0657F, 0x0CF3D58B, 0x34E2BA27,
	0xD6D1DE21, 0xEEC0B18D, 0xA6F30179, 0x9EE26ED5, 0x36946091, 0x0E850F3D, 0x46B6BFC9, 0x7EA7D065,
	0x13B6D5B0, 0x2BA7BA1C, 0x63940AE8, 0x5B856544, 0xF3F36B00, 0xCBE204AC, 0x83D1B458, 0xBBC0DBF4,
	0x425B0AA5, 0x7A4A6509, 0x3279D5FD, 0x0A68BA51, 0xA21EB415, 0x9A0FDBB9, 0xD23C6B4D, 0xEA2D04E1,
	0x873C0134, 0xBF2D6E98, 0xF71EDE6C, 0xCF0FB1C0, 0x6779BF84, 0x5F68D028, 0x175B60DC, 0x2F4A0F70,
	0xCD796B76, 0xF56804DA, 0xBD5BB42E, 0x854ADB82, 0x2D3CD5C6, 0x152DBA6A, 0x5D1E0A9E, 0x650F6532,
	0x081E60E7, 0x300F0F4B, 0x783CBFBF, 0x402DD013, 0xE85BDE57, 0xD04AB1FB, 0x9879010F, 0xA0686EA3
};

/*
 * end of the CRC lookup table crc_tableil8_o64
 */



/*
 * The following CRC lookup table was generated automagically using the
 * following model parameters:
 *
 * Generator Polynomial = ................. 0x1EDC6F41
 * Generator Polynomial Length = .......... 32 bits
 * Reflected Bits = ....................... TRUE
 * Table Generation Offset = .............. 32 bits
 * Number of Slices = ..................... 8 slices
 * Slice Lengths = ........................ 8 8 8 8 8 8 8 8
 * Directory Name = ....................... .\
 * File Name = ............................ 8x256_tables.c
 */

static unsigned int sctp_crc_tableil8_o72[256] =
{
	0x00000000, 0xEF306B19, 0xDB8CA0C3, 0x34BCCBDA, 0xB2F53777, 0x5DC55C6E, 0x697997B4, 0x8649FCAD,
	0x6006181F, 0x8F367306, 0xBB8AB8DC, 0x54BAD3C5, 0xD2F32F68, 0x3DC34471, 0x097F8FAB, 0xE64FE4B2,
	0xC00C303E, 0x2F3C5B27, 0x1B8090FD, 0xF4B0FBE4, 0x72F90749, 0x9DC96C50, 0xA975A78A, 0x4645CC93,
	0xA00A2821, 0x4F3A4338, 0x7B8688E2, 0x94B6E3FB, 0x12FF1F56, 0xFDCF744F, 0xC973BF95, 0x2643D48C,
	0x85F4168D, 0x6AC47D94, 0x5E78B64E, 0xB148DD57, 0x370121FA, 0xD8314AE3, 0xEC8D8139, 0x03BDEA20,
	0xE5F20E92, 0x0AC2658B, 0x3E7EAE51, 0xD14EC548, 0x570739E5, 0xB83752FC, 0x8C8B9926, 0x63BBF23F,
	0x45F826B3, 0xAAC84DAA, 0x9E748670, 0x7144ED69, 0xF70D11C4, 0x183D7ADD, 0x2C81B107, 0xC3B1DA1E,
	0x25FE3EAC, 0xCACE55B5, 0xFE729E6F, 0x1142F576, 0x970B09DB, 0x783B62C2, 0x4C87A918, 0xA3B7C201,
	0x0E045BEB, 0xE13430F2, 0xD588FB28, 0x3AB89031, 0xBCF16C9C, 0x53C10785, 0x677DCC5F, 0x884DA746,
	0x6E0243F4, 0x813228ED, 0xB58EE337, 0x5ABE882E, 0xDCF77483, 0x33C71F9A, 0x077BD440, 0xE84BBF59,
	0xCE086BD5, 0x213800CC, 0x1584CB16, 0xFAB4A00F, 0x7CFD5CA2, 0x93CD37BB, 0xA771FC61, 0x48419778,
	0xAE0E73CA, 0x413E18D3, 0x7582D309, 0x9AB2B810, 0x1CFB44BD, 0xF3CB2FA4, 0xC777E47E, 0x28478F67,
	0x8BF04D66, 0x64C0267F, 0x507CEDA5, 0xBF4C86BC, 0x39057A11, 0xD6351108, 0xE289DAD2, 0x0DB9B1CB,
	0xEBF65579, 0x04C63E60, 0x307AF5BA, 0xDF4A9EA3, 0x5903620E, 0xB6330917, 0x828FC2CD, 0x6DBFA9D4,
	0x4BFC7D58, 0xA4CC1641, 0x9070DD9B, 0x7F40B682, 0xF9094A2F, 0x16392136, 0x2285EAEC, 0xCDB581F5,
	0x2BFA6547, 0xC4CA0E5E, 0xF076C584, 0x1F46AE9D, 0x990F5230, 0x763F3929, 0x4283F2F3, 0xADB399EA,
	0x1C08B7D6, 0xF338DCCF, 0xC7841715, 0x28B47C0C, 0xAEFD80A1, 0x41CDEBB8, 0x75712062, 0x9A414B7B,
	0x7C0EAFC9, 0x933EC4D0, 0xA7820F0A, 0x48B26413, 0xCEFB98BE, 0x21CBF3A7, 0x1577387D, 0xFA475364,
	0xDC0487E8, 0x3334ECF1, 0x0788272B, 0xE8B84C32, 0x6EF1B09F, 0x81C1DB86, 0xB57D105C, 0x5A4D7B45,
	0xBC029FF7, 0x5332F4EE, 0x678E3F34, 0x88BE542D, 0x0EF7A880, 0xE1C7C399, 0xD57B0843, 0x3A4B635A,
	0x99FCA15B, 0x76CCCA42, 0x42700198, 0xAD406A81, 0x2B09962C, 0xC439FD35, 0xF08536EF, 0x1FB55DF6,
	0xF9FAB944, 0x16CAD25D, 0x22761987, 0xCD46729E, 0x4B0F8E33, 0xA43FE52A, 0x90832EF0, 0x7FB345E9,
	0x59F09165, 0xB6C0FA7C, 0x827C31A6, 0x6D4C5ABF, 0xEB05A612, 0x0435CD0B, 0x308906D1, 0xDFB96DC8,
	0x39F6897A, 0xD6C6E263, 0xE27A29B9, 0x0D4A42A0, 0x8B03BE0D, 0x6433D514, 0x508F1ECE, 0xBFBF75D7,
	0x120CEC3D, 0xFD3C8724, 0xC9804CFE, 0x26B027E7, 0xA0F9DB4A, 0x4FC9B053, 0x7B757B89, 0x94451090,
	0x720AF422, 0x9D3A9F3B, 0xA98654E1, 0x46B63FF8, 0xC0FFC355, 0x2FCFA84C, 0x1B736396, 0xF443088F,
	0xD200DC03, 0x3D30B71A, 0x098C7CC0, 0xE6BC17D9, 0x60F5EB74, 0x8FC5806D, 0xBB794BB7, 0x544920AE,
	0xB206C41C, 0x5D36AF05, 0x698A64DF, 0x86BA0FC6, 0x00F3F36B, 0xEFC39872, 0xDB7F53A8, 0x344F38B1,
	0x97F8FAB0, 0x78C891A9, 0x4C745A73, 0xA344316A, 0x250DCDC7, 0xCA3DA6DE, 0xFE816D04, 0x11B1061D,
	0xF7FEE2AF, 0x18CE89B6, 0x2C72426C, 0xC3422975, 0x450BD5D8, 0xAA3BBEC1, 0x9E87751B, 0x71B71E02,
	0x57F4CA8E, 0xB8C4A197, 0x8C786A4D, 0x63480154, 0xE501FDF9, 0x0A3196E0, 0x3E8D5D3A, 0xD1BD3623,
	0x37F2D291, 0xD8C2B988, 0xEC7E7252, 0x034E194B, 0x8507E5E6, 0x6A378EFF, 0x5E8B4525, 0xB1BB2E3C
};

/*
 * end of the CRC lookup table crc_tableil8_o72
 */



/*
 * The following CRC lookup table was generated automagically using the
 * following model parameters:
 *
 * Generator Polynomial = ................. 0x1EDC6F41
 * Generator Polynomial Length = .......... 32 bits
 * Reflected Bits = ....................... TRUE
 * Table Generation Offset = .............. 32 bits
 * Number of Slices = ..................... 8 slices
 * Slice Lengths = ........................ 8 8 8 8 8 8 8 8
 * Directory Name = ....................... .\
 * File Name = ............................ 8x256_tables.c
 */

static unsigned int sctp_crc_tableil8_o80[256] =
{
	0x00000000, 0x68032CC8, 0xD0065990, 0xB8057558, 0xA5E0C5D1, 0xCDE3E919, 0x75E69C41, 0x1DE5B089,
	0x4E2DFD53, 0x262ED19B, 0x9E2BA4C3, 0xF628880B, 0xEBCD3882, 0x83CE144A, 0x3BCB6112, 0x53C84DDA,
	0x9C5BFAA6, 0xF458D66E, 0x4C5DA336, 0x245E8FFE, 0x39BB3F77, 0x51B813BF, 0xE9BD66E7, 0x81BE4A2F,
	0xD27607F5, 0xBA752B3D, 0x02705E65, 0x6A7372AD, 0x7796C224, 0x1F95EEEC, 0xA7909BB4, 0xCF93B77C,
	0x3D5B83BD, 0x5558AF75, 0xED5DDA2D, 0x855EF6E5, 0x98BB466C, 0xF0B86AA4, 0x48BD1FFC, 0x20BE3334,
	0x73767EEE, 0x1B755226, 0xA370277E, 0xCB730BB6, 0xD696BB3F, 0xBE9597F7, 0x0690E2AF, 0x6E93CE67,
	0xA100791B, 0xC90355D3, 0x7106208B, 0x19050C43, 0x04E0BCCA, 0x6CE39002, 0xD4E6E55A, 0xBCE5C992,
	0xEF2D8448, 0x872EA880, 0x3F2BDDD8, 0x5728F110, 0x4ACD4199, 0x22CE6D51, 0x9ACB1809, 0xF2C834C1,
	0x7AB7077A, 0x12B42BB2, 0xAAB15EEA, 0xC2B27222, 0xDF57C2AB, 0xB754EE63, 0x0F519B3B, 0x6752B7F3,
	0x349AFA29, 0x5C99D6E1, 0xE49CA3B9, 0x8C9F8F71, 0x917A3FF8, 0xF9791330, 0x417C6668, 0x297F4AA0,
	0xE6ECFDDC, 0x8EEFD114, 0x36EAA44C, 0x5EE98884, 0x430C380D, 0x2B0F14C5, 0x930A619D, 0xFB094D55,
	0xA8C1008F, 0xC0C22C47, 0x78C7591F, 0x10C475D7, 0x0D21C55E, 0x6522E996, 0xDD279CCE, 0xB524B006,
	0x47EC84C7, 0x2FEFA80F, 0x97EADD57, 0xFFE9F19F, 0xE20C4116, 0x8A0F6DDE, 0x320A1886, 0x5A09344E,
	0x09C17994, 0x61C2555C, 0xD9C72004, 0xB1C40CCC, 0xAC21BC45, 0xC422908D, 0x7C27E5D5, 0x1424C91D,
	0xDBB77E61, 0xB3B452A9, 0x0BB127F1, 0x63B20B39, 0x7E57BBB0, 0x16549778, 0xAE51E220, 0xC652CEE8,
	0x959A8332, 0xFD99AFFA, 0x459CDAA2, 0x2D9FF66A, 0x307A46E3, 0x58796A2B, 0xE07C1F73, 0x887F33BB,
	0xF56E0EF4, 0x9D6D223C, 0x25685764, 0x4D6B7BAC, 0x508ECB25, 0x388DE7ED, 0x808892B5, 0xE88BBE7D,
	0xBB43F3A7, 0xD340DF6F, 0x6B45AA37, 0x034686FF, 0x1EA33676, 0x76A01ABE, 0xCEA56FE6, 0xA6A6432E,
	0x6935F452, 0x0136D89A, 0xB933ADC2, 0xD130810A, 0xCCD53183, 0xA4D61D4B, 0x1CD36813, 0x74D044DB,
	0x27180901, 0x4F1B25C9, 0xF71E5091, 0x9F1D7C59, 0x82F8CCD0, 0xEAFBE018, 0x52FE9540, 0x3AFDB988,
	0xC8358D49, 0xA036A181, 0x1833D4D9, 0x7030F811, 0x6DD54898, 0x05D66450, 0xBDD31108, 0xD5D03DC0,
	0x8618701A, 0xEE1B5CD2, 0x561E298A, 0x3E1D0542, 0x23F8B5CB, 0x4BFB9903, 0xF3FEEC5B, 0x9BFDC093,
	0x546E77EF, 0x3C6D5B27, 0x84682E7F, 0xEC6B02B7, 0xF18EB23E, 0x998D9EF6, 0x2188EBAE, 0x498BC766,
	0x1A438ABC, 0x7240A674, 0xCA45D32C, 0xA246FFE4, 0xBFA34F6D, 0xD7A063A5, 0x6FA516FD, 0x07A63A35,
	0x8FD9098E, 0xE7DA2546, 0x5FDF501E, 0x37DC7CD6, 0x2A39CC5F, 0x423AE097, 0xFA3F95CF, 0x923CB907,
	0xC1F4F4DD, 0xA9F7D815, 0x11F2AD4D, 0x79F18185, 0x6414310C, 0x0C171DC4, 0xB412689C, 0xDC114454,
	0x1382F328, 0x7B81DFE0, 0xC384AAB8, 0xAB878670, 0xB66236F9, 0xDE611A31, 0x66646F69, 0x0E6743A1,
	0x5DAF0E7B, 0x35AC22B3, 0x8DA957EB, 0xE5AA7B23, 0xF84FCBAA, 0x904CE762, 0x2849923A, 0x404ABEF2,
	0xB2828A33, 0xDA81A6FB, 0x6284D3A3, 0x0A87FF6B, 0x17624FE2, 0x7F61632A, 0xC7641672, 0xAF673ABA,
	0xFCAF7760, 0x94AC5BA8, 0x2CA92EF0, 0x44AA0238, 0x594FB2B1, 0x314C9E79, 0x8949EB21, 0xE14AC7E9,
	0x2ED97095, 0x46DA5C5D, 0xFEDF2905, 0x96DC05CD, 0x8B39B544, 0xE33A998C, 0x5B3FECD4, 0x333CC01C,
	0x60F48DC6, 0x08F7A10E, 0xB0F2D456, 0xD8F1F89E, 0xC5144817, 0xAD1764DF, 0x15121187, 0x7D113D4F
};

/*
 * end of the CRC lookup table crc_tableil8_o80
 */



/*
 * The following CRC lookup table was generated automagically using the
 * following model parameters:
 *
 * Generator Polynomial = ................. 0x1EDC6F41
 * Generator Polynomial Length = .......... 32 bits
 * Reflected Bits = ....................... TRUE
 * Table Generation Offset = .............. 32 bits
 * Number of Slices = ..................... 8 slices
 * Slice Lengths = ........................ 8 8 8 8 8 8 8 8
 * Directory Name = ....................... .\
 * File Name = ............................ 8x256_tables.c
 */

static unsigned int sctp_crc_tableil8_o88[256] =
{
	0x00000000, 0x493C7D27, 0x9278FA4E, 0xDB448769, 0x211D826D, 0x6821FF4A, 0xB3657823, 0xFA590504,
	0x423B04DA, 0x0B0779FD, 0xD043FE94, 0x997F83B3, 0x632686B7, 0x2A1AFB90, 0xF15E7CF9, 0xB86201DE,
	0x847609B4, 0xCD4A7493, 0x160EF3FA, 0x5F328EDD, 0xA56B8BD9, 0xEC57F6FE, 0x37137197, 0x7E2F0CB0,
	0xC64D0D6E, 0x8F717049, 0x5435F720, 0x1D098A07, 0xE7508F03, 0xAE6CF224, 0x7528754D, 0x3C14086A,
	0x0D006599, 0x443C18BE, 0x9F789FD7, 0xD644E2F0, 0x2C1DE7F4, 0x65219AD3, 0xBE651DBA, 0xF759609D,
	0x4F3B6143, 0x06071C64, 0xDD439B0D, 0x947FE62A, 0x6E26E32E, 0x271A9E09, 0xFC5E1960, 0xB5626447,
	0x89766C2D, 0xC04A110A, 0x1B0E9663, 0x5232EB44, 0xA86BEE40, 0xE1579367, 0x3A13140E, 0x732F6929,
	0xCB4D68F7, 0x827115D0, 0x593592B9, 0x1009EF9E, 0xEA50EA9A, 0xA36C97BD, 0x782810D4, 0x31146DF3,
	0x1A00CB32, 0x533CB615, 0x8878317C, 0xC1444C5B, 0x3B1D495F, 0x72213478, 0xA965B311, 0xE059CE36,
	0x583BCFE8, 0x1107B2CF, 0xCA4335A6, 0x837F4881, 0x79264D85, 0x301A30A2, 0xEB5EB7CB, 0xA262CAEC,
	0x9E76C286, 0xD74ABFA1, 0x0C0E38C8, 0x453245EF, 0xBF6B40EB, 0xF6573DCC, 0x2D13BAA5, 0x642FC782,
	0xDC4DC65C, 0x9571BB7B, 0x4E353C12, 0x07094135, 0xFD504431, 0xB46C3916, 0x6F28BE7F, 0x2614C358,
	0x1700AEAB, 0x5E3CD38C, 0x857854E5, 0xCC4429C2, 0x361D2CC6, 0x7F2151E1, 0xA465D688, 0xED59ABAF,
	0x553BAA71, 0x1C07D756, 0xC743503F, 0x8E7F2D18, 0x7426281C, 0x3D1A553B, 0xE65ED252, 0xAF62AF75,
	0x9376A71F, 0xDA4ADA38, 0x010E5D51, 0x48322076, 0xB26B2572, 0xFB575855, 0x2013DF3C, 0x692FA21B,
	0xD14DA3C5, 0x9871DEE2, 0x4335598B, 0x0A0924AC, 0xF05021A8, 0xB96C5C8F, 0x6228DBE6, 0x2B14A6C1,
	0x34019664, 0x7D3DEB43, 0xA6796C2A, 0xEF45110D, 0x151C1409, 0x5C20692E, 0x8764EE47, 0xCE589360,
	0x763A92BE, 0x3F06EF99, 0xE44268F0, 0xAD7E15D7, 0x572710D3, 0x1E1B6DF4, 0xC55FEA9D, 0x8C6397BA,
	0xB0779FD0, 0xF94BE2F7, 0x220F659E, 0x6B3318B9, 0x916A1DBD, 0xD856609A, 0x0312E7F3, 0x4A2E9AD4,
	0xF24C9B0A, 0xBB70E62D, 0x60346144, 0x29081C63, 0xD3511967, 0x9A6D6440, 0x4129E329, 0x08159E0E,
	0x3901F3FD, 0x703D8EDA, 0xAB7909B3, 0xE2457494, 0x181C7190, 0x51200CB7, 0x8A648BDE, 0xC358F6F9,
	0x7B3AF727, 0x32068A00, 0xE9420D69, 0xA07E704E, 0x5A27754A, 0x131B086D, 0xC85F8F04, 0x8163F223,
	0xBD77FA49, 0xF44B876E, 0x2F0F0007, 0x66337D20, 0x9C6A7824, 0xD5560503, 0x0E12826A, 0x472EFF4D,
	0xFF4CFE93, 0xB67083B4, 0x6D3404DD, 0x240879FA, 0xDE517CFE, 0x976D01D9, 0x4C2986B0, 0x0515FB97,
	0x2E015D56, 0x673D2071, 0xBC79A718, 0xF545DA3F, 0x0F1CDF3B, 0x4620A21C, 0x9D642575, 0xD4585852,
	0x6C3A598C, 0x250624AB, 0xFE42A3C2, 0xB77EDEE5, 0x4D27DBE1, 0x041BA6C6, 0xDF5F21AF, 0x96635C88,
	0xAA7754E2, 0xE34B29C5, 0x380FAEAC, 0x7133D38B, 0x8B6AD68F, 0xC256ABA8, 0x19122CC1, 0x502E51E6,
	0xE84C5038, 0xA1702D1F, 0x7A34AA76, 0x3308D751, 0xC951D255, 0x806DAF72, 0x5B29281B, 0x1215553C,
	0x230138CF, 0x6A3D45E8, 0xB179C281, 0xF845BFA6, 0x021CBAA2, 0x4B20C785, 0x906440EC, 0xD9583DCB,
	0x613A3C15, 0x28064132, 0xF342C65B, 0xBA7EBB7C, 0x4027BE78, 0x091BC35F, 0xD25F4436, 0x9B633911,
	0xA777317B, 0xEE4B4C5C, 0x350FCB35, 0x7C33B612, 0x866AB316, 0xCF56CE31, 0x14124958, 0x5D2E347F,
	0xE54C35A1, 0xAC704886, 0x7734CFEF, 0x3E08B2C8, 0xC451B7CC, 0x8D6DCAEB, 0x56294D82, 0x1F1530A5
};

/*
 * end of the CRC lookup table crc_tableil8_o88
 */


static unsigned int
crc_c_tbl_nocase( const byte *buf,  unsigned int bufLen,
                  unsigned int key )
{
  register const byte * endBuf = &buf[ bufLen ];
  for ( ; buf < endBuf; buf++ ) {
    key = sctp_crc_tableil8_o32[ (byte) key ^ nocase( buf[ 0 ] ) ] ^
          ( key >> 8 );
  }
  return key;
}

unsigned int
Hash32::crc_c_tbl_hashInt( unsigned int key )
{
  return sctp_crc_tableil8_o56[ (byte) key ] ^
         sctp_crc_tableil8_o48[ (byte) ( key >> 8 ) ] ^
         sctp_crc_tableil8_o40[ (byte) ( key >> 16 ) ] ^
         sctp_crc_tableil8_o32[ (byte) ( key >> 24 ) ];
}

unsigned int
Hash32::crc_c_tbl_hashLong( unsigned long a )
{
  register unsigned int key = (unsigned int) a;
#if ! defined( __i386 )
  if ( sizeof( a ) == 8 ) {
    register unsigned int tmp = (unsigned int) ( a >> 32 );
    return sctp_crc_tableil8_o88[ (byte) key ] ^
           sctp_crc_tableil8_o80[ (byte) ( key >> 8 ) ] ^
           sctp_crc_tableil8_o72[ (byte) ( key >> 16 ) ] ^
           sctp_crc_tableil8_o64[ (byte) ( key >> 24 ) ] ^
           sctp_crc_tableil8_o56[ (byte) tmp ] ^
           sctp_crc_tableil8_o48[ (byte) ( tmp >> 8 ) ] ^
           sctp_crc_tableil8_o40[ (byte) ( tmp >> 16 ) ] ^
           sctp_crc_tableil8_o32[ (byte) ( tmp >> 24 ) ];
  }
#endif
  return sctp_crc_tableil8_o56[ (byte) key ] ^
         sctp_crc_tableil8_o48[ (byte) ( key >> 8 ) ] ^
         sctp_crc_tableil8_o40[ (byte) ( key >> 16 ) ] ^
         sctp_crc_tableil8_o32[ (byte) ( key >> 24 ) ];
}

unsigned int
Hash32::crc_c_tbl_hashULLong( ullong a )
{
  register unsigned int key = (unsigned int) a,
                        tmp = (unsigned int) ( a >> 32 );
  return sctp_crc_tableil8_o88[ (byte) key ] ^
         sctp_crc_tableil8_o80[ (byte) ( key >> 8 ) ] ^
         sctp_crc_tableil8_o72[ (byte) ( key >> 16 ) ] ^
         sctp_crc_tableil8_o64[ (byte) ( key >> 24 ) ] ^
         sctp_crc_tableil8_o56[ (byte) tmp ] ^
         sctp_crc_tableil8_o48[ (byte) ( tmp >> 8 ) ] ^
         sctp_crc_tableil8_o40[ (byte) ( tmp >> 16 ) ] ^
         sctp_crc_tableil8_o32[ (byte) ( tmp >> 24 ) ];
}

unsigned int
Hash32::crc_c_tbl( const byte *buf,  unsigned int bufLen,  unsigned int key )
{
  register const byte * endBuf = &buf[ bufLen ];
  register unsigned int tmp, tmp2;

#if ! defined( __amd64__ ) && ! defined( __i386 )
  if ( buf == endBuf )
    return key;
  /* align buf */
  while ( ( ( (ulongptr) (void *) buf ) & ( sizeof( int ) - 1 ) ) != 0 ) {
    key = sctp_crc_tableil8_o32[ (byte) key ^ buf[ 0 ] ] ^ ( key >> 8 );
    if ( ++buf == endBuf )
      return key;
  }
#endif
  if ( Aligned::isLittleEndian ) {
    /* calc 2 ints */
    while ( endBuf >= &buf[ sizeof( unsigned int ) * 2 ] ) {
      key ^= ((const unsigned int *) buf)[ 0 ];
      tmp  = ((const unsigned int *) buf)[ 1 ];
      buf  = (const byte *) &((const unsigned int *) buf)[ 2 ];
      key  = sctp_crc_tableil8_o88[ (byte) key ] ^
             sctp_crc_tableil8_o80[ (byte) ( key >> 8 ) ] ^
             sctp_crc_tableil8_o72[ (byte) ( key >> 16 ) ] ^
             sctp_crc_tableil8_o64[ (byte) ( key >> 24 ) ] ^
             sctp_crc_tableil8_o56[ (byte) tmp ] ^
             sctp_crc_tableil8_o48[ (byte) ( tmp >> 8 ) ] ^
             sctp_crc_tableil8_o40[ (byte) ( tmp >> 16 ) ] ^
             sctp_crc_tableil8_o32[ (byte) ( tmp >> 24 ) ];
    }
    /* calc ints */
    while ( endBuf >= &buf[ sizeof( unsigned int ) ] ) {
      key ^= ((const unsigned int *) buf)[ 0 ];
      buf  = (const byte *) &((const unsigned int *) buf)[ 1 ];
      key  = sctp_crc_tableil8_o56[ (byte) key ] ^
             sctp_crc_tableil8_o48[ (byte) ( key >> 8 ) ] ^
             sctp_crc_tableil8_o40[ (byte) ( key >> 16 ) ] ^
             sctp_crc_tableil8_o32[ (byte) ( key >> 24 ) ];
    }
  }
  else {
    /* calc 2 ints */
    while ( endBuf >= &buf[ sizeof( unsigned int ) * 2 ] ) {
      tmp2 = ((const unsigned int *) buf)[ 0 ];
      tmp  = ((const unsigned int *) buf)[ 1 ];
      Aligned::swap( tmp2 );
      Aligned::swap( tmp );
      key ^= tmp2;
      buf  = (const byte *) &((const unsigned int *) buf)[ 2 ];
      key  = sctp_crc_tableil8_o88[ (byte) key ] ^
             sctp_crc_tableil8_o80[ (byte) ( key >> 8 ) ] ^
             sctp_crc_tableil8_o72[ (byte) ( key >> 16 ) ] ^
             sctp_crc_tableil8_o64[ (byte) ( key >> 24 ) ] ^
             sctp_crc_tableil8_o56[ (byte) tmp ] ^
             sctp_crc_tableil8_o48[ (byte) ( tmp >> 8 ) ] ^
             sctp_crc_tableil8_o40[ (byte) ( tmp >> 16 ) ] ^
             sctp_crc_tableil8_o32[ (byte) ( tmp >> 24 ) ];
    }
    /* calc ints */
    while ( endBuf >= &buf[ sizeof( unsigned int ) ] ) {
      tmp2 = ((const unsigned int *) buf)[ 0 ];
      Aligned::swap( tmp2 );
      key ^= tmp2;
      buf  = (const byte *) &((const unsigned int *) buf)[ 1 ];
      key  = sctp_crc_tableil8_o56[ (byte) key ] ^
             sctp_crc_tableil8_o48[ (byte) ( key >> 8 ) ] ^
             sctp_crc_tableil8_o40[ (byte) ( key >> 16 ) ] ^
             sctp_crc_tableil8_o32[ (byte) ( key >> 24 ) ];
    }
  }
  /* calc bytes */
  while ( endBuf > buf ) {
    key = sctp_crc_tableil8_o32[ (byte) key ^ buf[ 0 ] ] ^ ( key >> 8 );
    buf++;
  }
  return key;
}


static unsigned int crc_c_cpu_test( const byte *buf,  unsigned int bufLen,
                                    unsigned int key );
static unsigned int crc_c_nocase_cpu_test( const byte *buf,  unsigned int bufLen,
                                           unsigned int key );

#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
static unsigned int crc_c_sse4_nocase( const byte *buf,  unsigned int bufLen,
                                       unsigned int key );
static unsigned int crc_c_sse4 ( const byte *buf,  unsigned int bufLen,
                            unsigned int key );
bool Hash32::hasSSE42;
#endif

unsigned int (*Hash32::crc_c_f)( const byte *buf,  unsigned int bufLen,
                                 unsigned int key ) = crc_c_cpu_test;
static unsigned int (*crc_c_nocase_f)( const byte *buf,  unsigned int bufLen,
                                     unsigned int key ) = crc_c_nocase_cpu_test;

#include "util/test_cpu_feature.h"

static unsigned int
crc_c_cpu_test( const byte *buf,  unsigned int bufLen,
                unsigned int keyInit ) {
  Hash32::crc_c_f = Hash32::crc_c_tbl;
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  if ( test_cpu_feature( CPU_FEATURE_SSE4_2 ) ) {
    if ( ::getenv( "RAI_NO_SSE4" ) == NULL ) {
      Hash32::hasSSE42 = true;
      Hash32::crc_c_f = crc_c_sse4;
    }
  }
#endif
  return Hash32::crc_c_f( buf, bufLen, keyInit );
}

static unsigned int
crc_c_nocase_cpu_test( const byte *buf,  unsigned int bufLen,
                       unsigned int keyInit ) {
  crc_c_nocase_f = crc_c_tbl_nocase;
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  if ( test_cpu_feature( CPU_FEATURE_SSE4_2 ) ) {
    if ( ::getenv( "RAI_NO_SSE4" ) == NULL ) {
      Hash32::hasSSE42 = true;
      crc_c_nocase_f = crc_c_sse4_nocase;
    }
  }
#endif
  return crc_c_nocase_f( buf, bufLen, keyInit );
}

unsigned int
Hash32::crc_cs( const char *buf,  bool noCase,  unsigned int keyInit )
{
  if ( ! noCase )
    return Hash32::crc_c_f( (const byte *) buf, ::strlen( buf ), keyInit );
  return crc_c_nocase_f( (const byte *) buf, ::strlen( buf ), keyInit );
}

#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
static unsigned int
crc_c_sse4_nocase( const byte *buf,  unsigned int bufLen,  unsigned int key )
{
#define crc32b( crc, x ) \
  __asm__ __volatile__ ( "crc32b %1, %0" : "+r" (crc) : "m" (x) : )
  register const byte * endBuf = &buf[ bufLen ];
  for ( ; buf < endBuf; buf++ ) {
    register byte c = nocase( buf[ 0 ] );
    crc32b( key, c );
  }
  return key;
}


static unsigned int
crc_c_sse4( const byte *buf,  unsigned int bufLen,  unsigned int key )
{
#define crc32q( crc, x ) \
  __asm__ __volatile__ ( "crc32q %1, %0" : "+r" (crc) : "m" (x) : )
#define crc32l( crc, x ) \
  __asm__ __volatile__ ( "crc32l %1, %0" : "+r" (crc) : "m" (x) : )
#define crc32w( crc, x ) \
  __asm__ __volatile__ ( "crc32w %1, %0" : "+r" (crc) : "m" (x) : )

  register const byte * endBuf = &buf[ bufLen ];

#ifndef __amd64__
  /* calc ints */
  while ( endBuf >= &buf[ sizeof( unsigned int ) ] ) {
    crc32l( key , ((const unsigned int*) buf)[ 0 ] );
    buf  = (const byte *) &((const unsigned int *) buf)[ 1 ];
  }
#else
  register ullong key64 = key;
  while ( endBuf >= &buf[ sizeof( unsigned int ) * 2 ] ) {
    crc32q( key64, ((const ullong *) buf)[ 0 ] );
    buf = (const byte *) &((const ullong *) buf)[ 1 ];
  }
  key = (unsigned int) key64;
#endif

  switch ( endBuf - buf ) {
    case 7:
      crc32b( key, buf[ 0 ] ); buf++;
    /* FALLTHRU */
    case 6:
      crc32w( key, ((const unsigned short *) buf)[ 0 ] ); buf = &buf[ 2 ];
    /* FALLTHRU */
    case 4:
      crc32l( key, ((const unsigned int *) buf)[ 0 ] );
      break;
    case 3:
      crc32b( key, buf[ 0 ] ); buf++;
    /* FALLTHRU */
    case 2:
      crc32w( key, ((const unsigned short *) buf)[ 0 ] );
      break;
    case 5:
      crc32l( key, ((const unsigned int *) buf)[ 0 ] ); buf = &buf[ 4 ];
    /* FALLTHRU */
    case 1:
      crc32b( key, buf[ 0 ] );
      break;
    default:
    case 0:
      break;
  }

  return key;
}
#endif

#if defined( __linux ) && defined( __amd64__ )
/* as fast as crc_c_sse4, not quite as good with collisions */
unsigned int
Hash32::bswap_mul128( const byte *p,  unsigned int len,
                      unsigned int keyInit )
{
  register unsigned long long r8  = 0x1591aefa5e7e5a17ULL,
                              r9  = 0x2bb6863566c4e761ULL,
                              rax = keyInit ^ r8,
                              rcx = r9,
                              rdx;
#define bswap( r ) \
  __asm__ __volatile__ ( "bswapq %0" : "+r" (r) : : )
#define mul128( a, d, r ) \
  __asm__ __volatile__ ( "mulq %2" : "+a" (a), "=d" (d) : "r" (r) : )
  while ( len >= 16 ) {
    rax = ( rax ^ ((const ullong *) p)[ 0 ] ) * r8;
    rcx = ( rcx ^ ((const ullong *) p)[ 1 ] ) * r9;
    bswap( rax );
    bswap( rcx );
    p    = (const byte *) &((const ullong *) p)[ 2 ];
    len -= 16;
  }
  if ( len != 0 ) {
    if ( ( len & 8 ) != 0 ) {
      rdx  = 0;
      rax ^= ((const ullong *) p)[ 0 ];
      p    = (const byte *) &((ullong *) p)[ 1 ];
    }
    if ( ( len & 4 ) != 0 ) {
      rdx = ((const unsigned int *) p)[ 0 ];
      p   = (const byte *) &((const unsigned int *) p)[ 1 ];
    }
    if ( ( len & 2 ) != 0 ) {
      rdx = ( rdx << 16 ) | ((const unsigned short *) p)[ 0 ];
      p   = (const byte *) &((const unsigned short *) p)[ 1 ];
    }
    if ( ( len & 1 ) != 0 ) {
      rdx = ( rdx << 8 ) | ((const byte *) p)[ 0 ];
    }
    rcx ^= rdx;
  }
  mul128( rax, rdx, r8 );
  rcx = ( rcx * r9 ) + rdx;
  rax ^= rcx;
  mul128( rax, rdx, r8 );
  rcx = ( rcx * r9 ) + rdx;
  rax ^= rcx;
#undef bswap
#undef mul128
  return ( rax >> 32 ) ^ rax;
}
#endif


/**
 * Orignally:
 *
 * Rijndael.java
 *
 * Optimised Java implementation of the Rijndael (AES) block cipher.
 *
 * @author Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * This software is hereby placed in the public domain.
 *
 */

/* Substitution table (S-box) */
const byte AES::Se[] = {
0x63,0x7c,0x77,0x7b, 0xf2,0x6b,0x6f,0xc5, 0x30,0x01,0x67,0x2b,
0xfe,0xd7,0xab,0x76, 0xca,0x82,0xc9,0x7d, 0xfa,0x59,0x47,0xf0,
0xad,0xd4,0xa2,0xaf, 0x9c,0xa4,0x72,0xc0, 0xb7,0xfd,0x93,0x26,
0x36,0x3f,0xf7,0xcc, 0x34,0xa5,0xe5,0xf1, 0x71,0xd8,0x31,0x15,
0x04,0xc7,0x23,0xc3, 0x18,0x96,0x05,0x9a, 0x07,0x12,0x80,0xe2,
0xeb,0x27,0xb2,0x75, 0x09,0x83,0x2c,0x1a, 0x1b,0x6e,0x5a,0xa0,
0x52,0x3b,0xd6,0xb3, 0x29,0xe3,0x2f,0x84, 0x53,0xd1,0x00,0xed,
0x20,0xfc,0xb1,0x5b, 0x6a,0xcb,0xbe,0x39, 0x4a,0x4c,0x58,0xcf,
0xd0,0xef,0xaa,0xfb, 0x43,0x4d,0x33,0x85, 0x45,0xf9,0x02,0x7f,
0x50,0x3c,0x9f,0xa8, 0x51,0xa3,0x40,0x8f, 0x92,0x9d,0x38,0xf5,
0xbc,0xb6,0xda,0x21, 0x10,0xff,0xf3,0xd2, 0xcd,0x0c,0x13,0xec,
0x5f,0x97,0x44,0x17, 0xc4,0xa7,0x7e,0x3d, 0x64,0x5d,0x19,0x73,
0x60,0x81,0x4f,0xdc, 0x22,0x2a,0x90,0x88, 0x46,0xee,0xb8,0x14,
0xde,0x5e,0x0b,0xdb, 0xe0,0x32,0x3a,0x0a, 0x49,0x06,0x24,0x5c,
0xc2,0xd3,0xac,0x62, 0x91,0x95,0xe4,0x79, 0xe7,0xc8,0x37,0x6d,
0x8d,0xd5,0x4e,0xa9, 0x6c,0x56,0xf4,0xea, 0x65,0x7a,0xae,0x08,
0xba,0x78,0x25,0x2e, 0x1c,0xa6,0xb4,0xc6, 0xe8,0xdd,0x74,0x1f,
0x4b,0xbd,0x8b,0x8a, 0x70,0x3e,0xb5,0x66, 0x48,0x03,0xf6,0x0e,
0x61,0x35,0x57,0xb9, 0x86,0xc1,0x1d,0x9e, 0xe1,0xf8,0x98,0x11,
0x69,0xd9,0x8e,0x94, 0x9b,0x1e,0x87,0xe9, 0xce,0x55,0x28,0xdf,
0x8c,0xa1,0x89,0x0d, 0xbf,0xe6,0x42,0x68, 0x41,0x99,0x2d,0x0f,
0xb0,0x54,0xbb,0x16 };

/* Inverse S-box, AES::Sd[ AES::Se[ X ] ] == X */
byte AES::Sd[ 256 ];

/* Round constants */
const unsigned int AES::rcon[ 30 ] = {
0x01, 0x02, 0x04, 0x08,  0x10, 0x20, 0x40, 0x80,
0x1b, 0x36, 0x6c, 0xd8,  0xab, 0x4d, 0x9a, 0x2f,
0x5e, 0xbc, 0x63, 0xc6,  0x97, 0x35, 0x6a, 0xd4,
0xb3, 0x7d, 0xfa, 0xef,  0xc5, 0x91 };

/* Transforms */
unsigned int AES::Te0[ 256 ], /* Se[x].[02, 01, 01, 03] */
             AES::Te1[ 256 ], /* Se[x].[03, 02, 01, 01] */
             AES::Te2[ 256 ], /* Se[x].[01, 03, 02, 01] */
             AES::Te3[ 256 ], /* Se[x].[01, 01, 03, 02] */
             AES::Te4[ 256 ], /* Se[x].[01, 01, 01, 01] */
             AES::Td0[ 256 ], /* Sd[x].[0e, 09, 0d, 0b] */
             AES::Td1[ 256 ], /* Sd[x].[0b, 0e, 09, 0d] */
             AES::Td2[ 256 ], /* Sd[x].[0d, 0b, 0e, 09] */
             AES::Td3[ 256 ], /* Sd[x].[09, 0d, 0b, 0e] */
             AES::Td4[ 256 ]; /* Sd[x].[01, 01, 01, 01] */
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
bool AES::hasSSE_AES;
#endif

/* Calculate the Sd/Te/Td using AES::Se S-box */
void
AES::initTables( void )
{
  static const unsigned int ROOT = 0x11B;
  unsigned int s1, s2, s3, i1, i2, i4, i8, i9, ib, id, ie, t;

#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  if ( ::getenv( "RAI_NO_SSE_AES" ) == NULL )
    AES::hasSSE_AES = test_cpu_feature( CPU_FEATURE_AES );
#endif
  /* Set up AES::Sd, the inverse S-box */
  for ( i1 = 0; i1 < 256; i1++ ) {
    s1 = AES::Se[ i1 ];
    AES::Sd[ s1 ] = (byte) i1;
  }
  for ( i1 = 0; i1 < 256; i1++ ) {
    s1 = AES::Se[ i1 ];
    s2 = s1 << 1;
    if ( s2 >= 0x100 )
      s2 ^= ROOT;
    s3 = s2 ^ s1;
    i2 = i1 << 1;
    if ( i2 >= 0x100 )
      i2 ^= ROOT;
    i4 = i2 << 1;
    if ( i4 >= 0x100 )
      i4 ^= ROOT;
    i8 = i4 << 1;
    if ( i8 >= 0x100 )
      i8 ^= ROOT;
    i9 = i8 ^ i1;
    ib = i9 ^ i2;
    id = i9 ^ i4;
    ie = i8 ^ i4 ^ i2;

    t = ( s2 << 24 ) | ( s1 << 16 ) | ( s1 << 8 ) | s3;
    AES::Te0[ i1 ] = t;
    AES::Te1[ i1 ] = ( t >>  8 ) | ( t  << 24 );
    AES::Te2[ i1 ] = ( t >> 16 ) | ( t  << 16 );
    AES::Te3[ i1 ] = ( t >> 24 ) | ( t  <<  8 );

    t = ( ie << 24 ) | ( i9 << 16 ) | ( id << 8 ) | ib;
    AES::Td0[ s1 ] = t;
    AES::Td1[ s1 ] = ( t >>  8 ) | ( t  << 24 );
    AES::Td2[ s1 ] = ( t >> 16 ) | ( t  << 16 );
    AES::Td3[ s1 ] = ( t >> 24 ) | ( t  <<  8 );

    /* The Te4[] and Td4[] are useful to construct ints without shifting
     * the S-box byte, a performance optimization (possibly insignificant) */
    t = AES::Sd[ i1 ];
    AES::Te4[ i1 ] = ( s1 << 24 ) | ( s1 << 16 ) | ( s1 << 8 ) | s1;
    AES::Td4[ i1 ] = ( t << 24 ) | ( t << 16 ) | ( t << 8 ) | t;
  }
}

/* Setup the AES key schedule for encryption and decryption */
bool
AES::setKey( const byte *key,  const unsigned int keylen )
{
  ::memset( this->Ke, 0, sizeof( this->Ke ) );
  ::memset( this->Kd, 0, sizeof( this->Kd ) );
  this->Nk = this->Nr = this->Nw = 0;

  if ( AES::Td4[ 255 ] == 0 )
    AES::initTables();

  /* check key size */
  if ( keylen != 128/8 && keylen != 192/8 && keylen != 256/8 )
    return false;
  this->Nk = keylen / 4;           /* 4=128, 6=192, 8=256 */
  this->Nr = this->Nk + 6;         /* 10=128, 12=192, 14=256 */
  this->Nw = 4 * ( this->Nr + 1 ); /* 44=128, 52=192, 60=256 */
  ::memset( this->Ke, 0, sizeof( this->Ke ) );
  ::memset( this->Kd, 0, sizeof( this->Kd ) );

  this->expandKey( key );
  this->invertKey();

#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  if ( AES::hasSSE_AES ) {
    unsigned int i;

    for ( i = 0; i < sizeof( this->Ke ) / sizeof( this->Ke[ 0 ] ); i++ )
      Aligned::endianSwap( this->Ke[ i ] );
    for ( i = 0; i < sizeof( this->Kd ) / sizeof( this->Kd[ 0 ] ); i++ )
      Aligned::endianSwap( this->Kd[ i ] );
  }
#endif
  return true;
}

/* Expand a cipher key into a full encryption key schedule */
void
AES::expandKey( const byte *key )
{
  unsigned int i, j, k, temp, r;

  for ( i = 0, k = 0; i < this->Nk; i++, k += 4 ) {
    this->Ke[ i ] = ( ( key[ k     ]        ) << 24 ) |
                    ( ( key[ k + 1 ] & 0xff ) << 16 ) |
                    ( ( key[ k + 2 ] & 0xff ) <<  8 ) |
                    ( ( key[ k + 3 ] & 0xff ) );
  }
  j = 0;
  for ( r = 0; ; ) {
    temp = this->Ke[ i - 1 ];
    this->Ke[ i ] = this->Ke[ j ] ^
                    ( AES::Te4[ ( temp >> 16 ) & 0xff ] & 0xff000000 ) ^
                    ( AES::Te4[ ( temp >>  8 ) & 0xff ] & 0x00ff0000 ) ^
                    ( AES::Te4[ ( temp       ) & 0xff ] & 0x0000ff00 ) ^
                    ( AES::Te4[ ( temp >> 24 ) & 0xff ] & 0x000000ff ) ^
                    ( AES::rcon[ r++ ] << 24 );
    this->Ke[ i + 1 ] = this->Ke[ j + 1 ] ^ this->Ke[ i ];
    this->Ke[ i + 2 ] = this->Ke[ j + 2 ] ^ this->Ke[ i + 1 ];
    this->Ke[ i + 3 ] = this->Ke[ j + 3 ] ^ this->Ke[ i + 2 ];
    if ( this->Nk == 4 ) {
      if ( r == 10 )
        return;
    }
    else if ( this->Nk == 6 ) {
      if ( r == 8 )
        return;
      this->Ke[ i + 4 ] = this->Ke[ j + 4 ] ^ this->Ke[ i + 3 ];
      this->Ke[ i + 5 ] = this->Ke[ j + 5 ] ^ this->Ke[ i + 4 ];
    }
    else { /* this->Nk == 8 */
      if ( r == 7 )
        return;
      temp = this->Ke[ i + 3 ];
      this->Ke[ i + 4 ] = this->Ke[ j + 4 ] ^
                      ( AES::Te4[ ( temp >> 24 ) & 0xff ] & 0xff000000 ) ^
                      ( AES::Te4[ ( temp >> 16 ) & 0xff ] & 0x00ff0000 ) ^
                      ( AES::Te4[ ( temp >>  8 ) & 0xff ] & 0x0000ff00 ) ^
                      ( AES::Te4[ ( temp       ) & 0xff ] & 0x000000ff );
      this->Ke[ i + 5 ] = this->Ke[ j + 5 ] ^ this->Ke[ i + 4 ];
      this->Ke[ i + 6 ] = this->Ke[ j + 6 ] ^ this->Ke[ i + 5 ];
      this->Ke[ i + 7 ] = this->Ke[ j + 7 ] ^ this->Ke[ i + 6 ];
    }
    i += this->Nk; j += this->Nk;
  }
}

/* Compute the decryption schedule from the encryption schedule */
void
AES::invertKey( void )
{
  unsigned int d = 0,
               e = 4 * this->Nr,
               w, r;
  /* apply the inverse MixColumn transform to all round keys but the first and
   * the last */
  this->Kd[ d ]     = this->Ke[ e ];
  this->Kd[ d + 1 ] = this->Ke[ e + 1 ];
  this->Kd[ d + 2 ] = this->Ke[ e + 2 ];
  this->Kd[ d + 3 ] = this->Ke[ e + 3 ];
  d += 4;
  e -= 4;
  for ( r = 1; r < this->Nr; r++ ) {
    w = this->Ke[ e ];
    this->Kd[ d ] = AES::Td0[ AES::Se[ ( w >> 24 )        ] & 0xff ] ^
                    AES::Td1[ AES::Se[ ( w >> 16 ) & 0xff ] & 0xff ] ^
                    AES::Td2[ AES::Se[ ( w >>  8 ) & 0xff ] & 0xff ] ^
                    AES::Td3[ AES::Se[ ( w       ) & 0xff ] & 0xff ];
    w = this->Ke[ e + 1 ];
    this->Kd[ d + 1 ] = AES::Td0[ AES::Se[ ( w >> 24 )        ] & 0xff ] ^
                        AES::Td1[ AES::Se[ ( w >> 16 ) & 0xff ] & 0xff ] ^
                        AES::Td2[ AES::Se[ ( w >>  8 ) & 0xff ] & 0xff ] ^
                        AES::Td3[ AES::Se[ ( w       ) & 0xff ] & 0xff ];
    w = this->Ke[ e + 2 ];
    this->Kd[ d + 2 ] = AES::Td0[ AES::Se[ ( w >> 24 )        ] & 0xff ] ^
                        AES::Td1[ AES::Se[ ( w >> 16 ) & 0xff ] & 0xff ] ^
                        AES::Td2[ AES::Se[ ( w >>  8 ) & 0xff ] & 0xff ] ^
                        AES::Td3[ AES::Se[ ( w       ) & 0xff ] & 0xff ];
    w = this->Ke[ e + 3 ];
    this->Kd[ d + 3 ] = AES::Td0[ AES::Se[ ( w >> 24 )        ] & 0xff ] ^
                        AES::Td1[ AES::Se[ ( w >> 16 ) & 0xff ] & 0xff ] ^
                        AES::Td2[ AES::Se[ ( w >>  8 ) & 0xff ] & 0xff ] ^
                        AES::Td3[ AES::Se[ ( w       ) & 0xff ] & 0xff ];
    d += 4;
    e -= 4;
  }
  this->Kd[ d ]     = this->Ke[ e ];
  this->Kd[ d + 1 ] = this->Ke[ e + 1 ];
  this->Kd[ d + 2 ] = this->Ke[ e + 2 ];
  this->Kd[ d + 3 ] = this->Ke[ e + 3 ];
}

/* Encrypt exactly 128 bits of plaintext.  */
void
AES::encrypt( const byte *pt,  byte *ct )
{
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  /* intel AES asm version */
  if ( AES::hasSSE_AES ) {
    byte pt_tmp[ 128 / 8 ] AES_SSE_ALIGNMENT,
         ct_tmp[ 128 / 8 ] AES_SSE_ALIGNMENT;
    const byte * a_pt = pt;
    byte       * a_ct = ct;
    if ( ( ( (ulongptr) (void *) pt | (ulongptr) (void *) ct ) & 0xf ) != 0 ) {
      ::memcpy( pt_tmp, pt, sizeof( pt_tmp ) );
      a_pt = pt_tmp;
      a_ct = ct_tmp;
    }

  #define AESENC     ".byte 0x66, 0x0f, 0x38, 0xdc, 0xc1\n\t"
  #define AESENCLAST ".byte 0x66, 0x0f, 0x38, 0xdd, 0xc1\n\t"

    __asm__ __volatile__ (
      "movdqu %1, %%xmm0\n\t"       /* xmm0 = *pt */
      "movdqa (%2), %%xmm1\n\t"     /* xmm1 = Ke[0] */
      "pxor   %%xmm1, %%xmm0\n\t"   /* pt  ^= Ke[0] */
      "movdqa 0x10(%2), %%xmm1\n\t" /* xmm1 = Ke[1] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0x20(%2), %%xmm1\n\t" /* xmm1 = Ke[2] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0x30(%2), %%xmm1\n\t" /* xmm1 = Ke[3] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0x40(%2), %%xmm1\n\t" /* xmm1 = Ke[4] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0x50(%2), %%xmm1\n\t" /* xmm1 = Ke[5] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0x60(%2), %%xmm1\n\t" /* xmm1 = Ke[6] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0x70(%2), %%xmm1\n\t" /* xmm1 = Ke[7] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0x80(%2), %%xmm1\n\t" /* xmm1 = Ke[8] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0x90(%2), %%xmm1\n\t" /* xmm1 = Ke[9] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0xa0(%2), %%xmm1\n\t" /* xmm1 = Ke[10] */
      "cmp $10, %3\n\t"             /* if rounds == 10 goto last */
      "jz 1f\n\t"
      AESENC
      "movdqa 0xb0(%2), %%xmm1\n\t" /* xmm1 = Ke[11] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0xc0(%2), %%xmm1\n\t" /* xmm1 = Ke[12] */
      "cmp $12, %3\n\t"             /* if rounds == 12 goto last */
      "jz 1f\n\t"
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0xd0(%2), %%xmm1\n\t" /* xmm1 = Ke[13] */
      AESENC                        /* aesenc %xmm1, %xmm0 */
      "movdqa 0xe0(%2), %%xmm1\n\t" /* xmm1 = Ke[14] */
  "1: " AESENCLAST                  /* aesenclast %xmm1, %xmm0 */
      "movdqu %%xmm0, %0\n"         /* *ct = xmm0 */
      : "=m" (*a_ct)
      : "m" (*a_pt), "r" (this->Ke), "r" (this->Nr)
      : "cc", "memory");

    if ( a_ct != ct )
      ::memcpy( ct, ct_tmp, sizeof( ct_tmp ) );
  #undef AESENC
  #undef AESENCLAST
    return;
  }
#endif
  unsigned int k = 0, r, s0, s1, s2, s3, t0, t1, t2, t3;

  /* map byte array block to cipher state and add initial round key */
  Unaligned::endianGetInt( pt, t0 );        t0 ^= this->Ke[ 0 ];
  Unaligned::endianGetInt( &pt[ 4 ], t1 );  t1 ^= this->Ke[ 1 ];
  Unaligned::endianGetInt( &pt[ 8 ], t2 );  t2 ^= this->Ke[ 2 ];
  Unaligned::endianGetInt( &pt[ 12 ], t3 ); t3 ^= this->Ke[ 3 ];
  /* Nr - 1 full rounds */
  for ( r = 1; r < this->Nr; r++ ) {
    k += 4;
    s0 = AES::Te0[ ( t0 >> 24 )        ] ^
         AES::Te1[ ( t1 >> 16 ) & 0xff ] ^
         AES::Te2[ ( t2 >>  8 ) & 0xff ] ^
         AES::Te3[ ( t3       ) & 0xff ] ^ this->Ke[ k ],
    s1 = AES::Te0[ ( t1 >> 24 )        ] ^
         AES::Te1[ ( t2 >> 16 ) & 0xff ] ^
         AES::Te2[ ( t3 >>  8 ) & 0xff ] ^
         AES::Te3[ ( t0       ) & 0xff ] ^ this->Ke[ k + 1 ],
    s2 = AES::Te0[ ( t2 >> 24 )        ] ^
         AES::Te1[ ( t3 >> 16 ) & 0xff ] ^
         AES::Te2[ ( t0 >>  8 ) & 0xff ] ^
         AES::Te3[ ( t1       ) & 0xff ] ^ this->Ke[ k + 2 ],
    s3 = AES::Te0[ ( t3 >> 24 )        ] ^
         AES::Te1[ ( t0 >> 16 ) & 0xff ] ^
         AES::Te2[ ( t1 >>  8 ) & 0xff ] ^
         AES::Te3[ ( t2       ) & 0xff ] ^ this->Ke[ k + 3 ];
    t0 = s0; t1 = s1; t2 = s2; t3 = s3;
  }
  /* last round */
  k += 4;
  s0 = ( AES::Te4[ ( t0 >> 24 )        ] & 0xff000000 ) ^
       ( AES::Te4[ ( t1 >> 16 ) & 0xff ] & 0x00ff0000 ) ^
       ( AES::Te4[ ( t2 >>  8 ) & 0xff ] & 0x0000ff00 ) ^
       ( AES::Te4[ ( t3       ) & 0xff ] & 0x000000ff ) ^ this->Ke[ k ];
  s1 = ( AES::Te4[ ( t1 >> 24 )        ] & 0xff000000 ) ^
       ( AES::Te4[ ( t2 >> 16 ) & 0xff ] & 0x00ff0000 ) ^
       ( AES::Te4[ ( t3 >>  8 ) & 0xff ] & 0x0000ff00 ) ^
       ( AES::Te4[ ( t0       ) & 0xff ] & 0x000000ff ) ^ this->Ke[ k + 1 ];
  s2 = ( AES::Te4[ ( t2 >> 24 )        ] & 0xff000000 ) ^
       ( AES::Te4[ ( t3 >> 16 ) & 0xff ] & 0x00ff0000 ) ^
       ( AES::Te4[ ( t0 >>  8 ) & 0xff ] & 0x0000ff00 ) ^
       ( AES::Te4[ ( t1       ) & 0xff ] & 0x000000ff ) ^ this->Ke[ k + 2 ];
  s3 = ( AES::Te4[ ( t3 >> 24 )        ] & 0xff000000 ) ^
       ( AES::Te4[ ( t0 >> 16 ) & 0xff ] & 0x00ff0000 ) ^
       ( AES::Te4[ ( t1 >>  8 ) & 0xff ] & 0x0000ff00 ) ^
       ( AES::Te4[ ( t2       ) & 0xff ] & 0x000000ff ) ^ this->Ke[ k + 3 ];
  Unaligned::endianPutInt( s0, ct );
  Unaligned::endianPutInt( s1, &ct[ 4 ] );
  Unaligned::endianPutInt( s2, &ct[ 8 ] );
  Unaligned::endianPutInt( s3, &ct[ 12 ] );
}

/* Decrypt exactly 128 bits of plaintext.  */
void
AES::decrypt( const byte *ct,  byte *pt )
{
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  /* intel AES asm version */
  if ( AES::hasSSE_AES ) {
    byte ct_tmp[ 128 / 8 ] AES_SSE_ALIGNMENT,
         pt_tmp[ 128 / 8 ] AES_SSE_ALIGNMENT;
    const byte * a_ct = ct;
    byte       * a_pt = pt;
    if ( ( ( (ulongptr) (void *) ct | (ulongptr) (void *) pt ) & 0xf ) != 0 ) {
      ::memcpy( ct_tmp, ct, sizeof( ct_tmp ) );
      a_ct = ct_tmp;
      a_pt = pt_tmp;
    }
  #define AESDEC     ".byte 0x66, 0x0f, 0x38, 0xde, 0xc1\n\t"
  #define AESDECLAST ".byte 0x66, 0x0f, 0x38, 0xdf, 0xc1\n\t"

    __asm__ __volatile__ (
      "movdqu %1, %%xmm0\n\t"       /* xmm0 = *ct */
      "movdqa (%2), %%xmm1\n\t"     /* xmm1 = Kd[0] */
      "pxor   %%xmm1, %%xmm0\n\t"   /* pt  ^= Kd[0] */
      "movdqa 0x10(%2), %%xmm1\n\t" /* xmm1 = Kd[1] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0x20(%2), %%xmm1\n\t" /* xmm1 = Kd[2] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0x30(%2), %%xmm1\n\t" /* xmm1 = Kd[3] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0x40(%2), %%xmm1\n\t" /* xmm1 = Kd[4] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0x50(%2), %%xmm1\n\t" /* xmm1 = Kd[5] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0x60(%2), %%xmm1\n\t" /* xmm1 = Kd[6] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0x70(%2), %%xmm1\n\t" /* xmm1 = Kd[7] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0x80(%2), %%xmm1\n\t" /* xmm1 = Kd[8] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0x90(%2), %%xmm1\n\t" /* xmm1 = Kd[9] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0xa0(%2), %%xmm1\n\t" /* xmm1 = Kd[10] */
      "cmp $10, %3\n\t"             /* if rounds == 10 goto last */
      "jz 1f\n\t"
      AESDEC
      "movdqa 0xb0(%2), %%xmm1\n\t" /* xmm1 = Kd[11] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0xc0(%2), %%xmm1\n\t" /* xmm1 = Kd[12] */
      "cmp $12, %3\n\t"             /* if rounds == 12 goto last */
      "jz 1f\n\t"
      AESDEC                           /* aesdec %xmm1, %xmm0 */
      "movdqa 0xd0(%2), %%xmm1\n\t" /* xmm1 = Kd[13] */
      AESDEC                        /* aesdec %xmm1, %xmm0 */
      "movdqa 0xe0(%2), %%xmm1\n\t" /* xmm1 = Kd[14] */
  "1: " AESDECLAST                  /* aesdeclast %xmm1, %xmm0 */
      "movdqu %%xmm0, %0\n"         /* *pt = xmm0 */
      : "=m" (*a_pt)
      : "m" (*a_ct), "r" (this->Kd), "r" (this->Nr)
      : "%rsi", "cc", "memory");

    if ( a_pt != pt )
      ::memcpy( pt, pt_tmp, sizeof( pt_tmp ) );
  #undef AESENC
  #undef AESENCLAST
     return;
  }
#endif
  unsigned int k = 0, r, s0, s1, s2, s3, t0, t1, t2, t3;

  /* map byte array block to cipher state and add initial round key */
  Unaligned::endianGetInt( ct, t0 );        t0 ^= this->Kd[ 0 ];
  Unaligned::endianGetInt( &ct[ 4 ], t1 );  t1 ^= this->Kd[ 1 ];
  Unaligned::endianGetInt( &ct[ 8 ], t2 );  t2 ^= this->Kd[ 2 ];
  Unaligned::endianGetInt( &ct[ 12 ], t3 ); t3 ^= this->Kd[ 3 ];
  /* Nr - 1 full rounds */
  for ( r = 1; r < this->Nr; r++ ) {
    k += 4;
    s0 = AES::Td0[ ( t0 >> 24 )        ] ^
         AES::Td1[ ( t3 >> 16 ) & 0xff ] ^
         AES::Td2[ ( t2 >>  8 ) & 0xff ] ^
         AES::Td3[ ( t1       ) & 0xff ] ^ this->Kd[ k ],            
    s1 = AES::Td0[ ( t1 >> 24 )        ] ^
         AES::Td1[ ( t0 >> 16 ) & 0xff ] ^
         AES::Td2[ ( t3 >>  8 ) & 0xff ] ^
         AES::Td3[ ( t2       ) & 0xff ] ^ this->Kd[ k + 1 ],
    s2 = AES::Td0[ ( t2 >> 24 )        ] ^
         AES::Td1[ ( t1 >> 16 ) & 0xff ] ^
         AES::Td2[ ( t0 >>  8 ) & 0xff ] ^
         AES::Td3[ ( t3       ) & 0xff ] ^ this->Kd[ k + 2 ],
    s3 = AES::Td0[ ( t3 >> 24 )        ] ^
         AES::Td1[ ( t2 >> 16 ) & 0xff ] ^
         AES::Td2[ ( t1 >>  8 ) & 0xff ] ^
         AES::Td3[ ( t0       ) & 0xff ] ^ this->Kd[ k + 3 ];            
    t0 = s0; t1 = s1; t2 = s2; t3 = s3;
  }
  /* last round */
  k += 4;
  s0 = ( AES::Td4[ ( t0 >> 24 )        ] & 0xff000000 ) ^
       ( AES::Td4[ ( t3 >> 16 ) & 0xff ] & 0x00ff0000 ) ^
       ( AES::Td4[ ( t2 >>  8 ) & 0xff ] & 0x0000ff00 ) ^
       ( AES::Td4[ ( t1       ) & 0xff ] & 0x000000ff ) ^ this->Kd[ k ];
  s1 = ( AES::Td4[ ( t1 >> 24 )        ] & 0xff000000 ) ^
       ( AES::Td4[ ( t0 >> 16 ) & 0xff ] & 0x00ff0000 ) ^
       ( AES::Td4[ ( t3 >>  8 ) & 0xff ] & 0x0000ff00 ) ^
       ( AES::Td4[ ( t2       ) & 0xff ] & 0x000000ff ) ^ this->Kd[ k + 1 ];
  s2 = ( AES::Td4[ ( t2 >> 24 )        ] & 0xff000000 ) ^
       ( AES::Td4[ ( t1 >> 16 ) & 0xff ] & 0x00ff0000 ) ^
       ( AES::Td4[ ( t0 >>  8 ) & 0xff ] & 0x0000ff00 ) ^
       ( AES::Td4[ ( t3       ) & 0xff ] & 0x000000ff ) ^ this->Kd[ k + 2 ];
  s3 = ( AES::Td4[ ( t3 >> 24 )        ] & 0xff000000 ) ^
       ( AES::Td4[ ( t2 >> 16 ) & 0xff ] & 0x00ff0000 ) ^
       ( AES::Td4[ ( t1 >>  8 ) & 0xff ] & 0x0000ff00 ) ^
       ( AES::Td4[ ( t0       ) & 0xff ] & 0x000000ff ) ^ this->Kd[ k + 3 ];
  Unaligned::endianPutInt( s0, pt );
  Unaligned::endianPutInt( s1, &pt[ 4 ] );
  Unaligned::endianPutInt( s2, &pt[ 8 ] );
  Unaligned::endianPutInt( s3, &pt[ 12 ] );
}

#if defined( TEST_AES )
static char
tohex( byte x )
{
  if ( x < 10 )
    return '0' + x;
  return 'a' + ( x - 10 );
}
static void
print_bytes( const char *s,  const byte *b,  size_t n )
{
  printf( "%s", s );
  for ( size_t i = 0; i < n; i++ ) {
    printf( "0x%c%c,", tohex( ( b[ i ] >> 4 ) & 0xf ), 
                       tohex( b[ i ] & 0xf ) );
  }
  printf( "\n" );
  fflush( stdout );
}
#endif
#if 0
static void
vcopy( byte *o, const byte *i, size_t z )
{
  do {
    *o++ = *i++;
  } while ( --z != 0 );
}
#endif
void
AES::encryptCTR( const byte *in,  byte *out,  const ullong ctr,
                 const ullong nonce )
{
  union {
    byte buf[ 16 ] AES_SSE_ALIGNMENT;
    struct {
      ullong ulx, uly;
    } x;
  } iv, ct;
  ullong a, b;

  /*printf( "ctr %lx\n", ctr );
  printf( "nonce %lx\n", nonce );
  print_bytes( "ctr_in: ", in, 16 );*/
  iv.x.ulx = nonce; Aligned::endianSwap( iv.x.ulx );
  iv.x.uly = ctr;   Aligned::endianSwap( iv.x.uly );

  this->encrypt( iv.buf, ct.buf );

  Unaligned::getInt( in, a );
  Unaligned::getInt( &in[ 8 ], b );
  a ^= ct.x.ulx;
  b ^= ct.x.uly;
  /* gcc 14.2.1 ccompiles this wrong */
  ::memcpy( (byte *) out, (const byte *) &a, sizeof( a ) );
  ::memcpy( (byte *) &out[ 8 ], (const byte *) &b, sizeof( b ) );
  /*print_bytes( "ctr_out: ", out, 16 );*/
}


ullong
AES::encryptBytesCTR( const byte *in,  byte *out,  unsigned int len,
                      ullong ctr,  ullong nonce )
{
  union {
    byte buf[ 16 ] AES_SSE_ALIGNMENT;
    struct {
      ullong ulx, uly;
    } x;
  } iv, ct;
  ullong a, b;

  Aligned::endianSwap( nonce );

  for ( unsigned int off = 0; off < len; off += 16 ) {
    iv.x.ulx = nonce;
    iv.x.uly = ctr++;
    Aligned::endianSwap( iv.x.uly );

    this->encrypt( iv.buf, ct.buf );

    if ( off + 16 <= len ) {
      Unaligned::getInt( &in[ off ], a );
      Unaligned::getInt( &in[ off + 8 ], b );
      a ^= ct.x.ulx;
      b ^= ct.x.uly;
      Unaligned::putInt( a, &out[ off ] );
      Unaligned::putInt( b, &out[ off + 8 ] );
    }
    else {
      for ( unsigned int i = 0; off < len; off++ )
        out[ off ] = in[ off ] ^ ct.buf[ i++ ];
      break;
    }
  }
  return ctr;
}


bool
AES::selftest128_unaligned( void )
{
  byte plain[]     = { 0, 0x01,0x4b,0xaf,0x22,0x78,0xa6,0x9d,0x33,
                       0x1d,0x51,0x80,0x10,0x36,0x43,0xe9,0x9a };
  byte key[]       = { 0, 0xe8,0xe9,0xea,0xeb,0xed,0xee,0xef,0xf0,
                       0xf2,0xf3,0xf4,0xf5,0xf7,0xf8,0xf9,0xfa };
  byte encrypted[] = { 0, 0x67,0x43,0xc3,0xd1,0x51,0x9a,0xb4,0xf2,
                       0xcd,0x9a,0x78,0xab,0x09,0xa5,0x11,0xbd };
  byte buf[ 17 ], buf2[ 17 ];
  AES cipher;

  ::memset( buf, 0, sizeof( buf ) );
  ::memset( buf2, 0, sizeof( buf2 ) );
  cipher.setKey( &key[ 1 ], 16 );
  cipher.encrypt( &plain[ 1 ], &buf[ 1 ] );
  cipher.decrypt( &buf[ 1 ], &buf2[ 1 ] );
#if defined( TEST_AES )
  print_bytes( "enc: ", buf, sizeof( buf ) );
  print_bytes( "pln: ", buf2, sizeof( buf2 ) );
  printf( " = %d\n", ::memcmp( &encrypted[ 1 ], &buf[ 1 ], 16 ) );
  printf( " = %d\n", ::memcmp( &plain[ 1 ], &buf2[ 1 ], 16 ) );
#endif
  return ::memcmp( &encrypted[ 1 ], &buf[ 1 ], 16 ) == 0 &&
         ::memcmp( &plain[ 1 ], &buf2[ 1 ], 16 ) == 0;
}


bool
AES::selftest128( void )
{
  byte plain[]     = { 0x01,0x4b,0xaf,0x22,0x78,0xa6,0x9d,0x33,
                       0x1d,0x51,0x80,0x10,0x36,0x43,0xe9,0x9a };
  byte key[]       = { 0xe8,0xe9,0xea,0xeb,0xed,0xee,0xef,0xf0,
                       0xf2,0xf3,0xf4,0xf5,0xf7,0xf8,0xf9,0xfa };
  byte encrypted[] = { 0x67,0x43,0xc3,0xd1,0x51,0x9a,0xb4,0xf2,
                       0xcd,0x9a,0x78,0xab,0x09,0xa5,0x11,0xbd };
  byte buf[ 16 ], buf2[ 16 ];
  AES cipher;

  ::memset( buf, 0, sizeof( buf ) );
  ::memset( buf2, 0, sizeof( buf2 ) );
  cipher.setKey( key, sizeof( key ) );
  cipher.encrypt( plain, buf );
  cipher.decrypt( buf, buf2 );
#if defined( TEST_AES )
  print_bytes( "enc: ", buf, sizeof( buf ) );
  print_bytes( "pln: ", buf2, sizeof( buf2 ) );
  printf( " = %d\n", ::memcmp( encrypted, buf, 16 ) );
  printf( " = %d\n", ::memcmp( plain, buf2, 16 ) );
#endif
  return ::memcmp( encrypted, buf, 16 ) == 0 &&
         ::memcmp( plain, buf2, 16 ) == 0;
}


bool
AES::selftest192( void )
{
  byte plain[]     = { 0x76,0x77,0x74,0x75,0xf1,0xf2,0xf3,0xf4,
                       0xf8,0xf9,0xe6,0xe7,0x77,0x70,0x71,0x72 };
  byte key[]       = { 0x04,0x05,0x06,0x07,0x09,0x0a,0x0b,0x0c,
                       0x0e,0x0f,0x10,0x11,0x13,0x14,0x15,0x16,
                       0x18,0x19,0x1a,0x1b,0x1d,0x1e,0x1f,0x20 };
  byte encrypted[] = { 0x5d,0x1e,0xf2,0x0d,0xce,0xd6,0xbc,0xbc,
                       0x12,0x13,0x1a,0xc7,0xc5,0x47,0x88,0xaa };
  byte buf[ 16 ], buf2[ 16 ];
  AES cipher;

  ::memset( buf, 0, sizeof( buf ) );
  ::memset( buf2, 0, sizeof( buf2 ) );
  cipher.setKey( key, sizeof( key ) );
  cipher.encrypt( plain, buf );
  cipher.decrypt( buf, buf2 );
#if defined( TEST_AES )
  print_bytes( "enc: ", buf, sizeof( buf ) );
  print_bytes( "pln: ", buf2, sizeof( buf2 ) );
  printf( " = %d\n", ::memcmp( encrypted, buf, 16 ) );
  printf( " = %d\n", ::memcmp( plain, buf2, 16 ) );
#endif
  return ::memcmp( encrypted, buf, 16 ) == 0 &&
         ::memcmp( plain, buf2, 16 ) == 0;
}


bool
AES::selftest256( void )
{
  byte plain[]     = { 0x06,0x9a,0x00,0x7f,0xc7,0x6a,0x45,0x9f,
                       0x98,0xba,0xf9,0x17,0xfe,0xdf,0x95,0x21 };
  byte key[]       = { 0x08,0x09,0x0a,0x0b,0x0d,0x0e,0x0f,0x10,
                       0x12,0x13,0x14,0x15,0x17,0x18,0x19,0x1a,
                       0x1c,0x1d,0x1e,0x1f,0x21,0x22,0x23,0x24,
                       0x26,0x27,0x28,0x29,0x2b,0x2c,0x2d,0x2e };
  byte encrypted[] = { 0x08,0x0e,0x95,0x17,0xeb,0x16,0x77,0x71,
                       0x9a,0xcf,0x72,0x80,0x86,0x04,0x0a,0xe3 };
  byte buf[ 16 ], buf2[ 16 ];
  AES cipher;

  ::memset( buf, 0, sizeof( buf ) );
  ::memset( buf2, 0, sizeof( buf2 ) );
  cipher.setKey( key, sizeof( key ) );
  cipher.encrypt( plain, buf );
  cipher.decrypt( buf, buf2 );
#if defined( TEST_AES )
  print_bytes( "enc: ", buf, sizeof( buf ) );
  print_bytes( "pln: ", buf2, sizeof( buf2 ) );
  printf( " = %d\n", ::memcmp( encrypted, buf, 16 ) );
  printf( " = %d\n", ::memcmp( plain, buf2, 16 ) );
#endif
  return ::memcmp( encrypted, buf, 16 ) == 0 &&
         ::memcmp( plain, buf2, 16 ) == 0;
}

bool
AES::selftestCTR( void )
{
  const char plain[] = "0123456789ABCDE";
  const byte enc[]  = { 0xd1,0xe0,0xc8,0x94,0xda,0xef,0xab,0xdc,
                        0xe2,0xff,0x76,0x79,0x6b,0xe0,0x60,0x44 };
  byte key[]        = { 0xe8,0xe9,0xea,0xeb,0xed,0xee,0xef,0xf0,
                        0xf2,0xf3,0xf4,0xf5,0xf7,0xf8,0xf9,0xfa };
  byte buf[ 16 ], buf2[ 16 ];
  AES cipher;

  ::memset( buf, 0, sizeof( buf ) );
  ::memset( buf2, 0, sizeof( buf2 ) );
  cipher.setKey( key, sizeof( key ) );
  cipher.encryptCTR( (const byte *) plain, buf, 1 );
  if ( ::memcmp( enc, buf, 16 ) != 0 )
    return false;
  cipher.encryptCTR( buf, buf2, 1 );

#if defined( TEST_AES )
  print_bytes( "enc: ", buf, sizeof( buf ) );
  print_bytes( "pln: ", buf2, sizeof( buf2 ) );
  printf( " = %d\n", ::memcmp( enc, buf, 16 ) );
  printf( " = %d\n", ::strcmp( plain, (char *) buf2 ) );
  printf( "%s\n", (char *) buf2 );
#endif
  return /*::memcmp( enc, buf, 16 ) == 0 &&*/
         ::strcmp( plain, (char *) buf2 ) == 0;
}


#if 0
#define ROR(x, r) ((x >> r) | (x << (64 - r)))
#define ROL(x, r) ((x << r) | (x >> (64 - r)))
#define R(x, y, k) (x = ROR(x, 8), x += y, x ^= k, y = ROL(y, 3), y ^= x)
#define ROUNDS 32

void
Speck::encrypt( ullong const pt[ 2 ], /* input plaintext */
                ullong ct[ 2 ],       /* output cyphertext */
                ullong const K[ 2 ] ) /* key */
{
   ullong y = pt[ 0 ], x = pt[ 1 ], b = K[ 0 ], a = K[ 1 ];

   R( x, y, b );
   for ( unsigned int i = 0; i < ROUNDS - 1; i++ ) {
      R( a, b, i );
      R( x, y, b );
   }

   ct[ 0 ] = y;
   ct[ 1 ] = x;
}
#endif

#if defined( TEST_AES )
int
main( int argc, char *argv[] )
{
  if ( AES::selftest128_unaligned() &&
       AES::selftest128() &&
       AES::selftest192() &&
       AES::selftest256() &&
       AES::selftestCTR() ) {
    printf( "passed AES tests\n" );
  }
  
  return 0;
}
#endif


int
Hash32::selftest( void )
{
  static const char s[] = "ci_sass", t[] = "CI_SASS";
  static const byte b[] = { 0x2e, 0xdb, 0x66, 0x94, 0x22, 0x7f, 0x00, 0x00 };
  static const char c[] = "abcdefghijklmnopqrstuvwxyz";
  static unsigned int crc_chash[] = {
    0x0, 0x93ad1061, 0x13c35ee4, 0x562f9ccd, 0xdaaf41f6, 0x8122a0a2, 0x496937b,
    0x5d199e2c, 0x86bc933d, 0x9639f15f, 0x584645c, 0xe42ca93e, 0xb0fa868d,
    0xe380529c, 0x12b69fd0, 0xec14f872, 0xe1d7640f, 0xdd013c4e, 0x7d7d875d,
    0x8cb84c7e, 0x6b6e9074, 0xf200ed93, 0xd633bceb, 0xdf3d007b, 0x138fcef4,
    0x3dcdf865, 0x4e7036b3
  };
  static unsigned int crc_hash[] = {
    0x0, 0xe8b7be43, 0x9e83486d, 0x352441c2, 0xed82cd11, 0x8587d865, 0x4b8e39ef,
    0x312a6aa6, 0xaeef2a50, 0x8da988af, 0x3981703a, 0xce570f9f, 0xf6781b24,
    0xddf46ea2, 0x400d9578, 0x519167df, 0x943ac093, 0x9c925619, 0x8fec50b,
    0x8cd4e846, 0x1a596ae5, 0x221725a3, 0x2499def3, 0x38f3316a, 0x21836df4,
    0x412a937d, 0x4c2750bd
  };
  static unsigned int crc_hashint[] = {
    0x0, 0x14e8b055, 0x381b26de, 0x1b592aa7, 0xba26e92e, 0x78d219f5, 0x7a791761,
    0x1758b8cb, 0xb38d012f, 0xaf92843f, 0xf7e3ec21, 0x738c8aad, 0x14cb1f27,
    0xc5a9245e, 0x84617950, 0x10496186, 0x79c2bdb, 0xd197073d, 0x5ce3f317,
    0x2f204ea9, 0x99484ebe, 0x657bea96, 0x87d0bfd7, 0xc33e591, 0x673f94a,
    0x74329c6a, 0x1ccee325, 
  };
  unsigned int i;

  if ( Hash32::crc_cs( s, true ) != 0x62669718U )
    return -1;
  if ( Hash32::crc_c( (byte *) t, sizeof( t ) - 1 ) != 0x62669718U )
    return -2;
  for ( i = 0; i < sizeof( c ); i++ )
    if ( Hash32::crc_c( (const byte *) c, i ) != crc_chash[ i ] )
      return -3;
  if ( crc_c_tbl_nocase( (const byte *) s, sizeof( s ) - 1, 0 ) != 0x62669718U )
    return -4;
  if ( Hash32::crc_c_tbl( (const byte *) t, sizeof( t ) - 1 ) != 0x62669718U )
    return -5;
  for ( i = 0; i < sizeof( c ); i++ )
    if ( Hash32::crc_c_tbl( (const byte *) c, i ) != crc_chash[ i ] )
      return -6;
  if ( sizeof( void * ) == 8 ) {
    if ( Hash32::hashPtr( (const void *) 0x7f229466db2eULL ) != 0x88d8d94fU )
      return -7;
#if defined( _WIN64 )
    if ( Hash32::crc_c_tbl_hashULLong(
                          (ullong) 0x7f229466db2eULL ) != 0x88d8d94fU )
      return -8;
#else
    if ( Hash32::crc_c_tbl_hashLong(
                          (unsigned long) 0x7f229466db2eULL ) != 0x88d8d94fU )
      return -8;
#endif
  }
  else {
    if ( Hash32::hashPtr( (const void *) 0x9466db2eUL ) != 0xb7da0e85U )
      return -9;
    if ( Hash32::crc_c_tbl_hashLong( 0x9466db2eUL ) != 0xb7da0e85U )
      return -10;
  }
  if ( Hash32::hashULLong( 0x7f229466db2eULL ) != 0x88d8d94fU )
    return -11;
  if ( Hash32::crc_c_tbl_hashULLong( 0x7f229466db2eULL ) != 0x88d8d94fU )
    return -12;
  if ( Hash32::crc_c( b, sizeof( b ) ) != 0x88d8d94fU )
    return -13;
  if ( Hash32::crc_c_tbl( b, sizeof( b ) ) != 0x88d8d94fU )
    return -14;
  if ( Hash32::newhashs( s, true ) != 0x2a80dfe9U )
    return -15;
  if ( Hash32::newhash( (const byte *) t, sizeof( t ) - 1 ) != 0x2a80dfe9U )
    return -16;
  if ( Hash32::djbs( s, true ) != 0x353aa642U )
    return -17;
  if ( Hash32::djb( (const byte *) t, sizeof( t ) - 1 ) != 0x353aa642U )
    return -18;
  if ( Hash32::fnvs( s, true ) != 0xb4873c1aU )
    return -19;
  if ( Hash32::fnv( (const byte *) t, sizeof( t ) - 1 ) != 0xb4873c1aU )
    return -20;
  if ( Hash32::crc( (const byte *) t, sizeof( t ) - 1 ) != 0xb18ffa46U )
    return -21;
  for ( i = 0; i < sizeof( c ); i++ )
    if ( Hash32::crc( (const byte *) c, i ) != crc_hash[ i ] )
      return -22;
  if ( Hash32::hashInt( 750 ) != 0x534da1caU )
    return -23;
  if ( Hash32::crc_c_tbl_hashInt( 750 ) != 0x534da1caU )
    return -24;
  if ( Hash32::bjHashInt( 750 ) != 0x83ff6085U )
    return -25;
  for ( i = 0; i < sizeof( crc_chash ) / sizeof( crc_chash[ 0 ] ); i++ )
    if ( Hash32::hashInt( crc_chash[ i ] ) != crc_hashint[ i ] )
      return -26;
  for ( i = 0; i < sizeof( crc_chash ) / sizeof( crc_chash[ 0 ] ); i++ )
    if ( Hash32::crc_c_tbl_hashInt( crc_chash[ i ] ) != crc_hashint[ i ] )
      return -27;
  MD4Ctx md4;
  if ( ! md4.selftest() )
    return -28;
  MD5Ctx md5;
  if ( ! md5.selftest() )
    return -29;
  SHA1Ctx sha1;
  if ( ! sha1.selftest() )
    return -30;
  if ( ! AES::selftest128_unaligned() )
    return -31;
  if ( ! AES::selftest128() )
    return -32;
  if ( ! AES::selftest192() )
    return -33;
  if ( ! AES::selftest256() )
    return -34;
  if ( ! AES::selftestCTR() )
    return -35;
  SHA256Ctx sha2;
  if ( ! sha2.selftest() )
    return -36;
  SHA512Ctx sha2_512;
  if ( ! sha2_512.selftest() )
    return -37;
  return 0;
}

