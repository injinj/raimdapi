#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>
#include "stream/record_stream.h"

using namespace rai;

/* don't send buf to the output stream until flush() is called */
unsigned int 
RecordOutputStream::emptyBuf( const byte *buf,  unsigned int
                        bufLen )       throw( Error ) 
{
  if ( this->dataOff + bufLen > this->dataOff ) {
    unsigned int len = ( ( this->dataOff + bufLen ) | 1023 ) + 1;
    REALLOC( len, &this->data );
    this->dataLen = len;
  }
  ::memcpy( &this->data[ this->dataOff ], buf, bufLen );
  this->dataOff += bufLen;
  return bufLen;
}


RecordOutputStream::RecordOutputStream( OutputStream *out,
                                        unsigned int bufLen,
                                        bool closePipe )
  : OutputStream( bufLen, false, closePipe, (StreamOffset) 0UL ) 
{
  this->out     = out;
  this->data    = NULL;
  this->dataOff = 0;
  this->dataLen = 0;
}
 
RecordOutputStream::~RecordOutputStream() {
  if ( this->data != NULL ) {
    FREE( this->data );
    this->data = NULL;
  }
}

/* cause everything to be written */
void 
RecordOutputStream::flush( void ) throw( Error ) 
{
  this->OutputStream::flush();
  if ( this->dataOff > 0 ) {
    this->out->writeBytes( this->data, this->dataOff );
    this->out->flush();
    this->dataOff = 0;
  }
}

void 
RecordOutputStream::close( void ) throw( Error ) {
  Error e2;
  
  e2 = NULL;
  try {
    this->flush();
    this->OutputStream::close();
  } catch( Error e ) {
    e2 = e;
  }
  if ( this->closePipe && this->out != NULL ) {
    try {
      this->out->close();
    } catch( Error e ) {
      if ( e2 == NULL )
        e2 = e;
    }
    delete this->out;
  }
  if ( e2 != NULL )
    throw e2;
}
