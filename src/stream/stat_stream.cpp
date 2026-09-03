/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "stream/stat_stream.h"
#include "util/hash_util.h"
#include "util/array.h"
#include "base/thread.h"
#include "store/marshal_buf.h"

using namespace rai;

static const byte NEW_STAT_ID_BIT = 0x80U;
static const byte TIMESTAMP_BIT   = 0x40U;
static const byte INTERVAL_BIT    = 0x20U;


StatId *
StatId::create( const char *name,  const char *name2,
                const char *name3,  const char *units ) throw( Error )
{
  StatId     * id;
  unsigned int len;

  id = NULL;
  try {
    id  = NEW StatId( NULL, NULL );
    len = ::strlen( name ) + 1;
    if ( name2 != NULL )
      len += ::strlen( name2 );
    if ( name3 != NULL )
      len += ::strlen( name3 );
    if ( units != NULL )
      len += ::strlen( units ) + 1;

    MALLOC( len, &id->ident );

    ::strcpy( id->ident, name );
    if ( name2 != NULL )
      ::strcat( id->ident, name2 );
    if ( name3 != NULL )
      ::strcat( id->ident, name3 );

    id->identLen = ::strlen( id->ident );

    if ( units != NULL ) {
      id->units = &id->ident[ id->identLen + 1 ];
      ::strcpy( id->units, units );
      id->unitsLen = 0; /* don't free, its part of ident */
    }
    return id;
  } catch ( ... ) {
    if ( id != NULL )
      delete id;
    throw;
  }
}


namespace rai {
class StatTable : public Array<StatId *> {
  public:
    SYS_OPS( StatTable );
    StatTable() : Array<StatId *>( 32 ) {};
};
}


StatOutputStream::StatOutputStream( OutputStream *out,  unsigned int bufLen,
                                    bool closePipe )
                : OutputStream( bufLen, false, closePipe )
{
  this->out       = out;
  this->statTab   = NULL;
  this->stamp     = 0;
  this->deltaTime = 0;
}


StatOutputStream::~StatOutputStream()
{
  if ( this->out != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
  if ( this->statTab != NULL )
    delete this->statTab;
}


unsigned int
StatOutputStream::emptyBuf( const byte *buf,  unsigned int bufLen )
                 throw( Error )
{
  return this->out->writeBytes( buf, bufLen );
}


void
StatOutputStream::close( void ) throw( Error )
{
  Error          e2;
  OutputStream * out;

  if ( this->lock != NULL )
    this->lock->lock();
  e2 = NULL;
  if ( this->out != NULL ) {
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
    out = this->out;
    this->out = NULL;

    if ( this->closePipe ) {
      try {
        out->close();
      } catch( Error e ) {
        if ( e2 == NULL )
          e2 = e;
      }
      delete out;
    }
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


void
StatOutputStream::flush( void ) throw( Error )
{
  Error e2;

  if ( this->lock != NULL )
    this->lock->lock();
  e2 = NULL;
  try {
    this->OutputStream::flush();
    this->out->flush();
  } catch ( Error e ) {
    e2 = e;
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


void
StatOutputStream::makeSpace( void ) throw( Error )
{
  if ( this->buf == NULL || 256 > this->bufLen ) {
    unsigned int len = ( 256 < this->bufLen ) ?  this->bufLen : 256;
    REALLOC( len, &this->buf );
    this->bufLen = len;
  }

  if ( this->offset + 256 > this->bufLen ) {
    this->emptyBuf( this->buf, this->offset );
    this->streamOffset += this->offset;
    this->offset        = 0;
  }
}


void
StatOutputStream::putTimestamp( TimeMSecs stamp ) throw( Error )
{
  MarshalBuf msgOut;
  Error      e2;

  if ( this->lock != NULL )
    this->lock->lock();
  e2 = NULL;
  try {
    this->makeSpace();

    msgOut.data    = &this->buf[ this->offset ];
    msgOut.dataOff = sizeof( byte );
    msgOut.put( stamp );
    msgOut.data[ 0 ] = (byte) ( msgOut.dataOff -
                                sizeof( byte ) ) | TIMESTAMP_BIT;

    this->offset += msgOut.dataOff;
    this->stamp   = stamp;
  } catch ( Error e ) {
    e2 = e;
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


void
StatOutputStream::putInterval( double interval ) throw( Error )
{
  MarshalBuf msgOut;
  Error      e2;

  if ( this->lock != NULL )
    this->lock->lock();
  e2 = NULL;
  try {
    this->makeSpace();

    msgOut.data    = &this->buf[ this->offset ];
    msgOut.dataOff = sizeof( byte );
    msgOut.put( interval );
    msgOut.data[ 0 ] = (byte) ( msgOut.dataOff -
                                sizeof( byte ) ) | INTERVAL_BIT;

    this->offset    += msgOut.dataOff;
    this->deltaTime += interval;
  } catch ( Error e ) {
    e2 = e;
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


template<class Val> void
StatOutputStream::put( StatId *id,  Val val ) throw( Error )
{
  MarshalBuf msgOut;
  Error      e2;

  if ( this->lock != NULL )
    this->lock->lock();
  e2 = NULL;
  try {
    this->makeSpace();

    msgOut.data    = &this->buf[ this->offset ];
    msgOut.dataOff = sizeof( byte );

    if ( this->statTab == NULL ||
         id->order >= this->statTab->length() ||
         id != this->statTab->get( id->order ) ) {

      msgOut.put( id->ident )
            .put( val );

      msgOut.data[ 0 ] = (byte) ( msgOut.dataOff -
                                  sizeof( byte ) ) | NEW_STAT_ID_BIT;

      if ( this->statTab == NULL )
        this->statTab = NEW StatTable();
      id->order = this->statTab->length();
      this->statTab->pushTail( id );
    }
    else {
      msgOut.put( id->order )
            .put( val );

      msgOut.data[ 0 ] = (byte) ( msgOut.dataOff - sizeof( byte ) );
    }

    this->offset += msgOut.dataOff;
  } catch ( Error e ) {
    e2 = e;
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}

/* instantiate */
template void StatOutputStream::put( StatId *id,  unsigned int val )
                                                               throw( Error );
template void StatOutputStream::put( StatId *id,  ullong val ) throw( Error );


StatInputStream::StatInputStream( StatFilter *filter,  InputStream *in,
                                  unsigned int bufLen,  bool closePipe )
               : InputStream( bufLen, closePipe )
{
  this->filter    = filter;
  this->in        = in;
  this->statTab   = NULL;
  this->stamp     = 0;
  this->deltaTime = 0;
}


StatInputStream::~StatInputStream()
{
  if ( this->in != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
  if ( this->statTab != NULL ) {
    while ( ! this->statTab->isEmpty() ) {
      if ( this->filter == NULL )
        delete this->statTab->popTail();
      else
        this->filter->release( this->statTab->popTail() );
    }
    delete this->statTab;
  }
}


unsigned int
StatInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  return this->in->readBytes( buf, bufLen );
}


bool
StatInputStream::available( void ) throw( Error )
{
  Error e2 = NULL;
  if ( this->lock != NULL )
    this->lock->lock();
  try {
    this->endOfFile = ! ( this->in != NULL &&
                          ( this->InputStream::available() ||
                            this->in->available() ) );
  } catch ( Error e ) {
    e2 = e;
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
  return ! this->endOfFile;
}


void
StatInputStream::close( void ) throw( Error )
{
  Error         e2;
  InputStream * in;

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
    this->in = NULL;

    if ( this->closePipe ) {
      try {
        in->close();
      } catch( Error e ) {
        if ( e2 == NULL )
          e2 = e;
      }
      delete in;
    }
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


bool
StatInputStream::loadNext( unsigned int &dataLen ) throw( Error )
{
  unsigned int nBytes,
               len;

  if ( ! this->endOfFile ) {
    if ( this->offset == this->length ) {

      if ( this->buf == NULL )
        MALLOC( this->bufLen, &this->buf );

      if ( (nBytes = this->fillBuf( this->buf, this->bufLen )) == 0 )
        this->endOfFile = true;
      else {
        this->offset        = 0;
        this->length        = nBytes;
        this->streamOffset += nBytes;
      }
    }

    if ( this->offset + sizeof( byte ) <= this->length ) {
      dataLen = this->buf[ this->offset ];
      if ( ( dataLen & NEW_STAT_ID_BIT ) != 0 )
        len = ( dataLen & ~NEW_STAT_ID_BIT ) + sizeof( byte );
      else
        len = ( dataLen & ~( TIMESTAMP_BIT | INTERVAL_BIT ) ) + sizeof( byte );

      if ( len > this->length - this->offset ) {
        if ( len > this->bufLen ) {
          REALLOC( len, &this->buf );
          this->bufLen = len;
        }
        if ( this->offset < this->length ) {
          ::memmove( this->buf, &this->buf[ this->offset ],
                     this->length - this->offset );
          this->length -= this->offset;
          this->offset  = 0;
        }

        if ( (nBytes = this->fillBuf( &this->buf[ this->length ],
                                      this->bufLen - this->length )) == 0 )
          this->endOfFile = true;
        else {
          this->length       += nBytes;
          this->streamOffset += nBytes;

          if ( len > this->length - this->offset )
            throw StatStreamErr::getErr( StatStreamErr::TRUNC_STAT_STREAM );
        }
      }
    }
  }
  if ( this->endOfFile )
    return false;
  return true;
}


template<class Val> bool
StatInputStream::get( StatId *&id,  Val &val ) throw( Error )
{
  MarshalBuf   msgIn;
  char       * name;
  unsigned int order,
               len;
  double       interval;
  Error        e2;

  if ( this->lock != NULL )
    this->lock->lock();
  e2 = NULL;
  if ( ! this->endOfFile ) {
    try {
      while ( this->loadNext( msgIn.dataLen ) ) {
        msgIn.data    = &this->buf[ this->offset ];
        msgIn.dataOff = sizeof( byte );

        if ( ( msgIn.dataLen & NEW_STAT_ID_BIT ) != 0 ) {
          msgIn.get( name )
               .get( val );

          len = ::strlen( name );
          if ( this->filter != NULL )
            id = this->filter->create( name, len, NULL, 0 );
          else
            id = NEW StatId( len, name, 0, NULL );

          if ( this->statTab == NULL )
            this->statTab = NEW StatTable();

          id->order     = this->statTab->length();
          id->startTime = this->stamp;
          id->deltaTime = this->deltaTime;
          this->statTab->pushTail( id );
          this->offset += msgIn.dataOff;
          break;
        }
        else if ( ( msgIn.dataLen & TIMESTAMP_BIT ) != 0 ) {
          msgIn.get( this->stamp );
          this->offset += msgIn.dataOff;
          if ( this->filter != NULL )
            this->filter->start( this->stamp );
        }
        else if ( ( msgIn.dataLen & INTERVAL_BIT ) != 0 ) {
          msgIn.get( interval );
          this->deltaTime += interval;
          this->offset    += msgIn.dataOff;
          if ( this->filter != NULL )
            this->filter->next( this->deltaTime, interval );
        }
        else {
          msgIn.get( order )
               .get( val );
          id = this->statTab->get( order );
          id->deltaTime = this->deltaTime;
          this->offset += msgIn.dataOff;
          break;
        }
      }
    } catch ( Error e ) {
      e2 = e;
    }
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
  if ( this->endOfFile )
    return false;
  return true;
}

template bool StatInputStream::get( StatId *&id,  unsigned int &val )
                                                                throw( Error );
template bool StatInputStream::get( StatId *&id,  ullong &val ) throw( Error );

Error
StatStreamErr::getErr( unsigned int status )
{
  static const char     mod[] = "StatStream";
  static const ErrorRec err[] = {
  /*  0 */ { TRUNC_STAT_STREAM, "Truncated stat stream", mod },
  /*  1 */ { 1,                 "Unknown StatStream error", mod },
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}
