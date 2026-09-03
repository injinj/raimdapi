/* Copyright (c) 2004 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include "util/raibuf.h"
#include "base/log.h"
#include "base/mem.h"

using namespace rai;

RaiBuf::RaiBuf( int size ) {
  MALLOC( size, &b );
  bSize		= size;
  bUsed		= 0;
  memAllocType	= RaiBuf::DYNAMIC_BUFFER;
}

void RaiBuf::resize( int newSize ) throw( Error )
{
  if( memAllocType == RaiBuf::DYNAMIC_BUFFER ) {
    REALLOC( newSize, &b );
    bSize = newSize;
  } else {
    throw( RaiBufErr::getErr( STATIC_BUFFER ) );
  }
}

void RaiBuf::dump( int indent ) {
  logDebug( LDEBUG, "%*.sRaiBuf %d bytes allocated as %s bytes used: %d", indent, "",
            bSize, 
            memAllocType == RaiBuf::DYNAMIC_BUFFER ? "DYNAMIC_BUFFER" : "STATIC_BUFFER",
            bUsed );
}

void RaiBuf::dumpHex( ) {
  logDebugHex( LDEBUG, "RaiBuf", b, bUsed);
}

RaiBuf::~RaiBuf() {
  if( memAllocType == RaiBuf::DYNAMIC_BUFFER ) {
    FREE( b );
  }
}

Error
RaiBufErr::getErr( unsigned int status )
{
  static const char     mod[] = "RaiBuf";
  static const ErrorRec err[] = {
  /*  0 */ { OK,                "Ok", mod },
  /*  1 */ { NO_MEMORY,         "Insufficient memory for the operation", mod },
  /*  2 */ { STATIC_BUFFER,     "Trying to resize static buffer", mod },
  /*  3 */ { WRITE_BOUNDS_ERR,  "Attempting to write outside buff", mod },
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}
