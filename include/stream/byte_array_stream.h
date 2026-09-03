/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_stream__byte_array_stream_h__
#define __rai_stream__byte_array_stream_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#ifndef __rai_stream__io_stream_h__
#include "stream/io_stream.h"
#endif

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

namespace rai {

class ByteArrayOutputStream;

class RAIBASE_DLL_EXP ByteArrayInputStream : public InputStream {
  protected:
    const byte * data;
    unsigned int dataOff,
                 dataLen;
    bool         dataIsCopied;

    virtual unsigned int fillBuf( byte *buf,  unsigned int bufLen )
;

  public:
    SYS_OPS( ByteArrayInputStream );

    ByteArrayInputStream( const byte *data,  unsigned int dataLen,
                          bool copyData = false,  bool dataIsAlloced = false )
;
    ByteArrayInputStream( ByteArrayOutputStream &bout );
    virtual ~ByteArrayInputStream();
    virtual bool available( void );
    virtual StreamOffset seekSet( StreamSeekOffset offset,  int whence )
;
};


class RAIBASE_DLL_EXP ByteArrayOutputStream : public OutputStream {
  protected:
    byte       * data;			// data buffer passed in with dataIsAlloced is false, or
                                        // buffer allocated internally when dataIsAlloced is true. 
    unsigned int dataOff,		// Current position in the buffer
                 dataLen,		// Length of data when dataIsAlloced is false,
                                        // When dataIsAlloced is true, mask of 2^n - 1 to realloc on. This defines the 
                                        // minimum size to allocate the first time, and alignes future allocs on 2^n boundy. 
                 allocedLen;		// Length of data when dataIsAlloced is true
    bool         dataIsAlloced;         // True: ByteStreamOutputStream owns data, false: user passed in fixed size buffer.

    virtual unsigned int emptyBuf( const byte *buf,  unsigned int bufLen )
;
  public:
    SYS_OPS( ByteArrayOutputStream );

    // If allocData is false, then data points to a buffer to use, and dataLen is the length
    // of the buffer.
    // When dataIsAlloced is true, mask of 2^n - 1 to realloc on. This defines the 
    // minimum size to allocate the first time, and alignes future allocs on 2^n boundy.
    // If dataLen is zero, then the internal buffer will be allocated to the
    // exact size needed.

    ByteArrayOutputStream( byte *data,  unsigned int dataLen,
                           bool allocData = false );
    virtual ~ByteArrayOutputStream();

    virtual StreamOffset seekSet( StreamSeekOffset offset,  int whence )
;
    unsigned int length( void ) {
      return this->dataOff;
    };

    // Get internal buffer and return size
    unsigned int getData( byte **dataPtr = NULL ) {
      if ( dataPtr != NULL )
        *dataPtr = this->data;
      return this->dataOff;
    };

    // Note that reset does not free allocated data.
    // This will leak memory if allocData is true. Call FREE() on the pointer set
    // by getData().
    void reset( void ) {
      this->flush();
      this->dataOff = 0;
      this->streamOffset = 0;
      if ( this->dataIsAlloced ) {
        this->data = NULL;
      }
    };
};
} // namespace rai

#endif
