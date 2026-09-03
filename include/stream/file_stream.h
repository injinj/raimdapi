/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_stream__file_stream_h__
#define __rai_stream__file_stream_h__

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
class File;


class RAIBASE_DLL_EXP FileInputStream : public InputStream {
  protected:
    File * file;

    virtual unsigned int fillBuf( byte *buf,  unsigned int bufLen ) 
                                                         throw( Error );
  public:
    SYS_OPS( FileInputStream );
    FileInputStream( File *file,
                     unsigned int bufLen       = InputStream::BUF_LEN,
                     bool closePipe            = false,
                     StreamOffset streamOffset = (StreamOffset) 0UL );
    virtual ~FileInputStream();

    virtual void close( void )                                 throw( Error );

    virtual bool available( void )                             throw( Error );

    virtual StreamOffset seekSet( StreamSeekOffset offset,  int whence ) 
                                                               throw( Error );
    static InputStream * create( File *file,
                               unsigned int bufLen       = InputStream::BUF_LEN,
                               bool closePipe            = false,
                               StreamOffset streamOffset = (StreamOffset) 0UL )
                                                               throw( Error );
    static InputStream * open( const char *filepath,
                               unsigned int bufLen       = InputStream::BUF_LEN,
                               StreamOffset streamOffset = (StreamOffset) 0UL )
                                                               throw( Error );
};


class RAIBASE_DLL_EXP FileOutputStream : public OutputStream {
  protected:
    File * file;

    virtual unsigned int emptyBuf( const byte *buf,  unsigned int bufLen )
                                                               throw( Error );
  public:
    SYS_OPS( FileOutputStream );
    FileOutputStream( File *file,
                      unsigned int bufLen     = OutputStream::BUF_LEN,
                      bool lineBuffered       = false,
                      bool closePipe          = false,
                      StreamOffset streamOffset = (StreamOffset) 0UL );
    virtual ~FileOutputStream();

    virtual void close( void )                                 throw( Error );

    virtual StreamOffset seekSet( StreamSeekOffset offset,  int whence )
                                                               throw( Error );
    static OutputStream * create( File *file,
                                unsigned int bufLen   = OutputStream::BUF_LEN,
                                bool lineBuffered     = false,
                                bool closePipe        = false,
                                StreamOffset streamOffset = (StreamOffset) 0UL )
                                                               throw( Error );
    static OutputStream * open( const char *filepath,
                                unsigned int bufLen     = OutputStream::BUF_LEN,
                                bool lineBuffered       = false,
                                StreamOffset streamOffset = (StreamOffset) 0UL )
                                                               throw( Error );
    static OutputStream * append( const char *filepath,
                                  unsigned int bufLen   = OutputStream::BUF_LEN,
                                  bool lineBuffered     = false )
                                                               throw( Error );
};
} // namespace rai

#endif
