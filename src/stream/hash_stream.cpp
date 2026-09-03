/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "stream/hash_stream.h"
#include "util/hash_util.h"

using namespace rai;

HashInputStream::HashInputStream( InputStream *in,  HashContext *ctx,
                                  unsigned int bufLen,  bool closePipe,
                                  StreamOffset streamOffset )
                    : InputStream( bufLen, closePipe, streamOffset )
{
  this->in  = in;
  this->ctx = ctx;
  this->ctx->init();
}


HashInputStream::~HashInputStream()
{
  if ( this->in != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
HashInputStream::close( void ) throw( Error )
{
  Error         e2;
  InputStream * in;

  e2 = NULL;
  try {
    this->InputStream::close();
  } catch( Error e ) {
    e2 = e;
  }

  if ( this->ctx != NULL ) {
    this->ctx->final();
    this->ctx = NULL;
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
HashInputStream::available( void ) throw( Error )
{
  if ( this->in == NULL )
    return false;

  if ( this->InputStream::available() ||
       this->in->available() )
    return true;
  return false;
}


unsigned int
HashInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  unsigned int nBytes;

  if ( this->in == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

  if ( (nBytes = this->in->readBytes( buf, bufLen )) > 0 )
    this->ctx->update( buf, nBytes );

  return nBytes;
}


HashOutputStream::HashOutputStream( OutputStream *out,  HashContext *ctx,
                                    unsigned int bufLen,  bool lineBuffered,
                                    bool closePipe,  StreamOffset streamOffset )
                : OutputStream( bufLen, lineBuffered, closePipe, streamOffset )
{
  this->out = out;
  this->ctx = ctx;
  this->ctx->init();
}


HashOutputStream::~HashOutputStream()
{
  if ( this->out != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
HashOutputStream::close( void ) throw( Error )
{
  OutputStream * out;
  Error          e2;

  e2 = NULL;
  try {
    this->flush();
  } catch( Error e ) {
    e2 = e;
  }

  try {
    this->OutputStream::close();
  } catch( Error e ) {
    if ( e2 == NULL )
      e2 = e;
  }

  if ( this->ctx != NULL ) {
    this->ctx->final();
    this->ctx = NULL;
  }
  out = this->out;
  this->out = NULL;

  if ( out != NULL && this->closePipe ) {
    try {
      out->close();
    } catch( Error e ) {
      if ( e2 == NULL )
        e2 = e;
    }
    delete out;
  }

  if ( e2 != NULL )
    throw e2;
}


unsigned int
HashOutputStream::emptyBuf( const byte *buf,  unsigned int bufLen )
                  throw( Error )
{
  unsigned int nBytes;

  if ( this->out == NULL ) {
    this->ctx->update( buf, bufLen );
    return bufLen;
  }

  if ( (nBytes = this->out->writeBytes( buf, bufLen )) > 0 )
    this->ctx->update( buf, nBytes );
  return nBytes;
}

