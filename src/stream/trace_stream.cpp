/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "base/file.h"
#include "stream/trace_stream.h"
#include "base/mem.h"
#include "base/thread.h"

using namespace rai;

static const char hexChars[] = "0123456789abcdef";

TraceFile::TraceFile( OutputStream * f )
    : file( f ), curOff( 0 ), curDir( 0 ), boff( 0 ), hex( 9 ), ascii( 61 ),
      lck( NULL ) {
  this->lck = Mutex::create();
}

TraceFile::~TraceFile()
{
  if ( this->lck != NULL )
    delete this->lck;
}

void
TraceFile::initLine( void )
{
  StreamOffset k = this->curOff;
  unsigned int j;
  ::memset( this->line, ' ', 79 );
  this->line[ 5 ] = '0';
  if ( this->curDir ) {
    this->line[ 6 ] = '-';
    this->line[ 7 ] = '>';
  }
  else {
    this->line[ 6 ] = '<';
    this->line[ 7 ] = '-';
  }
  for ( j = 5; k > 0; ) {
    this->line[ j ] = (byte) hexChars[ k & 0xf ];
    if ( j-- == 0 )
      break;
    k >>= 4;
  }
}

void
TraceFile::flushLine( void )
{
  if ( this->boff > 0 ) {
    this->line[ 79 ] = '\n';
    try {
      this->file->writeBytes( this->line, 80 );
    } catch ( ... ) {
    }
    this->curOff += this->boff;
    this->boff  = 0;
    this->hex   = 9;
    this->ascii = 61;
  }
}

void
TraceFile::dumpHex( unsigned int direction,  const byte *buf,
                    unsigned int bufLen )
{
  unsigned int i;

  this->lck->lock();

  if ( direction != this->curDir || ( bufLen == 0 && this->boff > 0 ) ) {
    this->flushLine();
    this->curDir = direction;
    this->curOff = 0;
  }

  for ( i = 0; i < bufLen; i++ ) {
    if ( ( this->boff & 15 ) == 0 ) {
      if ( this->boff > 0 )
        this->flushLine();
      this->initLine();
    }
    this->line[ this->hex ]   = hexChars[ buf[ i ] >> 4 ];
    this->line[ this->hex+1 ] = hexChars[ buf[ i ] & 0xf ];
    this->hex += 3;
    if ( buf[ i ] >= ' ' && buf[ i ] <= 127 )
      line[ this->ascii ] = buf[ i ];
    this->ascii++;
    if ( ( ++this->boff & 0x3 ) == 0 )
      this->hex++;
  }
  try {
    this->file->flush();
  } catch ( ... ) {
  }
  this->lck->unlock();
}


TraceInputStream::TraceInputStream( TraceFile &f,  InputStream *i )
  : file( f ), in( i ) {}


TraceInputStream::~TraceInputStream()
{
  if ( this->in != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


InputStream *
TraceInputStream::create( TraceFile &file,  InputStream *in )
                  throw( Error )
{
  return NEW TraceInputStream( file, in );
}


void
TraceInputStream::close( void ) throw( Error )
{
  Error         e2;
  InputStream * in;

  this->file.dumpHex( 0, NULL, 0 );

  if ( this->lock != NULL )
    this->lock->lock();

  e2 = NULL;
  if ( this->in != NULL ) {
    try {
      this->InputStream::close();
    } catch( Error e ) {
      e2 = e;
    }
    in = this->in;
    this->in   = NULL;

    if ( this->closePipe ) {
      if ( in != NULL ) {
        try {
          in->close();
        } catch( Error e ) {
          if ( e2 == NULL )
            e2 = e;
        }
        delete in;
      }
    }
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


StreamOffset
TraceInputStream::seekSet( StreamSeekOffset offset,  int whence ) throw( Error )
{
  if ( this->lock != NULL )
    this->lock->lock();

  try {
    if ( this->in == NULL )
      throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

    this->streamOffset = this->in->seekSet( offset, whence );
    this->offset       = 0;
    this->length       = 0;
    this->endOfFile    = false;

    if ( this->lock != NULL )
      this->lock->unlock();

    return (StreamOffset) offset;
  } catch ( ... ) {
    if ( this->lock != NULL )
      this->lock->unlock();
    throw;
  }
}


bool
TraceInputStream::available( void ) throw( Error )
{
  bool avail;

  if ( this->lock != NULL )
    this->lock->lock();

  avail = false;
  try {
    if ( this->in != NULL ) {
      if ( this->InputStream::available() || this->in->available() )
        avail = true;
    }
    if ( this->lock != NULL )
      this->lock->unlock();
  } catch ( ... ) {
    if ( this->lock != NULL )
      this->lock->unlock();
    throw;
  }

  return avail;
}


unsigned int
TraceInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  if ( this->in == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

  unsigned int n = this->in->readBytes( buf, bufLen );
  if ( n > 0 )
    this->file.dumpHex( 0, buf, n );
  return n;
}


TraceOutputStream::TraceOutputStream( TraceFile &f,  OutputStream *o )
  : file( f ), out( o ) {}


TraceOutputStream::~TraceOutputStream()
{
  if ( this->out != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


OutputStream *
TraceOutputStream::create( TraceFile &file,  OutputStream *out )
                  throw( Error )
{
  return NEW TraceOutputStream( file, out );
}


void
TraceOutputStream::close( void ) throw( Error )
{
  Error          e2;
  OutputStream * out;

  this->file.dumpHex( 1, NULL, 0 );

  if ( this->lock != NULL )
    this->lock->lock();

  e2 = NULL;
  if ( this->out != NULL ) {
    try {
      this->OutputStream::close();
    } catch( Error e ) {
      e2 = e;
    }
    out = this->out;
    this->out  = NULL;

    if ( this->closePipe ) {
      if ( out != NULL ) {
        try {
          out->close();
        } catch( Error e ) {
          if ( e2 == NULL )
            e2 = e;
        }
        delete out;
      }
    }
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


StreamOffset
TraceOutputStream::seekSet( StreamSeekOffset offset,  int whence ) throw( Error )
{
  if ( this->lock != NULL )
    this->lock->lock();

  try {
    this->flush();
    this->streamOffset = this->out->seekSet( offset, whence );

    if ( this->lock != NULL )
      this->lock->unlock();

    return (StreamOffset) offset;
  } catch ( ... ) {
    if ( this->lock != NULL )
      this->lock->unlock();
    throw;
  }
}


unsigned int
TraceOutputStream::emptyBuf( const byte *buf,  unsigned int bufLen )
                  throw( Error )
{
  if ( this->out == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );
  unsigned int n = this->out->writeBytes( buf, bufLen );
  if ( n > 0 )
    this->file.dumpHex( 1, buf, n );
  return n;
}


void
TraceOutputStream::flush( void ) throw( Error )
{
  this->OutputStream::flush();
  this->out->flush();
  this->file.dumpHex( 1, NULL, 0 );
}

