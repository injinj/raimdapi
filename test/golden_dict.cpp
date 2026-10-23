/* golden diff: RaiMsg_config from the legacy cfile parser vs from raimd */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msg/dict.h"
#include "msg/msg.h"
#include "base/sys.h"
#include <raimd/md_dict.h>
#include <raimd/dict_load.h>

using namespace rai;

static void
dump_entry( FILE *fp,  const RaiMsg_dict *e,  bool off )
{
  fprintf( fp, "%u %s size %u type %u partial %u", e->fid, e->fname, e->fsize,
           e->ftype, e->partial );
  if ( off )
    fprintf( fp, " off %u", e->foffset );
}

static void
dump( RaiMsg_config *d,  const char *fn )
{
  FILE * fp = fopen( fn, "w" );
  fprintf( fp, "entries %u forms %u reserved %x\n", d->entryCount, d->formCount,
           d->reservedMask );
  fprintf( fp, "msgType %d recType %d seqNo %d recStatus %d\n",
           d->msgType ? d->msgType->fid : -1, d->recType ? d->recType->fid : -1,
           d->seqNo ? d->seqNo->fid : -1, d->recStatus ? d->recStatus->fid : -1 );
  for ( unsigned int fid = 0; fid < MAX_FID; fid++ ) {
    const RaiMsg_dict * e = d->getEntry( (Rai_u16) fid );
    if ( e == NULL )
      continue;
    dump_entry( fp, e, false );
    const RaiMsg_dict * n = d->getEntry( e->fname );
    if ( n != e )
      fprintf( fp, " NAME-LOOKUP-FAIL" );
    const RaiMsg_form * f = d->getForm( (Rai_u16) fid );
    if ( f != NULL ) {
      fprintf( fp, " form size %u fields %u", f->getFormSize(), f->fieldCount );
      fprintf( fp, " shortcuts %d %d %d %d",
               f->msgType ? f->msgType->fid : -1, f->recType ? f->recType->fid : -1,
               f->seqNo ? f->seqNo->fid : -1, f->recStatus ? f->recStatus->fid : -1 );
      if ( d->getForm( e->fname ) != f )
        fprintf( fp, " FORM-NAME-LOOKUP-FAIL" );
      for ( unsigned int i = 0; i < f->fieldCount; i++ ) {
        fprintf( fp, "\n  " );
        dump_entry( fp, &f->fields[ i ], true );
        if ( f->getEntry( f->fields[ i ].fid ) != &f->fields[ i ] )
          fprintf( fp, " FORM-FID-LOOKUP-FAIL" );
        if ( f->getEntry( f->fields[ i ].fname ) != &f->fields[ i ] )
          fprintf( fp, " FORM-FNAME-LOOKUP-FAIL" );
      }
    }
    fprintf( fp, "\n" );
  }
  fclose( fp );
}

int
main( int argc,  char **argv )
{
  const char * path = ( argc > 1 ? argv[ 1 ] : "." );
  const char * out  = ( argc > 2 ? argv[ 2 ] : "/tmp/golden" );
  char fn[ 256 ];
  Sys::initialize();
  RaiMsg_config * d1 = RaiMsg_config::parseDictionary( "tss_fields.cf",
                                                       "tss_records.cf", path );
  if ( d1 == NULL ) { fprintf( stderr, "old parse failed\n" ); return 2; }
  snprintf( fn, sizeof( fn ), "%s.old.txt", out );
  dump( d1, fn );
  fprintf( stderr, "old: entries %u forms %u\n", d1->entryCount, d1->formCount );

  md::MDDict * md = md::load_dict_files( path, true );
  if ( md == NULL ) { fprintf( stderr, "raimd load failed\n" ); return 3; }
  RaiMsg_config * d2 = RaiMsg_config::fromMDDict( md );
  snprintf( fn, sizeof( fn ), "%s.new.txt", out );
  dump( d2, fn );
  fprintf( stderr, "new: entries %u forms %u\n", d2->entryCount, d2->formCount );
  return 0;
}

