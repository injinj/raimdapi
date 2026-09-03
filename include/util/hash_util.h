/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__hash_util_h__
#define __rai_util__hash_util_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

/*
times of hash( "TSE.EQ.3726.To" ) * 1000000

p4 @ 3.06               x86_64 @ 1.80 (32 bit)  x86_64 @ 2.00 (64 bit)
ripemd160:  876.620ms   ripemd160:  867.529ms   ripemd160:  674.631ms
ripemd128:  892.659ms   ripemd128:  668.842ms   ripemd128:  500.306ms
sha1(160):  617.494ms   sha1(160):  510.849ms   sha1(160):  388.758ms
md5(128):   354.325ms   md5(128):   411.465ms   md5(128):   328.290ms
md4(128):   277.896ms   md4(128):   300.348ms   md4(128):   243.840ms
newhash64:  237.072ms   newhash64:  180.439ms   newhash32:  96.910ms
fnv64:      161.778ms   newhash32:  124.847ms   newhash64:  89.952ms
newhash32:  118.386ms   fnv64:      104.719ms   crc32:      39.977ms
fnv32:      58.368ms    djb32:      47.426ms    djb32:      38.975ms
djb32:      34.112ms    fnv32:      41.170ms    fnv64:      37.478ms
adler32:    33.032ms    crc32:      40.466ms    fnv32:      31.488ms
crc32:      24.594ms    adler32:    35.997ms    adler32:    32.477ms

times of hash( <640 bytes> ) * 1000000

p4 @ 3.06                     x86_64 @ 1.80
ripemd160:  8643.761ms        ripemd160:  8480.532ms
ripemd128:  8321.082ms        ripemd128:  6114.806ms
fnv64:      7975.377ms        sha1(160):  4513.570ms
sha1(160):  5575.145ms        fnv64:      4132.338ms
newhash64:  3188.747ms        md5(128):   3428.823ms
fnv32:      3169.974ms        newhash32:  2777.039ms
newhash32:  2876.472ms        newhash64:  2568.998ms
md5(128):   2452.636ms        md4(128):   2258.392ms
md4(128):   1865.886ms        fnv32:      1486.013ms
djb32:      1479.760ms        djb32:      1844.328ms
crc32:      840.437ms         crc32:      1049.467ms
adler32:    348.390ms         adler32:    581.507ms

the times above ignore caching of code and data...
crc32 uses tables which might affect cache performance
the secure hashes (ripemd, md, sha) always hash 64 bytes even on short strings,
and some are quite a bit larger codewise than the unsecure hashes, this also
might affect cache performance
*/

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

namespace rai {
/* Works with HashStream to hash io streams */
class RAIBASE_DLL_EXP HashContext {
  public:
    virtual ~HashContext() {};
    /* (re)initialize state */
    virtual void init( void )                                        = 0;
    /* add bytes to hash */
    virtual void update( const byte *input,  unsigned int inputLen ) = 0;
    /* after all input updated, call final to make digest available  */
    virtual void final( void )                                       = 0;
    /* hash size in bytes */
    virtual unsigned int digestSize( void )                          = 0;
    /* block size in bytes */
    virtual unsigned int blockSize( void )                           = 0;
    /* copy digest bits to d, should only be called after final() */
    virtual void digest( void *d )                                   = 0;
    /* tell hash to check precalculated values */
    virtual bool selftest( void )                                    = 0;
    /* dup the context, create a copy for multiple threads */
    virtual HashContext *dup( void ) = 0;
    /* dup the context into memory at p, return null if not enough mem */
    virtual HashContext *dup2( void *p, unsigned int &sz )           = 0;

    enum HashType {
      MD4, MD5, SHA1, RIPEMD128, RIPEMD160, SHA256, SHA512
    };
    static HashContext *create( HashType type );

    static HashContext *createHMAC( HashType type,  const byte *key,
                                    unsigned int keyLen );
};

namespace Hash32 {

/* return 0 if all passed < 0 if something failed */
RAIBASE_DLL_EXP
int selftest( void ); 

/* comp.lang.c post credited to Dan Bernstein */
const unsigned int djbKeyInit = 5381U;
RAIBASE_DLL_EXP
unsigned int djb( const byte *buf,  unsigned int bufLen,
                  unsigned int keyInit = djbKeyInit );
RAIBASE_DLL_EXP
unsigned int djbs( const char *buf,  bool noCase = false,
                   unsigned int keyInit = djbKeyInit );

/* sleepycat dbm hash credited to Glenn Fowler, Landon Curt Noll, Phong Vo
 * http://www.isthe.com/chongo/index.html */
const unsigned int fnvKeyInit = 0x811c9dc5U;
RAIBASE_DLL_EXP
unsigned int fnv( const byte *buf,  unsigned int bufLen,
                  unsigned int keyInit = fnvKeyInit );
RAIBASE_DLL_EXP
unsigned int fnvs( const char *buf,  bool noCase = false,
                   unsigned int keyInit = fnvKeyInit );

/* Bob Jenkins' newhash http://burtleburtle.net/bob/hash/evahash.html */
const unsigned int newhashKeyInit = 0;
RAIBASE_DLL_EXP
unsigned int newhash( const byte *buf,  unsigned int bufLen,
                      unsigned int keyInit = newhashKeyInit );
RAIBASE_DLL_EXP
unsigned int newhashs( const char *buf,  bool noCase = false,
                       unsigned int keyInit = newhashKeyInit );

#if 0
const unsigned int newhashMagic = 0x9e3779b9U;
inline unsigned int mixInt( unsigned int a, unsigned int b = newhashMagic,
                            unsigned int c = newhashMagic ) {
  newhash_mix( a, b, c );
  return c;
}
#endif

/* Bob Jenkins http://www.burtleburtle.net/bob/hash/integer.html       */
/* Bob says: I've confirmed this does well with sequences incremented  */
/* by common amounts whether you use the high or low bits of the hash. */
RAIBASE_DLL_EXP
unsigned int bjHashInt( unsigned int a );

#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
extern bool hasSSE42;
#endif

RAIBASE_DLL_EXP
unsigned int crc_c_tbl_hashInt( unsigned int a );

inline unsigned int hashInt( unsigned int a ) {
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  if ( hasSSE42 ) {
    register unsigned int b = 0;
    __asm__ __volatile__ ( "crc32l %1, %0" : "+r" (b) : "m" (a) : );
    return b;
  }
#endif
  return crc_c_tbl_hashInt( a );
}

RAIBASE_DLL_EXP
unsigned int crc_c_tbl_hashLong( unsigned long a );

inline unsigned int hashLong( unsigned long a ) {
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  if ( hasSSE42 ) {
    register unsigned long b = 0;
#if defined( __amd64__ )
    __asm__ __volatile__ ( "crc32q %1, %0" : "+r" (b) : "m" (a) : );
#else
    __asm__ __volatile__ ( "crc32l %1, %0" : "+r" (b) : "m" (a) : );
#endif
    return (unsigned int) b;
  }
#endif
  return crc_c_tbl_hashLong( a );
}

RAIBASE_DLL_EXP
unsigned int crc_c_tbl_hashULLong( ullong a );

inline unsigned int hashULLong( ullong a ) {
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  if ( hasSSE42 ) {
    register unsigned long b = 0;
#if defined( __amd64__ )
    __asm__ __volatile__ ( "crc32q %1, %0" : "+r" (b) : "m" (a) : );
#else
    register unsigned int c = (unsigned int) ( a >> 32 ),
                          d = (unsigned int) a;
    __asm__ __volatile__ ( "crc32l %1, %0\n\t"
                           "crc32l %2, %0"
                           : "+r" (b) : "m" (d), "m" (c) : );
#endif
    return (unsigned int) b;
  }
#endif
  return crc_c_tbl_hashULLong( a );
}

inline unsigned int hashPtr( const void *a ) {
  if ( sizeof( void * ) == sizeof( ullong ) )
    return Hash32::hashULLong( (ulongptr) a );
  return Hash32::hashLong( (ulongptr) a );
}

const unsigned int crcKeyInit = 0;
#if 0
/* Mark Adler's crc32 & adler32 http://www.gzip.org/zlib */
RAIBASE_DLL_EXP
unsigned int crc( const byte *ptr,  unsigned int size,
                  unsigned int keyInit = crcKeyInit );

const unsigned int adlerKeyInit = 0;
RAIBASE_DLL_EXP
unsigned int adler( const byte *buf,  unsigned int len,
                    unsigned int keyInit = adlerKeyInit );
#endif
/* SSE4.2 crc_c_tbl() version does not use crc instruction.
 * Use Hash32::crc_c() instead, since it determines if the CPU supports SSE4.2
 * then either uses the instruction or uses crc_c_tbl() as a fallback */
RAIBASE_DLL_EXP
unsigned int crc_c_tbl( const byte *ptr,  unsigned int size,
                        unsigned int keyInit = crcKeyInit );

extern RAIBASE_DLL_EXP
       unsigned int (*crc_c_f)( const byte *buf,  unsigned int bufLen,
                                unsigned int keyInit );

/* SSE4.2 crc c hash which uses the crc instruction if available */
static inline unsigned int
crc_c( const byte *ptr,  unsigned int size,
       unsigned int keyInit = crcKeyInit ) {
  return (*crc_c_f)( ptr, size, keyInit );
}
RAIBASE_DLL_EXP
unsigned int crc_cs( const char *buf,  bool noCase = false,
                     unsigned int keyInit = crcKeyInit );
#if defined( __linux ) && defined( __amd64__ )
/* fast, but not as fast as SSE4.2 */
unsigned int bswap_mul128( const byte *p,  unsigned int len, 
                           unsigned int keyInit = 0 );
#endif
}


namespace Hash16 {
/* comp.lang.c post credited to Dan Bernstein */
RAIBASE_DLL_EXP
unsigned short djb( const byte *buf,  unsigned int bufLen,
                    unsigned int keyInit = Hash32::djbKeyInit );

RAIBASE_DLL_EXP
unsigned short djbs( const char *buf,  bool noCase = false,
                     unsigned int keyInit = Hash32::djbKeyInit );
}


namespace Hash64 {

/* sleepycat dbm hash credited to Glenn Fowler, Landon Curt Noll, Phong Vo
 * http://www.isthe.com/chongo/index.html */
const ullong fnvKeyInit = ( (ullong) 0xcbf29ce4 << 32 ) | (ullong) 0x84222325U;
RAIBASE_DLL_EXP
ullong fnv( const byte *buf,  unsigned int bufLen,
            ullong keyInit = fnvKeyInit );

/* Bob Jenkins' newhash http://burtleburtle.net/bob/hash/evahash.html */
const ullong newhashKeyInit = 0;
RAIBASE_DLL_EXP
ullong newhash( const byte *buf,  unsigned int bufLen,
                ullong keyInit = newhashKeyInit );
RAIBASE_DLL_EXP
ullong newhashs( const char *buf,  bool noCase = false,
                 ullong keyInit = newhashKeyInit );

const ullong newhashMagic = ( (ullong) 0x9e3779b9U << 32 ) |
                              (ullong) 0x7f4a7c13U;

inline void newhash_mix( ullong &a,  ullong &b,  ullong &c ) {
  a = a - b;  a = a - c;  a = a ^ ( c >> 43 );
  b = b - c;  b = b - a;  b = b ^ ( a << 9 );
  c = c - a;  c = c - b;  c = c ^ ( b >> 8 );
  a = a - b;  a = a - c;  a = a ^ ( c >> 38 );
  b = b - c;  b = b - a;  b = b ^ ( a << 23 );
  c = c - a;  c = c - b;  c = c ^ ( b >> 5 );
  a = a - b;  a = a - c;  a = a ^ ( c >> 35 );
  b = b - c;  b = b - a;  b = b ^ ( a << 49 );
  c = c - a;  c = c - b;  c = c ^ ( b >> 11 );
  a = a - b;  a = a - c;  a = a ^ ( c >> 12 );
  b = b - c;  b = b - a;  b = b ^ ( a << 18 );
  c = c - a;  c = c - b;  c = c ^ ( b >> 22 );
}

inline ullong mixInt( ullong a,  ullong b = newhashMagic,
                      ullong c = newhashMagic ) {
  newhash_mix( a, b, c );
  return c;
}

}

#if 0
namespace Hash128 {

/* RSA hashes */
RAIBASE_DLL_EXP
void md4( const byte *buf,  unsigned int bufLen,  void *digest );

RAIBASE_DLL_EXP
void md5( const byte *buf,  unsigned int bufLen,  void *digest );

/* EU hash: RIPEMD-160 software written by Antoon Bosselaers, 
 * available at http://www.esat.kuleuven.ac.be/~cosicart/ps/AB-9601/ */
RAIBASE_DLL_EXP
void ripemd( const byte *buf,  unsigned int bufLen,  void *digest );

}

namespace Hash160 {

/* NIST hash */
RAIBASE_DLL_EXP
void sha1( const byte *buf,  unsigned int bufLen,  void *digest );

/* EU hash: RIPEMD-160 software written by Antoon Bosselaers, 
 * available at http://www.esat.kuleuven.ac.be/~cosicart/ps/AB-9601/ */
RAIBASE_DLL_EXP
void ripemd( const byte *buf,  unsigned int bufLen,  void *digest );

}

namespace Hash256 {

/* NIST hash */
RAIBASE_DLL_EXP
void sha2( const byte *buf,  unsigned int bufLen,  void *digest );

}

namespace Hash512 {

/* NIST hash */
RAIBASE_DLL_EXP
void sha2( const byte *buf,  unsigned int bufLen,  void *digest );

}

namespace Random {
  /* Bob Jenkins ISAAC prng */
  class RAIBASE_DLL_EXP Isaac {
  public:
    static const unsigned int RANDSIZL = 8;
    static const unsigned int RANDSIZ = 1U << RANDSIZL;
    static const int NOT_INITIALIZED = -1;

  private:
    unsigned int randmem[ RANDSIZ ],
                 randa,
                 randb,
                 randc;
    int          randcnt;

    union {
      byte           randrslb[ RANDSIZ * 4 ];
      unsigned int   randrsli[ RANDSIZ ];
      unsigned short randrsls[ RANDSIZ * 2 ];
      ullong         randrsll[ RANDSIZ / 2 ];
    } u;

    static const unsigned int BYTES_MASK = ( RANDSIZ * 4 ) - 1;
    static const unsigned int INTS_MASK = RANDSIZ - 1;
    static const unsigned int SHORTS_MASK = ( RANDSIZ * 2 ) - 1;
    static const unsigned int LONGS_MASK = ( RANDSIZ / 2 ) - 1;

    void fill( void );
  public:
    Isaac() {
      this->randcnt = NOT_INITIALIZED;
    }
    bool isInitialized( void ) const {
      return this->randcnt != NOT_INITIALIZED;
    }
    void init( const byte *seed = NULL,  unsigned int seedBytes = 0 );

    ullong nextLong( void );

    unsigned int nextInt( void );

    unsigned short nextShort( void );

    byte nextByte( void );

    void nextBytes( byte *buf,  unsigned int bufLen );

    template<class T> void next( T &arg ) {
      if ( sizeof( T ) == sizeof( ullong ) )
        arg = (T) this->nextLong();
      else if ( sizeof( T ) == sizeof( unsigned int ) )
        arg = (T) this->nextInt();
      else if ( sizeof( T ) == sizeof( unsigned short ) )
        arg = (T) this->nextShort();
      else if ( sizeof( T ) == sizeof( byte ) )
        arg = (T) this->nextByte();
      else
        this->nextBytes( (byte *) &arg, sizeof( T ) );
    }
  };
}

struct AES {
  /* Substitution table (S-box).  */
  static const byte Se[ 256 ]; /* encryption */
  /* Transforms precalculated from the S-box */
  static unsigned int Te0[ 256 ], Te1[ 256 ], Te2[ 256 ], Te3[ 256 ],
                      Te4[ 256 ];
  static byte Sd[ 256 ]; /* inversion of the S-box */
  /* Transforms for the decryption side */
  static unsigned int Td0[ 256 ], Td1[ 256 ], Td2[ 256 ], Td3[ 256 ],
                      Td4[ 256 ];
  /* Round constants */
  static const unsigned int rcon[ 30 ];
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  /* if SSE AES instructions */
  static bool hasSSE_AES;
#endif
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  #define AES_SSE_ALIGNMENT __attribute__((aligned (16)))
#else
  #define AES_SSE_ALIGNMENT
#endif
  /* Key schedules */
  unsigned int Ke[ 64 ] AES_SSE_ALIGNMENT, /* 60 is max */
               Kd[ 64 ] AES_SSE_ALIGNMENT;

  /* Nk = number of 32 bit words in key                4=128, 6=192, 8=256
   * Nr = number of rounds,             Nk + 6         10=128, 12=192, 14=256
   * Nw = number of words in schedule,  4 * ( Nr + 1 ) 44=128, 52=192, 60=256 */
  unsigned int Nk, Nr, Nw;

  /* setKey() calls this if it is not initialized */
  static void initTables( void );

  /* set the key, which can be 128, 192, or 256 bits -- returns false on
   * failure to provide the correct number of bits or if a selftest fails */
  bool setKey( const byte *key,  unsigned int keylen );

  /* setKey() calls this to create Ke key encryption schedule */
  void expandKey( const byte *key );

  /* setKey() calls this to create the Kd decryption key schedule */
  void invertKey( void );

  /* En/Decryption uses Ke/Kd to transform exactly 128 bit chunks */
  void encrypt( const byte *pt,  byte *ct );

  void decrypt( const byte *ct,  byte *pt );

  /* 64 bits of nonce for ctr above */
  static const ullong CTR_NONCE = ( (ullong) 0x9e3779b9U << 32 ) |
                                    (ullong) 0x7f4a7c13U;
  /* Xor 128 bits in CTR mode:  out = in ^ encrypt( ctr + nonce )
   * *must* increment ctr after each call, it can't be the same value */
  void encryptCTR( const byte *in,  byte *out,  const ullong ctr,
                   const ullong nonce = CTR_NONCE );
  /* same as above but takes arbitrary length and returns ctr */
  ullong encryptBytesCTR( const byte *in,  byte *out,  unsigned int len,
                          ullong ctr,  ullong nonce );
  /* Test 128 bit keys, 192 bit, 256 bit */
  static bool selftest128( void );

  static bool selftest128_unaligned( void );

  static bool selftest192( void );

  static bool selftest256( void );

  static bool selftestCTR( void );
};
#endif

} // namespace rai

#endif
