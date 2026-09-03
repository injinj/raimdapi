#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "base/sys.h"
#include "base/mem.h"
#include "util/tarfs.h"
#include "util/array.h"
#include "util/hash_util.h"
#include "stream/byte_array_stream.h"

using namespace rai;

struct TarFSImpl : public TarFS {
  static const unsigned TARBUFSZ = 256 * 1024;
  Array<byte *>  bufs;
  ullong         tarsz,
               * offset;
  Entry        * entry;
  unsigned int * ht,
                 count,
                 htsize;

  SYS_OPS( TarFSImpl );
  TarFSImpl () : tarsz( 0 ), offset( 0 ), entry( 0 ), ht( 0 ), count( 0 ),
                 htsize( 0 ) {}
  virtual ~TarFSImpl();

  virtual InputStream *openFile( const char *fname,
                                 Entry *entry ) throw( Error );
  virtual bool firstEntry( unsigned int &i,  Entry &entry ) throw( Error );
  
  virtual bool nextEntry( unsigned int &i,  Entry &entry ) throw( Error );

  unsigned int copyBytes( ullong off,  unsigned int size,  void *buf );
};

/* align byte fields */
#pragma pack(1)
struct tar_header {
   char name[ 100 ];       /*   NUL-terminated if NUL fits */
   char mode[ 8 ];
   char uid[ 8 ];
   char gid[ 8 ];
   char size[ 12 ];
   char mtime[ 12 ];
   char chksum[ 8 ];
   char typeflag[ 1 ];     /*   see below */
   char linkname[ 100 ];   /*   NUL-terminated if NUL fits */
   char magic[ 6 ];        /*   must be TMAGIC (NUL term.) */
   char version[ 2 ];      /*   must be TVERSION */
   char uname[ 32 ];       /*   NUL-terminated */
   char gname[ 32 ];       /*   NUL-terminated */
   char devmajor[ 8 ];
   char devminor[ 8 ];
   char prefix[ 155 ];     /*   NUL-terminated if NUL fits */
   char trail[ 12 ];
};
/* no more alignment */
#pragma pack()

static ullong
uintoctal( const char *s,  unsigned int len )
{
  ullong val = 0;
  while ( len > 0 && ( *s <= '0' || *s > '7' ) ) {
    len--; s++;
  }
  while ( len > 0 && ( *s >= '0' && *s <= '7' ) ) {
    val = ( val << 3 ) + ( *s - '0' );
    len--; s++;
  }
  return val;
}

static void
convertToEntry( tar_header &hdr,  TarFS::Entry &entry )
{
  unsigned int i, j = 0;

  /* most likely will be empty */
  for ( i = 0; i < sizeof( hdr.prefix ); i++ ) {
    if ( hdr.prefix[ i ] == 0 )
      break;
    entry.fname[ j++ ] = hdr.prefix[ i ];
  }
  if ( j > 0 ) {
    if ( entry.fname[ j - 1 ] != '/' )
      entry.fname[ j++ ] = '/';
  }
  /* file or dir name */
  for ( i = 0; i < sizeof( hdr.name ); i++ ) {
    entry.fname[ j++ ] = hdr.name[ i ];
    if ( hdr.name[ i ] == 0 )
      break;
  }
  ::strncpy( entry.uname, hdr.uname, sizeof( entry.uname ) );
  ::strncpy( entry.gname, hdr.gname, sizeof( entry.gname ) );
  entry.mode  = uintoctal( hdr.mode, sizeof( hdr.mode ) );
  entry.uid   = uintoctal( hdr.uid, sizeof( hdr.uid ) );
  entry.gid   = uintoctal( hdr.gid, sizeof( hdr.gid ) );
  entry.mtime = uintoctal( hdr.mtime, sizeof( hdr.mtime ) );
  entry.size  = uintoctal( hdr.size, sizeof( hdr.size ) );
}

TarFS *
TarFS::create( InputStream *input ) throw( Error )
{
  static const char zeromagic[] = { 0, 0, 0, 0, 0, 0 };
  TarFSImpl  * fs  = NEW TarFSImpl();
  byte       * buf = NULL;
  unsigned int off, n, count, i;
  tar_header   tarhdr;
  ullong       taroff;

  try {
    for (;;) {
      MALLOC( TarFSImpl::TARBUFSZ, &buf );
      for ( off = 0; off < TarFSImpl::TARBUFSZ; ) {
        n = input->readBytes( &buf[ off ], TarFSImpl::TARBUFSZ - off );
        if ( n == 0 )
          break;
        off += n;
      }
      if ( off == 0 ) {
        FREE( buf );
        break;
      }
      fs->bufs.pushTail( buf );
      fs->tarsz += off;
      buf = NULL;
      if ( off < TarFSImpl::TARBUFSZ )
        break;
      if ( fs->tarsz > 1024 * 1024 * 1024 )
        throw TarErr::getErr( TarErr::TAR_TOO_BIG );
    }
    count = 0;
    for ( taroff = 0; taroff < fs->tarsz; ) {
      if ( fs->copyBytes( taroff, sizeof( tarhdr ),
                          &tarhdr ) != sizeof( tarhdr ) )
        throw TarErr::getErr( TarErr::TRUNC_TAR );
      taroff += 512;
      if ( ::memcmp( tarhdr.magic, zeromagic, sizeof( tarhdr.magic ) ) == 0 )
        continue;
      if ( ::strncmp( tarhdr.magic, "ustar", 5 ) != 0 )
        throw TarErr::getErr( TarErr::NOT_TAR_FMT );

      taroff += uintoctal( tarhdr.size, sizeof( tarhdr.size ) );
      if ( taroff % 512 != 0 )
        taroff += ( 512 - ( taroff % 512 ) );
      count++;
    }
    if ( count > 0 ) {
      fs->htsize = ( count * 2 ) + 13;
      MALLOC( count * sizeof( TarFS::Entry ), &fs->entry );
      MALLOC( count * sizeof( fs->offset[ 0 ] ), &fs->offset );
      MALLOC( fs->htsize * sizeof( fs->ht[ 0 ] ), &fs->ht );
      ::memset( fs->entry, 0, count * sizeof( TarFS::Entry ) );
      ::memset( fs->offset, 0, count * sizeof( fs->offset[ 0 ] ) );
      ::memset( fs->ht, 0, fs->htsize * sizeof( fs->ht[ 0 ] ) );

      fs->count = count;
      taroff = 0;
      for ( count = 0; count < fs->count; ) {
        if ( fs->copyBytes( taroff, sizeof( tarhdr ),
                            &tarhdr ) != sizeof( tarhdr ) )
          throw TarErr::getErr( TarErr::TRUNC_TAR );
        taroff += 512;
        if ( ::memcmp( tarhdr.magic, zeromagic, sizeof( tarhdr.magic ) ) == 0 )
          continue;
        if ( ::strncmp( tarhdr.magic, "ustar", 5 ) != 0 )
          throw TarErr::getErr( TarErr::NOT_TAR_FMT );

        convertToEntry( tarhdr, fs->entry[ count ] );
        fs->offset[ count ] = taroff;
        taroff += fs->entry[ count ].size;
        if ( taroff % 512 != 0 )
          taroff += ( 512 - ( taroff % 512 ) );
        count++;
        i = Hash32::crc_cs( fs->entry[ count - 1 ].fname ) % fs->htsize;
        while ( fs->ht[ i ] != 0 )
          i = ( i + 1 ) % fs->htsize;
        fs->ht[ i ] = count;
      }
      fs->count = count;
    }
  } catch ( ... ) {
    if ( fs != NULL )
      delete fs;
    if ( buf != NULL )
      FREE( buf );
    throw;
  }
  return fs;
}

TarFSImpl::~TarFSImpl()
{
  while ( ! this->bufs.isEmpty() ) {
    byte *buf = this->bufs.popTail();
    FREE( buf );
  }
  if ( this->entry != NULL )
    FREE( this->entry );
  if ( this->offset != NULL )
    FREE( this->offset );
  if ( this->ht != NULL )
    FREE( this->ht );
}

unsigned int 
TarFSImpl::copyBytes( ullong off,  unsigned int size,  void *buf )
{
  unsigned int cp = 0, i, n, fill, end = TARBUFSZ;
  i = (unsigned int) ( off / TARBUFSZ );
  n = (unsigned int) ( off % TARBUFSZ );
  for ( ; i < this->bufs.length(); i++ ) {
    if ( off >= this->tarsz )
      break;
    if ( i == this->bufs.length() - 1 )
      end = this->tarsz % TARBUFSZ;
    fill = size - cp;
    if ( fill > end - n )
      fill = end - n;
    ::memcpy( &((byte *) buf)[ cp ], &(this->bufs.get( i ))[ n ], fill );
    cp += fill;
    if ( cp == size )
      break;
    off += fill;
    n = 0;
  }
  return cp;
}

class TarFSInputStream : public InputStream {
protected:
  TarFSImpl  & tarfs;
  const ullong taroff, tarend;
  ullong       seekptr;

  virtual unsigned int fillBuf( byte *buf,  unsigned int bufLen )
                                                             throw( Error );
public:
  SYS_OPS( TarFSInputStream );

  TarFSInputStream( TarFSImpl &fs,  ullong off,  ullong len ) :
    tarfs( fs ), taroff( off ), tarend( off + len ), seekptr( 0 ) {}
  virtual ~TarFSInputStream() {}
  virtual bool available( void )                             throw( Error );
  virtual StreamOffset seekSet( StreamSeekOffset offset,  int whence )
                                                             throw( Error );
};

unsigned int
TarFSInputStream::fillBuf( byte *buf,  unsigned int bufLen ) throw( Error )
{
  ullong position = this->seekptr + this->taroff;
  if ( bufLen > this->tarend - position )
    bufLen = this->tarend - position;
  this->seekptr += bufLen;
  return this->tarfs.copyBytes( position, bufLen, buf );
}

bool
TarFSInputStream::available( void ) throw( Error )
{
  if ( this->taroff + this->seekptr < this->tarend ||
       this->InputStream::available() )
    return true;
  return false;
}

StreamOffset
TarFSInputStream::seekSet( StreamSeekOffset offset,  int whence ) throw( Error )
{
  switch( whence ) {
    case IOStream::IO_SEEK_SET:
      break;
    case IOStream::IO_SEEK_END:
      offset += (StreamSeekOffset) ( this->tarend - this->taroff );
      break;
    case IOStream::IO_SEEK_CUR:
      offset += (StreamSeekOffset) this->getStreamOffset();
      break;
  }

  if ( (ullong) offset > ( this->tarend - this->taroff ) )
    throw IOStreamErr::getErr( IOStreamErr::BAD_SEEK );

  this->streamOffset = (StreamOffset) offset;
  this->offset       = 0;
  this->length       = 0;
  this->endOfFile    = false;
  this->seekptr      = (ullong) offset;

  return this->streamOffset;
}

InputStream *
TarFSImpl::openFile( const char *fname,  Entry *entry ) throw( Error )
{
  if ( this->htsize == 0 )
    return NULL;
  unsigned int i, h = Hash32::crc_cs( fname ) % this->htsize;
  for (;;) {
    i = this->ht[ h ];
    if ( i == 0 ) {
      if ( entry != NULL )
        ::memset( entry, 0, sizeof( Entry ) );
      return NULL;
    }
    if ( ::strcmp( fname, this->entry[ i - 1 ].fname ) == 0 ) {
      if ( entry != NULL )
        *entry = this->entry[ i - 1 ];
      return NEW TarFSInputStream( *this, this->offset[ i - 1 ],
                                    this->entry[ i - 1 ].size );
    }
    h = ( h + 1 ) % this->htsize;
  }
}

bool
TarFSImpl::firstEntry( unsigned int &i,  Entry &entry ) throw( Error )
{
  i = 0;
  return this->nextEntry( i, entry );
}

bool
TarFSImpl::nextEntry( unsigned int &i,  Entry &entry ) throw( Error )
{
  if ( i >= this->count )
    return false;
  entry = this->entry[ i++ ];
  return true;
}

Error
TarErr::getErr( unsigned int status )
{
  static const char     mod[] = "Tar";
  static const ErrorRec err[] = {
  /*  0 */ { NOT_TAR_FMT, "Input not in 'tar' format (no ustar @257)", mod },
  /*  1 */ { TAR_TOO_BIG, "Input 'tar' is too big (more that 1GB)", mod },
  /*  2 */ { TRUNC_TAR,   "Truncated 'tar', header not found", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}

#ifdef TEST

int
main( int argc, char *argv[] )
{
  InputStream *in;
  unsigned int i, n;
  byte buf[ 8024 ];

  Sys::initialize();

  try {
    TarFS *tarfs = TarFS::create( Sys::in );
    TarFS::Entry entry;
    Sys::err->printf( "sz %u\n", sizeof( tar_header ) );
    in = tarfs->openFile( "doc/cache-admin-guide.html", &entry );
    while ( (n = in->readBytes( buf, sizeof( buf ) )) > 0 ) {
      Sys::out->writeBytes( buf, n );
    }
  } catch ( Error e ) {
    Sys::err->printf( "err: %s+%u: %s\n", e->module, e->status, e->reason );
  }

  return 0;
}

#endif
