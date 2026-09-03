/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__dict_h__
#define __rai_msg__dict_h__
 
#if ! defined( RAIMSG_DLL_EXP )
#if defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllimport)
#else
#define RAIMSG_DLL_EXP
#endif
#endif

#ifndef __rai_msg__types_h__
#include "msg/types.h"
#endif

struct RaiMsg;
struct RaiField;
struct RaiMsg_config;
struct RaiMsg_dtab;
struct RaiMsg_ttab;
struct RaiMsg_formref;
struct RaiMsg_form;
namespace rai {
  class CFileExpr;
  class CFileLocator;
  class InputStream;
  class Mutex;
  extern RAIMSG_DLL_EXP
  RaiMsg_config *DataDictionary;
  extern RAIMSG_DLL_EXP
  RaiMsg        *PackedDataDictionaryFull;
}

/*extern const char MSG_TYPE_STRING[];
extern const char REC_TYPE_STRING[];
extern const char SEQ_NO_STRING[];
extern const char REC_STATUS_STRING[];*/
static const Rai_u16 MAX_FID            = 0x4000U,
                     FID_FIXED_FLAG     = 0x8000U,
                     FID_PRIMITIVE_FLAG = 0x4000U,
                     FID_CTRL           = (FID_FIXED_FLAG | FID_PRIMITIVE_FLAG),
                     TSS_SDATE_HINT     = 256,
                     TSS_STIME_HINT     = 257;

struct RAIMSG_DLL_EXP RaiMsg_dict {
  RaiMsg_name fname;
  Rai_u16     fsize,
              fid,
              foffset;
  Rai_u8      ftype;
  Rai_u32     partial    : 1,
              fname_size : 7;

  void init( RaiMsg_name fname_ptr,  Rai_u8 fname_sz ) {
    this->fname      = fname_ptr;
    this->fsize      = 0;
    this->fid        = 0;
    this->foffset    = 0;
    this->ftype      = RAI_TSS_NULL;
    this->partial    = 0;
    this->fname_size = fname_sz;
  }

  RaiMsg_size partialPackSize( Rai_u16 partialLen ) const {
    return ( 3U + ( ( (RaiMsg_size) partialLen + 1U ) >> 1 ) ) << 1;
  }

  RaiMsg_size packSize( void ) const {
    if ( this->partial )
      return this->partialPackSize( this->fsize );
    return ( 1U + ( ( (RaiMsg_size) this->fsize + 1U ) >> 1 ) ) << 1;
  }
  /* determine the number of significant digits in a real for the grocery hint*/
  static Rai_u8 convertTssPrecision( Rai_f64 );

  void convert( RaiField &field ) const                throw( RaiMsgException );

  void pack( RaiField &field,  Rai_u8 *to_ptr ) const  throw( RaiMsgException );

  void packPartial( RaiField &field,  Rai_u8 *to_ptr ) const
                                                       throw( RaiMsgException );
  Rai_u8 *unpack( RaiField &field,  Rai_u8 *from_ptr,
                  unsigned int length ) const          throw( RaiMsgException );
  RaiMsg_type getFieldType( void ) const;
};


struct RAIMSG_DLL_EXP RaiMsg_form {
  RaiMsg_config     * dict;
  RaiMsg_dict       * entry,
                    * fields;
  const RaiMsg_dict * msgType,
                    * recType,
                    * seqNo,
                    * recStatus;
  Rai_u16           * fidIndex;
  Rai_u64             typeHash,
                      fidHash;
  Rai_u32             fieldCount;
  Rai_u16             fidMask;
  Rai_u8              fidBits,
                      fieldBits;

  void init( RaiMsg_config *dict ) {
    this->dict        = dict;
    this->entry       = NULL;
    this->fields      = NULL;
    this->msgType     = NULL;
    this->recType     = NULL;
    this->seqNo       = NULL;
    this->recStatus   = NULL;
    this->fidIndex    = NULL;
    this->typeHash    = 0;
    this->fidHash     = 0;
    this->fieldCount  = 0;
    this->fidMask     = 0;
    this->fidBits     = 0;
    this->fieldBits   = 0;
  }

  const RaiMsg_dict *getEntryByName( const char *fname,
                                     unsigned int fname_size ) const {
    for ( unsigned int i = 0; i < this->fieldCount; i++ ) {
      if ( this->fields[ i ].fname_size == fname_size &&
           ::memcmp( this->fields[ i ].fname, fname, fname_size ) == 0 ) 
        return &this->fields[ i ];
    }
    return NULL;
  }

  const RaiMsg_dict *getEntryByName( const char *fname ) const {
    return this->getEntryByName( fname, (unsigned int) ::strlen( fname ) + 1 );
  }

  const RaiMsg_dict *getEntryByClass( const char *fname,
                                      unsigned int fname_size ) const;

  const RaiMsg_dict *getEntryByClass( const char *fname ) const {
    return this->getEntryByClass( fname, (unsigned int) ::strlen( fname ) + 1 );
  }

  const RaiMsg_dict *getEntry( const char *fname ) const {
    return this->getEntryByClass( fname, (unsigned int) ::strlen( fname ) + 1 );
  }

  const RaiMsg_dict *getEntry( const char *fname,
                               unsigned int fname_size ) const {
    return this->getEntryByClass( fname, fname_size );
  }

  const RaiMsg_dict *getEntry( Rai_u16 fid ) const {
    const RaiMsg_dict * entry;
    unsigned int        bitOffset;
    Rai_u16           * index;
  #if 0
    if ( (entry = this->dict->getEntry( fid )) == NULL )
      return NULL;
    return this->getEntry( entry );
  #endif
    fid      &= ( MAX_FID - 1 );
    bitOffset = ( fid & this->fidMask ) * this->fieldBits;
    index     = &this->fidIndex[ bitOffset >> 4 ];
    entry     = &this->fields[ ( ( (unsigned int) index[ 0 ] |
                                   ( (unsigned int) index[ 1 ] << 16 ) ) >>
                                 ( bitOffset & 15U ) ) &
                                   ( ( 1 << this->fieldBits ) - 1 ) ];
    if ( entry->fid == fid )
      return entry;
    return NULL;
  }

  const RaiMsg_dict *getEntry( const RaiMsg_dict *entry ) const {
    const RaiMsg_dict * entry2;
    unsigned int        bitOffset;
    Rai_u16           * index;

    if ( this->fieldCount > 0 ) {
      bitOffset = ( entry->fid & this->fidMask ) * this->fieldBits;
      index     = &this->fidIndex[ bitOffset >> 4 ];
      entry2    = &this->fields[ ( ( (unsigned int) index[ 0 ] |
                                     ( (unsigned int) index[ 1 ] << 16 ) ) >>
                                   ( bitOffset & 15U ) ) &
                                     ( ( 1 << this->fieldBits ) - 1 ) ];
      if ( entry->fid == entry2->fid )
        return entry2;
    }
    return NULL;
  }

  Rai_u16 getFormSize( void ) const {
    return this->entry->fsize;
  }

  Rai_u16 getFormClass( void ) const {
    return this->entry->fid;
  }

  RaiMsg_name getFormName( void ) const {
    return this->entry->fname;
  }

  void initFields( Rai_u16 *fid,  unsigned int fidCount,
                   bool setFieldName )                 throw( RaiMsgException );
  void computeHashes( void );

  static unsigned int calcIndexSize( Rai_u16 *fid,  unsigned int fidCount );

  static unsigned int getIndexSize( Rai_u16 *fid,  unsigned int fidCount,
                                    Rai_u8 &fidBits,  Rai_u8 &fieldBits );
  unsigned int indexSize( void );
};


struct RAIMSG_DLL_EXP RaiMsg_config {
  RaiMsg_dict       * entry,      /* all the unique classes + the form fields */
                    * hashIndex[ MAX_FID ]; /* hash by class fname */
  const RaiMsg_dict * msgType,    /* short-cuts */
                    * recType,
                    * seqNo,
                    * recStatus,
                    * tstamp;
  Rai_u16       fidIndex[ MAX_FID ],  /* fid index for unique class ids: */
                fidForm[ MAX_FID ];   /* entry[ fidIndex[ fid ] ] defined */
                                      /* for each class declared */
  RaiMsg_form * form;                 /* array of forms */
  RaiMsg_dtab * dynamicForm;          /* table of dynamically created forms */
  //RaiMsg_ttab * typeForm;             /* table of forms indexed by type */
  Rai_u32       entryCount,           /* number of unique class ids */
                formCount;            /* number of forms */
  rai::Mutex  * dynamicFormLock;
  Rai_u16       reservedMask;
  Rai_u32       dictSize;

  void init( void ) {
    for ( unsigned int i = 0; i < MAX_FID; i++ ) {
      this->hashIndex[ i ] = NULL;
      this->fidIndex[ i ]  = (Rai_u16) 0xffffU;
      this->fidForm[ i ]   = (Rai_u16) 0xffffU;
    }
    this->entry           = NULL;
    this->msgType         = NULL;
    this->recType         = NULL;
    this->seqNo           = NULL;
    this->recStatus       = NULL;
    this->tstamp          = NULL;
    this->form            = NULL;
    this->dynamicForm     = NULL;
    this->entryCount      = 0;
    this->formCount       = 0;
    this->dynamicFormLock = NULL;
    this->reservedMask    = 0;
    this->dictSize        = 0;
  }

  static RaiMsg_config *parseDictionary( const char *tss_fields_fname,
                                         const char *tss_records_fname,
                                         const char *cfile_path = NULL,
                                         char path_sep = 0,
                                         rai::CFileLocator *loc = NULL )
                                                       throw( RaiMsgException );
  static RaiMsg_config *parseDictionary( rai::InputStream *in )
                                                       throw( RaiMsgException );
  static RaiMsg_config *parseDictionary( rai::CFileExpr *expr )
                                                       throw( RaiMsgException );
  static RaiMsg_config *parseDictionary( rai::InputStream *in,
                                         const char *fname,  
                                         const char *cfile_path = NULL,
                                   char path_sep = 0 ) throw( RaiMsgException );

  static void release( RaiMsg_config *dict );

  static const char *tssTypeString( unsigned int typ );

  void packDataDictionary( RaiMsg &msg,  bool addForms = false ) const
                                                       throw( RaiMsgException );
  static RaiMsg_config *unpackDataDictionary( RaiMsg &msg )
                                                       throw( RaiMsgException );
  const RaiMsg_dict *getEntry( const char *fname,
                               unsigned int fname_size ) const;
  const RaiMsg_dict *getEntry( const char *fname ) const {
    return this->getEntry( fname, (unsigned int) ::strlen( fname ) + 1 );
  }
  const RaiMsg_dict *getEntry( Rai_u16 fid ) const {
    fid &= ( MAX_FID - 1 );
    if ( this->fidIndex[ fid ] >= this->entryCount ||
         this->entry[ this->fidIndex[ fid ] ].fname == NULL )
      return NULL;
    return &this->entry[ this->fidIndex[ fid ] ];
  }
  const RaiMsg_form *getForm( const char *fname,
                              unsigned int fname_size ) const;
  const RaiMsg_form *getForm( const char *fname ) const {
    return this->getForm( fname, (unsigned int) ::strlen( fname ) + 1 );
  }
  const RaiMsg_form *getForm( Rai_u16 fid ) const {
    fid &= ( MAX_FID - 1 );
    if ( fid == 0 || this->fidForm[ fid ] >= this->formCount )
      return NULL;
    return &this->form[ this->fidForm[ fid ] ];
  }
  /* must not be from form->getEntry( fid ), which is another entry */
  bool isReserved( const RaiMsg_dict *entry ) const {
    return ( ( entry->fid & this->reservedMask ) == entry->fid ) &&
             ( entry == this->msgType ||
               entry == this->recType ||
               entry == this->seqNo ||
               entry == this->recStatus );
  }

  const RaiMsg_form *getAnonForm( Rai_u16 *fid,  unsigned int fidCount,
                                  bool mustExist = false )
                                                       throw( RaiMsgException );
  const RaiMsg_form *getHashedForm( Rai_u64 typeHash ) throw( RaiMsgException );

  void initAnonForms( void )                           throw( RaiMsgException );

  void derefForm( const RaiMsg_form *form );

  void refForm( const RaiMsg_form *form );

  void describeMemory( RaiMsg &msg )                   throw( RaiMsgException );

  const RaiMsg_form *describeAnonForm( RaiMsg &msg,  const char *name )
                                                       throw( RaiMsgException );
  bool getFormRefCount( const RaiMsg_form *form,  ullong &fidHash,
                        unsigned int &count );
};


inline const RaiMsg_dict *
RaiMsg_form::getEntryByClass( const char *fname, unsigned int fname_size ) const
{
  const RaiMsg_dict *entry;
  if ( (entry = this->dict->getEntry( fname, fname_size )) == NULL )
    return NULL;
  return this->getEntry( entry );
}

#endif
