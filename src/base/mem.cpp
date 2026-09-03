/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "base/mem.h"

using namespace rai;

#if defined( MEM_DEBUG ) || defined( MEM_SUMMARY )
#include "base/sys.h"
#include "stream/io_stream.h"
#include "base/thread.h"
#include "base/log.h"
#include "util/linear_hash_table.h"
#include "util/hash_util.h"

static Mutex * memLock;

struct AllocSumHash  {
  unsigned int  hash;
  AllocSumHash() {};
  AllocSumHash( const char *fileName,  unsigned int lineNum,
                const char *className ) {
    this->set( fileName, lineNum, className );
  };
  void set( const char *fileName, unsigned int lineNum,
            const char *className ) {
    hash = Hash32::newhashs( fileName ) ^ Hash32::hashInt( lineNum );
    /*if ( className != NULL )
      hash ^= Hash32::newhashs( className );*/
  };
  void set( AllocSumHash &h ) {
    hash = h.hash;
  };
};

struct AllocSumEntry {
  const char    * fileName,
                * className;
  long            nBytes;
  unsigned int    lineNum;
  AllocSumHash    allocSumHash;
  
  AllocSumEntry() {
    this->fileName  = NULL;
    this->className = NULL;
    this->lineNum   = 0;
    this->nBytes    = 0;
  };
    
  /*AllocSumEntry( const char *fileName,  unsigned short lineNum,
                 long nBytes,  const char *className ) {
    init( fileName, lineNum, nBytes, className );
  };*/

  void init( const char *fileName,  unsigned int lineNum,  long nBytes,
             const char *className,  AllocSumHash &h ) {
    this->fileName  = fileName;
    this->className = className;
    this->lineNum   = lineNum;
    this->nBytes    = nBytes;
    this->allocSumHash.set( h );
  };
};

class AllocSumTable : public LinearHashTable<AllocSumEntry *, AllocSumHash & > 
{
  virtual AllocSumHash &value( AllocSumEntry *entry ) {
    return entry->allocSumHash;
  };
  virtual unsigned int hash( AllocSumHash &val ) {
    return ( unsigned int ) val.hash;
  };
  virtual bool equals( AllocSumHash &val,  AllocSumEntry *entry ) {
    return val.hash == entry->allocSumHash.hash;
  };
  AllocSumEntry * entryArray,
               ** sortArray;
  unsigned int    maxSize;
  bool            maxSizeExceeded,
                  recursiveInsert;

 public:
  AllocSumTable( unsigned int size )
    : LinearHashTable<AllocSumEntry *, AllocSumHash &>( size * 2 ) {
    // pre-allocate memory for entries so we don't have to alloc later. This allows us
    // to lock when checking allocation blocks
    this->entryArray      = new AllocSumEntry[ size ];
    this->sortArray       = new AllocSumEntry *[ size ];
    this->maxSize         = size;
    this->maxSizeExceeded = false;    
    this->recursiveInsert = false;    
  };

  void init( void ) {
    // Jiggery-Pokery to get Table to actually alloc memory for hash table.
    AllocSumEntry * allocSumEntry = &entryArray[ 0 ];
    AllocSumHash    allocSumHash( "nofile", 1, NULL );
    allocSumEntry->init( "nofile", 1, 1, NULL, allocSumHash );
    this->insert( allocSumEntry );
    this->removeElem( allocSumEntry->allocSumHash );
  };

  static int cmp( AllocSumEntry **e1,  AllocSumEntry **e2 ) {
    return e1[ 0 ]->nBytes - e2[ 0 ]->nBytes;
  };

  void dump( OutputStream * out ) {
    unsigned int i;
    AllocSumEntry * allocSumEntry;
    out->printf( "File name   Line No.   Bytes Allocated.\n");
    if ( maxSizeExceeded ) {
      out->printf( "WARNING: Entries exceeded max size of %u\n", maxSize );
    }
    if ( memLock != NULL )
      memLock->lock();
    try {
      if ( this->tab != NULL && this->elemCount > 0 ) {
        unsigned int count = 0;
        for ( i = 0; i < this->tabSize; i++ ) {
          if ( (this->tab[ i ].hashVal & SLOT_USED) != 0 ) {
            this->sortArray[ count++ ] = this->tab[ i ].elem;
          }
        }
        ::qsort( this->sortArray, count, sizeof( this->sortArray[ 0 ] ),
                 (int (*) (const void *, const void *)) AllocSumTable::cmp );
        for ( i = 0; i < count; i++ ) {
          allocSumEntry = this->sortArray[ i ];
          if ( allocSumEntry->className != NULL ) {
            out->printf( "%s:%u %ld (%s)\n", 
                         allocSumEntry->fileName,
                         allocSumEntry->lineNum,
                         allocSumEntry->nBytes,
                         allocSumEntry->className );
          }
          else {
            out->printf( "%s:%u %ld\n", 
                         allocSumEntry->fileName,
                         allocSumEntry->lineNum,
                         allocSumEntry->nBytes );
          }
        }
      }
    } catch ( ... ) {
    }
    if ( memLock != NULL )
      memLock->unlock();
  };
    
  void update( const char *fileName,  unsigned int lineNum,  long nBytes,
               const char *className ) {
    AllocSumEntry * allocSumEntry;
    AllocSumHash allocSumHash( fileName, lineNum, className );
    if ( memLock != NULL )
      memLock->lock();
    if ( ! this->recursiveInsert ) {
      this->recursiveInsert = true;
      try {
        if ( (allocSumEntry = this->findElem( allocSumHash )) == NULL ) {
          if ( this->elemCount >= this->maxSize ) {
            maxSizeExceeded = true;
          }
          else {
            allocSumEntry = &this->entryArray[ this->elemCount ];
            allocSumEntry->init( fileName, lineNum, nBytes, className,
                                 allocSumHash );
            this->insert( allocSumEntry );
          }
        } else {
          allocSumEntry->nBytes += nBytes;
        }
      } catch ( ... ) {
      }
      this->recursiveInsert = false;
    }
    if ( memLock != NULL )
      memLock->unlock();
  };
  
  void removeAll() {
    unsigned int i;
    if ( this->tab != NULL && this->elemCount > 0 ) {
      for ( i = 0; i < this->tabSize; i++ ) {
        if ( (this->tab[ i ].hashVal & SLOT_USED) != 0 ) {
          this->tab[ i ].hashVal = 0;
          // delete this->tab[ i ].elem;
        }
      }
      this->elemCount = 0;
    }
  };
  
  virtual ~AllocSumTable() {
    removeAll();
    if ( entryArray ) {
      delete[] entryArray;
    }
  };
};
#endif

#if defined( MEM_SUMMARY )
static AllocSumTable *asum = NULL;
void
Mem::initAllocSummary( void ) throw( Error )
{
  asum = new AllocSumTable( 4096 );
  asum->init();
}

void
Mem::dumpAllocSummary( OutputStream *out ) throw( Error )
{
  if ( asum != NULL )
    asum->dump( out );
}
#endif

#if defined( MEM_DEBUG ) || defined( MEM_SUMMARY )

#include <assert.h>

#if defined( __ICC ) && __ICC == 600
  /* disable: invalid type conversion: void * to unsigned long */
  #pragma warning(disable:171)
#endif

#undef malloc
#undef calloc
#undef realloc
#undef free

bool   Mem::traceMemory = true;
size_t Mem::memAlloced;
size_t Mem::blocksAlloced;

void
Mem::initialize( void ) throw( Error )
{
  memLock = Mutex::create( Mutex::RECURSIVE_LOCK );
#if defined( MEM_SUMMARY )
  Mem::initAllocSummary();
#endif
}


void
Mem::terminate( void )
{
  if ( memLock != NULL ) {
    Mutex * lock = memLock;
    memLock = NULL;
    delete lock;
  }
}
#endif

#if defined( MEM_DEBUG )
/**
 * Debugging memallocs
 */

#define MARK          "%%%%"
#define MARK_CHAR     '%'
#define MARK_SZ       4
#define BLOCKS_TBL_SZ 23993


static struct MemTrace {
  void       * ptr,
             * oldPtr;
  size_t       nBytes,
               oldnBytes;
  const char * filename,
             * oldFilename,
             * className,
             * oldClassName;
  unsigned int lineNum,
               oldLineNum;
} **blocks;

static unsigned int nTables;

static inline void
MARK_MEMORY( void *ptr,  size_t nBytes )
{
  ::memset( ptr, MARK_CHAR, nBytes + MARK_SZ );
}

static void
addPtr( void *ptr,  size_t nBytes,  const char *filename,
        unsigned int lineNum,  const char *className )
{
  unsigned int i,
               j,
               h;

  h = ((ulongptr) ptr) % BLOCKS_TBL_SZ;
  i = 0;

  if ( memLock != NULL )
    memLock->lock();

  do {
    for ( j = 0; j < nTables; j++ ) {
      if ( blocks[ j ][ h ].ptr == NULL )
        goto breakLoop;
    }
    h = ( h + 1 ) % BLOCKS_TBL_SZ;
  } while ( ++i < BLOCKS_TBL_SZ );

  if ( blocks == NULL )
    blocks = (MemTrace **) ::malloc( sizeof( blocks[ 0 ] ) * ( nTables + 1 ) );
  else
    blocks = (MemTrace **) ::realloc( blocks, sizeof( blocks[ 0 ] ) *
                                                    ( nTables + 1 ) );
  assert( blocks != NULL );
  blocks[ nTables ] = (MemTrace *) ::calloc( BLOCKS_TBL_SZ,
                                             sizeof( blocks[ 0 ][ 0 ] ) );
  assert( blocks[ nTables ] != NULL );
  j = nTables++;

breakLoop:;
  blocks[ j ][ h ].ptr       = ptr;
  blocks[ j ][ h ].nBytes    = nBytes;
  blocks[ j ][ h ].filename  = filename;
  blocks[ j ][ h ].className = className;
  blocks[ j ][ h ].lineNum   = lineNum;

  Mem::blocksAlloced++;
  Mem::memAlloced += nBytes;

  if ( memLock != NULL )
    memLock->unlock();
}


static bool
removePtr( void *ptr,  size_t *nBytes,  const char *filename,
           unsigned int lineNum,  const char *className )
{
  unsigned int i,
               j,
               h,
               len;

  h = ((ulongptr) ptr) % BLOCKS_TBL_SZ;
  i = 0;

  if ( memLock != NULL )
    memLock->lock();

  do {
    for ( j = 0; j < nTables; j++ ) {
      if ( blocks[ j ][ h ].ptr == ptr ) {
        blocks[ j ][ h ].oldPtr       = ptr;
        blocks[ j ][ h ].ptr          = NULL;
        len                           = blocks[ j ][ h ].nBytes;
        blocks[ j ][ h ].oldnBytes    = len;
        blocks[ j ][ h ].oldFilename  = blocks[ j ][ h ].filename;
        blocks[ j ][ h ].filename     = filename;
        blocks[ j ][ h ].oldLineNum   = blocks[ j ][ h ].lineNum;
        blocks[ j ][ h ].lineNum      = lineNum;
        blocks[ j ][ h ].oldClassName = blocks[ j ][ h ].className;
        blocks[ j ][ h ].className    = className;

        if ( nBytes != NULL )
          *nBytes = len;

        if ( memcmp( &((byte *) ptr)[ len ], MARK, MARK_SZ ) != 0 ) {
          const char * ocn = blocks[ j ][ h ].oldClassName;
          const char * ofn = blocks[ j ][ h ].oldFilename;
          unsigned int oln = blocks[ j ][ h ].oldLineNum;
          if ( memLock != NULL )
            memLock->unlock();
          Error e = MemErr::getErr( MemErr::MEM_FREE_FAILED );
          logError( LERROR, e, "Mem 0x%lx overwrote boundary marker, "
                    "allocated %s/%s:%u, freed %s/%s:%u",
                    (unsigned long) (ulongptr) ptr, ocn, ofn, oln,
                    className, filename, lineNum );
          return false;
        }

        assert( Mem::blocksAlloced >= 1 && Mem::memAlloced >= len );
        Mem::blocksAlloced--;
        Mem::memAlloced -= len;

        if ( memLock != NULL )
          memLock->unlock();
        return true;
      }
    }
    h = ( h + 1 ) % BLOCKS_TBL_SZ;
  } while ( ++i < BLOCKS_TBL_SZ );

  i = 0;
  do {
    for ( j = 0; j < nTables; j++ ) {
      if ( blocks[ j ][ h ].oldPtr == ptr ) {
        const char * ocn = blocks[ j ][ h ].oldClassName;
        const char * ofn = blocks[ j ][ h ].oldFilename;
        unsigned int oln = blocks[ j ][ h ].oldLineNum;
        if ( memLock != NULL )
          memLock->unlock();
        Error e = MemErr::getErr( MemErr::MEM_FREE_FAILED );
        logError( LERROR, e, "Mem 0x%lx freed twice, "
                  "once here %s/%s:%u, and here %s/%s:%u",
                  (unsigned long) (ulongptr) ptr, ocn, ofn, oln,
                  className, filename, lineNum );
        return false;
      }
    }
    h = ( h + 1 ) % BLOCKS_TBL_SZ;
  } while ( ++i < BLOCKS_TBL_SZ );

  if ( memLock != NULL )
    memLock->unlock();

  Error e = MemErr::getErr( MemErr::MEM_FREE_FAILED );
  logError( LERROR, e, "Mem 0x%lx not allocated but freed %s/%s:%u",
            (unsigned long) (ulongptr) ptr, className, filename, lineNum );

  return false;
}


void
Mem::printAlloced( OutputStream *out ) throw( Error )
{
  unsigned int i,
               j,
               k = 0;
  MemTrace     err[ 32 ];

  if ( out == NULL )
    out = Sys::out;

  if ( memLock != NULL )
    memLock->lock();
  i = 0;
  do {
    for ( j = 0; j < nTables; j++ ) {
      if ( blocks[ j ][ i ].ptr != NULL ) {
        void * ptr2;
        size_t len;
        len = blocks[ j ][ i ].nBytes;
        ptr2 = blocks[ j ][ i ].ptr;

        if ( memcmp( &((byte *) ptr2)[ len ], MARK, MARK_SZ ) != 0 ) {
          err[ k++ ] = blocks[ j ][ i ];
          if ( k == sizeof( err ) / sizeof( err[ 0 ] ) )
            goto break_loop;
        }
      }
    }
  } while ( ++i < BLOCKS_TBL_SZ );
break_loop:;
  if ( memLock != NULL )
    memLock->unlock();

  if ( k > 0 ) {
    for ( j = 0; j < k; j++ ) {
      MemTrace b = err[ k ];
      out->printf( "incorrect mark: 0x%lx %lu %s %s:%u\n",
                   (unsigned long) (ulongptr) b.ptr,
                   (unsigned long) b.nBytes,
                   b.className, b.filename, b.lineNum );
      out->flush();
    }
  }

  out->printf( "memAlloced %lu, blocksAlloced %lu\n",
               (unsigned long) Mem::memAlloced,
               (unsigned long) Mem::blocksAlloced );
  i = 0;
  do {
    for ( j = 0; j < nTables; j++ ) {
      if ( blocks[ j ][ i ].ptr != NULL ) {
        out->printf( "0x%lx %lu %s %s:%u\n",
                     (unsigned long) (ulongptr) blocks[ j ][ i ].ptr,
                     (unsigned long) blocks[ j ][ i ].nBytes,
                                     blocks[ j ][ i ].className,
                                     blocks[ j ][ i ].filename,
                                     blocks[ j ][ i ].lineNum );

      }
    }
  } while ( ++i < BLOCKS_TBL_SZ );

  out->flush();
}

void
Mem::printAllocedSummary( OutputStream *out ) throw( Error )
{
  unsigned int i,
               j;

  AllocSumTable * allocSumTable;

  // Pre-allocate enough memory for 2048 entries so that we don't have to 
  // request more memory while we're scaning blocks.
  allocSumTable = new AllocSumTable( 2048 );
  // force it to alloc memory.
  allocSumTable->init();
  if ( out == NULL )
    out = Sys::out;

  out->printf( "memAlloced %lu, blocksAlloced %lu\n",
               (unsigned long) Mem::memAlloced,
               (unsigned long) Mem::blocksAlloced );

  if ( memLock != NULL )
    memLock->lock();

  try {
    i = 0;
    do {
      for ( j = 0; j < nTables; j++ ) {
        if ( blocks[ j ][ i ].ptr != NULL ) {
          allocSumTable->update( blocks[ j ][ i ].filename, 
                                 blocks[ j ][ i ].lineNum,
                                 blocks[ j ][ i ].nBytes,
                                 blocks[ j ][ i ].className );
          
        }
      }
    } while ( ++i < BLOCKS_TBL_SZ );
  } catch( Error e ) {
    logError( LERROR, e, "printAllocedSummary" );
  }

  if ( memLock != NULL )
    memLock->unlock();

  allocSumTable->dump( out );

  out->flush();
  delete allocSumTable;
}

/* stupid gdb */
extern "C"
void
dbgPrintAlloced( OutputStream *out )
{
  Mem::printAlloced( out );
}

extern "C"
void
dbgPrintAllocedSummary( OutputStream *out )
{
  Mem::printAllocedSummary( out );
}

void
Mem::findMem( void *ptr,  OutputStream *out ) throw( Error )
{
  unsigned int i,
               j,
               h;

  if ( out == NULL )
    out = Sys::out;

  h = ((unsigned long) (ulongptr) ptr) % BLOCKS_TBL_SZ;
  i = 0;
  do {
    for ( j = 0; j < nTables; j++ ) {
      if ( blocks[ j ][ h ].ptr != NULL ) {
        if ( (ulongptr) ptr >= (ulongptr) blocks[ j ][ h ].ptr &&
             (ulongptr) ptr < (ulongptr) blocks[ j ][ h ].ptr +
                              (ulongptr) blocks[ j ][ h ].nBytes ) {
          out->printf( "tab[ %u ][ %u ] (0x%lx) = 0x%lx %lu %s %s:%u\n",
                       j, h, (unsigned long) (ulongptr) ptr,
                       (unsigned long) (ulongptr) blocks[ j ][ h ].ptr,
                       (unsigned long) blocks[ j ][ h ].nBytes,
                                       blocks[ j ][ h ].className,
                                       blocks[ j ][ h ].filename,
                                       blocks[ j ][ h ].lineNum );
          out->flush();
          return;
        }
        else 
          if ( (ulongptr) ptr >= (ulongptr) blocks[ j ][ h ].oldPtr &&
               (ulongptr) ptr < (ulongptr) blocks[ j ][ h ].oldPtr +
                                  (ulongptr) blocks[ j ][ h ].oldnBytes ) {
          out->printf( "tab[ %u ][ %u ] (old 0x%lx) = 0x%lx %lu %s %s:%u\n",
                       j, h, (unsigned long) (ulongptr) ptr,
                       (unsigned long) (ulongptr) blocks[ j ][ h ].oldPtr,
                       (unsigned long) blocks[ j ][ h ].oldnBytes,
                                       blocks[ j ][ h ].oldClassName,
                                       blocks[ j ][ h ].oldFilename,
                                       blocks[ j ][ h ].oldLineNum );
          out->flush();
        }
      }
    }
    h = ( h + 1 ) % BLOCKS_TBL_SZ;
  } while ( ++i < BLOCKS_TBL_SZ );
}


void
dbgFindMem( void *ptr,  OutputStream *out )
{
  Mem::findMem( ptr, out );
}


void
Mem::checkMem( void *ptr,  OutputStream *out ) throw( Error )
{
  unsigned int i,
               j,
               h,
               freedj,
               freedh;
  bool         wasFreed;

  if ( out == NULL )
    out = Sys::out;

  if ( memLock != NULL )
    memLock->lock();
  h = ((unsigned long) (ulongptr) ptr) % BLOCKS_TBL_SZ;
  i = 0;
  freedj = 0;
  freedh = 0;
  wasFreed = false;
  do {
    for ( j = 0; j < nTables; j++ ) {
      if ( blocks[ j ][ h ].ptr != NULL ) {
        if ( (ulongptr) ptr >= (ulongptr) blocks[ j ][ h ].ptr &&
             (ulongptr) ptr < (ulongptr) blocks[ j ][ h ].ptr +
                              (ulongptr) blocks[ j ][ h ].nBytes ) {
          void * ptr2;
          size_t len;
          len = blocks[ j ][ h ].nBytes;
          ptr2 = blocks[ j ][ h ].ptr;

          if ( memcmp( &((byte *) ptr2)[ len ], MARK, MARK_SZ ) != 0 ) {
            MemTrace b = blocks[ j ][ h ];
            if ( memLock != NULL )
              memLock->unlock();
            out->printf( "incorrect mark: "
                         "tab[ %u ][ %u ] (0x%lx/0x%lx) = 0x%lx %lu %s %s:%u\n",
                         j, h, (unsigned long) (ulongptr) ptr,
                         (unsigned long) (ulongptr) ptr2,
                         (unsigned long) (ulongptr) b.ptr,
                         (unsigned long) b.nBytes,
                         b.className, b.filename, b.lineNum );
            out->flush();
          }
          else {
            if ( memLock != NULL )
              memLock->unlock();
          }
          return;
        }
        else 
          if ( (ulongptr) ptr >= (ulongptr) blocks[ j ][ h ].oldPtr &&
               (ulongptr) ptr < (ulongptr) blocks[ j ][ h ].oldPtr +
                                (ulongptr) blocks[ j ][ h ].oldnBytes ) {
          freedj = j;
          freedh = h;
          wasFreed = true;
        }
      }
    }
    h = ( h + 1 ) % BLOCKS_TBL_SZ;
  } while ( ++i < BLOCKS_TBL_SZ );

  if ( wasFreed ) {
    MemTrace b = blocks[ j ][ h ];
    if ( memLock != NULL )
      memLock->unlock();
    j = freedj;
    h = freedh;
    out->printf( "is freed: "
                 "tab[ %u ][ %u ] (old 0x%lx) = 0x%lx %lu %s %s:%u\n",
                 j, h, (unsigned long) (ulongptr) ptr,
                 (unsigned long) (ulongptr) b.oldPtr,
                 (unsigned long) b.oldnBytes, b.oldClassName,
                                 b.oldFilename, b.oldLineNum );
    out->flush();
  }
  else {
    if ( memLock != NULL )
      memLock->unlock();
    out->printf( "mem not found (0x%lx)\n", (unsigned long) (ulongptr) ptr );
    out->flush();
  }
}


void
dbgCheckMem( void *ptr,  OutputStream *out ) throw( Error )
{
  Mem::checkMem( ptr, out );
}


/**
 * Memory routines
 */
void
Mem::mallocFLC( size_t nBytes,  void *ptr,  const char *fileName,
                unsigned int lineNum,  const char *className )
     throw( Error )
{
  if ( Mem::traceMemory ) {
    *(void **)ptr = ::malloc( nBytes + MARK_SZ );
    if ( *(void **)ptr == NULL )
      throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );
    MARK_MEMORY( *(void **) ptr, nBytes );
    addPtr( *(void **)ptr, nBytes, fileName, lineNum, className );

    logTrace( LTRACE, "malloc( %lu, %s:%u, %s ) = 0x%lx",
              (unsigned long) nBytes, fileName, lineNum, className,
              (unsigned long) (ulongptr) *(void **) ptr );
  }
  else {
    *(void **)ptr = ::malloc( nBytes );
    if ( *(void **)ptr == NULL )
      throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );
  }
}


void
Mem::reallocFL( size_t nBytes,  void *ptr,  const char *fileName,
                unsigned int lineNum )
     throw( Error )
{
  void       * newPtr,
             * oldPtr;
  size_t       old_nBytes;

  if ( Mem::traceMemory ) {
    oldPtr = *(void **) ptr;
    if ( oldPtr == NULL ) {
      *(void **) ptr = ::malloc( nBytes + MARK_SZ );
      if ( *(void **) ptr == NULL )
        throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );
      MARK_MEMORY( *(void **) ptr, nBytes );
      addPtr( *(void **) ptr, nBytes, fileName, lineNum, NULL );

      logTrace( LTRACE, "realloc( NULL, 0 -> %lu, %s:%u ) = 0x%lx",
                (unsigned long) nBytes, fileName, lineNum,
                (unsigned long) (ulongptr) *(void **) ptr );
      return;
    }

    if ( removePtr( oldPtr, &old_nBytes, fileName, lineNum, NULL ) ) {
      if ( nBytes > old_nBytes || nBytes <= old_nBytes / 2 ) {
        newPtr = ::malloc( nBytes + MARK_SZ );
        if ( newPtr == NULL )
          throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );
        if ( nBytes < old_nBytes ) {
          memcpy( newPtr, oldPtr, nBytes );
          MARK_MEMORY( &((byte *) newPtr)[ nBytes ], 0 );
        }
        else {
          memcpy( newPtr, oldPtr, old_nBytes );
          MARK_MEMORY( &((byte *) newPtr)[ old_nBytes ], nBytes - old_nBytes );
        }
        ::free( oldPtr );
      }
      else {
        newPtr = oldPtr;
        MARK_MEMORY( &((byte *) newPtr)[ nBytes ], old_nBytes - nBytes );
      }

      addPtr( newPtr, nBytes, fileName, lineNum, NULL );

      *(void **)ptr = newPtr;

      logTrace( LTRACE, "realloc( 0x%lx, %lu -> %lu, %s:%u ) = 0x%lx",
                (unsigned long) (ulongptr) oldPtr, (unsigned long) old_nBytes,
                (unsigned long) nBytes, fileName, lineNum,
                (unsigned long) (ulongptr) newPtr );
    }
    else {
      throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );
    }
  }
  else {
    if ( *(void **)ptr == NULL )
      newPtr = ::malloc( nBytes );
    else
      newPtr = ::realloc( *(void **) ptr, nBytes );
    if ( newPtr == NULL )
      throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );

    *(void **)ptr = newPtr;
  }
}


void
Mem::freeFLC( void *ptr,  const char *fileName,  unsigned int lineNum,
              const char *className )
{
  size_t old_nBytes;

  if ( ptr != NULL ) {
    if ( Mem::traceMemory ) {
      if ( removePtr( ptr, &old_nBytes, fileName, lineNum, className ) ) {

        logTrace( LTRACE, "free( 0x%lx, %lu, %s:%u, %s )",
                  (unsigned long) (ulongptr) ptr, (unsigned long) old_nBytes,
                  fileName, lineNum, className );
        ::free( ptr );
      }
    }
    else {
      ::free( ptr );
    }
  }
}


void
Mem::copyStringFLC( char *&out,  const char *in,  const char *fileName,
                    unsigned int lineNum ) throw( Error )
{
  size_t len;

#if 0
  if ( Mem::traceMemory ) {
    if ( in == NULL ) {
      logError( LDEBUG, "copyString( out, 0x0, %s:%u ), null input string",
                fileName, lineNum );
    }
  }
#endif
  if ( in != NULL ) {
    len = ::strlen( in ) + 1;
    Mem::reallocFL( len, &out, fileName, lineNum );
    ::memcpy( out, in, len );
  }
  else if ( out != NULL ) {
    Mem::freeFLC( out, fileName, lineNum, NULL );
    out = NULL;
  }
}
#endif

#if defined( MEM_SUMMARY )
void
Mem::mallocFLC( size_t nBytes,  void *ptr,  const char *fileName,
                unsigned int lineNum,  const char *className )
     throw( Error )
{
  *(void **)ptr = ::malloc( nBytes );
  if ( asum != NULL && *(void **) ptr != NULL )
    asum->update( fileName, lineNum, 1, className );
  if ( *(void **) ptr == NULL )
    throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );
}


void
Mem::reallocFL( size_t nBytes,  void *ptr,  const char *fileName,
                unsigned int lineNum )
     throw( Error )
{
  void * newPtr;

  if ( *(void **) ptr == NULL ) {
    newPtr = ::malloc( nBytes );
    if ( asum != NULL && newPtr != NULL )
      asum->update( fileName, lineNum, 1, NULL );
  }
  else {
    newPtr = ::realloc( *(void **) ptr, nBytes );
  }
  if ( newPtr == NULL )
    throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );

  *(void **) ptr = newPtr;
}


void
Mem::freeFLC( void *ptr,  const char *fileName,  unsigned int lineNum,
              const char *className )
{
  if ( ptr != NULL ) {
    ::free( ptr );
    /*if ( asum != NULL ) {
      asum->update( fileName, lineNum, -1, className );
    }*/
  }
}


void
Mem::copyStringFLC( char *&out,  const char *in,  const char *fileName,
                    unsigned int lineNum ) throw( Error )
{
  size_t len;

  if ( in != NULL ) {
    len = ::strlen( in ) + 1;
    Mem::reallocFL( len, &out, fileName, lineNum );
    ::memcpy( out, in, len );
  }
  else if ( out != NULL ) {
    Mem::freeFLC( out, fileName, lineNum, NULL );
    out = NULL;
  }
}
#endif

#if ! defined( MEM_DEBUG ) && ! defined( MEM_SUMMARY )

extern "C" {
  unsigned int no_mem_debug = 1;
}

void
Mem::doMalloc( size_t n,  void *p ) throw( Error )
{
  if ( ( *(void **) p = malloc( n ) ) == NULL )
    throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );
}

void
Mem::doFree( void *p )
{
  free( p );
}

void
Mem::doRealloc( size_t n,  void *p ) throw( Error )
{
  void * p2;
  p2 = realloc( *(void **) p, n );
  if ( p2 == NULL )
    throw MemErr::getErr( MemErr::MEM_ALLOC_FAILED );
  *(void **) p = p2;
}

void
Mem::doCopyString( char *&out,  const char *in ) throw( Error )
{
  size_t len;

  if ( in != NULL ) {
    len = ::strlen( in ) + 1;
    doRealloc( len, &out );
    ::memcpy( out, in, len );
  }
  else if ( out != NULL ) {
    doFree( out );
    out = NULL;
  }
}

#endif

Error
MemErr::getErr( unsigned int errNum )
{
  static const ErrorRec err = {
    MEM_ALLOC_FAILED, "Memory allocation failed", "Memory" };
  static const ErrorRec err2 = {
    MEM_FREE_FAILED, "Memory free failed", "Memory" };
  if ( errNum == MEM_ALLOC_FAILED )
    return &err;
  return &err2;
}


#if 0
#include <pthread.h>

struct {
  volatile char * fp, * fp2;
  unsigned int pthr, pthr2;
  const char *fn, *fn2;
  unsigned int ln, ln2;
} mestk[ 1024 ];
unsigned int mestk_tos;

void
pushme_fl( const char *fn,  unsigned int ln )
{
  volatile char buf[ 8 ];

  mestk[ mestk_tos ].fp = buf;
  mestk[ mestk_tos ].pthr = pthread_self();
  mestk[ mestk_tos ].fn = fn;
  mestk[ mestk_tos ].ln = ln;
  mestk[ mestk_tos ].fn2 = fn;
  mestk[ mestk_tos ].ln2 = ln;
  mestk[ mestk_tos ].fp2 = buf;
  mestk[ mestk_tos ].pthr2 = mestk[ mestk_tos ].pthr;
  mestk_tos++;
  mestk_tos &= 1023;
}

void
popme_fl( const char *fn,  unsigned int ln )
{
  volatile char buf[ 8 ];

  --mestk_tos;
  mestk_tos &= 1023;
  mestk[ mestk_tos ].fp2 = buf;
  mestk[ mestk_tos ].pthr2 = pthread_self();
  mestk[ mestk_tos ].fn2 = fn;
  mestk[ mestk_tos ].ln2 = ln;
}
#endif
