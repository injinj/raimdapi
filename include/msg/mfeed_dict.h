/* Copyright (c) 2006 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__mfeed_dict_h__
#define __rai_msg__mfeed_dict_h__

#if ! defined( RAIMSG_DLL_EXP )
#if defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllimport)
#else
#define RAIMSG_DLL_EXP
#endif
#endif

#ifndef __rai_msg__dict_h__
#include "msg/dict.h"
#endif

namespace rai {
  class InputStream;
  class OutputStream;
  class CFileLocator;
}
struct RaiMfeed_dict;
extern RAIMSG_DLL_EXP
       RaiMfeed_dict *MfeedDictionary;
static const Rai_i16 MIN_MFEED_FID   = -24576,
                     MAX_MFEED_FID   = 0x7fff;
static const Rai_u16 MFEED_FID_RANGE = 57344;

enum RaiMfeed_type {
  MFT_NONE         = 0,
  MFT_ALPHANUMERIC = 1, /* (2) RAIMSG_STRING */
  MFT_TIME         = 2, /* (2) RAIMSG_STRING */
  MFT_DATE         = 3, /* (2) RAIMSG_STRING */
  MFT_ENUMERATED   = 4, /* (2) RAIMSG_STRING */
  MFT_INTEGER      = 5, /* 4 byte int, (5) RAIMSG_INT */
  MFT_PRICE        = 6, /* 8 byte real or long int, (7) RAIMSG_REAL */
  MFT_TIME_SECONDS = 7, /* (2) RAIMSG_STRING */
  MFT_BINARY       = 8  /* (3) RAIMSG_OPAQUE */
};

enum RaiMfeed_hint {
  MFH_DATE         = 258,
  MFH_TIME         = 259,
  MFH_TIME_SECONDS = 260,
  MFH_ENUMERATED   = 261
};

enum RaiRWF_type {
  RWF_NONE         = 0,
  RWF_RMTES_STRING = 19,/* size 1 -> 500 (Reuters Multilingual Text Enc) */
  RWF_ASCII_STRING = 17,/*      2 -> 80 */
  RWF_BUFFER       = 16,/*     32 -> 128 */
  RWF_DATE         = 9, /*      4 */
  RWF_TIME         = 10,/*      5,9,10,13,14 */
  RWF_ENUM         = 14,/*      1,2 */
  RWF_REAL         = 8,
  RWF_INT          = 3,
  RWF_UINT         = 4,
  RWF_MAP          = 137
#if 0
  RWF_REAL32       = 72,/*      5,8 */
  RWF_REAL64       = 73,/*      6,9 */
  RWF_UINT32       = 69,/*      1,2,3,4 */
  RWF_UINT64       = 71 /*      5,8 */
#endif
};

struct RAIMSG_DLL_EXP RaiMfeed_enumEntry {
  Rai_u16 fenum,
          value;
  char    display[ 4 ];
};

struct RAIMSG_DLL_EXP RaiMfeed_entry {
  Rai_i16 fid,
          ripple;
  Rai_u16 flen,
          fenumLen,
          fenum,
          rwflen;
  Rai_u8  ftype,
          rwftype,
          dde_off,
          rwfbits; /* 32 or 64 uint/real */
  char    acro[ 4 ];
  const char *ddeAcro( void ) const {
    return &this->acro[ this->dde_off ];
  }
  bool hasHint( Rai_u16 &hint ) const {
    if ( this->ftype == MFT_TIME )
      hint = MFH_TIME;
    else if ( this->ftype == MFT_DATE )
      hint = MFH_DATE;
    else if ( this->ftype == MFT_TIME_SECONDS )
      hint = MFH_TIME_SECONDS;
    else if ( this->ftype == MFT_ENUMERATED )
      hint = MFH_ENUMERATED;
    else
      return false;
    return true;
  }
};

struct RAIMSG_DLL_EXP RaiMfeed_enumList {
  Rai_u16   fidCount,
            valueCount,
            minValue,
            maxValue;
  Rai_i16 * fidArray;
  Rai_u16 * valueArray;
};

struct RAIMSG_DLL_EXP RaiMfeed_entry2 {
  const RaiMfeed_entry * mentry;
  const RaiMsg_dict    * entry;
};

struct RAIMSG_DLL_EXP RaiMfeed_mapEntry {
  Rai_i16 n;
  char    fname[ 2 ];
};

struct RAIMSG_DLL_EXP RaiMfeed_flistMap {
  const RaiMfeed_mapEntry * flistp;
  const RaiMsg_form       * form;
};

struct RAIMSG_DLL_EXP RaiMfeed_fidMap {
  const RaiMfeed_mapEntry * fidp;
  const RaiMsg_dict       * entry;
};

struct RAIMSG_DLL_EXP RaiMfeed_dict {
  char                   sig[ 8 ];
  RaiMfeed_entry2      * fidHash,
                       * sassFidHash;
  RaiMfeed_entry      ** acroHash;
  RaiMfeed_enumEntry  ** enumHashByValue,
                      ** enumHashByDisplay;
  RaiMfeed_flistMap    * flistMap;
  RaiMfeed_fidMap      * fidMap;
  RaiMfeed_enumList    * enumList;
  RaiMfeed_entry       * entryBase;
  RaiMfeed_enumEntry   * enumBase;
  RaiMfeed_mapEntry    * mapBase;
  Rai_u16              * entrySize,
                       * arrayBase;
  const RaiMfeed_entry * msgType,   /* -8001 */
                       * recType,   /* -8002 */
                       * seqNo,     /* -8003 */
                       * recStatus, /* 1008 */
                       * tstamp;
  unsigned int           hashSize,
                         enumHashSize,
                         entryCount,
                         enumEntryCount,
                         enumListCount,
                         enumListSize,
                         flistMapSize,
                         flistCount,
                         fidMapSize,
                         fidCount,
                         mapFnameBytes,
                         dictSize,
                         enumAcroElemCount,
                         rwfTypeCount;
  int                    maxFid;
  char                 * appendix_a_path,
                       * enumtype_defs_path,
                       * flist_path,
                       * fid_path;
  byte                 * endPtr;

  static void SetMfeedDictionary( RaiMfeed_dict *dictionary );

  static RaiMfeed_dict *GetMfeedDictionary( void );

  static RaiMfeed_dict *parseDictionary(
                               const char *appendix_a,
                               const char *enumtype_def,
                               const char *cfile_path = NULL,
                               char path_sep = 0,
                   rai::CFileLocator *locator = NULL );
  static RaiMfeed_dict *parseDictionary2(
                               const char *appendix_a,
                               const char *enumtype_def,
                               const char *cfile_path,
                               char path_sep,
                               rai::CFileLocator *locator,
                               int logLvl );
  static RaiMfeed_dict *parseDictionary3(
                               const char *appendix_a,
                               const char *enumtype_def,
                               const char *flistMap,
                               const char *fidMap,
                               const char *cfile_path,
                               char path_sep,
                               rai::CFileLocator *locator,
                               int logLvl );
  static RaiMfeed_dict *parse( rai::InputStream *appendixIn,
                               rai::InputStream *enumtypeIn,
                               rai::InputStream *flistIn = NULL,
                               rai::InputStream *fidIn = NULL );
  void addMfeedFlistMap( RaiMfeed_mapEntry *e );

  void addMfeedFidMap( RaiMfeed_mapEntry *e );

  void addMfeedEntry( RaiMfeed_entry *entry );

  void addMfeedEnum( RaiMfeed_enumEntry *entry );

  static RaiMfeed_dict *allocMfeedDict( unsigned int entryCount,
                               unsigned int enumCount,
                               unsigned int enumListElemCount,
                               unsigned int flistCount,  unsigned int fidCount,
                               unsigned int mapFnameBytes,
                               unsigned int dictSize,
                               unsigned int enumListSize,
                               unsigned int enumAcroElemCount,
                               int maxFid,
                               unsigned int rwfTypeCount )
;
  void printAppendix( rai::OutputStream *appendixOut ) const
;
  void printEnumtype( rai::OutputStream *enumtypeOut ) const
;
  const RaiMfeed_entry *getMapEntry( const RaiMsg_dict *entry ) const;

  const RaiMfeed_entry *getMapEntry( Rai_i16 fid,
                                     const RaiMfeed_entry **mentry = NULL,
                                     const RaiMsg_dict **entry = NULL ) const;
  const RaiMfeed_entry *getEntry( Rai_i16 fid,
                                  const RaiMfeed_entry **mentry = NULL,
                                  const RaiMsg_dict **entry = NULL ) const;

  const RaiMfeed_entry *getEntry( const RaiMsg_dict *entry ) const;

  const RaiMfeed_entry *getEntry( const char *acro ) const;

  const RaiMsg_form *getForm( Rai_i16 flist ) const;

  bool getFlist( Rai_u16 recType,  Rai_i16 &flist ) const;

  const RaiMfeed_enumEntry *getEnum( const RaiMfeed_entry *e,
                                     Rai_u16 value ) const {
    return this->getEnum( e->fenum, value );
  };
  const RaiMfeed_enumEntry *getEnum( Rai_u16 fenum,  Rai_u16 value ) const;

  const RaiMfeed_enumEntry *getEnum( const RaiMfeed_entry *e,
                                     const char *display ) const {
    return this->getEnum( e->fenum, display );
  };
  const RaiMfeed_enumEntry *getEnum( Rai_u16 fenum,  const char *disp ) const;

  const RaiMfeed_entry *firstEntry( Rai_u32 &i,  Rai_u32 &off ) const;

  const RaiMfeed_entry *nextEntry( Rai_u32 &i,  Rai_u32 &off ) const;

  const RaiMfeed_enumList *firstEnumList( Rai_u32 &i ) const;

  const RaiMfeed_enumList *nextEnumList( Rai_u32 &i ) const;

  void indexSass( void );

  void packDataDictionary( RaiMsg &msg,  bool full ) const
;
  void packDataDictionary2( RaiMsg &msg ) const;

  static RaiMfeed_dict *unpackDataDictionary2( RaiMsg &msg )
;
  static RaiMfeed_dict *unpackDataDictionary( RaiMsg &msg )
;
  static bool isMfeedPackedDict( RaiMsg &msg );

  static RaiMfeed_type getType( const char *p );

  static RaiRWF_type getRWFType( const char *p,  Rai_u8 &bits );

  static const char *getTypeString( RaiMfeed_type t );

  static const char *getRWFTypeString( RaiRWF_type t,  Rai_u8 bits );

  static void release( RaiMfeed_dict *d );
};

namespace RaiMfeedErr {
  enum {
    BAD_ENUM_LINE_FMT = 0,
    BAD_APP_LINE_FMT  = 1,
    DUPLICATE_ACRO    = 2,
    DUPLICATE_FID     = 3,
    NO_RIPPLE_ACRO    = 4,
    NO_ENUM_ACRO      = 5,
    ENUM_FID_MISMATCH = 6,
    ACRO_NOT_ENUM     = 7,
    DUPLICATE_ENUM    = 8,
    REDECLARED_ENUM   = 9,
    ENUM_COUNT_MIS    = 10,
    BAD_FLIST_MAP     = 11,
    BAD_FID_MAP       = 12,
    BAD_FLIST_FMT     = 13,
    BAD_FID_FMT       = 14,
    BAD_MSG_FMT       = 15
  };
  RAIMSG_DLL_EXP
  RaiMsgException getErr( unsigned int status );
}
#endif
