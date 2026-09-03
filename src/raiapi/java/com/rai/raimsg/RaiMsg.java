package com.rai.raimsg;

/** RaiMsg is a messaging interface for SASS Qforms, RaiMsg/TibMsg, and RV Msg */
public class RaiMsg {
  /* RaiMsgType */
  /** Field with NO_DATA, used to initialize */
  public static final int RAIMSG_NODATA   = 0;
  /** Field with a sub message */
  public static final int RAIMSG_MESSAGE  = 1;
  /** Field string type */
  public static final int RAIMSG_STRING   = 2;
  /** Field opaque type */
  public static final int RAIMSG_OPAQUE   = 3;
  /** Field boolean type */
  public static final int RAIMSG_BOOLEAN  = 4;
  /** Field int type */
  public static final int RAIMSG_INT      = 5;
  /** Field unsigned int type */
  public static final int RAIMSG_UINT     = 6;
  /** Field real floating point type */
  public static final int RAIMSG_REAL     = 7;
  /** Field array type, all the elements are the same type */
  public static final int RAIMSG_ARRAY    = 8;
  /** A field with a partial update, string or opaque */
  public static final int RAIMSG_PARTIAL  = 9;
  /** A field with network ordered bytes */
  public static final int RAIMSG_IPDATA   = 10;
  /** Last enum for range checking */
  public static final int RAIMSG_MAXVALID = 11;
  /* RaiMsgProtocol */
  /** RaiMsg format is selfdescribing, equivalent to TibMsg */
  public static final int RAIMSG_PROTO             = 0; /* tibmsg self describing */
  /** Not used */
  public static final int RV_SASS_PROTO            = 1; /* XXX t.b.d */
  /** A SASS Qform */
  public static final int TIB_SASS_PROTO           = 2; /* sass with tib header */
  /** A SASS Qform which contains all fields of the form */
  public static final int TIB_SASS_FORM_PROTO      = 3; /* sass with fixed field offset */
  /** Not used */
  public static final int RV_RAIMSG_PROTO          = 4; /* XXX t.b.d */
  /** Not used */
  public static final int XREP_PROTO               = 5; /* XXX t.b.d */
  /** RV format is selfdescribing, without some SASS field types */
  public static final int RV_PROTO                 = 6; /* rv self describing */
  /** A SASS Qform with a ciServer header */
  public static final int CISERVER_SASS_PROTO      = 7; /* sass with ciServer header */
  /** A SASS Qform with a ciServer header, contains all form fields */
  public static final int CISERVER_SASS_FORM_PROTO = 8; /* sass with fixed field offset */
  /* RaiMsgException status codes */
  /** RaiMsgException status, NULL arg or invalid input */
  public static final int BAD_ARG          = 1;
  /** RaiMsgException status, Packet data UnPacked bad */
  public static final int BAD_MAGIC_NUMBER = 2;
  /** RaiMsgException status, Message field not found */
  public static final int NOT_FOUND        = 9;
  /** RaiMsgException status, Message or field empty */
  public static final int NO_FIELD         = 11;
  /** RaiMsgException status, Can't convert to string */
  public static final int BAD_CVT_STRING   = 43;
  /** RaiMsgException status, Can't convert to boolean */
  public static final int BAD_CVT_BOOL     = 44;
  /** RaiMsgException status, Can't convert to int */
  public static final int BAD_CVT_INT      = 45;
  /** RaiMsgException status, Can't convert to real */
  public static final int BAD_CVT_REAL     = 46;

  long msg;

  public static String RaiMsgVersion() {
    String version = "RaiMsg Version 2.2 Java. Jul 27, 2008";
    return version;
  }

  public RaiMsg() {
    this.msg = Create( RAIMSG_PROTO );
  }
  public RaiMsg( int proto ) {
    this.msg = Create( proto );
  }
  protected RaiMsg( long m ) {
    this.msg = m;
  }
  private static native long Create( int proto );

  protected void finalize() {
    Delete( this.msg );
  }
  private static native void Delete( long msg );

  /**
   * Used internally to get C++ message handle from class */
  protected long getMsgHandle() {
    return this.msg;
  }
  /**
   * Erase message and set new RaiMsgProtocol type */
  public void ReUse( int proto ) throws RaiMsgException {
    ReUse( this.msg, proto );
  }
  private static native void ReUse( long msg,  int proto ) throws RaiMsgException;

  /**
   * Erase message and leave RaiMsgProtocol the same */
  public void ReUse() throws RaiMsgException {
    ReUse( this.msg );
  }
  private static native void ReUse( long msg ) throws RaiMsgException;

  /**
   * RaiMsgProtocol functions */
  public void SetProtocol( int proto ) throws RaiMsgException {
    ReUse( this.msg, proto );
  }
  public int GetProtocol() {
    return GetProtocol( this.msg );
  }
  private static native int GetProtocol( long msg );

  public String GetProtocolString() {
    return GetProtocolString( this.msg );
  }
  private static native String GetProtocolString( long msg );

  /**
   * MSG_TYPE functions */
  public static native String MsgTypeToString( short msgType );

  public static native short StringToMsgType( String s );

  public String GetMsgTypeString() throws RaiMsgException {
    return GetMsgTypeString( this.msg );
  }
  private static native String GetMsgTypeString( long msg ) throws RaiMsgException;

  public void SetMsgTypeString( String s ) throws RaiMsgException {
    SetMsgTypeString( this.msg, s );
  }
  private static native void SetMsgTypeString( long msg, String s ) throws RaiMsgException;

  /**
   * REC_TYPE functions */
  public static native String RecTypeToString( short recType ) throws RaiMsgException;

  public static native short StringToRecType( String s ) throws RaiMsgException;

  public String GetRecTypeString() throws RaiMsgException {
    return GetRecTypeString( this.msg );
  }
  private static native String GetRecTypeString( long msg ) throws RaiMsgException;

  public void SetRecTypeString( String s ) throws RaiMsgException {
    SetRecTypeString( this.msg, s );
  }
  private static native void SetRecTypeString( long msg, String s ) throws RaiMsgException;

  /**
   * REC_STATUS functions */
  public static native String RecStatusToString( short recStatus );

  public static native short StringToRecStatus( String s );

  public String GetRecStatusString() throws RaiMsgException {
    return GetRecStatusString( this.msg );
  }
  private static native String GetRecStatusString( long msg ) throws RaiMsgException;

  public void SetRecStatusString( String s ) throws RaiMsgException {
    SetRecStatusString( this.msg, s );
  }
  private static native void SetRecStatusString( long msg, String s ) throws RaiMsgException;

  /**
   * Initializing a message with a zero filled form, the RaiMsg Protocol should
   * be TIB_SASS_PROTO and the REC_TYPE should be set prior to clearing the
   * form;  Only the MSG_TYPE, REC_TYPE, SEQ_NO, REC_STATUS values are
   * preserved, all other values are set to zero */
  public void ClearForm() throws RaiMsgException {
    ClearForm( this.msg );
  }
  private static native void ClearForm( long msg ) throws RaiMsgException;

  /**
   * Release any memory used by the C++ message */
  public void Release() throws RaiMsgException {
    Release( this.msg );
  }
  private static native void Release( long msg ) throws RaiMsgException;

  /**
   * Get( fname ) returns Object:  Boolean, Byte, Short, Integer, Long, Float,
   * Double, String, Partial, boolean [], byte [], short [], int [], long [],
   * float [], double [], String [] */
  public Object Get( String name ) throws RaiMsgException {
    return Get( this.msg, name );
  }
  private static native Object Get( long msg,  String name ) throws RaiMsgException;

  /**
   * Get boolean, cast if necessary */
  public boolean GetBoolean( String name ) throws RaiMsgException {
    return GetBoolean( this.msg, name );
  }
  private static native boolean GetBoolean( long msg,  String name ) throws RaiMsgException;

  /**
   * Get byte, cast if necessary */
  public byte GetByte( String name ) throws RaiMsgException {
    return GetByte( this.msg, name );
  }
  private static native byte GetByte( long msg,  String name ) throws RaiMsgException;

  /**
   * Get short, cast if necessary */
  public short GetShort( String name ) throws RaiMsgException {
    return GetShort( this.msg, name );
  }
  private static native short GetShort( long msg,  String name ) throws RaiMsgException;

  /**
   * Get int, cast if necessary */
  public int GetInt( String name ) throws RaiMsgException {
    return GetInt( this.msg, name );
  }
  private static native int GetInt( long msg,  String name ) throws RaiMsgException;

  /**
   * Get long, cast if necessary */
  public long GetLong( String name ) throws RaiMsgException {
    return GetLong( this.msg, name );
  }
  private static native long GetLong( long msg,  String name ) throws RaiMsgException;

  /**
   * Get float, cast if necessary */
  public float GetFloat( String name ) throws RaiMsgException {
    return GetFloat( this.msg, name );
  }
  private static native float GetFloat( long msg,  String name ) throws RaiMsgException;

  /**
   * Get double, cast if necessary */
  public double GetDouble( String name ) throws RaiMsgException {
    return GetDouble( this.msg, name );
  }
  private static native double GetDouble( long msg,  String name ) throws RaiMsgException;

  /**
   * Get String, convert if necessary */
  public String GetString( String name ) throws RaiMsgException {
    return GetString( this.msg, name );
  }
  private static native String GetString( long msg,  String name ) throws RaiMsgException;

  /**
   * Get opaquee byte [], convert if necessary */
  public byte [] GetOpaque( String name ) throws RaiMsgException {
    return GetOpaque( this.msg, name );
  }
  private static native byte [] GetOpaque( long msg,  String name ) throws RaiMsgException;

  /**
   * Get Partial, won't convert from arbitrary types */
  public Partial GetPartial( String name ) throws RaiMsgException {
    return GetPartial( this.msg, name );
  }
  private static native Partial GetPartial( long msg,  String name ) throws RaiMsgException;

  /**
   * Get boolean [], tries to converts all other array types if necessary */
  public boolean [] GetBooleanArray( String name ) throws RaiMsgException {
    return GetBooleanArray( this.msg, name );
  }
  private static native boolean [] GetBooleanArray( long msg,  String name ) throws RaiMsgException;

  /**
   * Get byte [], tries to converts all other array types if necessary */
  public byte [] GetByteArray( String name ) throws RaiMsgException {
    return GetByteArray( this.msg, name );
  }
  private static native byte [] GetByteArray( long msg,  String name ) throws RaiMsgException;

  /**
   * Get short [], tries to converts all other array types if necessary */
  public short [] GetShortArray( String name ) throws RaiMsgException {
    return GetShortArray( this.msg, name );
  }
  private static native short [] GetShortArray( long msg,  String name ) throws RaiMsgException;

  /**
   * Get int [], tries to converts all other array types if necessary */
  public int [] GetIntArray( String name ) throws RaiMsgException {
    return GetIntArray( this.msg, name );
  }
  private static native int [] GetIntArray( long msg,  String name ) throws RaiMsgException;

  /**
   * Get long [], tries to converts all other array types if necessary */
  public long [] GetLongArray( String name ) throws RaiMsgException {
    return GetLongArray( this.msg, name );
  }
  private static native long [] GetLongArray( long msg,  String name ) throws RaiMsgException;

  /**
   * Get float [], tries to converts all other array types if necessary */
  public float [] GetFloatArray( String name ) throws RaiMsgException {
    return GetFloatArray( this.msg, name );
  }
  private static native float [] GetFloatArray( long msg,  String name ) throws RaiMsgException;

  /**
   * Get double [], tries to converts all other array types if necessary */
  public double [] GetDoubleArray( String name ) throws RaiMsgException {
    return GetDoubleArray( this.msg, name );
  }
  private static native double [] GetDoubleArray( long msg,  String name ) throws RaiMsgException;

  /**
   * Get String [], tries to converts all other array types if necessary */
  public String [] GetStringArray( String name ) throws RaiMsgException {
    return GetStringArray( this.msg, name );
  }
  private static native String [] GetStringArray( long msg,  String name ) throws RaiMsgException;

  /**
   * Append Object of type: Boolean, Byte, Short, Integer, Long, Float,
   * Double, String, Partial, boolean [], byte [], short [], int [], long [],
   * float [], double [], String [] */
  public void Append( String name,  Object val ) throws RaiMsgException {
    Append( this.msg, name, val );
  }
  private static native void Append( long msg,  String name,  Object val ) throws RaiMsgException;

  /**
   * Append Object like above, along with a hint value;  valid Object types are
   * Boolean, Byte, Short, Integer Long, Float, Double, String;  valid hint
   * Object types are Boolean, Byte, Short, Integer Long, Float, Double,
   * String -- note that hint values are only valid in RAIMSG_PROTO and SASS
   * protos (grocery is represented as a Double + Byte hint)
   */
  public void Append( String name,  Object val,  Object hintVal ) throws RaiMsgException {
    AppendWithHint( this.msg, name, val, hintVal );
  }
  private static native void AppendWithHint( long msg,  String name,  Object val,  Object hintVal ) throws RaiMsgException;

  /**
   * Append Opaque field byte [] */
  public void AppendOpaque( String name,  byte [] buf ) throws RaiMsgException {
    AppendOpaque( this.msg, name, buf, 0, buf.length );
  }
  public void AppendOpaque( String name,  byte [] buf,  int off,  int len ) throws RaiMsgException {
    AppendOpaque( this.msg, name, buf, off, len );
  }
  private static native void AppendOpaque( long msg,  String name,  byte [] buf,
                                           int off,  int len ) throws RaiMsgException;
  /**
   * Append U types, Byte becomes Rai_u8, Integer becomes Rai_u32, etc */
  public void AppendUnsigned( String name,  Object val ) throws RaiMsgException {
    AppendUnsigned( this.msg, name, val );
  }
  private static native void AppendUnsigned( long msg,  String name,
                                             Object val ) throws RaiMsgException;
  /**
   * Append RaiField from another message */
  public void Append( RaiField val ) throws RaiMsgException {
    Append( this.msg, val );
  }
  private static native void Append( long msg,  RaiField val ) throws RaiMsgException;

  /**
   * Append boolean */
  public void AppendBoolean( String name,  boolean val ) throws RaiMsgException {
    AppendBoolean( this.msg, name, val );
  }
  private static native void AppendBoolean( long msg,  String name,  boolean val ) throws RaiMsgException;

  /**
   * Append byte */
  public void AppendByte( String name,  byte val ) throws RaiMsgException {
    AppendByte( this.msg, name, val, false );
  }
  public void AppendUByte( String name,  byte val ) throws RaiMsgException {
    AppendByte( this.msg, name, val, true );
  }
  private static native void AppendByte( long msg,  String name,  byte val,  boolean isU ) throws RaiMsgException;

  /**
   * Append short */
  public void AppendShort( String name,  short val ) throws RaiMsgException {
    AppendShort( this.msg, name, val, false );
  }
  public void AppendUShort( String name,  short val ) throws RaiMsgException {
    AppendShort( this.msg, name, val, true );
  }
  private static native void AppendShort( long msg,  String name,  short val,  boolean isU ) throws RaiMsgException;

  /**
   * Append int */
  public void AppendInt( String name,  int val ) throws RaiMsgException {
    AppendInt( this.msg, name, val, false );
  }
  public void AppendUInt( String name,  int val ) throws RaiMsgException {
    AppendInt( this.msg, name, val, true );
  }
  private static native void AppendInt( long msg,  String name,  int val,  boolean isU ) throws RaiMsgException;

  /**
   * Append long */
  public void AppendLong( String name,  long val ) throws RaiMsgException {
    AppendLong( this.msg, name, val, false );
  }
  public void AppendULong( String name,  long val ) throws RaiMsgException {
    AppendLong( this.msg, name, val, true );
  }
  private static native void AppendLong( long msg,  String name,  long val,  boolean isU ) throws RaiMsgException;

  /**
   * Append float */
  public void AppendFloat( String name,  float val ) throws RaiMsgException {
    AppendFloat( this.msg, name, val );
  }
  private static native void AppendFloat( long msg,  String name,  float val ) throws RaiMsgException;

  /**
   * Append double */
  public void AppendDouble( String name,  double val ) throws RaiMsgException {
    AppendDouble( this.msg, name, val );
  }
  private static native void AppendDouble( long msg,  String name,  double val ) throws RaiMsgException;

  /**
   * Update Object of type: Boolean, Byte, Short, Integer, Long, Float,
   * Double, String, Partial, boolean [], byte [], short [], int [], long [],
   * float [], double [], String [] */
  public void Update( String name,  Object val ) throws RaiMsgException {
    Update( this.msg, name, val );
  }
  private static native void Update( long msg,  String name,  Object val ) throws RaiMsgException;

  /**
   * Update Object like above, along with a hint value;  valid Object types are
   * Boolean, Byte, Short, Integer Long, Float, Double, String;  valid hint
   * Object types are Boolean, Byte, Short, Integer Long, Float, Double,
   * String -- note that hint values are only valid in RAIMSG_PROTO and SASS
   * protos (grocery is represented as a Double + Byte hint)
   */
  public void Update( String name,  Object val,  Object hintVal ) throws RaiMsgException {
    UpdateWithHint( this.msg, name, val, hintVal );
  }
  private static native void UpdateWithHint( long msg,  String name,  Object val,  Object hintVal ) throws RaiMsgException;

  /**
   * Update Opaque field byte [] */
  public void UpdateOpaque( String name,  byte [] buf ) throws RaiMsgException {
    UpdateOpaque( this.msg, name, buf, 0, buf.length );
  }
  public void UpdateOpaque( String name,  byte [] buf,  int off,  int len ) throws RaiMsgException {
    UpdateOpaque( this.msg, name, buf, off, len );
  }
  private static native void UpdateOpaque( long msg,  String name,  byte [] buf,
                                           int off,  int len ) throws RaiMsgException;
  /**
   * Update U types, Byte becomes Rai_u8, Integer becomes Rai_u32, etc */
  public void UpdateUnsigned( String name,  Object val ) throws RaiMsgException {
    UpdateUnsigned( this.msg, name, val );
  }
  private static native void UpdateUnsigned( long msg,  String name,
                                             Object val ) throws RaiMsgException;
  /**
   * Update RaiField from another message */
  public void Update( RaiField val ) throws RaiMsgException {
    Update( this.msg, val );
  }
  private static native void Update( long msg,  RaiField val ) throws RaiMsgException;

  /**
   * Update boolean */
  public void UpdateBoolean( String name,  boolean val ) throws RaiMsgException {
    UpdateBoolean( this.msg, name, val );
  }
  private static native void UpdateBoolean( long msg,  String name,  boolean val ) throws RaiMsgException;

  /**
   * Update byte */
  public void UpdateByte( String name,  byte val ) throws RaiMsgException {
    UpdateByte( this.msg, name, val, false );
  }
  public void UpdateUByte( String name,  byte val ) throws RaiMsgException {
    UpdateByte( this.msg, name, val, true );
  }
  private static native void UpdateByte( long msg,  String name,  byte val,  boolean isU ) throws RaiMsgException;

  /**
   * Update short */
  public void UpdateShort( String name,  short val ) throws RaiMsgException {
    UpdateShort( this.msg, name, val, false );
  }
  public void UpdateUShort( String name,  short val ) throws RaiMsgException {
    UpdateShort( this.msg, name, val, true );
  }
  private static native void UpdateShort( long msg,  String name,  short val,  boolean isU ) throws RaiMsgException;

  /**
   * Update int */
  public void UpdateInt( String name,  int val ) throws RaiMsgException {
    UpdateInt( this.msg, name, val, false );
  }
  public void UpdateUInt( String name,  int val ) throws RaiMsgException {
    UpdateInt( this.msg, name, val, true );
  }
  private static native void UpdateInt( long msg,  String name,  int val,  boolean isU ) throws RaiMsgException;

  /**
   * Update long */
  public void UpdateLong( String name,  long val ) throws RaiMsgException {
    UpdateLong( this.msg, name, val, false );
  }
  public void UpdateULong( String name,  long val ) throws RaiMsgException {
    UpdateLong( this.msg, name, val, true );
  }
  private static native void UpdateLong( long msg,  String name,  long val,  boolean isU ) throws RaiMsgException;

  /**
   * Update float */
  public void UpdateFloat( String name,  float val ) throws RaiMsgException {
    UpdateFloat( this.msg, name, val );
  }
  private static native void UpdateFloat( long msg,  String name,  float val ) throws RaiMsgException;

  /**
   * Update double */
  public void UpdateDouble( String name,  double val ) throws RaiMsgException {
    UpdateDouble( this.msg, name, val );
  }
  private static native void UpdateDouble( long msg,  String name,  double val ) throws RaiMsgException;

  /**
   * Unpack message and make available for extracting field values */
  public void UnPack( byte [] msgBuf ) throws RaiMsgException {
    UnPack( this.msg, msgBuf, 0, msgBuf.length );
  }
  public void UnPack( byte [] msgBuf,  int off,  int len ) throws RaiMsgException {
    UnPack( this.msg, msgBuf, off, len );
  }
  private static native void UnPack( long msg,  byte [] msgBuf,  int off,
                                     int len ) throws RaiMsgException;
  /**
   * Pack message bytes into buffer, len is amount of buffer available,
   * amount of buffer used is returned */
  public int Pack( byte [] msgBuf,  int off,  int len ) throws RaiMsgException {
    if ( len > 0 )
      return Pack( this.msg, msgBuf, off, len );
    return 0;
  }
  public int Pack( byte [] msgBuf,  int off ) throws RaiMsgException {
    if ( off < msgBuf.length )
      return Pack( this.msg, msgBuf, off, msgBuf.length - off );
    return 0;
  }
  public int Pack( byte [] msgBuf ) throws RaiMsgException {
    return Pack( this.msg, msgBuf, 0, msgBuf.length );
  }
  private static native int Pack( long msg,  byte [] msgBuf,  int off,
                                  int len ) throws RaiMsgException;
  /**
   * Pack message and return in newly allocated byte [] buffer */
  public byte [] Pack() throws RaiMsgException {
    return Pack( this.msg );
  }
  public byte [] Packed() throws RaiMsgException {
    return Pack( this.msg );
  }
  private static native byte [] Pack( long msg ) throws RaiMsgException;

  /**
   * Return number of bytes used by message buffer */
  public int PackSize() throws RaiMsgException {
    return PackSize( this.msg );
  }
  private static native int PackSize( long msg ) throws RaiMsgException;

  /**
   * Activate sub message */
  public boolean Activate( String name ) throws RaiMsgException {
    return Activate( this.msg, name );
  }
  private static native boolean Activate( long msg,  String name ) throws RaiMsgException;

  /**
   * Rename a field */
  public boolean Rename( String oldName,  String newName ) throws RaiMsgException {
    return Rename( this.msg, oldName, newName );
  }
  private static native boolean Rename( long msg,  String oldName,
                                        String newName ) throws RaiMsgException;
  /**
   * Remove a field */
  public boolean Remove( String name ) throws RaiMsgException {
    return Remove( this.msg, name );
  }
  private static native boolean Remove( long msg,  String name ) throws RaiMsgException;

  /**
   * Print message int 'TIB' format to output stream */
  public void Print( java.io.OutputStream out ) throws RaiMsgException {
    Print( this.msg, out, true, "%-14s : ", true, "%-7s %3d : ", null );
  }
  public void Print( java.io.OutputStream out,  boolean field_newlines,
                     String fname_fmt,  boolean print_opaques,
                     String dbg_format,
                     String dbg_hformat ) throws RaiMsgException {
    Print( this.msg, out, field_newlines, fname_fmt, print_opaques, dbg_format,
                          dbg_hformat );
  }
  public static native void Print( long msg,  java.io.OutputStream out,
                                   boolean field_newlines,  String fname_fmt,
                                   boolean print_opaques,  String dbg_format,
                                   String dbg_hformat ) throws RaiMsgException;
  /**
   * Dump message hex bytes to output stream, like od -t x1 */
  public void PrintHex( java.io.OutputStream out ) throws RaiMsgException {
    PrintHex( this.msg, out );
  }
  public static native void PrintHex( long msg,  java.io.OutputStream out ) throws RaiMsgException;

  /**
   * Dump msgBuf hex bytes to output stream, like od -t x1 */
  public static void PrintHex( java.io.OutputStream out,  byte [] msgBuf ) throws RaiMsgException {
    PrintHex( out, msgBuf, 0, msgBuf.length );
  }
  public static native void PrintHex( java.io.OutputStream out,  byte [] msgBuf,
                                      int off,  int len ) throws RaiMsgException;
  /**
   * Print an XML representation of message */
  public void PrintXML( java.io.OutputStream out ) throws RaiMsgException {
    PrintXML( this.msg, out, 0, true, "MSG", null );
  }
  public void PrintXML( java.io.OutputStream out,  int attr_flags,
                        boolean field_nl,  String msg_name,
                        String[] msg_atts ) throws RaiMsgException {
    PrintXML( this.msg, out, attr_flags, field_nl, msg_name, msg_atts );
  }
  /* attr_flags */
  public static final int ADD_TYPE_ATTR         = 0x1;
  public static final int ADD_SIZE_ATTR         = 0x2;
  public static final int ADD_FID_ATTR          = 0x4;
  public static final int ADD_PARTIAL_OFF_ATTR  = 0x8;
  public static final int ADD_ARRAY_COUNT_ATTR  = 0x10;
  public static final int ADD_ARRAY_TYPE_ATTR   = 0x20;
  public static final int ADD_ARRAY_ELSIZE_ATTR = 0x40;
  public static final int ADD_HINT_ATTR         = 0x80;
  public static final int ADD_HINT_TYPE_ATTR    = 0x100;
  public static final int ADD_HINT_SIZE_ATTR    = 0x200;
  public static final int ADD_ALL_ATTRS         = 0x3ff;

  private static native void PrintXML( long msg,  java.io.OutputStream out,
                                      int attr_flags,  boolean field_nl,
                                      String msg_name,  String[] msg_atts ) throws RaiMsgException;

  /**
   * Convert a String to a RaiMsgType */
  public static native int StrType( String name );

  public static native String TypeStr( int typ );

  /**
   * Load dictionary from cfile configuration */
  public static native void ReadDataDictionary( String fields_cf, 
                                                String records_cf,
                                                String cfile_path,
                                                char path_sep ) throws RaiMsgException;
  /**
   * Set dictionary packed in a TibMsg network format */
  public static native void SetDataDictionary( RaiMsg msg ) throws RaiMsgException;

  /**
   * Get dictionary packed in a TibMsg network format */
  public static native RaiMsg GetDataDictionary() throws RaiMsgException;

  /**
   * Initializes the C++ RaiMsg Java type mapping */
  private static native void initClasses( Class boolean_cls, Class byte_cls,
                                          Class short_cls, Class int_cls,
                                          Class long_cls, Class float_cls,
                                          Class double_cls );

  static {
    try {
      System.loadLibrary( "jraimsg64" );
    } catch ( java.lang.LinkageError e ) {
      System.loadLibrary( "jraimsg" );
    }
    initClasses( java.lang.Boolean.TYPE, java.lang.Byte.TYPE,
                 java.lang.Short.TYPE, java.lang.Integer.TYPE,
                 java.lang.Long.TYPE, java.lang.Float.TYPE,
                 java.lang.Double.TYPE );
  }
};
