/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#include <string.h>

#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include "base/sys.h"
#include "stream/io_stream.h"
#include "util/snprintf.h"
#include "base/thread.h"

using namespace rai;

InputStream::InputStream( unsigned int bufLen,  bool closePipe,
                          StreamOffset streamOffset )
{
  this->buf          = NULL;
  this->offset       = 0;
  this->length       = 0;
  this->bufLen       = bufLen; /* > 0 for buffered stream, 0 for unbuffered */
  this->endOfFile    = false;
  this->closePipe    = closePipe;
  this->streamOffset = streamOffset;
  this->lock         = NULL;
}


InputStream::~InputStream( void )
{
  if ( this->buf != NULL ) {
    try {
      this->close();
    } catch( Error ) {
    }
  }
  if ( this->lock != NULL )
    delete this->lock;
}


void
InputStream::close( void )
{
  byte * buf;

  if ( this->lock != NULL )
    this->lock->lock();

  buf                = this->buf;
  this->buf          = NULL;
  this->offset       = 0;
  this->length       = 0;
  this->streamOffset = 0;
  this->endOfFile    = true;

  if ( this->lock != NULL )
    this->lock->unlock();

  if ( buf != NULL )
    FREE( buf );
}


bool
InputStream::isEof( void )
{
  bool eof;

  if ( this->lock != NULL )
    this->lock->lock();

  eof = this->endOfFile;

  if ( this->lock != NULL )
    this->lock->unlock();

  return eof;
}


void
InputStream::initThreadAccess( bool multithreaded )
{
  if ( multithreaded ) {
    if ( this->lock == NULL ) {
      this->lock = Mutex::create( Mutex::RECURSIVE_LOCK );
    }
  }
  else {
    if ( this->lock != NULL ) {
      delete this->lock;
      this->lock = NULL;
    }
  }
}


StreamOffset
InputStream::seekSet( StreamSeekOffset,  int )
{
  throw IOStreamErr::getErr( IOStreamErr::NOT_SEEKABLE );
}


StreamOffset
InputStream::getStreamOffset( void )
{
  StreamOffset off;

  if ( this->lock != NULL )
    this->lock->lock();

  off = this->streamOffset - (StreamOffset) ( this->length - this->offset );

  if ( this->lock != NULL )
    this->lock->unlock();

  return off;
}


bool
InputStream::available( void )
{
  bool haveBufferedData;

  if ( this->lock != NULL )
    this->lock->lock();

  /* if data in buffer available to be read */
  if ( this->offset < this->length )
    haveBufferedData = true;
  else
    haveBufferedData = false;

  if ( this->lock != NULL )
    this->lock->unlock();

  return haveBufferedData;
}


unsigned int
InputStream::gets( char *line,  unsigned int nBytes )
{
  unsigned int i,
               maxBytes,
               inputOff,
               inputLen,
               count;
  const char * ptr,
             * ptr2;
  Error        e2;

  if ( this->lock != NULL )
    this->lock->lock();

  if( this->endOfFile && this->available() )
    this->endOfFile = false;

  if ( this->endOfFile || nBytes == 0 ) {
    if ( this->lock != NULL )
      this->lock->unlock();
    return 0;
  }

  /* leave room for nul termination */
  maxBytes = nBytes - 1;
  e2 = NULL;

  try {
    /* unbuffered stream */
    if ( this->bufLen == 0 ) {
      try {
        for ( i = 0; i < maxBytes; ) {
          if ( this->fillBuf( (byte *) &line[ i ], 1U ) == 0 )
            break;
          if ( line[ i++ ] == '\n' )
            break;
        }
      } catch( Error e ) {
        /* can't put back data we already have because we have no buffer */
        if ( e == IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK ) )
          e2 = IOStreamErr::getErr( IOStreamErr::MUST_BE_BUFFERED );
        else
          e2 = e;
        goto got_exception;
      }
    }
    /* buffered stream */
    else {
      inputOff = this->offset;
      inputLen = this->length;

      for ( i = 0; i < maxBytes; ) {
        /* if no more bytes in buffer */
        if ( (count = inputLen - inputOff) == 0 ) {
          /* alloc buf */
          if ( this->buf == NULL ) {
            MALLOC( this->bufLen, &this->buf );
          }

          /* get more data */
          try {
            inputLen = this->fillBuf( this->buf, this->bufLen );
          } catch( Error e ) {

            if ( i > this->bufLen ) {
              /* need more space to put line back */
              REALLOC( i, &this->buf );
              this->bufLen = i;
            }

            /* put everything back */
            ::memcpy( this->buf, line, i );
            this->length = i;
            this->offset = 0;

            e2 = e;
            goto got_exception;
          }

          /* check if end of file */
          if ( inputLen == 0 ) {
            this->endOfFile = true;
            break;
          }

          this->streamOffset += inputLen;
          inputOff = 0;
          count = inputLen;
        }

        if ( count > maxBytes - i )
          count = maxBytes - i;
        ptr2 = (const char *) &this->buf[ inputOff ];

        if ( (ptr = (const char *) ::memchr( ptr2, '\n', count )) != NULL ) {
          count = ( ptr + 1 ) - ptr2;

          ::memcpy( &line[ i ], ptr2, count );
          i        += count;
          inputOff += count;
          break;
        }
        ::memcpy( &line[ i ], ptr2, count );
        i        += count;
        inputOff += count;
        /* copy next char */
        /*if ( (line[ i++ ] = (char) this->buf[ inputOff++ ]) == '\n' )
          break;*/
      }

      this->offset = inputOff;
      this->length = inputLen;
    }

    line[ i ] = '\0';
  } catch ( Error e ) {
    e2 = e;
  }
got_exception:;
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;

  return i;
}


unsigned int
InputStream::readBytes( byte *data,  unsigned int nBytes )
{
  unsigned int needBytes,
               dataOff,
               inputOff,
               inputLen,
               bytesRead;
  Error        e2;

  if ( this->lock != NULL )
    this->lock->lock();

  if( this->endOfFile && this->available() )
    this->endOfFile = false;

  if ( this->endOfFile || nBytes == 0 ) {
    if ( this->lock != NULL )
      this->lock->unlock();
    return 0;
  }

  dataOff = 0;
  e2 = NULL;
  try {
    /* unbuffered stream */
    if ( this->bufLen == 0 ) {
      dataOff = this->fillBuf( data, nBytes );
      if ( dataOff == 0 )
        this->endOfFile = true;
      else
        this->streamOffset += dataOff;
    }
    /* buffered */
    else {
      inputOff = this->offset;
      inputLen = this->length;

      if ( inputLen - inputOff >= nBytes ) {
        ::memcpy( data, &this->buf[ inputOff ], nBytes );
        dataOff   = nBytes;
        inputOff += nBytes;
      }
      else {
        try {
          /* if data buffered, copy that first */
          if ( inputOff < inputLen ) {
            dataOff = inputLen - inputOff;
            ::memcpy( data, &this->buf[ inputOff ], dataOff );
            inputOff = inputLen;
          }
          else {
            dataOff = 0;
          }

          needBytes = nBytes - dataOff;

          /* read directly into data buffer if it is bigger than input buffer */
          if ( needBytes >= this->bufLen ) {
            if ( (needBytes = this->fillBuf( &data[ dataOff ], needBytes )) == 0 )
              this->endOfFile = true;
            else {
              this->streamOffset += needBytes;
              dataOff            += needBytes;
            }
          }
          /* fill up input buffer and copy to data buffer */
          else {
            /* allocate space */
            if ( this->buf == NULL ) {
              MALLOC( this->bufLen, &this->buf );
            }

            /* fill up input buffer */
            if ( (bytesRead = this->fillBuf( this->buf, this->bufLen )) == 0 )
              this->endOfFile = true;
            /* copy into data buffer */
            else {
              this->streamOffset += bytesRead;
              inputOff            = 0;
              inputLen            = bytesRead;

              if ( needBytes > inputLen )
                needBytes = inputLen;

              ::memcpy( &data[ dataOff ], this->buf, needBytes );
              dataOff  += needBytes;
              inputOff += needBytes;
            }
          }
        } catch( Error e ) {
          /* if wouldblock and returning some data, clear the error */
          if ( e != IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK ) ||
               dataOff == 0 ) {
            e2 = e;
            goto got_exception;
          }
        }
      }

      this->offset = inputOff;
      this->length = inputLen;
    }
  } catch ( Error e ) {
    e2 = e;
  }
got_exception:;
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;

  return dataOff;
}


OutputStream::OutputStream( unsigned int bufLen,  bool lineBuffered,
                            bool closePipe,  StreamOffset streamOffset )
{
  this->buf          = NULL;
  this->offset       = 0;
  this->bufLen       = bufLen; /* > 0 for buffered stream, 0 for unbuffered */
  this->lineBuffered = lineBuffered;
  this->closePipe    = closePipe;
  this->streamOffset = streamOffset;
  this->endOfFile    = false;
  this->lock         = NULL;
}


/* should be overridden by stream subclasses */
OutputStream::~OutputStream( void )
{
  if ( this->buf != NULL ) {
    try {
      this->close();
    } catch( Error ) {
    }
  }
  if ( this->lock != NULL )
    delete this->lock;
}


void
OutputStream::close( void )
{
  byte * buf;

  if ( this->lock != NULL )
    this->lock->lock();

  buf = this->buf;
  this->buf          = NULL;
  this->offset       = 0;
  this->streamOffset = 0;
  this->bufLen       = 0;
  this->endOfFile    = true;

  if ( this->lock != NULL )
    this->lock->unlock();

  if ( buf != NULL )
    FREE( buf );
}


void
OutputStream::initThreadAccess( bool multithreaded )
{
  if ( multithreaded ) {
    if ( this->lock == NULL ) {
      this->lock = Mutex::create( Mutex::RECURSIVE_LOCK );
    }
  }
  else {
    if ( this->lock != NULL ) {
      delete this->lock;
      this->lock = NULL;
    }
  }
}


StreamOffset
OutputStream::seekSet( StreamSeekOffset,  int )
{
  throw IOStreamErr::getErr( IOStreamErr::NOT_SEEKABLE );
}


StreamOffset
OutputStream::getStreamOffset( void )
{
  StreamOffset off;

  if ( this->lock != NULL )
    this->lock->lock();

  off = streamOffset + (StreamOffset) this->offset;

  if ( this->lock != NULL )
    this->lock->unlock();

  return off;
}


unsigned int
OutputStream::puts( const char *s )
{
  return this->writeBytes( (const byte *) s, ::strlen( s ) );
}


struct VarArgsFormatter {
  rai_vformatter_buff buf;
  char             * bufStart;
  unsigned int       bufIncr,
                     bufLen;
  Error              e;
};

extern "C" {
static int
vformatter_flush( rai_vformatter_buff *fmtBuff )
{
  VarArgsFormatter * vfmt;

  vfmt = (VarArgsFormatter *) fmtBuff;

  /* alloc more buffer space... can't write data because printf needs to be
   * atomic, it can't partially output data... EAGAIN tells the code to 
   * execute until successful */
  try {
    if ( vfmt->bufStart == NULL ) {
      MALLOC( vfmt->bufLen + vfmt->bufIncr, &vfmt->bufStart );
      ::memcpy( vfmt->bufStart, &vfmt->buf.endpos[ -(int) vfmt->bufLen ],
                vfmt->bufLen );
    }
    else {
      REALLOC( vfmt->bufLen + vfmt->bufIncr, &vfmt->bufStart );
    }
  } catch( Error e ) {
    vfmt->e = e;
    return -1;
  }

  /* setup formatter ptrs to the extra space */
  vfmt->buf.curpos = (char *) &vfmt->bufStart[ vfmt->bufLen ];
  vfmt->buf.endpos = (char *) &vfmt->bufStart[ vfmt->bufLen + vfmt->bufIncr ];
  vfmt->bufLen    += vfmt->bufIncr;

  return 0;
} }


unsigned int
OutputStream::vprintf( const char *fmt,  va_list ap )
{
  VarArgsFormatter vfmt;
  char             tmpBuf[ 1024 ];
  unsigned int     n;
  int              nBytes;
  Error            e2;

  if ( this->lock != NULL )
    this->lock->lock();

  if ( this->endOfFile ) {
    if ( this->lock != NULL )
      this->lock->unlock();
    throw IOStreamErr::getErr( IOStreamErr::BROKEN_PIPE );
  }

  nBytes = 0;
  e2 = NULL;
  try {
    if ( this->bufLen > 0 ) {
      /* allocate space for buf */
      if ( this->buf == NULL )
        MALLOC( this->bufLen, &this->buf );
      /* empty buffer if half full, want to keep bufLen from ballooning */
      else if ( this->offset >= this->bufLen / 2 )
        this->doEmptyBuf();

      /* setup callback structure to alloc more buffer space */
      vfmt.buf.curpos = (char *) &this->buf[ this->offset ];
      vfmt.buf.endpos = (char *) &this->buf[ this->bufLen ];
      vfmt.bufStart   = (char *) this->buf;
      vfmt.bufIncr    = ( this->bufLen + 1 ) / 2;
      vfmt.bufLen     = this->bufLen;
    }
    else {
      /* unbuffered... buffer just this printf anyway */
      vfmt.bufStart   = NULL;
      vfmt.buf.curpos = tmpBuf;
      vfmt.buf.endpos = &tmpBuf[ sizeof( tmpBuf ) ];
      vfmt.bufIncr    = 1024;
      vfmt.bufLen     = sizeof( tmpBuf );
    }

    vfmt.e = NULL;

    /* call printf formatter */
    nBytes = rai_vformatter( vformatter_flush, &vfmt.buf, fmt, ap );

    if ( vfmt.e == NULL && nBytes > 0 ) {
      /* if unbuffered, empty temp buf */
      if ( this->bufLen == 0 ) {
        try {
          if ( vfmt.bufStart == NULL )
            n = this->emptyBuf( (byte *) tmpBuf, (unsigned int) nBytes );
          else
            n = this->emptyBuf( (byte *) vfmt.bufStart, (unsigned int) nBytes );
          this->streamOffset += n;
          /* if partial write, can't buffer the rest because we're in unbuffered
           * mode... throw error */
          if ( n != (unsigned int) nBytes )
            vfmt.e = IOStreamErr::getErr( IOStreamErr::MUST_BE_BUFFERED );
        } catch( Error e ) {
          vfmt.e = e;
        }
      }
      /* otherwise empty if needed */
      else {
        this->offset += (unsigned int) nBytes;
        this->bufLen  = vfmt.bufLen;
        this->buf     = (byte *) vfmt.bufStart;

        /* try emptying buffer if over half full */
        try {
          if ( this->offset >= this->bufLen / 2 )
            this->doEmptyBuf();
          else if ( this->lineBuffered )
            this->tryEmptyLines();
        } catch( Error e ) {
          vfmt.e = e;
        }
      }
    }

    if ( vfmt.bufStart != (char *) this->buf )
      FREE( vfmt.bufStart );

    if ( vfmt.e != NULL &&
         vfmt.e != IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK ) ) {
      e2 = vfmt.e;
      goto got_exception;
    }
  } catch ( Error e ) {
    e2 = e;
  }
got_exception:;
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;

  return (unsigned int) nBytes;
}


unsigned int
OutputStream::printf( const char *fmt,  ... )
{
  va_list      ap;
  unsigned int n;
  Error        e2;

  n = 0;
  e2 = NULL;
  va_start( ap, fmt );
  try {
    n = this->vprintf( fmt, ap );
  } catch( Error e ) {
    e2 = e;
  }
  va_end( ap );
  if ( e2 != NULL )
    throw e2;
  return n;
}


unsigned int
OutputStream::writeBytes( const byte *data,  unsigned int nBytes )

{
  unsigned int n,
               dataLeft;
  Error        e2;

  if ( this->lock != NULL )
    this->lock->lock();

  if ( this->endOfFile ) {
    if ( this->lock != NULL )
      this->lock->unlock();
    throw IOStreamErr::getErr( IOStreamErr::BROKEN_PIPE );
  }

  e2 = NULL;
  try {
    /* if no buffering */
    if ( this->bufLen == 0 ) {
      n = this->emptyBuf( data, nBytes );
      this->streamOffset += n;
      /* if can't write atomically and can't buffer the rest, throw error */
      if ( n != nBytes ) {
        e2 = IOStreamErr::getErr( IOStreamErr::MUST_BE_BUFFERED );
        goto got_exception;
      }
    }
    else {
      /* if length of data is more than buf space left, empty buffer */
      if ( this->offset > 0 && nBytes > this->bufLen - this->offset ) {
        /* will throw wouldblock if can't empty all data in this->buf[]
         * otherwise returns the amount of data that was sent */
        n = this->doEmptyBuf2( data, nBytes );
        dataLeft = nBytes - n;
        data     = &data[ n ];
      }
      else {
        dataLeft = nBytes;
      }

      /* try to send big chunks without buffering them, this->buf[] is empty */
      if ( dataLeft >= this->bufLen ) {
        n = this->emptyBuf( data, dataLeft );
        this->streamOffset += n;
        dataLeft -= n;
        data = &data[ n ];
      }

      if ( dataLeft > 0 ) {
        /* partial empty, buffer the unwritten data */
        /* realloc buffer if too much data to fit */
        if ( dataLeft > this->bufLen || this->buf == NULL ) {
          /* can assume this->offset == 0 because the doEmptyBuf above would
           * throw if couldn't empty all data */
          if ( dataLeft > this->bufLen ) {
            REALLOC( dataLeft, &this->buf );
            this->bufLen = dataLeft;
          }
          else {
            REALLOC( this->bufLen, &this->buf );
          }
        }

        /* copy data into buffer */
        ::memcpy( &this->buf[ this->offset ], data, dataLeft );
        this->offset += dataLeft;

        if ( this->lineBuffered )
          this->tryEmptyLines();
      }
    }
  } catch ( Error e ) {
    e2 = e;
  }
got_exception:;
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;

  return nBytes;
}


void
OutputStream::flush( void )
{
  Error e2;

  if ( this->lock != NULL )
    this->lock->lock();

  if ( this->endOfFile ) {
    if ( this->lock != NULL )
      this->lock->unlock();
    if ( this->offset == 0 )
      return;
    throw IOStreamErr::getErr( IOStreamErr::BROKEN_PIPE );
  }

  e2 = NULL;
  try {
    /* don't call flush() from this class unless you really want to flush()
     * the entire stream stack (it's virtual) -- use doEmptyBuf() to flush
     * the buffer */
    this->doEmptyBuf();
  } catch ( Error e ) {
    e2 = e;
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


void
OutputStream::doEmptyBuf( void )
{
  unsigned int n;

  if ( this->offset > 0 ) {
    while ( (n = this->emptyBuf( this->buf, this->offset )) > 0 ) {
      this->streamOffset += n;
      this->offset       -= n;
      if ( this->offset == 0 )
        return;
      ::memmove( this->buf, &this->buf[ n ], this->offset );
    }

    if ( this->offset > 0 ) {
      /* this could be misleading for streams which are intended to block
       * until all data is written.  should maybe have a way to determine
       * whether layer below is in blocking mode */
      throw IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK );
    }
  }
}


unsigned int
OutputStream::emptyBuf2( const byte *buf1,  unsigned int bufLen1,
                         const byte *buf2,  unsigned int bufLen2 )

{
  unsigned int n, off = 0;
  
  try {
    /* this function is virtual, could be overridden to send buf2 as well */
    do {
      n = this->emptyBuf( &buf1[ off ], bufLen1 - off );
      if ( n == 0 )
        return off;
      off += n;
    } while ( off < bufLen1 );
  } catch ( Error e ) {
    if ( e != IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK ) )
      throw e;
  }
  return off;
}


unsigned int
OutputStream::doEmptyBuf2( const byte *buf2,  unsigned int bufLen2 )

{
  unsigned int n;

  if ( this->offset == 0 )
    return 0;

  if ( (n = this->emptyBuf2( this->buf, this->offset, buf2, bufLen2 )) > 0 ) {
    /* return the amount of bufLen2 that was sent */
    if ( n >= this->offset ) {
      this->streamOffset += n;
      n -= this->offset;
      this->offset = 0;
      return n;
    }
    /* not all of this->buf[] was sent */
    this->streamOffset += n;
    this->offset       -= n;
    ::memmove( this->buf, &this->buf[ n ], this->offset );
  }

  throw IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK );
}


void
OutputStream::tryEmptyLines( void )
{
  unsigned int newlineOffset,
               n;
  Error        e2;

  e2 = NULL;
  /* check for newlines and empty them all */
  for ( newlineOffset = this->offset; newlineOffset > 0; ) {
    /* find last newline in buffer */
    if ( this->buf[ --newlineOffset ] == '\n' ) {
      newlineOffset++;

      try {
        n = this->emptyBuf( this->buf, newlineOffset );
        this->streamOffset += n;
        this->offset       -= n;

        /* move partial line to beginning of the buffer */
        if ( this->offset > 0 )
          ::memmove( this->buf, &this->buf[ n ], this->offset );
        return;
      } catch( Error e ) {
        if ( e != IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK ) )
          e2 = e;
        break;
      }
    }
  }
  if ( e2 != NULL )
    throw e2;
}


Error
IOStreamErr::getErr( unsigned int status )
{
  static const char     mod[] = "IOStream";
  static const ErrorRec err[] = {
  /*  0 */ { NOT_SEEKABLE,     "Stream not seekable", mod },
  /*  1 */ { WOULD_BLOCK,      "Stream would block", mod },
  /*  2 */ { MUST_BE_BUFFERED, "Async stream must be buffered in order to be "
                               "atomic", mod },
  /*  3 */ { NOT_OPEN,         "Can't use a stream that's been closed", mod },
  /*  4 */ { BAD_SEEK,         "Stream seek out of range", mod },
  /*  5 */ { BUF_OVERFLOW,     "Stream overflow", mod },
  /*  6 */ { BROKEN_PIPE,      "Can't write to stream that's been closed",
                               mod },
  /*  7 */ { INTERRUPTED,      "Stream interrupted", mod },
  /*  8 */ { BAD_WHENCE,       "Seek whence is invalid", mod },
  /*  9 */ { 9,                "Unknown stream error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}

