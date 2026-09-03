/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 * Mirrors com.rai.raimsg.RaiMsg / RaiField / Partial. */
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using Com.Rai.Interop;
using Com.Rai.Raiexception;

namespace Com.Rai.Raimsg {

/** A partial field: opaque data at an offset within the field */
public class Partial {
  readonly byte[] data;
  readonly int    offset;
  public Partial( byte[] data,  int offset ) { this.data = data; this.offset = offset; }
  public override string ToString() { return Encoding.UTF8.GetString( this.data ); }
  public byte[] getData() { return this.data; }
  public int getOffset() { return this.offset; }
}

/** RaiMsg is a self describing message with named, typed fields, which can
 * pack into several wire protocols (RAIMSG, TIB_SASS, RV, ...).  The Java
 * binding is the reference for this class; methods keep the Java names. */
public class RaiMsg : IDisposable {
  /* field types */
  public const int RAIMSG_NODATA   = 0;
  public const int RAIMSG_MESSAGE  = 1;
  public const int RAIMSG_STRING   = 2;
  public const int RAIMSG_OPAQUE   = 3;
  public const int RAIMSG_BOOLEAN  = 4;
  public const int RAIMSG_INT      = 5;
  public const int RAIMSG_UINT     = 6;
  public const int RAIMSG_REAL     = 7;
  public const int RAIMSG_ARRAY    = 8;
  public const int RAIMSG_PARTIAL  = 9;
  public const int RAIMSG_IPDATA   = 10;
  public const int RAIMSG_MAXVALID = 11;
  /* protocols */
  public const int RAIMSG_PROTO             = 0; /* tibmsg self describing */
  public const int RV_SASS_PROTO            = 1;
  public const int TIB_SASS_PROTO           = 2; /* sass with tib header */
  public const int TIB_SASS_FORM_PROTO      = 3; /* sass with fixed field offset */
  public const int RV_RAIMSG_PROTO          = 4;
  public const int XREP_PROTO               = 5;
  public const int RV_PROTO                 = 6; /* rv self describing */
  public const int CISERVER_SASS_PROTO      = 7;
  public const int CISERVER_SASS_FORM_PROTO = 8;
  /* common error codes (RaiMsgException.getErrno()) */
  public const int BAD_ARG          = 1;
  public const int BAD_MAGIC_NUMBER = 2;
  public const int NOT_FOUND        = 9;
  public const int NO_FIELD         = 11;
  public const int BAD_CVT_STRING   = 43;
  public const int BAD_CVT_BOOL     = 44;
  public const int BAD_CVT_INT      = 45;
  public const int BAD_CVT_REAL     = 46;
  /* PrintXML attribute flags */
  public const int ADD_TYPE_ATTR         = 0x1;
  public const int ADD_SIZE_ATTR         = 0x2;
  public const int ADD_FID_ATTR          = 0x4;
  public const int ADD_PARTIAL_OFF_ATTR  = 0x8;
  public const int ADD_ARRAY_COUNT_ATTR  = 0x10;
  public const int ADD_ARRAY_TYPE_ATTR   = 0x20;
  public const int ADD_ARRAY_ELSIZE_ATTR = 0x40;
  public const int ADD_HINT_ATTR         = 0x80;
  public const int ADD_ALL_ATTRS         = 0x3ff;

  internal IntPtr msg;
  readonly bool   owned;

  public static string RaiMsgVersion() { return Native.Str( Native.rai_msg_version() ); }

  /** Create an empty message using the RAIMSG_PROTO */
  public RaiMsg() : this( RAIMSG_PROTO ) {}
  /** Create an empty message using the protocol */
  public RaiMsg( int proto ) {
    IntPtr m;
    RaiException.Check( Native.rai_msg_create( proto, out m ) );
    this.msg = m; this.owned = true;
  }
  /* wrap a native message owned by someone else (callbacks, NewRaiMsg) */
  internal RaiMsg( IntPtr m,  bool owned ) { this.msg = m; this.owned = owned; }

  ~RaiMsg() { this.Dispose( false ); }
  public void Dispose() { this.Dispose( true ); GC.SuppressFinalize( this ); }
  void Dispose( bool disposing ) {
    if ( this.msg != IntPtr.Zero && this.owned ) {
      Native.rai_msg_delete( this.msg );
    }
    this.msg = IntPtr.Zero;
  }
  internal IntPtr getMsgHandle() { return this.msg; }

  public void ReUse( int proto ) { RaiException.Check( Native.rai_msg_reuse( this.msg, proto ) ); }
  public void ReUse() { RaiException.Check( Native.rai_msg_reuse( this.msg, -1 ) ); }
  public void SetProtocol( int proto ) { Native.rai_msg_set_protocol( this.msg, proto ); }
  public int GetProtocol() { return Native.rai_msg_get_protocol( this.msg ); }
  public string GetProtocolString() { return Native.Str( Native.rai_msg_get_protocol_string( this.msg ) ); }

  /* SASS header helpers */
  public static string MsgTypeToString( short msgType ) {
    StringBuilder b = new StringBuilder( 32 );
    return Native.Str( Native.rai_msg_type_to_string( (ushort) msgType, b, 32 ) );
  }
  public static short StringToMsgType( string s ) { return (short) Native.rai_msg_string_to_type( s ); }
  public static string RecStatusToString( short recStatus ) {
    StringBuilder b = new StringBuilder( 32 );
    return Native.Str( Native.rai_rec_status_to_string( (ushort) recStatus, b, 32 ) );
  }
  public static short StringToRecStatus( string s ) { return (short) Native.rai_string_to_rec_status( s ); }
  public static string RecTypeToString( short recType ) {
    IntPtr s; RaiException.Check( Native.rai_msg_rec_type_to_string( (ushort) recType, out s ) );
    return Native.Str( s );
  }
  public static short StringToRecType( string s ) {
    ushort r; RaiException.Check( Native.rai_msg_string_to_rec_type( s, out r ) );
    return (short) r;
  }
  string HdrString( string f ) {
    IntPtr s; RaiException.Check( Native.rai_msg_get_hdr_string( this.msg, f, out s ) );
    return Native.Str( s );
  }
  public string GetMsgTypeString() { return this.HdrString( "MSG_TYPE" ); }
  public void SetMsgTypeString( string s ) { RaiException.Check( Native.rai_msg_set_hdr_string( this.msg, "MSG_TYPE", s ) ); }
  public string GetRecTypeString() { return this.HdrString( "REC_TYPE" ); }
  public void SetRecTypeString( string s ) { RaiException.Check( Native.rai_msg_set_hdr_string( this.msg, "REC_TYPE", s ) ); }
  public string GetRecStatusString() { return this.HdrString( "REC_STATUS" ); }
  public void SetRecStatusString( string s ) { RaiException.Check( Native.rai_msg_set_hdr_string( this.msg, "REC_STATUS", s ) ); }

  public void ClearForm() { RaiException.Check( Native.rai_msg_clear_form( this.msg ) ); }
  public void Release() { RaiException.Check( Native.rai_msg_release( this.msg ) ); }
  public void Copy( RaiMsg from ) { RaiException.Check( Native.rai_msg_copy( this.msg, from.msg ) ); }

  /* ---- typed getters, throw RaiMsgException NOT_FOUND when missing ---- */
  /** Get a field value as an object of the natural type: bool, sbyte..long,
   * float, double, string, byte[] (opaque), Partial, RaiMsg (sub message) or
   * an array of those */
  public object Get( string name ) {
    using ( RaiField f = new RaiField() ) {
      if ( ! f.Find( this, name ) )
        throw new RaiMsgException( "field not found: " + name );
      return f.Get();
    }
  }
  public bool GetBoolean( string name ) { int v; RaiException.Check( Native.rai_msg_get_bool( this.msg, name, out v ) ); return v != 0; }
  public sbyte GetByte( string name ) { sbyte v; RaiException.Check( Native.rai_msg_get_i8( this.msg, name, out v ) ); return v; }
  public short GetShort( string name ) { short v; RaiException.Check( Native.rai_msg_get_i16( this.msg, name, out v ) ); return v; }
  public int GetInt( string name ) { int v; RaiException.Check( Native.rai_msg_get_i32( this.msg, name, out v ) ); return v; }
  public long GetLong( string name ) { long v; RaiException.Check( Native.rai_msg_get_i64( this.msg, name, out v ) ); return v; }
  public float GetFloat( string name ) { float v; RaiException.Check( Native.rai_msg_get_f32( this.msg, name, out v ) ); return v; }
  public double GetDouble( string name ) { double v; RaiException.Check( Native.rai_msg_get_f64( this.msg, name, out v ) ); return v; }
  public string GetString( string name ) {
    IntPtr s; uint len;
    RaiException.Check( Native.rai_msg_get_string( this.msg, name, out s, out len ) );
    return Native.Str( s, len );
  }
  public byte[] GetOpaque( string name ) {
    IntPtr p; uint len;
    RaiException.Check( Native.rai_msg_get_opaque( this.msg, name, out p, out len ) );
    return Native.Bytes( p, len );
  }
  public Partial GetPartial( string name ) {
    using ( RaiField f = new RaiField() ) {
      if ( ! f.Find( this, name ) ) throw new RaiMsgException( "field not found: " + name );
      return f.GetPartial();
    }
  }
  public bool[] GetBooleanArray( string name ) { using ( RaiField f = this.Fld( name ) ) return f.GetBooleanArray(); }
  public sbyte[] GetByteArray( string name ) { using ( RaiField f = this.Fld( name ) ) return f.GetByteArray(); }
  public short[] GetShortArray( string name ) { using ( RaiField f = this.Fld( name ) ) return f.GetShortArray(); }
  public int[] GetIntArray( string name ) { using ( RaiField f = this.Fld( name ) ) return f.GetIntArray(); }
  public long[] GetLongArray( string name ) { using ( RaiField f = this.Fld( name ) ) return f.GetLongArray(); }
  public float[] GetFloatArray( string name ) { using ( RaiField f = this.Fld( name ) ) return f.GetFloatArray(); }
  public double[] GetDoubleArray( string name ) { using ( RaiField f = this.Fld( name ) ) return f.GetDoubleArray(); }
  public string[] GetStringArray( string name ) { using ( RaiField f = this.Fld( name ) ) return f.GetStringArray(); }
  RaiField Fld( string name ) {
    RaiField f = new RaiField();
    if ( ! f.Find( this, name ) ) { f.Dispose(); throw new RaiMsgException( "field not found: " + name ); }
    return f;
  }

  /* ---- append ---- */
  /** Append a field, choosing the type from the object: bool, sbyte, byte,
   * short, ushort, int, uint, long, ulong, float, double, string, byte[],
   * Partial, RaiMsg */
  public void Append( string name,  object val ) { this.Put( name, val, false ); }
  public void AppendUnsigned( string name,  object val ) { this.Put( name, val, false, true ); }
  public void AppendOpaque( string name,  byte[] buf ) { RaiException.Check( Native.rai_msg_append_opaque( this.msg, name, buf, (uint) buf.Length ) ); }
  public void AppendOpaque( string name,  byte[] buf,  int off,  int len ) {
    byte[] b = new byte[ len ]; Array.Copy( buf, off, b, 0, len ); this.AppendOpaque( name, b );
  }
  public void Append( RaiField val ) { RaiException.Check( Native.rai_msg_append_field( this.msg, val.fld ) ); }
  public void AppendBoolean( string n,  bool v ) { RaiException.Check( Native.rai_msg_append_bool( this.msg, n, v ? 1 : 0 ) ); }
  public void AppendByte( string n,  sbyte v ) { RaiException.Check( Native.rai_msg_append_i8( this.msg, n, v, 0 ) ); }
  public void AppendUByte( string n,  byte v ) { RaiException.Check( Native.rai_msg_append_i8( this.msg, n, (sbyte) v, 1 ) ); }
  public void AppendShort( string n,  short v ) { RaiException.Check( Native.rai_msg_append_i16( this.msg, n, v, 0 ) ); }
  public void AppendUShort( string n,  ushort v ) { RaiException.Check( Native.rai_msg_append_i16( this.msg, n, (short) v, 1 ) ); }
  public void AppendUShort( string n,  short v ) { RaiException.Check( Native.rai_msg_append_i16( this.msg, n, v, 1 ) ); }
  public void AppendInt( string n,  int v ) { RaiException.Check( Native.rai_msg_append_i32( this.msg, n, v, 0 ) ); }
  public void AppendUInt( string n,  uint v ) { RaiException.Check( Native.rai_msg_append_i32( this.msg, n, (int) v, 1 ) ); }
  public void AppendLong( string n,  long v ) { RaiException.Check( Native.rai_msg_append_i64( this.msg, n, v, 0 ) ); }
  public void AppendULong( string n,  ulong v ) { RaiException.Check( Native.rai_msg_append_i64( this.msg, n, (long) v, 1 ) ); }
  public void AppendULong( string n,  long v ) { RaiException.Check( Native.rai_msg_append_i64( this.msg, n, v, 1 ) ); }
  public void AppendFloat( string n,  float v ) { RaiException.Check( Native.rai_msg_append_f32( this.msg, n, v ) ); }
  public void AppendDouble( string n,  double v ) { RaiException.Check( Native.rai_msg_append_f64( this.msg, n, v ) ); }
  public void AppendString( string n,  string v ) { RaiException.Check( Native.rai_msg_append_string( this.msg, n, v ) ); }
  public void AppendPartial( string n,  Partial p ) {
    RaiException.Check( Native.rai_msg_append_partial( this.msg, n, p.getData(), (uint) p.getData().Length, (uint) p.getOffset() ) );
  }
  public void AppendMsg( string n,  RaiMsg sub ) { RaiException.Check( Native.rai_msg_append_msg( this.msg, n, sub.msg ) ); }

  /* ---- update (replace in place, or append when missing) ---- */
  public void Update( string name,  object val ) { this.Put( name, val, true ); }
  public void UpdateUnsigned( string name,  object val ) { this.Put( name, val, true, true ); }
  public void UpdateOpaque( string name,  byte[] buf ) { RaiException.Check( Native.rai_msg_update_opaque( this.msg, name, buf, (uint) buf.Length ) ); }
  public void UpdateOpaque( string name,  byte[] buf,  int off,  int len ) {
    byte[] b = new byte[ len ]; Array.Copy( buf, off, b, 0, len ); this.UpdateOpaque( name, b );
  }
  public void Update( RaiField val ) { RaiException.Check( Native.rai_msg_update_field( this.msg, val.fld ) ); }
  public void UpdateBoolean( string n,  bool v ) { RaiException.Check( Native.rai_msg_update_bool( this.msg, n, v ? 1 : 0 ) ); }
  public void UpdateByte( string n,  sbyte v ) { RaiException.Check( Native.rai_msg_update_i8( this.msg, n, v, 0 ) ); }
  public void UpdateUByte( string n,  byte v ) { RaiException.Check( Native.rai_msg_update_i8( this.msg, n, (sbyte) v, 1 ) ); }
  public void UpdateShort( string n,  short v ) { RaiException.Check( Native.rai_msg_update_i16( this.msg, n, v, 0 ) ); }
  public void UpdateUShort( string n,  ushort v ) { RaiException.Check( Native.rai_msg_update_i16( this.msg, n, (short) v, 1 ) ); }
  public void UpdateUShort( string n,  short v ) { RaiException.Check( Native.rai_msg_update_i16( this.msg, n, v, 1 ) ); }
  public void UpdateInt( string n,  int v ) { RaiException.Check( Native.rai_msg_update_i32( this.msg, n, v, 0 ) ); }
  public void UpdateUInt( string n,  uint v ) { RaiException.Check( Native.rai_msg_update_i32( this.msg, n, (int) v, 1 ) ); }
  public void UpdateLong( string n,  long v ) { RaiException.Check( Native.rai_msg_update_i64( this.msg, n, v, 0 ) ); }
  public void UpdateULong( string n,  ulong v ) { RaiException.Check( Native.rai_msg_update_i64( this.msg, n, (long) v, 1 ) ); }
  public void UpdateULong( string n,  long v ) { RaiException.Check( Native.rai_msg_update_i64( this.msg, n, v, 1 ) ); }
  public void UpdateFloat( string n,  float v ) { RaiException.Check( Native.rai_msg_update_f32( this.msg, n, v ) ); }
  public void UpdateDouble( string n,  double v ) { RaiException.Check( Native.rai_msg_update_f64( this.msg, n, v ) ); }
  public void UpdateString( string n,  string v ) { RaiException.Check( Native.rai_msg_update_string( this.msg, n, v ) ); }

  void Put( string n,  object val,  bool update,  bool unsigned = false ) {
    if ( val == null ) throw new RaiMsgException( "null value for field " + n );
    IntPtr e;
    int u = unsigned ? 1 : 0;
    switch ( Type.GetTypeCode( val.GetType() ) ) {
      case TypeCode.Boolean: e = update ? Native.rai_msg_update_bool( this.msg, n, (bool) val ? 1 : 0 ) : Native.rai_msg_append_bool( this.msg, n, (bool) val ? 1 : 0 ); break;
      case TypeCode.SByte:   e = update ? Native.rai_msg_update_i8( this.msg, n, (sbyte) val, u ) : Native.rai_msg_append_i8( this.msg, n, (sbyte) val, u ); break;
      case TypeCode.Byte:    e = update ? Native.rai_msg_update_i8( this.msg, n, (sbyte)(byte) val, 1 ) : Native.rai_msg_append_i8( this.msg, n, (sbyte)(byte) val, 1 ); break;
      case TypeCode.Int16:   e = update ? Native.rai_msg_update_i16( this.msg, n, (short) val, u ) : Native.rai_msg_append_i16( this.msg, n, (short) val, u ); break;
      case TypeCode.UInt16:  e = update ? Native.rai_msg_update_i16( this.msg, n, (short)(ushort) val, 1 ) : Native.rai_msg_append_i16( this.msg, n, (short)(ushort) val, 1 ); break;
      case TypeCode.Int32:   e = update ? Native.rai_msg_update_i32( this.msg, n, (int) val, u ) : Native.rai_msg_append_i32( this.msg, n, (int) val, u ); break;
      case TypeCode.UInt32:  e = update ? Native.rai_msg_update_i32( this.msg, n, (int)(uint) val, 1 ) : Native.rai_msg_append_i32( this.msg, n, (int)(uint) val, 1 ); break;
      case TypeCode.Int64:   e = update ? Native.rai_msg_update_i64( this.msg, n, (long) val, u ) : Native.rai_msg_append_i64( this.msg, n, (long) val, u ); break;
      case TypeCode.UInt64:  e = update ? Native.rai_msg_update_i64( this.msg, n, (long)(ulong) val, 1 ) : Native.rai_msg_append_i64( this.msg, n, (long)(ulong) val, 1 ); break;
      case TypeCode.Single:  e = update ? Native.rai_msg_update_f32( this.msg, n, (float) val ) : Native.rai_msg_append_f32( this.msg, n, (float) val ); break;
      case TypeCode.Double:  e = update ? Native.rai_msg_update_f64( this.msg, n, (double) val ) : Native.rai_msg_append_f64( this.msg, n, (double) val ); break;
      case TypeCode.String:  e = update ? Native.rai_msg_update_string( this.msg, n, (string) val ) : Native.rai_msg_append_string( this.msg, n, (string) val ); break;
      default:
        if ( val is byte[] ) { byte[] b = (byte[]) val; e = update ? Native.rai_msg_update_opaque( this.msg, n, b, (uint) b.Length ) : Native.rai_msg_append_opaque( this.msg, n, b, (uint) b.Length ); }
        else if ( val is Partial ) { if ( update ) throw new RaiMsgException( "update partial not supported" ); this.AppendPartial( n, (Partial) val ); return; }
        else if ( val is RaiMsg ) { if ( update ) throw new RaiMsgException( "update sub message not supported" ); this.AppendMsg( n, (RaiMsg) val ); return; }
        else throw new RaiMsgException( "unsupported field type " + val.GetType().Name + " for " + n );
        break;
    }
    RaiException.Check( e );
  }

  /* ---- pack / unpack ---- */
  public void UnPack( byte[] msgBuf ) { this.UnPack( msgBuf, 0, msgBuf.Length ); }
  public void UnPack( byte[] msgBuf,  int off,  int len ) {
    GCHandle h = GCHandle.Alloc( msgBuf, GCHandleType.Pinned );
    try {
      IntPtr p = IntPtr.Add( h.AddrOfPinnedObject(), off );
      RaiException.Check( Native.rai_msg_unpack( this.msg, p, (uint) len ) );
    } finally {
      /* the message copies the buffer on unpack only when it must own it;
       * keep the pin until Release()/Dispose() would be safer, but the C++
       * api copies into its own memory for dynamic messages */
      h.Free();
    }
  }
  public int Pack( byte[] msgBuf,  int off,  int len ) {
    GCHandle h = GCHandle.Alloc( msgBuf, GCHandleType.Pinned );
    try {
      uint size;
      RaiException.Check( Native.rai_msg_pack( this.msg, IntPtr.Add( h.AddrOfPinnedObject(), off ), (uint) len, out size ) );
      return (int) size;
    } finally { h.Free(); }
  }
  public int Pack( byte[] msgBuf,  int off ) { return this.Pack( msgBuf, off, msgBuf.Length - off ); }
  public int Pack( byte[] msgBuf ) { return this.Pack( msgBuf, 0, msgBuf.Length ); }
  /** A copy of the packed message */
  public byte[] Pack() {
    IntPtr p; uint size;
    RaiException.Check( Native.rai_msg_packed( this.msg, out p, out size ) );
    return Native.Bytes( p, size );
  }
  public byte[] Packed() { return this.Pack(); }
  public int PackSize() { uint s; RaiException.Check( Native.rai_msg_pack_size( this.msg, out s ) ); return (int) s; }

  public bool Activate( string name ) { int ok; RaiException.Check( Native.rai_msg_activate( this.msg, name, out ok ) ); return ok != 0; }
  public bool Rename( string oldName,  string newName ) { int ok; RaiException.Check( Native.rai_msg_rename( this.msg, oldName, newName, out ok ) ); return ok != 0; }
  public bool Remove( string name ) { int ok; RaiException.Check( Native.rai_msg_remove( this.msg, name, out ok ) ); return ok != 0; }

  /* ---- printing ---- */
  public void Print( Stream o ) { this.Print( o, true, null, true, null, null ); }
  public void Print( TextWriter o ) { this.Print( o, true, null, true, null, null ); }
  public void Print( Stream o,  bool field_newlines,  string fname_format,  bool print_opaques,  string debug_format,  string debug_hformat ) {
    using ( WriteAdapter w = new WriteAdapter( o ) )
      RaiException.Check( Native.rai_msg_print( this.msg, w.Fn, w.Closure, field_newlines ? 1 : 0, fname_format, print_opaques ? 1 : 0, debug_format, debug_hformat ) );
  }
  public void Print( TextWriter o,  bool field_newlines,  string fname_format,  bool print_opaques,  string debug_format,  string debug_hformat ) {
    using ( WriteAdapter w = new WriteAdapter( o ) )
      RaiException.Check( Native.rai_msg_print( this.msg, w.Fn, w.Closure, field_newlines ? 1 : 0, fname_format, print_opaques ? 1 : 0, debug_format, debug_hformat ) );
  }
  public void PrintHex( Stream o ) {
    using ( WriteAdapter w = new WriteAdapter( o ) )
      RaiException.Check( Native.rai_msg_print_hex( this.msg, w.Fn, w.Closure ) );
  }
  public void PrintHex( TextWriter o ) {
    using ( WriteAdapter w = new WriteAdapter( o ) )
      RaiException.Check( Native.rai_msg_print_hex( this.msg, w.Fn, w.Closure ) );
  }
  public static void PrintHex( TextWriter o,  byte[] msgBuf ) { PrintHex( o, msgBuf, 0, msgBuf.Length ); }
  public static void PrintHex( TextWriter o,  byte[] msgBuf,  int off,  int len ) {
    GCHandle h = GCHandle.Alloc( msgBuf, GCHandleType.Pinned );
    try {
      using ( WriteAdapter w = new WriteAdapter( o ) )
        RaiException.Check( Native.rai_msg_print_hex_buf( IntPtr.Add( h.AddrOfPinnedObject(), off ), (uint) len, w.Fn, w.Closure ) );
    } finally { h.Free(); }
  }
  public void PrintXML( TextWriter o ) { this.PrintXML( o, 0, true ); }
  public void PrintXML( TextWriter o,  int attr_flags,  bool print_newlines ) {
    using ( WriteAdapter w = new WriteAdapter( o ) )
      RaiException.Check( Native.rai_msg_print_xml( this.msg, w.Fn, w.Closure, attr_flags, print_newlines ? 1 : 0 ) );
  }
  public override string ToString() {
    StringWriter sw = new StringWriter();
    try { this.Print( sw ); } catch ( RaiException e ) { sw.Write( "<" + e.Message + ">" ); }
    return sw.ToString();
  }
}

/** RaiField is a cursor over a message field: name, type, size, value, and
 * iteration (First / Next / Find).  The data it points at belongs to the
 * message and is valid only while the message is unchanged. */
public class RaiField : IDisposable {
  internal IntPtr fld;

  public RaiField() {
    this.fld = Native.rai_field_create();
    if ( this.fld == IntPtr.Zero ) throw new RaiMsgException( "unable to create field" );
  }
  ~RaiField() { this.Dispose( false ); }
  public void Dispose() { this.Dispose( true ); GC.SuppressFinalize( this ); }
  void Dispose( bool d ) {
    if ( this.fld != IntPtr.Zero ) { Native.rai_field_delete( this.fld ); this.fld = IntPtr.Zero; }
  }

  public string Name() { return Native.Str( Native.rai_field_name( this.fld ) ); }
  public int Type() { return Native.rai_field_type( this.fld ); }
  public int Size() { return (int) Native.rai_field_size( this.fld ); }
  public int HintType() { return Native.rai_field_hint_type( this.fld ); }
  public int HintSize() { return (int) Native.rai_field_hint_size( this.fld ); }
  public int EntryType() { return Native.rai_field_entry_type( this.fld ); }
  public int EntrySize() { return (int) Native.rai_field_entry_size( this.fld ); }
  public int NumEntries() { return (int) Native.rai_field_num_entries( this.fld ); }
  public int Offset() { return (int) Native.rai_field_offset( this.fld ); }
  public bool Fid( out ushort fid ) { return Native.rai_field_fid( this.fld, out fid ) != 0; }
  public static string TypeToString( int type ) { return Native.Str( Native.rai_field_type_string( type ) ); }

  /** The value as the natural object type (see RaiMsg.Get) */
  public object Get() {
    int t = this.Type(), sz = this.Size();
    switch ( t ) {
      case RaiMsg.RAIMSG_BOOLEAN: return this.GetBoolean();
      case RaiMsg.RAIMSG_INT:
        switch ( sz ) { case 1: return this.GetByte(); case 2: return this.GetShort(); case 4: return this.GetInt(); default: return this.GetLong(); }
      case RaiMsg.RAIMSG_UINT:
        switch ( sz ) { case 1: return (byte) this.GetByte(); case 2: return (ushort) this.GetShort(); case 4: return (uint) this.GetInt(); default: return (ulong) this.GetLong(); }
      case RaiMsg.RAIMSG_REAL: return sz == 4 ? (object) this.GetFloat() : (object) this.GetDouble();
      case RaiMsg.RAIMSG_STRING: return this.GetString();
      case RaiMsg.RAIMSG_OPAQUE: case RaiMsg.RAIMSG_IPDATA: return this.GetOpaque();
      case RaiMsg.RAIMSG_PARTIAL: return this.GetPartial();
      case RaiMsg.RAIMSG_MESSAGE: { RaiMsg m = new RaiMsg(); this.GetMsg( m ); return m; }
      case RaiMsg.RAIMSG_ARRAY:
        switch ( this.EntryType() ) {
          case RaiMsg.RAIMSG_BOOLEAN: return this.GetBooleanArray();
          case RaiMsg.RAIMSG_INT: case RaiMsg.RAIMSG_UINT:
            switch ( this.EntrySize() ) { case 1: return this.GetByteArray(); case 2: return this.GetShortArray(); case 4: return this.GetIntArray(); default: return this.GetLongArray(); }
          case RaiMsg.RAIMSG_REAL: return this.EntrySize() == 4 ? (object) this.GetFloatArray() : (object) this.GetDoubleArray();
          default: return this.GetStringArray();
        }
      default: return null;
    }
  }
  public bool GetBoolean() { int v; RaiException.Check( Native.rai_field_get_bool( this.fld, out v ) ); return v != 0; }
  public sbyte GetByte() { sbyte v; RaiException.Check( Native.rai_field_get_i8( this.fld, out v ) ); return v; }
  public short GetShort() { short v; RaiException.Check( Native.rai_field_get_i16( this.fld, out v ) ); return v; }
  public int GetInt() { int v; RaiException.Check( Native.rai_field_get_i32( this.fld, out v ) ); return v; }
  public long GetLong() { long v; RaiException.Check( Native.rai_field_get_i64( this.fld, out v ) ); return v; }
  public float GetFloat() { float v; RaiException.Check( Native.rai_field_get_f32( this.fld, out v ) ); return v; }
  public double GetDouble() { double v; RaiException.Check( Native.rai_field_get_f64( this.fld, out v ) ); return v; }
  public string GetString() { IntPtr s; uint len; RaiException.Check( Native.rai_field_get_string( this.fld, out s, out len ) ); return Native.Str( s, len ); }
  public byte[] GetOpaque() { return Native.Bytes( Native.rai_field_data( this.fld ), Native.rai_field_size( this.fld ) ); }
  public Partial GetPartial() { return new Partial( this.GetOpaque(), this.Offset() ); }
  public void GetMsg( RaiMsg sub ) { RaiException.Check( Native.rai_field_get_msg( this.fld, sub.msg ) ); }

  long Entry( uint i ) { long v; RaiException.Check( Native.rai_field_get_entry_i64( this.fld, i, out v ) ); return v; }
  double EntryF( uint i ) { double v; RaiException.Check( Native.rai_field_get_entry_f64( this.fld, i, out v ) ); return v; }
  public bool[] GetBooleanArray() { int n = this.NumEntries(); bool[] a = new bool[ n ]; for ( uint i = 0; i < n; i++ ) a[ i ] = this.Entry( i ) != 0; return a; }
  public sbyte[] GetByteArray() { int n = this.NumEntries(); sbyte[] a = new sbyte[ n ]; for ( uint i = 0; i < n; i++ ) a[ i ] = (sbyte) this.Entry( i ); return a; }
  public short[] GetShortArray() { int n = this.NumEntries(); short[] a = new short[ n ]; for ( uint i = 0; i < n; i++ ) a[ i ] = (short) this.Entry( i ); return a; }
  public int[] GetIntArray() { int n = this.NumEntries(); int[] a = new int[ n ]; for ( uint i = 0; i < n; i++ ) a[ i ] = (int) this.Entry( i ); return a; }
  public long[] GetLongArray() { int n = this.NumEntries(); long[] a = new long[ n ]; for ( uint i = 0; i < n; i++ ) a[ i ] = this.Entry( i ); return a; }
  public float[] GetFloatArray() { int n = this.NumEntries(); float[] a = new float[ n ]; for ( uint i = 0; i < n; i++ ) a[ i ] = (float) this.EntryF( i ); return a; }
  public double[] GetDoubleArray() { int n = this.NumEntries(); double[] a = new double[ n ]; for ( uint i = 0; i < n; i++ ) a[ i ] = this.EntryF( i ); return a; }
  public string[] GetStringArray() {
    int n = this.NumEntries(); string[] a = new string[ n ];
    for ( uint i = 0; i < n; i++ ) { IntPtr s; uint len; RaiException.Check( Native.rai_field_get_entry_string( this.fld, i, out s, out len ) ); a[ i ] = Native.Str( s, len ); }
    return a;
  }

  /* iteration */
  public bool Find( RaiMsg msg,  string name ) { int f; RaiException.Check( Native.rai_field_find( this.fld, msg.msg, name, out f ) ); return f != 0; }
  public bool First( RaiMsg msg ) { int m; RaiException.Check( Native.rai_field_first( this.fld, msg.msg, out m ) ); return m != 0; }
  public bool Next() { int m; RaiException.Check( Native.rai_field_next( this.fld, out m ) ); return m != 0; }
}

} /* namespace */
