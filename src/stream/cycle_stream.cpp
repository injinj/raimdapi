/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "stream/cycle_stream.h"

using namespace rai;

CycleOutputStream::CycleOutputStream( byte *data,  unsigned int dataLen )
                 : OutputStream( 0U, false, false, (StreamOffset) 0UL )
{
  this->reset( data, dataLen );
}


void
CycleOutputStream::reset( byte *data,  unsigned int dataLen )
{
  this->dataStart = 0;
  this->dataUsed  = 0;
  this->dataLen   = dataLen;
  this->data      = data;
}


unsigned int
CycleOutputStream::emptyBuf( const byte *buf,  unsigned int bufLen )
                   throw( Error )
{
  unsigned int len, nBytes = bufLen;

  if ( this->dataUsed < this->dataLen ) {
    len = this->dataLen - this->dataUsed;
    if ( len > bufLen )
      len = bufLen;
    ::memcpy( &this->data[ this->dataUsed ], buf, len );
    this->dataUsed += len;
    buf     = &buf[ len ];
    bufLen -= len;
  }
  while ( bufLen > 0 ) {
    len = this->dataLen - this->dataStart;
    if ( len > bufLen )
      len = bufLen;
    ::memcpy( &this->data[ this->dataStart ], buf, len );
    buf     = &buf[ len ];
    bufLen -= len;
    if ( (this->dataStart += len) == this->dataLen )
      this->dataStart = 0;
  }

  return nBytes;
}


unsigned int
CycleOutputStream::copyTo( byte *toBuf,  unsigned int bufLen ) const
{
  unsigned int len, nBytes = 0;

  len = this->dataLen - this->dataStart;
  if ( len > this->dataUsed )
    len = this->dataUsed;
  if ( len > bufLen )
    len = bufLen;
  ::memcpy( toBuf, &this->data[ this->dataStart ], len );
  nBytes += len;
  bufLen -= len;

  if ( bufLen > 0 && nBytes < this->dataUsed ) {
    toBuf = &toBuf[ len ];
    len = this->dataUsed - nBytes;
    if ( len > bufLen )
      len = bufLen;
    ::memcpy( toBuf, this->data, len );
    nBytes += len;
  }

  return nBytes;
}

