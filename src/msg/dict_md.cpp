/* build the RaiMsg_config / RaiMsg_form / RaiMsg_dict structures from a
 * raimd::MDDict cfile dictionary (the output of raimd CFile::parse_path(),
 * CFile::unpack_sass() or CFile::unpack_sass3()) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "msg/dict.h"
#include "msg/msg.h"
#include "msg/defs.h"
#include "util/hash_util.h"
#include "base/log.h"
#include "base/mem.h"

#include <raimd/md_dict.h>
#include <raimd/cfile.h>

using namespace rai;
using namespace rai::md;

static const char MSG_TYPE_STRING[]   = "MSG_TYPE";
static const char REC_TYPE_STRING[]   = "REC_TYPE";
static const char SEQ_NO_STRING[]     = "SEQ_NO";
static const char REC_STATUS_STRING[] = "REC_STATUS";

/* the TSS type / size / partial flag of a cfile entry, as pack_sass encodes
 * it; the MDDict does not keep the TSS type, raimd derives it the same way */
static inline void
md_entry_tss( MDLookup &by,  Rai_u32 &dataType,  Rai_u32 &dataSize,
              bool &isPartial )
{
  Rai_u32 sz = 0;
  dataType  = (Rai_u32) CFile::md_to_tss_type( by, sz );
  dataSize  = sz;
  isPartial = ( by.ftype == MD_PARTIAL );
}

static inline bool
md_is_form( MDDict *cfile,  MDLookup &by,  MDFormClass *&fc )
{
  fc = NULL;
  if ( by.ftype != MD_MESSAGE || by.map_num == 0 )
    return false;
  fc = cfile->get_form_class( by.fid );
  return fc != NULL;
}

RaiMsg_config *
RaiMsg_config::fromMDDict( MDDict *cfile )
{
  Rai_u16         fieldFid[ MAX_FID ],
                * index;
  Rai_u32         entryCount = 0,
                  classCount = 0,
                  formCount  = 0,
                  indexSize  = 0,
                  stringLen  = 0,
                  dataType,
                  dataSize,
                  i,
                  count;
  Rai_u8          fname_size;
  bool            isPartial;
  char          * fname;
  RaiMsg_dict   * entry;
  RaiMsg_form   * form;
  RaiMsg_config * dict;
  MDFormClass   * fc;
  Error           tssErr;

  /* find the cfile dict in a list from load_dict_files() / unpack_sass3() */
  for ( ; cfile != NULL; cfile = cfile->get_next() ) {
    if ( cfile->dict_type[ 0 ] == 'c' )
      break;
  }
  if ( cfile == NULL ) {
    tssErr = RaiMsgErr::getErr( RaiMsgErr::MISSING_FIDS_MSG );
    logError( LERROR, tssErr, "No cfile dictionary in MDDict list" );
    throw tssErr;
  }
  if ( cfile->max_fid >= (MDFid) MAX_FID || cfile->min_fid < 0 ) {
    tssErr = RaiMsgErr::getErr( RaiMsgErr::BIG_MAX_FID );
    logError( LERROR, tssErr, "MAX_FID (%d) is larger than MAX_ENTRIES (%u)",
              cfile->max_fid, MAX_FID );
    throw tssErr;
  }
  /* pass 1: sizes */
  for ( MDFid fid = cfile->min_fid; fid <= cfile->max_fid; fid++ ) {
    MDLookup by( fid );
    if ( ! cfile->lookup( by ) )
      continue;
    /* raimd fname_len includes the terminating nul, as fname_size does */
    if ( by.fname_len <= 1 || by.fname_len >= 128 ) {
      tssErr = RaiMsgErr::getErr( RaiMsgErr::NULL_DICT_FNAME );
      logError( LERROR, tssErr, "Bad class name length (%u) fid %d",
                by.fname_len, fid );
      throw tssErr;
    }
    stringLen += by.fname_len;
    entryCount++;
    classCount++;
    if ( md_is_form( cfile, by, fc ) ) {
      count = fc->nentries;
      if ( count > MAX_FID ) {
        tssErr = RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS );
        logError( LERROR, tssErr, "Form %.*s has too many fields (%u)",
                  (int) by.fname_len, by.fname, count );
        throw tssErr;
      }
      for ( i = 0; i < count; i++ )
        fieldFid[ i ] = (Rai_u16) fc->entries[ i ].fid;
      formCount++;
      indexSize  += RaiMsg_form::calcIndexSize( fieldFid, count );
      entryCount += count; /* the form fields */
    }
  }
  if ( classCount == 0 || classCount > MAX_FID ) {
    tssErr = RaiMsgErr::getErr( RaiMsgErr::TOO_MANY_CLASSES );
    logError( LERROR, tssErr, "Bad dictionary entry count (%u)", classCount );
    throw tssErr;
  }
  dict = NULL;
  unsigned int sz1 = ( sizeof( RaiMsg_config ) + 7 ) & ~7,
               sz2 = ( ( entryCount * sizeof( RaiMsg_dict ) ) + 7 ) & ~7,
               sz3 = ( ( formCount * sizeof( RaiMsg_form ) ) + 7 ) & ~7,
               sz4 = ( indexSize * sizeof( Rai_u16 ) + 7 ) & ~7;
  MALLOC( sz1 + sz2 + sz3 + sz4 + stringLen, &dict );
  dict->init();
  dict->entry = (RaiMsg_dict *) &((Rai_u8 *) dict)[ sz1 ];
  dict->form  = (RaiMsg_form *) &((Rai_u8 *) dict)[ sz1 + sz2 ];
  index       = (Rai_u16 *) &((Rai_u8 *) dict)[ sz1 + sz2 + sz3 ];
  fname       = (char *)  &((Rai_u8 *) dict)[ sz1 + sz2 + sz3 + sz4 ];
  dict->dictSize = sz1 + sz2 + sz3 + sz4 + stringLen;

  try {
    /* pass 2: the class entries, forms included (ftype NULL, fsize later) */
    for ( MDFid fid = cfile->min_fid; fid <= cfile->max_fid; fid++ ) {
      MDLookup by( fid );
      if ( ! cfile->lookup( by ) )
        continue;
      fname_size = (Rai_u8) by.fname_len;
      ::memcpy( fname, by.fname, by.fname_len );
      if ( md_is_form( cfile, by, fc ) ) {
        dataType  = RAI_TSS_NULL;
        dataSize  = 0;
        isPartial = false;
      }
      else {
        md_entry_tss( by, dataType, dataSize, isPartial );
      }
      i = Hash32::crc_c( (Rai_u8 *) fname, fname_size - 1 );
      for ( i &= ( MAX_FID - 1 ); dict->hashIndex[ i ] != NULL;
            i = ( i + 1 ) & ( MAX_FID - 1 ) ) {
        if ( dict->hashIndex[ i ]->fname_size == fname_size &&
             ::memcmp( dict->hashIndex[ i ]->fname, fname,
                       fname_size ) == 0 ) {
          tssErr = RaiMsgErr::getErr( RaiMsgErr::DUPLICATE_CLASS );
          logError( LERROR, tssErr,
                    "Duplicate class entry name=%s, fid %u/%u",
                    fname, (Rai_u32) fid, (Rai_u32) dict->hashIndex[ i ]->fid );
          throw tssErr;
        }
      }
      dict->fidIndex[ fid ] = dict->entryCount;
      entry = &dict->entry[ dict->entryCount++ ];
      entry->init( fname, fname_size );
      entry->fsize   = (Rai_u16) dataSize;
      entry->fid     = (Rai_u16) fid;
      entry->partial = isPartial ? 1 : 0;
      entry->ftype   = (Rai_u8) dataType;
      dict->hashIndex[ i ] = entry;
      fname = &fname[ fname_size ];
    }
    /* pass 3: the forms, field entries are at the end of entry[] */
    for ( MDFid fid = cfile->min_fid; fid <= cfile->max_fid; fid++ ) {
      MDLookup by( fid );
      if ( ! cfile->lookup( by ) || ! md_is_form( cfile, by, fc ) )
        continue;
      count = fc->nentries;
      dict->fidForm[ fid ] = dict->formCount;
      form  = &dict->form[ dict->formCount++ ];
      entry = &dict->entry[ dict->fidIndex[ fid ] ];
      form->init( dict );
      form->entry      = entry;
      form->fieldCount = count;
      entryCount  -= count;
      form->fields = &dict->entry[ entryCount ];
      for ( i = 0; i < count; i++ ) {
        fieldFid[ i ] = (Rai_u16) fc->entries[ i ].fid;
        if ( dict->getEntry( fieldFid[ i ] ) == NULL ) {
          tssErr = RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS );
          logError( LERROR, tssErr,
                    "Form %s field fid (%u) has no class entry",
                    form->entry->fname, (unsigned int) fieldFid[ i ] );
          throw tssErr;
        }
      }
      form->fidIndex = index;
      form->initFields( fieldFid, count, true );
      index = &index[ form->indexSize() ];
    }
    dict->msgType   = dict->getEntry( MSG_TYPE_STRING,
                                      sizeof( MSG_TYPE_STRING ) );
    dict->recType   = dict->getEntry( REC_TYPE_STRING,
                                      sizeof( REC_TYPE_STRING ) );
    dict->seqNo     = dict->getEntry( SEQ_NO_STRING,
                                      sizeof( SEQ_NO_STRING ) );
    dict->recStatus = dict->getEntry( REC_STATUS_STRING,
                                      sizeof( REC_STATUS_STRING ) );
    dict->reservedMask = 0;
    if ( dict->msgType != NULL )
      dict->reservedMask |= dict->msgType->fid;
    if ( dict->recType != NULL )
      dict->reservedMask |= dict->recType->fid;
    if ( dict->seqNo != NULL )
      dict->reservedMask |= dict->seqNo->fid;
    if ( dict->recStatus != NULL )
      dict->reservedMask |= dict->recStatus->fid;
    return dict;
  } catch ( ... ) {
    FREE( dict );
    throw;
  }
}

