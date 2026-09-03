#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#define unlink _unlink
#endif

#include "util/selfpatch.h"
#include "util/hash_util.h"

using namespace rai;

/* patch_myself( argv[ 0 ], patch_filename, patchbuffer )
 *  1. scans binary for magic, reads patch info data buffer (patch_filename)
 *  2. overwrites original binary with a patched binary (progname)
 * return -1 if failed, 0 if success
 */
int
rai::patch_myself( const char *progname,  const char *patchname,
                   patch_buffer &patch )
{
  char buf[ 64 * 1024 ], buf2[ 64 * 1024 ], tmppath[ 1024 ], *p;
  unsigned int prog_crc = 0, patch_crc = 0;

  ::strncpy( tmppath, progname, sizeof( tmppath ) - 1 );
  ::strncat( tmppath, ".tmp", sizeof( tmppath ) - ::strlen( tmppath ) - 1 );

  FILE *patchfp = ::fopen( patchname, "rb" );
  FILE *progfp  = ::fopen( progname, "rb" );
  FILE *outfp   = ::fopen( tmppath, "w+b" );
  int status = -1;
  static const size_t SZ = sizeof( patch.start_edge );

  if ( patchfp == NULL )
    ::perror( patchname );
  else if ( progfp == NULL )
    ::perror( progname );
  else if ( outfp == NULL )
    ::perror( tmppath );
  else {
    size_t n, off, size = 0;
    ::memset( buf, 0, SZ );
    /* read program bytes and write to patched program */
    while ( (n = ::fread( &buf[ SZ ], 1, sizeof( buf ) - SZ, progfp )) > 0 ) {
      if ( ::fwrite( &buf[ SZ ], 1, n, outfp ) != n ) {
        ::perror( tmppath );
        goto done;
      }
      /* find patch buffer */
      for ( off = 0; off < n; ) {
        if ( (p = (char *) ::memchr( &buf[ off ], patch.start_edge[ 0 ],
                                     n + SZ - off )) != NULL ) {
          if ( ::memcmp( p, patch.start_edge, SZ ) == 0 ) {
            /* seek to the buffer edge, which starts SZ bytes after start_edge*/
            if ( ::fseek( outfp, p - &buf[ n ], SEEK_CUR ) == -1 ) {
              ::perror( tmppath );
              goto done;
            }
            goto break_loop;
          }
        }
        else
          break;
        off += &p[ 1 ] - &buf[ off ];
      }
      /* keep SZ bytes in front to handle when wrapped around buf */
      if ( n >= SZ )
        ::memmove( buf, &buf[ n - SZ ], SZ );
    }
    ::fprintf( stderr, "Unable to find \"%s\" in file %s\n",
               patch.start_edge, progname );
    goto done;

  break_loop:;
    /* read patch and write to prog in the patch buffer */
    ::fseek( progfp, ::ftell( outfp ), SEEK_SET );
    while ( (n = ::fread( buf, 1, sizeof( buf ), patchfp )) > 0 ) {
      if ( ::fread( buf2, 1, n, progfp ) != n ) {
        perror( progname );
        goto done;
      }
      /* track the crc of prog and patch, to check if it is already in prog */
      patch_crc = Hash32::crc_c( (byte *) buf, n, patch_crc );
      prog_crc  = Hash32::crc_c( (byte *) buf2, n, prog_crc );
      /* write to patched prog */
      if ( ::fwrite( buf, 1, n, outfp ) != n ) {
        ::perror( tmppath );
        goto done;
      }
      size += n;
    }
    /* check crc of patch and prog */
    patch_crc = Hash32::crc_c( (byte *) (void *) &size, sizeof( size ),
                               patch_crc );
    prog_crc  = Hash32::crc_c( (byte *) (void *) &size, sizeof( size ),
                               prog_crc );
    if ( patch_crc == prog_crc ) {
      /* already there, close files and remove uneeded patched prog */
      //fprintf( stderr, "Binary \"%s\" already contains patch\n", progname );
      status = 1;
      goto done;
    }
    /* zero the rest of the patch buffer in patched program */
    patch.patch_size = size;
    while ( size < sizeof( patch.s.data ) ) {
      n = sizeof( buf );
      if ( size + n > sizeof( patch.s.data ) )
        n = sizeof( patch.s.data ) - size;
      ::memset( buf, 0, n );
      if ( ::fwrite( buf, 1, n, outfp ) != n ) {
        ::perror( tmppath );
        goto done;
      }
      size += n;
    }
    /* check that patch fits inside the buffer */
    if ( size > sizeof( patch.s.data ) ) {
      ::fprintf( stderr, "Patch \"%s\" is too big for buffer (%u)\n",
                 patchname, (unsigned int) sizeof( patch.s.data ) );
      goto done;
    }
    /* record the size of the patch in the patch_size variable in patched prog*/
    if ( ::fwrite( &patch.patch_size, 1, sizeof( patch.patch_size ),
                   outfp ) != sizeof( patch.patch_size ) ) {
      ::perror( tmppath );
      goto done;
    }
    /* read the rest of the program and write it to patched program */
    ::fseek( progfp, ::ftell( outfp ), SEEK_SET );
    while ( (n = ::fread( buf, 1, sizeof( buf ), progfp )) > 0 ) {
      if ( ::fwrite( buf, 1, n, outfp ) != n ) {
        ::perror( tmppath );
        goto done;
      }
    }
    if ( ::fflush( outfp ) == 0 )
      status = 0;
  }
done:;
  /* set the mode of the patched program to be the same as the original */
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  if ( status == 0 ) {
    struct stat st;
    if ( ::fstat( fileno( progfp ), &st ) != 0 ) {
      status = -1;
      perror( progname );
    }
    else if ( ::fchmod( fileno( outfp ), st.st_mode ) != 0 ) {
      status = -1;
      perror( tmppath );
    }
  }
#endif
  if ( patchfp != NULL )
    ::fclose( patchfp );
  if ( progfp != NULL )
    ::fclose( progfp );
  if ( outfp != NULL )
    ::fclose( outfp );
  /* rename the patched program to original prog */
  if ( status == 0 ) {
    if ( ::unlink( progname ) != 0 ) {
      status = -1;
      perror( progname );
    }
    else if ( ::rename( tmppath, progname ) != 0 ) {
      status = -1;
      perror( tmppath );
    }
  }
  /* remove patched program when not needed */
  else {
    ::unlink( tmppath );
  }
  if ( status == 1 )
    return 0;
  return status;
}

/* write patch data to stdout, if any exists
 * return -1 if failed, 0 if success */
int
rai::patch_dump( patch_buffer &patch )
{
  if ( patch.patch_size > 0 ) {
    if ( ::fwrite( patch.s.data, 1, patch.patch_size, stdout ) !=
         patch.patch_size )
      return -1;
    if ( ::fflush( stdout ) != 0 )
      return -1;
  }
  return 0;
}

