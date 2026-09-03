/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_stream__io_stream_h__
#define __rai_stream__io_stream_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

#include <stdarg.h> /* va_list */

namespace rai {

class Mutex;

typedef ullong StreamOffset;
typedef llong  StreamSeekOffset;

namespace IOStream {
  enum Whence {
    IO_SEEK_SET = 0,
    IO_SEEK_CUR = 1,
    IO_SEEK_END = 2
  };
}

class RAIBASE_DLL_EXP InputStream {
  public:
    static const unsigned int BUF_LEN = 1024;

  protected:
    byte       * buf;
    unsigned int offset,
                 length,
                 bufLen;
    StreamOffset streamOffset;
    bool         endOfFile,
                 closePipe;
    Mutex      * lock;

    virtual unsigned int fillBuf( byte *buf,  unsigned int bufLen )
 = 0;
  public:
    InputStream( unsigned int bufLen       = BUF_LEN,
                 bool closePipe            = false,
                 StreamOffset streamOffset = (StreamOffset) 0UL );
    virtual ~InputStream();

    virtual void close( void );

    void initThreadAccess( bool multithreaded );

    unsigned int gets( char *line,  unsigned int nBytes );

    unsigned int readBytes( byte *data,  unsigned int nBytes );

    virtual bool available( void );

    bool available( unsigned int nBytes ) {
      return this->offset + nBytes <= this->length;
    };

    virtual bool isEof( void );

    virtual StreamOffset seekSet( StreamSeekOffset offset,  int whence )
;
    StreamOffset getStreamOffset( void );
};

class RAIBASE_DLL_EXP OutputStream {
  public:
    static const unsigned int BUF_LEN = 1024;

  protected:
    byte       * buf;
    unsigned int offset,
                 bufLen;
    StreamOffset streamOffset;
    bool         closePipe,
                 endOfFile,
                 lineBuffered;
    Mutex      * lock;

    void tryEmptyLines( void );

    void doEmptyBuf( void );

    unsigned int doEmptyBuf2( const byte *buf2,  unsigned int bufLen2 )
;
    virtual unsigned int emptyBuf( const byte *buf,  unsigned int bufLen )
 = 0;
    /* this function does emptyBuf() by default, could be overriden */
    virtual unsigned int emptyBuf2( const byte *buf,  unsigned int bufLen,
                                    const byte *buf2,  unsigned int bufLen2 )
;
  public:
    OutputStream( unsigned int bufLen       = BUF_LEN,
                  bool lineBuffered         = false,
                  bool closePipe            = false,
                  StreamOffset streamOffset = (StreamOffset) 0UL );
    virtual ~OutputStream();

    virtual void close( void );

    void initThreadAccess( bool multithreaded );

    unsigned int puts( const char *s );

    unsigned int printf( const char *fmt,  ... )
#if defined( __GNUC__ )
      __attribute__((format(printf,2,3)));
#else
      ;
#endif
    unsigned int vprintf( const char *fmt,  va_list ap );

    unsigned int writeBytes( const byte *data,  unsigned int nBytes )
;
    virtual void flush( void );

    virtual StreamOffset seekSet( StreamSeekOffset offset,  int whence )
;
    StreamOffset getStreamOffset( void );

    unsigned int getBufLen( void ) { return this->bufLen; };

    unsigned int getBufOffset( void ) { return this->offset; };

    void setLineBuffered( bool setting = true ) { this->lineBuffered = setting;}
};


namespace IOStreamErr {
  enum {
    NOT_SEEKABLE     = 0,
    WOULD_BLOCK      = 1,
    MUST_BE_BUFFERED = 2,
    NOT_OPEN         = 3,
    BAD_SEEK         = 4,
    BUF_OVERFLOW     = 5,
    BROKEN_PIPE      = 6,
    INTERRUPTED      = 7,
    BAD_WHENCE       = 8
  };
  RAIBASE_DLL_EXP
  Error getErr( unsigned int status );
}
} // namespace rai

#endif
