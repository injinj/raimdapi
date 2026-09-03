/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIMSG_DLL_EXP ) && defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "msg/msg.h"
#include "msg/field.h"
#include "msg/dict.h"
#include "msg/defs.h"
#include "stream/file_stream.h"
#include "util/int_bits.h"
#include "msg/sass_const.h"

using namespace rai;

/*
 * Message layout for { fname = "nam", ftype = RAIMSG_UINT,
 *                      fsize = 4,     fdata = 0x12345678 }
 *
 * RaiMsg:
 *           Version        NameSize        Type Size
 * |- Magic ---|  |- MsgSize -|  |- Name ----|  |  |- Data ----|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |ce|13|aa|1f|01|00|00|00|0b|04|6e|61|6d|00|05|04|12|34|56|78|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20
 *
 * RV:
 *                       NameSize        Type Size
 * |- MsgSize -|- Magic ---|  |- Name ----|  |  |- Data ----|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |00|00|00|13|99|55|ee|aa|04|6e|61|6d|00|0c|04|12|34|56|78|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
 *
 * Rai SASS:
 *                        Dict FID
 * |- Magic ---|- MsgSize -|     |- Data ----|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |11|11|11|12|00|00|00|06|03|e9|12|34|56|78|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14
 *
 * globalDictionary[ 0x3e9 ] = { fname = "nam", fsize = 4, ftype = TSS_U_INT,
 *                               partial = 0, fname_size = 4 }
 */

static int sepChar = '.';

const char CI_SASS_PROTO_STRING[]       = "CI_SASS";
const char CI_SASS_FORM_PROTO_STRING[]  = "CI_SASS_FORM";
const char TIB_SASS_PROTO_STRING[]      = "TIB_SASS";
const char TIB_SASS_FORM_PROTO_STRING[] = "TIB_SASS_FORM";
const char RV_PROTO_STRING[]            = "RVMSG";
const char RAIMSG_PROTO_STRING[]        = "TIBMSG";
const char BAD_PROTO_STRING[]           = "BAD_RAIMSG_PROTOCOL";


const char *
RaiMsg::ProtoToString( RaiMsg_protocol proto )
{
  switch ( proto ) {
    case RAIMSG_PROTO:         return RAIMSG_PROTO_STRING;
    case TIB_SASS_PROTO:       return TIB_SASS_PROTO_STRING;
    case TIB_SASS_FORM_PROTO:  return TIB_SASS_FORM_PROTO_STRING;
    case RV_PROTO:             return RV_PROTO_STRING;
    case CI_SASS_PROTO:        return CI_SASS_PROTO_STRING;
    case CI_SASS_FORM_PROTO:   return CI_SASS_FORM_PROTO_STRING;

    case RV_SASS_PROTO:
    case RV_RAIMSG_PROTO:
    case XREP_PROTO:               
    default:                   return BAD_PROTO_STRING;
  }
}


RaiMsg_protocol
RaiMsg::StringToProto( const char *s )
{
  if ( s != NULL ) {
    switch ( s[ 0 ] ) {
      case 'C':
        if ( ::strcmp( s, CI_SASS_PROTO_STRING ) == 0 )
          return CI_SASS_PROTO;
        if ( ::strcmp( s, CI_SASS_FORM_PROTO_STRING ) == 0 )
          return CI_SASS_FORM_PROTO;
        break;
      case 'T':
        if ( ::strcmp( s, TIB_SASS_PROTO_STRING ) == 0 )
          return TIB_SASS_PROTO;
        if ( ::strcmp( s, TIB_SASS_FORM_PROTO_STRING ) == 0 )
          return TIB_SASS_FORM_PROTO;
        if ( ::strcmp( s, RAIMSG_PROTO_STRING ) == 0 ) /* actually TIBMSG */
          return RAIMSG_PROTO;
        break;
      case 'R':
        if ( ::strcmp( s, RV_PROTO_STRING ) == 0 )
          return RV_PROTO;
        break;
      default:
        break;
    }
  }
  return (RaiMsg_protocol) -1;
}


void
RaiMsg::ReUse( void )
{
  if ( this->msgBuf != NULL ) {
    if ( this->msgEx != NULL )
      this->ReleaseExtra();
    if ( this->proto == RAIMSG_PROTO &&
         this->isDynamic == RAIMSG_MEMORY_DYNAMIC &&
         this->msgStart == 10 && this->msgBuf[ 0 ] == 'I' ) {
      this->parent = NULL;
      this->msgEx  = NULL;
      RaiMsg::InitHeader( RaiMsgConst::START_OFF[ RAIMSG_PROTO ] );
    }
    else {
      this->InitBuffer( this->msgBuf, 0, this->msgSize, this->proto,
                        this->isDynamic );
    }
  }
}


void
RaiMsg::ReUse( RaiMsg_protocol proto )
{
  if ( this->msgBuf != NULL ) {
    if ( this->msgEx != NULL )
      this->ReleaseExtra();
    this->InitBuffer( this->msgBuf, 0, this->msgSize, proto, this->isDynamic );
  }
  else {
    this->proto = proto;
  }
}


void
RaiMsg::Release( void )
{
  if ( this->msgBuf != NULL ) {
    if ( this->isDynamic == RAIMSG_MEMORY_DYNAMIC )
      FREE( this->msgBuf );
    if ( this->msgEx != NULL )
      this->ReleaseExtra();
    this->msgBuf = NULL;
  }
}


RaiMsg_size
RaiMsg::HeaderSize( RaiMsg_protocol proto )
{
  if ( ! RaiMsg::isValidProto( proto ) )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_PROTO );

  return RaiMsgConst::HDR_BYTES[ proto ];
}


bool
RaiMsg::isEmpty( void ) const
{
  if ( this->msgBuf == NULL || this->SubMsgSize() == 0 )
    return true;
  return false;
}


static inline void
initHeader( RaiMsg_protocol proto,  Rai_u8 *hdr,  RaiMsg_size messageSize )
{
  Rai_u32 magic = RaiMsgConst::MAGIC_NUM[ proto ];

  switch ( proto ) {
    case RAIMSG_PROTO:
      Unaligned::endianPutInt( magic, &hdr[ 0 ] );
      hdr[ 4 ] = 1;
      Unaligned::endianPutInt( (Rai_u32) messageSize, &hdr[ 5 ] );
      break;

    case TIB_SASS_PROTO:
    case TIB_SASS_FORM_PROTO:
      Unaligned::endianPutInt( magic, hdr );
      Unaligned::endianPutInt( (Rai_u32) messageSize, &hdr[ 4 ] );
      break;

    case RV_PROTO:
      Unaligned::endianPutInt( (Rai_u32) messageSize, hdr );
      Unaligned::endianPutInt( magic, (Rai_u8 *) &hdr[ 4 ] );
      break;

    case CI_SASS_PROTO:
    case CI_SASS_FORM_PROTO:
      Unaligned::endianPutInt( (Rai_u16) messageSize, hdr );
      break;

    case RV_SASS_PROTO:
    case RV_RAIMSG_PROTO:
    case XREP_PROTO:
    default:
      break;
  }
}


void
RaiMsg::InitHeader( RaiMsg_protocol proto,  RaiMsg_data msgBuf,
                    RaiMsg_size messageSize )
{
  initHeader( proto, (Rai_u8 *) msgBuf,
              messageSize - RaiMsgConst::HDR_SIZE[ proto ] );
}


void
RaiMsg::InitHeader( RaiMsg_size messageSize )
{
  initHeader( this->proto,
        &this->msgBuf[ this->msgStart - RaiMsgConst::HDR_SIZE[ this->proto ] ],
        messageSize );
}


void
RaiMsg::InitBuffer( RaiMsg_data msg_buffer,  RaiMsg_size msg_off,
                    RaiMsg_size msg_buffer_size,  RaiMsg_protocol proto,
                    RaiMsg_memory is_dynamic )
{
  if ( ! RaiMsg::isValidProto( proto ) )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_PROTO );

  if ( is_dynamic == RAIMSG_MEMORY_FIXED ) {
    if ( msg_off > msg_buffer_size )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_BUFFER );

    this->msgStart = msg_off;
  }
  else {
    if ( msg_off + RaiMsgConst::HDR_BYTES[ proto ] > msg_buffer_size )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_BUFFER );

    this->msgStart  = msg_off + RaiMsgConst::HDR_SIZE[ proto ];
  }

  this->msgBuf    = (Rai_u8 *) msg_buffer;
  this->proto     = proto;
  this->msgSize   = msg_buffer_size;
  this->isDynamic = is_dynamic;
  this->parent    = NULL;
  this->msgEx     = NULL;

  if ( is_dynamic != RAIMSG_MEMORY_FIXED )
    this->InitHeader( RaiMsgConst::START_OFF[ proto ] );
}


void
RaiMsg::InitSubMessage( Rai_u8 *msg_buffer,  RaiMsg_size msg_buffer_size,
                        RaiMsg *parent,  RaiMsg_protocol proto,
                        RaiMsg_memory isDynamic )
{
  if ( this->msgBuf != NULL ) {
    if ( this->isDynamic == RAIMSG_MEMORY_DYNAMIC )
      FREE( this->msgBuf );
    if ( this->msgEx != NULL )
      this->ReleaseExtra();
  }
  this->proto     = proto;
  this->msgBuf    = msg_buffer;
  this->msgStart  = 0;
  this->msgSize   = msg_buffer_size;
  this->isDynamic = isDynamic;
  this->parent    = parent;
  this->msgEx     = NULL;
}


void
RaiMsg::ClearForm( const RaiMsg_form *form )
{
  Rai_u8    * formBuf,
            * msgBuf;
  RaiMsg_size formSize,
              msgOff,
              msgLen;
  Rai_u32     i;
  Rai_u16     partialLen,
              foffset;

  if ( form == NULL || ! this->isForm() )
    throw RaiMsgErr::getErr( RaiMsgErr::NOT_SASS_FORM );

  formSize = (RaiMsg_size) form->getFormSize();
  msgBuf   = this->msgBuf;
  msgOff   = 0;

  if ( msgBuf == NULL )
    msgLen = RaiMsgConst::HDR_BYTES[ this->proto ] + formSize;
  else if ( this->isDynamic == RAIMSG_MEMORY_FIXED )
    msgLen = this->msgStart + formSize;
  else {
    msgLen = this->msgStart + RaiMsgConst::START_OFF[ this->proto ] + formSize;
    msgOff = this->msgStart - RaiMsgConst::HDR_SIZE[ this->proto ];
  }

  if ( msgBuf == NULL || this->isDynamic == RAIMSG_MEMORY_DYNAMIC ) {
    if ( msgBuf == NULL || this->msgSize < msgLen ) {
      REALLOC( msgLen, &msgBuf );
      this->InitBuffer( msgBuf, msgOff, msgLen, this->proto,
                        RAIMSG_MEMORY_DYNAMIC );
    }
  }
  else {
    if ( this->msgSize < msgLen )
      throw RaiMsgErr::getErr( RaiMsgErr::MEMORY_STATIC_ERR );
  }

  if ( this->isDynamic == RAIMSG_MEMORY_FIXED ) {
    /* no header in fixed form msgs */
    formBuf = &msgBuf[ this->msgStart ];
  }
  else {
    formBuf = &msgBuf[ this->msgStart + RaiMsgConst::SIZE_OFF[ this->proto ] ];
    msgLen  = formSize + RaiMsgConst::START_OFF[ this->proto ];

    if ( RaiMsgConst::SIZE_LEN[ this->proto ] == 2 )
      Unaligned::endianPutInt( (Rai_u16) msgLen, (Rai_u8 *) formBuf );
    else
      Unaligned::endianPutInt( (Rai_u32) msgLen, (Rai_u8 *) formBuf );

    formBuf = &msgBuf[ this->msgStart + RaiMsgConst::START_OFF[ this->proto ] ];
  }

  ::memset( formBuf, 0, formSize );
  for ( i = 0; i < form->fieldCount; i++ ) {
    foffset = form->fields[ i ].foffset;
    Unaligned::endianPutInt(
      (Rai_u16) ( form->fields[ i ].fid | FID_FIXED_FLAG ),
      (Rai_u8 *) &formBuf[ foffset ] );
    if ( form->fields[ i ].partial ) {
      partialLen = form->fields[ i ].fsize;
      foffset   += 4;
      Unaligned::endianPutInt( partialLen, (Rai_u8 *) &formBuf[ foffset ] );
    }
  }
}


bool
RaiMsg::CheckIsForm( const RaiMsg_form *form,  bool inFormOrder )

{
  Rai_u8            * formBuf,
                    * msgBuf,
                    * msgEnd;
  const RaiMsg_dict * entry;
  RaiField            field;
  RaiMsg_size         formSize,
                      msgLen;
  Rai_u16             fid,
                      u16;
  Rai_u32             i;

  if ( form == NULL )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );

  if ( form->fieldCount == 0 )
    return false;

  formSize = (RaiMsg_size) form->getFormSize();
  msgBuf   = this->msgBuf;

  if ( this->isDynamic == RAIMSG_MEMORY_FIXED ) {
    msgEnd = &msgBuf[ this->msgSize ];
    if ( inFormOrder ) {
      msgLen = this->msgStart + formSize;
      if ( msgEnd != &msgBuf[ msgLen ] ) {
        if ( &msgEnd[ -2 ] != &msgBuf[ msgLen ] ||
             msgEnd[ -1 ] != 0 || msgEnd[ -2 ] != 0 )
          return false;
      }
    }
    formBuf = &msgBuf[ this->msgStart ];
  }
  else {
    formBuf = &msgBuf[ this->msgStart + RaiMsgConst::SIZE_OFF[ this->proto ] ];
    if ( RaiMsgConst::SIZE_LEN[ this->proto ] == 2 ) {
      Unaligned::endianGetInt( (Rai_u8 *) formBuf, u16 );
      msgLen = (Rai_u32) u16;
    }
    else {
      Unaligned::endianGetInt( (Rai_u8 *) formBuf, msgLen );
    }
    msgEnd  = &msgBuf[ this->msgStart + msgLen ];
    formBuf = &msgBuf[ this->msgStart + RaiMsgConst::START_OFF[ this->proto ] ];

    if ( inFormOrder ) {
      if ( msgEnd != &formBuf[ formSize ] ) {
        if ( &msgEnd[ -2 ] != &formBuf[ formSize ] ||
             msgEnd[ -1 ] != 0 || msgEnd[ -2 ] != 0 )
          return false;
      }
    }
  }

  if ( inFormOrder ) {
    for ( i = 0; i < form->fieldCount; i++ ) {
      Unaligned::endianGetInt( (Rai_u8 *) &formBuf[ form->fields[ i ].foffset ],
                               fid );
      u16 = form->fields[ i ].fid;
      if ( ( fid & ( MAX_FID - 1 ) ) != u16 )
        return false;
    }
  }
  else {
    while ( formBuf < msgEnd ) {
      Unaligned::endianGetInt( (Rai_u8 *) formBuf, fid );
      if ( (entry = form->getEntry( fid )) == NULL ) {
        if ( fid == 0 && &formBuf[ 2 ] == msgEnd )
          return true;
        return false;
      }
      if ( entry->partial ) {
        formBuf = entry->unpack( field, formBuf, msgEnd - formBuf );
      }
      else {
        formBuf = &formBuf[ entry->packSize() ];
      }
    }
    if ( formBuf != msgEnd )
      return false;
  }
  return true;
}


void
RaiMsg::Copy( RaiMsg *msg_ptr )
{
  RaiMsg_size msgSize,
              newMsgSize;
  Rai_u8    * msgBuf,
            * oldMsgBuf,
            * bufPtr;

  if ( ! RaiMsg::isValidProto( msg_ptr->proto ) )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_PROTO );

  /* could be sub msg of msg_ptr */
  msgSize = msg_ptr->SubMsgSize();
  if ( msg_ptr->isDynamic == RAIMSG_MEMORY_FIXED )
    msgSize += RaiMsgConst::START_OFF[ msg_ptr->proto ];
  msgBuf     = this->msgBuf;
  newMsgSize = msgSize + RaiMsgConst::HDR_SIZE[ msg_ptr->proto ];

  /* check if static buffer space is enough to hold new message */
  if ( msgBuf != NULL && this->msgSize < newMsgSize &&
      this->isDynamic != RAIMSG_MEMORY_DYNAMIC )
    throw RaiMsgErr::getErr( RaiMsgErr::MEMORY_STATIC_ERR );

  /* realloc if dynamic */
  oldMsgBuf = msgBuf;
  if ( msgBuf == NULL || this->isDynamic == RAIMSG_MEMORY_DYNAMIC ) {
    REALLOC( newMsgSize, &msgBuf );
    this->msgBuf    = msgBuf;
    this->msgSize   = newMsgSize;
    this->isDynamic = RAIMSG_MEMORY_DYNAMIC;
  }

  /* we have space, now clear out stuff */
  if ( oldMsgBuf != NULL ) {
    if ( this->msgEx != NULL )
      this->ReleaseExtra();
  }

  this->proto    = msg_ptr->proto;
  this->msgStart = RaiMsgConst::HDR_SIZE[ this->proto ];
  this->parent   = NULL;
  this->msgEx    = NULL;

  /* make a new header with a magic in it */
  this->InitHeader( msgSize );

  if ( msg_ptr->isDynamic == RAIMSG_MEMORY_FIXED )
    bufPtr = &msg_ptr->msgBuf[ msg_ptr->msgStart ];
  else
    bufPtr = &msg_ptr->msgBuf[ msg_ptr->msgStart +
                               RaiMsgConst::START_OFF[ this->proto ] ];
  /* copy the message body */
  ::memcpy( &msgBuf[ RaiMsgConst::HDR_BYTES[ this->proto ] ], bufPtr,
            msgSize - RaiMsgConst::START_OFF[ this->proto ] );
}


bool
RaiMsg::Get( const RaiMsg_dict *entry,  RaiField &field )

{
  if ( this->isForm() ) {
    RaiMsg_size off;

    off = this->msgStart + entry->foffset;
    if ( this->isDynamic != RAIMSG_MEMORY_FIXED )
      off += RaiMsgConst::START_OFF[ this->proto ];
    entry->unpack( field, &this->msgBuf[ off ], this->msgSize - off );

    return true;
  }
  return field.Find( this, entry );
}


bool
RaiMsg::Get( const RaiMsg_dict *entry,  char *str,
             RaiMsg_size limit )
{
  RaiField field;
  if ( this->Get( entry, field ) ) {
    field.Convert( str, limit );
    return true;
  }
  return false;
}


bool
RaiMsg::Get( const RaiMsg_dict *entry,  char *&str )
{
  RaiMsg_type type;
  RaiMsg_data data;
  RaiField    field;

  if ( this->Get( entry, field ) ) {
    type = field.Type();
    data = field.Data();
    if ( type == RAIMSG_STRING || type == RAIMSG_OPAQUE ) {
      str = (char *) data;
      return true;
    }
    throw RaiMsgErr::getErr( RaiMsgErr::NOT_STRING_FIELD );
  }
  return false;
}


bool
RaiMsg::Get( const RaiMsg_dict *entry,  RaiMsg_type ftype,
             RaiMsg_size fsize,  RaiMsg_data fdata )
{
  RaiField      field;
  RaiField_data data;

  if ( this->Get( entry, field ) ) {
#if defined( __ICC ) && __ICC == 600
  /* disable: operands are evaluated in unspecified order */
  #pragma warning(disable:981)
#endif
    RaiField::Convert( ftype, fsize, fdata, field.Type(), field.Size(),
                       field.AlignData( data ) );
    return true;
  }
  return false;
}


bool
RaiMsg::Get( RaiMsg_name fname,  RaiField &field )
{
  RaiMsg_name fieldName,
              ptr;
  RaiMsg_size fnameLen;
  RaiMsg    * subMsg;

  if ( fname == NULL )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
  if ( field.First( this ) ) {
    if ( (ptr = ::strchr( fname, sepChar )) != NULL ) {
      fnameLen = ptr - fname;
      do {
        fieldName = field.Name();
        if ( fieldName != NULL &&
             ::strncmp( fieldName, fname, fnameLen ) == 0 &&
             fieldName[ fnameLen ] == '\0' &&
             field.Type() == RAIMSG_MESSAGE ) {

          subMsg = (RaiMsg *) field.Data();
          if ( subMsg->Get( &fname[ fnameLen + 1 ], field ) )
            return true;
        }
      } while ( field.Next() );
    }
    else {
      do {
        fieldName = field.Name();
        if ( fieldName != NULL && ::strcmp( fieldName, fname ) == 0 )
          return true;
      } while ( field.Next() );
    }
  }

  return false;
}


bool
RaiMsg::GetEx( RaiMsg_name fname,  Rai_u16 fid,  RaiField &field )

{
  char        buf[ 256 ];
  RaiMsg_size fnameLen;

  fnameLen = RaiField::MakeFidName( fname, fid, buf );
  if ( field.FindEx( this, buf, fnameLen ) )
    return true;
  return false;
}


bool
RaiMsg::Get( RaiMsg_name fname,  char *str,  RaiMsg_size limit )

{
  RaiField field;
  if ( this->Get( fname, field ) ) {
    field.Convert( str, limit );
    return true;
  }
  return false;
}


bool
RaiMsg::GetEx( RaiMsg_name fname,  Rai_u16 fid,  char *str,  RaiMsg_size limit )

{
  RaiField field;
  if ( this->GetEx( fname, fid, field ) ) {
    field.Convert( str, limit );
    return true;
  }
  return false;
}


static unsigned int
match_hdr_field( const char *name,  const byte nameLen )
{
  static const char msg_type_str[]   = "MSG_TYPE",
                    rec_type_str[]   = "REC_TYPE",
                    seq_no_str[]     = "SEQ_NO",
                    rec_status_str[] = "REC_STATUS";
  if ( name[ 0 ] >= 'M' && name[ 0 ] <= 'S' && name[ 3 ] == '_' ) { /* M,R,S */
    if ( nameLen <= 9 ) {
      if ( nameLen == 9 ) { /* MSG_TYPE, REC_TYPE */
        if ( ::memcmp( name, msg_type_str, sizeof( msg_type_str ) - 1 ) == 0 )
          return 0;
        if ( ::memcmp( name, rec_type_str, sizeof( rec_type_str ) - 1 ) == 0 )
          return 1;
      }
      else if ( nameLen == 7 ) { /* SEQ_NO */
        if ( ::memcmp( name, seq_no_str, sizeof( seq_no_str ) - 1 ) == 0 )
          return 2;
      }
    }
    else if ( nameLen == 11 ) { /* REC_STATUS */
      if ( ::memcmp( name, rec_status_str, sizeof(rec_status_str) - 1 ) == 0 )
        return 3;
    }
  }
  return 4;
}

static Rai_u16
get_hdr_int_val( const byte *data,  const unsigned int size )
{
  Rai_u16 tmp16;
  Rai_u32 tmp32;
  Rai_u64 tmp64;

  if ( size == 2 ) {
    Unaligned::endianGetInt( data, tmp16 );
    return tmp16;
  }
  if ( size == 4 ) {
    Unaligned::endianGetInt( data, tmp32 );
    return (Rai_u16) tmp32;
  }
  if ( size == 8 ) {
    Unaligned::endianGetInt( data, tmp64 );
    return (Rai_u16) tmp64;
  }
  return data[ 0 ];
}


static Rai_u16
get_hdr_real_val( const byte *data,  const unsigned int size )
{
  Rai_f32 tmp32;
  Rai_f64 tmp64;

  if ( size == 4 ) {
    Unaligned::endianGetFloat( data, tmp32 );
    return (Rai_u16) tmp32;
  }
  if ( size == 8 ) {
    Unaligned::endianGetFloat( data, tmp64 );
    return (Rai_u16) tmp64;
  }
  return 0;
}


static Rai_u16
get_hdr_string_val( const byte *data,  const unsigned int size,
                    const unsigned int j )
{
  if ( size > 0 ) {
    switch ( j ) {
      case 0: /* msg_type */
        return (Rai_u16) SassConst::stringToMsgType( (char *) data );
      case 1: /* rec_type */
        if ( DataDictionary != NULL ) {
          const RaiMsg_form *f =
            DataDictionary->getForm( (char *) data );
          if ( f != NULL )
            return f->entry->fid;
        }
        break;
      case 3: /* rec_status */
        return (Rai_u16) SassConst::stringToRecStatus( (char *) data );
      default:
        break;
    }
  }
  return 0;
}

struct SassHdrParse {
  Rai_u32       fieldStart;
  const Rai_u32 fieldEnd;
  byte        * msgBuf;
  Rai_u16       ar[ 4 ];
  Rai_u32       fptr[ 4 ];
  byte          status,
                needFlds;
  unsigned int  k;

  SassHdrParse( Rai_u32 fs,  Rai_u32 fe,  byte *b )
      : fieldStart( fs ), fieldEnd( fe ), msgBuf( b ) {
    ::memset( this->ar, 0, (byte *) &this->k - (byte *) this->ar );
    this->needFlds = 8 | 4 | 2 | 1;
  }

  void getSassHdr( void );

  void getRaiMsgHdr( void );

  void getRvMsgHdr( void );

  bool matchName( const char *name,  const Rai_u8 nameLen ) {
    this->k = match_hdr_field( name,  nameLen );
    if ( this->k < 4 ) {
      this->fptr[ this->k ] = this->fieldStart;
      return true;
    }
    return false;
  }
  bool matchInt( unsigned int dataoff,  unsigned int fsize ) {
    this->ar[ this->k ] = get_hdr_int_val( &this->msgBuf[ dataoff ], fsize );
    this->status |= 1 << this->k;
    return this->status == this->needFlds;
  }
  bool matchString( unsigned int dataoff,  unsigned int fsize ) {
    this->ar[ this->k ] =
      get_hdr_string_val( &this->msgBuf[ dataoff ], fsize, this->k );
    this->status |= 1 << this->k;
    return this->status == this->needFlds;
  }
  bool matchReal( unsigned int dataoff,  unsigned int fsize ) {
    this->ar[ this->k ] = get_hdr_real_val( &this->msgBuf[ dataoff ], fsize );
    this->status |= 1 << this->k;
    return this->status == this->needFlds;
  }
};


void
SassHdrParse::getSassHdr( void )
{
  Rai_u16             hdr[ 4 ];
  const RaiMsg_dict * entry;
  RaiMsg_size         fsize;
  Rai_u16             partialLen,
                      fid,
                      mask;
  unsigned int        tested = 0,
                      i;

  if ( DataDictionary != NULL ) {
    hdr[ 0 ] = DataDictionary->msgType != NULL ?
               DataDictionary->msgType->fid : 4001;
    hdr[ 1 ] = DataDictionary->recType != NULL ?
               DataDictionary->recType->fid : 4002;
    hdr[ 2 ] = DataDictionary->seqNo != NULL ?
               DataDictionary->seqNo->fid : 4003;
    hdr[ 3 ] = DataDictionary->recStatus != NULL ?
               DataDictionary->recStatus->fid : 4005;
  }
  else {
    hdr[ 0 ] = 4001;
    hdr[ 1 ] = 4002;
    hdr[ 2 ] = 4003;
    hdr[ 3 ] = 4005;
  }

  this->k = 0;
  mask = hdr[ 0 ] | hdr[ 1 ] | hdr[ 2 ] | hdr[ 3 ];
  for ( i = this->fieldStart; ; this->fieldStart = i ) {
    if ( i + 4 > this->fieldEnd )
      return;
    Unaligned::endianGetInt( &this->msgBuf[ i ], fid );

    if ( DataDictionary != NULL &&
         (entry = DataDictionary->getEntry( fid )) != NULL ) {
      if ( ! entry->partial )
        fsize = 1U + ( ( (unsigned int) entry->fsize + 1 ) >> 1 );
      else {
        if ( i + 6 > this->fieldEnd )
          return;
        Unaligned::endianGetInt( &this->msgBuf[ i + 4 ], partialLen );
        fsize = 3U + ( ( (unsigned int) partialLen + 1 ) >> 1 );
      }
      fsize <<= 1;
    }
    else {
      fsize = 4; /* if no dictionary, just look at first 4 fields */
      tested++;
    }
    if ( ( fid & mask ) == ( fid & ( MAX_FID - 1 ) ) ) {
      for ( unsigned int j = 0; j < 4; j++ ) {
        if ( ( fid & ( MAX_FID - 1 ) ) == hdr[ this->k ] ) {
          this->fptr[ this->k ] = this->fieldStart;
          if ( this->matchInt( i + 2, fsize - 2 ) )
            return;
          this->k = ( this->k + 1 ) % 4;
          break;
        }
        this->k = ( this->k + 1 ) % 4;
      }
    }
    if ( tested == 4 )
      return;
    i += fsize;
  }
}


void
SassHdrParse::getRaiMsgHdr( void )
{
  unsigned int i;
  const char * name;
  Rai_u32      size,
               hintSize;
  Rai_u8       nameLen,
               typeKey,
               type;

  for ( i = this->fieldStart; ; this->fieldStart = i ) {
    if ( i + 3 > this->fieldEnd )
      return;
    name       = (char *) &this->msgBuf[ i + 1 ];
    nameLen    = this->msgBuf[ i++ ];
    i         += nameLen;
    typeKey    = this->msgBuf[ i++ ];
    type       = typeKey & 0xfU;
    if ( ( typeKey & 0x80U ) == 0 )
      size = this->msgBuf[ i++ ] & 0xff;
    else {
      if ( i + 4 > this->fieldEnd )
        return;
      Unaligned::endianGetInt( &this->msgBuf[ i ], size );
      i += 4;
    }
    i += size;
    if ( i > this->fieldEnd )
      return;
    switch ( type ) {
      case RAIMSG_STRING:
      case RAIMSG_INT:
      case RAIMSG_UINT:
      case RAIMSG_REAL:
        if ( this->matchName( name, nameLen ) ) {
          if ( ( type == RAIMSG_UINT || type == RAIMSG_INT ) ?
                 this->matchInt( i - size, size ) :
               ( type == RAIMSG_STRING ) ?
                 this->matchString( i - size, size ) :
                 this->matchReal( i - size, size ) )
            return;
        }
        /* fall through */
      case RAIMSG_OPAQUE:
      case RAIMSG_IPDATA:
      case RAIMSG_BOOLEAN:
        if ( ( typeKey & 0x40 ) == 0 )
          break;

      case RAIMSG_PARTIAL:
      case RAIMSG_ARRAY:
        if ( i + 2 > this->fieldEnd )
          return;
        typeKey = this->msgBuf[ i++ ];
        if ( ( typeKey & 0x80U ) == 0 )
          hintSize = this->msgBuf[ i++ ] & 0xff;
        else {
          if ( i + 4 > this->fieldEnd )
            return;
          Unaligned::endianGetInt( &this->msgBuf[ i ], hintSize );
          i += 4;
        }

        if ( type != RAIMSG_PARTIAL && type != RAIMSG_ARRAY )
          i += hintSize;
        break;
      case RAIMSG_MESSAGE:
        break;
      default:
        return;
    }
  }
}


void
SassHdrParse::getRvMsgHdr( void )
{
  const char * name;
  Rai_u32      size;
  Rai_u8       nameLen,
               type;
  unsigned int i;

  for ( i = this->fieldStart;; this->fieldStart = i ) {
    if ( i + 3 > this->fieldEnd )
      return;
    name    = (char *) &this->msgBuf[ i + 1 ];
    nameLen = this->msgBuf[ i++ ];
    i += nameLen;
    if ( i + 3 > this->fieldEnd )
      return;
    type = this->msgBuf[ i++ ];

    unsigned int szbytes = 0;
    switch ( type ) {
      case RAI_RV_RVMSG:
        if ( this->msgBuf[ i++ ] != RAI_RV_LONG_SIZE )
          return;
        if ( i + 4 > this->fieldEnd )
          return;
        Unaligned::endianGetInt( &this->msgBuf[ i ], size );
        break;

      case RAI_RV_STRING:
      case RAI_RV_SUBJECT:
      case RAI_RV_ENCRYPTED:
      case RAI_RV_OPAQUE:
      case RAI_RV_ARRAY_I8:
      case RAI_RV_ARRAY_U8:
      case RAI_RV_ARRAY_I16:
      case RAI_RV_ARRAY_U16:
      case RAI_RV_ARRAY_I32:
      case RAI_RV_ARRAY_U32:
      case RAI_RV_ARRAY_I64:
      case RAI_RV_ARRAY_U64:
      case RAI_RV_ARRAY_F32:
      case RAI_RV_ARRAY_F64:
        size = this->msgBuf[ i++ ];
        switch ( size ) {
          case RAI_RV_LONG_SIZE:
            if ( i + 4 > this->fieldEnd )
              return;
            Unaligned::endianGetInt( &this->msgBuf[ i ], size );
            szbytes = 4;
            break;
          case RAI_RV_SHORT_SIZE: {
            Rai_u16 tmps;
            Unaligned::endianGetInt( &this->msgBuf[ i ], tmps );
            size = tmps;
            szbytes = 2;
            break;
          }
          case RAI_RV_TINY_SIZE:
            size = this->msgBuf[ i ];
            szbytes = 1;
            break;
          default:
            break;
        }
        break;

      case RAI_RV_DATETIME:
      case RAI_RV_BOOLEAN:
      case RAI_RV_IPDATA:
      case RAI_RV_INT:
      case RAI_RV_UINT:
      case RAI_RV_REAL:
        size = this->msgBuf[ i++ ];
        break;

      default:
        return;
    }

    i += size;
    size -= szbytes;
    if ( i > this->fieldEnd )
      return;

    switch ( type ) {
      case RAI_RV_STRING:
      case RAI_RV_INT:
      case RAI_RV_UINT:
      case RAI_RV_REAL:
        if ( this->matchName( name, nameLen ) ) {
          if ( ( type == RAI_RV_UINT || type == RAI_RV_INT ) ?
                 this->matchInt( i - size, size ) :
               ( type == RAI_RV_STRING ) ?
                 this->matchString( i - size, size ) :
                 this->matchReal( i - size, size ) )
            return;
        }
        break;
    }
  }
}


byte
RaiMsg::GetSassHeader( Rai_u16 &msgType,  Rai_u16 &recType,
                       Rai_u16 &seqNo,  Rai_u16 &recStatus,
                       Rai_u8 needFlds )

{
  SassHdrParse hdr( this->msgStart +
                      ( ( this->isDynamic != RAIMSG_MEMORY_FIXED ) ?
                          RaiMsgConst::START_OFF[ this->proto ] : 0 ),
                      this->msgStart + this->SubMsgSize(),
                      this->msgBuf );
  hdr.needFlds = needFlds;

  if ( this->proto == RAIMSG_PROTO )
    hdr.getRaiMsgHdr();
  else if ( this->proto == RV_PROTO )
    hdr.getRvMsgHdr();
  else
    hdr.getSassHdr();
  msgType   = hdr.ar[ 0 ];
  recType   = hdr.ar[ 1 ];
  seqNo     = hdr.ar[ 2 ];
  recStatus = hdr.ar[ 3 ];

  return hdr.status;
}

byte
RaiMsg::GetSassHeaderFields( RaiField &msgTypeF,  RaiField &recTypeF,
                             RaiField &seqNoF,  RaiField &recStatusF,
                             Rai_u16 &msgType,  Rai_u16 &recType,
                             Rai_u16 &seqNo,  Rai_u16 &recStatus )

{
  SassHdrParse hdr( this->msgStart +
                      ( ( this->isDynamic != RAIMSG_MEMORY_FIXED ) ?
                          RaiMsgConst::START_OFF[ this->proto ] : 0 ),
                      this->msgStart + this->SubMsgSize(),
                      this->msgBuf );

  if ( this->proto == RAIMSG_PROTO )
    hdr.getRaiMsgHdr();
  else if ( this->proto == RV_PROTO )
    hdr.getRvMsgHdr();
  else
    hdr.getSassHdr();
  msgType   = hdr.ar[ 0 ];
  recType   = hdr.ar[ 1 ];
  seqNo     = hdr.ar[ 2 ];
  recStatus = hdr.ar[ 3 ];

  if ( ( hdr.status & HAVE_MSG_TYPE ) != 0 )
    msgTypeF.SetPointer( this, hdr.fptr[ 0 ] );
  if ( ( hdr.status & HAVE_REC_TYPE ) != 0 )
    recTypeF.SetPointer( this, hdr.fptr[ 1 ] );
  if ( ( hdr.status & HAVE_SEQ_NO ) != 0 )
    seqNoF.SetPointer( this, hdr.fptr[ 2 ] );
  if ( ( hdr.status & HAVE_REC_STATUS ) != 0 )
    recStatusF.SetPointer( this, hdr.fptr[ 3 ] );

  return hdr.status;
}

#if 0
/* template version of Get works for this, must pass const char * */
bool
RaiMsg::Get( RaiMsg_name fname,  char *&str )
{
  RaiMsg_type type;
  RaiMsg_data data;
  RaiField    field;

  if ( this->Get( fname, field ) ) {
    type = field.Type();
    data = field.Data();
    if ( type == RAIMSG_STRING || type == RAIMSG_OPAQUE ) {
      str = (char *) data;
      return true;
    }
    throw RaiMsgErr::getErr( RaiMsgErr::NOT_STRING_FIELD );
  }
  return false;
}
#endif


bool
RaiMsg::Get( RaiMsg_name fname,  RaiMsg_type ftype,
             RaiMsg_size fsize,  RaiMsg_data fdata )
{
  RaiField      field;
  RaiField_data data;

  if ( this->Get( fname, field ) ) {
    RaiField::Convert( ftype, fsize, fdata, field.Type(), field.Size(),
                       field.AlignData( data ) );
    return true;
  }
  return false;
}


bool
RaiMsg::GetEx( RaiMsg_name fname,  Rai_u16 fid,  RaiMsg_type ftype,
               RaiMsg_size fsize,  RaiMsg_data fdata )
{
  RaiField      field;
  RaiField_data data;

  if ( this->GetEx( fname, fid, field ) ) {
    RaiField::Convert( ftype, fsize, fdata, field.Type(), field.Size(),
                       field.AlignData( data ) );
    return true;
  }
  return false;
}


RaiMsg_extra::RaiMsg_extra( RaiMsg_extra *enext,  RaiMsg_size arrayOffset,
                            RaiMsg_data decodedArray )
{
  this->next = enext;
  this->type = ARRAY;

  this->u.ar.arrayOffset  = arrayOffset;
  this->u.ar.decodedArray = decodedArray;
}


RaiMsg_extra::RaiMsg_extra( RaiMsg_extra *enext,  Rai_u32 top,
                            RaiMsg_size *offset )
{
  this->next = enext;
  this->type = ACTIVATE;

  this->u.act.top = top;
  ::memcpy( this->u.act.offset, offset, sizeof( offset[ 0 ] ) * top );
}


RaiMsg_extra::~RaiMsg_extra()
{
  if ( this->type == ARRAY ) {
    if ( this->u.ar.decodedArray != NULL )
      FREE( this->u.ar.decodedArray );
  }
}


void
RaiMsg::ReleaseExtra( void )
{
  RaiMsg_extra * next;
  while ( this->msgEx != NULL ) {
    next  = msgEx->next;
    delete msgEx;
    this->msgEx = next;
  }
}


void
RaiMsg::GetDecodedArray( RaiMsg_data array,  RaiMsg_type elem_type,
                         RaiMsg_size elem_size,  RaiMsg_size array_size,
                         RaiMsg_data &decodedArray )
{
  RaiMsg       * msg;
  RaiMsg_extra * msgEx;
  RaiMsg_size    arrayOffset;
  Rai_u16      * ar16;
  Rai_u32      * ar32;
  Rai_u64      * ar64;
  RaiMsg_data    ar;

  decodedArray = NULL;
  if ( ! RaiField::isValidMachineType( elem_type, elem_size ) )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_RAIMSG_MTYPE );

  if ( array_size == 0 )
    return;

#if defined( __ICC ) && __ICC == 600
  /* disable: invalid type conversion: "void *" to "unsigned long" */
  #pragma warning(disable:171)
#endif
  /* no need to align bytes */
  if ( elem_size == 1 || ( elem_type == RAIMSG_IPDATA && elem_size == 4 &&
                           ( (ulongptr) (void *) array & 3 ) == 0 ) ) {
    decodedArray = array;
    return;
  }
  /* if already aligned and machine is big */
  if ( ! Aligned::isLittleEndian && ( (ulongptr) (void *) array &
                                      ( elem_size - 1 ) ) == 0 ) {
    decodedArray = array;
    return;
  }
  /* find parent and cache decoded arrays with it */
  for ( msg = this; msg->parent != NULL; msg = msg->parent )
    ;
  if ( array < msg->msgBuf || array > &msg->msgBuf[ msg->msgSize ] )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
  /* find array by it's offset into the message */
  arrayOffset = (Rai_u8 *) array - msg->msgBuf;
  for ( msgEx = msg->msgEx; msgEx != NULL; msgEx = msgEx->next ) {
    if ( msgEx->type == ARRAY && msgEx->u.ar.arrayOffset == arrayOffset ) {
      decodedArray = msgEx->u.ar.decodedArray;
      return;
    }
  }
  /* create a new decoded array element and swap bytes */
  switch ( elem_type ) {
    case RAIMSG_BOOLEAN:
    case RAIMSG_INT:
    case RAIMSG_UINT:
    case RAIMSG_REAL:
    case RAIMSG_IPDATA:
      MALLOC( array_size, &ar );
      try {
        msgEx = NEW RaiMsg_extra( msg->msgEx, arrayOffset, ar );
      } catch ( ... ) {
        FREE( ar );
        throw;
      }
      msg->msgEx = msgEx;
      ::memcpy( ar, array, array_size );

      if ( Aligned::isLittleEndian && ( elem_type != RAIMSG_IPDATA ||
                                        elem_size != 4 ) ) {
        switch ( elem_size ) {
          case 2:
            ar16 = (Rai_u16 *) ar;
            do {
              Aligned::swap( ar16[ 0 ] );
              ar16++;
            } while ( (char *) ar16 < &((char *) ar)[ array_size ] );
            break;
          case 4:
            ar32 = (Rai_u32 *) ar;
            do {
              Aligned::swap( ar32[ 0 ] );
              ar32++;
            } while ( (char *) ar32 < &((char *) ar)[ array_size ] );
            break;
          case 8:
            ar64 = (Rai_u64 *) ar;
            do {
              Aligned::swap( ar64[ 0 ] );
              ar64++;
            } while ( (char *) ar64 < &((char *) ar)[ array_size ] );
            break;
        }
      }
      decodedArray = ar;
      break;

    default:
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_RAIMSG_MTYPE );
  }
}


void
RaiMsg::GetDecodedString( RaiMsg_data string,  RaiMsg_size string_size,
                          RaiMsg_data &decodedString )
{
  RaiMsg       * msg;
  RaiMsg_extra * msgEx;
  RaiMsg_size    stringOffset;
  RaiMsg_data    ar;

  decodedString = NULL;

  if ( string_size == 0 )
    return;
  if ( ((char *) string)[ string_size - 1 ] == '\0' ) {
    decodedString = string;
    return;
  }
  /* find parent and cache decoded arrays with it */
  for ( msg = this; msg->parent != NULL; msg = msg->parent )
    ;
  if ( string < msg->msgBuf || string > &msg->msgBuf[ msg->msgSize ] )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
  /* find string by it's offset into the message */
  stringOffset = (Rai_u8 *) string - msg->msgBuf;
  for ( msgEx = msg->msgEx; msgEx != NULL; msgEx = msgEx->next ) {
    if ( msgEx->type == ARRAY && msgEx->u.ar.arrayOffset == stringOffset ) {
      decodedString = msgEx->u.ar.decodedArray;
      return;
    }
  }
  /* create a new decoded array element and swap bytes */
  MALLOC( string_size + 1, &ar );
  try {
    msgEx = NEW RaiMsg_extra( msg->msgEx, stringOffset, ar );
  } catch ( ... ) {
    FREE( ar );
    throw;
  }
  msg->msgEx = msgEx;
  ::memcpy( ar, string, string_size );
  ((char *) ar)[ string_size ] = '\0';
  decodedString = (char *) ar;
}


void
RaiMsg::Apply( RaiMsg_name fname,  RaiMsg_apply_cb callback_function,
               void *closure )
{
  RaiMsg_name fieldName;
  RaiField    field;

  if ( field.First( this ) ) {
    do {
      fieldName = field.Name();
      if ( fname == NULL ||
           ( fieldName != NULL && ::strcmp( fieldName, fname ) == 0 ) ) {
        if ( callback_function( this, &field, closure ) != 0 )
          throw RaiMsgErr::getErr( RaiMsgErr::APPLY_ERROR );
      }
    } while ( field.Next() );
  }
}


void
RaiMsg::SetNameSeparator( char sep_char )
{
  sepChar = (int) (unsigned char) sep_char;
}


void
RaiMsg::AppendHeader( void )
{
  Rai_u8    * msgBuf;
  RaiMsg_size start = 0;

  if ( this->isDynamic == RAIMSG_MEMORY_MAMA )
    start = 1;
  if ( (msgBuf = this->msgBuf) == NULL ||
       ( ( this->isDynamic == RAIMSG_MEMORY_DYNAMIC ||
           this->isDynamic == RAIMSG_MEMORY_MAMA ) &&
         this->msgSize < RaiMsgConst::HDR_BYTES[ this->proto ] + start ) ) {
    REALLOC( 256, &msgBuf );

    if ( this->isDynamic == RAIMSG_MEMORY_MAMA )
      msgBuf[ 0 ] = 'I';
    this->msgBuf    = msgBuf;
    this->msgSize   = 256;
    this->isDynamic = RAIMSG_MEMORY_DYNAMIC;
    this->parent    = NULL;
    this->msgEx     = NULL;
  }
  if ( this->isDynamic == RAIMSG_MEMORY_FIXED ||
       this->msgSize < RaiMsgConst::HDR_BYTES[ this->proto ] )
    throw RaiMsgErr::getErr( RaiMsgErr::MEMORY_STATIC_ERR );

  this->msgStart = RaiMsgConst::HDR_SIZE[ this->proto ] + start;
  //this->InitHeader( RaiMsgConst::START_OFF[ this->proto ] + start );
  RaiMsg::InitHeader( RaiMsgConst::START_OFF[ this->proto ] );
}


Rai_u8 *
RaiMsg::Adjust( RaiMsg_size field_offset, RaiMsg_size new_size,
                RaiMsg_size old_size )
{
  Rai_u8       * msgBuf;
  RaiMsg_extra * msgEx;
  Rai_u8       * msgLenPtr;
  Rai_i32        addBytes,
                 szoff;
  Rai_u16        u16;
  RaiMsg_size    msgLen,
                 topLevelEnd,
                 topEnd,
                 allocSize,
                 count,
                 msgOffset;

  if ( new_size == old_size )
    return this->msgBuf;
  msgBuf = this->msgBuf;
  if ( this->parent != NULL ) {
    msgOffset = (RaiMsg_size) ( msgBuf - this->parent->msgBuf );
    msgBuf    = this->parent->Adjust( field_offset + msgOffset, new_size,
                                      old_size );
    this->msgBuf = &msgBuf[ msgOffset ];
    msgBuf = this->msgBuf;
    szoff = (Rai_i32) ( this->msgStart +
                        RaiMsgConst::SIZE_OFF[ this->proto ] );
    msgLenPtr = &msgBuf[ szoff ];

    if ( RaiMsgConst::SIZE_LEN[ this->proto ] == 2 ) {
      Unaligned::endianGetInt( (Rai_u8 *) msgLenPtr, u16 );
      msgLen = (Rai_u32) u16;
    }
    else {
      Unaligned::endianGetInt( (Rai_u8 *) msgLenPtr, msgLen );
    }
    addBytes = new_size - old_size;
    msgLen = (Rai_u32) ( (Rai_i32) msgLen + addBytes );
    if ( RaiMsgConst::SIZE_LEN[ this->proto ] == 2 ) {
      Unaligned::endianPutInt( (Rai_u16) msgLen, (Rai_u8 *) msgLenPtr );
    }
    else {
      Unaligned::endianPutInt( msgLen, (Rai_u8 *) msgLenPtr );
    }
    if ( this->isDynamic == RAIMSG_MEMORY_FIXED ||
         this->msgStart < RaiMsgConst::HDR_SIZE[ this->proto ] )
      this->msgSize = msgLen;
    return msgBuf;
  }

  /* fixed have no headers */
  if ( this->isDynamic == RAIMSG_MEMORY_FIXED )
    throw RaiMsgErr::getErr( RaiMsgErr::MEMORY_STATIC_ERR );
  szoff = (Rai_i32) ( this->msgStart + RaiMsgConst::SIZE_OFF[ this->proto ] );
  msgLenPtr = &msgBuf[ szoff ];
  if ( RaiMsgConst::SIZE_LEN[ this->proto ] == 2 ) {
    Unaligned::endianGetInt( (Rai_u8 *) msgLenPtr, u16 );
    msgLen = (Rai_u32) u16;
  }
  else {
    Unaligned::endianGetInt( (Rai_u8 *) msgLenPtr, msgLen );
  }
  topLevelEnd = this->msgStart + msgLen;

  if ( (msgEx = this->msgEx) != NULL ) {
    for (;;) {
      if ( msgEx->type == ACTIVATE ) {
        topLevelEnd = msgEx->u.act.offset[ 0 ];
        szoff       = (Rai_i32) ( topLevelEnd +
                                  RaiMsgConst::SIZE_OFF[ this->proto ] );
        if ( RaiMsgConst::SIZE_LEN[ this->proto ] == 2 ) {
          Unaligned::endianGetInt( (Rai_u8 *) &msgBuf[ szoff ], u16 );
          topEnd = (Rai_u32) u16;
        }
        else {
          Unaligned::endianGetInt( (Rai_u8 *) &msgBuf[ szoff ], topEnd );
        }
        topLevelEnd += topEnd;
        break;
      }
      if ( (msgEx = msgEx->next) == NULL )
        break;
    }
  }

  addBytes = new_size - old_size;
  topEnd = topLevelEnd + addBytes;

  if ( topEnd > this->msgSize ) {
    if ( this->isDynamic == RAIMSG_MEMORY_DYNAMIC ) {
      if ( topEnd < 8 * 1024 )
        allocSize = ( topEnd | 0xff ) + 1; /* incr 256 */
      else if ( topEnd < 64 * 1024 )
        allocSize = ( topEnd | 0x3ff ) + 1; /* incr 1024 */
      else if ( topEnd < 512 * 1024 )
        allocSize = ( topEnd | 0x3fff ) + 1; /* incr 16k */
      else
        allocSize = ( topEnd | 0x3ffff ) + 1; /* incr 256k */
      REALLOC( allocSize, &msgBuf );
      this->msgBuf  = msgBuf;
      this->msgSize = allocSize;
      szoff = (Rai_i32) ( this->msgStart +
                          RaiMsgConst::SIZE_OFF[ this->proto ] );
      msgLenPtr = &msgBuf[ szoff ];
    }
    else {
      throw RaiMsgErr::getErr( RaiMsgErr::MEMORY_STATIC_ERR );
    }
  }

  msgLen = (Rai_u32) ( (Rai_i32) msgLen + addBytes );
  if ( RaiMsgConst::SIZE_LEN[ this->proto ] == 2 ) {
    Unaligned::endianPutInt( (Rai_u16) msgLen, (Rai_u8 *) msgLenPtr );
  }
  else {
    Unaligned::endianPutInt( msgLen, (Rai_u8 *) msgLenPtr );
  }

  if ( msgEx != NULL ) {
    for ( count = msgEx->u.act.top; count > 0; ) {
      szoff = (Rai_i32) ( msgEx->u.act.offset[ --count ] +
                          RaiMsgConst::SIZE_OFF[ this->proto ] );
      msgLenPtr = &msgBuf[ szoff ];
      if ( RaiMsgConst::SIZE_LEN[ this->proto ] == 2 ) {
        Unaligned::endianGetInt( (Rai_u8 *) msgLenPtr, u16 );
        msgLen = (Rai_u32) u16;
        msgLen = (Rai_u32) ( (Rai_i32) msgLen + addBytes );
        Unaligned::endianPutInt( (Rai_u16) msgLen, (Rai_u8 *) msgLenPtr );
      }
      else {
        Unaligned::endianGetInt( (Rai_u8 *) msgLenPtr, msgLen );
        msgLen = (Rai_u32) ( (Rai_i32) msgLen + addBytes );
        Unaligned::endianPutInt( msgLen, (Rai_u8 *) msgLenPtr );
      }
    }
  }

  topLevelEnd -= field_offset + old_size;
  if ( topLevelEnd != 0 )
    ::memmove( &msgBuf[ field_offset + new_size ],
               &msgBuf[ field_offset + old_size ], topLevelEnd );
  return msgBuf;
}


RaiMsg_data
RaiMsg::AppendRaw( RaiMsg_data field_data,
                   RaiMsg_size field_size )
{
  RaiMsg_size msgEnd;
  Rai_u8    * msgBuf;

  if ( field_size == 0 )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
  if ( this->msgBuf == NULL )
    this->AppendHeader();

  if ( this->Overlaps( field_data, field_size ) )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_BUFFER );

  msgEnd = this->PackSize();
  msgBuf = this->Adjust( msgEnd, field_size, 0 );
  ::memcpy( &msgBuf[ msgEnd ], field_data, field_size );
  return (RaiMsg_data) &msgBuf[ msgEnd ];
}


RaiMsg_data
RaiMsg::RawData( RaiMsg_size *dataSize )
{
  RaiMsg_size off;

  off = this->msgStart;
  if ( this->isDynamic != RAIMSG_MEMORY_FIXED )
    off += RaiMsgConst::START_OFF[ this->proto ];
  if ( dataSize != NULL )
    *dataSize = ( this->msgStart + this->SubMsgSize() ) - off;
  return &this->msgBuf[ off ];
}

#if 0
static void
raimsg_copy_field_func( RaiField &field1,  RaiMsg * const _this,
                        void ( RaiMsg::*func )( RaiField * ) )
{
  RaiField    field2;
  Rai_u8      memBuf[ 256 ],
            * fmem  = memBuf;
  RaiMsg_size fsize = field1.PackSize( _this->GetProtocol() );
  try {
    if ( fsize > sizeof ( memBuf ) )
      MALLOC( fsize, &fmem );
    field1.Pack( _this->GetProtocol(), fmem );
    field2.UnPack( _this->GetProtocol(), fmem, fsize );
    (_this->*func)( &field2 );
  } catch ( ... ) {
    if ( fmem != memBuf )
      FREE( fmem );
    throw;
  }
  if ( fmem != memBuf )
    FREE( fmem );
}


void
RaiMsg::Append( RaiField *field_ptr )
{
  if ( ! field_ptr->Overlaps( *this ) )
    this->Append2( field_ptr );
  else
    raimsg_copy_field_func( *field_ptr, this, &RaiMsg::Append2 );
}
#endif

void
RaiMsg::Append( RaiField *field_ptr )
{
  RaiMsg_size         msgEnd,
                      fieldSize;
  Rai_u8            * msgBuf;
  const RaiMsg_dict * entry;

  if ( this->isSass() ) {
    if ( DataDictionary == NULL )
      throw RaiMsgErr::getErr( RaiMsgErr::NO_DICTIONARY );
    entry = DataDictionary->getEntry( field_ptr->name, field_ptr->nameLen );
    if ( entry == NULL )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_DICT_FNAME );
    this->Append( entry, field_ptr );
    return;
  }

  if ( this->msgBuf == NULL )
    this->AppendHeader();
  if ( (fieldSize = field_ptr->PackSize( this->proto )) == 0 )
    throw RaiMsgErr::getErr( RaiMsgErr::NO_FIELD );

  msgEnd = this->PackSize();
  msgBuf = this->Adjust( msgEnd, fieldSize, 0 );
  field_ptr->Pack( this->proto, &msgBuf[ msgEnd ] );
}


void
RaiMsg::Append( RaiMsg_name fname,  RaiMsg *msg_ptr )
{
  RaiField field;
  /* Need to tell field what proto we are so it can initialize the sub message
   * correctly */
  if ( msg_ptr == NULL || msg_ptr->msgBuf == NULL ) {
    RaiMsg tempMsg( msg_ptr ? msg_ptr->proto : this->proto );
    byte   tempBuf[ 64 ];
    RaiMsg::InitHeader( tempMsg.proto, tempBuf,
                        RaiMsgConst::START_OFF[ tempMsg.proto ] );
    tempMsg.UnPack( tempMsg.proto,  tempBuf, sizeof( tempBuf ),
                    RAIMSG_MEMORY_STATIC, 0 );
    if ( this->proto == tempMsg.proto )
      field.Update( fname, &tempMsg );
    else
      field.Update( fname, RAIMSG_OPAQUE, tempMsg.PackSize(),
                    tempMsg.Packed() );
  }
  else {
    if ( this->proto == msg_ptr->proto )
      field.Update( fname, msg_ptr );
    else
      field.Update( fname, RAIMSG_OPAQUE, msg_ptr->PackSize(),
                    msg_ptr->Packed() );
  }
  this->Append( &field );
}


void
RaiMsg::Append( RaiMsg_name fname,  RaiMsg_data partial_data,
                RaiMsg_size partial_size,  RaiMsg_size offset )

{
  RaiField field;
  field.Update( fname, partial_data, partial_size, offset );
  this->Append( &field );
}


void
RaiMsg::Append( RaiMsg_name fname,  RaiMsg_data array_data,
                RaiMsg_size num_entries,  RaiMsg_type entry_type,
                RaiMsg_size entry_size )
{
  RaiField field;
  field.Update( fname, array_data, num_entries, entry_type, entry_size );
  this->Append( &field );
}


void
RaiMsg::Append( RaiMsg_name fname,  RaiMsg_type ftype,
                RaiMsg_size fsize,  RaiMsg_data fdata )
{
  RaiField field;
  field.Update( fname, ftype, fsize, fdata );
  this->Append( &field );
}


void
RaiMsg::Append( RaiMsg_name fname,  RaiMsg_type ftype,
                RaiMsg_size fsize,  RaiMsg_data fdata,
                RaiMsg_type hint_type,  RaiMsg_size hint_size,
                RaiMsg_data hint_data )
{
  RaiField field;
  field.Update( fname, ftype, fsize, fdata, hint_type, hint_size, hint_data );
  this->Append( &field );
}

#if 0
static void
raimsg_copy_field_func2( const RaiMsg_dict *entry,  RaiField &field1,
                         RaiMsg * const _this,
      void ( RaiMsg::*func )( const RaiMsg_dict *, RaiField * ) )
{
  RaiField    field2;
  Rai_u8      memBuf[ 256 ],
            * fmem  = memBuf;
  RaiMsg_size fsize = entry->partial ?
                      6 + ( ( field1.size + 1U ) & ~1U ) : entry->packSize();
  try {
    if ( fsize > sizeof ( memBuf ) )
      MALLOC( fsize, &fmem );
    Unaligned::endianPutInt( (Rai_u16) ( entry->fid | FID_FIXED_FLAG ), fmem );
    entry->pack( &field1, fmem );
    entry->unpack( &field2, fmem, fsize );
    (_this->*func)( entry, &field2 );
  } catch ( ... ) {
    if ( fmem != memBuf )
      FREE( fmem );
    throw;
  }
  if ( fmem != memBuf )
    FREE( fmem );
}


void
RaiMsg::Append( const RaiMsg_dict *entry,  RaiField *field_ptr )
{
  if ( ! field_ptr->Overlaps( *this ) )
    this->Append2( entry, field_ptr );
  else
    raimsg_copy_field_func2( entry, *field_ptr, this, &RaiMsg::Append2 );
}
#endif

void
RaiMsg::Append( const RaiMsg_dict *entry,  RaiField *field_ptr )

{
  RaiMsg_size msgEnd,
              fieldSize;
  Rai_u8    * msgBuf;

  if ( this->isSass() ) {
    if ( field_ptr->type == RAIMSG_NODATA )
      throw RaiMsgErr::getErr( RaiMsgErr::NO_FIELD );

    if ( this->msgBuf == NULL )
      this->AppendHeader();

    if ( entry->partial ) {
      fieldSize = field_ptr->size;
      if ( fieldSize > entry->fsize )
        fieldSize = entry->fsize;
      fieldSize = 6 + ( ( fieldSize + 1U ) & ~1U );
    }
    else
      fieldSize = entry->packSize();

    msgEnd = this->PackSize();
    msgBuf = this->Adjust( msgEnd, fieldSize, 0 );
    Unaligned::endianPutInt( (Rai_u16) ( entry->fid | FID_CTRL ),
                             (Rai_u8 *) &msgBuf[ msgEnd ] );
    entry->pack( *field_ptr, &msgBuf[ msgEnd ] );
  }
  else {
    RaiField field;
    field.Update( field_ptr );
    field.name    = entry->fname;
    field.nameLen = entry->fname_size;
    field.iterMsg = field_ptr->iterMsg;
    entry->convert( field );
    this->Append( &field );
  }
}


void
RaiMsg::Append( RaiMsg_name fname,  RaiField *field_ptr )

{
  RaiField field;
  field.Update( field_ptr );
  field.name    = fname;
  field.nameLen = ( fname == NULL ? 0 : ::strlen( fname ) + 1 );
  field.iterMsg = field_ptr->iterMsg;
  this->Append( &field );
}


void
RaiMsg::Append( const RaiMsg_dict *entry,  RaiMsg_data partial_data,
                RaiMsg_size partial_size,  RaiMsg_size offset )

{
  RaiField field;
  field.Update( NULL, partial_data, partial_size, offset );
  this->Append( entry, &field );
}


void
RaiMsg::Append( const RaiMsg_dict *entry,  RaiMsg_data array_data,
                RaiMsg_size num_entries,  RaiMsg_type entry_type,
                RaiMsg_size entry_size )
{
  RaiField field;
  field.Update( NULL, array_data, num_entries, entry_type, entry_size );
  this->Append( entry, &field );
}


void
RaiMsg::Append( const RaiMsg_dict *entry,  RaiMsg_type ftype,
                RaiMsg_size fsize,  RaiMsg_data fdata )
{
  RaiField field;
  field.Update( NULL, ftype, fsize, fdata );
  this->Append( entry, &field );
}


void
RaiMsg::Append( const RaiMsg_dict *entry,  RaiMsg_type ftype,
                RaiMsg_size fsize,  RaiMsg_data fdata,
                RaiMsg_type hint_type,  RaiMsg_size hint_size,
                RaiMsg_data hint_data )
{
  RaiField field;
  field.Update( NULL, ftype, fsize, fdata, hint_type, hint_size, hint_data );
  this->Append( entry, &field );
}


void
RaiMsg::Update( RaiMsg_name fname,  RaiField *field_ptr )

{
  RaiField field;
  field.Update( field_ptr );
  field.name    = fname;
  field.nameLen = ( fname == NULL ? 0 : ::strlen( fname ) + 1 );
  field.iterMsg = field_ptr->iterMsg;
  this->Update( &field );
}

#if 0
void
RaiMsg::Update( RaiField *field_ptr )
{
  if ( ! field_ptr->Overlaps( *this ) )
    this->Update2( field_ptr );
  else
    raimsg_copy_field_func( *field_ptr, this, &RaiMsg::Update2 );
}
#endif

void
RaiMsg::Update( RaiField *field_ptr )
{
  RaiMsg_name         fname;
  const RaiMsg_dict * entry;
  RaiMsg_size         fnameSize;

  fname     = field_ptr->Name();
  fnameSize = field_ptr->NameSize();

  if ( this->isSass() ) {
    if ( DataDictionary == NULL )
      throw RaiMsgErr::getErr( RaiMsgErr::NO_DICTIONARY );
    entry = DataDictionary->getEntry( fname, fnameSize );
    if ( entry == NULL )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_DICT_FNAME );
    this->Update( entry, field_ptr );
  }
  else {
    RaiField field;

    if ( field.FindEx( this, fname, fnameSize ) ) {
      this->Replace_SD( field, *field_ptr );
    }
    else {
      this->Append( field_ptr );
    }
  }
}

void
RaiMsg::Replace( RaiField *old_field,  RaiField *new_field )

{
  RaiMsg_name         fname;
  const RaiMsg_dict * old_entry;
  RaiMsg_size         fnameSize;

  if ( this->isSass() ) {
    if ( DataDictionary == NULL )
      throw RaiMsgErr::getErr( RaiMsgErr::NO_DICTIONARY );
    fname     = old_field->Name();
    fnameSize = old_field->NameSize();
    old_entry = DataDictionary->getEntry( fname, fnameSize );
    if ( old_entry == NULL )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_DICT_FNAME );
    this->Replace_SASS( old_entry, *old_field, *new_field );
  }
  else {
    this->Replace_SD( *old_field, *new_field );
  }
  old_field->SetPointer( this, old_field->fieldStart );
}

void
RaiMsg::Replace_SD( RaiField &field,  RaiField &new_field )

{
  char        tmpBuf[ 128 ];
  Rai_u8    * msgBuf;
  RaiMsg_data data;
  RaiMsg_size newSize,
              oldSize,
              partialOff,
              partialLen,
              partialOff2,
              partialLen2,
              dataOff,
              dataOff2,
              off,
              len,
              end,
              end2;
  Rai_u8      partialType;

  if ( field.type == RAIMSG_PARTIAL ||
     ( ( new_field.type == RAIMSG_PARTIAL &&
         ( field.type == RAIMSG_STRING || field.type == RAIMSG_OPAQUE ) ) &&
           this->proto == RAIMSG_PROTO ) ) {
    if ( field.type == RAIMSG_PARTIAL ) {
      partialOff  = field.hintSize;
      partialType = RAIMSG_PARTIAL | 0x40;
    }
    else {
      partialOff = 0;
      partialType = field.type;
    }
    partialLen = field.size;
    if ( new_field.type == RAIMSG_PARTIAL ) {
      partialOff2 = new_field.hintSize;
      partialLen2 = new_field.size;
      data        = new_field.data;
    }
    else if ( new_field.type == RAIMSG_STRING ||
              new_field.type == RAIMSG_OPAQUE ) {
      partialOff2 = 0;
      partialLen2 = new_field.size;
      data        = new_field.data;
    }
    else {
      partialOff2 = 0;
      new_field.Get( tmpBuf, sizeof( tmpBuf ) );
      partialLen2 = ::strlen( tmpBuf );
      data        = tmpBuf;
    }

    off  = ( partialOff < partialOff2 ) ? partialOff : partialOff2;
    end  = partialOff + partialLen;
    end2 = partialOff2 + partialLen2;
    len  = ( end > end2 ) ? end : end2;
    len -= off;

    oldSize        = field.fieldEnd - field.fieldStart;
    field.size     = len;
    field.hintSize = off;
    newSize        = field.PackSize( this->proto );

    msgBuf   = this->Adjust( field.fieldStart, newSize, oldSize );
    msgBuf   = &msgBuf[ field.fieldStart ];

    dataOff  = new_field.NameSize() + 1;
    dataOff2 = dataOff;

    if ( ( msgBuf[ dataOff ] & 0x80U ) != 0 )
      dataOff += 3;
    dataOff += 2;

    if ( len > 0xffU )
      dataOff2 += 3;
    dataOff2 += 2;

    partialOff -= off;
    if ( partialOff + dataOff2 != dataOff )
      ::memmove( &msgBuf[ partialOff + dataOff2 ], &msgBuf[ dataOff ],
                 partialLen );

    if ( len > 0xffU ) {
      msgBuf    = &msgBuf[ dataOff2 - 5 ];
      *msgBuf++ = (Rai_u8) ( 0x80U | partialType );
      Unaligned::endianPutInt( (Rai_u32) len, msgBuf );
      msgBuf    = &msgBuf[ 4 ];
    }
    else {
      msgBuf    = &msgBuf[ dataOff2 - 2 ];
      *msgBuf++ = (Rai_u8) partialType;
      *msgBuf++ = (Rai_u8) len;
    }
    if ( ( partialType & 0x40U ) != 0 ) {
      if ( off <= 0xffU ) {
        msgBuf[ len ] = (Rai_u8) RAIMSG_UINT;
        msgBuf[ len + 1 ] = (Rai_u8) off;
      }
      else {
        msgBuf[ len ] = (Rai_u8) ( 0x80U | RAIMSG_UINT );
        Unaligned::endianPutInt( (Rai_u32) off, &msgBuf[ len + 1 ] );
      }
    }

    partialOff2 -= off;
    ::memcpy( &msgBuf[ partialOff2 ], data, partialLen2 );

    if ( partialOff < partialOff2 ) {
      partialOff += partialLen;
      if ( partialOff < partialOff2 )
        ::memset( &msgBuf[ partialOff ], ' ', partialOff2 - partialOff );
    }
    else {
      partialOff2 += partialLen2;
      if ( partialOff2 < partialOff )
        ::memset( &msgBuf[ partialOff2 ], ' ', partialOff - partialOff2 );
    }
  }
  else {
    if ( (newSize = new_field.PackSize( this->proto )) == 0 )
      throw RaiMsgErr::getErr( RaiMsgErr::NO_FIELD );

    oldSize = field.fieldEnd - field.fieldStart;
    msgBuf  = this->Adjust( field.fieldStart, newSize, oldSize );
    new_field.Pack( this->proto, &msgBuf[ field.fieldStart ] );
  }
}


void
RaiMsg::Update( RaiMsg_name fname,  RaiMsg *msg_ptr )
{
  RaiField field;
  if ( msg_ptr == NULL || msg_ptr->msgBuf == NULL ) {
    RaiMsg tempMsg( msg_ptr ? msg_ptr->proto : this->proto );
    byte   tempBuf[ 64 ];
    RaiMsg::InitHeader( tempMsg.proto, tempBuf,
                        RaiMsgConst::START_OFF[ tempMsg.proto ] );
    tempMsg.UnPack( tempMsg.proto,  tempBuf, sizeof( tempBuf ),
                    RAIMSG_MEMORY_STATIC, 0 );
    if ( this->proto == tempMsg.proto )
      field.Update( fname, &tempMsg );
    else
      field.Update( fname, RAIMSG_OPAQUE, tempMsg.PackSize(),
                    tempMsg.Packed() );
  }
  else {
    if ( this->proto == msg_ptr->proto )
      field.Update( fname, msg_ptr );
    else
      field.Update( fname, RAIMSG_OPAQUE, msg_ptr->PackSize(),
                    msg_ptr->Packed() );
  }
  this->Update( &field );
}


void
RaiMsg::Update( RaiMsg_name fname,  RaiMsg_data partial_data,
                RaiMsg_size partial_size,  RaiMsg_size offset )

{
  RaiField field;
  field.Update( fname, partial_data, partial_size, offset );
  this->Update( &field );
}


void
RaiMsg::Update( RaiMsg_name fname,  RaiMsg_data array_data,
                RaiMsg_size num_entries,  RaiMsg_type entry_type,
                RaiMsg_size entry_size )
{
  RaiField field;
  field.Update( fname, array_data, num_entries, entry_type, entry_size );
  this->Update( &field );
}


void
RaiMsg::Update( RaiMsg_name fname,  RaiMsg_type ftype,
                RaiMsg_size fsize,  RaiMsg_data fdata )
{
  RaiField field;
  field.Update( fname, ftype, fsize, fdata );
  this->Update( &field );
}


void
RaiMsg::Update( RaiMsg_name fname,  RaiMsg_type ftype,
                RaiMsg_size fsize,  RaiMsg_data fdata,
                RaiMsg_type hint_type,  RaiMsg_size hint_size,
                RaiMsg_data hint_data )
{
  RaiField field;
  field.Update( fname, ftype, fsize, fdata, hint_type, hint_size, hint_data );
  this->Update( &field );
}

#if 0
void
RaiMsg::Update( const RaiMsg_dict *entry,  RaiField *field_ptr )
{
  if ( ! field_ptr->Overlaps( *this ) )
    this->Update2( entry, field_ptr );
  else
    raimsg_copy_field_func2( entry, *field_ptr, this, &RaiMsg::Update2 );
}
#endif

void
RaiMsg::Update( const RaiMsg_dict *entry,  RaiField *field_ptr )

{
  RaiMsg_size off;
  Rai_u16     fid;

  if ( this->isForm() ) {
    off = this->msgStart + entry->foffset;
    if ( this->isDynamic != RAIMSG_MEMORY_FIXED )
      off += RaiMsgConst::START_OFF[ this->proto ];

    if ( off < this->msgStart + this->SubMsgSize() ) {
      Unaligned::endianGetInt( &this->msgBuf[ off ], fid );
      if ( ( fid & ( MAX_FID - 1 ) ) == entry->fid ) {
        if ( entry->partial )
          entry->packPartial( *field_ptr, &this->msgBuf[ off ] );
        else
          entry->pack( *field_ptr, &this->msgBuf[ off ] );
        return;
      }
    }
    if ( entry->foffset != 0 )
      throw RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
  }

  if ( this->isSass() ) {
    RaiField field;
    if ( field.Find( this, entry ) ) {
      this->Replace_SASS( entry, field, *field_ptr );
    }
    else {
      this->Append( entry, field_ptr );
    }
  }
  else {
    RaiField field;
    field.Update( field_ptr );
    field.name    = entry->fname;
    field.nameLen = entry->fname_size;
    field.iterMsg = field_ptr->iterMsg;
    entry->convert( field );
    this->Update( &field );
  }
}


void
RaiMsg::Replace_SASS( const RaiMsg_dict *entry,  RaiField &old_field,
                      RaiField &new_field )
{
  char        tmpBuf[ 128 ];
  RaiMsg_data data;
  Rai_u8    * msgBuf;
  RaiMsg_size partialOff,
              partialLen,
              partialOff2,
              partialLen2,
              off,
              len,
              end,
              end2,
              oldSize,
              newSize;

  if ( entry->partial ) {
    partialOff = old_field.hintSize;
    partialLen = old_field.size;

    if ( partialOff == 0 && partialLen == entry->fsize ) {
      entry->packPartial( new_field, &this->msgBuf[ old_field.fieldStart ] );
    }
    else {
      if ( new_field.type == RAIMSG_PARTIAL ) {
        partialOff2 = new_field.hintSize;
        partialLen2 = new_field.size;
        data        = new_field.data;
      }
      else if ( new_field.type == RAIMSG_STRING ||
                new_field.type == RAIMSG_OPAQUE ) {
        partialOff2 = 0;
        partialLen2 = new_field.size;
        data        = new_field.data;
      }
      else {
        partialOff2 = 0;
        new_field.Get( tmpBuf, sizeof( tmpBuf ) );
        partialLen2 = ::strlen( tmpBuf );
        data        = tmpBuf;
      }

      off  = ( partialOff < partialOff2 ) ? partialOff : partialOff2;
      end  = partialOff + partialLen;
      end2 = partialOff2 + partialLen2;
      len  = ( end > end2 ) ? end : end2;
      len -= off;
      if ( len > (RaiMsg_size) entry->fsize )
        len = (RaiMsg_size) entry->fsize;

      oldSize = old_field.fieldEnd - old_field.fieldStart;
      newSize = 6 + ( ( len + 1U ) & ~1U );

      msgBuf = this->Adjust( old_field.fieldStart, newSize, oldSize );
      msgBuf = &msgBuf[ old_field.fieldStart ];

      partialOff -= off;
      if ( partialOff >= len ) {
        partialOff = len;
        partialLen = 0;
      }
      else {
        if ( partialLen > len - partialOff )
          partialLen = len - partialOff;
        if ( partialOff != 0 )
          ::memmove( &msgBuf[ 6 + partialOff ], &msgBuf[ 6 ], partialLen );
      }

      partialOff2 -= off;
      if ( partialOff2 >= len ) {
        partialOff2 = len;
        partialLen2 = 0;
      }
      else {
        if ( partialLen2 > len - partialOff2 )
          partialLen2 = len - partialOff2;
        ::memcpy( &msgBuf[ 6 + partialOff2 ], data, partialLen2 );
      }

      if ( partialOff < partialOff2 ) {
        partialOff += partialLen;
        if ( partialOff < partialOff2 )
          ::memset( &msgBuf[ 6 + partialOff ], ' ',
                    partialOff2 - partialOff );
      }
      else {
        partialOff2 += partialLen2;
        if ( partialOff2 < partialOff )
          ::memset( &msgBuf[ 6 + partialOff2 ], ' ',
                    partialOff - partialOff2 );
      }
      if ( ( len & 1U ) != 0 )
        msgBuf[ 6 + len ] = 0;

      Unaligned::endianPutInt( (Rai_u16) off, &msgBuf[ 2 ] );
      Unaligned::endianPutInt( (Rai_u16) len, &msgBuf[ 4 ] );
    }
  }
  else {
    entry->pack( new_field, &this->msgBuf[ old_field.fieldStart ] );
  }
}


void
RaiMsg::Update( const RaiMsg_dict *entry,  RaiMsg_data partial_data,
                RaiMsg_size partial_size,  RaiMsg_size offset )

{
  RaiField field;
  field.Update( NULL, partial_data, partial_size, offset );
  this->Update( entry, &field );
}


void
RaiMsg::Update( const RaiMsg_dict *entry,  RaiMsg_data array_data,
                RaiMsg_size num_entries,  RaiMsg_type entry_type,
                RaiMsg_size entry_size )
{
  RaiField field;
  field.Update( NULL, array_data, num_entries, entry_type, entry_size );
  this->Update( entry, &field );
}


void
RaiMsg::Update( const RaiMsg_dict *entry,  RaiMsg_type ftype,
                RaiMsg_size fsize,  RaiMsg_data fdata )
{
  RaiField field;
  field.Update( NULL, ftype, fsize, fdata );
  this->Update( entry, &field );
}


void
RaiMsg::Update( const RaiMsg_dict *entry,  RaiMsg_type ftype,
                RaiMsg_size fsize,  RaiMsg_data fdata,
                RaiMsg_type hint_type,  RaiMsg_size hint_size,
                RaiMsg_data hint_data )
{
  RaiField field;
  field.Update( NULL, ftype, fsize, fdata, hint_type, hint_size, hint_data );
  this->Update( entry, &field );
}


template<class T> bool
RaiMsg::Get( RaiMsg_name fname,  T &arg )
{
  RaiField field;
  if ( this->Get( fname, field ) ) {
    field.Get( arg );
    return true;
  }
  return false;
}
#define INSTANTIATE_GET1( TYPE ) \
  template bool RAIMSG_DLL_EXP \
  RaiMsg::Get( RaiMsg_name fname,  TYPE &arg )

template<class T> bool
RaiMsg::GetEx( RaiMsg_name fname,  Rai_u16 fid,  T &arg )

{
  RaiField field;
  if ( this->GetEx( fname, fid, field ) ) {
    field.Get( arg );
    return true;
  }
  return false;
}
#define INSTANTIATE_GET2( TYPE ) \
  template bool RAIMSG_DLL_EXP \
  RaiMsg::GetEx( RaiMsg_name fname,  Rai_u16 fid,  TYPE &arg )

template<class T> bool
RaiMsg::Get( const RaiMsg_dict *entry,  T &arg )
{
  RaiField field;
  if ( this->Get( entry, field ) ) {
    field.Get( arg );
    return true;
  }
  return false;
}
#define INSTANTIATE_GET3( TYPE ) \
  template bool RAIMSG_DLL_EXP \
  RaiMsg::Get( const RaiMsg_dict *entry,  TYPE &arg )

template<class T> void
RaiMsg::Append( RaiMsg_name fname,  T arg )
{
  RaiField field;
  field.Update( fname, arg );
  this->Append( &field );
}
#define INSTANTIATE_APPEND1( TYPE ) \
  template void RAIMSG_DLL_EXP \
  RaiMsg::Append( RaiMsg_name fname,  TYPE arg )

template<class T> void
RaiMsg::AppendEx( RaiMsg_name fname,  Rai_u16 fid,  T arg )

{
  RaiField    field;
  char        buf[ 256 ];
  RaiMsg_size fnameLen;
  fnameLen = RaiField::MakeFidName( fname, fid, buf );
  field.UpdateEx( buf, fnameLen, arg );
  this->Append( &field );
}
#define INSTANTIATE_APPEND2( TYPE ) \
  template void RAIMSG_DLL_EXP \
  RaiMsg::AppendEx( RaiMsg_name fname,  Rai_u16 fid,  TYPE arg )

template<class T> void
RaiMsg::Append( const RaiMsg_dict *entry, T arg )
{
  RaiField field;
  field.Update( NULL, arg );
  this->Append( entry, &field );
}
#define INSTANTIATE_APPEND3( TYPE ) \
  template void RAIMSG_DLL_EXP \
  RaiMsg::Append( const RaiMsg_dict *entry,  TYPE arg )

template<class T> void
RaiMsg::Update( RaiMsg_name fname,  T arg )
{
  RaiField field;
  field.Update( fname, arg );
  this->Update( &field );
}
#define INSTANTIATE_UPDATE1( TYPE ) \
  template void RAIMSG_DLL_EXP \
  RaiMsg::Update( RaiMsg_name fname,  TYPE arg )

template<class T> void
RaiMsg::UpdateEx( RaiMsg_name fname,  Rai_u16 fid,  T arg )

{
  RaiField    field;
  char        buf[ 256 ];
  RaiMsg_size fnameLen;
  fnameLen = RaiField::MakeFidName( fname, fid, buf );
  field.UpdateEx( buf, fnameLen, arg );
  this->Update( &field );
}
#define INSTANTIATE_UPDATE2( TYPE ) \
  template void RAIMSG_DLL_EXP \
  RaiMsg::UpdateEx( RaiMsg_name fname,  Rai_u16 fid,  TYPE arg )

template<class T> void
RaiMsg::Update( const RaiMsg_dict *entry, T arg )
{
  RaiField field;
  field.Update( NULL, arg );
  this->Update( entry, &field );
}
#define INSTANTIATE_UPDATE3( TYPE ) \
  template void RAIMSG_DLL_EXP \
  RaiMsg::Update( const RaiMsg_dict *entry,  TYPE arg )

#define INSTANTIATE_ALL( D ) \
  D( bool ); D( Rai_u8 ); D( Rai_i8 ); D( Rai_u16 ); D( Rai_i16 ); \
  D( Rai_u32 ); D( Rai_i32 ); D( Rai_u64 ); D( Rai_i64 ); \
  D( Rai_f32 ); D( Rai_f64 ); D( const char * ); \
  D( rai::SassConst::MsgType ); D( rai::SassConst::RecStatus )

#define INSTANTIATE_ALL2( D ) \
  INSTANTIATE_ALL( D ); D( char * )

/*#define INSTANTIATE_ALL3( D ) \
  INSTANTIATE_ALL2( D ); D( RaiField * )*/

INSTANTIATE_ALL( INSTANTIATE_GET1 );
INSTANTIATE_ALL( INSTANTIATE_GET2 );
INSTANTIATE_ALL( INSTANTIATE_GET3 );
INSTANTIATE_ALL2( INSTANTIATE_APPEND1 );
INSTANTIATE_ALL2( INSTANTIATE_APPEND2 );
INSTANTIATE_ALL2( INSTANTIATE_APPEND3 );
INSTANTIATE_ALL2( INSTANTIATE_UPDATE1 );
INSTANTIATE_ALL2( INSTANTIATE_UPDATE2 );
INSTANTIATE_ALL2( INSTANTIATE_UPDATE3 );


void
RaiMsg::AppendEx( RaiMsg_name fname,  Rai_u16 fid,  RaiField *field_ptr )

{
  RaiField field;
  char     buf[ 256 ];

  field.Update( field_ptr );
  field.nameLen = RaiField::MakeFidName( fname, fid, buf );
  field.name    = buf;
  field.iterMsg = field_ptr->iterMsg;

  this->Append( &field );
}


bool
RaiMsg::Activate( RaiMsg_name msg_field_name )
{
  RaiMsg_size    offsetStack[ MAX_MSG_DEPTH ];
  Rai_u32        offsetTop;
  RaiMsg_extra * msgEx,
              ** msgExRef;

  if ( this->msgBuf == NULL )
    return false;

  msgEx = NULL;
  for ( msgExRef = &this->msgEx; *msgExRef != NULL;
        msgExRef = &(*msgExRef)->next ) {
    if ( (*msgExRef)->type == ACTIVATE ) {
      msgEx     = *msgExRef;
      *msgExRef = (*msgExRef)->next;
      break;
    }
  }

  /* restore top level */
  if ( msg_field_name == NULL ) {
    if ( msgEx == NULL )
      return false;
    this->msgStart = msgEx->u.act.offset[ 0 ];
    delete msgEx;
  }
  /* pop up one message level */
  else if ( ::strcmp( msg_field_name, ".." ) == 0 ) {
    if ( msgEx == NULL )
      return false;
    if ( msgEx->u.act.top == 1 ) {
      this->msgStart = msgEx->u.act.offset[ 0 ];
      delete msgEx;
    }
    else {
      this->msgStart = msgEx->u.act.offset[ --msgEx->u.act.top ];
      msgEx->next = this->msgEx;
      this->msgEx = msgEx;
    }
  }
  /* find the last message of the current level (accepts '.' & '.field') */
  else if ( msg_field_name[ 0 ] == '.' && msgEx != NULL ) {
    msgEx->next  = this->msgEx;
    this->msgEx  = msgEx;
    offsetTop = msgEx->u.act.top;
    msgEx->u.act.offset[ msgEx->u.act.top++ ] = this->msgStart;
    if ( ! this->GetMsgOffset( msg_field_name, msgEx->u.act.offset,
                               msgEx->u.act.top, this ) ) {
      msgEx->u.act.top = offsetTop;
      return false;
    }
    this->msgStart = msgEx->u.act.offset[ --msgEx->u.act.top ];
  }
  /* start from current level if already activated */
  else if ( msgEx != NULL ) {
    msgEx->next    = this->msgEx;
    this->msgEx    = msgEx;
    offsetTop      = msgEx->u.act.top;
    msgEx->u.act.offset[ msgEx->u.act.top++ ] = this->msgStart;
    if ( ! this->GetMsgOffset( msg_field_name, msgEx->u.act.offset,
                               msgEx->u.act.top, this ) ) {
      msgEx->u.act.top = offsetTop;
      return false;
    }
    this->msgStart = msgEx->u.act.offset[ --msgEx->u.act.top ];
  }
  /* nothing active, start from top */
  else {
    offsetTop = 0;
    offsetStack[ offsetTop++ ] = this->msgStart;
    if ( ! this->GetMsgOffset( msg_field_name, offsetStack, offsetTop, this ) )
      return false;

    msgEx = NEW RaiMsg_extra( this->msgEx, --offsetTop, offsetStack );
    this->msgEx    = msgEx;
    this->msgStart = offsetStack[ offsetTop ];
  }

  return true;
}


bool
RaiMsg::GetMsgOffset( RaiMsg_name msg_field_name,  RaiMsg_size *offset_stack,
                      Rai_u32 &offset_top,  RaiMsg *msgBase )

{
  RaiMsg_name fieldName,
              ptr;
  Rai_u32     fnameLen;
  RaiField    field;
  RaiMsg    * subMsg;

  if ( offset_top == MAX_MSG_DEPTH )
    return false;
  if ( ! field.First( this ) )
    return false;

  if ( (ptr = ::strchr( msg_field_name, sepChar )) != NULL ) {
    fnameLen = ptr - msg_field_name;
    if ( fnameLen == 0 ) {
      /* . */
      if ( msg_field_name[ fnameLen + 1 ] == '\0' ) {
        offset_stack[ offset_top ] = 0;
        do {
          if ( field.Type() == RAIMSG_MESSAGE ) {
            subMsg = (RaiMsg *) field.Data();
            offset_stack[ offset_top ] = (RaiMsg_size) ( subMsg->msgBuf -
                                                         msgBase->msgBuf );
          }
        } while ( field.Next() );
        if ( offset_stack[ offset_top ] != 0 ) {
          offset_top++;
          return true;
        }
      }
      /* .subfield */
      else {
        do {
          if ( field.Type() == RAIMSG_MESSAGE ) {
            subMsg = (RaiMsg *) field.Data();
            offset_stack[ offset_top++ ] = (RaiMsg_size) ( subMsg->msgBuf -
                                                           msgBase->msgBuf );
            if ( subMsg->GetMsgOffset( &msg_field_name[ fnameLen + 1 ],
                                       offset_stack, offset_top, msgBase ) )
              return true;
            --offset_top;
          }
        } while ( field.Next() );
      }
    }
    /* field.subfield */
    else {
      do {
        fieldName = field.Name();
        if ( fieldName != NULL &&
             ::strncmp( fieldName, msg_field_name, fnameLen ) == 0 &&
             fieldName[ fnameLen ] == '\0' &&
             field.Type() == RAIMSG_MESSAGE ) {

          subMsg = (RaiMsg *) field.Data();
          offset_stack[ offset_top++ ] = (RaiMsg_size) ( subMsg->msgBuf -
                                                         msgBase->msgBuf );
          if ( subMsg->GetMsgOffset( &msg_field_name[ fnameLen + 1 ],
                                     offset_stack, offset_top, msgBase ) )
            return true;
          --offset_top;
        }
      } while ( field.Next() );
    }
  }
  /* field */
  else {
    do {
      fieldName = field.Name();
      if ( fieldName != NULL &&
           ::strcmp( fieldName, msg_field_name ) == 0 &&
           field.Type() == RAIMSG_MESSAGE ) {
        subMsg = (RaiMsg *) field.Data();
        offset_stack[ offset_top++ ] = (RaiMsg_size) ( subMsg->msgBuf -
                                                       msgBase->msgBuf );
        return true;
      }
    } while ( field.Next() );
  }

  return false;
}


bool
RaiMsg::Rename( RaiMsg_name old_fname, RaiMsg_name new_fname )

{
  Rai_u8    * msgBuf;
  RaiMsg_size newSize,
              oldSize;
  RaiField    field;

  if ( this->msgBuf == NULL )
    return false;
  if ( new_fname == NULL || old_fname == NULL || ::strlen( new_fname ) > 254 )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
  if ( ! field.Find( this, old_fname ) )
    return false;

  newSize = ::strlen( new_fname ) + 2;
  oldSize = ::strlen( old_fname ) + 2;
  msgBuf  = this->Adjust( field.fieldStart, newSize, oldSize );
  if ( msgBuf != NULL ) {
    msgBuf[ field.fieldStart ] = (Rai_u8) ( newSize - 1 );
    ::strcpy( (char *) &msgBuf[ field.fieldStart + 1 ], new_fname );
  }

  return true;
}


bool
RaiMsg::Remove( RaiMsg_name fname )
{
  RaiMsg_size oldSize;
  RaiField    field;

  if ( this->msgBuf == NULL )
    return false;
  if ( fname == NULL )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
  if ( ! field.Find( this, fname ) )
    return false;

  oldSize = field.fieldEnd - field.fieldStart;
  this->Adjust( field.fieldStart, 0, oldSize );

  return true;
}


void
RaiMsg::UnPack( RaiMsg_data from_ptr )
{
  this->UnPack( from_ptr, 0xffffffffU );
}


void
RaiMsg::UnPack( RaiMsg_data from_ptr,  RaiMsg_size from_size )

{
  RaiMsg_protocol proto;
  RaiMsg_size start = 0;

  if ( ! RaiMsg::ExtractProtocol( from_ptr, from_size, proto ) ) {
    /* check for mama encapsulation */
    if ( from_size < 1 || ((byte *) from_ptr)[ 0 ] != 'I' ||
         ! RaiMsg::ExtractProtocol( &((byte *) from_ptr)[ 1 ], from_size - 1,
                                    proto ) )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_MAGIC_NUMBER );
    start = 1;
  }
  this->UnPack( proto, from_ptr, from_size, RAIMSG_MEMORY_STATIC, start );
}


bool
RaiMsg::ExtractProtocol( RaiMsg_data from_ptr,  RaiMsg_size from_size,
                         RaiMsg_protocol &proto )
{
  Rai_u32 magic,
          off,
          i;

  for ( i = 0; i < RaiMsgConst::MAX_PROTO; i++ ) {
    if ( RaiMsgConst::MAGIC_NUM[ i ] != 0 ) {
      off = RaiMsgConst::HDR_SIZE[ i ] + RaiMsgConst::MAGIC_OFF[ i ];
      if ( off + sizeof( magic ) <= from_size ) {
        Unaligned::endianGetInt( &((Rai_u8 *) from_ptr)[ off ], magic );
        if ( magic == RaiMsgConst::MAGIC_NUM[ i ] ) {
          proto = (RaiMsg_protocol) i;
          return true;
        }
      }
    }
  }
  return false;
}


static const Rai_u8 tibrv_encap_data_[]  = "\007_data_\000\007",
                    tibrv_encap_QFORM[]  = "\007_QFORM\000\007",
                    tibrv_encap_RAIMSG[] = "\010_RAIMSG\000\007",
                    tibrv_encap_TIBMSG[] = "\010_TIBMSG\000\007";
void
RaiMsg::TibrvEncapsulate( void )
{
  RaiMsg_size rvSize, hdrSize, fnameSize, newSize, off;
  const byte * fname;

  switch ( this->proto ) {
    case RV_PROTO:
      return;

    default:
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_PROTO );

    case RAIMSG_PROTO:
      fname = tibrv_encap_TIBMSG;
      fnameSize = hdrSize = sizeof( tibrv_encap_TIBMSG ) - 1;
      break;

    case CI_SASS_PROTO:
    case CI_SASS_FORM_PROTO:
    case TIB_SASS_PROTO:
    case TIB_SASS_FORM_PROTO:
      fname = tibrv_encap_QFORM;
      fnameSize = hdrSize = sizeof( tibrv_encap_QFORM ) - 1;
      break;
  }

  rvSize = this->PackSize();
  if ( rvSize < RAI_RV_TINY_SIZE )
    hdrSize += 1;
  else if ( rvSize < MAX_RV_SHORT_SIZE )
    hdrSize += 3;
  else
    hdrSize += 5;

  off     = RaiMsgConst::HDR_BYTES[ RV_PROTO ];
  newSize = off + hdrSize + rvSize;
  if ( newSize > this->msgSize ) {
    if ( this->isDynamic == RAIMSG_MEMORY_DYNAMIC ) {
      REALLOC( newSize, &this->msgBuf );
      this->msgSize = newSize;
    }
    else
      throw RaiMsgErr::getErr( RaiMsgErr::MEMORY_STATIC_ERR );
  }
  ::memmove( &this->msgBuf[ off + hdrSize ], this->msgBuf, rvSize );
  initHeader( RV_PROTO, this->msgBuf, newSize );
  ::memcpy( &this->msgBuf[ off ], fname, fnameSize );
  off += fnameSize;

  if ( rvSize < RAI_RV_TINY_SIZE ) {
    this->msgBuf[ off ] = (Rai_u8) rvSize;
  }
  else if ( rvSize < MAX_RV_SHORT_SIZE ) {
    this->msgBuf[ off ] = (Rai_u8) RAI_RV_SHORT_SIZE;
    Unaligned::endianPutInt( (Rai_u16) ( rvSize + 2 ),
                             (Rai_u8 *) &this->msgBuf[ off + 1 ] );
  }
  else {
    this->msgBuf[ off ] = (Rai_u8) RAI_RV_LONG_SIZE;
    Unaligned::endianPutInt( (Rai_u32) ( rvSize + 4 ),
                             (Rai_u8 *) &this->msgBuf[ off + 1 ] );
  }
  this->proto = RV_PROTO;
  this->msgStart = RaiMsgConst::HDR_SIZE[ RV_PROTO ];
}

bool
RaiMsg::ExtractProtocolEx( RaiMsg_data from_ptr,  RaiMsg_size from_size,
                           RaiMsg_protocol &proto,  RaiMsg_size &msgOff )
{
  RaiMsg_protocol proto2;
  RaiMsg_size     off,
                  szLen,
                  msgSize;
  Rai_u32         dataLen;
  Rai_u16         shortLen;
  const Rai_u8 *  ptr;

  msgOff = 0;
check_mama_encap:;
  if ( from_size > msgOff ) {
    if ( RaiMsg::ExtractProtocol( &((byte *) from_ptr)[ msgOff ],
                                  from_size - msgOff, proto ) ) {
      if ( proto == RV_PROTO ) {
        bool isQform = false;
        ptr      = (const Rai_u8 *) from_ptr;
        Unaligned::endianGetInt( &ptr[ msgOff +
                                      RaiMsgConst::SIZE_OFF[ proto ] ], msgSize );
        off = RaiMsgConst::HDR_BYTES[ RV_PROTO ];
        /* check for _data_ rv encapsulation */
        if ( from_size > off + sizeof( tibrv_encap_data_ ) + 1 &&
             ( ::memcmp( &ptr[ off ], tibrv_encap_data_, 9 ) == 0 ||
               ( isQform = ( ::memcmp( &ptr[ off ],
                             tibrv_encap_QFORM, 9 ) == 0 ) ) ) ) {
          off += 9;
        raimsg_rv_encap:;
          if ( ptr[ off ] < RAI_RV_TINY_SIZE ) {
            dataLen = (Rai_u32) ptr[ off++ ];
            szLen = 0;
          }
          else if ( ptr[ off ] == RAI_RV_SHORT_SIZE ) {
            Unaligned::endianGetInt( &ptr[ ++off ], shortLen );
            dataLen = (Rai_u32) shortLen;
            szLen = 2;
          }
          else if ( ptr[ off ] == RAI_RV_LONG_SIZE ) {
            Unaligned::endianGetInt( &ptr[ ++off ], dataLen );
            szLen = 4;
          }
          else {
            return true;
          }

          if ( off + dataLen == msgSize &&
               off + dataLen <= from_size ) {
            off += szLen;
            if ( ExtractProtocol( (RaiMsg_data) &ptr[ off ], from_size - off,
                                  proto2 ) ) {
              proto  = proto2;
              msgOff = off;
            }
            else if ( isQform ) {
              proto  = CI_SASS_PROTO;
              msgOff = off;
            }
          }
        }
        /* check for _RAIMSG/_TIBMSG rv encapsulation */
        else if ( from_size > off + sizeof( tibrv_encap_RAIMSG ) + 1 &&
                  ( ::memcmp( &ptr[ off ], tibrv_encap_TIBMSG, 10 ) == 0 ||
                    ::memcmp( &ptr[ off ], tibrv_encap_RAIMSG, 10 ) == 0 ) ) {
          off += 10;
          goto raimsg_rv_encap;
        }
      }
      return true;
    }
    /* check for Mama encapsulation */
    else if ( ((byte *) from_ptr )[ msgOff ] == 'I' ) {
      msgOff++;
      goto check_mama_encap;
    }
  }
  return false;
}


void
RaiMsg::UnPack( RaiMsg_protocol proto,  RaiMsg_data from_ptr,
                RaiMsg_size from_size,  RaiMsg_memory memKind )

{
  this->UnPack( proto, from_ptr, from_size, memKind, 0 );
}


void
RaiMsg::UnPack( RaiMsg_protocol proto,  RaiMsg_data from_ptr,
                RaiMsg_size from_size,  RaiMsg_memory memKind,
                RaiMsg_size from_off )
{
  Rai_u32     magic;
  Rai_u16     u16;
  RaiMsg_size msgSize,
              msgStart;

  if ( ! RaiMsg::isValidProto( proto ) )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_PROTO );

  if ( memKind == RAIMSG_MEMORY_FIXED ) {
    msgStart = from_off;
    msgSize  = from_size;
  }
  else {
    if ( memKind == RAIMSG_MEMORY_MAMA ) {
      from_off++;
      memKind = RAIMSG_MEMORY_DYNAMIC;
    }
    else if ( from_off < from_size && ((byte *) from_ptr)[ from_off ] == 'I' ) {
      from_off++;
    }
    if ( from_size < RaiMsgConst::HDR_BYTES[ proto ] )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_MAGIC_NUMBER );

    msgStart = RaiMsgConst::HDR_SIZE[ proto ] + from_off;

    if ( RaiMsgConst::MAGIC_NUM[ proto ] != 0 ) {
      Unaligned::endianGetInt(
        &((Rai_u8 *) from_ptr)[ msgStart + RaiMsgConst::MAGIC_OFF[ proto ] ],
          magic );
      if ( magic != RaiMsgConst::MAGIC_NUM[ proto ] )
        throw RaiMsgErr::getErr( RaiMsgErr::BASE_PROTO_ERR + proto );
    }

    if ( RaiMsgConst::SIZE_LEN[ proto ] == 2 ) {
      Unaligned::endianGetInt(
       &((Rai_u8 *) from_ptr)[ msgStart + RaiMsgConst::SIZE_OFF[ proto ] ],
         u16 );
      msgSize = (RaiMsg_size) u16;
    }
    else {
      Unaligned::endianGetInt(
       &((Rai_u8 *) from_ptr)[ msgStart + RaiMsgConst::SIZE_OFF[ proto ] ],
         msgSize );
    }

    if ( msgStart + msgSize > from_size )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_UNPACK_BUF_SZ );
  }

  /* free old message, if dynamic */
  if ( this->msgBuf != NULL ) {
    if ( this->isDynamic == RAIMSG_MEMORY_DYNAMIC )
      FREE( this->msgBuf );
    if ( this->msgEx != NULL )
      this->ReleaseExtra();
  }
  this->proto     = proto;
  this->msgBuf    = (Rai_u8 *) from_ptr;
  this->msgSize   = msgSize + msgStart;
  this->msgStart  = msgStart;
  this->parent    = NULL;
  this->msgEx     = NULL;
  this->isDynamic = memKind;
}


void
RaiMsg::Pack( RaiMsg_data to_ptr ) const
{
  RaiMsg_size msgSize;

  if ( (msgSize = this->PackSize()) == 0 )
    throw RaiMsgErr::getErr( RaiMsgErr::NO_FIELD );
  ::memcpy( to_ptr, this->msgBuf, msgSize );
}


RaiMsg_size
RaiMsg::PackSize( void ) const
{
  if ( this->msgBuf == NULL )
    return 0;

  if ( ! RaiMsg::isValidProto( this->proto ) )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_PROTO );

  return this->msgStart + this->SubMsgSize();
}


RaiMsg_size
RaiMsg::SubMsgSize( void ) const
{
  RaiMsg_size msgSize;
  Rai_u16     u16;

  if ( this->msgBuf == NULL )
    return 0;

  if ( this->isDynamic == RAIMSG_MEMORY_FIXED ||
       this->msgStart < RaiMsgConst::HDR_SIZE[ this->proto ] )
    msgSize = this->msgSize;
  else {
    if ( RaiMsgConst::SIZE_LEN[ this->proto ] == 2 ) {
      Unaligned::endianGetInt(
          (Rai_u8 *) &this->msgBuf[ this->msgStart +
                                  RaiMsgConst::SIZE_OFF[ this->proto ] ], u16 );
      msgSize = (RaiMsg_size) u16;
    }
    else {
      Unaligned::endianGetInt(
          (Rai_u8 *) &this->msgBuf[ this->msgStart +
                              RaiMsgConst::SIZE_OFF[ this->proto ] ], msgSize );
    }
  }

  return msgSize;
}


RaiMsg_size
RaiMsg::SubMsgOff( void ) const
{
  if ( this->msgBuf == NULL )
    return 0;

  if ( this->isDynamic == RAIMSG_MEMORY_FIXED )
    return this->msgStart;
  return this->msgStart + RaiMsgConst::START_OFF[ this->proto ];
}


const RaiMsg_data
RaiMsg::Packed( void ) const
{
  if ( this->msgBuf == NULL )
    throw RaiMsgErr::getErr( RaiMsgErr::NO_FIELD );
  return this->msgBuf;
}


void
RaiMsg::Print( OutputStream *output_file,  Rai_u32 field_newlines,
               const char *fname_format,  Rai_u32 print_opaques,
               const char *debug_format,  const char *debug_hformat )

{
  RaiField field;

  if ( field.First( this ) ) {
    do {
      field.Print( output_file, field_newlines, fname_format, print_opaques,
                   debug_format, debug_hformat );
    } while ( field.Next() );
  }
}


void
RaiMsg::PrintXML( OutputStream *output_file,  Rai_u32 attr_flags,
                  Rai_u32 field_newlines,  const char *msg_name,
                  const char **msg_atts )
{
  RaiField     field;
  unsigned int i;

  if ( msg_name != NULL ) {
    output_file->printf( "%*s<%s",
              ( field_newlines <= 1 ? 0 : field_newlines - 1 ), "", msg_name );
    if ( msg_atts != NULL ) {
      for ( i = 0; msg_atts[ i ] != NULL && msg_atts[ i + 1 ] != NULL;
            i += 2 ) {
        output_file->printf( " %s=\"%s\"", msg_atts[ i ], msg_atts[ i + 1 ] );
      }
    }
    else {
      if ( ( attr_flags & ADD_TYPE_ATTR ) != 0 )
        output_file->printf( " typ=\"%s\"", RaiMsg::TypeStr( RAIMSG_MESSAGE ) );
      if ( ( attr_flags & ADD_SIZE_ATTR ) != 0 )
        output_file->printf( " siz=\"%u\"", this->SubMsgSize() );
    }
    output_file->printf( ">%s",
              ( field_newlines == 0 ? "" : "\n" ) );
  }
  if ( field_newlines >= 1 )
    field_newlines += 4;

  if ( field.First( this ) ) {
    do {
      field.PrintXML( output_file, attr_flags, field_newlines );
    } while ( field.Next() );
  }

  if ( field_newlines >= 1 )
    field_newlines -= 4;
  if ( msg_name != NULL )
    output_file->printf( "%*s</%s>%s",
              ( field_newlines <= 1 ? 0 : field_newlines - 1 ), "", msg_name,
              ( field_newlines == 0 ? "" : "\n" ) );
}


void
RaiMsg::PrintHex( OutputStream *output_file )
{
  Rai_u8    * msg;
  RaiMsg_size msgSize;

  msgSize = this->PackSize();
  msg     = (Rai_u8 *) this->Packed();
  RaiMsg::PrintHex( output_file, msg, msgSize, 0 );
}


void
RaiMsg::PrintHex( OutputStream *output_file,  Rai_u8 *msg,
                  RaiMsg_size msgSize,  RaiMsg_size offset )

{
  static const char hexChars[] = "0123456789abcdef";
  RaiMsg_size i, j, k, l, m;
  char        line[ 80 ];

  ::strcpy( line, "     0:  " );
  for ( j = 5, k = offset; k > 0; ) {
    line[ j ] = hexChars[ k & 0xf ];
    if ( j-- == 0 )
      break;
    k >>= 4;
  }
  for ( i = 0; i < msgSize; ) {
    k = 9;
    l = 61;
    m = i;
    for ( j = 0; j < 16 && m < msgSize; m++ ) {
      line[ k++ ] = hexChars[ msg[ m ] >> 4 ];
      line[ k++ ] = hexChars[ msg[ m ] & 0xf ];
      line[ k++ ] = ' ';
      line[ l++ ] = ( msg[ m ] >= ' ' && msg[ m ] <= 127 ) ? msg[ m ] : '.';
      if ( ( ++j & 0x3 ) == 0 )
        line[ k++ ] = ' ';
    }
    while ( k < 61 )
      line[ k++ ] = ' ';
    line[ l++ ] = '\n';
    output_file->writeBytes( (Rai_u8 *) line, l );
    ::strcpy( line, "     0:  " );
    i += 16;
    k  = i + offset;
    for ( j = 5; k > 0; ) {
      line[ j ] = hexChars[ k & 0xf ];
      if ( j-- == 0 )
        break;
      k >>= 4;
    }
  }
  if ( msgSize > 0 ) {
    ::strcpy( line, "     0:\n" );
    msgSize += offset;
    for ( j = 5; msgSize > 0; ) {
      line[ j ] = hexChars[ msgSize & 0xf ];
      if ( j-- == 0 )
        break;
      msgSize >>= 4;
    }
    output_file->writeBytes( (Rai_u8 *) line, 8 );
  }
}


void
RaiMsg::Read( InputStream *f,  OutputStream *err )
{
  unsigned int  i,
                j,
                k,
                lineno;
  char          buf[ 1024 ],
              * tokens[ 256 ],
              * end,
              * fname;
  RaiMsg_type   ftype,
                hintType;
  RaiMsg_size   fsize,
                hintSize,
                arraySize;
  RaiMsg_data   fdata,
                hintData,
                dataPtr;
  RaiField_data val,
                hintVal;

  for ( lineno = 1; f->gets( buf, sizeof( buf ) ) > 0; lineno++ ) {
    for ( i = 0; isspace( buf[ i ] ) && buf[ i ] != '\0'; i++ )
      ;
    if ( buf[ i ] == '/' || buf[ i ] == '#' ) /* comment */
      continue;

    for ( j = 0; j < 256; ) {
      if ( buf[ i ] == '\0' )
        break;
      tokens[ j++ ] = &buf[ i ];
      if ( buf[ i ] == '\"' ) {
        for ( i++; buf[ i ] != '\"' && buf[ i ] != '\0'; i++ )
          ;
        if ( buf[ i ] != '\"' )
          break;
        i++;
        if ( buf[ i ] == '\0' )
          break;
        buf[ i++ ] = '\0';
      }
      else {
        while ( ! isspace( buf[ i ] ) && buf[ i ] != '\0' )
          i++;
        if ( buf[ i ] == '\0' )
          break;
        buf[ i++ ] = '\0';
      }
      while ( isspace( buf[ i ] ) )
        i++;
    }

    if ( j == 0 )
      continue;

    if ( tokens[ 0 ][ 0 ] == '{' && tokens[ 0 ][ 1 ] == '\0' ) {
      if ( j == 2 ) {
        this->Append( tokens[ 1 ], (RaiMsg *) NULL );
        if ( ! this->Activate( "." ) ) {
          err->printf( "Line %u: Unable to append entry\n", lineno );
          throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
        }
      }
      else {
        this->Append( (RaiMsg_name) NULL, (RaiMsg *) NULL );
        if ( ! this->Activate( "." ) ) {
          err->printf( "Line %u: Unable to append entry\n", lineno );
          throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
        }
      }
    }
    else if ( j == 1 && tokens[ 0 ][ 0 ] == '}' &&
                        tokens[ 0 ][ 1 ] == '\0' ) {
      if ( ! this->Activate( ".." ) ) {
        err->printf( "Line %u: Unable to end message\n", lineno );
        throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
      }
    }
    else if ( j >= 4 ) {

      ftype = RaiMsg::StrType( tokens[ 1 ] );
      if ( ftype == RAIMSG_NODATA ) {
        err->printf( "Line %u: Bad type: \"%s\"\n", lineno, tokens[ 1 ] );
        throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
      }

      fsize = (RaiMsg_size) ::strtoul( tokens[ 2 ], &end, 0 );
      if ( end == tokens[ 2 ] ) {
        err->printf( "Line %u: Bad size: \"%s\"\n", lineno, tokens[ 2 ] );
        throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
      }

      if ( ftype == RAIMSG_STRING || ftype == RAIMSG_OPAQUE ) {
        if ( j > 4 ) {
          for ( i = 4; i < j; i++ ) {
            if ( ::strcmp( tokens[ i ], "_hint" ) == 0 )
              break;
            tokens[ i - 1 ][ ::strlen( tokens[ i - 1 ] ) ] = ' ';
          }
          if ( i > 4 ) {
            for ( k = 4; i < j; )
              tokens[ k++ ] = tokens[ i++ ];
            j = k;
          }
        }
        if ( ::strcmp( tokens[ 3 ], "null" ) == 0 )
          fsize = 0;
        else if ( fsize == 0 )
          fsize = ::strlen( tokens[ 3 ] ) + 1;
      }

      hintType = RAIMSG_NODATA;
      hintSize = 0;
      hintData = NULL;

      for ( i = 4; i < j; i++ ) {
        if ( ::strcmp( tokens[ i ], "_hint" ) == 0 )
          break;
      }

      if ( i + 1 < j ) {
        hintType = RaiMsg::StrType( tokens[ i + 1 ] );
        if ( hintType == RAIMSG_NODATA ) {
          err->printf( "Line %u: Bad hint type: \"%s\"\n", lineno,
                       tokens[ i + 1 ] );
          throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
        }

        if ( i + 2 < j ) {
          hintSize = (RaiMsg_size) ::strtoul( tokens[ i + 2 ], &end, 0 );

          if ( end == tokens[ i + 2 ] ) {
            err->printf( "Line %u: Bad hint size: \"%s\"\n", lineno,
                         tokens[ i + 2 ] );
            throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
          }

          if ( i + 3 < j ) {
            if ( hintType == RAIMSG_STRING || hintType == RAIMSG_OPAQUE ) {
              if ( i + 4 < j ) {
                for ( k = i + 4; k < j; k++ )
                  tokens[ k - 1 ][ ::strlen( tokens[ k - 1 ] ) ] = ' ';
                j = i + 4;
              }

              if ( ::strcmp( tokens[ i + 3 ], "null" ) == 0 )
                hintSize = 0;
              else if ( hintSize == 0 )
                hintSize = ::strlen( tokens[ i + 3 ] ) + 1;
            }
          }
        }
      }

      if ( ::strcmp( tokens[ 0 ], "null" ) == 0 )
        fname = NULL;
      else
        fname = tokens[ 0 ];

      if ( ftype != RAIMSG_ARRAY && ftype != RAIMSG_PARTIAL &&
           hintType != RAIMSG_NODATA ) {
        switch ( hintType ) {
          case RAIMSG_IPDATA:
          case RAIMSG_INT:
          case RAIMSG_UINT:
          case RAIMSG_REAL:
          case RAIMSG_BOOLEAN:
            if ( i + 3 < j ) {
              try {
                RaiField::Convert( hintType, hintSize, &hintVal, RAIMSG_STRING,
                             ::strlen( tokens[ i + 3 ] ) + 1, tokens[ i + 3 ] );
                hintData = (RaiMsg_data) &hintVal;
              } catch ( ... ) {
                err->printf( "Line %u: Unable to convert \"%s\" to %s\n",
                             lineno, tokens[ i + 3 ], tokens[ i + 1 ] );
                throw;
              }
            }
            else {
              err->printf( "Line %u: Expecting data with hint type %s\n",
                           lineno, tokens[ i + 1 ] );
              throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
            }
            break;
          case RAIMSG_STRING:
          case RAIMSG_OPAQUE:
            if ( i + 3 < j ) {
              hintData = tokens[ i + 3 ];
            }
            else {
              err->printf( "Line %u: Expecting data with hint type %s\n",
                           lineno, tokens[ i + 1 ] );
              throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
            }
            break;
          default:
            err->printf( "Line %u: Invalid hint type %s\n",
                         lineno, tokens[ i + 1 ] );
            throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
        }
      }

      try {
        switch ( ftype ) {
          case RAIMSG_IPDATA:
          case RAIMSG_INT:
          case RAIMSG_UINT:
          case RAIMSG_REAL:
          case RAIMSG_BOOLEAN:
            try {
              RaiField::Convert( ftype, fsize, &val, RAIMSG_STRING,
                                 ::strlen( tokens[ 3 ] ) + 1, tokens[ 3 ] );
              fdata = (RaiMsg_data) &val;
            } catch ( ... ) {
              err->printf( "Line %u: Unable to convert \"%s\" to %s\n",
                           lineno, tokens[ 3 ], tokens[ 1 ] );
              throw;
            }
            if ( hintType == RAIMSG_NODATA )
              this->Append( fname, ftype, fsize, fdata );
            else
              this->Append( fname, ftype, fsize, fdata, hintType, hintSize,
                            hintData );
            break;
          case RAIMSG_STRING:
          case RAIMSG_OPAQUE:
            fdata = tokens[ 3 ];
            if ( hintType == RAIMSG_NODATA )
              this->Append( fname, ftype, fsize, fdata );
            else
              this->Append( fname, ftype, fsize, fdata, hintType, hintSize,
                            hintData );
            break;
          case RAIMSG_PARTIAL:
            if ( this->proto != RAIMSG_PROTO )
              break;
            fdata = tokens[ 3 ];
            this->Append( fname, fdata, fsize, hintSize );
            break;
          case RAIMSG_ARRAY:
            if ( this->proto != RAIMSG_PROTO )
              break;
            if ( ! RaiField::isValidArrayType( hintType, hintSize ) ) {
              err->printf( "Line %u: Bad array type %s size %u\n",
                           lineno, RaiMsg::TypeStr( hintType ), hintSize );
              throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
            }
            if ( fsize == 0 )
              fdata = NULL;
            else {
              arraySize = fsize * hintSize;
              MALLOC( arraySize, &fdata );
              dataPtr = fdata;
              for ( k = 3; k < i; k++ ) {
                try {
                  RaiField::Convert( hintType, hintSize, dataPtr,
                                     RAIMSG_STRING, ::strlen( tokens[ k ] ) + 1,
                                     tokens[ k ] );
                  dataPtr = (RaiMsg_data) &((char *) dataPtr)[ hintSize ];
                } catch ( ... ) {
                  err->printf( "Line %u: Unable to convert \"%s\" to %s\n",
                             lineno, tokens[ k ], RaiMsg::TypeStr( hintType ) );
                  FREE( fdata );
                  throw;
                }
              }
              if ( (RaiMsg_size) ( (char *) dataPtr -
                                   (char *) fdata ) != arraySize ) {
                err->printf( "Line %u: Number of elements mismatch\n", lineno );
                FREE( fdata );
                throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
              }
            }
            this->Append( fname, fdata, fsize, hintType, hintSize );
            if ( fdata != NULL )
              FREE( fdata );
            break;
          default:
            break;
        }
      } catch ( ... ) {
        err->printf( "Line %u: Unable to append entry\n", lineno );
        throw;
      }
    }
    else {
      err->printf( "Line %u: Can't parse tokens\n", lineno );
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_READ_FILE );
    }
  }
}


void
RaiMsg::Read( const char *filename,  OutputStream *err )

{
  InputStream * fp;

  fp = FileInputStream::open( filename );
  try {
    this->Read( fp, err );
    fp->close();
  } catch ( ... ) {
    delete fp;
    throw;
  }
  delete fp;
}

RaiMsg_config *rai::DataDictionary;
RaiMsg        *rai::PackedDataDictionaryFull;

void
RaiMsg::SetDataDictionary( RaiMsg_config *dictionary )
{
  DataDictionary = dictionary;
  PackedDataDictionaryFull = NULL;
}


RaiMsg_config *
RaiMsg::GetDataDictionary( void )
{
  return DataDictionary;
}


void
RaiMsg::ReadDataDictionary( const char *tss_fields_fname,
                            const char *tss_records_fname )

{
  DataDictionary = RaiMsg_config::parseDictionary( tss_fields_fname,
                                                   tss_records_fname );
}


bool
RaiMsg::GetDictEntry( const char *fname,  const RaiMsg_dict *&entry )
{
  if ( DataDictionary != NULL ) {
    if ( (entry = DataDictionary->getEntry( fname )) != NULL )
      return true;
  }
  return false;
}


bool
RaiMsg::GetDictEntry( Rai_u16 fid,  const RaiMsg_dict *&entry )
{
  if ( DataDictionary != NULL ) {
    if ( (entry = DataDictionary->getEntry( fid )) != NULL )
      return true;
  }
  return false;
}


bool
RaiMsg::GetDictForm( const char *fname,  const RaiMsg_form *&form )
{
  if ( DataDictionary != NULL ) {
    if ( (form = DataDictionary->getForm( fname )) != NULL )
      return true;
  }
  return false;
}


bool
RaiMsg::GetDictForm( Rai_u16 fid,  const RaiMsg_form *&form )
{
  if ( DataDictionary != NULL ) {
    if ( (form = DataDictionary->getForm( fid )) != NULL )
      return true;
  }
  return false;
}


RaiMsg_type
RaiMsg::StrType( const char *type_str )
{
  if ( type_str == NULL )
    return RAIMSG_NODATA;
  switch ( type_str[ 0 ] ) {
    case 'A':
      return ::strcmp( type_str, "ARRAY" ) == 0   ? RAIMSG_ARRAY :
                                                    RAIMSG_MAXVALID;
    case 'B':
      return ::strcmp( type_str, "BOOLEAN" ) == 0 ? RAIMSG_BOOLEAN :
                                                    RAIMSG_MAXVALID;
    case 'I':
      return ::strcmp( type_str, "INT" ) == 0     ? RAIMSG_INT : 
             ::strcmp( type_str, "IPDATA" ) == 0  ? RAIMSG_IPDATA :
                                                    RAIMSG_MAXVALID;
    case 'M':
      return ::strcmp( type_str, "MESSAGE" ) == 0 ? RAIMSG_MESSAGE :
                                                    RAIMSG_MAXVALID;
    case 'N':
      return ::strcmp( type_str, "NODATA" ) == 0  ? RAIMSG_NODATA :
                                                    RAIMSG_MAXVALID;
    case 'O':
      return ::strcmp( type_str, "OPAQUE" ) == 0  ? RAIMSG_OPAQUE :
                                                    RAIMSG_MAXVALID;
    case 'P':
      return ::strcmp( type_str, "PARTIAL" ) == 0 ? RAIMSG_PARTIAL :
                                                    RAIMSG_MAXVALID;
    case 'R':
      return ::strcmp( type_str, "REAL" ) == 0    ? RAIMSG_REAL :
                                                    RAIMSG_MAXVALID;
    case 'S':
      return ::strcmp( type_str, "STRING" ) == 0  ? RAIMSG_STRING :
                                                    RAIMSG_MAXVALID;
    case 'U':
      return ::strcmp( type_str, "UINT" ) == 0    ? RAIMSG_UINT :
                                                    RAIMSG_MAXVALID;
    default:
      return RAIMSG_MAXVALID;
  }
}


const char *
RaiMsg::TypeStr( RaiMsg_type type )
{
  static const char *types[] = {
    /*  0 */ "NODATA",
    /*  1 */ "MESSAGE",
    /*  2 */ "STRING",
    /*  3 */ "OPAQUE",
    /*  4 */ "BOOLEAN",
    /*  5 */ "INT",
    /*  6 */ "UINT",
    /*  7 */ "REAL",
    /*  8 */ "ARRAY",
    /*  9 */ "PARTIAL",
    /* 10 */ "IPDATA",
    /* 11 */ "INVALID"
  };

  if ( type >= RAIMSG_NODATA && type <= RAIMSG_MAXVALID )
    return types[ type ];
  return types[ RAIMSG_MAXVALID ];
}


RaiMsgException
RaiMsgErr::getErr( unsigned int status )
{
  static const char     mod[] = "RaiMsg";
  static const ErrorRec err[] = {
  /*  0 */ { OK,                "Ok", mod },
  /*  1 */ { BAD_ARG,           "An invalid or null argument was provided to "
                                "method", mod },
  /*  2 */ { BAD_MAGIC_NUMBER,  "Message or field was not created by RaiMsg "
                                "methods", mod },
  /*  3 */ { VERSION_MISMATCH,  "Version of message or field does not match",
                                mod },
  /*  4 */ { MEMORY_STATIC_ERR, "Attempted to update a static "
                                "message or static buffer too small", mod },
  /*  5 */ { NO_MEMORY,         "Insufficient memory for the operation", mod },
  /*  6 */ { BAD_READ_FILE,     "Unable to open or read specified input file",
                                mod },
  /*  7 */ { BAD_READ_SYNTAX,   "Input file does not contain correct message "
                                "syntax", mod },
  /*  8 */ { RESERVED,          "Unknown", mod },
  /*  9 */ { NOT_FOUND,         "The specified field was not found", mod },
  /* 10 */ { APPLY_ERROR,       "Callback function returned non-zero during "
                                "Apply", mod },
  /* 11 */ { NO_FIELD,          "Cound not access first/next field of message",
                                mod },
  /* 12 */ { BAD_TSS_PARTIAL,   "Partial data type not string or opaque", mod },
  /* 13 */ { BAD_TSS_DATETIME,  "Dict expecting 6 byte opaque TSS date/time",
                                mod },
  /* 14 */ { BAD_TSS_GROCERY,   "Dict expecting 4 or 8 byte real TSS grocery",
                                mod },
  /* 15 */ { BAD_TSS_GROCERY2,  "Dict expecting TSS grocery fraction of int "
                                "type", mod },
  /* 16 */ { BAD_TSS_PRICE,     "Dict expecting 4 or 8 byte TSS real/price",
                                mod },
  /* 17 */ { BAD_TSS_INTEGER,   "Dict expecting 1,2,4 or 8 byte TSS integer",
                                mod },
  /* 18 */ { BAD_TSS_TYPE,      "Dict found invalid TSS type", mod },
  /* 19 */ { BIG_TSS_PARTIAL,   "Dict overflow unpacking TSS partial string",
                                mod },
  /* 20 */ { BIG_TSS_STIME,     "Dict overflow unpacking TSS sdate/stime",
                                mod },
  /* 21 */ { BIG_TSS_STRING,    "Dict overflow unpacking TSS string", mod },
  /* 22 */ { BIG_TSS_DATETIME,  "Dict overflow unpacking TSS date/time", mod },
  /* 23 */ { BIG_TSS_GROCERY,   "Dict overflow unpacking TSS grocery", mod },
  /* 24 */ { BIG_TSS_PRICE,     "Dict overflow unpacking TSS price", mod },
  /* 25 */ { BIG_TSS_INTEGER,   "Dict overflow unpacking TSS integer", mod },
  /* 26 */ { MISSING_MAX_FID,   "Packed dictionary missing MAX_FID", mod },
  /* 27 */ { BIG_MAX_FID,       "MAX_FID is too big for 2 byte integer", mod },
  /* 28 */ { MISSING_FIDS_MSG,  "Packed dictioary missing FIDS component",mod },
  /* 29 */ { BIG_FID,           "Dict FID is larger than MAX_FID", mod },
  /* 30 */ { NULL_DICT_FNAME,   "Dictionary has NULL fname", mod },
  /* 31 */ { BAD_TSS_SIZE,      "Invalid type size in dictionary", mod },
  /* 32 */ { TOO_MANY_CLASSES,  "Too many classes in dictionary", mod },
  /* 33 */ { DUPLICATE_FID,     "Class FID is a duplicate", mod },
  /* 34 */ { DUPLICATE_CLASS,   "Class name is a duplicate", mod },
  /* 35 */ { UNDEFINED_CLASS,   "Class name undefined", mod },
  /* 36 */ { BAD_FORM_CLASS,    "Invalid form class", mod },
  /* 37 */ { BAD_DICTIONARY,    "Invalid dictionary", mod },
  /* 38 */ { BAD_DICT_FNAME,    "Dictionary field not found", mod },
  /* 39 */ { BAD_PROTO,         "Bad tib protocol enum value", mod },
  /* 40 */ { BAD_RAIMSG_TYPE,   "Bad RaiMsg_type enum value", mod },
  /* 41 */ { BAD_RV_SIZE,       "Bad RV size descriptor", mod },
  /* 42 */ { BAD_RV_TYPE,       "Bad RV type enum value", mod },
  /* 43 */ { BAD_CVT_STRING,    "Unable to convert field to/from string", mod },
  /* 44 */ { BAD_CVT_BOOL,      "Unable to convert field to/from bool", mod },
  /* 45 */ { BAD_CVT_INT,       "Unable to convert field to/from int", mod },
  /* 46 */ { BAD_CVT_REAL,      "Unable to convert field to/from real", mod },
  /* 47 */ { BAD_CVT_IPDATA,    "Unable to convert field to/from ipdata", mod },
  /* 48 */ { BAD_CVT_OVERFLOW,  "Conversion overflows string buffer", mod },
  /* 49 */ { BAD_RAIMSG_HINT,   "Hint type or size invalid", mod },
  /* 50 */ { BAD_RAIMSG_RV,     "Unable to convert RaiMsg field to RV", mod },
  /* 51 */ { BAD_DICT_FID,      "Dictionary FID not found", mod },
  /* 52 */ { BAD_RV_MTYPE,      "RV machine type or size invalid", mod },
  /* 53 */ { BAD_RV_MAGIC,      "RV message magic number invalid", mod },
  /* 54 */ { BAD_RAIMSG_MTYPE,  "RaiMsg machine type or size invalid", mod },
  /* 55 */ { NOT_SASS_FORM,     "Not dictionary form type message", mod },
  /* 56 */ { NOT_STRING_FIELD,  "Field is not a string type", mod },
  /* 57 */ { BAD_BUFFER,        "Buffer is null or too small", mod },
  /* 58 */ { NULL_DICT_FID,     "Dictionary FID is zero", mod },
  /* 59 */ { NO_DICTIONARY,     "No dictionary loaded for SASS messages", mod },
  /* 60 */ { RV_PACK_PARTIAL,   "Can't convert PARTIAL datatype to RVMSG field",
                                mod },
  /* 61 */ { BAD_UNPACK_BUF_SZ, "Size of message is larger than "
                                "the UnPack() data buffer", mod },
  /* 62 */ { NOT_RAIMSG,        "UnPack RAIMSG failed, magic mismatch", mod },
  /* 63 */ { NOT_RV_SASS,       "UnPack RV_SASS failed, magic mismatch", mod },
  /* 64 */ { NOT_TIB_SASS,      "UnPack TIB_SASS failed, magic mismatch", mod },
  /* 65 */ { NOT_TIB_SASS_FORM, "UnPack TIB_SASS_FORM failed, magic mismatch",
                                mod },
  /* 66 */ { NOT_RV_RAIMSG,     "UnPack RV_RAIMSG failed, magic mismatch", mod},
  /* 67 */ { NOT_XREP,          "UnPack XREP failed, magic mismatch", mod },
  /* 68 */ { NOT_RV,            "UnPack RV failed, magic mismatch", mod },
  /* 69 */ { NOT_CI_SASS,       "UnPack CI_SASS failed, magic mismatch", mod },
  /* 70 */ { NOT_CI_SASS_FORM,  "UnPack CI_SASS_FORM failed, magic mismatch",
                                mod },
  /* 71 */ { 71,                "Unknown error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}
