/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "stream/limit_stream.h"
#include "util/str_util.h"

using namespace rai;

LimitInputStream::LimitInputStream( InputStream *in,  StreamOffset limit,
                                    unsigned int bufLen,  bool closePipe,
                                    StreamOffset streamOffset )
                    : InputStream( bufLen, closePipe, streamOffset )
{
  this->in    = in;
  this->limit = limit;
}


LimitInputStream::~LimitInputStream()
{
  if ( this->in != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
LimitInputStream::close( void ) throw( Error )
{
  Error         e2;
  InputStream * in;

  e2 = NULL;

  try {
    this->InputStream::close();
  } catch( Error e ) {
    e2 = e;
  }

  in = this->in;
  this->in = NULL;

  if ( this->closePipe && in != NULL ) {
    try {
      in->close();
    } catch( Error e ) {
      if ( e2 == NULL )
        e2 = e;
    }
    delete in;
  }

  if ( e2 != NULL )
    throw e2;
}


bool
LimitInputStream::available( void ) throw( Error )
{
  if ( this->in == NULL || this->limit == 0 )
    return false;

  if ( this->InputStream::available() ||
       this->in->available() )
    return true;
  return false;
}


unsigned int
LimitInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  unsigned int nBytes;

  if ( this->in == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

  if ( this->limit == 0 )
    return 0;

  if ( (StreamOffset) bufLen > this->limit )
    bufLen = (unsigned int) this->limit;

  nBytes = this->in->readBytes( buf, bufLen );
  this->limit -= (StreamOffset) nBytes;

  return nBytes;
}

