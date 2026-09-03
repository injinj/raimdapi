/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_stream__stdio_stream_h__
#define __rai_stream__stdio_stream_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#ifndef __rai_stream__file_stream_h__
#include "stream/file_stream.h"
#endif


namespace rai {

class RAIBASE_DLL_EXP StdioInputStream : public FileInputStream {
  protected:
    virtual unsigned int fillBuf( byte *buf,  unsigned int bufLen )
;
  public:
    SYS_OPS( StdioInputStream );
    StdioInputStream( File *file,  unsigned int bufLen );

    virtual bool available( void );

    static InputStream * createStdin( unsigned int bufLen );
};


class RAIBASE_DLL_EXP StdioOutputStream : public FileOutputStream {
  protected:
    virtual unsigned int emptyBuf( const byte *buf,  unsigned int bufLen )
;
  public:
    SYS_OPS( StdioOutputStream );
    StdioOutputStream( File *file,  unsigned int bufLen,  bool lineBuffered );

    static OutputStream * createStdout( unsigned int bufLen );

    static OutputStream * createStderr( unsigned int bufLen );
};
} // namespace rai

#endif
