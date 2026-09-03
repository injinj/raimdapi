/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "base/file.h"
#include "stream/file_stream.h"
#include "stream/stdio_stream.h"
#include "base/mem.h"
#include "base/thread.h"

using namespace rai;

FileInputStream::FileInputStream( File *file,  unsigned int bufLen,
                                  bool closePipe,  StreamOffset streamOffset )
               : InputStream( bufLen, closePipe, streamOffset )
{
  this->file = file;
}


FileInputStream::~FileInputStream()
{
  if ( this->file != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


InputStream *
FileInputStream::create( File *file,  unsigned int bufLen,  bool closePipe,
                         StreamOffset streamOffset )
                 throw( Error )
{
  return NEW FileInputStream( file, bufLen, closePipe, streamOffset );
}


InputStream *
FileInputStream::open( const char *filepath,  unsigned int bufLen,
                       StreamOffset streamOffset )
                 throw( Error )
{
  File * file;

  file = File::openFile( filepath, File::FILE_RDONLY );

  try {
    if ( ::strcmp( filepath, STDIN_SPECIAL_FILE ) == 0 )
      return NEW StdioInputStream( file, bufLen );
    return FileInputStream::create( file, bufLen, true, streamOffset );
  } catch( Error e ) {
    delete file;
    throw e;
  }
}


void
FileInputStream::close( void ) throw( Error )
{
  Error e2;
  File * file;

  if ( this->lock != NULL )
    this->lock->lock();

  e2 = NULL;
  if ( this->file != NULL ) {
    try {
      this->InputStream::close();
    } catch( Error e ) {
      e2 = e;
    }
    file = this->file;
    this->file = NULL;

    if ( this->closePipe && file != NULL ) {
      try {
        file->close();
      } catch( Error e ) {
        if ( e2 == NULL )
          e2 = e;
      }
      delete file;
    }
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


StreamOffset
FileInputStream::seekSet( StreamSeekOffset offset,  int whence ) throw( Error )
{
  if ( this->lock != NULL )
    this->lock->lock();

  try {
    if ( this->file == NULL )
      throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

    switch ( whence ) {
      case IOStream::IO_SEEK_SET:
        break;
      case IOStream::IO_SEEK_END:
        offset += (StreamSeekOffset) this->file->length();
        break;
      case IOStream::IO_SEEK_CUR:
        offset += (StreamSeekOffset) this->getStreamOffset();
        break;
    }

    this->streamOffset = (StreamOffset) offset;
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
FileInputStream::available( void ) throw( Error )
{
  StreamOffset off;
  bool         avail;

  if ( this->lock != NULL )
    this->lock->lock();

  avail = false;
  try {
    if ( this->file != NULL ) {
      if ( this->InputStream::available() )
        avail = true;
      else {
        off = this->getStreamOffset();
        if ( off < this->file->length() )
          avail = true;
      }
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
FileInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  if ( this->file == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

  return this->file->readAt( buf, bufLen, this->streamOffset );
}


FileOutputStream::FileOutputStream( File *file,  unsigned int bufLen,
                                    bool lineBuffered,  bool closePipe,
                                    StreamOffset streamOffset ) :
                  OutputStream( bufLen, lineBuffered, closePipe, streamOffset )
{
  this->file = file;
}


FileOutputStream::~FileOutputStream()
{
  if ( this->file != NULL ) {
    try {
      this->close();
    } catch ( ... ) {
    }
  }
}


OutputStream *
FileOutputStream::create( File *file,  unsigned int bufLen,  bool lineBuffered,
                          bool closePipe,  StreamOffset streamOffset )
                  throw( Error )
{
  return NEW FileOutputStream( file, bufLen, lineBuffered, closePipe,
                               streamOffset );
}


OutputStream *
FileOutputStream::open( const char *filepath,  unsigned int bufLen,
                        bool lineBuffered,  StreamOffset streamOffset )
                  throw( Error )
{
  File * file;

  if ( streamOffset == 0 )
    file = File::openFile( filepath, File::FILE_WRONLY | File::FILE_CREAT |
                                     File::FILE_TRUNC );
  else
    file = File::openFile( filepath, File::FILE_WRONLY | File::FILE_CREAT );

  try {
    if ( streamOffset != 0 )
      file->seekSet( streamOffset, File::IO_SEEK_SET );
    if ( ::strcmp( filepath, STDOUT_SPECIAL_FILE ) == 0 ||
         ::strcmp( filepath, STDERR_SPECIAL_FILE ) == 0 )
      return NEW StdioOutputStream( file, bufLen, lineBuffered );
    return FileOutputStream::create( file, bufLen, lineBuffered, true,
                                     streamOffset );
  } catch( Error e ) {
    delete file;
    throw e;
  }
}


OutputStream *
FileOutputStream::append( const char *filepath,  unsigned int bufLen,
                          bool lineBuffered )
                  throw( Error )
{
  File         * file;
  OutputStream * fileOutput;

  file = File::openFile( filepath, File::FILE_WRONLY | File::FILE_APPEND |
                                   File::FILE_CREAT );

  try {
    if ( ::strcmp( filepath, STDOUT_SPECIAL_FILE ) == 0 ||
         ::strcmp( filepath, STDERR_SPECIAL_FILE ) == 0 )
      fileOutput = NEW StdioOutputStream( file, bufLen, lineBuffered );
    else
      fileOutput = FileOutputStream::create( file, bufLen, lineBuffered, true,
                                             file->length() );
    return fileOutput;
  } catch( Error e ) {
    delete file;
    throw e;
  }
}


void
FileOutputStream::close( void ) throw( Error )
{
  Error e2;
  File * file;

  if ( this->lock != NULL )
    this->lock->lock();

  e2 = NULL;
  if ( this->file != NULL ) {
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
    file = this->file;
    this->file = NULL;

    if ( this->closePipe ) {
      try {
        file->close();
      } catch( Error e ) {
        if ( e2 == NULL )
          e2 = e;
      }
      delete file;
    }
  }
  if ( this->lock != NULL )
    this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}


StreamOffset
FileOutputStream::seekSet( StreamSeekOffset offset,  int whence ) throw( Error )
{
  if ( this->lock != NULL )
    this->lock->lock();

  try {
    this->flush();

    switch ( whence ) {
      case IOStream::IO_SEEK_SET:
        break;
      case IOStream::IO_SEEK_END:
        offset += (StreamSeekOffset) this->file->length();
        break;
      case IOStream::IO_SEEK_CUR:
        offset += (StreamSeekOffset) this->getStreamOffset();
        break;
    }

    this->streamOffset = (StreamOffset) offset;

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
FileOutputStream::emptyBuf( const byte *buf,  unsigned int bufLen )
                  throw( Error )
{
  if ( this->file == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );
  return this->file->writeAt( buf, bufLen, this->streamOffset );
}

