/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAINET_DLL_EXP ) && defined( RAI_DLL )
#define RAINET_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "http/mime_magic.h"
#include "stream/mime_magic_stream.h"

using namespace rai;

MimeMagicInputStream::MimeMagicInputStream( InputStream *in,  MimeMagic *magik,
                                            unsigned int magikBufLen,
                                            unsigned int bufLen,
                                            bool closePipe,
                                            StreamOffset streamOffset )
                    : InputStream( bufLen, closePipe, streamOffset )
{
  this->in           = in;
  this->magik        = magik;
  this->mimeType     = NULL;
  this->magikBuf     = NULL;
  this->magikBufOff  = 0;
  this->magikBufUsed = 0;
  this->magikBufLen  = magikBufLen;
}


MimeMagicInputStream::~MimeMagicInputStream()
{
  if ( this->in != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
MimeMagicInputStream::close( void ) throw( Error )
{
  Error         e2;
  byte        * buf;
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

  buf = this->magikBuf;
  this->magikBuf     = NULL;
  this->magikBufLen  = 0;
  this->magikBufOff  = 0;
  this->magikBufUsed = 0;

  if ( buf != NULL )
    FREE( buf );

  if ( e2 != NULL )
    throw e2;
}


bool
MimeMagicInputStream::available( void ) throw( Error )
{
  if ( this->in == NULL )
    return false;

  if ( this->InputStream::available() ||
       this->magikBufUsed < this->magikBufOff ||
       this->in->available() )
    return true;
  return false;
}


void
MimeMagicInputStream::putBack( const byte *buf,  unsigned int bufLen )
                      throw( Error )
{
  /* this->magikBufOff and this->magikBufUsed should be zero */
  if ( this->magikBuf == NULL || bufLen > this->magikBufLen ) {
    if ( bufLen > this->magikBufLen )
      this->magikBufLen = bufLen;
    REALLOC( this->magikBufLen, &this->magikBuf );
  }
  /* put buf back into front of stream */
  ::memcpy( this->magikBuf, buf, bufLen );
  this->magikBufOff = bufLen;
}


unsigned int
MimeMagicInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  unsigned int n,
               n2,
               nBytes;

  if ( this->in == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

  if ( this->magikBufUsed < this->magikBufOff ||
       ( this->magikBufOff < this->magikBufLen && this->mimeType == NULL ) ) {

    if ( this->magikBuf == NULL )
      MALLOC( this->magikBufLen, &this->magikBuf );

    nBytes = this->magikBufOff - this->magikBufUsed;
    /* all unread bytes from magik buf */
    if ( nBytes >= bufLen ) {
      ::memcpy( buf, &this->magikBuf[ this->magikBufUsed ], bufLen );
      this->magikBufUsed += bufLen;
      nBytes = bufLen;
    }
    else {
      /* partial or no unread bytes from magik buf */
      ::memcpy( buf, &this->magikBuf[ this->magikBufUsed ], nBytes );
      /* read rest of bytes needed, add nBytes after in case of error */
      n = this->in->readBytes( &buf[ nBytes ], bufLen - nBytes );
      this->magikBufUsed += nBytes;

      /* copy bytes read into buf into magik buf */
      if ( this->magikBufOff < this->magikBufLen ) {
        n2 = this->magikBufLen - this->magikBufOff;
        if ( n2 > n )
          n2 = n;
        ::memcpy( &this->magikBuf[ this->magikBufUsed ], &buf[ nBytes ], n2 );
        this->magikBufOff  += n2;
        this->magikBufUsed += n2;
      }

      nBytes += n;
    }

    /* if magik buf no longer needed to determine mime type */
    if ( this->magikBufUsed == this->magikBufOff && this->mimeType != NULL ) {
      FREE( this->magikBuf );
      this->magikBuf = NULL;
    }
  }
  else {
    /* finished mime typing, just pass through to input stream */
    nBytes = this->in->readBytes( buf, bufLen );
  }

  return nBytes;
}


const char *
MimeMagicInputStream::getMimeType( void ) throw( Error )
{
  unsigned int nBytes;

  if ( this->in == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

  /* check if already determined mime type */
  if ( this->mimeType != NULL )
    return this->mimeType;

  if ( this->magikBuf == NULL )
    MALLOC( this->magikBufLen, &this->magikBuf );

  /* fill up magik buffer for typing */
  if ( this->magikBufOff < this->magikBufLen ) {
    nBytes = this->in->readBytes( &this->magikBuf[ this->magikBufOff ],
                                  this->magikBufLen - this->magikBufOff );
    this->magikBufOff += nBytes;
  }

  /* try to determine mime type */
  this->mimeType = this->magik->getMimeType( this->magikBuf,
                                             this->magikBufOff );
  /* no longer need magik buf if used up by fillBuf */
  if ( this->magikBufUsed == this->magikBufOff ) {
    FREE( this->magikBuf );
    this->magikBuf = NULL;
  }

  return this->mimeType;
}


