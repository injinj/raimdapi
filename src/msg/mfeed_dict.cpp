#if ! defined( RAIMSG_DLL_EXP ) && defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "base/sys.h"
#include "base/log.h"
#include "base/file.h"
#include "stream/io_stream.h"
#include "stream/stdio_stream.h"
#include "stream/file_stream.h"
#include "msg/rai_msg.h"
#include "util/str_util.h"
#include "util/hash_util.h"
#include "util/list_queue.h"
#include "msg/mfeed_dict.h"
#include "msg/cfile_parser.h"

using namespace rai;

RaiMfeed_dict *MfeedDictionary;

void
RaiMfeed_dict::SetMfeedDictionary( RaiMfeed_dict *dictionary )
{
  MfeedDictionary = dictionary;
}

RaiMfeed_dict *
RaiMfeed_dict::GetMfeedDictionary( void )
{
  return MfeedDictionary;
}


const RaiMfeed_entry *
RaiMfeed_dict::getMapEntry( const RaiMsg_dict *entry ) const
{
  if ( this->fidMapSize == 0 )
    return NULL;

  unsigned int h = Hash32::hashInt( entry->fid ) % this->fidMapSize +
                                                  this->fidMapSize;
  while ( this->fidMap[ h ].entry != NULL ) {
    if ( this->fidMap[ h ].entry->fid == entry->fid ) {
      const RaiMfeed_entry * e;
      Rai_i16 fid = this->fidMap[ h ].fidp->n;
      h = Hash32::hashInt( (unsigned int) (unsigned short) fid ) %
          this->hashSize;
      while ( (e = this->fidHash[ h ].mentry) != NULL ) {
        if ( e->fid == fid )
          return e;
        h = ( h + 1 ) % this->hashSize;
      }
      break;
    }
    h = ( h + 1 ) % this->fidMapSize + this->fidMapSize;
  }
  return NULL;
}


const RaiMfeed_entry *
RaiMfeed_dict::getMapEntry( Rai_i16 fid,  const RaiMfeed_entry **mentry,
                            const RaiMsg_dict **entry ) const
{
  const RaiMfeed_entry    * e;
  const RaiMfeed_mapEntry * m;
  unsigned int hash = Hash32::hashInt( (unsigned int) (unsigned short) fid ), h;

  if ( mentry != NULL )
    *mentry = NULL;
  if ( entry != NULL )
    *entry = NULL;
  h = hash % this->hashSize;
  while ( (e = this->fidHash[ h ].mentry) != NULL ) {
    if ( e->fid == fid ) {
      if ( mentry != NULL )
        *mentry = e;
      if ( entry != NULL )
        *entry = this->fidHash[ h ].entry;
      break;
    }
    h = ( h + 1 ) % this->hashSize;
  }
  if ( entry != NULL && this->fidMapSize != 0 ) {
    h = hash % this->fidMapSize;
    while ( (m = this->fidMap[ h ].fidp) != NULL ) {
      if ( m->n == fid ) {
        if ( entry != NULL )
          *entry = this->fidMap[ h ].entry;
        break;
      }
      h = ( h + 1 ) % this->fidMapSize;
    }
  }
  if ( e != NULL )
    return e;
  return NULL;
}


const RaiMfeed_entry *
RaiMfeed_dict::getEntry( Rai_i16 fid,  const RaiMfeed_entry **mentry,
                         const RaiMsg_dict **entry ) const
{
  const RaiMfeed_entry * e;
  unsigned int h = Hash32::hashInt( (unsigned int) (unsigned short) fid );
  h %= this->hashSize;
  while ( (e = this->fidHash[ h ].mentry) != NULL ) {
    if ( e->fid == fid ) {
      if ( mentry != NULL )
        *mentry = e;
      if ( entry != NULL )
        *entry = this->fidHash[ h ].entry;
      return e;
    }
    h = ( h + 1 ) % this->hashSize;
  }
  if ( mentry != NULL )
    *mentry = NULL;
  if ( entry != NULL )
    *entry = NULL;
  return NULL;
}


const RaiMfeed_entry *
RaiMfeed_dict::getEntry( const RaiMsg_dict *entry ) const
{
  const RaiMsg_dict * e;
  unsigned int h = Hash32::hashInt( (unsigned int) entry->fid );
  h %= this->hashSize;
  while ( (e = this->sassFidHash[ h ].entry) != NULL ) {
    if ( e->fid == entry->fid )
      return this->sassFidHash[ h ].mentry;
    h = ( h + 1 ) % this->hashSize;
  }
  return NULL;
}


const RaiMfeed_entry *
RaiMfeed_dict::getEntry( const char *acro ) const
{
  const RaiMfeed_entry * e;
  unsigned int h = Hash32::crc_cs( acro ) % this->hashSize;
  while ( (e = this->acroHash[ h ]) != NULL ) {
    if ( ::strcmp( e->acro, acro ) == 0 )
      return e;
    h = ( h + 1 ) % this->hashSize;
  }
  return NULL;
}


const RaiMsg_form *
RaiMfeed_dict::getForm( Rai_i16 flist ) const
{
  unsigned int h;

  if ( this->flistMapSize == 0 )
    return NULL;
  h = Hash32::hashInt( (unsigned int) (unsigned short) flist ) %
      this->flistMapSize;
  while ( this->flistMap[ h ].flistp != NULL ) {
    if ( this->flistMap[ h ].flistp->n == flist )
      return this->flistMap[ h ].form;
    h = ( h + 1 ) % this->flistMapSize;
  }
  return NULL;
}


bool
RaiMfeed_dict::getFlist( Rai_u16 recType,  Rai_i16 &flist ) const
{
  recType &= ~FID_CTRL;
  if ( this->flistMapSize != 0 ) {
    unsigned int h = Hash32::hashInt( recType ) %
                     this->flistMapSize + this->flistMapSize;
    while ( this->flistMap[ h ].form != NULL ) {
      if ( this->flistMap[ h ].form->entry->fid == recType ) {
        flist = this->flistMap[ h ].flistp->n;
        return true;
      }
      h = ( h + 1 ) % this->flistMapSize + this->flistMapSize;
    }
  }
  return false;
}


const RaiMfeed_enumEntry *
RaiMfeed_dict::getEnum( Rai_u16 fenum,  Rai_u16 value ) const
{
  const RaiMfeed_enumEntry * e;
  unsigned int h = Hash32::hashInt( ( (unsigned int) fenum << 14 ) ^
                                   ( (unsigned int) value ) );
  h %= this->enumHashSize;
  while ( (e = this->enumHashByValue[ h ]) != NULL ) {
    if ( e->fenum == fenum && e->value == value )
      return e;
    h = ( h + 1 ) % this->enumHashSize;
  }
  return NULL;
}


const RaiMfeed_enumEntry *
RaiMfeed_dict::getEnum( Rai_u16 fenum,  const char *disp ) const
{
  const RaiMfeed_enumEntry * e;
  unsigned int h = Hash32::crc_cs( disp, false, fenum );
  h %= this->enumHashSize;
  while ( (e = this->enumHashByDisplay[ h ]) != NULL ) {
    if ( e->fenum == fenum && ::strcmp( e->display, disp ) == 0 )
      return e;
    h = ( h + 1 ) % this->enumHashSize;
  }
  return NULL;
}



struct MfeedTypeName {
  const char  * name;
  RaiMfeed_type type;
};
const MfeedTypeName mfeed_type_names[] = {
  { "PRICE",        MFT_PRICE        },
  { "INTEGER",      MFT_INTEGER      },
  { "ENUMERATED",   MFT_ENUMERATED   },
  { "TIME",         MFT_TIME         },
  { "TIME_SECONDS", MFT_TIME_SECONDS },
  { "DATE",         MFT_DATE         },
  { "ALPHANUMERIC", MFT_ALPHANUMERIC },
  { "BINARY",       MFT_BINARY       }
};
struct RwfTypeName {
  const char * name;
  RaiRWF_type  type;
  Rai_u8       bits;
};
const RwfTypeName rwf_type_names[] = {
  { "RMTES_STRING", RWF_RMTES_STRING, 0  },
  { "REAL32",       RWF_REAL,         32 },
  { "REAL64",       RWF_REAL,         64 },
  { "ASCII_STRING", RWF_ASCII_STRING, 0  },
  { "BUFFER",       RWF_BUFFER,       0  },
  { "DATE",         RWF_DATE,         0  },
  { "TIME",         RWF_TIME,         0  },
  { "ENUM",         RWF_ENUM,         0  },
  { "UINT32",       RWF_UINT,         32 },
  { "UINT64",       RWF_UINT,         64 },
  { "INT32",        RWF_INT,          32 },
  { "INT64",        RWF_INT,          64 },
  { "MAP",          RWF_MAP,          0  }
};

RaiMfeed_type
RaiMfeed_dict::getType( const char *p ) {
  for ( const MfeedTypeName &t : mfeed_type_names ) {
    if ( ::strcmp( p, t.name ) == 0 )
      return t.type;
  }
  return MFT_NONE;
}


RaiRWF_type
RaiMfeed_dict::getRWFType( const char *p,  Rai_u8 &bits ) {
  bits = 0;
  if ( p == NULL )
    return RWF_NONE;
  for ( const RwfTypeName &t : rwf_type_names ) {
    if ( ::strcmp( p, t.name ) == 0 ) {
      bits = t.bits;
      return t.type;
    }
  }
  return RWF_NONE;
}

void
RaiMfeed_dict::release( RaiMfeed_dict *d )
{
  if ( d->appendix_a_path != NULL )
    FREE( d->appendix_a_path );
  if ( d->enumtype_defs_path != NULL )
    FREE( d->enumtype_defs_path );
  if ( d->flist_path != NULL )
    FREE( d->flist_path );
  if ( d->fid_path != NULL )
    FREE( d->fid_path );
  FREE( d );
}


static char *
scanTok( char *p,  unsigned int &off,  unsigned int len )
{
  char * s = NULL;
  while ( off < len && isspace( p[ off ] ) )
    off++;
  if ( off < len ) {
    if ( p[ off ] == '\"' ) {
      s = &p[ ++off ];
      while ( off < len && p[ off ] != '\"' )
        off++;
    }
    else {
      s = &p[ off++ ];
      while ( off < len && ! isspace( p[ off ] ) )
        off++;
    }
    if ( off < len )
      p[ off++ ] = '\0';
  }
  return s;
}


static bool
scanNum( char *p,  unsigned int &off,  unsigned int len,  Rai_i32 &val )
{
  bool sawdigit = false;
  Rai_u32 n = 0;
  Rai_i32 m = 1;
  while ( off < len && isspace( p[ off ] ) )
    off++;
  if ( off < len && p[ off ] == '-' ) {
    m = -1;
    off++;
  }
  while ( off < len && isdigit( p[ off ] ) ) {
    n = n * 10 + (unsigned int) (byte) ( p[ off++ ] - '0' );
    sawdigit = true;
  }
  val = (Rai_i32) n * m;
  return sawdigit;
}


static bool
scanNum( char *p,  unsigned int &off,  unsigned int len,  Rai_i16 &val )
{
  Rai_i32 v;
  bool b = scanNum( p, off, len, v );
  val = (Rai_i16) v;
  return b;
}


static bool
scanNum( char *p,  unsigned int &off,  unsigned int len,  Rai_u16 &val )
{
  Rai_i32 v;
  bool b = scanNum( p, off, len, v );
  val = (Rai_u16) v;
  return b;
}


static void
badLine( Error e,  char *line,  unsigned int len )
{
  StrUtil::stripNewline( line, &len );
  logError( LERROR, e, "\"%s\"", line );
}

namespace rai {
struct MfeedTmpBlock {
  MfeedTmpBlock * next;
  byte          * ptr;
  unsigned int    len,
                  size;

  static MfeedTmpBlock *allocElem( MfeedTmpBlock *block,  unsigned int len,
                                   void *ptr );
  static void release( MfeedTmpBlock *block );
};

struct MfeedTmpEntry {
  MfeedTmpEntry  * next;
  RaiMfeed_entry * e;
  char           * acro,
                 * dde_acro,
                 * ripple;
  Rai_i16          fid;
  Rai_u16          flen,
                   fenumLen,
                   rwflen;
  Rai_u8           ftype,
                   rwftype,
                   rwfbits;
};

struct MfeedTmpEntry2 {
  MfeedTmpEntry2 * next;
  char           * dde_acro;
  RaiMfeed_entry * e;
  Rai_u32          fenum;
  Rai_i16          fid;
  Rai_u16          ripple,
                   tss_len,
                   mh_type,
                   flen,
                   fenumLen;
  Rai_u8           tm_type,
                   nm_len,
                   ftype;
  char             acro[ 1 ];
};

struct MfeedTmpAcroEnum {
  MfeedTmpAcroEnum * next;
  Rai_i16            fid;
  char               acro[ 2 ];
};

struct MfeedTmpEnumList {
  MfeedTmpEnumList * next;
  Rai_u16            value;
  char               display[ 2 ];
};

struct MfeedTmpEnum {
  MfeedTmpEnum     * next;
  MfeedTmpAcroEnum * acroEnumHd,
                   * acroEnumTl;
  MfeedTmpEnumList * enumListHd,
                   * enumListTl;
};

struct MfeedTmpMap {
  MfeedTmpMap * next;
  Rai_i32       n;
  char        * fname;
};

struct MfeedTmpEnumList2 {
  char ** val;
  Rai_u16 start,
          count;
};

}

MfeedTmpBlock *
MfeedTmpBlock::allocElem( MfeedTmpBlock *block,  unsigned int len,
                          void *ptr )
{
  unsigned int left;
  len = ( len + 7U ) & ~7U;
  if ( block != NULL )
    left = block->size - block->len;
  else
    left = 0;
  if ( len > left) {
    MfeedTmpBlock * block2;
    unsigned int allocSize = 64 * 1024;
    if ( len > allocSize - sizeof( MfeedTmpBlock ) )
      allocSize = len + sizeof( MfeedTmpBlock );
    MALLOC( allocSize, &block2 );
    block2->next = block;
    block2->len  = 0;
    block2->size = allocSize - sizeof( MfeedTmpBlock );
    block2->ptr  = (byte *) &block2[ 1 ];
    block = block2;
  }
  *((void **) ptr) = (void *) &block->ptr[ block->len ];
  block->len += len;
  return block;
}

void
MfeedTmpBlock::release( MfeedTmpBlock *block )
{
  while ( block != NULL ) {
    MfeedTmpBlock * block2 = block->next;
    FREE( block );
    block = block2;
  }
}


RaiMfeed_dict *
RaiMfeed_dict::parseDictionary( const char *appendix_a,
                                const char *enumtype_def,
                                const char *cfile_path,  char path_sep,
                                CFileLocator *locator )
{
  return RaiMfeed_dict::parseDictionary2( appendix_a, enumtype_def,
                                          cfile_path, path_sep, locator,
                                          Log::LVL_ERROR );
}


RaiMfeed_dict *
RaiMfeed_dict::parseDictionary2( const char *appendix_a,
                                 const char *enumtype_def,
                                 const char *cfile_path,  char path_sep,
                                 CFileLocator *locator,
                                 int logLvl )
{
  return RaiMfeed_dict::parseDictionary3( appendix_a, enumtype_def, NULL, NULL,
                                        cfile_path, path_sep, locator, logLvl );
}


RaiMfeed_dict *
RaiMfeed_dict::parseDictionary3( const char *appendix_a,
                                 const char *enumtype_def,
                                 const char *flistMap,
                                 const char *fidMap,
                                 const char *cfile_path,  char path_sep,
                                 CFileLocator *locator,
                                 int logLvl )
{
  CFileLoc        loc;
  char            appPath[ 1024 ],
                  enumPath[ 1024 ],
                  flistPath[ 1024 ],
                  fidPath[ 1024 ];
  RaiMfeed_dict * dict       = NULL;
  Error           e2         = NULL;
  InputStream   * appendixIn = NULL,
                * enumtypeIn = NULL,
                * flistIn    = NULL,
                * fidIn      = NULL;
  char            fmt[ 2 * 1024 ], *p, *e;
  const char    * q;
#define catfmt( str ) \
  for ( q = str; *q && p < e; *p++ = *q++ ) ; \
  if ( p < e ) *p++ = ' '

  if ( locator == NULL )
    locator = &loc;
  if ( cfile_path == NULL )
    cfile_path = ::getenv( "cfile_path" );

  ::strncpy( fmt, "Parsing: ", sizeof( fmt ) );
  p = &fmt[ ::strlen( fmt ) ];
  e = &fmt[ sizeof( fmt ) - 1 ];
  appPath[ 0 ]   = '\0';
  enumPath[ 0 ]  = '\0';
  flistPath[ 0 ] = '\0';
  fidPath[ 0 ]   = '\0';

  if ( appendix_a != NULL ) {
    try {
      /*if ( path_sep == 0 )
        path_sep = ' ';*/
      appendixIn = locator->openFile( appendix_a, cfile_path, path_sep,
                                      appPath, sizeof( appPath ) );
      catfmt( appPath );
    } catch ( Error e ) {
      Log::printLog( (Log::LogLevel) logLvl, __FILE__, __LINE__, e,
                     "File \"%s\" Search path \"%s\" Path sep \"%c\"",
                     appendix_a, cfile_path, path_sep?path_sep:' ' );
      e2 = e;
      goto on_error;
    }
  }

  if ( enumtype_def != NULL ) {
    try {
      enumtypeIn = locator->openFile( enumtype_def, cfile_path, path_sep,
                                      enumPath, sizeof( enumPath ) );
      catfmt( enumPath );
    } catch ( Error e ) {
      Log::printLog( (Log::LogLevel) logLvl, __FILE__, __LINE__, e,
                     "File \"%s\" Search path \"%s\" Path sep \"%c\"",
                     enumtype_def, cfile_path, path_sep?path_sep:' ' );
      e2 = e;
      goto on_error;
    }
  }

  if ( flistMap != NULL ) {
    try {
      flistIn = locator->openFile( flistMap, cfile_path, path_sep,
                                   flistPath, sizeof( flistPath ) );
      catfmt( flistPath );
    } catch ( Error e ) {
      Log::printLog( (Log::LogLevel) logLvl, __FILE__, __LINE__, e,
                     "File \"%s\" Search path \"%s\" Path sep \"%c\"",
                     flistMap, cfile_path, path_sep?path_sep:' ' );
      e2 = e;
      goto on_error;
    }
  }

  if ( fidMap != NULL ) {
    try {
      fidIn = locator->openFile( fidMap, cfile_path, path_sep,
                                 fidPath, sizeof( fidPath ) );
      catfmt( fidPath );
    } catch ( Error e ) {
      Log::printLog( (Log::LogLevel) logLvl, __FILE__, __LINE__, e,
                     "File \"%s\" Search path \"%s\" Path sep \"%c\"",
                     fidMap, cfile_path, path_sep?path_sep:' ' );
      e2 = e;
      goto on_error;
    }
  }

  try {
    logDebug( LDEBUG, "%s", fmt );
    dict = RaiMfeed_dict::parse( appendixIn, enumtypeIn, flistIn, fidIn );
    if ( dict != NULL ) {
      if ( appPath[ 0 ] != '\0' )
        STRDUP( dict->appendix_a_path, appPath );
      if ( enumPath[ 0 ] != '\0' )
        STRDUP( dict->enumtype_defs_path, enumPath );
      if ( flistPath[ 0 ] != '\0' )
        STRDUP( dict->flist_path, flistPath );
      if ( fidPath[ 0 ] != '\0' )
        STRDUP( dict->fid_path, fidPath );
    }
  } catch ( Error e ) {
    logError( LERROR, e, "%s", fmt );
    e2 = e;
    goto on_error;
  }

on_error:;
  try {
    if ( appendixIn != NULL ) {
      appendixIn->close();
      delete appendixIn;
    }
  } catch ( ... ) {
  }
  try {
    if ( enumtypeIn != NULL ) {
      enumtypeIn->close();
      delete enumtypeIn;
    }
  } catch ( ... ) {
  }
  try {
    if ( flistIn != NULL ) {
      flistIn->close();
      delete flistIn;
    }
  } catch ( ... ) {
  }
  try {
    if ( fidIn != NULL ) {
      fidIn->close();
      delete fidIn;
    }
  } catch ( ... ) {
  }
  if ( dict != NULL )
    return dict;
  throw e2;
}


static int
u16cmp( Rai_u16 *x,  Rai_u16 *y )
{
  return (int) *x - (int) *y;
}


RaiMfeed_dict *
RaiMfeed_dict::parse( InputStream *appendixIn,  InputStream *enumtypeIn,
            InputStream *flistIn,  InputStream *fidIn )
{
  RaiMfeed_dict    * d;
  char               line[ 1024 ],
                     line2[ 1024 ],
                   * acronym,
                   * dde_acro,
                   * ripple,
                   * ftype,
                   * first,
                   * second,
                   * third,
                   * rwftype;
  MfeedTmpEntry    * tmp,
                   * hd,
                   * tl;
  MfeedTmpBlock    * block;
  MfeedTmpEnum     * enumHd,
                   * enumTl,
                   * enumTmp;
  MfeedTmpMap      * flistHd,
                   /** flistTl,*/
                   * fidHd;
                   /** fidTl;*/
  MfeedTmpAcroEnum * tmpAcro;
  MfeedTmpEnumList * tmpList;
  unsigned int       len,
                     i,
                     n,
                     entryCount,
                     dictSize,
                     enumListCount,
                     enumListElemCount,
                     enumAcroElemCount,
                     enumListSize,
                     appendixEnumCount,
                     flistCount,
                     fidCount,
                     rwfTypeCount;
  int                fid,
                     flen,
                     rwflen,
                     fenumLen,
                     maxFid,
                     mapFnameBytes;
  Rai_u16            tmpValue;
  Rai_i16            tmpFid;

  hd                = NULL;
  tl                = NULL;
  entryCount        = 0;
  dictSize          = 0;
  maxFid            = 0;
  enumHd            = NULL;
  enumTl            = NULL;
  flistHd           = NULL;
  //flistTl           = NULL;
  fidHd             = NULL;
  //fidTl             = NULL;
  block             = NULL;
  enumListCount     = 0;
  enumListElemCount = 0;
  enumAcroElemCount = 0;
  enumListSize      = 0;
  appendixEnumCount = 0;
  flistCount        = 0;
  fidCount          = 0;
  mapFnameBytes     = 0;
  rwfTypeCount      = 0;

  if ( enumtypeIn != NULL ) {
    while ( (n = enumtypeIn->gets( line, sizeof( line ) )) > 0 ) {
      while ( n > 0 && isspace( line[ n - 1 ] ) )
        line[ --n ] = '\0';
      if ( n == 0 || line[ 0 ] == '!' || line[ 0 ] == '\n' ||
           line[ 0 ] == '\r' )
        continue;

      i = 0;
      ::memcpy( line2, line, n );
      if ( (first = scanTok( line, i, n )) == NULL ||
           (second = scanTok( line, i, n )) == NULL ) {
        Error e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_ENUM_LINE_FMT );
        badLine( e, line2, n );
        continue;
      }
      while ( i < n && isspace( line[ i ] ) )
        i++;
      if ( i < n )
        third = &line[ i ];
      else {
        static char mt[1] = {0};
        third = mt;
      }
      unsigned int x = ::strlen( first );
      unsigned int y = ::strlen( second );
      unsigned int z = ::strlen( third );
      i = 0;
      if ( ! scanNum( first, i, x, tmpValue ) || i != x ) {
        i = 0;
        if ( ! scanNum( second, i, y, tmpFid ) || i != y ) {
          Error e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_ENUM_LINE_FMT );
          badLine( e, line2, n );
          continue;
        }

        len = sizeof( MfeedTmpAcroEnum ) - 2 + x + 1;
        block = MfeedTmpBlock::allocElem( block, len, &tmpAcro );
        tmpAcro->next = NULL;
        tmpAcro->fid = tmpFid;
        ::strcpy( tmpAcro->acro, first );

        if ( enumTl == NULL || enumTl->enumListHd != NULL ) {
          block = MfeedTmpBlock::allocElem( block, sizeof( MfeedTmpEnum ),
                                            &enumTmp );
          ::memset( enumTmp, 0, sizeof( MfeedTmpEnum ) );
          if ( enumTl == NULL )
            enumHd = enumTmp;
          else
            enumTl->next = enumTmp;
          enumTl = enumTmp;
          enumListCount++;
        }
        if ( enumTl->acroEnumHd == NULL )
          enumTl->acroEnumHd = tmpAcro;
        else
          enumTl->acroEnumTl->next = tmpAcro;
        enumTl->acroEnumTl = tmpAcro;
        enumAcroElemCount++;
      }
      else {
        len = sizeof( MfeedTmpEnumList ) - 2 + y + 1 + z + 1;
        block = MfeedTmpBlock::allocElem( block, len, &tmpList );
        tmpList->next = NULL;
        tmpList->value = tmpValue;
        ::strcpy( tmpList->display, second );
        ::strcpy( &tmpList->display[ y + 1 ], third );

        if ( enumTl == NULL ) {
          Error e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_ENUM_LINE_FMT );
          badLine( e, line2, n );
          continue;
        }
        if ( enumTl->enumListHd == NULL )
          enumTl->enumListHd = tmpList;
        else
          enumTl->enumListTl->next = tmpList;
        enumTl->enumListTl = tmpList;
        enumListElemCount++;
        enumListSize += ( sizeof( RaiMfeed_enumEntry )-4 + y+1 + z+1 + 3U)& ~3U;
      }
    }
  }

  if ( appendixIn != NULL ) {
    while ( (n = appendixIn->gets( line, sizeof( line ) )) > 0 ) {
      while ( n > 0 && isspace( line[ n - 1 ] ) )
        line[ --n ] = '\0';
      if ( n == 0 || line[ 0 ] == '!' || line[ 0 ] == '\n' || line[ 0 ] == '\r')
        continue;

      i = 0;
      ::memcpy( line2, line, n );
      if ( (acronym  = scanTok( line, i, n )) == NULL ||
           (dde_acro = scanTok( line, i, n )) == NULL ||
           ! scanNum( line, i, n, fid ) ||
           (ripple   = scanTok( line, i, n )) == NULL ||
           (ftype    = scanTok( line, i, n )) == NULL ||
           ! scanNum( line, i, n, flen ) ) {
        Error e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_APP_LINE_FMT );
        badLine( e, line2, n );
        continue;
      }
      if ( ::strcmp( ftype, "ENUMERATED" ) == 0 ) {
        while ( i < n && line[ i++ ] != '(' )
          ;
        if ( ! scanNum( line, i, n, fenumLen ) ) {
          Error e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_APP_LINE_FMT );
          badLine( e, line2, n );
          continue;
        }
        while ( i < n && line[ i++ ] != ')' )
          ;
        appendixEnumCount++;
      }
      else {
        fenumLen = 0;
      }
      if ( (rwftype = scanTok( line, i, n )) != NULL &&
           scanNum( line, i, n, rwflen ) ) {
        rwfTypeCount++;
      }
      else {
        rwftype = NULL;
        rwflen = 0;
      }
      unsigned int x = ( ::strlen( acronym ) + 1 ),
                   y = ( ::strlen( dde_acro ) + 1 ),
                   z = ( ::strcmp( ripple, "NULL" ) == 0 ) ? 0 :
                       ( ::strlen( ripple ) + 1 );

      len   = sizeof( MfeedTmpEntry ) + x + y + z;
      block = MfeedTmpBlock::allocElem( block, len, &tmp );

      tmp->next = NULL;
      if ( hd == NULL )
        hd = tmp;
      else
        tl->next = tmp;
      tl = tmp;
      tmp->e        = NULL;
      tmp->acro     = (char *) &tmp[ 1 ];
      tmp->dde_acro = &tmp->acro[ x ];
      ::strcpy( tmp->acro, acronym );
      ::strcpy( tmp->dde_acro, dde_acro );
      if ( z == 0 )
        tmp->ripple = NULL;
      else {
        tmp->ripple = &tmp->dde_acro[ y ];
        ::strcpy( tmp->ripple, ripple );
      }
      tmp->fid      = fid;
      tmp->ftype    = RaiMfeed_dict::getType( ftype );
      tmp->flen     = flen;
      tmp->fenumLen = fenumLen;
      tmp->rwftype  = RaiMfeed_dict::getRWFType( rwftype, tmp->rwfbits );
      tmp->rwflen   = rwflen;

      dictSize += ( sizeof( RaiMfeed_entry ) - 4 + x + y + 3U ) & ~3U;
      entryCount++;
      if ( fid > maxFid )
        maxFid = fid;
    }
  }

  for ( unsigned int k = 0; k < 2; k++ ) {
    InputStream * in = ( k == 0 ) ? flistIn : fidIn;
    MfeedTmpMap * tmp, * hd = NULL, * tl = NULL;
    unsigned int count = 0;

    if ( in != NULL ) {
      while ( (n = in->gets( line, sizeof( line ) )) > 0 ) {
        while ( n > 0 && isspace( line[ n - 1 ] ) )
          line[ --n ] = '\0';
        if ( n == 0 || line[ 0 ] == '!' || line[ 0 ] == '#' ||
             line[ 0 ] == '\n' || line[ 0 ] == '\r' )
          continue;

        i = 0;
        Rai_i32 num;
        char  * fname;
        if ( ! scanNum( line, i, n, num ) ||
             (fname = scanTok( line, i, n )) == NULL ) {
          Error e = RaiMfeedErr::getErr( k == 0 ? RaiMfeedErr::BAD_FLIST_FMT :
                                                  RaiMfeedErr::BAD_FID_FMT );
          badLine( e, line, n );
          continue;
        }

        unsigned int x = ::strlen( fname ) + 1;
        len   = sizeof( MfeedTmpMap ) + x;
        block = MfeedTmpBlock::allocElem( block, len, &tmp );

        tmp->next = NULL;
        if ( hd == NULL )
          hd = tmp;
        else
          tl->next = tmp;
        tl = tmp;

        tmp->n     = num;
        tmp->fname = (char *) &tmp[ 1 ];
        ::memcpy( tmp->fname, fname, x );
        mapFnameBytes += ( x + 3U ) & ~3U;
        count++;
      }
    }
    if ( k == 0 ) {
      flistHd    = hd;
      //flistTl    = tl;
      flistCount = count;
    }
    else {
      fidHd    = hd;
      //fidTl    = tl;
      fidCount = count;
    }
  }

  d = RaiMfeed_dict::allocMfeedDict( entryCount, enumListCount,
                                     enumListElemCount, flistCount, fidCount,
                                     mapFnameBytes, dictSize, enumListSize,
                                     enumAcroElemCount, maxFid, rwfTypeCount );
  byte    * dictPtr   = (byte *) d->entryBase;
  byte    * enumPtr   = (byte *) d->enumBase;
  byte    * mapPtr    = (byte *) d->mapBase;
  Rai_u16 * szPtr     = d->entrySize;
  Rai_u16 * fidValPtr = d->arrayBase;

  for ( tmp = hd; tmp != NULL; tmp = tmp->next ) {
    unsigned int x  = ::strlen( tmp->acro ) + 1,
                 y  = ::strlen( tmp->dde_acro ) + 1;
    RaiMfeed_entry * entry  = (RaiMfeed_entry *) dictPtr;
    Rai_u16      sz = ( ( sizeof( RaiMfeed_entry ) - 4 + x + y + 3U ) & ~3U );

    *szPtr++ = sz;
    dictPtr  = &dictPtr[ sz ];

    entry->fid      = tmp->fid;
    entry->ripple   = 0;
    entry->flen     = tmp->flen;
    entry->fenumLen = tmp->fenumLen;
    entry->ftype    = tmp->ftype;
    entry->rwftype  = tmp->rwftype;
    entry->rwflen   = tmp->rwflen;
    entry->rwfbits  = tmp->rwfbits;
    ::strcpy( entry->acro, tmp->acro );
    entry->dde_off  = (Rai_u8) x;
    ::strcpy( &entry->acro[ x ], tmp->dde_acro );

    if ( tmp->ripple != NULL )
      tmp->e = entry;

    d->addMfeedEntry( entry );
  }

  for ( tmp = hd; tmp != NULL; tmp = tmp->next ) {
    if ( tmp->e != NULL ) {
      const RaiMfeed_entry * entry = d->getEntry( tmp->ripple );
      if ( entry != NULL )
        tmp->e->ripple = entry->fid;
      else {
        Error e = RaiMfeedErr::getErr( RaiMfeedErr::NO_RIPPLE_ACRO );
        Log::printLog( Log::LVL_MINOR, __FILE__, __LINE__, e,
                       "\"%s\"", tmp->acro );
      }
    }
  }

  entryCount = 0;
  for ( enumTmp = enumHd; enumTmp != NULL; enumTmp = enumTmp->next ) {
    d->enumList[ entryCount ].fidArray = (Rai_i16 *) fidValPtr;

    Rai_u16 fidCount = 0;
    for ( tmpAcro = enumTmp->acroEnumHd; tmpAcro != NULL;
          tmpAcro = tmpAcro->next ) {
      const RaiMfeed_entry * entry = d->getEntry( tmpAcro->acro );
      if ( entry == NULL ) {
        Error e = RaiMfeedErr::getErr( RaiMfeedErr::NO_ENUM_ACRO );
        Log::printLog( Log::LVL_DEBUG, __FILE__, __LINE__, e,
                       "\"%s\" in enumtype.def doesn't exist in "
                       "appendix_a", tmpAcro->acro );
      }
      else if ( entry->fid != tmpAcro->fid ) {
        Error e = RaiMfeedErr::getErr( RaiMfeedErr::ENUM_FID_MISMATCH );
        Log::printLog( Log::LVL_MINOR, __FILE__, __LINE__, e,
                      "\"%s\" in enumtype.def declared in "
                      "appendix_a: %d != %d", tmpAcro->acro, (int) tmpAcro->fid,
                      (int) entry->fid );
      }
      else if ( entry->fenumLen == 0 || entry->ftype != MFT_ENUMERATED ) {
        Error e = RaiMfeedErr::getErr( RaiMfeedErr::ACRO_NOT_ENUM );
        Log::printLog( Log::LVL_MINOR, __FILE__, __LINE__, e,
                       "\"%s\" in enumtype.def and appendix_a", tmpAcro->acro );
      }
      else {
        ((RaiMfeed_entry *) entry)->fenum = entryCount + 1;
        fidCount++;
        *fidValPtr++ = entry->fid;
      }
    }
    d->enumList[ entryCount ].fidCount   = fidCount;
    d->enumList[ entryCount ].valueArray = fidValPtr;

    Rai_u16 valueCount = 0,
            minValue = 0xffffU,
            maxValue = 0,
            lastValue = 0;
    bool    outOfOrder = false;
    for ( tmpList = enumTmp->enumListHd; tmpList != NULL;
          tmpList = tmpList->next ) {
      valueCount++;
      *fidValPtr++ = tmpList->value;
      if ( tmpList->value < lastValue )
        outOfOrder = true;
      lastValue = tmpList->value;
      if ( tmpList->value < minValue )
        minValue = tmpList->value;
      if ( tmpList->value > maxValue )
        maxValue = tmpList->value;

      RaiMfeed_enumEntry * entry = (RaiMfeed_enumEntry *) enumPtr;
      entry->fenum = entryCount + 1;
      entry->value = tmpList->value;
      ::strcpy( entry->display, tmpList->display );
      unsigned int displen = ::strlen( entry->display );
      ::strcpy( &entry->display[ displen + 1 ],
                &tmpList->display[ displen + 1 ] );
      enumPtr = &enumPtr[ ( sizeof( RaiMfeed_enumEntry ) - 4 +
                            displen + 1 +
                  ::strlen( &entry->display[ displen + 1 ] ) + 1 + 3U ) & ~3U ];

      d->addMfeedEnum( entry );
    }
    d->enumList[ entryCount ].valueCount = valueCount;
    d->enumList[ entryCount ].minValue   = minValue;
    d->enumList[ entryCount ].maxValue   = maxValue;
    if ( outOfOrder ) {
      ::qsort( d->enumList[ entryCount ].valueArray, valueCount,
              sizeof( Rai_u16 ), (int (*)(const void *, const void *)) u16cmp );
    }

    entryCount++;
  }

  for ( unsigned int k = 0; k < 2; k++ ) {
    MfeedTmpMap *tmp = ( k == 0 ? flistHd : fidHd );
    for ( ; tmp != NULL; tmp = tmp->next ) {
      RaiMfeed_mapEntry * e = (RaiMfeed_mapEntry *) mapPtr;
      e->n = tmp->n;
      unsigned int x = ::strlen( tmp->fname ) + 1;
      ::memcpy( e->fname, tmp->fname, x );
      while ( ( x & 3 ) != 0 )
        x++;
      mapPtr = (byte *) &e->fname[ x ];

      if ( k == 0 )
        d->addMfeedFlistMap( e );
      else
        d->addMfeedFidMap( e );
    }
  }

  if ( appendixEnumCount != enumAcroElemCount ) {
    if ( enumAcroElemCount > appendixEnumCount ) {
      Error e = RaiMfeedErr::getErr( RaiMfeedErr::REDECLARED_ENUM );
      Log::printLog( Log::LVL_MINOR, __FILE__, __LINE__, e, "enumtype.def" );
    }
    Error e = RaiMfeedErr::getErr( RaiMfeedErr::ENUM_COUNT_MIS );
    Log::printLog( Log::LVL_MINOR, __FILE__, __LINE__, e,
                   "%u (appendix_a), %u (enumtype.def)",
                   appendixEnumCount, enumAcroElemCount );
  }

  d->msgType   = d->getEntry( "MSG_TYPE" );
  d->recType   = d->getEntry( "REC_TYPE" );
  d->seqNo     = d->getEntry( "SEQ_NO" );
  d->recStatus = d->getEntry( "REC_STATUS" );
  d->tstamp    = NULL;
  //d->latencyField = d->getEntry( "RAI_TIMESTAMP" );

  MfeedTmpBlock::release( block );

  return d;
}


void
RaiMfeed_dict::addMfeedFlistMap( RaiMfeed_mapEntry *e )
{
  unsigned int h;

  h = Hash32::hashInt( (unsigned int) (unsigned short) e->n ) %
      this->flistMapSize;
  while ( this->flistMap[ h ].flistp != NULL )
    h = ( h + 1 ) % this->flistMapSize;
  this->flistMap[ h ].flistp = e;
}


void
RaiMfeed_dict::addMfeedFidMap( RaiMfeed_mapEntry *e )
{
  unsigned int h;

  h = Hash32::hashInt( (unsigned int) (unsigned short) e->n ) %
                      this->fidMapSize;
  while ( this->fidMap[ h ].fidp != NULL )
    h = ( h + 1 ) % this->fidMapSize;
  this->fidMap[ h ].fidp = e;
}


void
RaiMfeed_dict::addMfeedEntry( RaiMfeed_entry *entry )
{
  unsigned int h;

  h  = Hash32::crc_cs( entry->acro );
  h %= this->hashSize;
  while ( this->acroHash[ h ] != NULL ) {
    if ( ::strcmp( this->acroHash[ h ]->acro, entry->acro ) == 0 ) {
      Error e = RaiMfeedErr::getErr( RaiMfeedErr::DUPLICATE_ACRO );
      Log::LogLevel lvl = Log::LVL_MINOR;
      if ( ::memcmp( this->acroHash[ h ], entry, sizeof( *entry ) ) == 0 )
        lvl = Log::LVL_DEBUG;
      Log::printLog( lvl, __FILE__, __LINE__, e, "\"%s\" fids=%d/%d",
                     entry->acro, entry->fid, this->acroHash[ h ]->fid );
    }
    h = ( h + 1 ) % this->hashSize;
  }
  this->acroHash[ h ] = entry;

  h  = Hash32::hashInt( (unsigned int) (unsigned short) entry->fid );
  h %= this->hashSize;
  while ( this->fidHash[ h ].mentry != NULL ) {
    if ( this->fidHash[ h ].mentry->fid == entry->fid ) {
      Error e = RaiMfeedErr::getErr( RaiMfeedErr::DUPLICATE_FID );
      Log::LogLevel lvl = Log::LVL_MINOR;
      if ( ::memcmp( this->fidHash[ h ].mentry, entry, sizeof( *entry ) ) == 0 )
        lvl = Log::LVL_DEBUG;
      Log::printLog( lvl, __FILE__, __LINE__, e, "\"%s\", \"%s\": %d",
              entry->acro, this->fidHash[ h ].mentry->acro, (int) entry->fid );
    }
    h = ( h + 1 ) % this->hashSize;
  }
  this->fidHash[ h ].mentry = entry;
}


void
RaiMfeed_dict::addMfeedEnum( RaiMfeed_enumEntry *entry )
{
  unsigned int h;

  h = Hash32::hashInt( ( entry->fenum << 14 ) ^
                      ( (unsigned int) entry->value ) );
  h %= this->enumHashSize;
  while ( this->enumHashByValue[ h ] != NULL ) {
    if ( this->enumHashByValue[ h ]->fenum == entry->fenum &&
         this->enumHashByValue[ h ]->value == entry->value ) {
      Error e = RaiMfeedErr::getErr( RaiMfeedErr::DUPLICATE_ENUM );
      Log::printLog( Log::LVL_DEBUG, __FILE__, __LINE__, e,
                     "Value for %u: \"%s\" in enumtype.def",
                     (unsigned int) entry->value, entry->display );
    }
    h = ( h + 1 ) % this->enumHashSize;
  }
  this->enumHashByValue[ h ] = entry;

  h = Hash32::crc_cs( entry->display, false, entry->fenum );
  h %= this->enumHashSize;
  while ( this->enumHashByDisplay[ h ] != NULL ) {
    if ( this->enumHashByDisplay[ h ]->fenum == entry->fenum &&
       ::strcmp( this->enumHashByDisplay[ h ]->display, entry->display ) == 0) {
      /* there will be many duplicate display -> value entries */
      Error e = RaiMfeedErr::getErr( RaiMfeedErr::DUPLICATE_ENUM );
      Log::printLog( Log::LVL_DEBUG, __FILE__, __LINE__, e,
                     "Display for %u: \"%s\" in enumtype.def",
                     (unsigned int) entry->value, entry->display );
    }
    h = ( h + 1 ) % this->enumHashSize;
  }
  this->enumHashByDisplay[ h ] = entry;
}


RaiMfeed_dict *
RaiMfeed_dict::allocMfeedDict( unsigned int entryCount,
                               unsigned int enumListCount,
                               unsigned int enumListElemCount,
                               unsigned int flistCount,  unsigned int fidCount,
                               unsigned int mapFnameBytes,
                               unsigned int dictSize,
                               unsigned int enumListSize,
                               unsigned int enumAcroElemCount,
                               int maxFid,
                            unsigned int rwfTypeCount )
{
  RaiMfeed_dict * d;
  byte          * endPtr;
  unsigned int    hashSize,
                  enumHashSize,
                  mapSize,
                  flistMapSize,
                  fidMapSize;
  hashSize     = ( entryCount * 2 ) | 0x101;
  enumHashSize = ( enumListElemCount * 2 ) | 0x101;
  mapSize      = ( flistCount + fidCount ) * sizeof( RaiMfeed_mapEntry ) +
                 mapFnameBytes;
  flistMapSize = ( flistCount == 0 ) ? 0 : ( ( flistCount * 2 ) | 0x1 );
  fidMapSize   = ( fidCount == 0 ) ? 0 : ( ( fidCount * 2 ) | 0x101 );

  unsigned int dsz =
      sizeof( RaiMfeed_dict ) +
      hashSize * sizeof( RaiMfeed_entry2 ) +      /* fidHash[] ptr aligned */
      hashSize * sizeof( RaiMfeed_entry2 ) +      /* sassFidHash[] ptr aligned*/
      hashSize * sizeof( RaiMfeed_entry * ) +     /* acroHash[] ptr aligned */
      enumHashSize * 2 * sizeof( RaiMfeed_enumEntry * ) + /* enumHash[] ptr align */
      flistMapSize * 2 * sizeof( RaiMfeed_flistMap ) +/* flistMap[] ptr align */
      fidMapSize * 2 * sizeof( RaiMfeed_fidMap ) +  /* fidMap[] ptr align */
      enumListCount * sizeof( RaiMfeed_enumList ) + /* enumList[] ptr aligned */
      dictSize +                              /* entryBase[] int aligned */
      enumListSize +                          /* enumBase[] int aligned */
      mapSize +                               /* mapBase[] int aligned */
      entryCount * sizeof( Rai_u16 ) +        /* entrySize[] short align */
      ( enumAcroElemCount + enumListElemCount ) *
        sizeof( Rai_u16 ) +                   /* fidArray[] short aligned */
      sizeof( int );

  logDebug( LDEBUG, "MaxFid: %d, NumFids: %u, dictSize: %u, "
            "EnumListCount: %u, enumListSize: %u, flistMapSize: %u, "
            "fidMapSize: %u, mapSize: %u, totalSize: %u, rwfTypeCount: %u",
                    maxFid, entryCount, dictSize,
                    enumListElemCount, enumListSize, flistMapSize,
                    fidMapSize, mapSize, dsz, rwfTypeCount );

  MALLOC( dsz, &d );
  ::memset( d, 0, sizeof( RaiMfeed_dict ) );
  endPtr = &((byte *) d)[ dsz - sizeof( int ) ];
  ::memset( endPtr, 0xa5, sizeof( int ) );

  d->sig[ 0 ] = 'R'; d->sig[ 1 ] = 'a'; d->sig[ 2 ] = 'i'; d->sig[ 3 ] = 'M';
  d->sig[ 4 ] = 'f'; d->sig[ 5 ] = 'e'; d->sig[ 6 ] = 'e'; d->sig[ 7 ] = 'd';
  d->fidHash           = (RaiMfeed_entry2 *) &d[ 1 ];
  d->sassFidHash       = (RaiMfeed_entry2 *) &d->fidHash[ hashSize ];
  d->acroHash          = (RaiMfeed_entry **) &d->sassFidHash[ hashSize ];
  d->enumHashByValue   = (RaiMfeed_enumEntry **) &d->acroHash[ hashSize ];
  d->enumHashByDisplay = (RaiMfeed_enumEntry **)
                         &d->enumHashByValue[ enumHashSize ];
  d->flistMap          = (RaiMfeed_flistMap *)
                         &d->enumHashByDisplay[ enumHashSize ];
  d->fidMap            = (RaiMfeed_fidMap *) &d->flistMap[ flistMapSize * 2 ];
  d->enumList          = (RaiMfeed_enumList *) &d->fidMap[ fidMapSize * 2 ];
  d->hashSize          = hashSize;
  d->enumHashSize      = enumHashSize;
  d->entryCount        = entryCount;
  d->enumEntryCount    = enumListElemCount;
  d->enumListCount     = enumListCount;
  d->enumListSize      = enumListSize;
  d->flistMapSize      = flistMapSize;
  d->flistCount        = flistCount;
  d->fidMapSize        = fidMapSize;
  d->fidCount          = fidCount;
  d->mapFnameBytes     = mapFnameBytes;
  d->dictSize          = dictSize;
  d->enumAcroElemCount = enumAcroElemCount;
  d->rwfTypeCount      = rwfTypeCount;
  d->maxFid            = maxFid;
  d->endPtr            = endPtr;

  ::memset( d->fidHash, 0, sizeof( RaiMfeed_entry2 ) * hashSize );
  ::memset( d->sassFidHash, 0, sizeof( RaiMfeed_entry2 ) * hashSize );
  ::memset( d->acroHash, 0, sizeof( RaiMfeed_entry * ) * hashSize );
  ::memset( d->enumHashByValue, 0,
            sizeof( RaiMfeed_enumEntry * ) * enumHashSize );
  ::memset( d->enumHashByDisplay, 0,
            sizeof( RaiMfeed_enumEntry * ) * enumHashSize );
  ::memset( d->flistMap, 0, sizeof( RaiMfeed_flistMap ) * flistMapSize * 2 );
  ::memset( d->fidMap, 0, sizeof( RaiMfeed_fidMap ) * fidMapSize * 2 );
  ::memset( d->enumList, 0, sizeof( RaiMfeed_enumList ) * enumListCount );

  byte    * dictPtr   = (byte *) &d->enumList[ enumListCount ];
  byte    * enumPtr   = &dictPtr[ dictSize ];
  byte    * mapPtr    = &enumPtr[ enumListSize ];
  Rai_u16 * szPtr     = (Rai_u16 *) &mapPtr[ mapSize ];
  Rai_u16 * fidValPtr = &szPtr[ entryCount ];

  d->entryBase = (RaiMfeed_entry *) dictPtr;
  d->enumBase  = (RaiMfeed_enumEntry *) enumPtr;
  d->mapBase   = (RaiMfeed_mapEntry *) mapPtr;
  d->entrySize = szPtr;
  d->arrayBase = fidValPtr;

  return d;
}


void
RaiMfeed_dict::indexSass( void )
{
  const RaiMfeed_mapEntry * map;
  const RaiMfeed_entry    * mentry;
  const RaiMsg_dict       * entry;
  const RaiMsg_form       * form;
  Rai_u32                   i, h;

  if ( DataDictionary == NULL )
    return;
  for ( i = 0; i < this->hashSize; i++ ) {
    if ( (mentry = this->fidHash[ i ].mentry) != NULL ) {
      entry = DataDictionary->getEntry( mentry->acro );
      if ( entry != NULL ) {
        h  = Hash32::hashInt( (unsigned int) entry->fid );
        h %= this->hashSize;
        while ( this->sassFidHash[ h ].mentry != NULL )
          h = ( h + 1 ) % this->hashSize;
        this->sassFidHash[ h ].mentry = mentry;
        this->sassFidHash[ h ].entry  = entry;
      }
      this->fidHash[ i ].entry = entry;
    }
  }
  unsigned int errCount = 0;
  for ( i = 0; i < this->flistMapSize; i++ ) {
    if ( (map = this->flistMap[ i ].flistp) != NULL ) {
      if ( (form = DataDictionary->getForm( map->fname )) != NULL ) {
        this->flistMap[ i ].form = form;
        h = Hash32::hashInt( form->entry->fid ) % this->flistMapSize +
                                                 this->flistMapSize;
        while ( this->flistMap[ h ].form != NULL )
          h = ( h + 1 ) % this->flistMapSize + this->flistMapSize;
        this->flistMap[ h ].form   = form;
        this->flistMap[ h ].flistp = map;
      }
      else {
        if ( ++errCount < 10 ) {
          Error e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_FLIST_MAP );
          Log::printLog( Log::LVL_MINOR, __FILE__, __LINE__, e,
                         "flist entry %d, form %s", map->n, map->fname );
        }
      }
    }
  }
  if ( errCount >= 10 ) {
    Error e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_FLIST_MAP );
    Log::printLog( Log::LVL_MINOR, __FILE__, __LINE__, e,
                   "%d more flist entries undefined", errCount - 9 );
  }
  errCount = 0;
  for ( i = 0; i < this->fidMapSize; i++ ) {
    if ( (map = this->fidMap[ i ].fidp) != NULL ) {
      if ( (entry = DataDictionary->getEntry( map->fname )) != NULL ) {
        this->fidMap[ i ].entry = entry;
        h = Hash32::hashInt( entry->fid ) % this->fidMapSize + this->fidMapSize;
        while ( this->fidMap[ h ].entry != NULL )
          h = ( h + 1 ) % this->fidMapSize + this->fidMapSize;
        this->fidMap[ h ].entry = entry;
        this->fidMap[ h ].fidp  = map;
      }
      else {
        if ( ++errCount < 10 ) {
          Error e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_FID_MAP );
          Log::printLog( Log::LVL_MINOR, __FILE__, __LINE__, e,
                         "fid entry %d, field %s", map->n, map->fname );
        }
      }
    }
  }
  if ( errCount >= 10 ) {
    Error e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_FID_MAP );
    Log::printLog( Log::LVL_MINOR, __FILE__, __LINE__, e,
                   "%d more fid entries undefined", errCount - 9 );
  }
}


static const char entry_count_f[]          = "entry-count",
                  enum_entry_count_f[]     = "enum-entry-count",
                  enum_list_count_f[]      = "enum-list-count",
                  enum_list_size_f[]       = "enum-list-size",
                  flist_count_f[]          = "flist-count",
                  fid_count_f[]            = "fid-count",
                  map_fname_bytes_f[]      = "map-fname-bytes",
                  dict_size_f[]            = "dict-size",
                  enum_acro_elem_count_f[] = "enum-acro-elem-count",
                  max_fid_f[]              = "max-fid",
                  rwf_type_count_f[]       = "rwf-type-num",
                  acro_f[]                 = "a",
                  enum_f[]                 = "e",
                  enumlist_f[]             = "l",
                  map_f[]                  = "m",
                  rwf_f[]                  = "w";

void
RaiMfeed_dict::packDataDictionary2( RaiMsg &msg ) const
{
  byte         pak[ 8 * 1024 ];
  unsigned int i, j, k;

  msg.Append( "MARKETFEED", (RaiMsg *) NULL );
  msg.Activate( "." );

  msg.Append( entry_count_f, this->entryCount );
  msg.Append( enum_entry_count_f, this->enumEntryCount );
  msg.Append( enum_list_count_f, this->enumListCount );
  msg.Append( enum_list_size_f, this->enumListSize );
  msg.Append( flist_count_f, this->flistCount );
  msg.Append( fid_count_f, this->fidCount );
  msg.Append( map_fname_bytes_f, this->mapFnameBytes );
  msg.Append( dict_size_f, this->dictSize );
  msg.Append( enum_acro_elem_count_f, this->enumAcroElemCount );
  msg.Append( max_fid_f, this->maxFid );

  byte       * dictPtr   = (byte *) this->entryBase;
  byte       * enumPtr   = (byte *) this->enumBase;
  byte       * mapPtr    = (byte *) this->mapBase;
  Rai_u16    * szPtr     = this->entrySize;

  for ( i = 0; i < this->entryCount; i++ ) {
    const byte     * src;
    byte           * dst;
    RaiMfeed_entry * entry  = (RaiMfeed_entry *) dictPtr;

    Unaligned::endianPutInt( entry->fid, &pak[ 0 ] );
    Unaligned::endianPutInt( entry->ripple, &pak[ 2 ] );
    Unaligned::endianPutInt( entry->flen, &pak[ 4 ] );
    Unaligned::endianPutInt( entry->fenumLen, &pak[ 6 ] );
    Unaligned::endianPutInt( entry->fenum, &pak[ 8 ] );
    pak[ 10 ] = entry->ftype;
    pak[ 11 ] = entry->dde_off;
    ::memcpy( &pak[ 12 ], entry->acro, entry->dde_off );
    for ( dst = &pak[ 12 + entry->dde_off ],
          src = (byte *) &entry->acro[ entry->dde_off ];
          ( *dst++ = *src++ ) != '\0'; )
      ;
    dictPtr = &dictPtr[ *szPtr++ ];

    msg.Append( acro_f, RAIMSG_OPAQUE, (RaiMsg_size) ( dst - pak ), pak );
  }

  for ( i = 0; i < this->enumEntryCount; i++ ) {
    const byte         * src;
    byte               * dst;
    RaiMfeed_enumEntry * entry = (RaiMfeed_enumEntry *) enumPtr;
    size_t               sz;

    Unaligned::endianPutInt( entry->fenum, &pak[ 0 ] );
    Unaligned::endianPutInt( entry->value, &pak[ 2 ] );
    for ( dst = &pak[ 4 ], src = (byte *) entry->display;
          ( *dst++ = *src++ ) != '\0'; )
      ;
    sz  = (Rai_u16) ( dst - &pak[ 4 ] );
    sz += ::strlen( (char *) src ) + 1;
    sz  = ( ( sizeof( RaiMfeed_enumEntry ) - 4 + sz + 3U ) & ~3U );
    enumPtr = &enumPtr[ sz ];

    msg.Append( enum_f, RAIMSG_OPAQUE, (RaiMsg_size) ( dst - pak ), pak );
  }

  for ( i = 0; i < this->enumListCount; i++ ) {
    RaiMfeed_enumList & e = this->enumList[ i ];

    Unaligned::endianPutInt( e.fidCount, &pak[ 0 ] );
    Unaligned::endianPutInt( e.valueCount, &pak[ 2 ] );
    Unaligned::endianPutInt( e.minValue, &pak[ 4 ] );
    Unaligned::endianPutInt( e.maxValue, &pak[ 6 ] );

    for ( j = 0; j < e.fidCount; j++ )
      Unaligned::endianPutInt( e.fidArray[ j ], &pak[ 8 + j * 2 ] );

    j = 8 + j * 2;
    for ( k = 0; k < e.valueCount; k++ )
      Unaligned::endianPutInt( e.valueArray[ k ], &pak[ j + k * 2 ] );

    j += k * 2;
    msg.Append( enumlist_f, RAIMSG_OPAQUE, j, pak );
  }

  for ( i = 0; i < this->flistCount + this->fidCount; i++ ) {
    RaiMfeed_mapEntry * e = (RaiMfeed_mapEntry *) mapPtr;

    Unaligned::endianPutInt( e->n, &pak[ 0 ] );
    for ( j = 0; ( pak[ 2 + j ] = e->fname[ j ] ) != '\0'; j++ )
      ;
    msg.Append( map_f, RAIMSG_OPAQUE, j + 3, pak );
    for ( j++; ( j & 3 ) != 0; j++ )
      ;
    mapPtr = (byte *) &e->fname[ j ];
  }

  if ( this->rwfTypeCount != 0 ) {
    msg.Append( rwf_type_count_f, this->rwfTypeCount );

    dictPtr = (byte *) this->entryBase;
    szPtr   = this->entrySize;
    for ( i = 0; i < this->entryCount; i++ ) {
      RaiMfeed_entry * entry = (RaiMfeed_entry *) dictPtr;

      Unaligned::endianPutInt( entry->fid, &pak[ 0 ] );
      Unaligned::endianPutInt( entry->rwflen, &pak[ 2 ] );
      pak[ 4 ] = entry->rwftype;
      pak[ 5 ] = entry->rwfbits;
      dictPtr = &dictPtr[ *szPtr++ ];

      msg.Append( rwf_f, RAIMSG_OPAQUE, 6, pak );
    }
  }

  msg.Activate( NULL );
}


RaiMfeed_dict *
RaiMfeed_dict::unpackDataDictionary2( RaiMsg &msg )
{
  RaiMfeed_dict * d;
  RaiField     field,
               field2;
  RaiMsg       msg2,
             * msgPtr;
  unsigned int entryCount,
               enumEntryCount,
               enumListCount,
               enumListSize,
               flistCount,
               fidCount,
               mapFnameBytes,
               dictSize,
               enumAcroElemCount,
               rwfTypeCount;
  int          maxFid;
  unsigned int i, j, k;
  Error        e = RaiMfeedErr::getErr( RaiMfeedErr::BAD_MSG_FMT );

  /* check for rv7 _data_ opaque */
  if ( field.First( &msg ) && field.Type() == RAIMSG_OPAQUE &&
       field.isNamed( "_data_" ) ) {
    msg2.UnPack( (char *) field.Data(), field.Size() );
    msgPtr = &msg2;
  }
  else {
    msgPtr = &msg;
  }

  if ( ! msgPtr->Activate( "MARKETFEED" ) )
    return NULL;

  if ( ! field.First( msgPtr, entry_count_f, entryCount ) ||
       ! field.Next( enum_entry_count_f, enumEntryCount ) ||
       ! field.Next( enum_list_count_f, enumListCount ) ||
       ! field.Next( enum_list_size_f, enumListSize ) ||
       ! field.Next( flist_count_f, flistCount ) ||
       ! field.Next( fid_count_f, fidCount ) ||
       ! field.Next( map_fname_bytes_f, mapFnameBytes ) ||
       ! field.Next( dict_size_f, dictSize ) ||
       ! field.Next( enum_acro_elem_count_f, enumAcroElemCount ) ||
       ! field.Next( max_fid_f, maxFid ) ) {
    logError( LERROR, e, "Missing marketfeed field value" );
    throw e;
  }
  if ( field2.Find( msgPtr, rwf_type_count_f ) )
    field2.Get( rwfTypeCount );
  else
    rwfTypeCount = 0;
  d = RaiMfeed_dict::allocMfeedDict( entryCount, enumListCount,
                                     enumEntryCount, flistCount,
                                     fidCount, mapFnameBytes,
                                     dictSize, enumListSize,
                                     enumAcroElemCount, maxFid,
                                     rwfTypeCount );

  byte       * dictPtr   = (byte *) d->entryBase;
  byte       * enumPtr   = (byte *) d->enumBase;
  byte       * mapPtr    = (byte *) d->mapBase;
  Rai_u16    * szPtr     = d->entrySize;
  Rai_u16    * fidValPtr = d->arrayBase;

  try {
    for ( i = 0; i < d->entryCount; i++ ) {
      const byte     * acropak,
                     * src;
      byte           * dst;
      RaiMfeed_entry * entry  = (RaiMfeed_entry *) dictPtr;

      if ( ! field.Next( acro_f, acropak ) ) {
        logError( LERROR, e, "Missing marketfeed acro field" );
        throw e;
      }
      Unaligned::endianGetInt( &acropak[ 0 ], entry->fid );
      Unaligned::endianGetInt( &acropak[ 2 ], entry->ripple );
      Unaligned::endianGetInt( &acropak[ 4 ], entry->flen );
      Unaligned::endianGetInt( &acropak[ 6 ], entry->fenumLen );
      Unaligned::endianGetInt( &acropak[ 8 ], entry->fenum );
      entry->ftype   = acropak[ 10 ];
      entry->dde_off = acropak[ 11 ];
      ::memcpy( entry->acro, &acropak[ 12 ], entry->dde_off );
      for ( src = &acropak[ 12 + entry->dde_off ],
            dst = (byte *) &entry->acro[ entry->dde_off ];
            ( *dst++ = *src++ ) != '\0'; )
        ;
      *szPtr  = (Rai_u16) ( src - &acropak[ 12 ] );
      *szPtr  = ( ( sizeof( RaiMfeed_entry ) - 4 + *szPtr + 3U ) & ~3U );
      dictPtr = &dictPtr[ *szPtr++ ];

      d->addMfeedEntry( entry );
    }

    for ( i = 0; i < d->enumEntryCount; i++ ) {
      const byte         * enumpak,
                         * src;
      byte               * dst;
      RaiMfeed_enumEntry * entry = (RaiMfeed_enumEntry *) enumPtr;
      Rai_u16              sz;

      if ( ! field.Next( enum_f, enumpak ) ) {
        logError( LERROR, e, "Missing marketfeed enum field" );
        throw e;
      }
      Unaligned::endianGetInt( &enumpak[ 0 ], entry->fenum );
      Unaligned::endianGetInt( &enumpak[ 2 ], entry->value );
      for ( src = &enumpak[ 4 ], dst = (byte *) entry->display;
            ( *dst++ = *src++ ) != '\0'; )
        ;
      sz = (Rai_u16) ( src - &enumpak[ 4 ] );
      sz = ( ( sizeof( RaiMfeed_enumEntry ) - 4 + sz + 3U ) & ~3U );
      enumPtr = &enumPtr[ sz ];

      d->addMfeedEnum( entry );
    }

    for ( i = 0; i < d->enumListCount; i++ ) {
      const byte        * listpak;
      RaiMfeed_enumList & l = d->enumList[ i ];

      if ( ! field.Next( enumlist_f, listpak ) ) {
        logError( LERROR, e, "Missing marketfeed enum list field" );
        throw e;
      }
      Unaligned::endianGetInt( &listpak[ 0 ], l.fidCount );
      Unaligned::endianGetInt( &listpak[ 2 ], l.valueCount );
      Unaligned::endianGetInt( &listpak[ 4 ], l.minValue );
      Unaligned::endianGetInt( &listpak[ 6 ], l.maxValue );
      l.fidArray   = (Rai_i16 *) fidValPtr;
      l.valueArray = (Rai_u16 *) &l.fidArray[ l.fidCount ];
      fidValPtr    = &l.valueArray[ l.valueCount ];

      for ( j = 0; j < l.fidCount; j++ )
        Unaligned::endianGetInt( &listpak[ 8 + j * 2 ], l.fidArray[ j ] );

      j = 8 + j * 2;
      for ( k = 0; k < l.valueCount; k++ )
        Unaligned::endianGetInt( &listpak[ j + k * 2 ], l.valueArray[ k ] );
    }

    for ( i = 0; i < d->flistCount + d->fidCount; i++ ) {
      const byte        * mappak;
      RaiMfeed_mapEntry * m = (RaiMfeed_mapEntry *) mapPtr;

      if ( ! field.Next( map_f, mappak ) ) {
        logError( LERROR, e, "Missing marketfeed map field" );
        throw e;
      }
      Unaligned::endianGetInt( &mappak[ 0 ], m->n );
      for ( j = 0; ( m->fname[ j ] = (char) mappak[ 2 + j ] ) != '\0'; j++ )
        ;
      for ( j++; ( j & 3 ) != 0; j++ )
        ;
      mapPtr = (byte *) &m->fname[ j ];

      if ( i < d->flistCount )
        d->addMfeedFlistMap( m );
      else
        d->addMfeedFidMap( m );
    }

    if ( d->rwfTypeCount != 0 ) {
      if ( field.Next( rwf_type_count_f, d->rwfTypeCount ) ) {
        dictPtr = (byte *) d->entryBase;
        szPtr   = d->entrySize;
        for ( i = 0; i < d->entryCount; i++ ) {
          RaiMfeed_entry * entry = (RaiMfeed_entry *) dictPtr;
          const byte     * pak;
          Rai_i16          fid;

          if ( ! field.Next( rwf_f, pak ) ) {
            logError( LERROR, e, "Missing marketfeed rwf type" );
            throw e;
          }
          Unaligned::endianGetInt( &pak[ 0 ], fid );
          if ( fid != entry->fid ) {
            logError( LERROR, e, "Marketfeed rwf type fid mismatch" );
            throw e;
          }
          Unaligned::endianGetInt( &pak[ 2 ], entry->rwflen );
          entry->rwftype = pak[ 4 ];
          entry->rwfbits = pak[ 5 ];
          dictPtr = &dictPtr[ *szPtr++ ];
        }
      }
    }
  } catch ( Error e2 ) {
    RaiMfeed_dict::release( d );
    msgPtr->Activate( NULL );
    throw e2;
  }
  msgPtr->Activate( NULL );

  d->msgType   = d->getEntry( "MSG_TYPE" );
  d->recType   = d->getEntry( "REC_TYPE" );
  d->seqNo     = d->getEntry( "SEQ_NO" );
  d->recStatus = d->getEntry( "REC_STATUS" );
  d->tstamp    = NULL;

  return d;
}


RaiMsgException
RaiMfeedErr::getErr( unsigned int status )
{
  static const char     mod[] = "RaiMfeed";
  static const ErrorRec err[] = {
  /*  0 */ { BAD_ENUM_LINE_FMT, "Bad enumtype.def line format", mod },
  /*  1 */ { BAD_APP_LINE_FMT,  "Bad appendix_a line format", mod },
  /*  2 */ { DUPLICATE_ACRO,    "Duplicate acronym declared", mod },
  /*  3 */ { DUPLICATE_FID,     "Duplicate fid declared", mod },
  /*  4 */ { NO_RIPPLE_ACRO,    "Unable to find ripple acronym", mod },
  /*  5 */ { NO_ENUM_ACRO,      "Unable to find acronym", mod },
  /*  6 */ { ENUM_FID_MISMATCH, "Enumtype.def declared fid doesn't match as "
                                "declared", mod },
  /*  7 */ { ACRO_NOT_ENUM,     "Acronym is not an ENUMERATED type", mod },
  /*  8 */ { DUPLICATE_ENUM,    "Duplicate enumeration", mod },
  /*  9 */ { REDECLARED_ENUM,   "Redeclared acronym", mod },
  /* 10 */ { ENUM_COUNT_MIS,    "Count of enumerations doesn't match", mod },
  /* 11 */ { BAD_FLIST_MAP, "Flist value doesn't map to a tss record", mod },
  /* 12 */ { BAD_FID_MAP,   "Fid value doesn't map to a tss field", mod },
  /* 13 */ { BAD_FLIST_FMT, "Flist file line format error [num, record]", mod },
  /* 14 */ { BAD_FID_FMT,   "Fid file line format error [num, field]", mod },
  /* 15 */ { BAD_MSG_FMT,   "Marketfeed packed dictionary message bad", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}
