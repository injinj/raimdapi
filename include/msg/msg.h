/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__msg_h__
#define __rai_msg__msg_h__

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

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

/*struct RaiMsg_mfeed;*/
namespace rai {
  class InputStream;
  class OutputStream;
}
struct RaiMsg_dict;
struct RaiMsg_form;
struct RaiMsg;
struct RaiField;
struct RaiMsg_extra;
struct RaiMsg_config;

typedef Rai_i32 (* RaiMsg_apply_cb )( RaiMsg *msg_ptr, RaiField *field_ptr,
                                      void *closure );
typedef Rai_u32 (* RaiMsg_XRepHashFunc )( RaiMsg_name fname );


enum RaiMsg_protocol {
  RAIMSG_PROTO        = 0, /* tibmsg self describing */
  RV_SASS_PROTO       = 1, /* XXX t.b.d */
  TIB_SASS_PROTO      = 2, /* sass with tib header */
  TIB_SASS_FORM_PROTO = 3, /* sass with fixed field offset */
  RV_RAIMSG_PROTO     = 4, /* XXX t.b.d */
  XREP_PROTO          = 5, /* XXX t.b.d */
  RV_PROTO            = 6, /* rv self describing */
  CI_SASS_PROTO       = 7, /* sass with ciServer header */
  CI_SASS_FORM_PROTO  = 8  /* sass with fixed field offset */
};

enum RaiMsg_memory {
  RAIMSG_MEMORY_STATIC  = 0, /* msgBuf won't be freed */
  RAIMSG_MEMORY_DYNAMIC = 1, /* msgBuf can be resized with realloc() */
  RAIMSG_MEMORY_FIXED   = 2, /* msgBuf won't be freed, has no header */
  RAIMSG_MEMORY_MAMA    = 3  /* dynamic with 'I' at the first byte */
};


struct RAIMSG_DLL_EXP RaiMsg {
  private:
    friend struct RaiField;

    RaiMsg_protocol proto;     /* current protocol */
    Rai_u8        * msgBuf;    /* the msg, if null all other fields are junk */
    RaiMsg_size     msgSize,   /* amount of msgBuf allocated */
                    msgStart;  /* where in msgBuf the message starts */
    RaiMsg_memory   isDynamic; /* if msgBuf is allocated or static memory */
    RaiMsg        * parent;    /* if msg is part of another, this is its parent */
    RaiMsg_extra  * msgEx;     /* contains decoded arrays and activation levels */

  public:
    SYS_OPS( RaiMsg );

    RaiMsg()
     : proto(RAIMSG_PROTO), msgBuf(0), msgSize(0), msgStart(0),
       isDynamic(RAIMSG_MEMORY_STATIC), parent(0), msgEx(0) {}

    RaiMsg( RaiMsg_protocol protoArg )
     : proto(protoArg), msgBuf(0), msgSize(0), msgStart(0),
       isDynamic(RAIMSG_MEMORY_STATIC), parent(0), msgEx(0) {}

    RaiMsg( RaiMsg_memory memArg )
     : proto(RAIMSG_PROTO), msgBuf(0), msgSize(0), msgStart(0),
       isDynamic(memArg), parent(0), msgEx(0) {}

    /* move message data from this RaiMsg to another */
    void moveTo( RaiMsg &toMsg ) {
      if ( toMsg.msgBuf != NULL )
        toMsg.Release();
      toMsg.proto     = this->proto;
      toMsg.msgBuf    = this->msgBuf;    this->msgBuf    = NULL;
      toMsg.msgSize   = this->msgSize;   this->msgSize   = 0;
      toMsg.msgStart  = this->msgStart;  this->msgStart  = 0;
      toMsg.isDynamic = this->isDynamic; this->isDynamic = RAIMSG_MEMORY_STATIC;
      toMsg.parent    = this->parent;    this->parent    = NULL;
      toMsg.msgEx     = this->msgEx;     this->msgEx     = NULL;
    }
    /* if no fields are in message */
    bool isEmpty( void ) const;

    /* return how many bytes are added to message body for proto */
    static RaiMsg_size HeaderSize( RaiMsg_protocol proto )
;
    /* setup a static or dynamic message buffer for msg construction */
    void InitBuffer( RaiMsg_data msg_buffer,  RaiMsg_size msg_off,
                     RaiMsg_size msg_buffer_size,  RaiMsg_protocol proto,
                     RaiMsg_memory is_dynamic );

    ~RaiMsg() {
      if ( this->msgBuf != NULL )
        this->Release();
    }
  private:
    /* initialize message size and magic number in header */
    void InitHeader( RaiMsg_size messageSize );
  public:
    /* initialize message size and magic number in header */
    static void InitHeader( RaiMsg_protocol proto,  RaiMsg_data msgBuf,
                            RaiMsg_size messageSize );

    /* check proto range */
    static bool isValidProto( RaiMsg_protocol proto ) {
      static const Rai_u32 validMap =
        ( 1 << RAIMSG_PROTO )    | ( 0 << RV_SASS_PROTO )       |
        ( 1 << TIB_SASS_PROTO )  | ( 1 << TIB_SASS_FORM_PROTO ) |
        ( 0 << RV_RAIMSG_PROTO ) | ( 0 << XREP_PROTO )          |
        ( 1 << RV_PROTO )        | ( 1 << CI_SASS_PROTO ) |
        ( 1 << CI_SASS_FORM_PROTO );
      unsigned int p = (unsigned int) proto;
      if ( p >= 32 )
        return false;
      if ( (validMap & ( 1 << p )) != 0 )
        return true;
      return false;
    }

    /* if field offsets are known */
    static bool isFormProto( RaiMsg_protocol proto ) {
      static const Rai_u32 validMap =
        ( 0 << RAIMSG_PROTO )    | ( 0 << RV_SASS_PROTO )       |
        ( 0 << TIB_SASS_PROTO )  | ( 1 << TIB_SASS_FORM_PROTO ) |
        ( 0 << RV_RAIMSG_PROTO ) | ( 0 << XREP_PROTO )          |
        ( 0 << RV_PROTO )        | ( 0 << CI_SASS_PROTO ) |
        ( 1 << CI_SASS_FORM_PROTO );
      unsigned int p = (unsigned int) proto;
      if ( p >= 32 )
        return false;
      if ( (validMap & ( 1 << p )) != 0 )
        return true;
      return false;
    }

    /* if field offsets are known */
    bool isForm( void ) const {
      return isFormProto( this->proto );
    }

    /* if uses dictionary */
    static bool isSassProto( RaiMsg_protocol proto ) {
      static const Rai_u32 validMap =
        ( 0 << RAIMSG_PROTO )    | ( 0 << RV_SASS_PROTO )       |
        ( 1 << TIB_SASS_PROTO )  | ( 1 << TIB_SASS_FORM_PROTO ) |
        ( 0 << RV_RAIMSG_PROTO ) | ( 0 << XREP_PROTO )          |
        ( 0 << RV_PROTO )        | ( 1 << CI_SASS_PROTO ) |
        ( 1 << CI_SASS_FORM_PROTO );
      unsigned int p = (unsigned int) proto;
      if ( p >= 32 )
        return false;
      if ( (validMap & ( 1 << p )) != 0 )
        return true;
      return false;
    }

    static const char *ProtoToString( RaiMsg_protocol proto );

    static RaiMsg_protocol StringToProto( const char *s );

    /* if qforms used */
    bool isSass( void ) const {
      return isSassProto( this->proto );
    }

    /* used to upgrade from xxx_ to xxx_FORM_, where field offsets are known */
    void SetProtocol( RaiMsg_protocol proto ) {
      this->proto = proto;
    }

    /* change proto to form version */
    bool UpgradeToForm( void ) {
      if ( this->proto == TIB_SASS_FORM_PROTO ||
           this->proto == CI_SASS_FORM_PROTO )
        return true;
      if ( this->proto == TIB_SASS_PROTO ) {
        this->proto = TIB_SASS_FORM_PROTO;
        return true;
      }
      else if ( this->proto == CI_SASS_PROTO ) {
        this->proto = CI_SASS_FORM_PROTO;
        return true;
      }
      return false;
    }

    /* return the current protocol */
    RaiMsg_protocol GetProtocol( void ) const {
      return this->proto;
    }
    const char *GetProtocolString( void ) const {
      return RaiMsg::ProtoToString( this->proto );
    }
    RaiMsg_memory GetMemory( void ) const {
      return this->isDynamic;
    }

    /* reinitialize msgBuf without freeing it */
    void ReUse( void );

    /* ReUse and switch protos */
    void ReUse( RaiMsg_protocol proto );

    /* clear all alloced memory, can be reused afterwards */
    void Release( void );

    /* clear cached array decodings (e.g. bswap endian) and activate levels */
    void ReleaseExtra( void );

    /* check if has extra data allocated before calling ReleaseExtra() */
    void ReleaseExtraCheck( void ) {
      if ( this->msgBuf != NULL && this->msgEx != NULL )
        this->ReleaseExtra();
    }
    /* grab packed buffer and release */
    void Reset( RaiMsg_data *buf = NULL,  RaiMsg_size *bufLen = NULL ) {
      if ( this->msgBuf != NULL ) {
        if ( buf != NULL ) {
          *buf = this->msgBuf;
          if ( bufLen != NULL )
            *bufLen = this->msgSize;
          this->msgBuf = NULL;
          if ( this->msgEx != NULL )
            this->ReleaseExtra();
        }
        else {
          this->Release();
        }
      }
      else {
        if ( buf != NULL ) {
          *buf = NULL;
          if ( bufLen != NULL )
            *bufLen = 0;
        }
      }
    }
  private:
    /* used by RaiField for accessing sub messages */
    void InitSubMessage( Rai_u8 *msg_buffer,  RaiMsg_size msg_buffer_size,
                         RaiMsg *parent,  RaiMsg_protocol proto,
                         RaiMsg_memory isDynamic );
  public:
    /* zero all fields in a form, allocating buffer if necessary */
    void ClearForm( const RaiMsg_form *form );

    /* check that elements are in form order */
    bool CheckIsForm( const RaiMsg_form *form,  bool inFormOrder )
;
    /* allocate buf space and copy message to this message */
    void Copy( RaiMsg *msg_ptr );

    /* locate field fname and setup a RaiField */
    bool Get( RaiMsg_name fname,  RaiField &field );

    /* locate field fname and setup a RaiField, with rv style fid */
    bool GetEx( RaiMsg_name fname,  Rai_u16 fid,  RaiField &field )
;
    /* get field fname's contents */
    template<class T> bool Get( RaiMsg_name fname,  T &arg );
    /* get field fname's contents, with rv style fid */
    template<class T> bool GetEx( RaiMsg_name fname,  Rai_u16 fid,  T &arg )
;
    /* copy or convert field's contents to a string */
    bool Get( RaiMsg_name fname,  char *str,  RaiMsg_size limit )
;
    /* copy or convert field's contents to a string, with rv style fid */
    bool GetEx( RaiMsg_name fname,  Rai_u16 fid,  char *str,
                RaiMsg_size limit );
#if 0
    /* return a pointer to field's string data */
    bool Get( RaiMsg_name fname,  char *&str );
#endif
    /* copy or convert field's contents to fdata */
    bool Get( RaiMsg_name fname,  RaiMsg_type ftype,
              RaiMsg_size fsize,  RaiMsg_data fdata );
    /* copy or convert field's contents to fdata, with rv style fid */
    bool GetEx( RaiMsg_name fname,  Rai_u16 fid,  RaiMsg_type ftype,
                RaiMsg_size fsize,  RaiMsg_data fdata );
    /* locate field by dictionary fid */
    bool Get( const RaiMsg_dict *entry,  RaiField &field )
;
    /* copy or convert field to arg */
    template<class T> bool Get( const RaiMsg_dict *entry, T &arg )
;
    /* copy or convert field's contents to a string */
    bool Get( const RaiMsg_dict *entry,  char *str,  RaiMsg_size limit )
;
    /* return a pointer to field's string data */
    bool Get( const RaiMsg_dict *entry,  char *&str );

    /* copy or convert field's contents to fdata */
    bool Get( const RaiMsg_dict *entry,  RaiMsg_type ftype,
              RaiMsg_size fsize,  RaiMsg_data fdata );
    enum {
      HAVE_MSG_TYPE   = 1,
      HAVE_REC_TYPE   = 2,
      HAVE_SEQ_NO     = 4,
      HAVE_REC_STATUS = 8
    };
    /* returns mask of whether field is present and type is 2 byte integer
       0 = none, 1 = msgType, 2 = recType, 4 = seqNo, 8 = recStatus */
    byte GetSassHeader( Rai_u16 &msgType,  Rai_u16 &recType,
                        Rai_u16 &seqNo,  Rai_u16 &recStatus,
                        Rai_u8 needFlds = 15 /* all HAVE_ field bits above */)
;
    byte GetSassHeaderFields( RaiField &msgTypeF,  RaiField &recTypeF,
                              RaiField &seqNoF,  RaiField &recStatusF,
                              Rai_u16 &msgType,  Rai_u16 &recType,
                              Rai_u16 &seqNo,  Rai_u16 &recStatus )
;
  private:
    /* used by RaiField to get array data, which sets up a RaiMsg_extra element
     * to cache decoded array and byteswaps the array */
    void GetDecodedArray( RaiMsg_data array,  RaiMsg_type elem_type,
                          RaiMsg_size elem_size,  RaiMsg_size num_elems,
                          RaiMsg_data &decodedArray );
    /* not used, null terminates string */
    void GetDecodedString( RaiMsg_data string,  RaiMsg_size string_size,
                           RaiMsg_data &decodedString )
;
  public:
    /* for each field of message call callback_function() */
    void Apply( RaiMsg_name fname,  RaiMsg_apply_cb callback_function,
                void *closure = NULL );
    /* separator for accessing sub components of message, defaults to '.' */
    static void SetNameSeparator( char sep_char );
  private:
    /* used to initialize a dynamically allocated message */
    void AppendHeader( void );

    /* makes space for new field */
    Rai_u8 *Adjust( RaiMsg_size field_offset,  RaiMsg_size new_size,
                    RaiMsg_size old_size );
  public:
    /* append field data, doesn't look at data being appended */
    RaiMsg_data AppendRaw( RaiMsg_data field_data,  RaiMsg_size field_size )
;
    /* return pointer to message start, skipping over the header */
    RaiMsg_data RawData( RaiMsg_size *dataSize = NULL );
    /* append new field to message */
    void Append( RaiField *field_ptr );
    /* append field with different name */
    void Append( RaiMsg_name fname,  RaiField *field_ptr )
;
    /* append field with different name and fid */
    void AppendEx( RaiMsg_name fname,  Rai_u16 fid,  RaiField *field_ptr )
;
    /* encode arg into field and append */
    template<class T> void Append( RaiMsg_name fname, T arg )
;
    /* encode arg into field and append using RV style fid */
    template<class T> void AppendEx( RaiMsg_name fname,  Rai_u16 fid,  T arg )
;
    /* encode a field with a message type and append it */
    void Append( RaiMsg_name fname,  RaiMsg *msg_ptr );
    /* append a partial data type, which is an opaque with an offset  */
    void Append( RaiMsg_name fname,  RaiMsg_data partial_data,
                 RaiMsg_size partial_size,  RaiMsg_size offset )
;
    /* append an array of primary types */
    void Append( RaiMsg_name fname,  RaiMsg_data array_data,
                 RaiMsg_size num_entries,  RaiMsg_type entry_type,
                 RaiMsg_size entry_size );
    /* append an fdata pointer of any type except array and partial */
    void Append( RaiMsg_name fname,  RaiMsg_type ftype,
                 RaiMsg_size fsize,  RaiMsg_data fdata )
;
    /* append fdata and hint_data of any type except array and partial */
    void Append( RaiMsg_name fname,  RaiMsg_type ftype,
                 RaiMsg_size fsize,  RaiMsg_data fdata,
                 RaiMsg_type hint_type,  RaiMsg_size hint_size,
                 RaiMsg_data hint_data );
    /* append new field by fid */
    void Append( const RaiMsg_dict *entry, RaiField *field_ptr )
;
    /* encode arg into field and append */
    template<class T> void Append( const RaiMsg_dict *entry,  T arg )
;
    /* append a partial data type, which is an opaque with an offset  */
    void Append( const RaiMsg_dict *entry,  RaiMsg_data partial_data,
                 RaiMsg_size partial_size,  RaiMsg_size offset ) 
;
    /* append an array of primary types */
    void Append( const RaiMsg_dict *entry,  RaiMsg_data array_data,
                 RaiMsg_size num_entries,  RaiMsg_type entry_type,
                 RaiMsg_size entry_size );
    /* append an fdata pointer of any type except array and partial */
    void Append( const RaiMsg_dict *entry,  RaiMsg_type ftype,
                 RaiMsg_size fsize,  RaiMsg_data fdata )
;
    /* append fdata and hint_data of any type except array and partial */
    void Append( const RaiMsg_dict *entry,  RaiMsg_type ftype,
                 RaiMsg_size fsize,  RaiMsg_data fdata,
                 RaiMsg_type hint_type,  RaiMsg_size hint_size,
                 RaiMsg_data hint_data );
    /* try to locate field and replace it with new data, appended if not found */
    void Update( RaiField *field_ptr );
    /* replace one field with another, the old_field should reference a field
     * within the message */
    void Replace( RaiField *old_field,  RaiField *new_field )
;
  protected:
    /* used by the above for non-sass type msessages */
    void Replace_SD( RaiField &field,  RaiField &new_field )
;
    /* used by the above for non-sass type msessages */
    void Replace_SASS( const RaiMsg_dict *entry,  RaiField &field,
                       RaiField &new_field );
  public:
    /* replace field with arg */
    template<class T> void Update( RaiMsg_name fname,  T arg )
;
    /* replace field with arg using RV style fid */
    template<class T> void UpdateEx( RaiMsg_name fname,  Rai_u16 fid,  T arg )
;
    /* replace field with message */
    void Update( RaiMsg_name fname,  RaiMsg *msg_ptr );

    /* replace field with partial data, which is an opaque with an offset */
    void Update( RaiMsg_name fname,  RaiMsg_data partial_data,
                 RaiMsg_size partial_size,  RaiMsg_size offset )
;
    /* replace field with an array */
    void Update( RaiMsg_name fname,  RaiMsg_data array_data,
                 RaiMsg_size num_entries,  RaiMsg_type entry_type,
                 RaiMsg_size entry_size );
    /* replace field with an fdata pointer of any type except array and partial */
    void Update( RaiMsg_name fname,  RaiMsg_type ftype,
                 RaiMsg_size fsize,  RaiMsg_data fdata )
;
    /* replace field fdata and hint_data of any type except array and partial */
    void Update( RaiMsg_name fname,  RaiMsg_type ftype,
                 RaiMsg_size fsize,  RaiMsg_data fdata,
                 RaiMsg_type hint_type,  RaiMsg_size hint_size,
                 RaiMsg_data hint_data );
    /* try to locate field by dictionary fid and replace it */
    void Update( const RaiMsg_dict *entry, RaiField *field_ptr )
;
    /* append field with different name */
    void Update( RaiMsg_name fname,  RaiField *field_ptr )
;
    /* replace field with arg */
    template<class T> void Update( const RaiMsg_dict *entry,  T arg )
;
    /* replace field with partial data, which is an opaque with an offset */
    void Update( const RaiMsg_dict *entry,  RaiMsg_data partial_data,
                 RaiMsg_size partial_size,  RaiMsg_size offset )
;
    /* replace field with an array */
    void Update( const RaiMsg_dict *entry,  RaiMsg_data array_data,
                 RaiMsg_size num_entries,  RaiMsg_type entry_type,
                 RaiMsg_size entry_size );
    /* replace field with an fdata pointer of any type except array and partial */
    void Update( const RaiMsg_dict *entry,  RaiMsg_type ftype,
                 RaiMsg_size fsize,  RaiMsg_data fdata )
;
    /* replace field fdata and hint_data of any type except array and partial */
    void Update( const RaiMsg_dict *entry,  RaiMsg_type ftype,
                 RaiMsg_size fsize,  RaiMsg_data fdata,
                 RaiMsg_type hint_type,  RaiMsg_size hint_size,
                 RaiMsg_data hint_data );
    /* setup message for accessing and updating field of type message */
    bool Activate( RaiMsg_name msg_field_name = NULL );
  private:
    /* used by Activate to locate sub messages by name */
    bool GetMsgOffset( RaiMsg_name msg_field_name,  RaiMsg_size *offset_stack,
                       Rai_u32 &offset_top,  RaiMsg *msgBase )
;
  public:
    /* update field name */
    bool Rename( RaiMsg_name old_fname, RaiMsg_name new_fname )
;
    /* remove field by name */
    bool Remove( RaiMsg_name fname );

    /* setup message to access encoded static memory */
    void UnPack( RaiMsg_data from_ptr );

    /* setup message limited by size */
    void UnPack( RaiMsg_data from_ptr,  RaiMsg_size from_size )
;
    /* setup message limited by size and protocol */
    void UnPack( RaiMsg_protocol proto,  RaiMsg_data from_ptr,
                 RaiMsg_size from_size,  RaiMsg_memory memKind )
;
    /* setup message at offset limited by size and protocol */
    void UnPack( RaiMsg_protocol proto,  RaiMsg_data from_ptr,
                 RaiMsg_size from_size,  RaiMsg_memory memKind,
                 RaiMsg_size from_off );
    /* look at message header to determine the type of message */
    static bool ExtractProtocol( RaiMsg_data from_ptr,  RaiMsg_size from_size,
                                 RaiMsg_protocol &proto );
    /* same as above except it also checks for _data_ rv7 opaque encapsulation
     * and returns the offset in msgOff where the sub message starts */
    static bool ExtractProtocolEx( RaiMsg_data from_ptr,  RaiMsg_size from_size,
                                   RaiMsg_protocol &proto,
                                   RaiMsg_size &msgOff );
    /* if proto != RV_PROTO, put message into an opaque with a field name
     * identifying the encapsulation of the message: _TIBMSG, _QFORM
     * this is useful for Tibrv version 6+, since they must use RV_PROTO
     * messages in the transport */
    void TibrvEncapsulate( void );

    /* copy encoded message */
    void Pack( RaiMsg_data to_ptr ) const;

    /* length of encoded message */
    RaiMsg_size PackSize( void ) const;

    /* length of currently activated message or toplevel without header */
    RaiMsg_size SubMsgSize( void ) const;

    /* offset of currently activated message or toplevel without header */
    RaiMsg_size SubMsgOff( void ) const;

    /* return a pointer to packed message */
    RaiMsg_data Packed( void ) const;

    /* always packed */
    bool NeedPack( void ) const {
      return false;
    }
    /* format message to stream, default is 'PrintTib' format */
    void Print( rai::OutputStream *output_file,
                Rai_u32 field_newlines       = 1,
                const char  * fname_format   = "%-14s : ",
                Rai_u32 print_opaques        = 1,
                const char  * debug_format   = "%-7s %3d : ",
                const char  * debug_hformat  = NULL );
    /* format message in xml */
    enum {
      ADD_TYPE_ATTR         = 0x1,
      ADD_SIZE_ATTR         = 0x2,
      ADD_FID_ATTR          = 0x4,
      ADD_PARTIAL_OFF_ATTR  = 0x8,
      ADD_ARRAY_COUNT_ATTR  = 0x10,
      ADD_ARRAY_TYPE_ATTR   = 0x20,
      ADD_ARRAY_ELSIZE_ATTR = 0x40,
      ADD_HINT_ATTR         = 0x80,
      ADD_HINT_TYPE_ATTR    = 0x100,
      ADD_HINT_SIZE_ATTR    = 0x200,
      ADD_ALL_ATTRS         = 0x3ff
    };
    void PrintXML( rai::OutputStream *output_file,
                   Rai_u32 attr_flags = 0,
                   Rai_u32 field_newlines = 1,
                   const char *msg_name   = "MSG",
                   const char **msg_atts  = NULL );

    /* dump message buffer in hex */
    void PrintHex( rai::OutputStream *output_file );
    /* dump message buffer in hex */
    static void PrintHex( rai::OutputStream *output_file,  Rai_u8 *msg,
                          RaiMsg_size msgSize,  RaiMsg_size offset = 0 )
;
    /* read message data */
    void Read( rai::InputStream *f,  rai::OutputStream *err )
;
    /* read message data from file */
    void Read( const char *filename,  rai::OutputStream *err )
;
    /* convert string to RaiMsg_type */
    static RaiMsg_type StrType( const char *type_str );

    /* get string name of RaiMsg_type */
    static const char *TypeStr( RaiMsg_type type );

    /* setup dictionary for decoding and encoding sass type messages */
    static void SetDataDictionary( RaiMsg_config *dictionary );

    /* get dictionary currently set */
    static RaiMsg_config *GetDataDictionary( void );

    /* read dictionary from cfile formated files */
    static void ReadDataDictionary( const char *tss_fields_fname,
                                    const char *tss_records_fname )
;
    /* get dictionary entry by field class name */
    static bool GetDictEntry( const char *fname,  const RaiMsg_dict *&entry );

    /* get dictionary entry by field class id */
    static bool GetDictEntry( Rai_u16 fid,  const RaiMsg_dict *&entry );

    /* get dictionary form by field class name */
    static bool GetDictForm( const char *fname,  const RaiMsg_form *&form );

    /* get dictionary form by field class id */
    static bool GetDictForm( Rai_u16 fid,  const RaiMsg_form *&form );

    /* return true when two memory regions intersect */
    static bool Overlaps( const RaiMsg_data p1,  RaiMsg_size s1,
                          const RaiMsg_data p2,  RaiMsg_size s2 ) {
      /* max( p1, p2 ) < min( p1 + s1, p2 + s2 ) */
      return ( (const Rai_u8 *) p1 > (const Rai_u8 *) p2 ?
                 (const Rai_u8 *) p1 : (const Rai_u8 *) p2 ) <
               ( &((const Rai_u8 *) p1)[ s1 ] < &((const Rai_u8 *) p2)[ s2 ] ?
                 &((const Rai_u8 *) p1)[ s1 ] : &((const Rai_u8 *) p2)[ s2 ] );
    }
    /* return true when memory region intersects message buffer */
    bool Overlaps( const RaiMsg_data p2,  RaiMsg_size s2 ) const {
      if ( this->msgBuf == NULL )
        return false;
      return RaiMsg::Overlaps( this->msgBuf, this->msgSize, p2, s2 );
    }
};


namespace RaiMsgErr {
  enum {
    OK                 = 0,
    BAD_ARG            = 1,
    BAD_MAGIC_NUMBER   = 2,
    VERSION_MISMATCH   = 3,
    MEMORY_STATIC_ERR  = 4,
    NO_MEMORY          = 5,
    BAD_READ_FILE      = 6,
    BAD_READ_SYNTAX    = 7,
    RESERVED           = 8,
    NOT_FOUND          = 9,
    APPLY_ERROR        = 10,
    NO_FIELD           = 11,
    BAD_TSS_PARTIAL    = 12,
    BAD_TSS_DATETIME   = 13,
    BAD_TSS_GROCERY    = 14,
    BAD_TSS_GROCERY2   = 15,
    BAD_TSS_PRICE      = 16,
    BAD_TSS_INTEGER    = 17,
    BAD_TSS_TYPE       = 18,
    BIG_TSS_PARTIAL    = 19,
    BIG_TSS_STIME      = 20,
    BIG_TSS_STRING     = 21,
    BIG_TSS_DATETIME   = 22,
    BIG_TSS_GROCERY    = 23,
    BIG_TSS_PRICE      = 24,
    BIG_TSS_INTEGER    = 25,
    MISSING_MAX_FID    = 26,
    BIG_MAX_FID        = 27,
    MISSING_FIDS_MSG   = 28,
    BIG_FID            = 29,
    NULL_DICT_FNAME    = 30,
    BAD_TSS_SIZE       = 31,
    TOO_MANY_CLASSES   = 32,
    DUPLICATE_FID      = 33,
    DUPLICATE_CLASS    = 34,
    UNDEFINED_CLASS    = 35,
    BAD_FORM_CLASS     = 36,
    BAD_DICTIONARY     = 37,
    BAD_DICT_FNAME     = 38,
    BAD_PROTO          = 39,
    BAD_RAIMSG_TYPE    = 40,
    BAD_RV_SIZE        = 41,
    BAD_RV_TYPE        = 42,
    BAD_CVT_STRING     = 43,
    BAD_CVT_BOOL       = 44,
    BAD_CVT_INT        = 45,
    BAD_CVT_REAL       = 46,
    BAD_CVT_IPDATA     = 47,
    BAD_CVT_OVERFLOW   = 48,
    BAD_RAIMSG_HINT    = 49,
    BAD_RAIMSG_RV      = 50,
    BAD_DICT_FID       = 51,
    BAD_RV_MTYPE       = 52,
    BAD_RV_MAGIC       = 53,
    BAD_RAIMSG_MTYPE   = 54,
    NOT_SASS_FORM      = 55,
    NOT_STRING_FIELD   = 56,
    BAD_BUFFER         = 57,
    NULL_DICT_FID      = 58,
    NO_DICTIONARY      = 59,
    RV_PACK_PARTIAL    = 60,
    BAD_UNPACK_BUF_SZ  = 61,

    BASE_PROTO_ERR     =     62,
    NOT_RAIMSG         = 0 + 62, /* add protocol to BASE_PROTO_ERROR */
    NOT_RV_SASS        = 1 + 62,
    NOT_TIB_SASS       = 2 + 62,
    NOT_TIB_SASS_FORM  = 3 + 62,
    NOT_RV_RAIMSG      = 4 + 62,
    NOT_XREP           = 5 + 62,
    NOT_RV             = 6 + 62,
    NOT_CI_SASS        = 7 + 62,
    NOT_CI_SASS_FORM   = 8 + 62
  };
  RAIMSG_DLL_EXP
  RaiMsgException getErr( unsigned int status );
}

#endif
