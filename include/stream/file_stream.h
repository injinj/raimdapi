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
;
  public:
    SYS_OPS( FileInputStream );
    FileInputStream( File *file,
                     unsigned int bufLen       = InputStream::BUF_LEN,
                     bool closePipe            = false,
                     StreamOffset streamOffset = (StreamOffset) 0UL );
    virtual ~FileInputStream();

    virtual void close( void );

    virtual bool available( void );

    virtual StreamOffset seekSet( StreamSeekOffset offset,  int whence ) 
;
    static InputStream * create( File *file,
                               unsigned int bufLen       = InputStream::BUF_LEN,
                               bool closePipe            = false,
                               StreamOffset streamOffset = (StreamOffset) 0UL )
;
    static InputStream * open( const char *filepath,
                               unsigned int bufLen       = InputStream::BUF_LEN,
                               StreamOffset streamOffset = (StreamOffset) 0UL )
;
};


class RAIBASE_DLL_EXP FileOutputStream : public OutputStream {
  protected:
    File * file;

    virtual unsigned int emptyBuf( const byte *buf,  unsigned int bufLen )
;
  public:
    SYS_OPS( FileOutputStream );
    FileOutputStream( File *file,
                      unsigned int bufLen     = OutputStream::BUF_LEN,
                      bool lineBuffered       = false,
                      bool closePipe          = false,
                      StreamOffset streamOffset = (StreamOffset) 0UL );
    virtual ~FileOutputStream();

    virtual void close( void );

    virtual StreamOffset seekSet( StreamSeekOffset offset,  int whence )
;
    static OutputStream * create( File *file,
                                unsigned int bufLen   = OutputStream::BUF_LEN,
                                bool lineBuffered     = false,
                                bool closePipe        = false,
                                StreamOffset streamOffset = (StreamOffset) 0UL )
;
    static OutputStream * open( const char *filepath,
                                unsigned int bufLen     = OutputStream::BUF_LEN,
                                bool lineBuffered       = false,
                                StreamOffset streamOffset = (StreamOffset) 0UL )
;
    static OutputStream * append( const char *filepath,
                                  unsigned int bufLen   = OutputStream::BUF_LEN,
                                  bool lineBuffered     = false )
;
};
} // namespace rai

#endif
