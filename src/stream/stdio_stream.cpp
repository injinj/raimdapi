/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include "stream/stdio_stream.h"
#include "base/mem.h"
#include "base/file.h"

using namespace rai;

StdioInputStream::StdioInputStream( File *file,  unsigned int bufLen ) :
                  FileInputStream( file, bufLen, true )
{
}


unsigned int
StdioInputStream::fillBuf( byte *buf,  unsigned int bufLen )

{
  if ( this->file == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

  return this->file->read( buf, bufLen );
}


bool
StdioInputStream::available( void )
{
  if ( this->file == NULL )
    return false;

  return this->InputStream::available();
}


InputStream *
StdioInputStream::createStdin( unsigned int bufLen )
{
  File * file;

  file = File::openFile( STDIN_SPECIAL_FILE, File::FILE_RDONLY );

  try {
    return NEW StdioInputStream( file, bufLen );
  } catch( Error e ) {
    delete file;
    throw e;
  }
}


StdioOutputStream::StdioOutputStream( File *file,  unsigned int bufLen,
                                      bool lineBuffered ) :
                   FileOutputStream( file, bufLen, lineBuffered, true )
{
}


unsigned int
StdioOutputStream::emptyBuf( const byte *buf,  unsigned int bufLen )

{
  if ( this->file == NULL )
    throw IOStreamErr::getErr( IOStreamErr::NOT_OPEN );

  return this->file->write( buf, bufLen );
}


OutputStream *
StdioOutputStream::createStdout( unsigned int bufLen )
{
  File * file;

  file = File::openFile( STDOUT_SPECIAL_FILE, File::FILE_WRONLY );

  try {
    return NEW StdioOutputStream( file, bufLen, false );
  } catch( Error e ) {
    delete file;
    throw e;
  }
}


OutputStream *
StdioOutputStream::createStderr( unsigned int bufLen )
{
  File * file;

  file = File::openFile( STDERR_SPECIAL_FILE, File::FILE_WRONLY );

  try {
    return NEW StdioOutputStream( file, bufLen, true );
  } catch( Error e ) {
    delete file;
    throw e;
  }
}

