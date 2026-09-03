/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIMSG_DLL_EXP ) && defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllexport)
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "msg/dict.h"
#include "msg/msg.h"
#include "msg/defs.h"
#include "msg/field.h"
#include "msg/cfile_parser.h"
#include "util/hash_util.h"
#include "util/int_bits.h"
#include "util/atomic.h"
#include "util/linear_hash_table.h"
#include "base/log.h"
#include "base/thread.h"

#if defined( _WIN32 ) || defined( _WIN64 )
#include <float.h>
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
#endif

using namespace rai;

static const char MSG_TYPE_STRING[]       = "MSG_TYPE";
static const char REC_TYPE_STRING[]       = "REC_TYPE";
static const char SEQ_NO_STRING[]         = "SEQ_NO";
static const char REC_STATUS_STRING[]     = "REC_STATUS";
static const char ANONYMOUS_FORM_STRING[] = "anonymous-form";

enum InvalidType {
  RAI_TSS_TYPE_OK,
  BAD_TSS_TYPE,
  BAD_TSS_SIZE
};

static InvalidType
tssDataTypeIsValid( unsigned int dataType,  unsigned int dataSize,
                    bool isPartial )
{
  if ( isPartial && dataType != RAI_TSS_STRING && dataType != RAI_TSS_OPAQUE )
    return BAD_TSS_TYPE;

  switch ( dataType ) {
    case RAI_TSS_SDATE:
      /*if ( dataSize >  )
        return RAI_TSS_TYPE_OK;
      break;*/
    case RAI_TSS_STIME:
      /*if ( dataSize == 9 )
        return RAI_TSS_TYPE_OK;
      break;*/
    case RAI_TSS_STRING:
    case RAI_TSS_OPAQUE:
      if ( dataSize < 0xffffU )
        return RAI_TSS_TYPE_OK;
      break;
    case RAI_TSS_DATE:
    case RAI_TSS_TIME:
      if ( dataSize == 6 )
        return RAI_TSS_TYPE_OK;
      break;
    case RAI_TSS_GROCERY:
      if ( dataSize == 9 || dataSize == 5 )
        return RAI_TSS_TYPE_OK;
      break;
    case RAI_TSS_PRICE:
    case RAI_TSS_FLOAT:
    case RAI_TSS_DOUBLE:
    case RAI_TSS_DOUBLE_INT:
      if ( dataSize == 4 || dataSize == 8 )
        return RAI_TSS_TYPE_OK;
      break;
    case RAI_TSS_INTEGER:
    case RAI_TSS_SHORT_INT:
    case RAI_TSS_LONG:
    case RAI_TSS_BOOLEAN:
    case RAI_TSS_BYTE:
    case RAI_TSS_U_SHORT:
    case RAI_TSS_U_INT:
    case RAI_TSS_U_LONG:
      if ( dataSize == 1 || dataSize == 2 || dataSize == 4 || dataSize == 8 )
        return RAI_TSS_TYPE_OK;
      break;
    default:
      return BAD_TSS_TYPE;
  }
  return BAD_TSS_SIZE;
}


const char *
RaiMsg_config::tssTypeString( unsigned int t )
{
  switch ( t ) {
    default:
    case RAI_TSS_NODATA:     return "TSS_NODATA";
    case RAI_TSS_INTEGER:    return "TSS_INTEGER";
    case RAI_TSS_STRING:     return "TSS_STRING";
    case RAI_TSS_BOOLEAN:    return "TSS_BOOLEAN";
    case RAI_TSS_DATE:       return "TSS_DATE";
    case RAI_TSS_TIME:       return "TSS_TIME";
    case RAI_TSS_PRICE:      return "TSS_PRICE";
    case RAI_TSS_BYTE:       return "TSS_BYTE";
    case RAI_TSS_FLOAT:      return "TSS_FLOAT";
    case RAI_TSS_SHORT_INT:  return "TSS_SHORT_INT";
    case RAI_TSS_DOUBLE:     return "TSS_DOUBLE";
    case RAI_TSS_OPAQUE:     return "TSS_OPAQUE";
    case RAI_TSS_NULL:       return "TSS_NULL";
    case RAI_TSS_RESERVED:   return "TSS_RESERVED";
    case RAI_TSS_DOUBLE_INT: return "TSS_DOUBLE_INT";
    case RAI_TSS_GROCERY:    return "TSS_GROCERY";
    case RAI_TSS_SDATE:      return "TSS_SDATE";
    case RAI_TSS_STIME:      return "TSS_STIME";
    case RAI_TSS_LONG:       return "TSS_LONG";
    case RAI_TSS_U_SHORT:    return "TSS_U_SHORT";
    case RAI_TSS_U_INT:      return "TSS_U_INT";
    case RAI_TSS_U_LONG:     return "TSS_U_LONG";
  }
}


RaiMsg_config *
RaiMsg_config::parseDictionary( const char *tss_fields_fname,
                                const char *tss_records_fname,
                                const char *cfile_path,
                                char path_sep,
                                CFileLocator *loc )
{
  CFileParser   * parser;
  CFileStrings  * strings;
  CFileExpr     * expr,
                * expr2;
  RaiMsg_config * dict;

  parser  = NULL;
  strings = NULL;
  dict    = NULL;

  try {
    strings = CFileParser::createStrings();
    parser  = NEW CFileParser( strings );
    if ( cfile_path == NULL )
      cfile_path = ::getenv( "cfile_path" );
    /*if ( path_sep == 0 )
      path_sep = ' ';*/
    if ( tss_fields_fname == NULL )
      tss_fields_fname = "tss_fields.cf";
    expr = parser->parsePath( loc, tss_fields_fname, cfile_path, path_sep );
    if ( tss_records_fname != NULL ) {
      expr2 = parser->parsePath( loc, tss_records_fname, cfile_path, path_sep );
      expr  = CFileExpr::append( expr, expr2 );
    }

    dict = RaiMsg_config::parseDictionary( expr );

    delete parser;
    CFileParser::releaseStrings( strings );

    return dict;
  } catch ( ... ) {
    if ( parser != NULL )
      delete parser;
    if ( strings != NULL )
      CFileParser::releaseStrings( strings );
    throw;
  }
}


RaiMsg_config *
RaiMsg_config::parseDictionary( InputStream *in )
{
  CFileParser   * parser;
  CFileStrings  * strings;
  CFileExpr     * expr;
  RaiMsg_config * dict;

  parser  = NULL;
  strings = NULL;
  dict    = NULL;

  try {
    strings = CFileParser::createStrings();
    parser  = NEW CFileParser( strings );
    expr    = parser->parseStream( in );

    dict = RaiMsg_config::parseDictionary( expr );

    delete parser;
    CFileParser::releaseStrings( strings );

    return dict;
  } catch ( ... ) {
    if ( parser != NULL )
      delete parser;
    if ( strings != NULL )
      CFileParser::releaseStrings( strings );
    throw;
  }
}

static bool
tssFixEntry2( Rai_u32 &fnameLen,  Rai_u32 &dataType,  Rai_u32 &dataSize,
              bool &isPartial );

RaiMsg_config *
RaiMsg_config::parseDictionary( CFileExpr *expr )
{
  CFileExpr         * defn,
                    * fields,
                    * fieldClass,
                    * fld,
                    * classIdDefn;
  CFileExprIter       iter;
  RaiMsg_config     * dict;
  RaiMsg_dict       * entry;
  RaiMsg_form       * form;
  Error               e;
  const char        * name,
                    * fieldName,
                    * className,
                    * classTable[ MAX_FID ];
  Rai_u16           * index;
  char              * ptr;
  unsigned short      classIdTable[ MAX_FID ],
                      formFieldFids[ MAX_FID ];
  unsigned int        count,
                      formCount,
                      fieldCount,
                      formFieldCount,
                      classCount,
                      len,
                      nameLen,
                      classId,
                      dataSize,
                      dataType,
                      i,
                      j,
                      indexSize;
  bool                isPrimitive,
                      isFixed,
                      isPartial,
                      hasErrors;

  dict      = NULL;
  index     = NULL;
  hasErrors = false;

  try {
    if ( expr != NULL ) {
      len        = 0;
      classCount = 0;
      formCount  = 0;
      fieldCount = 0;
      indexSize  = 0;
      ::memset( classTable, 0, sizeof( classTable ) );
      ::memset( classIdTable, 0, sizeof( classIdTable ) );

      /* find all of the field classes */
      iter.push( expr );
      if ( (classIdDefn = iter.find( "CLASS_ID" )) != NULL ) {
        do {
          name = iter.parent()->getIdent();
          /* can't have a null name */
          if ( name == NULL ) {
            e = RaiMsgErr::getErr( RaiMsgErr::NULL_DICT_FNAME );
            logError( LERROR, e, "Class not named" );
            hasErrors = true;
          }
          /* can't be more field classes than fids allowed */
          else if ( classCount == MAX_FID ) {
            e = RaiMsgErr::getErr( RaiMsgErr::TOO_MANY_CLASSES );
            logError( LERROR, e, "Maximum %u", MAX_FID );
            throw e;
          }
          /* hash the class name for later */
          else {
            nameLen = ::strlen( name );
            len    += nameLen + 1;
            i       = Hash32::crc_c( (Rai_u8 *) name, nameLen );

            for ( i &= ( MAX_FID - 1 ); ; i = ( i + 1 ) & ( MAX_FID - 1 ) ) {
              if ( classTable[ i ] == NULL ) {
                classTable[ i ]   = name;
                classIdDefn->getValue( classId );
                classIdTable[ i ] = classId;
                /* count the field classes */
                classCount++;
                break;
              }
              if ( ::strcmp( classTable[ i ], name ) == 0 )
                break;
            }
          }
        } while ( (classIdDefn = iter.findNext( "CLASS_ID" )) != NULL );
      }

      /* find all of the form class definitions */
      iter.clear();
      iter.push( expr );
      if ( (fields = iter.find( "FIELDS" )) != NULL ) {
        /* foreach form */
        do {
          formFieldCount = 0;
          if ( (fields = fields->getChild()) != NULL ) {
            /* foreach field in the form */
            do {
              /* check each of the form fields classes */
              if ( (fieldName = fields->getIdent()) != NULL &&
                   (fieldClass = fields->getChild()) != NULL &&
                   (name = fieldClass->getIdent()) != NULL &&
                   ::strcmp( name, "FIELD_CLASS_NAME" ) == 0 &&
                   fieldClass->getValue( className ) &&
                   className != NULL ) {

                nameLen = ::strlen( className );
                i       = Hash32::crc_c( (Rai_u8 *) className, nameLen );

                /* find form field in class table */
                for ( i &= ( MAX_FID - 1 ); ;
                      i = ( i + 1 ) & ( MAX_FID - 1 ) ) {
                  if ( classTable[ i ] == NULL ) {
                    e = RaiMsgErr::getErr( RaiMsgErr::UNDEFINED_CLASS );
                    logError( LERROR, e, "Class %s referenced but undefined",
                              className );
                    hasErrors = true;
                    break;
                  }
                  /* found it */
                  if ( ::strcmp( classTable[ i ], className ) == 0 ) {
                    /* sum the lengths of all the field names */
                    len += ::strlen( fieldName ) + 1;
                    /* all of the fids in this form */
                    formFieldFids[ formFieldCount++ ] = classIdTable[ i ];
                    /* count all of the fields */
                    fieldCount++;
                    break;
                  }
                }
              }
            } while ( (fields = fields->getNext()) != NULL );
          }

          /*if ( ! hasErrors ) {*/
            indexSize +=
              RaiMsg_form::calcIndexSize( formFieldFids, formFieldCount );
            formCount++;
          /*}*/
        } while ( (fields = iter.findNext( "FIELDS" )) != NULL );
      }

      /*if ( hasErrors )
        throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_SYNTAX );*/

      count = classCount + fieldCount;
      if ( count > 0 ) {
        /* align everything on long long boundary */
        unsigned int sz1 = ( sizeof( RaiMsg_config ) + 7 ) & ~7,
                     sz2 = ( ( count * sizeof( RaiMsg_dict ) ) + 7 ) & ~7,
                     sz3 = ( ( formCount * sizeof( RaiMsg_form ) ) + 7 ) & ~7,
                     sz4 = ( indexSize * sizeof( Rai_u16 ) + 7 ) & ~7,
                     dictSize = sz1 + sz2 + sz3 + sz4 + len;

        MALLOC( sz1 + sz2 + sz3 + sz4 + len, &dict );
        logDebug( LDEBUG, "classCount %u fieldCount %u, formCount %u indexSize %u, dictSize %u\n",
                  classCount, fieldCount, formCount, indexSize, dictSize );

        dict->init();
        dict->dictSize = dictSize;
        dict->entry    = (RaiMsg_dict *) &((Rai_u8 *) dict)[ sz1 ];
        dict->form     = (RaiMsg_form *) &((Rai_u8 *) dict)[ sz1 + sz2 ];
        index          = (Rai_u16 *) &((Rai_u8 *) dict)[ sz1 + sz2 + sz3 ];
        ptr            = (char *) &((Rai_u8 *) dict)[ sz1 + sz2 + sz3 + sz4 ];

        classCount = 0;
        formCount  = 0;
        /* get the first field class again */
        iter.clear();
        iter.push( expr );
        iter.find( "CLASS_ID" );

        do {
          defn        = iter.parent();
          className   = defn->getIdent();
          defn        = defn->getChild();
          classId     = 0xffffU;
          dataSize    = 0;
          dataType    = RAI_TSS_NULL;
          isPrimitive = true;
          isPartial   = false;
          isFixed     = true;
          fields      = NULL;

          /* fetch all the classes attributes */
          do {
            if ( (name = defn->getIdent()) != NULL ) {
              if ( ::strcmp( name, "CLASS_ID" ) == 0 )
                defn->getValue( classId );
              else if ( ::strcmp( name, "IS_PRIMITIVE" ) == 0 )
                defn->getValue( isPrimitive );
              else if ( ::strcmp( name, "IS_FIXED" ) == 0 )
                defn->getValue( isFixed );
              else if ( ::strcmp( name, "IS_PARTIAL" ) == 0 )
                defn->getValue( isPartial );
              else if ( ::strcmp( name, "DATA_SIZE" ) == 0 )
                defn->getValue( dataSize );
              else if ( ::strcmp( name, "DATA_TYPE" ) == 0 )
                defn->getValue( dataType );
              else if ( ::strcmp( name, "FIELDS" ) == 0 ) {
                isPrimitive = false;
                fields = defn;
              }
            }
          } while ( (defn = defn->getNext()) != NULL );

          /* a form */
          if ( ! isPrimitive )
            dataType = RAI_TSS_NULL;

          /* check that the class id is a valid fid */
          if ( classId >= MAX_FID ) {
            e = RaiMsgErr::getErr( RaiMsgErr::BIG_FID );
            logError( LERROR, e, "Invalid CLASS_ID (%u) %s", classId,
                      className );
            hasErrors = true;
          }
          /* check that another entry doesn't have the same fid */
          else if ( dict->fidIndex[ classId ] != 0xffffU ) {
            const RaiMsg_dict *tmp = &dict->entry[ dict->fidIndex[ classId ] ];
            static Error dup_fid_err;
            if ( dup_fid_err == NULL )
              dup_fid_err = RaiMsgErr::getErr( RaiMsgErr::DUPLICATE_FID );
            e = dup_fid_err;
            Log::LogLevel lvl = Log::LVL_ERROR;
            if ( ::strcmp( tmp->fname, className ) == 0 &&
                 tmp->fid == classId ) {
              Rai_u32 tmplen = tmp->fname_size;
              if ( tmp->ftype == dataType &&
                   tmp->fsize == dataSize && tmp->partial == isPartial ) {
                lvl = Log::LVL_DEBUG;
              }
              else if ( tssFixEntry2( tmplen, dataType, dataSize,
                                      isPartial ) ) {
                if ( tmp->ftype == dataType &&
                     tmp->fsize == dataSize && tmp->partial == isPartial )
                  lvl = Log::LVL_DEBUG;
              }
            }
            if ( lvl == Log::LVL_ERROR )
              hasErrors = true;

            Log::printLog( lvl, __FILE__, __LINE__, e,
                           "Redeclared CLASS_ID (%u) %s/%s (ftype=%u/%u;%u/%u)",
                           classId, className, tmp->fname,
                           tmp->ftype, tmp->fsize,
                           dataType, dataSize );
          }
          else {
            const RaiMsg_dict *redeclared = NULL;
            len = ::strlen( className ) + 1;
            i   = Hash32::crc_c( (Rai_u8 *) className, len - 1 );

            /* lookup the entry, check that it's unique, even if it's declared
             * more than once */
            for ( i &= ( MAX_FID - 1 ); dict->hashIndex[ i ] != NULL;
                  i = ( i + 1 ) & ( MAX_FID - 1 ) ) {
              if ( dict->hashIndex[ i ]->fname_size == (Rai_u8) len &&
                   ::strcmp( dict->hashIndex[ i ]->fname, className ) == 0 ) {
                if ( dict->hashIndex[ i ]->fid != classId ||
                     dict->hashIndex[ i ]->ftype != dataType ||
                     dict->hashIndex[ i ]->fsize != dataSize ||
                     (bool) dict->hashIndex[ i ]->partial != isPartial ||
                     ! isPrimitive ) {
                  redeclared = dict->hashIndex[ i ];
                }
                break;
              }
            }

            if ( redeclared != NULL ) {
              e = RaiMsgErr::getErr( RaiMsgErr::DUPLICATE_CLASS );
              logError( LERROR, e, "Class %s redeclared fids=%u/%u, using "
                                   "first definition", className,
                        redeclared->fid, classId );
              hasErrors = true;
            }
            else {
              /* initialize the entry */
              if ( (entry = dict->hashIndex[ i ]) == NULL ) {
                entry = &dict->entry[ classCount++ ];
                dict->hashIndex[ i ] = entry;
                entry->init( ptr, len );

                ::memcpy( ptr, className, len );
                ptr = &ptr[ len ];
              }
              /* index by fid */
              dict->fidIndex[ classId ] = (Rai_u16) ( entry - dict->entry );
              entry->fid = (Rai_u16) classId;

              /* check that the primative type is valid */
              if ( isPrimitive ) {
                e = NULL;
                switch ( tssDataTypeIsValid( dataType, dataSize, isPartial ) ) {
                  case BAD_TSS_TYPE:
                    e = RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_TYPE );
                    break;
                  case BAD_TSS_SIZE:
                    e = RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_SIZE );
                    break;
                  case RAI_TSS_TYPE_OK:
                    break;
                }
                if ( e != NULL ) {
                  Rai_u32 tmplen = len;
                  if ( tssFixEntry2( tmplen, dataType, dataSize, isPartial ) ) {
                    logDebug( LDEBUG,
                      "Fixed, DATA_TYPE (%u), DATA_SIZE (%u) in class %s",
                              dataType, dataSize, className );
                    e = NULL;
                  }
                }
                if ( e == NULL ) {
                  entry->ftype   = dataType;
                  entry->fsize   = dataSize;
                  entry->partial = ( isPartial ? 1 : 0 );
                }
                else {
                  logError( LERROR, e,
                            "DATA_TYPE (%u), DATA_SIZE (%u) in class %s",
                            dataType, dataSize, className );
                  hasErrors = true;
                }
              }
              else {
                /* make a form definition */
                if ( fields != NULL )
                  fields = fields->getChild();
                /*if ( fields == NULL || (fields = fields->getChild()) == NULL ) {
                  e = RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS );
                  logError( LERROR, e, "Form %s has no fields", className );
                  hasErrors = true;
                }
                else*/ {
                  /* the forms dictionary entry */
                  entry->ftype   = RAI_TSS_NULL;
                  entry->fsize   = 0;
                  entry->partial = 0;

                  fieldCount = 0;
                  if ( fields != NULL ) {
                    fieldCount++;
                    for ( fld = fields->getNext(); fld != NULL;
                          fld = fld->getNext() )
                      fieldCount++;
                  }
                  /* the forms fields must be at the end of the dictionary
                     entry array because the fidIndex[] is only 16 bits */
                  form = &dict->form[ dict->formCount++ ];
                  form->init( dict );
                  form->entry  = entry;
                  count       -= fieldCount;
                  form->fields = &dict->entry[ count ];

                  /* for each field in the form */
                  for ( ; fields != NULL; fields = fields->getNext() ) {
                    fieldName = fields->getIdent();
                    if ( fieldName == NULL ) {
                      e = RaiMsgErr::getErr( RaiMsgErr::NULL_DICT_FNAME );
                      logError( LERROR, e, "Field of class %s should be named",
                                form->entry->fname );
                    }
                    else {
                      if ( (fieldClass = fields->getChild()) == NULL ||
                           (name = fieldClass->getIdent()) == NULL ||
                           ::strcmp( name, "FIELD_CLASS_NAME" ) != 0 ||
                           ! fieldClass->getValue( className ) ||
                           className == NULL ) {
                      field_class_not_found:;
                        e = RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS );
                        logError( LERROR, e, "Field %s of class %s missing "
                                             "FIELD_CLASS_NAME",
                                  fieldName, form->entry->fname );
                        hasErrors = true;
                      }
                      else {
                        len = ::strlen( className ) + 1;
                        i   = Hash32::crc_c( (const Rai_u8 *) className,
                                               len - 1 );
                        for ( i &= ( MAX_FID - 1 ); ;
                              i = ( i + 1 ) & ( MAX_FID - 1 ) ) {
                          if ( classTable[ i ] == NULL )
                            goto field_class_not_found;
                          if ( ::strcmp( classTable[ i ], className ) == 0 )
                            break;
                        }

                        /* initialize the entry for the field, the
                         * size and offset will be filled in later after
                         * all of the field classes are initialized */
                        entry = &form->fields[ form->fieldCount++ ];
                        len   = ::strlen( fieldName ) + 1;

                        entry->init( ptr, len );
                        entry->fid = (Rai_u16) i; /* temporary */

                        ::memcpy( ptr, fieldName, len );
                        ptr = &ptr[ len ];
                      }
                    }
                  }
                }
              }
            }
          }
        } while ( iter.findNext( "CLASS_ID" ) != NULL );

        /* the total number of field classes, excluding the entries
         * added for each of the form fields */
        dict->entryCount = classCount;
      }
    }

    if ( hasErrors || dict == NULL || dict->entryCount == 0 ) {
      if ( ! hasErrors ) {
        e = RaiMsgErr::getErr( RaiMsgErr::BAD_DICTIONARY );
        logError( LERROR, e, "No classes defined" );
      }
      if ( dict == NULL || dict->entryCount == 0 )
        throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_SYNTAX );
    }

    /* foreach form, initialize all of the form's fields */
    for ( i = 0; i < dict->formCount; i++ ) {
      form = &dict->form[ i ];

      for ( j = 0; j < form->fieldCount; j++ )
        formFieldFids[ j ] = classIdTable[ form->fields[ j ].fid ];

      form->fidIndex = index;
      form->initFields( formFieldFids, j, false );
      index = &index[ form->indexSize() ];
      form->computeHashes();

      dict->fidForm[ form->entry->fid ] = i;
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

    dict->dynamicFormLock = Mutex::create();

    return dict;
  } catch ( ... ) {
    if ( dict != NULL )
      FREE( dict );
    throw;
  }
}

RaiMsg_config *
RaiMsg_config::parseDictionary( InputStream *in, 
                                const char *fname,
                                const char *cfile_path,
                                char path_sep )
{
  CFileParser   * parser;
  CFileStrings  * strings;
  CFileExpr     * expr,
                * expr2;
  RaiMsg_config * dict;

  parser  = NULL;
  strings = NULL;
  dict    = NULL;

  try {
    strings = CFileParser::createStrings();
    parser  = NEW CFileParser( strings );
    if ( cfile_path == NULL )
      cfile_path = ::getenv( "cfile_path" );
    /*if ( path_sep == 0 )
      path_sep = ' ';*/
    
    expr = parser->parsePath( NULL, fname, cfile_path, path_sep );
    expr2 = parser->parseStream( in );
    expr  = CFileExpr::append( expr, expr2 );
    dict  = RaiMsg_config::parseDictionary( expr );

    delete parser;
    CFileParser::releaseStrings( strings );
    return dict;
  } catch ( ... ) {
    if ( parser != NULL )
      delete parser;
    if ( strings != NULL )
      CFileParser::releaseStrings( strings );
    throw;
  }
  return dict;
}

void
RaiMsg_form::initFields( Rai_u16 *fid,  unsigned int fidCount,
                         bool setFieldNames )
{
  RaiMsg_dict       * entry;
  const RaiMsg_dict * classEntry;
  unsigned int        j,
                      len,
                      bitOffset;
  Rai_u16           * idx;
  Rai_u32             val;

  this->fieldCount = fidCount;

  j = RaiMsg_form::getIndexSize( fid, fidCount, this->fidBits,
                                 this->fieldBits );
  this->fidMask = ( 1U << this->fidBits ) - 1;
  ::memset( this->fidIndex, 0, sizeof( this->fidIndex[ 0 ] ) * j );

  len = 0;
  for ( j = 0; j < fidCount; j++ ) {
    entry      = &this->fields[ j ];
    classEntry = this->dict->getEntry( fid[ j ] );

    if ( setFieldNames )
      entry->init( classEntry->fname, classEntry->fname_size );
    entry->fsize   = classEntry->fsize;
    entry->fid     = classEntry->fid;
    entry->ftype   = classEntry->ftype;
    entry->partial = classEntry->partial;
    entry->foffset = (Rai_u16) len;

    len += entry->packSize();

    bitOffset = (unsigned int) ( entry->fid & this->fidMask ) * this->fieldBits;
    idx       = &this->fidIndex[ bitOffset >> 4 ];
    val       = (Rai_u32) idx[ 0 ] | ( (Rai_u32) idx[ 1 ] << 16 );
    val      |= j << ( bitOffset & 15U );
    idx[ 0 ]  = (Rai_u16) val;
    idx[ 1 ]  = (Rai_u16) ( val >> 16 );
  }
  this->entry->fsize = len;

  this->msgType   = this->getEntry( MSG_TYPE_STRING,
                                    sizeof( MSG_TYPE_STRING ) );
  this->recType   = this->getEntry( REC_TYPE_STRING,
                                    sizeof( REC_TYPE_STRING ) );
  this->seqNo     = this->getEntry( SEQ_NO_STRING,
                                    sizeof( SEQ_NO_STRING ) );
  this->recStatus = this->getEntry( REC_STATUS_STRING,
                                    sizeof( REC_STATUS_STRING ) );
}


void
RaiMsg_form::computeHashes( void )
{
  Rai_u16        fid[ MAX_FID ];
  unsigned int   fidCount;
  RaiMsg_dict  * entry;
  Rai_u8         buf[ 1024 ];
  unsigned int   i,
                 off;

  off      = 0;
  fidCount = 0;
  ::memcpy( &buf[ off ], this->entry->fname, this->entry->fname_size );
  off += this->entry->fname_size;

  for ( i = 0; i < this->fieldCount; i++ ) {
    entry = &this->fields[ i ];

    ::memcpy( &buf[ off ], entry->fname, entry->fname_size );
    off += entry->fname_size;
    Unaligned::endianPutInt( entry->fsize, &buf[ off ] );
    off += sizeof( entry->fsize );
    fid[ fidCount++ ] = entry->fid;
    Unaligned::endianPutInt( entry->fid, &buf[ off ] );
    off += sizeof( entry->fid );
    Unaligned::endianPutInt( entry->ftype, &buf[ off ] );
    off += sizeof( entry->ftype );
    buf[ off++ ] = entry->partial;

    if ( off > 1024 - ( 128 + 16 ) ) {
      this->typeHash = Hash64::newhash( buf, off, this->typeHash );
      off = 0;
    }
  }
  if ( off > 0 )
    this->typeHash = Hash64::newhash( buf, off, this->typeHash );

  this->fidHash = Hash64::newhash( (const Rai_u8 *) fid,
                                   sizeof( fid[ 0 ] ) * fidCount, fidCount );
}


unsigned int
RaiMsg_form::calcIndexSize( Rai_u16 *fid,  unsigned int fidCount )
{
  Rai_u8 fidBits,
         fieldBits;

  return RaiMsg_form::getIndexSize( fid, fidCount, fidBits, fieldBits );
}


unsigned int
RaiMsg_form::getIndexSize( Rai_u16 *fid,  unsigned int fidCount,
                           Rai_u8 &fidBits,  Rai_u8 &fieldBits )
{
  unsigned int i,
               j,
               val;

  /* determine how many bits of the fid are needed to make
     it unique in the form */
  fidBits = 3;
loop:;
  if ( ( 1U << ++fidBits ) < MAX_FID ) {
    for ( i = 0; i < fidCount; i++ ) {
      val = fid[ i ] & ( ( 1U << fidBits ) - 1 );
      for ( j = i + 1; j < fidCount; j++ ) {
        if ( val == ( fid[ j ] & ( ( 1U << fidBits ) - 1 ) ) )
          goto loop;
      }
    }
  }
  /* determine the number of bits for an index into form's
     fields */
  for ( fieldBits = 2; ( 1U << fieldBits ) < fidCount; fieldBits++ )
    ;
  return ( ( 1U << fidBits ) * fieldBits + 31 ) / 16;
}


unsigned int
RaiMsg_form::indexSize( void )
{
  return ( ( 1U << this->fidBits ) * this->fieldBits + 31 ) / 16;
}

struct RaiMsg_formref {
  RaiMsg_form * form;
  AtomicUInt    refs;
  Rai_u32       size;
};

struct RaiMsg_dtab : public LinearHashTable<RaiMsg_formref *, Rai_u64> {
  protected:
    virtual Rai_u64 value( RaiMsg_formref *formref ) {
      return formref->form->fidHash;
    }
    virtual unsigned int hash( Rai_u64 val ) {
      return (unsigned int) val ^ (unsigned int) ( val >> 32 );
    }
    virtual bool equals( Rai_u64 val,  RaiMsg_formref *formref ) {
      return val == formref->form->fidHash;
    }
  public:
    RaiMsg_formref * cfgForms;
    unsigned int     cfgFormCount,
                     anonFormCount,
                     anonFormSize;
    SYS_OPS( RaiMsg_dtab );
    RaiMsg_dtab( unsigned int init ) :
      LinearHashTable<RaiMsg_formref *, Rai_u64>( init ),
      cfgForms( 0 ), cfgFormCount( 0 ), anonFormCount( 0 ),
      anonFormSize( 0 ) {}

    virtual ~RaiMsg_dtab() {
      unsigned int i;

      if ( ! this->isEmpty() ) {
        for ( i = 0; i < this->tabSize; i++ ) {
          if ( (this->tab[ i ].hashVal & SLOT_USED) != 0 ) {
            if ( this->tab[ i ].elem->form->entry->fname ==
                 ANONYMOUS_FORM_STRING ) {
              FREE( this->tab[ i ].elem );
            }
          }
        }
      }
      if ( this->cfgForms != NULL )
        FREE( this->cfgForms );
    }
};


struct RaiMsg_ttab : public LinearHashTable<RaiMsg_formref *, Rai_u64> {
  protected:
    virtual Rai_u64 value( RaiMsg_formref *formref ) {
      return formref->form->typeHash;
    }
    virtual unsigned int hash( Rai_u64 val ) {
      return (unsigned int) val ^ (unsigned int) ( val >> 32 );
    }
    virtual bool equals( Rai_u64 val,  RaiMsg_formref *formref ) {
      return val == formref->form->typeHash;
    }
  public:
    SYS_OPS( RaiMsg_ttab );
    RaiMsg_ttab( unsigned int init ) :
      LinearHashTable<RaiMsg_formref *, Rai_u64>( init ) {}
};


void
RaiMsg_config::release( RaiMsg_config *dict )
{
  if ( dict != NULL ) {
    if ( dict->dynamicForm != NULL )
      delete dict->dynamicForm;
    /*if ( dict->typeForm != NULL )
      delete dict->typeForm;*/
    if ( dict->dynamicFormLock != NULL )
      delete dict->dynamicFormLock;
    FREE( dict );
  }
}


const RaiMsg_dict *
RaiMsg_config::getEntry( const char *fname,  unsigned int fname_size ) const
{
  unsigned int i;

  if ( fname_size == 0 )
    return NULL;
  i = Hash32::crc_c( (Rai_u8 *) fname, fname_size - 1 );

  for ( i &= ( MAX_FID - 1 ); this->hashIndex[ i ] != NULL;
        i = ( i + 1 ) & ( MAX_FID - 1 ) ) {
    if ( this->hashIndex[ i ]->fname_size == (Rai_u8) fname_size &&
         ::memcmp( this->hashIndex[ i ]->fname, fname, fname_size ) == 0 ) {
      return this->hashIndex[ i ];
    }
  }
  return NULL;
}


const RaiMsg_form *
RaiMsg_config::getForm( const char *fname,  unsigned int fname_size ) const
{
  RaiMsg_dict * entry;
  const char  * end;
  unsigned int  i;
  int           recType;

  if ( fname_size == 0 )
    return NULL;
  i = Hash32::crc_c( (Rai_u8 *) fname, fname_size - 1 );

  for ( i &= ( MAX_FID - 1 ); this->hashIndex[ i ] != NULL;
        i = ( i + 1 ) & ( MAX_FID - 1 ) ) {
    if ( this->hashIndex[ i ]->fname_size == (Rai_u8) fname_size &&
         ::memcmp( this->hashIndex[ i ]->fname, fname, fname_size ) == 0 ) {
      entry = this->hashIndex[ i ];
      for ( i = 0; i < this->formCount; i++ ) {
        if ( entry == this->form[ i ].entry )
          return &this->form[ i ];
      }
      return NULL;
    }
  }
  if ( ( fname[ 0 ] >= '0' && fname[ 0 ] <= '9' ) || 
       ( fname[ 0 ] == '-' && fname[ 1 ] >= '0' && fname[ 1 ] <= '9' ) ) {
    bool neg = ( fname[ 0 ] == '-' );
    end = &fname[ fname_size - 1 ];
    if ( neg )
      fname++;
    recType = (int) (Rai_u8) ( *fname++ - '0' );
    while ( fname < end && fname[ 0 ] >= '0' && fname[ 0 ] <= '9' )
      recType = recType * 10 + (int) (Rai_u8) ( *fname++ - '0' );
    if ( fname == end )
      return this->getForm( (Rai_u16) ( neg ? -recType : recType ) );
  }
  return NULL;
}


void
RaiMsg_config::initAnonForms( void )
{
  unsigned int j;

  this->dynamicForm = NEW RaiMsg_dtab( this->formCount + 16 );
  /*this->typeForm = NEW RaiMsg_ttab( this->formCount + 16 );*/
  MALLOC( sizeof( this->dynamicForm->cfgForms[ 0 ] ) * this->formCount,
          &this->dynamicForm->cfgForms );
  this->dynamicForm->cfgFormCount = this->formCount;
  for ( j = 0; j < this->formCount; j++ ) {
    this->dynamicForm->cfgForms[ j ].form = &this->form[ j ];
    this->dynamicForm->cfgForms[ j ].refs.init( 0 );
    this->dynamicForm->cfgForms[ j ].size = 0;
  }
  for ( j = 0; j < this->formCount; j++ )
    this->dynamicForm->insert( &this->dynamicForm->cfgForms[ j ] );
  /*for ( j = 0; j < this->formCount; j++ )
    this->typeForm->insert( &this->dynamicForm->cfgForms[ j ] );*/
}


#define DBG_REF( w, f )
#if 0
static void
DBG_REF( const char *where,  const RaiMsg_formref *formRef )
{
  logMinor( LMINOR, "%s r=%u h=%qx", where, formRef->refs,
            formRef->form->typeHash );
}
#endif
#if 0
const RaiMsg_form *
RaiMsg_config::getHashedForm( Rai_u64 typeHash )
{
  RaiMsg_formref * formRef;

  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->lock();
  try {
    if ( this->typeForm == NULL )
      this->initAnonForms();
    if ( (formRef = this->typeForm->findElem( typeHash )) != NULL ) {
      formRef->refs.add( 1 );
      DBG_REF( "thash ref", formRef );
    }
    if ( this->dynamicFormLock != NULL )
      this->dynamicFormLock->unlock();

    return formRef != NULL ? formRef->form : NULL;
  } catch ( ... ) {
    if ( this->dynamicFormLock != NULL )
      this->dynamicFormLock->unlock();
    throw;
  }
}
#endif

const RaiMsg_form *
RaiMsg_config::getAnonForm( Rai_u16 *fid,  unsigned int fidCount,
                            bool mustExist )
{
  RaiMsg_formref    * formRef;
  RaiMsg_form       * newForm;
  const RaiMsg_dict * classEntry;
  Rai_u64             hash;
  unsigned int        i, j, h, sz;

  hash = Hash64::newhash( (const Rai_u8 *) fid, sizeof( fid[ 0 ] ) * fidCount,
                          fidCount );
  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->lock();

  try {
    if ( this->dynamicForm == NULL )
      this->initAnonForms();
    formRef = this->dynamicForm->findElem( hash );
    if ( formRef != NULL ) {
      formRef->refs.add( 1 );
      DBG_REF( "anon ref", formRef );
      if ( this->dynamicFormLock != NULL )
        this->dynamicFormLock->unlock();
      return formRef->form;
    }
    if ( mustExist ) {
      if ( this->dynamicFormLock != NULL )
        this->dynamicFormLock->unlock();
      return NULL;
    }
  } catch ( ... ) {
    if ( this->dynamicFormLock != NULL )
      this->dynamicFormLock->unlock();
    throw;
  }
  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->unlock();

  /* if newForm doesn't already exist */
  /* check all fids are valid */
  for ( j = 0; j < fidCount; j++ ) {
    if ( (classEntry = this->getEntry( fid[ j ] )) == NULL )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_DICT_FID );
  }
  /* allocate space for form */
  j  = RaiMsg_form::calcIndexSize( fid, fidCount );
  sz = sizeof( RaiMsg_formref ) + sizeof( RaiMsg_form ) +
       sizeof( RaiMsg_dict ) * ( fidCount + 1 ) +
       sizeof( Rai_u16 ) * j;
  MALLOC( sz, &formRef );

  newForm = (RaiMsg_form *) (void *) &formRef[ 1 ];
  formRef->form = newForm;
  formRef->refs.init( 1 );
  formRef->size = sz;
  newForm->init( this );
  newForm->entry      = (RaiMsg_dict *) (void *) &newForm[ 1 ];
  newForm->entry->init( ANONYMOUS_FORM_STRING,
                        sizeof( ANONYMOUS_FORM_STRING ) );
  newForm->entry->fid = 0;
  newForm->fields     = &newForm->entry[ 1 ];
  newForm->fidIndex   = (Rai_u16 *) (void *) &newForm->fields[ fidCount ];

  newForm->initFields( fid, fidCount, true );
  newForm->computeHashes();
  if ( newForm->fidHash != hash ) {
    logError( LERROR, NULL, "fidHash 0x%qx != hash 0x%qx",
              newForm->fidHash, hash );
  }

  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->lock();

  try {
    RaiMsg_formref *formRef2;
    formRef2 = this->dynamicForm->findInsert( hash, i, h );
    if ( formRef2 != NULL ) {
      if ( formRef2->refs.add( 0 ) == 0 ) { /* its about to be deleted */
        this->dynamicForm->addInsert( formRef, i, h );
      }
      else { /* use existing form instead */
        RaiMsg_formref *tmp = formRef;
        formRef = formRef2;
        formRef2 = tmp;
        formRef->refs.add( 1 );
      }
    }
    else { /* its new */
      this->dynamicForm->anonFormCount++;
      this->dynamicForm->anonFormSize += formRef->size;
      this->dynamicForm->addInsert( formRef, i, h );
    }
    if ( this->dynamicFormLock != NULL )
      this->dynamicFormLock->unlock();
    if ( formRef2 != NULL )
      FREE( formRef2 );

    return formRef->form;
  } catch ( ... ) {
    if ( this->dynamicFormLock != NULL )
      this->dynamicFormLock->unlock();
    throw;
  }
}


void
RaiMsg_config::derefForm( const RaiMsg_form *form )
{
  RaiMsg_formref * formRef;

  if ( form->entry->fname != ANONYMOUS_FORM_STRING )
    return;

  formRef = &((RaiMsg_formref *) (void *) form)[ -1 ];
  if ( formRef->form != form ) {
    logError( LERROR, NULL, "Formref incorrect pointer (deref)" );
    return;
  }
  /* get the key before derefing, since formRef may be deleted before lock()
   * and after deref */
  Rai_u64 hash = formRef->form->fidHash;

  if ( formRef->refs.add( -1 ) != 1 )
    return;

  RaiMsg_formref * formRef2 = NULL;
  unsigned int     i        = 0;

  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->lock();
  /* if its still in the hash table, it's not deleted yet */
  if ( this->dynamicForm->findSlot( hash, i ) == formRef ) {
    /* if no refs, delete it */
    if ( formRef->refs.add( 0 ) == 0 ) {
      this->dynamicForm->removeSlot( i );
      this->dynamicForm->anonFormCount--;
      this->dynamicForm->anonFormSize -= formRef->size;
      formRef2 = formRef;
    }
  }
  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->unlock();
  if ( formRef2 != NULL )
    FREE( formRef2 );
}


void
RaiMsg_config::refForm( const RaiMsg_form *form )
{
  RaiMsg_formref * formRef;

  if ( form->entry->fname != ANONYMOUS_FORM_STRING )
    return;

  formRef = &((RaiMsg_formref *) (void *) form)[ -1 ];
  if ( formRef->form != form ) {
    logError( LERROR, NULL, "Formref incorrect pointer (ref)" );
    return;
  }
  formRef->refs.add( 1 );
}


void
RaiMsg_config::describeMemory( RaiMsg &msg )
{
  Error e2 = NULL;
  msg.Append( "sass-dict-size", this->dictSize );
  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->lock();
  if ( this->dynamicForm != NULL ) {
    try {
      msg.Append( "sass-anon-formclass-count",
                  this->dynamicForm->anonFormCount );
      msg.Append( "sass-anon-formclass-size",
                  this->dynamicForm->anonFormSize );
    } catch ( Error e ) {
      e2 = e;
    }
  }
  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->unlock();
  if ( e2 != NULL )
    throw e2;
}


const RaiMsg_form *
RaiMsg_config::describeAnonForm( RaiMsg &msg,  const char *name )

{
  const RaiMsg_form * form = NULL;
  RaiMsg_formref    * ref;
  Error               e2 = NULL;
  ullong              fidHash;
  unsigned int        i, j, k, count = 0;
  char                buf[ 24 ];

  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->lock();
  if ( this->dynamicForm != NULL ) {
    try {
      if ( name == NULL ) {
        if ( (ref = this->dynamicForm->first( i )) != NULL ) {
          do {
            buf[ 0 ] = '0';
            buf[ 1 ] = 'x';
            k = 2;
            for ( j = 64 - 4; ; j -= 4 ) {
              byte x = ( ( ( 0xfULL << j ) & ref->form->fidHash ) >> j );
              buf[ k++ ] = ( x < 10 ? x + '0' : x - 10 + 'a' );
              if ( j == 0 )
                break;
            }
            buf[ k ] = '\0';
            if ( ref->form->entry->fname != ANONYMOUS_FORM_STRING )
              msg.Append( buf, ref->form->entry->fname );
            else
              msg.Append( buf, ref->refs.val );
            count += ref->refs.val;
          } while ( (ref = this->dynamicForm->next( i )) != NULL );
        }
        msg.Append( "total-refs", count );
      }
      else {
        fidHash = 0;
        k = 2;
        if ( name[ 0 ] == '0' && name[ 1 ] == 'x' ) {
          for ( j = 64 - 4; ; j -= 4 ) {
            byte x = (byte) name[ k++ ];
            if ( x >= '0' && x <= '9' )
              x -= '0';
            else if ( x >= 'a' && x <= 'f' )
              x = ( x - 'a' ) + 10;
            fidHash |= (ullong) x << j;
            if ( j == 0 )
              break;
          }
        }
        if ( (ref = this->dynamicForm->findElem( fidHash )) != NULL ) {
          ref->refs.add( 1 );
          form = ref->form;
        }
      }
    } catch ( Error e ) {
      e2 = e;
    }
  }
  if ( this->dynamicFormLock != NULL )
    this->dynamicFormLock->unlock();
  if ( count == 0 && form != NULL )
    msg.Append( "no-entries", true );
  if ( e2 != NULL )
    throw e2;
  return form;
}


bool
RaiMsg_config::getFormRefCount( const RaiMsg_form *form,  ullong &fidHash,
                                unsigned int &count )
{
  RaiMsg_formref * formRef;

  if ( form->entry->fname != ANONYMOUS_FORM_STRING )
    return false;

  formRef = &((RaiMsg_formref *) (void *) form)[ -1 ];
  if ( formRef->form != form ) {
    logError( LERROR, NULL, "Formref incorrect pointer (ref)" );
    return false;
  }
  fidHash = formRef->form->fidHash;
  count = formRef->refs.add( 0 );
  return true;
}


Rai_u8
RaiMsg_dict::convertTssPrecision( Rai_f64 f64 )
{
  if ( f64 < 0.0 )
    f64 = -f64;
  if ( ! isnan( f64 ) && ! isinf( f64 ) ) {
    double integral, decimal, p10, tmpf;
    const double fraction = modf( (double) f64, &integral );
    unsigned int places = 14;
    ullong decimal_ival;
    for ( ullong ival_places = (ullong) integral;
          ival_places >= 100 && places > 1; ival_places /= 10 ) {
      places--;
    }
    if ( places > 0 ) {
      static const double powtab[] = {
        1.0, 10.0, 100.0, 1000.0, 10000.0,
        100000.0, 1000000.0, 10000000.0,
        100000000.0, 1000000000.0, 10000000000.0,
        100000000000.0, 1000000000000.0, 10000000000000.0,
        100000000000000.0
        };
      if ( places < sizeof( powtab ) / sizeof( powtab[ 0 ] ) )
        p10 = powtab[ places ];
      else
        p10 = pow( (double) 10.0, (double) places );
      /* mult fraction + 1 * places wanted (.25 + 1.0) * 1000.0 = 1250 */
      tmpf = modf( ( fraction + 1.0 ) * p10, &decimal );
      /* round up, if fraction of decimal places >= 0.5 */
      if ( tmpf >= 0.5 )
        decimal += 1.0;
      else if ( decimal >= p10 * 2.0 )
        decimal -= 1.0;
      decimal_ival = (ullong) decimal;

      while ( decimal_ival > 1 && decimal_ival % 10 == 0 ) {
        decimal_ival /= 10;
        if ( --places == 0 )
          break;
      }
    }
    if ( places > 0 )
      return RAI_TSS_HINT_PRECISION_1 + places - 1;
  }
  return RAI_TSS_HINT_NONE;
}


void
RaiMsg_dict::convert( RaiField &field ) const
{
  static Rai_u8 sdateHint[ 2 ] = { 1U, 0 };
  static Rai_u8 stimeHint[ 2 ] = { 1U, 1U };
  RaiField_data tmp,
                val;
  switch ( this->ftype ) {
    case RAI_TSS_SDATE:
    case RAI_TSS_STIME:
      if ( field.hintType == RAIMSG_NODATA ) {
        field.hintType = RAIMSG_UINT;
        field.hintSize = 2;
        if ( this->ftype == RAI_TSS_SDATE )
          field.hintData = sdateHint;
        else
          field.hintData = stimeHint;
      }
    /* FALLTHRU */
    case RAI_TSS_STRING:
    case RAI_TSS_OPAQUE:
      switch ( field.type ) {
        case RAIMSG_STRING:
        case RAIMSG_OPAQUE:
        case RAIMSG_PARTIAL:
          break;

        default:
          RaiField::Convert( RAIMSG_STRING, sizeof( tmp.str ),
                             (RaiMsg_data) tmp.str,
                             field.type, field.size, field.AlignData( val ) );
          ::memcpy( &field.updateData, &tmp, sizeof( tmp ) );
          field.type = RAIMSG_STRING;
          field.size = ::strlen( field.updateData.str );
          field.data = (RaiMsg_data) field.updateData.str;
          break;
      }
      break;
        
    case RAI_TSS_DATE: /* struct { short year, mon, day } */
    case RAI_TSS_TIME: /* struct { short hour, min, sec } */
      break;

    case RAI_TSS_GROCERY: { /* struct { double numer; char denom } */
      RaiField::ConvertCtx fctx;
      RaiMsg_type stype = field.type;
      RaiMsg_size ssize = field.size;
      fctx.init( RAIMSG_REAL, this->fsize - 1, (RaiMsg_data) &tmp,
                 stype, ssize, field.AlignData( val ) );
      RaiField::Convert( fctx );

      field.type = RAIMSG_REAL;
      field.size = this->fsize - 1;
      field.data = (RaiMsg_data) &tmp;
      field.data = field.AlignData( field.updateData );
      if ( field.hintType == RAIMSG_NODATA ) {
        field.hintType   = RAIMSG_UINT;
        field.hintSize   = 1;
        field.hintData   = &field.updateHintData;
        field.updateHintData.u8 = fctx.destHint;
        /* conversion from real to price, discover the precision */
        if ( fctx.destHint == RAI_TSS_HINT_NONE && stype == RAIMSG_REAL ) {
          field.updateHintData.u8 = RaiMsg_dict::convertTssPrecision( 
                                 ( ssize == 8 ? tmp.f64 : (Rai_f64) tmp.f32 ) );
        }
      }
      break;
    }
    case RAI_TSS_PRICE:
    case RAI_TSS_FLOAT:
    case RAI_TSS_DOUBLE:
    case RAI_TSS_DOUBLE_INT:
    case RAI_TSS_INTEGER:
    case RAI_TSS_SHORT_INT:
    case RAI_TSS_LONG:
    case RAI_TSS_BOOLEAN:
    case RAI_TSS_BYTE:
    case RAI_TSS_U_SHORT:
    case RAI_TSS_U_INT:
    case RAI_TSS_U_LONG:
      RaiField::Convert( tssTypeToRaiMsgType[ this->ftype ],
                         this->fsize, &tmp, field.type,
                         field.size, field.AlignData( val ) );
      field.type = tssTypeToRaiMsgType[ this->ftype ];
      field.size = this->fsize;
      field.data = (RaiMsg_data) &tmp;
      field.data = field.AlignData( field.updateData );
      break;

    case RAI_TSS_NULL:
    case RAI_TSS_RESERVED:
    default:
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_TYPE );
  }
}


RaiMsg_type
RaiMsg_dict::getFieldType( void ) const
{
  return tssTypeToRaiMsgType[ this->ftype ];
}


void
RaiMsg_dict::pack( RaiField &field,  Rai_u8 *to_ptr ) const

{
  Rai_u16       u16,
                u16dt[ 3 ],
                partialOff,
                partialLen;
  Rai_u32       u32;
  RaiField_data tmpData,
                val;

  to_ptr = &to_ptr[ 2 ];

  switch ( this->ftype ) {
    case RAI_TSS_SDATE:
      /*if ( field.size >= 12 )
        ::memcpy( to_ptr, field.data, 12 );
      else {
        ::memcpy( to_ptr, field.data, field.size );
        ::memset( &to_ptr[ field.size ], 0, 12 - field.size );
      }
      break;*/

    case RAI_TSS_STIME:
      /*if ( field.size >= 10 )
        ::memcpy( to_ptr, field.data, 10 );
      else {
        ::memcpy( to_ptr, field.data, field.size );
        ::memset( &to_ptr[ field.size ], 0, 10 - field.size );
      }
      break;*/

    case RAI_TSS_STRING:
    case RAI_TSS_OPAQUE:
      if ( this->partial ) {
        char        tmpBuf[ 128 ];
        RaiMsg_data data;

        if ( field.type == RAIMSG_PARTIAL ) {
          partialOff = field.hintSize;
          partialLen = field.size;
          data       = field.data;
        }
        else if ( field.type == RAIMSG_STRING ||
                  field.type == RAIMSG_OPAQUE ) {
          partialOff = 0;
          partialLen = field.size;
          data       = field.data;
        }
        else {
          field.Get( tmpBuf, sizeof( tmpBuf ) );
          partialOff = 0;
          partialLen = ::strlen( tmpBuf );
          data       = tmpBuf;
        }

        if ( partialLen > this->fsize )
          partialLen = this->fsize;

        Unaligned::endianPutInt( partialOff, &((Rai_u8 *) to_ptr)[ 0 ] );
        Unaligned::endianPutInt( partialLen, &((Rai_u8 *) to_ptr)[ 2 ] );
        ::memcpy( &to_ptr[ 4 ], data, partialLen );

        if ( ( partialLen & 1U ) != 0 )
          to_ptr[ 4 + partialLen ] = 0;
      }
      else {
        if ( field.size >= this->fsize )
          ::memcpy( to_ptr, field.data, this->fsize );
        else {
          ::memcpy( to_ptr, field.data, field.size );
          ::memset( &to_ptr[ field.size ], 0, this->fsize - field.size );
        }
        if ( ( this->fsize & 1U ) != 0 )
          to_ptr[ this->fsize ] = 0;
      }
      break;
        
    case RAI_TSS_DATE: /* struct { short year, mon, day } */
    case RAI_TSS_TIME: /* struct { short hour, min, sec } */
      if ( field.size == 6 && field.type == RAIMSG_OPAQUE ) {
        ::memcpy( u16dt, field.data, 6 );
        Aligned::endianSwap( u16dt[ 0 ] );
        Aligned::endianSwap( u16dt[ 1 ] );
        Aligned::endianSwap( u16dt[ 2 ] );
        ::memcpy( to_ptr, u16dt, 6 );
        break;
      }
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_DATETIME );

    case RAI_TSS_GROCERY: /* struct { double numer; char denom } */
      if ( field.size == (RaiMsg_size) this->fsize - 1 &&
           field.type == RAIMSG_REAL ) {
        if ( field.size == 8 )
          ::memcpy( &to_ptr[ 0 ], field.data, 8 );
        else if ( field.size == 4 )
          ::memcpy( &to_ptr[ 0 ], field.data, 4 );
        else {
          throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_GROCERY );
        }

        if ( field.hintType != RAIMSG_NODATA ) {
          if ( field.hintType == RAIMSG_UINT ||
               field.hintType == RAIMSG_INT ) {
            if ( field.hintSize == 1 ) {
              to_ptr[ field.size ] = *(Rai_u8 *) field.hintData;
            }
            else if ( field.hintSize == 2 ) {
              Unaligned::endianGetInt( (Rai_u8 *) field.hintData, u16 );
              to_ptr[ field.size ] = (Rai_u8) u16;
            }
            else if ( field.hintSize == 4 ) {
              Unaligned::endianGetInt( (Rai_u8 *) field.hintData, u32 );
              to_ptr[ field.size ] = (Rai_u8) u32;
            }
            else {
              throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_GROCERY2 );
            }
            to_ptr[ field.size + 1 ] = 0;
          }
          else {
            RaiField::Convert( RAIMSG_UINT, 1, &val,
                               field.hintType, field.hintSize,
                               field.AlignHintData( tmpData ) );
            to_ptr[ field.size ]     = val.u8;
            to_ptr[ field.size + 1 ] = 0;
          }
        }
        else {
          to_ptr[ field.size ]     = 0;
          to_ptr[ field.size + 1 ] = 0;
        }
      }
      else { /* struct { double numer; char denom } */
        RaiField::ConvertCtx fctx;
        fctx.init( RAIMSG_REAL, this->fsize - 1, &val, field.type,
                   field.size, field.AlignData( tmpData ) );
        RaiField::Convert( fctx );

        if ( this->fsize == 5 ) {
          Unaligned::endianPutInt( val.u32, (Rai_u8 *) to_ptr );
        }
        else if ( this->fsize == 9 ) {
          Unaligned::endianPutInt( val.u64, (Rai_u8 *) to_ptr );
        }
        else {
          throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_GROCERY );
        }
        if ( field.hintType != RAIMSG_NODATA ) {
          if ( ( field.hintType == RAIMSG_UINT ||
                 field.hintType == RAIMSG_INT ) && field.hintSize == 1 ) {
            to_ptr[ this->fsize - 1 ] = *(Rai_u8 *) field.hintData;
            to_ptr[ this->fsize ]     = 0;
          }
          else {
            RaiField::Convert( RAIMSG_UINT, 1, &val,
                               field.hintType, field.hintSize,
                               field.AlignHintData( tmpData ) );
            to_ptr[ this->fsize - 1 ] = val.u8;
            to_ptr[ this->fsize ]     = 0;
          }
        }
        else {
          to_ptr[ this->fsize - 1 ] = fctx.destHint;
          to_ptr[ this->fsize ]     = 0;
        }
      }
      /*RaiField::Convert( RAIMSG_REAL, this->fsize - 1, &val, field.type,
                         field.size, field.AlignData( tmpData ) );*/

      break;

    case RAI_TSS_PRICE: /* tss_price flost */
    case RAI_TSS_FLOAT:
    case RAI_TSS_DOUBLE:
    case RAI_TSS_DOUBLE_INT:
      if ( field.size == this->fsize && field.type == RAIMSG_REAL ) {
        if ( field.size == 4 ) {
          ::memcpy( &to_ptr[ 0 ], field.data, 4 );
          break;
        }
        if ( field.size == 8 ) {
          ::memcpy( &to_ptr[ 0 ], field.data, 8 );
          break;
        }
        throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_PRICE );
      }
      RaiField::Convert( RAIMSG_REAL, this->fsize, &val, field.type,
                         field.size, field.AlignData( tmpData ) );

      if ( this->fsize == 4 )
        Unaligned::endianPutInt( val.u32, (Rai_u8 *) to_ptr );
      else
        Unaligned::endianPutInt( val.u64, (Rai_u8 *) to_ptr );
      break;

    case RAI_TSS_INTEGER:
    case RAI_TSS_SHORT_INT:
    case RAI_TSS_LONG:
    case RAI_TSS_BOOLEAN:
    case RAI_TSS_BYTE:
    case RAI_TSS_U_SHORT:
    case RAI_TSS_U_INT:
    case RAI_TSS_U_LONG:
      if ( field.size == this->fsize &&
           ( field.type == RAIMSG_INT || field.type == RAIMSG_UINT ) ) {
        if ( field.size == 4 ) {
          ::memcpy( &to_ptr[ 0 ], field.data, 4 );
          break;
        }
        if ( field.size == 2 ) {
          ::memcpy( &to_ptr[ 0 ], field.data, 2 );
          if ( DataDictionary->recType != NULL &&
               this->fid == DataDictionary->recType->fid ) {
            /* take out PRIMITIVE and add FIXED flag to REC_TYPE */
            if ( to_ptr[ 0 ] != 0 || to_ptr[ 1 ] != 0 )
              to_ptr[ 0 ] = to_ptr[ 0 ] & (Rai_u8) ~( FID_CTRL >> 8 );
#if 0
                              (Rai_u8) ~( FID_PRIMITIVE_FLAG >> 8 ) ) |
                            (Rai_u8) ( FID_FIXED_FLAG >> 8 );
#endif
          }
          break;
        }
        if ( field.size == 1 ) {
          to_ptr[ 0 ] = *(Rai_u8 *) field.data;
          break;
        }
        if ( field.size == 8 ) {
          ::memcpy( &to_ptr[ 0 ], field.data, 8 );
          break;
        }
        throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_INTEGER );
      }

      RaiField::Convert( tssTypeToRaiMsgType[ this->ftype ],
                         this->fsize, &val, field.type,
                         field.size, field.AlignData( tmpData ) );

      if ( this->fsize == 4 )
        Unaligned::endianPutInt( val.u32, (Rai_u8 *) to_ptr );
      else if ( this->fsize == 2 ) {
        Unaligned::endianPutInt( val.u16, (Rai_u8 *) to_ptr );
        if ( DataDictionary->recType != NULL &&
             this->fid == DataDictionary->recType->fid ) {
          if ( val.u16 != 0 )
            to_ptr[ 0 ] = to_ptr[ 0 ] & (Rai_u8) ~( FID_CTRL >> 8 );
#if 0
                            (Rai_u8) ~( FID_PRIMITIVE_FLAG >> 8 ) ) |
                          (Rai_u8) ( FID_FIXED_FLAG >> 8 );
#endif
        }
      }
      else if ( this->fsize == 1 )
        *(Rai_u8 *) to_ptr = val.u8;
      else
        Unaligned::endianPutInt( val.u64, (Rai_u8 *) to_ptr );
      break;

    case RAI_TSS_NULL:
    case RAI_TSS_RESERVED:
    default:
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_TYPE );
  }
}


void
RaiMsg_dict::packPartial( RaiField &field,  Rai_u8 *to_ptr ) const

{
  RaiMsg_size partialOff,
              partialLen;
  RaiMsg_data data;
  char        tmpBuf[ 128 ];

  if ( this->ftype == RAI_TSS_STRING || this->ftype == RAI_TSS_OPAQUE ) {
    if ( this->partial ) {
      if ( field.type == RAIMSG_PARTIAL ) {
        partialOff = field.hintSize;
        partialLen = field.size;
        data       = field.data;
      }
      else if ( field.type == RAIMSG_STRING || field.type == RAIMSG_OPAQUE ) {
        partialOff = 0;
        partialLen = field.size;
        data       = field.data;
      }
      else {
        field.Get( tmpBuf, sizeof( tmpBuf ) );
        partialOff = 0;
        partialLen = ::strlen( tmpBuf );
        data       = tmpBuf;
      }

      if ( partialOff < (RaiMsg_size) this->fsize ) {
        if ( partialOff + partialLen > (RaiMsg_size) this->fsize )
          partialLen = (RaiMsg_size) this->fsize - partialOff;
        ::memcpy( &to_ptr[ 6 + partialOff ], data, partialLen );
      }

      return;
    }
  }

  throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_PARTIAL );
}


Rai_u8 *
RaiMsg_dict::unpack( RaiField &field,  Rai_u8 *from_ptr,
                     unsigned int length ) const
{
  static Rai_u8 sdateHint[ 2 ] = { 1U, 0 };
  static Rai_u8 stimeHint[ 2 ] = { 1U, 1U };
  Rai_u16      partialOff,
               partialLen;
  unsigned int fsize;

  field.name    = this->fname;
  field.nameLen = this->fname_size;
  from_ptr       = &from_ptr[ 2 ];

  if ( ! this->partial ) {
    fsize = this->packSize();

    switch ( this->ftype ) {

      case RAI_TSS_SDATE:
      case RAI_TSS_STIME:
        field.type     = RAIMSG_STRING;
        field.size     = this->fsize;
        field.hintType = RAIMSG_UINT;
        field.hintSize = 2;
        field.data     = from_ptr;
        if ( this->ftype == RAI_TSS_SDATE )
          field.hintData = sdateHint;
        else
          field.hintData = stimeHint;
        
        if ( length < fsize )
          throw RaiMsgErr::getErr( RaiMsgErr::BIG_TSS_STIME );
        break;

      case RAI_TSS_STRING:
      case RAI_TSS_OPAQUE:
        field.type     = ( this->ftype == RAI_TSS_STRING ) ? RAIMSG_STRING :
                                                              RAIMSG_OPAQUE;
        field.size     = this->fsize;
        field.hintType = RAIMSG_NODATA;
        field.data     = from_ptr;

        if ( length < fsize )
          throw RaiMsgErr::getErr( RaiMsgErr::BIG_TSS_STRING );
        break;

      case RAI_TSS_TIME: /* struct { short year, mon, day } */
      case RAI_TSS_DATE: /* struct { short hour, min, sec } */
        field.type     = RAIMSG_OPAQUE;
        field.size     = this->fsize;
        field.hintType = RAIMSG_NODATA;
        field.data     = from_ptr;

        if ( length < fsize )
          throw RaiMsgErr::getErr( RaiMsgErr::BIG_TSS_DATETIME );
        break;

      case RAI_TSS_GROCERY: /* struct { double numer; char denom } */
        field.type     = RAIMSG_REAL;
        field.size     = this->fsize - 1;
        field.data     = from_ptr;
        field.hintType = RAIMSG_UINT;
        field.hintSize = 1;
        field.hintData = &from_ptr[ field.size ];

        if ( length < fsize )
          throw RaiMsgErr::getErr( RaiMsgErr::BIG_TSS_GROCERY );
        break;

      case RAI_TSS_DOUBLE_INT:
      case RAI_TSS_PRICE:
      case RAI_TSS_FLOAT:
      case RAI_TSS_DOUBLE:
        field.type              = RAIMSG_REAL;
        field.size              = this->fsize;
        field.data              = from_ptr;
        field.hintType          = RAIMSG_UINT;
        field.hintSize          = 1;
        field.updateHintData.u8 = 0;
        field.hintData          = &field.updateHintData;

        if ( length < fsize )
          throw RaiMsgErr::getErr( RaiMsgErr::BIG_TSS_PRICE );
        break;

      case RAI_TSS_LONG:
      case RAI_TSS_INTEGER:
      case RAI_TSS_SHORT_INT:
        field.type     = RAIMSG_INT;
        field.size     = this->fsize;
        field.hintType = RAIMSG_NODATA;
        field.data     = from_ptr;

        if ( length < fsize )
          throw RaiMsgErr::getErr( RaiMsgErr::BIG_TSS_INTEGER );
        break;

      case RAI_TSS_BOOLEAN:
      case RAI_TSS_BYTE:
      case RAI_TSS_U_SHORT:
      case RAI_TSS_U_INT:
      case RAI_TSS_U_LONG:
        field.type     = RAIMSG_UINT;
        field.size     = this->fsize;
        field.hintType = RAIMSG_NODATA;
        field.data     = from_ptr;

        if ( length < fsize )
          throw RaiMsgErr::getErr( RaiMsgErr::BIG_TSS_INTEGER );
        break;

      case RAI_TSS_NULL:
      case RAI_TSS_RESERVED:
      default:
        throw RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_TYPE );
    }
  }
  else {
    field.type = RAIMSG_PARTIAL;

    if ( length < 6 )
      throw RaiMsgErr::getErr( RaiMsgErr::BIG_TSS_PARTIAL );
    Unaligned::endianGetInt( (Rai_u8 *) &from_ptr[ 0 ], partialOff );
    Unaligned::endianGetInt( (Rai_u8 *) &from_ptr[ 2 ], partialLen );
    field.size     = partialLen;
    field.hintSize = partialOff;
    field.hintType = RAIMSG_NODATA;
    field.data     = &from_ptr[ 4 ];

    fsize = this->partialPackSize( partialLen );
    if ( length < fsize )
      throw RaiMsgErr::getErr( RaiMsgErr::BIG_TSS_PARTIAL );
  }

  return &from_ptr[ fsize - 2 ];
}


void
RaiMsg_config::packDataDictionary( RaiMsg &msg,  bool addForms ) const

{
  RaiMsg_dict * ptr;
  RaiField      field;
  Rai_u32       i,
                j,
                u32,
                len,
                count;
  Rai_u16       u16,
                fid;

  for ( fid = MAX_FID; fid > 0; ) {
    if ( this->fidIndex[ --fid ] != 0xffffU ) {
      if ( this->entry[ this->fidIndex[ fid ] ].ftype != RAI_TSS_NULL )
        break;
    }
  }
        
  len = 0;
  for ( i = 0; i <= fid; i++ ) {
    if ( this->fidIndex[ i ] != 0xffffU &&
         this->entry[ this->fidIndex[ i ] ].ftype != RAI_TSS_NULL ) {
      len += (Rai_u32) this->entry[ this->fidIndex[ i ] ].fname_size;
    }
  }

  msg.Append( "MAX_FID", (Rai_u16) fid /*+ 1*/ );
  msg.Append( "NAME_BYTES", len );
  msg.Append( "FIDS", (RaiMsg *) NULL );
  msg.Activate( "." );

  for ( i = 0; i <= fid; i++ ) {
    if ( this->fidIndex[ i ] != 0xffffU &&
         this->entry[ this->fidIndex[ i ] ].ftype != RAI_TSS_NULL ) {
      u16 = (Rai_u16) i;
      ptr = &this->entry[ this->fidIndex[ i ] ];
      u32 = ( (Rai_u32) ptr->partial << 24 ) |
            ( (Rai_u32) ptr->ftype << 16 ) |
            ( (Rai_u32) ptr->fsize );
      msg.Append( ptr->fname, RAIMSG_UINT, sizeof( u16 ), (RaiMsg_data) &u16,
                  RAIMSG_UINT, sizeof( u32 ), (RaiMsg_data) &u32 );
    }
  }

  msg.Activate( NULL );

  if ( addForms ) {
    msg.Append( "FORMS", (RaiMsg *) NULL );
    msg.Activate( "." );
    for ( i = 0; i < this->formCount; i++ ) {
      ptr   = this->form[ i ].entry;
      u16   = ptr->fid;
      count = this->form[ i ].fieldCount;
      msg.Append( ptr->fname, RAIMSG_UINT, sizeof( u16 ), (RaiMsg_data) &u16,
                  RAIMSG_UINT, sizeof( count ), (RaiMsg_data) &count );
      for ( j = 0; j < count; j++ ) {
        ptr = &this->form[ i ].fields[ j ];
        u16 = ptr->fid;
        u32 = ( (Rai_u32) ptr->partial << 24 ) |
              ( (Rai_u32) ptr->ftype << 16 ) |
              ( (Rai_u32) ptr->fsize );
        msg.Append( ptr->fname, RAIMSG_UINT, sizeof( u16 ), (RaiMsg_data) &u16,
                    RAIMSG_UINT, sizeof( u32 ), (RaiMsg_data) &u32 );
      }
    }
    msg.Activate( NULL );
  }
}


static Error
tssCheckEntry( Rai_u32 fnameLen,  Rai_u32 dataType,  Rai_u32 dataSize,
               bool isPartial )
{
  if ( fnameLen == 0 || fnameLen > 255 ) {
    return RaiMsgErr::getErr( RaiMsgErr::NULL_DICT_FNAME );
  }
  if ( dataType == (Rai_u32) RAI_TSS_NULL ||
       dataType == (Rai_u32) RAI_TSS_RESERVED ) { /* XXX */
    return RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_TYPE );
  }
  switch ( tssDataTypeIsValid( dataType, dataSize, isPartial ) ) {
    case BAD_TSS_TYPE:
      return RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_TYPE );
    case BAD_TSS_SIZE:
      return RaiMsgErr::getErr( RaiMsgErr::BAD_TSS_SIZE );
    case RAI_TSS_TYPE_OK:
      break;
  }
  return NULL;
}


static bool
tssFixEntry2( Rai_u32 &fnameLen,  Rai_u32 &dataType,  Rai_u32 &dataSize,
              bool &isPartial )
{
  Rai_u32 sz;

  if ( fnameLen == 0 || fnameLen > 255 )
    return false;
  if ( dataType == (Rai_u32) RAI_TSS_NULL ||
       dataType == (Rai_u32) RAI_TSS_RESERVED )
    return false;
  switch ( tssDataTypeIsValid( dataType, dataSize, isPartial ) ) {
    case BAD_TSS_TYPE:
    case BAD_TSS_SIZE:
      if ( tssDataTypeIsValid( dataType, dataSize,
                               ! isPartial ) == RAI_TSS_TYPE_OK ) {
        isPartial = ! isPartial;
        return true;
      }

      sz = dataSize;
      switch ( dataType ) {
        case RAI_TSS_DATE:
        case RAI_TSS_TIME:
          sz = 6;
          break;
        case RAI_TSS_GROCERY:
          sz = 9;
          break;
        case RAI_TSS_PRICE:
        case RAI_TSS_FLOAT:
        case RAI_TSS_INTEGER:
        case RAI_TSS_U_LONG:
        case RAI_TSS_U_INT:
        case RAI_TSS_LONG:
          sz = 4;
          break;
        case RAI_TSS_DOUBLE:
        case RAI_TSS_DOUBLE_INT:
          sz = 8;
          break;
        case RAI_TSS_SHORT_INT:
        case RAI_TSS_U_SHORT:
          sz = 2;
          break;
        case RAI_TSS_BOOLEAN:
        case RAI_TSS_BYTE:
          sz = 1;
          break;
      }
      if ( tssDataTypeIsValid( dataType, sz, isPartial ) == RAI_TSS_TYPE_OK ) {
        dataSize = sz;
        return true;
      }
      if ( tssDataTypeIsValid( dataType, sz, ! isPartial ) == RAI_TSS_TYPE_OK ){
        dataSize  = sz;
        isPartial = ! isPartial;
        return true;
      }
      return false;
    case RAI_TSS_TYPE_OK:
      return true;
  }
  return false;
}

static bool
tssFixEntry1( Rai_u32 fnameLen,  Rai_u32 dataType,  Rai_u32 dataSize,
              bool isPartial )
{
  return tssFixEntry2( fnameLen, dataType, dataSize, isPartial );
}


RaiMsg_config *
RaiMsg_config::unpackDataDictionary( RaiMsg &msg )
{
  Rai_u16         fieldFid[ MAX_FID ],
                * index;
  Rai_u32         maxFid,
                  entryCount,
                  formCount,
                  stringLen,
                  dataType,
                  dataSize,
                  len,
                  u32,
                  i,
                  count,
                  indexSize;
  Rai_u16         fid;
  Rai_u8          fname_size;
  bool            isPartial;
  char          * fname;
  RaiMsg_dict   * entry;
  RaiMsg_form   * form;
  RaiMsg_config * dict;
  RaiMsg        * msgPtr,
                  msg2;
  RaiField        field;
  Error           tssErr;

  /* check for rv7 _data_ opaque */
  if ( field.First( &msg ) && field.Type() == RAIMSG_OPAQUE &&
       field.isNamed( "_data_" ) ) {
    msg2.UnPack( (char *) field.Data(), field.Size() );
    msgPtr = &msg2;
  }
  else {
    msgPtr = &msg;
  }

  if ( ! msgPtr->Get( "MAX_FID", maxFid ) ) {
    tssErr = RaiMsgErr::getErr( RaiMsgErr::MISSING_MAX_FID );
    logError( LERROR, tssErr, "No MAX_FID field in dictionary message" );
    throw tssErr;
  }

  if ( maxFid > MAX_FID ) {
    tssErr = RaiMsgErr::getErr( RaiMsgErr::BIG_MAX_FID );
    logError( LERROR, tssErr, "MAX_FID (%u) is larger than MAX_ENTRIES (%u)",
              maxFid, MAX_FID );
    throw tssErr;
  }

  if ( ! msgPtr->Activate( "FIDS" ) ) {
    tssErr = RaiMsgErr::getErr( RaiMsgErr::MISSING_FIDS_MSG );
    logError( LERROR, tssErr, "No FIDS sub message in dictionary message" );
    throw tssErr;
  }

  entryCount = 0;
  formCount  = 0;
  indexSize  = 0;
  stringLen  = 0;
  len        = 0;
  if ( field.First( msgPtr ) ) {
    do {
      tssErr = NULL;
      field.Get( fid );
      if ( fid > MAX_FID ) {
        tssErr = RaiMsgErr::getErr( RaiMsgErr::BIG_FID );
        logError( LERROR, tssErr, "Fid (%u) is larger than MAX_ENTRIES (%u)",
                  fid, MAX_FID );
        throw tssErr;
      }
      field.GetHint( u32 );
      isPartial = (bool) ( ( u32 >> 24 ) ? true : false );
      dataType  = (Rai_u32) (RaiTSS_type) ( ( u32 >> 16 ) & 0xff );
      dataSize  = (Rai_u32) (Rai_u16) u32;
      len       = field.NameSize();
      tssErr    = tssCheckEntry( len, dataType, dataSize, isPartial );
      if ( tssErr != NULL ) {
        logError( LERROR, tssErr,
                  "Invalid field (%s), FID=%u DATA_TYPE=%u DATA_SIZE=%u "
                  "IS_PARTIAL=%s in packed dictionary, class name=\"%s\"",
                   ( tssFixEntry1( len, dataType, dataSize,
                                   isPartial ) ? "fixed" : "skipped" ),
                   (unsigned int) fid, dataType, dataSize,
                   ( isPartial ? "true" : "false" ),
                   field.Name() );
      }

      stringLen += len;
      entryCount++;
    } while ( field.Next() );
  }

  if ( entryCount == 0 || entryCount > MAX_FID ) {
    tssErr = RaiMsgErr::getErr( RaiMsgErr::TOO_MANY_CLASSES );
    logError( LERROR, tssErr, "Bad dictionary entry count (%u)", entryCount );
    throw tssErr;
  }

  msgPtr->Activate( NULL );
  if ( msgPtr->Activate( "FORMS" ) && field.First( msgPtr ) ) {
    do {
      field.Get( fid );
      if ( fid > MAX_FID ) {
        tssErr = RaiMsgErr::getErr( RaiMsgErr::BIG_FID );
        logError( LERROR, tssErr, "Fid (%u) is larger than MAX_ENTRIES (%u)",
                          fid, MAX_FID );
        throw tssErr;
      }
      len = field.NameSize();
      if ( len == 0 ) {
        tssErr = RaiMsgErr::getErr( RaiMsgErr::NULL_DICT_FNAME );
        logError( LERROR, tssErr, "Null form name in packed dictionary" );
        throw tssErr;
      }
      stringLen += len;
      field.GetHint( count );
      for ( i = 0; i < count; i++ ) {
        if ( ! field.Next() ) {
          tssErr = RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS );
          logError( LERROR, tssErr, "Missing field in form, field count %u",
                    count );
          throw tssErr;
        }
        len = field.NameSize();
        if ( len == 0 ) {
          tssErr = RaiMsgErr::getErr( RaiMsgErr::NULL_DICT_FNAME );
          logError( LERROR, tssErr, "Null field name in packed dictionary" );
          throw tssErr;
        }
        stringLen += len;
        field.Get( fieldFid[ i ] );
      }
      formCount++;
      indexSize  += RaiMsg_form::calcIndexSize( fieldFid, count );
      entryCount += count + 1; /* for each field + form */
    } while ( field.Next() );
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

  msgPtr->Activate( NULL );
  msgPtr->Activate( "FIDS" );
  try {
    field.First( msgPtr );
    do {
      tssErr = NULL;
      field.Get( fid );
      field.GetHint( u32 );
      isPartial  = (bool) ( ( u32 >> 24 ) ? true : false );
      dataType   = (Rai_u32) (RaiTSS_type) ( ( u32 >> 16 ) & 0xff );
      dataSize   = (Rai_u32) (Rai_u16) u32;
      len        = field.NameSize();

      tssErr = tssCheckEntry( len, dataType, dataSize, isPartial );
      if ( tssErr != NULL ) {
        if ( tssFixEntry2( len, dataType, dataSize, isPartial ) ) {
          logMinor( LMINOR,
                    "Fixed field, FID=%u DATA_TYPE=%u DATA_SIZE=%u "
                    "IS_PARTIAL=%s in packed dictionary, class name=\"%s\"",
                    fid, dataType, dataSize, ( isPartial ? "true" : "false" ),
                    field.Name() );
          tssErr = NULL;
        }
      }
      if ( tssErr == NULL ) {
        fname_size = (Rai_u8) len;
        ::memcpy( fname, field.Name(), fname_size );

        if ( dict->fidIndex[ fid ] != 0xffff ) {
          tssErr = RaiMsgErr::getErr( RaiMsgErr::DUPLICATE_FID );
          logError( LERROR, tssErr, "Duplicate fid=%u entry for %s and %s in "
                    "packed dictionary", fid,
                    dict->entry[ dict->fidIndex[ fid ] ].fname, fname );
        }

        if ( tssErr == NULL ) {
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
              break;
            }
          }
        }

        if ( tssErr == NULL ) {
          dict->fidIndex[ fid ] = dict->entryCount;
          entry = &dict->entry[ dict->entryCount++ ];

          entry->init( fname, fname_size );
          entry->fsize   = dataSize;
          entry->fid     = fid;
          entry->partial = isPartial ? 1 : 0;
          entry->ftype   = (Rai_u8) dataType;

          dict->hashIndex[ i ] = entry;
          fname = &fname[ fname_size ];
        }
      }
    } while ( field.Next() );

    msgPtr->Activate( NULL );

    if ( msgPtr->Activate( "FORMS" ) && field.First( msgPtr ) ) {
      do {
        tssErr = NULL;
        field.Get( fid );
        field.GetHint( count );
        fname_size = (Rai_u8) field.NameSize();
        ::memcpy( fname, field.Name(), fname_size );

        if ( dict->fidForm[ fid ] != 0xffffU ) {
          tssErr = RaiMsgErr::getErr( RaiMsgErr::DUPLICATE_FID );
          logError( LERROR, tssErr,
                    "Duplicate form fid (%u) entry for %s and %s in "
                    "packed dictionary",
                    (unsigned int) fid,
                    dict->form[ dict->fidForm[ fid ] ].entry->fname, fname );
          throw tssErr;
        }
        if ( dict->fidIndex[ fid ] != 0xffffU ) {
          tssErr = RaiMsgErr::getErr( RaiMsgErr::DUPLICATE_FID );
          logError( LERROR, tssErr,
                    "Duplicate fid (%u) entry for %s and %s in "
                    "packed dictionary",
                    (unsigned int) fid,
                    dict->entry[ dict->fidIndex[ fid ] ].fname, fname );
          throw tssErr;
        }

        dict->fidForm[ fid ]  = dict->formCount;
        dict->fidIndex[ fid ] = dict->entryCount;
        form  = &dict->form[ dict->formCount++ ];
        entry = &dict->entry[ dict->entryCount++ ];

        /* the entry for the form */
        entry->init( fname, fname_size );
        entry->fid = fid;

        form->init( dict );
        form->entry      = entry;
        form->fieldCount = count;

        i = Hash32::crc_c( (Rai_u8 *) fname, fname_size - 1 );
        for ( i &= ( MAX_FID - 1 ); dict->hashIndex[ i ] != NULL;
              i = ( i + 1 ) & ( MAX_FID - 1 ) ) {
          if ( dict->hashIndex[ i ]->fname_size == fname_size &&
               ::memcmp( dict->hashIndex[ i ]->fname, fname, fname_size ) == 0 ) {
            tssErr = RaiMsgErr::getErr( RaiMsgErr::DUPLICATE_CLASS );
            logError( LERROR, tssErr,
                      "Duplicate class entry name=%s, fid %u/%u",
                      fname, (Rai_u32) fid,
                      (Rai_u32) dict->hashIndex[ i ]->fid );
            throw tssErr;
          }
        }
        dict->hashIndex[ i ] = entry;

        /* form fields have to be at the end, otherwise fidIndex[] overflows */
        entryCount  -= count;
        form->fields = &dict->entry[ entryCount ];
        fname = &fname[ fname_size ];

        for ( i = 0; i < count; i++ ) {
          field.Next();
          fname_size = (Rai_u8) field.NameSize();
          ::memcpy( fname, field.Name(), fname_size );

          field.Get( fieldFid[ i ] );

          entry = &form->fields[ i ];
          entry->init( fname, fname_size );
          fname = &fname[ fname_size ];

          if ( dict->fidIndex[ fieldFid[ i ] ] == 0xffffU ) {
            tssErr = RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS );
            logError( LERROR, tssErr,
                      "Form %s field fid (%u) entry for %s has no "
                      "class entry in packed dictionary",
                      form->entry->fname, (unsigned int) fid, fname );
            throw tssErr;
          }
        }

        form->fidIndex = index;
        form->initFields( fieldFid, count, false );
        index = &index[ form->indexSize() ];
        form->computeHashes();

      } while ( field.Next() );
    }
    msgPtr->Activate( NULL );

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

    dict->dynamicFormLock = Mutex::create();

    return dict;
  } catch ( ... ) {
    FREE( dict );
    throw;
  }
}

