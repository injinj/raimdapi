/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_base__mem_h__
#define __rai_base__mem_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

/*#define MEM_SUMMARY*/

/*
 * Declaring "SYS_OPS" in a class:
 * class A {
 *   SYS_OPS( A );
 *   A() {}
 * };
 * Has the effects of
 *
 * If "new A" fails because of out-of-memory, it will throw a RaiException
 * (MemErr::MEM_ALLOC_FAILED), and not std::bad_alloc
 *
 * It also allows tracing memory allocations in debug mode, to make sure memory
 * is released and not corrupted
 *
 * Similarily,
 *
 * MALLOC( sizeof( obj ), &ptr ); // which defined to: Mem::doMalloc( n, &ptr )
 * // or Mem::mallocFLC( n, &ptr, __FILE__, __LINE__, __CLASS__ );
 *
 * Also throws a RaiException, described above
 */

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

#include <stdlib.h>

namespace rai {

class OutputStream;

namespace MemErr {
  enum {
    MEM_ALLOC_FAILED = 0,
    MEM_FREE_FAILED = 1
  };
  RAIBASE_DLL_EXP
  Error getErr( unsigned int );
}

namespace Mem {

//#define MEM_DEBUG
#if defined( MEM_DEBUG ) || defined( MEM_SUMMARY )
void initialize( void );

void terminate( void );

#if defined( MEM_SUMMARY )
void initAllocSummary( void );

void dumpAllocSummary( OutputStream *out );
#endif

/* alloc functions */
/*extern void callocFL( unsigned int nBytes,  void *ptr,  const char *fn,
                      unsigned int ln );*/
extern void mallocFLC( size_t nBytes,  void *ptr,  const char *fileName,
                       unsigned int lineNum,  const char *className )
;
extern void reallocFL( size_t nBytes,  void *ptr,  const char *fileName,
                       unsigned int ln );
extern void freeFLC( void *ptr,  const char *fileName,  unsigned int lineNum,
                     const char *className );
#if defined( MEM_DEBUG )
extern void printAlloced( OutputStream *out );

extern void findMem( void *ptr,  OutputStream *out );

extern void checkMem( void *ptr,  OutputStream *out );

extern void printAllocedSummary( OutputStream *out );
#endif
extern void copyStringFLC( char *&out,  const char *in,  const char *fileName,
                           unsigned int lineNum );

extern bool           traceMemory;    /* if true, debug memory */
extern size_t         memAlloced;     /* counts number of bytes allocated */
extern size_t         blocksAlloced;  /* counts number of chunks allocated */

/* add file:lineno to alloc & frees */
#define MALLOC( N, P )  rai::Mem::mallocFLC( N, P, __FILE__, __LINE__, NULL )
#define REALLOC( N, P ) rai::Mem::reallocFL( N, P, __FILE__, __LINE__ )
#define FREE( P )       rai::Mem::freeFLC( P, __FILE__, __LINE__, NULL )
#define STRDUP( O, I )  rai::Mem::copyStringFLC( O, I, __FILE__, __LINE__ )
#define NEW_SIZE_T size_t

#define SYS_NEW_OPERATOR( CLASS ) \
    void * operator new( NEW_SIZE_T n,  const char *fn,  unsigned int ln ) { \
      void *p; \
      rai::Mem::mallocFLC( n, &p, fn, ln, #CLASS ); \
      return p; \
    }
#define SYS_DELETE_OPERATOR( CLASS ) \
    void operator delete( void *p ) { \
      rai::Mem::freeFLC( p, __FILE__, __LINE__, #CLASS ); \
    }

#define SYS_NEW_ARRAY_OPERATOR( CLASS ) \
    void * operator new[]( NEW_SIZE_T n,  const char *fn,  unsigned int ln ){\
      void *p; \
      rai::Mem::mallocFLC( n, &p, fn, ln, #CLASS ); \
      return p; \
    }
#define SYS_DELETE_ARRAY_OPERATOR( CLASS ) \
    void operator delete[]( void *p ) { \
      rai::Mem::freeFLC( p, __FILE__, __LINE__, #CLASS ); \
    }

#define NEW new( __FILE__, __LINE__ )

#else /* ! MEM_DEBUG */

static inline void initialize( void ) {};

static inline void terminate( void ) {};

static inline void dumpAllocSummary( OutputStream */* out */ ) {};

#define MALLOC( N, P )  rai::Mem::doMalloc( N, P )
#define REALLOC( N, P ) rai::Mem::doRealloc( N, P )
#define FREE( P )       rai::Mem::doFree( P )
#define STRDUP( O, I )  rai::Mem::doCopyString( O, I )
#define NEW new
#define NEW_SIZE_T size_t

#define SYS_NEW_OPERATOR( CLASS ) \
    void * operator new( NEW_SIZE_T n ) { \
      void *p; \
      rai::Mem::doMalloc( n, &p ); \
      return p; \
    }
#define SYS_DELETE_OPERATOR( CLASS ) \
    void operator delete( void *p ) { \
      rai::Mem::doFree( p ); \
    }

#define SYS_NEW_ARRAY_OPERATOR( CLASS ) \
    void * operator new[]( NEW_SIZE_T n ){\
      void *p; \
      rai::Mem::doMalloc( n, &p ); \
      return p; \
    }
#define SYS_DELETE_ARRAY_OPERATOR( CLASS ) \
    void operator delete[]( void *p ) { \
      rai::Mem::doFree( p ); \
    }

RAIBASE_DLL_EXP
void doMalloc( size_t n,  void *p );

RAIBASE_DLL_EXP
void doFree( void *p );

RAIBASE_DLL_EXP
void doRealloc( size_t n,  void *p );

RAIBASE_DLL_EXP
void doCopyString( char *&out,  const char *in );

#endif /* ! MEM_DEBUG */

#define NO_COPY_CONSTRUCTOR( CLASS ) \
    CLASS& operator=( CLASS& ); \
    CLASS( CLASS& )

#define SYS_OPS( CLASS ) \
  private: \
    NO_COPY_CONSTRUCTOR( CLASS ); \
  public: \
    SYS_NEW_OPERATOR( CLASS ); \
    SYS_DELETE_OPERATOR( CLASS )

#define SYS_OPS_AR( CLASS ) \
  private: \
    NO_COPY_CONSTRUCTOR( CLASS ); \
  public: \
    SYS_NEW_ARRAY_OPERATOR( CLASS ); \
    SYS_DELETE_ARRAY_OPERATOR( CLASS )
}
} // namespace rai

#endif
