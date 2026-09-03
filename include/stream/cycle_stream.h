/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_stream__cycle_stream_h__
#define __rai_stream__cycle_stream_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#ifndef __rai_stream__io_stream_h__
#include "stream/io_stream.h"
#endif

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

namespace rai {

class RAIBASE_DLL_EXP CycleOutputStream : public OutputStream {
  protected:
    byte       * data;
    unsigned int dataStart,
                 dataUsed,
                 dataLen;

    virtual unsigned int emptyBuf( const byte *buf,  unsigned int bufLen )
                                                               throw( Error );
  public:
    SYS_OPS( CycleOutputStream );

    CycleOutputStream( byte *data,  unsigned int dataLen );
    virtual ~CycleOutputStream() {};

    void reset( byte *data,  unsigned int dataLen );

    unsigned int length( void ) const { return this->dataUsed; };

    unsigned int copyTo( byte *toBuf,  unsigned int bufLen ) const;
};
}

#endif
