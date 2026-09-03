/* Copyright (c) 2011 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__rai_form_msg_h__
#define __rai_msg__rai_form_msg_h__

#if ! defined( RAIMSG_DLL_EXP )
#if defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllimport)
#else
#define RAIMSG_DLL_EXP
#endif
#endif

#ifndef __rai_msg__rai_msg_h__
#include "msg/rai_msg.h"
#endif

#ifndef __rai_util__int_bits_h__
#include "util/int_bits.h"
#endif

class RaiFormMsg;

struct RAIMSG_DLL_EXP RaiFormPtr {
  RaiMsg_size foff, fsize;
  const RaiMsg_dict *entry;
  RaiFormMsg *msg;

  const RaiMsg_dict *First( RaiFormMsg *m ) throw( RaiException );

  const RaiMsg_dict *Next( void ) throw( RaiException );

  const RaiMsg_dict *Find( RaiFormMsg &m,
                           const RaiMsg_dict *entry ) throw( RaiException ) {
    return this->Find( &m, entry );
  }
  const RaiMsg_dict *Find( RaiFormMsg *m,
                           const RaiMsg_dict *entry ) throw( RaiException );
  template<class T> void Get( T &val ) throw( RaiException );
};

class RAIMSG_DLL_EXP RaiFormMsg {
protected:
  byte       * buf;
  unsigned int bufLen, bufOff;

public:
  void UnPack( RaiMsg_data b,  unsigned int len ) throw( RaiException ) {
    unsigned int magic, off;
    if ( len >= 8 ) {
      rai::Unaligned::endianGetInt( (byte *) b, magic );
      rai::Unaligned::endianGetInt( &((byte *) b)[ 4 ], off );
      off += 8;
      if ( magic == RAIMSG_MAGIC_TIB_SASS && off <= len ) {
        this->buf    = (byte *) b;
        this->bufLen = len;
        this->bufOff = off;
        return;
      }
    }
    throw badFormMsgErr();
  }

  const RaiMsg_dict *First( RaiFormPtr &fld ) const throw( RaiException ) {
    fld.foff = 8;
    fld.fsize = 0;
    return this->Next( fld );
  }

  const RaiMsg_dict *Next( RaiFormPtr &fld ) const throw( RaiException ) {
    Rai_u16 fid, partialLen;
    fld.foff += fld.fsize;
    if ( fld.foff >= this->bufOff - 1 )
      return NULL;
    rai::Unaligned::endianGetInt( &this->buf[ fld.foff ], fid );
    fld.entry = rai::DataDictionary->getEntry( fid );
    if ( fld.entry != NULL ) {
      if ( ! fld.entry->partial )
        fld.fsize = 2U + ( ( (unsigned int) fld.entry->fsize + 1 ) & ~1U );
      else {
        rai::Unaligned::endianGetInt( &this->buf[ fld.foff + 4 ], partialLen );
        fld.fsize = 6U + ( ( (unsigned int) partialLen + 1 ) & ~1U );
      }
      if ( fld.foff + fld.fsize > this->bufOff )
        throw truncatedMsgErr();
    }
    else {
      if ( fid != 0 || fld.foff + 3 < this->bufOff )
        throw unknownFidErr();
    }
    return fld.entry;
  }

  const RaiMsg_dict *Find( RaiFormPtr &fld,
                           const RaiMsg_dict *f ) const throw( RaiException ) {
    const RaiMsg_dict *g;
    if ( (g = this->First( fld )) != NULL ) {
      do {
        if ( f->fid == g->fid )
          return g;
      } while ( (g = this->Next( fld )) != NULL );
    }
    return NULL;
  }

  void Print( rai::OutputStream *out ) throw( RaiException );

  void ConvertGet( RaiFormPtr &fld,  RaiMsg_data fdata,
                  RaiMsg_type ftype,  RaiMsg_size fsize ) throw( RaiException );

  void Get( RaiFormPtr &fld,  Rai_i8 &i8 ) throw( RaiException ) {
    return this->Get( fld, (Rai_u8 &) i8 );
  }

  void Get( RaiFormPtr &fld,  Rai_u8 &u8 ) throw( RaiException ) {
    if ( isIntType( fld.entry->ftype ) && fld.entry->fsize == sizeof( u8 ) )
      u8 = this->buf[ fld.foff + 2 ];
    else
      this->ConvertGet( fld, &u8, RAIMSG_UINT, sizeof( u8 ) );
  }

  void Get( RaiFormPtr &fld,  Rai_i16 &i16 ) throw( RaiException ) {
    return this->Get( fld, (Rai_u16 &) i16 );
  }

  void Get( RaiFormPtr &fld,  Rai_u16 &u16 ) throw( RaiException ) {
    if ( isIntType( fld.entry->ftype ) && fld.entry->fsize == sizeof( u16 ) )
      rai::Unaligned::endianGetInt( &this->buf[ fld.foff + 2 ], u16 );
    else
      this->ConvertGet( fld, &u16, RAIMSG_UINT, sizeof( u16 ) );
  }

  void Get( RaiFormPtr &fld,  Rai_i32 &i32 ) throw( RaiException ) {
    return this->Get( fld, (Rai_u32 &) i32 );
  }

  void Get( RaiFormPtr &fld,  Rai_u32 &u32 ) throw( RaiException ) {
    if ( isIntType( fld.entry->ftype ) && fld.entry->fsize == sizeof( u32 ) )
      rai::Unaligned::endianGetInt( &this->buf[ fld.foff + 2 ], u32 );
    else
      this->ConvertGet( fld, &u32, RAIMSG_UINT, sizeof( u32 ) );
  }

  void Get( RaiFormPtr &fld,  Rai_i64 &i64 ) throw( RaiException ) {
    return this->Get( fld, (Rai_u64 &) i64 );
  }

  void Get( RaiFormPtr &fld,  Rai_u64 &u64 ) throw( RaiException ) {
    if ( isIntType( fld.entry->ftype ) && fld.entry->fsize == sizeof( u64 ) )
      rai::Unaligned::endianGetInt( &this->buf[ fld.foff + 2 ], u64 );
    else
      this->ConvertGet( fld, &u64, RAIMSG_UINT, sizeof( u64 ) );
  }

  void Get( RaiFormPtr &fld,  Rai_f32 &f32 ) throw( RaiException ) {
    if ( isFloatType( fld.entry->ftype ) && fld.entry->fsize == sizeof( f32 ) )
      rai::Unaligned::endianGetFloat( &this->buf[ fld.foff + 2 ], f32 );
    else
      this->ConvertGet( fld, &f32, RAIMSG_REAL, sizeof( f32 ) );
  }

  void Get( RaiFormPtr &fld,  Rai_f64 &f64 ) throw( RaiException ) {
    /* price is 9 bytes, 8 bytes followed by denom, ignoring the denom here */
    if ( isFloatType( fld.entry->ftype ) && fld.entry->fsize >= sizeof( f64 ) )
      rai::Unaligned::endianGetFloat( &this->buf[ fld.foff + 2 ], f64 );
    else
      this->ConvertGet( fld, &f64, RAIMSG_REAL, sizeof( f64 ) );
  }

  void Get( RaiFormPtr &fld,  const char *&s ) throw( RaiException ) {
    s = (const char *) &this->buf[ fld.foff + 2 ];
  }

  void InitBuffer( RaiMsg_data b,  unsigned int len ) throw( RaiException ) {
    this->buf    = (byte *) b;
    this->bufLen = len;
    this->bufOff = 8;
    if ( 8 > len )
      throw bufOverflowErr();
    this->Packed();
  }

  RaiMsg_data Packed( void ) {
    rai::Unaligned::endianPutInt( (Rai_u32) RAIMSG_MAGIC_TIB_SASS, this->buf );
    rai::Unaligned::endianPutInt( (Rai_u32) ( this->bufOff - 8 ),
                                  &this->buf[ 4 ] );
    return this->buf;
  }

  RaiMsg_size PackSize( void ) const {
    return this->bufOff;
  }

  static RaiException badFormMsgErr( void ) {
    static const rai::ErrorRec rec = { 0, "Msg is bad", "RaiFormMsg" };
    return &rec;
  }

  static RaiException bufOverflowErr( void ) {
    static const rai::ErrorRec rec = { 1, "Not enough space", "RaiFormMsg" };
    return &rec;
  }

  static RaiException truncatedMsgErr( void ) {
    static const rai::ErrorRec rec = { 2, "Truncated msg", "RaiFormMsg" };
    return &rec;
  }

  static RaiException unknownFidErr( void ) {
    static const rai::ErrorRec rec = { 3, "Unknown FID", "RaiFormMsg" };
    return &rec;
  }

  void ConvertAppend( const RaiMsg_dict *f,  RaiMsg_data fdata,
                  RaiMsg_type ftype,  RaiMsg_size fsize ) throw( RaiException );

  void ConvertAppend( const RaiMsg_dict *f,
                      RaiFormPtr &fld ) throw( RaiException );
  static bool isIntType( Rai_u8 t ) {
    static const unsigned int tssIntTypes = ( 1U << RAI_TSS_INTEGER ) |
                                            ( 1U << RAI_TSS_SHORT_INT ) |
                                            ( 1U << RAI_TSS_LONG ) |
                                            ( 1U << RAI_TSS_BOOLEAN ) |
                                            ( 1U << RAI_TSS_BYTE ) |
                                            ( 1U << RAI_TSS_U_SHORT ) |
                                            ( 1U << RAI_TSS_U_INT ) |
                                            ( 1U << RAI_TSS_U_LONG );
    return ( tssIntTypes & ( 1U << t ) ) != 0;
  }

  static bool isFloatType( Rai_u8 t ) {
    static const unsigned int tssFloatTypes = ( 1U << RAI_TSS_PRICE ) |
                                              ( 1U << RAI_TSS_FLOAT ) |
                                              ( 1U << RAI_TSS_DOUBLE ) |
                                              ( 1U << RAI_TSS_DOUBLE_INT ) |
                                              ( 1U << RAI_TSS_GROCERY );
    return ( tssFloatTypes & ( 1U << t ) ) != 0;
  }

  static bool isStringType( Rai_u8 t ) {
    static const unsigned int tssStringTypes = ( 1U << RAI_TSS_SDATE ) |
                                               ( 1U << RAI_TSS_STIME ) |
                                               ( 1U << RAI_TSS_STRING ) |
                                               ( 1U << RAI_TSS_OPAQUE );
    return ( tssStringTypes & ( 1U << t ) ) != 0;
  }

  void Append_REC_TYPE( const RaiMsg_dict *f,  Rai_u16 u16 )
      throw( RaiException ) {
    return this->Append( f, (Rai_u16) ( ( u16 & ~( FID_PRIMITIVE_FLAG ) ) |
                                        FID_FIXED_FLAG ) );

  }

  void Append( const RaiMsg_dict *f,  Rai_i8 i8 ) throw( RaiException ) {
    return this->Append( f, (Rai_u8) i8 );
  }

  void Append( const RaiMsg_dict *f,  Rai_u8 u8 ) throw( RaiException ) {
    RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );

    if ( this->bufOff + sz > this->bufLen )
      throw bufOverflowErr();

    rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                  &this->buf[ this->bufOff ] );
    if ( isIntType( f->ftype ) && f->fsize == sizeof( u8 ) ) {
      this->buf[ this->bufOff + 2 ] = u8;
      this->buf[ this->bufOff + 3 ] = 0;
    }
    else
      this->ConvertAppend( f, &u8, RAIMSG_UINT, sizeof( u8 ) );
    this->bufOff += sz;
  }

  void Append( const RaiMsg_dict *f,  Rai_i16 i16 ) throw( RaiException ) {
    return this->Append( f, (Rai_u16) i16 );
  }

  void Append( const RaiMsg_dict *f,  Rai_u16 u16 ) throw( RaiException ) {
    RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );

    if ( this->bufOff + sz > this->bufLen )
      throw bufOverflowErr();

    rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                  &this->buf[ this->bufOff ] );
    if ( isIntType( f->ftype ) && f->fsize == sizeof( u16 ) )
      rai::Unaligned::endianPutInt( u16, &this->buf[ this->bufOff + 2 ] );
    else
      this->ConvertAppend( f, &u16, RAIMSG_UINT, sizeof( u16 ) );
    this->bufOff += sz;
  }

  void Append( const RaiMsg_dict *f,  Rai_i32 i32 ) throw( RaiException ) {
    return this->Append( f, (Rai_u32) i32 );
  }

  void Append( const RaiMsg_dict *f,  Rai_u32 u32 ) throw( RaiException ) {
    RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );

    if ( this->bufOff + sz > this->bufLen )
      throw bufOverflowErr();

    rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                  &this->buf[ this->bufOff ] );
    if ( isIntType( f->ftype ) && f->fsize == sizeof( u32 ) )
      rai::Unaligned::endianPutInt( u32, &this->buf[ this->bufOff + 2 ] );
    else
      this->ConvertAppend( f, &u32, RAIMSG_UINT, sizeof( u32 ) );
    this->bufOff += sz;
  }

  void Append( const RaiMsg_dict *f,  Rai_i64 i64 ) throw( RaiException ) {
    return this->Append( f, (Rai_u64) i64 );
  }

  void Append( const RaiMsg_dict *f,  Rai_u64 u64 ) throw( RaiException ) {
    RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );

    if ( this->bufOff + sz > this->bufLen )
      throw bufOverflowErr();

    rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                  &this->buf[ this->bufOff ] );
    if ( isIntType( f->ftype ) && f->fsize == sizeof( u64 ) )
      rai::Unaligned::endianPutInt( u64, &this->buf[ this->bufOff + 2 ] );
    else
      this->ConvertAppend( f, &u64, RAIMSG_UINT, sizeof( u64 ) );
    this->bufOff += sz;
  }

  void Append( const RaiMsg_dict *f,  Rai_f32 f32 ) throw( RaiException ) {
    RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );

    if ( this->bufOff + sz > this->bufLen )
      throw bufOverflowErr();

    rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                  &this->buf[ this->bufOff ] );
    if ( isFloatType( f->ftype ) && ( f->fsize == sizeof( f32 ) ||
                                      f->fsize == sizeof( f32 ) + 1 ) ) {
      rai::Unaligned::endianPutFloat( f32, &this->buf[ this->bufOff + 2 ] );
      if ( f->fsize == sizeof( f32 ) + 1 ) /* grocery */
        rai::Unaligned::putInt( (Rai_u16) 0,
                              &this->buf[ this->bufOff + 2 + sizeof( f32 ) ] );
    }
    else
      this->ConvertAppend( f, &f32, RAIMSG_REAL, sizeof( f32 ) );
    this->bufOff += sz;
  }

  void Append( const RaiMsg_dict *f,  Rai_f64 f64 ) throw( RaiException ) {
    RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );

    if ( this->bufOff + sz > this->bufLen )
      throw bufOverflowErr();

    rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                  &this->buf[ this->bufOff ] );
    if ( isFloatType( f->ftype ) && ( f->fsize == sizeof( f64 ) ||
                                      f->fsize == sizeof( f64 ) + 1 ) ) {
      rai::Unaligned::endianPutFloat( f64, &this->buf[ this->bufOff + 2 ] );
      if ( f->fsize == sizeof( f64 ) + 1 ) /* grocery */
        rai::Unaligned::putInt( (Rai_u16) 0,
                              &this->buf[ this->bufOff + 2 + sizeof( f64 ) ] );
    }
    else
      this->ConvertAppend( f, &f64, RAIMSG_REAL, sizeof( f64 ) );
    this->bufOff += sz;
  }

  void Append( const RaiMsg_dict *f,  const char *str ) throw( RaiException ) {
    RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );

    if ( this->bufOff + sz > this->bufLen )
      throw bufOverflowErr();

    rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                  &this->buf[ this->bufOff ] );
    if ( str == NULL )
      str = "";
    if ( isStringType( f->ftype ) && ! f->partial )
      ::strncpy( (char *) &this->buf[ this->bufOff + 2 ], str, sz - 2 );
    else {
      unsigned int len = ::strlen( str );
      if ( f->partial ) {
        sz = 6U + ( ( len + 1 ) & ~1U );
        if ( this->bufOff + sz > this->bufLen )
          throw bufOverflowErr();
      }
      this->ConvertAppend( f, (RaiMsg_data) str, RAIMSG_STRING, len );
    }
    this->bufOff += sz;
  }

  void Append( const RaiMsg_dict *f,  const char *str,
               RaiMsg_size strLen ) throw( RaiException ) {
    RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );

    if ( this->bufOff + sz > this->bufLen )
      throw bufOverflowErr();

    rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                  &this->buf[ this->bufOff ] );
    if ( isStringType( f->ftype ) && ! f->partial ) {
      if ( strLen > sz - 2 )
        strLen = sz - 2;
      ::memcpy( &this->buf[ this->bufOff + 2 ], str, strLen );
      if ( strLen < sz - 2 )
        ::memset( &this->buf[ this->bufOff + 2 + strLen ], 0,
                  ( sz - 2 ) - strLen );
    }
    else {
      if ( f->partial ) {
        sz = 6U + ( ( strLen + 1 ) & ~1U );
        if ( this->bufOff + sz > this->bufLen )
          throw bufOverflowErr();
      }
      this->ConvertAppend( f, (RaiMsg_data) str, RAIMSG_STRING, strLen );
    }
    this->bufOff += sz;
  }

  void Append( const RaiMsg_dict *f,  RaiMsg_type type,  RaiMsg_size size,
               RaiMsg_data buf ) throw( RaiException ) {
    if ( type == RAIMSG_STRING )
      this->Append( f, (const char *) buf, size );
    else {
      RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );
      if ( this->bufOff + sz > this->bufLen )
        throw bufOverflowErr();

      rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                    &this->buf[ this->bufOff ] );
      this->ConvertAppend( f, buf, type, size );
      this->bufOff += sz;
    }
  }

  void Append( const RaiMsg_dict *f,  RaiFormPtr *fld ) {
    return this->Append( f, *fld );
  }

  void Append( const RaiMsg_dict *f,  RaiFormPtr &fld ) throw( RaiException ) {
    if ( isStringType( f->ftype ) && isStringType( fld.entry->ftype ) ) {
      this->Append( f, (const char *) &fld.msg->buf[ fld.foff + 2 ],
                    fld.entry->fsize );
    }
    else {
      RaiMsg_size sz = 2U + ( ( (unsigned int) f->fsize + 1 ) & ~1U );
      if ( this->bufOff + sz > this->bufLen )
        throw bufOverflowErr();

      rai::Unaligned::endianPutInt( (unsigned short) ( f->fid | FID_CTRL ),
                                    &this->buf[ this->bufOff ] );
      if ( f->fsize == fld.entry->fsize &&
          ( ( isIntType( f->ftype ) && isIntType( fld.entry->ftype ) ) ||
            ( isFloatType( f->ftype ) &&
              isFloatType( fld.entry->ftype ) ) ) ) {

        for ( unsigned int i = 0; i < f->fsize; i++ )
          this->buf[ this->bufOff + 2 + i ] = fld.msg->buf[ fld.foff + 2 + i ];
      }
      else {
        this->ConvertAppend( f, fld );
      }
      this->bufOff += sz;
    }
  }

  void Append( RaiFormPtr &fld ) throw( RaiException ) {
    if ( this->bufOff + fld.fsize > this->bufLen )
      throw bufOverflowErr();
    for ( unsigned int i = 0; i < fld.fsize; i++ )
      this->buf[ this->bufOff + i ] = fld.msg->buf[ fld.foff + i ];
    this->bufOff += fld.fsize;
  }
};


inline const RaiMsg_dict *
RaiFormPtr::First( RaiFormMsg *m ) throw( RaiException )
{
  this->msg = m;
  return m->First( *this );
}

inline const RaiMsg_dict *
RaiFormPtr::Next( void ) throw( RaiException )
{
  return this->msg->Next( *this );
}

inline const RaiMsg_dict *
RaiFormPtr::Find( RaiFormMsg *m,
                  const RaiMsg_dict *entry ) throw( RaiException )
{
  this->msg = m;
  return m->Find( *this, entry );
}

template<class T> inline void
RaiFormPtr::Get( T &val ) throw( RaiException )
{
  this->msg->Get( *this, val );
}

#endif
