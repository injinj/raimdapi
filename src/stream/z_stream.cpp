/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIDOC_DLL_EXP ) && defined( RAI_DLL )
#define RAIDOC_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>
#include <zlib.h>
#ifndef BZ_NO_STDIO
#define BZ_NO_STDIO
#endif
#include <bzlib.h>
#define HAVE_BZLIB
#ifdef HAVE_LZ4
#include <lz4.h>
#endif

#include "base/mem.h"
#include "stream/z_stream.h"
#include "util/hash_util.h"

using namespace rai;

#if defined( __ICC ) && __ICC == 600
  /* disable: invalid type conversion: void * to unsigned long */
  #pragma warning(disable:171)
#endif

ZInputStream::ZInputStream( InputStream *inPtr,  unsigned int zBufLen,
                            unsigned int bufLen,  bool closePipe,
                            StreamOffset streamOffset )
            : InputStream( bufLen, closePipe, streamOffset )
{
  this->inPtr    = inPtr;
  this->inBuf    = NULL;
  this->startPtr = NULL;
  this->endPtr   = NULL;
  this->inBufLen = zBufLen;
  this->zState   = UNCOMPRESS_INIT;
}


ZInputStream::~ZInputStream()
{
}


unsigned int
ZInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  unsigned int bufOff,
               nBytes,
               bytesLeft,
               loopCnt = 0;

  if ( this->zState == UNCOMPRESS_INIT ) {
    this->uncompressInit();

    if ( this->inBufLen == 0 )
      this->inBufLen = InputStream::BUF_LEN;
    MALLOC( this->inBufLen, &this->inBuf );
    this->zState = UNCOMPRESS_RUN;
  }
  /* the condition bufOff < bufLen may be slower than bufOff == 0 but requires
   * the application to handle variable size chunks rather than fixed sizes */
  for ( bufOff = 0; this->zState != UNCOMPRESS_END &&
                    bufOff < bufLen /*bufOff == 0*/; ) {
    /* if in finish state, no more input available */
    if ( this->zState != UNCOMPRESS_FINISH ) {
      bytesLeft = (unsigned int) ( this->endPtr - this->startPtr );
      /* keep compressed buffer full so that unzipper is at full speed */
      if ( bytesLeft <= this->inBufLen / 8 ) {
        if ( bytesLeft > 0 && this->startPtr > this->inBuf )
          ::memmove( this->inBuf, this->startPtr, bytesLeft );
        this->startPtr = this->inBuf;
        this->endPtr   = &this->inBuf[ bytesLeft ];

        nBytes = 0;
        try {
          nBytes = this->inPtr->readBytes( this->endPtr,
                                           this->inBufLen - bytesLeft );
          if ( nBytes == 0 )
            this->zState = UNCOMPRESS_FINISH;
        } catch ( Error e ) {
          if ( e != IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK ) )
            throw e;
          if ( bytesLeft == 0 ) { /* if no more bytes to uncompress */
            if ( bufOff > 0 ) /* return what is in buf */
              break;
            throw e; /* no data available to uncompress, throw wouldblock */
          }
        }
        this->endPtr = &this->endPtr[ nBytes ];
      }
    }
    nBytes  = this->uncompress( &buf[ bufOff ], bufLen - bufOff );
    bufOff += nBytes;
    if ( nBytes == 0 && this->zState == UNCOMPRESS_FINISH && ++loopCnt > 3 )
      throw ZStreamErr::getErr( ZStreamErr::ZINPUT_TRUNCATED );
  }

  /* uncompressor may know when to stop */
  if ( this->zState == UNCOMPRESS_END ) {
    /* if still have data from input and uncompressor discards it */
    if ( this->startPtr != this->endPtr )
      throw ZStreamErr::getErr( ZStreamErr::INPUT_TRUNCATED );
  }

  return bufOff;
}


bool
ZInputStream::available( void ) throw( Error )
{
  if ( this->endPtr > this->startPtr ||
       this->InputStream::available() ||
       this->inPtr->available() )
    return true;
  return false;
}


void
ZInputStream::close( void ) throw( Error )
{
  Error         e2;
  InputStream * in;

  e2 = NULL;

  try {
    this->InputStream::close();
  } catch( Error e ) {
    e2 = e;
  }

  /* close input source */
  if ( this->closePipe ) {
    if ( (in = this->inPtr) != NULL ) {
      this->inPtr = NULL;
      try {
        in->close();
      } catch( Error e ) {
        if ( e2 == NULL )
          e2 = e;
      }
      delete in;
    }
  }

  /* shutdown uncompressor */
  if ( this->zState > UNCOMPRESS_INIT && this->zState < UNCOMPRESS_TERM ) {
    try {
      this->uncompressEnd();
    } catch( Error e ) {
      if ( e2 == NULL )
        e2 = e;
    }
    this->zState = UNCOMPRESS_TERM;
  }

  if ( this->inBuf != NULL ) {
    FREE( this->inBuf );
    this->inBuf    = NULL;
    this->inBufLen = 0;
    this->startPtr = NULL;
    this->endPtr   = NULL;
  }

  if ( e2 != NULL )
    throw e2;
}


ZOutputStream::ZOutputStream( OutputStream *outPtr,  int compressLevel,
                              unsigned int zBufLen,  unsigned int bufLen,
                              bool lineBuffered,  bool closePipe,
                              StreamOffset streamOffset )
             : OutputStream( bufLen, lineBuffered, closePipe, streamOffset )
{
  this->outPtr        = outPtr;
  this->compressLevel = compressLevel;
  this->outBuf        = NULL;
  this->endPtr        = NULL;
  this->outBufLen     = zBufLen;
  this->zState        = COMPRESS_INIT;
}


ZOutputStream::~ZOutputStream()
{
}


void
ZOutputStream::init( void ) throw( Error )
{
  this->compressInit();
  if ( this->outBufLen == 0 )
    this->outBufLen = OutputStream::BUF_LEN;
  MALLOC( this->outBufLen, &this->outBuf );
  this->endPtr = this->outBuf;
  this->zState = COMPRESS_RUN;
}


unsigned int
ZOutputStream::emptyBuf( const byte *buf,  unsigned int bufLen ) throw( Error )
{
  unsigned int bufOff,
               nBytes;

  if ( this->zState == COMPRESS_INIT )
    this->init();

  if ( this->endPtr > this->outBuf ) {
    /* give output stream a chance to throw wouldblock before we consume */
    this->outPtr->writeBytes( this->outBuf, this->endPtr - this->outBuf );
    this->endPtr = this->outBuf;
  }

  if ( this->zState != COMPRESS_RUN ) {
    /* can't compress more after told compressor to quit */
    if ( this->zState > COMPRESS_FLUSH )
      throw ZStreamErr::getErr( ZStreamErr::OUTPUT_TRUNCATED );

    /* still flushing, complete the flush (should only happen when flush()
     * throws wouldblock and program doesn't complete the flush) */
    do {
      this->compress( NULL, 0 );
      /* flush output buffer */
      if ( this->endPtr > this->outBuf ) {
        this->outPtr->writeBytes( this->outBuf, this->endPtr - this->outBuf );
        this->endPtr = this->outBuf;
      }
    } while ( this->zState == COMPRESS_FLUSH );
  }

  for ( bufOff = 0; bufOff < bufLen; ) {
    nBytes  = this->compress( &buf[ bufOff ], bufLen - bufOff );
    bufOff += nBytes;

    if ( this->endPtr > this->outBuf ) {
      nBytes = this->endPtr - this->outBuf;
      /* try flushing output buffer */
      try {
        this->outPtr->writeBytes( this->outBuf, nBytes );
        this->endPtr = this->outBuf;
      } catch( Error e ) {
        /* if wouldblock and some data consumed, return how much */
        if ( bufOff > 0 &&
             e == IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK ) )
          break;
        throw e;
      }
    }
  }

  return bufOff;
}


void
ZOutputStream::flush( void ) throw( Error )
{
  this->OutputStream::flush();
  if ( this->zState == COMPRESS_INIT ) /* if flush doesn't init */
    this->init();
  /* flush out compressor */
  this->zState = COMPRESS_FLUSH;
  do {
    this->compress( NULL, 0 );
    if ( this->endPtr > this->outBuf ) {
      this->outPtr->writeBytes( this->outBuf, this->endPtr - this->outBuf );
      this->endPtr = this->outBuf;
    }
  } while ( this->zState == COMPRESS_FLUSH );

  /* flush out sink */
  this->outPtr->flush();
}


void
ZOutputStream::close( void ) throw( Error )
{
  Error          e2;
  OutputStream * out;

  e2 = NULL;
  if ( this->zState >= COMPRESS_INIT && this->zState < COMPRESS_END ) {
    try {
      this->OutputStream::flush();
      if ( this->zState == COMPRESS_INIT ) /* if flush doesn't init */
        this->init();
      this->zState = COMPRESS_FINISH;
      do {
        this->compress( NULL, 0 );
        if ( this->endPtr > this->outBuf ) {
          this->outPtr->writeBytes( this->outBuf, this->endPtr - this->outBuf );
          this->endPtr = this->outBuf;
        }
      } while ( this->zState == COMPRESS_FINISH );
      this->outPtr->flush();
    } catch( Error e ) {
      e2 = e;
    }
  }

  try {
    this->OutputStream::close();
  } catch( Error e ) {
    if ( e2 == NULL )
      e2 = e;
  }

  if ( this->closePipe ) {
    if ( (out = this->outPtr) != NULL ) {
      this->outPtr = NULL;
      try {
        out->close();
      } catch( Error e ) {
        if ( e2 == NULL )
          e2 = e;
      }
      delete out;
    }
  }

  if ( this->zState > COMPRESS_INIT && this->zState < COMPRESS_TERM ) {
    try {
      this->compressEnd();
    } catch( Error e ) {
      if ( e2 == NULL )
        e2 = e;
    }
    this->zState = COMPRESS_TERM;
  }

  if ( this->outBuf != NULL ) {
    FREE( this->outBuf );
    this->outBuf    = NULL;
    this->outBufLen = 0;
    this->endPtr    = NULL;
  }

  if ( e2 != NULL )
    throw e2;
}

extern "C" {
static void *
zAllocFunc( void *, unsigned int items, unsigned int size )
{
  void * mem;

  try {
    MALLOC( items * size, &mem );
    return mem;
  } catch( ... ) {
    return NULL;
  }
}


static void *
zAllocFunc2( void *, int items, int size )
{
  void * mem;

  try {
    MALLOC( (unsigned) items * (unsigned) size, &mem );
    return mem;
  } catch( ... ) {
    return NULL;
  }
}


static void
zFreeFunc( void *,  void *addr )
{
  FREE( addr );
}
}

namespace rai {
class ZLibInputStream : public ZInputStream {
  private:
    z_stream zStream;

    virtual void uncompressInit( void )                      throw( Error );

    virtual unsigned int uncompress( byte *buf,  unsigned int bufLen )
                                                             throw( Error );
    virtual void uncompressEnd( void )                       throw( Error );
  public:
    SYS_OPS( ZLibInputStream );

    ZLibInputStream( InputStream *inPtr,  unsigned int zBufLen,
                     unsigned int bufLen,  bool closePipe,
                     StreamOffset streamOffset ) :
        ZInputStream( inPtr, zBufLen, bufLen, closePipe, streamOffset )
    { ::memset( &zStream, 0, sizeof( zStream ) ); };
    virtual ~ZLibInputStream();
};
}


ZLibInputStream::~ZLibInputStream()
{
  if ( this->inPtr != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
ZLibInputStream::uncompressInit( void ) throw( Error )
{
  this->zStream.zalloc = zAllocFunc;
  this->zStream.zfree  = zFreeFunc;

  if ( inflateInit( &this->zStream ) != Z_OK )
    throw ZStreamErr::getErr( ZStreamErr::ZLIB_INFLATE_INIT );
}


unsigned int
ZLibInputStream::uncompress( byte *buf,  unsigned int bufLen ) throw( Error )
{
  int status;

  this->zStream.avail_out = bufLen;
  this->zStream.next_out  = buf;

  switch ( this->zState ) {
    case UNCOMPRESS_INIT:
    case UNCOMPRESS_TERM:
    case UNCOMPRESS_END:
    default:
      throw ZStreamErr::getErr( ZStreamErr::BAD_ZLIB_INPUT_STATE );

    case UNCOMPRESS_RUN:
    case UNCOMPRESS_FINISH:
      this->zStream.next_in  = this->startPtr;
      this->zStream.avail_in = this->endPtr - this->startPtr;
      status = inflate( &this->zStream, Z_NO_FLUSH
              /*( this->zState == UNCOMPRESS_RUN ?  Z_NO_FLUSH : Z_FINISH ) */);
      this->startPtr = this->zStream.next_in;

      if ( status == Z_OK || status == Z_STREAM_END ) {
        if ( status == Z_STREAM_END )
          this->zState = UNCOMPRESS_END;
        return bufLen - this->zStream.avail_out;
      }
      throw ZStreamErr::getErr( ZStreamErr::ZLIB_INFLATE );
  }
}


void
ZLibInputStream::uncompressEnd( void ) throw( Error )
{
  if ( inflateEnd( &this->zStream ) != Z_OK )
    throw ZStreamErr::getErr( ZStreamErr::ZLIB_INFLATE_END );
}


namespace rai {
class GZInputStream : public ZInputStream {
  private:
    enum GZState {
      GZ_MAGIC1,
      GZ_MAGIC2,
      GZ_METHOD,
      GZ_FLAGS,
      GZ_TIME1,
      GZ_TIME2,
      GZ_TIME3,
      GZ_TIME4,
      GZ_XFLAGS,
      GZ_OS_CODE,
      GZ_EXTRA_FIELD_LEN1,
      GZ_EXTRA_FIELD_LEN2,
      GZ_EXTRA_FIELD_DATA,
      GZ_ORIG_NAME,
      GZ_COMMENT,
      GZ_HEAD_CRC1,
      GZ_HEAD_CRC2,
      GZ_INFLATE,
      GZ_CHECK_CRC1,
      GZ_CHECK_CRC2,
      GZ_CHECK_CRC3,
      GZ_CHECK_CRC4,
      GZ_CHECK_LEN1,
      GZ_CHECK_LEN2,
      GZ_CHECK_LEN3,
      GZ_CHECK_LEN4,
      GZ_END
    };
    GZState        gzState;
    byte           gzFlags;
    unsigned short gzExtraLen,
                   gzHeadCrc;
    unsigned int   gzCrc,
                   gzLen,
                   gzCrcCheck,
                   gzLenCheck;
    z_stream       zStream;

    void initHeaderState( void );

    virtual void uncompressInit( void )                      throw( Error );

    virtual unsigned int uncompress( byte *buf,  unsigned int bufLen )
                                                             throw( Error );
    virtual void uncompressEnd( void )                       throw( Error );
  public:
    SYS_OPS( GZInputStream );

    GZInputStream( InputStream *inPtr,  unsigned int zBufLen,
                   unsigned int bufLen,  bool closePipe,
                   StreamOffset streamOffset ) :
        ZInputStream( inPtr, zBufLen, bufLen, closePipe, streamOffset )
    { ::memset( &zStream, 0, sizeof( zStream ) ); };
    virtual ~GZInputStream();
};
}


GZInputStream::~GZInputStream()
{
  if ( this->inPtr != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
GZInputStream::uncompressInit( void ) throw( Error )
{
  this->zStream.zalloc = zAllocFunc;
  this->zStream.zfree  = zFreeFunc;
  this->initHeaderState();
  if ( inflateInit2( &this->zStream, -MAX_WBITS ) != Z_OK )
    throw ZStreamErr::getErr( ZStreamErr::ZLIB_INFLATE_INIT );
}


void
GZInputStream::initHeaderState( void )
{
  this->gzState    = GZ_MAGIC1;
  this->gzFlags    = 0;
  this->gzExtraLen = 0;
  this->gzHeadCrc  = 0;
  this->gzCrc      = 0;
  this->gzLen      = 0;
  this->gzCrcCheck = 0;
  this->gzLenCheck = 0;
}


unsigned int
GZInputStream::uncompress( byte *buf,  unsigned int bufLen ) throw( Error )
{
  unsigned int nBytes,
               nBytes2;
  int          status;

  if ( this->zState != UNCOMPRESS_RUN && this->zState != UNCOMPRESS_FINISH )
    throw ZStreamErr::getErr( ZStreamErr::BAD_ZLIB_INPUT_STATE );

  for ( nBytes = 0;; ) {
    if ( this->startPtr == this->endPtr ) {
      if ( this->zState == UNCOMPRESS_FINISH ) {
        if ( this->gzState == GZ_END )
          this->zState = UNCOMPRESS_END;
      }
      return nBytes;
    }
    if ( nBytes == bufLen )
      return nBytes;

    switch ( this->gzState ) {
      case GZ_MAGIC1:
        if ( *this->startPtr++ != 0x1fU )
          throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_MAGIC );
        this->gzState = GZ_MAGIC2;
        break;
      case GZ_MAGIC2:
        if ( *this->startPtr++ != 0x8bU )
          throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_MAGIC );
        this->gzState = GZ_METHOD;
        break;
      case GZ_METHOD:
        if ( *this->startPtr++ != Z_DEFLATED )
          throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_METHOD );
        this->gzState = GZ_FLAGS;
        break;
      case GZ_FLAGS:
        this->gzFlags = *this->startPtr++;
        if ( (this->gzFlags & 0xe0) != 0 ) /* reserved flags */
          throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_FLAGS );
        this->gzState = GZ_TIME1;
        break;
      case GZ_TIME1:
        this->startPtr++;
        this->gzState = GZ_TIME2;
        break;
      case GZ_TIME2:
        this->startPtr++;
        this->gzState = GZ_TIME3;
        break;
      case GZ_TIME3:
        this->startPtr++;
        this->gzState = GZ_TIME4;
        break;
      case GZ_TIME4:
        this->startPtr++;
        this->gzState = GZ_XFLAGS;
        break;
      case GZ_XFLAGS:
        this->startPtr++;
        this->gzState = GZ_OS_CODE;
        break;
      case GZ_OS_CODE:
        this->startPtr++;
        if ( (this->gzFlags & 0x04U) != 0 ) /* extra field present */
          this->gzState = GZ_EXTRA_FIELD_LEN1;
        else
          this->gzState = GZ_ORIG_NAME;
        break;
      case GZ_EXTRA_FIELD_LEN1:
        this->gzExtraLen = (unsigned short) *this->startPtr++;
        this->gzState = GZ_EXTRA_FIELD_LEN2;
        break;
      case GZ_EXTRA_FIELD_LEN2:
        this->gzExtraLen |= (unsigned short) *this->startPtr++ << 8;
        this->gzState = GZ_EXTRA_FIELD_DATA;
        break;
      case GZ_EXTRA_FIELD_DATA:
        if ( this->gzExtraLen == 0 )
          this->gzState = GZ_ORIG_NAME;
        else {
          this->startPtr++;
          this->gzExtraLen--;
        }
        break;
      case GZ_ORIG_NAME:
        if ( (this->gzFlags & 0x08U) != 0 ) /* orig name present */
          if ( *this->startPtr++ != 0 )
            break;
        this->gzState = GZ_COMMENT;
        break;
      case GZ_COMMENT:
        if ( (this->gzFlags & 0x10U) != 0 ) /* comment present */
          if ( *this->startPtr++ != 0 )
            break;
        this->gzState = GZ_HEAD_CRC1;
        break;
      case GZ_HEAD_CRC1:
        if ( (this->gzFlags & 0x02U) != 0 ) /* head crc present */
          this->gzHeadCrc = (unsigned short) *this->startPtr++;
        this->gzState = GZ_HEAD_CRC2;
        break;
      case GZ_HEAD_CRC2:
        if ( (this->gzFlags & 0x02U) != 0 ) /* head crc present */
          this->gzHeadCrc |= (unsigned short) *this->startPtr++ << 8;
        this->gzState = GZ_INFLATE;
        break;
      case GZ_INFLATE:
        this->zStream.next_in   = this->startPtr;
        this->zStream.avail_in  = this->endPtr - this->startPtr;
        this->zStream.avail_out = bufLen - nBytes;
        this->zStream.next_out  = &buf[ nBytes ];

        status = inflate( &this->zStream, Z_NO_FLUSH );
        this->startPtr = this->zStream.next_in;

        /* check if inflate faled */
        if ( status != Z_OK && status != Z_STREAM_END )
          throw ZStreamErr::getErr( ZStreamErr::ZLIB_INFLATE );

        nBytes2  = bufLen - this->zStream.avail_out;
        nBytes2 -= nBytes;
        if ( nBytes2 > 0 ) {
          this->gzCrc  = Hash32::crc( &buf[ nBytes ], nBytes2, this->gzCrc );
          /*Sys::out->printf( "%x\n", this->gzCrc );*/
          /*this->gzCrc  = crc32( this->gzCrc, &buf[ nBytes ], nBytes2 );*/
          this->gzLen += nBytes2;
          nBytes += nBytes2;
        }
        else if ( status == Z_OK ) {
          /* might want to check that inflate() did something
           * if ( this->startPtr != this->endPtr )
           *   throw ... */
          return nBytes; /* no output and no state change */
        }
        if ( status == Z_STREAM_END )
          this->gzState = GZ_CHECK_CRC1;
        break;
      case GZ_CHECK_CRC1:
        this->gzCrcCheck = (unsigned int) *this->startPtr++;
        this->gzState = GZ_CHECK_CRC2;
        break;
      case GZ_CHECK_CRC2:
        this->gzCrcCheck |= (unsigned int) *this->startPtr++ << 8;
        this->gzState = GZ_CHECK_CRC3;
        break;
      case GZ_CHECK_CRC3:
        this->gzCrcCheck |= (unsigned int) *this->startPtr++ << 16;
        this->gzState = GZ_CHECK_CRC4;
        break;
      case GZ_CHECK_CRC4:
        this->gzCrcCheck |= (unsigned int) *this->startPtr++ << 24;
        this->gzState = GZ_CHECK_LEN1;
        break;
      case GZ_CHECK_LEN1:
        this->gzLenCheck = (unsigned int) *this->startPtr++;
        this->gzState = GZ_CHECK_LEN2;
        break;
      case GZ_CHECK_LEN2:
        this->gzLenCheck |= (unsigned int) *this->startPtr++ << 8;
        this->gzState = GZ_CHECK_LEN3;
        break;
      case GZ_CHECK_LEN3:
        this->gzLenCheck |= (unsigned int) *this->startPtr++ << 16;
        this->gzState = GZ_CHECK_LEN4;
        break;
      case GZ_CHECK_LEN4:
        this->gzLenCheck |= (unsigned int) *this->startPtr++ << 24;
        this->gzState = GZ_END;
        if ( this->gzCrcCheck != this->gzCrc )
          throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_CRC );
        if ( this->gzLenCheck != this->gzLen )
          throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_LEN );
        break;
      case GZ_END:
        if ( *this->startPtr == 0x1fU ) { /* start of header concat */
          this->initHeaderState();
          if ( inflateReset( &this->zStream ) != Z_OK )
            throw ZStreamErr::getErr( ZStreamErr::ZLIB_INFLATE_RESET );
          break;
        }
        throw ZStreamErr::getErr( ZStreamErr::INPUT_TRUNCATED );
      default:
        throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_STATE );
    }
  }
}


void
GZInputStream::uncompressEnd( void ) throw( Error )
{
  if ( inflateEnd( &this->zStream ) != Z_OK )
    throw ZStreamErr::getErr( ZStreamErr::ZLIB_INFLATE_END );
#if 0
  /* if crc check at end of stream wasn't decoded, stream may be incorrect */
  if ( this->gzState != GZ_END )
    throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_HEADER );
#endif
}


#ifdef HAVE_BZLIB
namespace rai {
class BZInputStream : public ZInputStream {
  private:
    bz_stream bzStream;

    virtual void uncompressInit( void )                      throw( Error );

    virtual unsigned int uncompress( byte *buf,  unsigned int bufLen )
                                                             throw( Error );
    virtual void uncompressEnd( void )                       throw( Error );
  public:
    SYS_OPS( BZInputStream );

    BZInputStream( InputStream *inPtr,  unsigned int zBufLen,
                   unsigned int bufLen,  bool closePipe,
                   StreamOffset streamOffset ) :
        ZInputStream( inPtr, zBufLen, bufLen, closePipe, streamOffset )
    { ::memset( &bzStream, 0, sizeof( bzStream ) ); };
    virtual ~BZInputStream();
};
}


BZInputStream::~BZInputStream()
{
  if ( this->inPtr != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
BZInputStream::uncompressInit( void ) throw( Error )
{
  this->bzStream.bzalloc = zAllocFunc2;
  this->bzStream.bzfree  = zFreeFunc;

  if ( BZ2_bzDecompressInit( &this->bzStream, 0, 0 ) != BZ_OK )
    throw ZStreamErr::getErr( ZStreamErr::BZLIB_DECOMPRESS_INIT );
}


unsigned int
BZInputStream::uncompress( byte *buf,  unsigned int bufLen ) throw( Error )
{
  int status;

  this->bzStream.avail_out = bufLen;
  this->bzStream.next_out  = (char *) buf;

  switch ( this->zState ) {
    case UNCOMPRESS_INIT:
    case UNCOMPRESS_TERM:
    case UNCOMPRESS_END:
    default:
      throw ZStreamErr::getErr( ZStreamErr::BAD_BZLIB_INPUT_STATE );

    case UNCOMPRESS_RUN:
    case UNCOMPRESS_FINISH:
      this->bzStream.next_in  = (char *) this->startPtr;
      this->bzStream.avail_in = this->endPtr - this->startPtr;
      status = BZ2_bzDecompress( &this->bzStream );
      this->startPtr = (byte *) this->bzStream.next_in;

      if ( status == BZ_OK || status == BZ_STREAM_END ) {
        if ( status == BZ_STREAM_END )
          this->zState = UNCOMPRESS_END;
        return bufLen - this->bzStream.avail_out;
      }
      throw ZStreamErr::getErr( ZStreamErr::BZLIB_DECOMPRESS );
  }
}


void
BZInputStream::uncompressEnd( void ) throw( Error )
{
  if ( BZ2_bzDecompressEnd( &this->bzStream ) != BZ_OK )
    throw ZStreamErr::getErr( ZStreamErr::BZLIB_DECOMPRESS_END );
}
#endif


#ifdef HAVE_LZ4_2
namespace rai {
class LZ4InputStream : public ZInputStream {
  private:
    bz_stream bzStream;

    virtual void uncompressInit( void )                      throw( Error );

    virtual unsigned int uncompress( byte *buf,  unsigned int bufLen )
                                                             throw( Error );
    virtual void uncompressEnd( void )                       throw( Error );
  public:
    SYS_OPS( LZ4InputStream );

    LZ4InputStream( InputStream *inPtr,  unsigned int zBufLen,
                   unsigned int bufLen,  bool closePipe,
                   StreamOffset streamOffset ) :
        ZInputStream( inPtr, zBufLen, bufLen, closePipe, streamOffset )
    { ::memset( &bzStream, 0, sizeof( bzStream ) ); };
    virtual ~LZ4InputStream();
};
}


LZ4InputStream::~LZ4InputStream()
{
  if ( this->inPtr != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
LZ4InputStream::uncompressInit( void ) throw( Error )
{
  LZ4_setStreamDecode( &this->lz4StreamDecode, NULL, 0 );
}


unsigned int
LZ4InputStream::uncompress( byte *buf,  unsigned int bufLen ) throw( Error )
{
  unsigned int avail_out = bufLen,
               chunk_size;
  char       * next_out  = (char *) buf;
  int          status;

  switch ( this->zState ) {
    case UNCOMPRESS_INIT:
    case UNCOMPRESS_TERM:
    case UNCOMPRESS_END:
    default:
      throw ZStreamErr::getErr( ZStreamErr::BAD_LZ4_INPUT_STATE );

    case UNCOMPRESS_RUN:
    case UNCOMPRESS_FINISH:
      next_in  = (char *) this->startPtr;
      avail_in = this->endPtr - this->startPtr;
      if ( avail_in >= 4 ) {
        ::memcpy( &chunk_size, next_in, 4 );
      }
      if ( chunk_size >= avail_in + 4 ) {
        status = LZ4_decompress_safe_continue( &this->lz4StreamDecode,
                             chunk_size, &next_in[ 4 ], next_out, avail_out );
        if ( status > 0 ) {
          avail_in = &avail_in[ 4 + status ];
          return (unsigned int) status;
        }
      }
      break;
  }
}


void
LZ4InputStream::uncompressEnd( void ) throw( Error )
{
}
#endif

namespace rai {
class ZLibOutputStream : public ZOutputStream {
  private:
    z_stream zStream;

    virtual void compressInit( void )                        throw( Error );

    virtual unsigned int compress( const byte *buf,  unsigned int bufLen )
                                                             throw( Error );
    virtual void compressEnd( void )                         throw( Error );
  public:
    SYS_OPS( ZLibOutputStream );

    ZLibOutputStream( OutputStream *outPtr,  int compressLevel,
                      unsigned int zBufLen,  unsigned int bufLen,
                      bool lineBuffered,  bool closePipe,
                      StreamOffset streamOffset ) :
        ZOutputStream( outPtr, compressLevel, zBufLen, bufLen, lineBuffered,
                       closePipe, streamOffset )
    { ::memset( &zStream, 0, sizeof( zStream ) ); };
    virtual ~ZLibOutputStream();
};
}


ZLibOutputStream::~ZLibOutputStream()
{
  if ( this->outPtr != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
ZLibOutputStream::compressInit( void ) throw( Error )
{
  this->zStream.zalloc = zAllocFunc;
  this->zStream.zfree  = zFreeFunc;

  if ( deflateInit( &this->zStream, this->compressLevel ) != Z_OK )
    throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE_INIT );
}


unsigned int
ZLibOutputStream::compress( const byte *buf,  unsigned int bufLen ) throw( Error )
{
  int status;

  this->zStream.next_in   = (byte *) buf;
  this->zStream.avail_in  = bufLen;
  this->zStream.next_out  = this->endPtr;
  this->zStream.avail_out = &this->outBuf[ this->outBufLen ] - this->endPtr;

  switch( this->zState ) {
    case COMPRESS_INIT:
    case COMPRESS_TERM:
    case COMPRESS_END:
    default:
      throw ZStreamErr::getErr( ZStreamErr::BAD_ZLIB_OUTPUT_STATE );

    case COMPRESS_RUN:
      status = deflate( &this->zStream, Z_NO_FLUSH );
      if ( status != Z_OK )
        throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE );
      break;

    case COMPRESS_FLUSH:
      status = deflate( &this->zStream, Z_SYNC_FLUSH );
      if ( status != Z_OK )
        throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE_FLUSH );
      if ( this->zStream.avail_out > 0 )
        this->zState = COMPRESS_RUN;
      break;

    case COMPRESS_FINISH:
      status = deflate( &this->zStream, Z_FINISH );
      if ( status == Z_STREAM_END )
        this->zState = COMPRESS_END;
      else if ( status != Z_OK )
        throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE_FLUSH );
      break;
  }

  this->endPtr = this->zStream.next_out;
  return bufLen - this->zStream.avail_in;
}


void
ZLibOutputStream::compressEnd( void ) throw( Error )
{
  if ( deflateEnd( &this->zStream ) != Z_OK )
    throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE_END );
}


namespace rai {
class GZOutputStream : public ZOutputStream {
  private:
    enum GZState {
      GZ_HEADER,
      GZ_BODY,
      GZ_CRC,
      GZ_LEN,
      GZ_END
    };
    GZState      gzState;
    unsigned int gzHdrOff,
                 gzCrcOff,
                 gzLenOff,
                 gzCrc,
                 gzLen;
    z_stream     zStream;

    void initState( void );

    virtual void compressInit( void )                        throw( Error );

    virtual unsigned int compress( const byte *buf,  unsigned int bufLen )
                                                             throw( Error );
    virtual void compressEnd( void )                         throw( Error );
  public:
    SYS_OPS( GZOutputStream );

    GZOutputStream( OutputStream *outPtr,  int compressLevel,
                    unsigned int zBufLen,  unsigned int bufLen,
                    bool lineBuffered,  bool closePipe,
                    StreamOffset streamOffset ) :
        ZOutputStream( outPtr, compressLevel, zBufLen, bufLen, lineBuffered,
                       closePipe, streamOffset )
    { ::memset( &zStream, 0, sizeof( zStream ) ); };
    virtual ~GZOutputStream();
};
}


GZOutputStream::~GZOutputStream()
{
  if ( this->outPtr != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
GZOutputStream::compressInit( void ) throw( Error )
{
  this->zStream.zalloc = zAllocFunc;
  this->zStream.zfree  = zFreeFunc;
  this->initState();
  if ( deflateInit2( &this->zStream, this->compressLevel,
                     Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY ) != Z_OK )
    throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE_INIT );
}


void
GZOutputStream::initState( void )
{
  this->gzState  = GZ_HEADER;
  this->gzHdrOff = 0;
  this->gzCrcOff = 0;
  this->gzLenOff = 0;
  this->gzCrc    = 0;
  this->gzLen    = 0;
}


unsigned int
GZOutputStream::compress( const byte *buf,  unsigned int bufLen ) throw( Error )
{
  static byte empty_gz_hdr[] = {
    0x1fU, 0x8bU, (byte) Z_DEFLATED, /*flags*/0, /*time*/0,0,0,0, /*xflags*/0,
#if defined( _WIN32 ) || defined( _WIN64 )
    0x0b /* win32 */
#else
    0x03 /* unix */
#endif
  };
  unsigned int len;
  int          status;

  if ( this->zState != COMPRESS_RUN && this->zState != COMPRESS_FLUSH &&
       this->zState != COMPRESS_FINISH )
    throw ZStreamErr::getErr( ZStreamErr::BAD_ZLIB_OUTPUT_STATE );

  this->zStream.next_in   = (byte *) buf;
  this->zStream.avail_in  = bufLen;
  this->zStream.next_out  = this->endPtr;
  this->zStream.avail_out = &this->outBuf[ this->outBufLen ] - this->endPtr;

  switch ( this->gzState ) {
    case GZ_HEADER:
      if ( this->zStream.avail_out > 0 ) {
        len = sizeof( empty_gz_hdr ) - this->gzHdrOff;
        if ( len > this->zStream.avail_out )
          len = this->zStream.avail_out;

        ::memcpy( this->zStream.next_out, &empty_gz_hdr[ this->gzHdrOff ],
                  len );
        this->zStream.next_out   = &this->zStream.next_out[ len ];
        this->zStream.avail_out -= len;
        this->gzHdrOff          += len;

        if ( this->gzHdrOff == sizeof( empty_gz_hdr ) )
          this->gzState = GZ_BODY;
      }
      if ( this->gzState != GZ_BODY )
        break;

    case GZ_BODY:
      if ( this->zStream.avail_out > 0 ) {
        if ( this->zState == COMPRESS_RUN ) {
          status = deflate( &this->zStream, Z_NO_FLUSH );
          if ( status != Z_OK )
            throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE );
        }
        else if ( this->zState == COMPRESS_FLUSH ) {
          status = deflate( &this->zStream, Z_SYNC_FLUSH );
          if ( status != Z_OK )
            throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE_FLUSH );
          if ( this->zStream.avail_out > 0 )
            this->zState = COMPRESS_RUN;
        }
        else { /* COMPRESS_FINISH */
          status = deflate( &this->zStream, Z_FINISH );
          if ( status == Z_STREAM_END )
            this->gzState = GZ_CRC;
          else if ( status != Z_OK )
            throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE_FLUSH );
        }

        len = this->zStream.next_in - buf;
        if ( len > 0 ) {
          this->gzCrc  = Hash32::crc( buf, len, this->gzCrc );
          /*this->gzCrc  = crc32( this->gzCrc, buf, len );*/
          this->gzLen += len;
        }
      }
      if ( this->gzState != GZ_CRC )
        break;

    case GZ_CRC:
      if ( this->zStream.avail_out > 0 ) {
        do {
          *this->zStream.next_out++ = (byte)
                                    ( this->gzCrc >> ( this->gzCrcOff * 8 ) );
          this->zStream.avail_out--;
          this->gzCrcOff++;
        } while ( this->zStream.avail_out > 0 && this->gzCrcOff < 4 );
        if ( this->gzCrcOff < 4 )
          break;
        this->gzState = GZ_LEN;
      }
      if ( this->gzState != GZ_LEN )
        break;

    case GZ_LEN:
      if ( this->zStream.avail_out > 0 ) {
        do {
          *this->zStream.next_out++ = (byte)
                                    ( this->gzLen >> ( this->gzLenOff * 8 ) );
          this->zStream.avail_out--;
          this->gzLenOff++;
        } while ( this->zStream.avail_out > 0 && this->gzLenOff < 4 );
        if ( this->gzLenOff < 4 )
          break;
        this->gzState = GZ_END;
        this->zState  = COMPRESS_END;
      }
      break;

    case GZ_END:
      throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_STATE );
  }

  this->endPtr = this->zStream.next_out;
  return bufLen - this->zStream.avail_in;
}


void
GZOutputStream::compressEnd( void ) throw( Error )
{
  if ( deflateEnd( &this->zStream ) != Z_OK )
    throw ZStreamErr::getErr( ZStreamErr::ZLIB_DEFLATE_END );
  if ( this->gzState != GZ_END )
    throw ZStreamErr::getErr( ZStreamErr::GZ_BAD_STATE );
}


#ifdef HAVE_BZLIB
namespace rai {
class BZOutputStream : public ZOutputStream {
  private:
    bz_stream bzStream;

    virtual void compressInit( void )                        throw( Error );

    virtual unsigned int compress( const byte *buf,  unsigned int bufLen )
                                                             throw( Error );
    virtual void compressEnd( void )                         throw( Error );
  public:
    SYS_OPS( BZOutputStream );

    BZOutputStream( OutputStream *outPtr,  int compressLevel,
                    unsigned int zBufLen,  unsigned int bufLen,
                    bool lineBuffered,  bool closePipe,
                    StreamOffset streamOffset ) :
        ZOutputStream( outPtr, compressLevel, zBufLen, bufLen, lineBuffered,
                       closePipe, streamOffset )
    { ::memset( &bzStream, 0, sizeof( bzStream ) ); };
    virtual ~BZOutputStream();
};
}


BZOutputStream::~BZOutputStream()
{
  if ( this->outPtr != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
BZOutputStream::compressInit( void ) throw( Error )
{
  this->bzStream.bzalloc = zAllocFunc2;
  this->bzStream.bzfree  = zFreeFunc;

  if ( BZ2_bzCompressInit( &this->bzStream, this->compressLevel,0,0 ) != BZ_OK )
    throw ZStreamErr::getErr( ZStreamErr::BZLIB_COMPRESS_INIT );
}


unsigned int
BZOutputStream::compress( const byte *buf,  unsigned int bufLen ) throw( Error )
{
  int status;

  this->bzStream.next_in   = (char *) buf;
  this->bzStream.avail_in  = bufLen;
  this->bzStream.next_out  = (char *) this->endPtr;
  this->bzStream.avail_out = &this->outBuf[ this->outBufLen ] - this->endPtr;

  switch( this->zState ) {
    case COMPRESS_INIT:
    case COMPRESS_TERM:
    case COMPRESS_END:
    default:
      throw ZStreamErr::getErr( ZStreamErr::BAD_BZLIB_OUTPUT_STATE );

    case COMPRESS_RUN:
      status = BZ2_bzCompress( &this->bzStream, BZ_RUN );
      if ( status != BZ_RUN_OK )
        throw ZStreamErr::getErr( ZStreamErr::BZLIB_COMPRESS );
      break;

    case COMPRESS_FLUSH:
      status = BZ2_bzCompress( &this->bzStream, BZ_FLUSH );
      if ( status != BZ_FLUSH_OK && status != BZ_RUN_OK )
        throw ZStreamErr::getErr( ZStreamErr::BZLIB_COMPRESS_FLUSH );
      if ( status == BZ_RUN_OK )
        this->zState = COMPRESS_RUN;
      break;

    case COMPRESS_FINISH:
      status = BZ2_bzCompress( &this->bzStream, BZ_FINISH );
      if ( status == BZ_STREAM_END )
        this->zState = COMPRESS_END;
      else if ( status != BZ_FINISH_OK )
        throw ZStreamErr::getErr( ZStreamErr::BZLIB_COMPRESS_FLUSH );
      break;
  }

  this->endPtr = (byte *) this->bzStream.next_out;
  return bufLen - this->bzStream.avail_in;
}


void
BZOutputStream::compressEnd( void ) throw( Error )
{
  if ( BZ2_bzCompressEnd( &this->bzStream ) != BZ_OK )
    throw ZStreamErr::getErr( ZStreamErr::BZLIB_COMPRESS_END );
}
#endif


#ifdef HAVE_LZ4
namespace rai {
class LZ4OutputStream : public ZOutputStream {
  private:
    LZ4_stream_t lz4Stream;

    virtual void compressInit( void )                        throw( Error );

    virtual unsigned int compress( const byte *buf,  unsigned int bufLen )
                                                             throw( Error );
    virtual void compressEnd( void )                         throw( Error );
  public:
    SYS_OPS( LZ4OutputStream );

    LZ4OutputStream( OutputStream *outPtr,  int compressLevel,
                    unsigned int zBufLen,  unsigned int bufLen,
                    bool lineBuffered,  bool closePipe,
                    StreamOffset streamOffset ) :
        ZOutputStream( outPtr, compressLevel, zBufLen, bufLen, lineBuffered,
                       closePipe, streamOffset )
    { ::memset( &lz4Stream, 0, sizeof( lz4Stream ) ); };
    virtual ~LZ4OutputStream();
};
}


LZ4OutputStream::~LZ4OutputStream()
{
  if ( this->outPtr != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
LZ4OutputStream::compressInit( void ) throw( Error )
{
  LZ4_resetStream( &this->lz4Stream );
}


unsigned int
LZ4OutputStream::compress( const byte *buf,  unsigned int bufLen ) throw( Error )
{
  const char * next_in   = (const char *) buf;
  char       * next_out  = (char *) this->endPtr;
  unsigned int avail_in  = bufLen,
               avail_out = &this->outBuf[ this->outBufLen ] - this->endPtr,
               bound, diff,
               out_bytes = 0;
  int          status;

  switch( this->zState ) {
    case COMPRESS_INIT:
    case COMPRESS_TERM:
    case COMPRESS_END:
    default:
      throw ZStreamErr::getErr( ZStreamErr::BAD_LZ4_OUTPUT_STATE );

    case COMPRESS_RUN:
      for (;;) {
        bound = LZ4_COMPRESSBOUND( avail_in );
        while ( bound + 4 > avail_out ) {
          diff = bound + 5 - avail_out;
          if ( avail_in <= diff ) {
            avail_in = 0;
            break;
          }
          avail_in -= diff;
          bound = LZ4_COMPRESSBOUND( avail_in );
        }
        if ( avail_in == 0 )
          break;
        status = LZ4_compress_fast_continue( &this->lz4Stream,
                              next_in, &next_out[ 4 ],
                              avail_in, avail_out - 4, 1 );
        if ( status < 0 )
          throw ZStreamErr::getErr( ZStreamErr::LZ4_COMPRESS );
        if ( status > 0 ) {
          ::memcpy( next_out, &status, 4 );
          next_out = &next_out[ 4 + status ];
          avail_out -= ( 4 + status );
        }
        bufLen    -= avail_in;
        out_bytes += avail_in;
        if ( bufLen == 0 || out_bytes > bufLen )
          break;
        next_in  = &next_in[ avail_in ];
        avail_in = bufLen;
      }
      break;

    case COMPRESS_FLUSH:
      this->zState = COMPRESS_RUN;
      break;

    case COMPRESS_FINISH:
      if ( avail_out >= 4 ) {
        ::memset( next_out, 0, 4 );
        next_out = &next_out[ 4 ];
        avail_out -= 4;
        this->zState = COMPRESS_END;
      }
      break;
  }

  this->endPtr = (byte *) next_out;
  return out_bytes;
}


void
LZ4OutputStream::compressEnd( void ) throw( Error )
{
  this->zState = COMPRESS_TERM;
}
#endif

InputStream *
ZInputStream::create( InputStream *inPtr,  ZStreamKind kind,
                      unsigned int zBufLen,  unsigned int bufLen,
                      bool closePipe,  StreamOffset streamOffset )
              throw( Error )
{
  if ( kind == ZLIB_STREAM )
    return NEW ZLibInputStream( inPtr, zBufLen, bufLen, closePipe,
                                streamOffset );
  if ( kind == GZIP_STREAM )
    return NEW GZInputStream( inPtr, zBufLen, bufLen, closePipe, streamOffset );
#ifdef HAVE_BZLIB
  if ( kind == BZLIB_STREAM )
    return NEW BZInputStream( inPtr, zBufLen, bufLen, closePipe, streamOffset );
#endif
  throw ZStreamErr::getErr( ZStreamErr::NO_ZSTREAM );
}


OutputStream *
ZOutputStream::create( OutputStream *outPtr,  ZStreamKind kind,
                       int compressLevel,  unsigned int zBufLen,
                       unsigned int bufLen,  bool lineBuffered,
                       bool closePipe,  StreamOffset streamOffset )
               throw( Error )
{
  if ( kind == ZLIB_STREAM )
    return NEW ZLibOutputStream( outPtr, compressLevel, zBufLen, bufLen,
                                 lineBuffered, closePipe, streamOffset );
  if ( kind == GZIP_STREAM )
    return NEW GZOutputStream( outPtr, compressLevel, zBufLen, bufLen,
                               lineBuffered, closePipe, streamOffset );
#ifdef HAVE_BZLIB
  if ( kind == BZLIB_STREAM )
    return NEW BZOutputStream( outPtr, compressLevel, zBufLen, bufLen,
                               lineBuffered, closePipe, streamOffset );
#endif
#ifdef HAVE_LZ4
  if ( kind == LZ4_STREAM )
    return NEW LZ4OutputStream( outPtr, compressLevel, zBufLen, bufLen,
                                lineBuffered, closePipe, streamOffset );
#endif
  throw ZStreamErr::getErr( ZStreamErr::NO_ZSTREAM );
}


Error
ZStreamErr::getErr( unsigned int status )
{
  static const char     mod[] = "ZStream";
  static const ErrorRec err[] = {
  /*  0 */ { INPUT_TRUNCATED,        "Bytes after end of zstream read", mod },
  /*  1 */ { OUTPUT_TRUNCATED,       "Bytes written after end of zstream "
                                     "written", mod },
  /*  2 */ { BAD_ZLIB_INPUT_STATE,   "Bad ZLib input/inflate state", mod },
  /*  3 */ { BAD_ZLIB_OUTPUT_STATE,  "Bad ZLib output/deflate state", mod },
  /*  4 */ { BAD_BZLIB_INPUT_STATE,  "Bad BZLib input/decompress state", mod },
  /*  5 */ { BAD_BZLIB_OUTPUT_STATE, "Bad BZLib output/compress state", mod },
  /*  6 */ { ZLIB_INFLATE_INIT,      "ZLib inflate init failed", mod },
  /*  7 */ { ZLIB_INFLATE_RESET,     "ZLib inflate reset failed", mod },
  /*  8 */ { ZLIB_INFLATE,           "ZLib inflate failed", mod },
  /*  9 */ { ZLIB_INFLATE_END,       "ZLib inflate end failed", mod },
  /* 10 */ { ZLIB_DEFLATE_INIT,      "ZLib deflate init failed", mod },
  /* 11 */ { ZLIB_DEFLATE,           "ZLib deflate failed", mod },
  /* 12 */ { ZLIB_DEFLATE_FLUSH,     "ZLib deflate flush failed", mod },
  /* 13 */ { ZLIB_DEFLATE_FINISH,    "ZLib deflate finish failed", mod },
  /* 14 */ { ZLIB_DEFLATE_END,       "ZLib deflate end failed", mod },
  /* 15 */ { BZLIB_DECOMPRESS_INIT,  "BZLib decompress init failed", mod },
  /* 16 */ { BZLIB_DECOMPRESS,       "BZLib decompress failed", mod },
  /* 17 */ { BZLIB_DECOMPRESS_END,   "BZLib decompress end failed", mod },
  /* 18 */ { BZLIB_COMPRESS_INIT,    "BZLib compress init failed", mod },
  /* 19 */ { BZLIB_COMPRESS,         "BZLib compress failed", mod },
  /* 20 */ { BZLIB_COMPRESS_FLUSH,   "BZLib compress flush failed", mod },
  /* 21 */ { BZLIB_COMPRESS_FINISH,  "BZLib compress finish failed", mod },
  /* 22 */ { BZLIB_COMPRESS_END,     "BZLib compress end failed", mod },
  /* 23 */ { GZ_BAD_HEADER,          "Bad gzip stream header", mod },
  /* 24 */ { GZ_BAD_STATE,           "Bad gzip stream header state", mod },
  /* 25 */ { GZ_BAD_MAGIC,           "Gzip magic number not found in stream "
                                     "header", mod },
  /* 26 */ { GZ_BAD_METHOD,          "Unknown gzip compression method", mod },
  /* 27 */ { GZ_BAD_FLAGS,           "Gzip stream header flags set to an "
                                     "invalid value", mod },
  /* 28 */ { GZ_BAD_CRC,             "Gzip stream crc32 check failed", mod },
  /* 29 */ { GZ_BAD_LEN,             "Gzip stream length check failed", mod },
  /* 30 */ { NO_ZSTREAM,             "Unable create zstream of that type",mod },
  /* 31 */ { LZ4_COMPRESS,           "Error compressing LZ4 stream",mod },
  /* 32 */ { BAD_LZ4_OUTPUT_STATE,   "Bad LZ4 stream state",mod },
  /* 33 */ { ZINPUT_TRUNCATED,       "Input trunc, not enough bytes "
                                     "in zstream", mod },
  /* 34 */ { 34,                     "Unknown zstream error", mod },
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}

