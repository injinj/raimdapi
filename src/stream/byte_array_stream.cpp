/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "stream/byte_array_stream.h"

using namespace rai;

ByteArrayInputStream::ByteArrayInputStream( const byte *data,
                                            unsigned int dataLen,
                                            bool copyData,
                                           bool dataIsAlloced ) :
                      InputStream( 0U, false, (StreamOffset) 0UL )
{
  this->data         = NULL;
  this->dataOff      = 0;
  this->dataLen      = dataLen;
  this->dataIsCopied = copyData || dataIsAlloced;

  if ( copyData ) {
    if ( dataLen > 0 ) {
      MALLOC( dataLen, &this->data );
      ::memcpy( (byte *) this->data, data, dataLen );
    }
  }
  else {
    this->data = data;
  }
}


ByteArrayInputStream::ByteArrayInputStream( ByteArrayOutputStream &bout )
 : InputStream( 0U, false, (StreamOffset) 0 )
{
  unsigned int n;
  byte       * buf;

  this->data    = NULL;
  this->dataOff = 0;
  this->dataLen = 0;
  this->dataIsCopied = true;
  if ( (n = bout.length()) > 0 ) {
    byte *ptr;
    MALLOC( bout.getData( &ptr ), &buf );
    ::memcpy( buf, ptr, n );
    this->data    = buf;
    this->dataLen = n;
  }
}


ByteArrayInputStream::~ByteArrayInputStream()
{
  if ( this->dataIsCopied ) {
    if ( this->data != NULL ) {
      FREE( (byte *) this->data );
      this->data = NULL;
    }
  }
}


bool
ByteArrayInputStream::available( void )
{
  if ( this->dataOff < this->dataLen || this->InputStream::available() )
    return true;
  return false;
}


unsigned int
ByteArrayInputStream::fillBuf( byte *buf,  unsigned int bufLen )
{
  if ( bufLen > this->dataLen - this->dataOff )
    bufLen = this->dataLen - this->dataOff;

  memcpy( buf, &this->data[ this->dataOff ], bufLen );
  this->dataOff += bufLen;

  return bufLen;
}


StreamOffset
ByteArrayInputStream::seekSet( StreamSeekOffset offset,  int whence )

{
  switch( whence ) {
    case IOStream::IO_SEEK_SET:
      break;
    case IOStream::IO_SEEK_END:
      offset += (StreamSeekOffset) this->dataLen;
      break;
    case IOStream::IO_SEEK_CUR:
      offset += (StreamSeekOffset) this->getStreamOffset();
      break;
  }

  if ( (unsigned int) offset > this->dataLen )
    throw IOStreamErr::getErr( IOStreamErr::BAD_SEEK );

  this->streamOffset = (StreamOffset) offset;
  this->offset       = 0;
  this->length       = 0;
  this->endOfFile    = false;
  this->dataOff      = (unsigned int) offset;

  return this->streamOffset;
}


ByteArrayOutputStream::ByteArrayOutputStream( byte *data,  unsigned int dataLen,
                                              bool allocData ) :
                       OutputStream( 0U, false, false, (StreamOffset) 0UL )
{
  unsigned int i;

  this->dataOff       = 0;
  this->allocedLen    = 0;
  this->dataIsAlloced = allocData;

  if ( allocData ) {
    for ( i = 0; ( 1U << i ) < dataLen; i++ )
      ;
    this->dataLen = ( 1U << i ) - 1;
    this->data    = NULL;
  }
  else {
    this->dataLen = dataLen;
    this->data    = data;
  }
}


ByteArrayOutputStream::~ByteArrayOutputStream()
{
  if ( this->dataIsAlloced ) {
    if ( this->data != NULL ) {
      FREE( this->data );
      this->data = NULL;
    }
  }
}


unsigned int
ByteArrayOutputStream::emptyBuf( const byte *buf,  unsigned int bufLen )

{
  unsigned int newLen;
  int          status;

  if ( bufLen == 0 )
    return 0;

  status = 0;

  if ( this->dataIsAlloced ) {
    newLen     = ( ( this->dataOff + bufLen ) | this->dataLen ) + 1;
    if ( newLen > this->allocedLen || this->data == NULL ) {
      this->allocedLen = newLen;
      REALLOC( newLen, &this->data );
    }
  }
  else {
    if ( this->dataOff + bufLen > this->dataLen ) {
      bufLen = this->dataLen - this->dataOff;
      status = IOStreamErr::BUF_OVERFLOW;
    }
  }

  memcpy( &this->data[ this->dataOff ], buf, bufLen );
  this->dataOff += bufLen;

  if ( status != 0 )
    throw IOStreamErr::getErr( status );

  return bufLen;
}


StreamOffset
ByteArrayOutputStream::seekSet( StreamSeekOffset offset,  int whence )

{
  this->flush();

  switch ( whence ) {
    case IOStream::IO_SEEK_SET:
      break;
    case IOStream::IO_SEEK_END:
      if ( this->dataIsAlloced )
        offset += (StreamSeekOffset) this->dataOff;
      else
        offset += (StreamSeekOffset) this->dataLen;
      break;
    case IOStream::IO_SEEK_CUR:
      offset += (StreamSeekOffset) this->getStreamOffset();
      break;
  }

  if ( this->dataIsAlloced ) {
    // allow seeking up to end of currently allocated buffer, even though
    // complete buffer may not have been written to. This allows caller to seek back
    // to beginning of buffer, followed by a seek to the end. AH
    if ( (unsigned int) offset >= this->allocedLen )
      throw IOStreamErr::getErr( IOStreamErr::BAD_SEEK );
  }
  else if ( (unsigned int) offset > this->dataLen ) {
    throw IOStreamErr::getErr( IOStreamErr::BAD_SEEK );
  }

  this->dataOff      = (unsigned int) offset;
  this->streamOffset = (StreamOffset) offset;

  return this->streamOffset;
}

