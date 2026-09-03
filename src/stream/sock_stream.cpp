/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAINET_DLL_EXP ) && defined( RAI_DLL )
#define RAINET_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "base/sys.h"
#include "net/sock.h"
#include "stream/sock_stream.h"

using namespace rai;

SocketInputStream::SocketInputStream( Socket *sock,  unsigned int bufLen,
                                      bool closePipe,
                                      StreamOffset streamOffset, ullong *rcp ) :
                   InputStream( bufLen, closePipe, streamOffset )
{
  this->sock         = sock;
  this->recvCountPtr = rcp;
}


SocketInputStream::~SocketInputStream()
{
  if ( this->sock != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
SocketInputStream::close( void ) throw( Error )
{
  Error   e2;
  Socket * sock;

  e2 = NULL;
  try {
    this->InputStream::close();
  } catch( Error e ) {
    e2 = e;
  }

  sock = this->sock;
  this->sock = NULL;

  if ( this->closePipe && sock != NULL ) {
    try {
      sock->close();
    } catch( Error e ) {
      e2 = e;
    }
    delete sock;
  }

  if ( e2 != NULL )
    throw e2;
}


bool
SocketInputStream::available( void ) throw( Error )
{
  if ( this->sock == NULL )
    return false;

  if ( this->InputStream::available() ||
       this->sock->testState( Socket::CAN_READ ) == Socket::CAN_READ )
    return true;

  return false;
}


unsigned int
SocketInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  try {
    unsigned int n = this->sock->recv( buf, bufLen );
    if ( this->recvCountPtr != NULL )
      this->recvCountPtr[ 0 ]++;
    return n;
  } catch( Error e ) {
    if ( e == SockErr::getErr( SockErr::WOULD_BLOCK ) )
      throw IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK );
    if ( e == SockErr::getErr( SockErr::RECV_INTR ) )
      throw IOStreamErr::getErr( IOStreamErr::INTERRUPTED );
    throw e;
  }
}


SocketOutputStream::SocketOutputStream( Socket *sock,  unsigned int bufLen,
                                        bool lineBuffered,  bool closePipe,
                                        StreamOffset streamOffset,
                                        ullong *scp ) :
                   OutputStream( bufLen, lineBuffered, closePipe, streamOffset )
{
  this->sock         = sock;
  this->sendCountPtr = scp;
}


SocketOutputStream::~SocketOutputStream()
{
  if ( this->sock != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


void
SocketOutputStream::close( void ) throw( Error )
{
  Socket * sock;
  Error   e2;

  if ( this->sock != NULL ) {
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

    sock = this->sock;
    this->sock = NULL;

    if ( this->closePipe ) {
      try {
        sock->close();
      } catch( Error e ) {
        if ( e2 == NULL )
          e2 = e;
      }
      delete sock;
    }

    if ( e2 != NULL )
      throw e2;
  }
}


unsigned int
SocketOutputStream::emptyBuf( const byte *buf,  unsigned int bufLen )
                    throw( Error )
{
  try {
    unsigned int n = this->sock->send( buf, bufLen );
    if ( this->sendCountPtr != NULL )
      this->sendCountPtr[ 0 ]++;
    return n;
  } catch( Error e ) {
    if ( e == SockErr::getErr( SockErr::WOULD_BLOCK ) )
      throw IOStreamErr::getErr( IOStreamErr::WOULD_BLOCK );
    if ( e == SockErr::getErr( SockErr::SEND_INTR ) )
      throw IOStreamErr::getErr( IOStreamErr::INTERRUPTED );
    throw e;
  }
}
