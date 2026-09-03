/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAINET_DLL_EXP ) && defined( RAI_DLL )
#define RAINET_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "stream/chunked_stream.h"
#include "util/str_util.h"

using namespace rai;

ChunkedInputStream::ChunkedInputStream( InputStream *in,
                                        unsigned int bufLen,
                                        bool closePipe,
                                        StreamOffset streamOffset )
                    : InputStream( bufLen, closePipe, streamOffset )
{
  this->in            = in;
  this->chunkOff      = 0;
  this->chunkLen      = 0;
  this->seenLastChunk = false;
}


ChunkedInputStream::~ChunkedInputStream()
{
  if ( this->in != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
ChunkedInputStream::close( void ) throw( Error )
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
ChunkedInputStream::available( void ) throw( Error )
{
  if ( this->in == NULL || this->seenLastChunk )
    return false;

  if ( this->InputStream::available() ||
       this->in->available() )
    return true;
  return false;
}


unsigned int
ChunkedInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  unsigned int bufOff,
               nBytes;
  char         line[ 120 ];

  if ( this->in == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

  if ( this->seenLastChunk )
    return 0;

  try {
    for ( bufOff = 0; bufOff < bufLen; ) {
      if ( this->chunkOff == this->chunkLen ) {
        /* if first line, this is the chunk size, otherwise this is empty */
        nBytes = this->in->gets( line, sizeof( line ) );

        /* if getting next chunk, eat the next line */
        if ( this->chunkLen > 0 ) {
          if ( nBytes != 2 || line[ 0 ] != '\r' || line[ 1 ] != '\n' )
            throw ChunkedStreamErr::getErr( ChunkedStreamErr::EXPECTING_CRLF );

          /* reset these in case next gets() throws a would block */
          this->chunkLen = 0;
          this->chunkOff = 0;

          /* this line should have a chunk size in hex on it */
          nBytes = this->in->gets( line, sizeof( line ) );
        }

        if ( nBytes <= 2 || line[ nBytes - 2 ] != '\r' ||
            line[ nBytes - 1 ] != '\n' )
          throw ChunkedStreamErr::getErr(
                                      ChunkedStreamErr::EXPECTING_CHUNK_SIZE );
        /* get chunk size */
        StrUtil::parseInt( line, &this->chunkLen, NULL, U_HEX );

        if ( this->chunkLen == 0 ) { /* last chunk */
          this->seenLastChunk = true;
          break;
        }
      }

      nBytes = this->chunkLen - this->chunkOff;
      if ( nBytes > bufLen - bufOff )
        nBytes = bufLen - bufOff;

      nBytes = this->in->readBytes( &buf[ bufOff ], nBytes );

      this->chunkOff += nBytes;
      bufOff         += nBytes;
    }
  } catch ( Error e ) {
    if ( bufOff == 0 || e != IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK ) )
      throw e;
  }

  return bufOff;
}


Error
ChunkedStreamErr::getErr( unsigned int status )
{
  static const char     mod[] = "ChunkedStream";
  static const ErrorRec err[] = {
  /*  0 */ { EXPECTING_CRLF,       "Chunked block size missing crlf", mod },
  /*  1 */ { EXPECTING_CHUNK_SIZE, "Chunked block size not found", mod },
  /*  2 */ { 2,                    "Unknown chunked stream error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}
