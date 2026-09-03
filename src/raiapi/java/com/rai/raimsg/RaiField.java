package com.rai.raimsg;

/** RaiField contains the name, type, size, value of a field in a messsage */
public class RaiField {
  long   fld;
  RaiMsg msg;

  public RaiField() {
    this.fld = Create();
    this.msg = null;
  }
  protected RaiField( long fld ) {
    this.fld = fld;
    this.msg = null;
  }
  private static native long Create();

  protected void finalize() {
    Delete( this.fld );
  }
  private static native void Delete( long fld );

  protected long getFieldHandle() {
    return this.fld;
  }

  public String Name() {
    return Name( this.fld );
  }
  private static native String Name( long fld );

  public int Type() {
    return Type( this.fld );
  }
  private static native int Type( long fld );

  public int Size() {
    return Size( this.fld );
  }
  private static native int Size( long fld );

  public int HintType() {
    return HintType( this.fld );
  }
  private static native int HintType( long fld );

  public int HintSize() {
    return HintSize( this.fld );
  }
  private static native int HintSize( long fld );

  public int EntryType() {
    return EntryType( this.fld );
  }
  private static native int EntryType( long fld );

  public int EntrySize() {
    return EntrySize( this.fld );
  }
  private static native int EntrySize( long fld );

  public int NumEntries() {
    return NumEntries( this.fld );
  }
  private static native int NumEntries( long fld );

  public Object Get() throws RaiMsgException {
    return Get( this.fld );
  }
  private static native Object Get( long fld ) throws RaiMsgException;

  public boolean GetBoolean() throws RaiMsgException {
    return GetBoolean( this.fld );
  }
  private static native boolean GetBoolean( long fld ) throws RaiMsgException;

  public byte GetByte() throws RaiMsgException {
    return GetByte( this.fld );
  }
  private static native byte GetByte( long fld ) throws RaiMsgException;

  public short GetShort() throws RaiMsgException {
    return GetShort( this.fld );
  }
  private static native short GetShort( long fld ) throws RaiMsgException;

  public int GetInt() throws RaiMsgException {
    return GetInt( this.fld );
  }
  private static native int GetInt( long fld ) throws RaiMsgException;

  public long GetLong() throws RaiMsgException {
    return GetLong( this.fld );
  }
  private static native long GetLong( long fld ) throws RaiMsgException;

  public float GetFloat() throws RaiMsgException {
    return GetFloat( this.fld );
  }
  private static native float GetFloat( long fld ) throws RaiMsgException;

  public double GetDouble() throws RaiMsgException {
    return GetDouble( this.fld );
  }
  private static native double GetDouble( long fld ) throws RaiMsgException;

  public String GetString() throws RaiMsgException {
    return GetString( this.fld );
  }
  private static native String GetString( long fld ) throws RaiMsgException;

  public byte [] GetOpaque() throws RaiMsgException {
    return GetOpaque( this.fld );
  }
  private static native byte [] GetOpaque( long fld ) throws RaiMsgException;

  public Partial GetPartial() throws RaiMsgException {
    return GetPartial( this.fld );
  }
  private static native Partial GetPartial( long fld ) throws RaiMsgException;

  public boolean [] GetBooleanArray() throws RaiMsgException {
    return GetBooleanArray( this.fld );
  }
  private static native boolean [] GetBooleanArray( long fld ) throws RaiMsgException;

  public byte [] GetByteArray() throws RaiMsgException {
    return GetByteArray( this.fld );
  }
  private static native byte [] GetByteArray( long fld ) throws RaiMsgException;

  public short [] GetShortArray() throws RaiMsgException {
    return GetShortArray( this.fld );
  }
  private static native short [] GetShortArray( long fld ) throws RaiMsgException;

  public int [] GetIntArray() throws RaiMsgException {
    return GetIntArray( this.fld );
  }
  private static native int [] GetIntArray( long fld ) throws RaiMsgException;

  public long [] GetLongArray() throws RaiMsgException {
    return GetLongArray( this.fld );
  }
  private static native long [] GetLongArray( long fld ) throws RaiMsgException;

  public float [] GetFloatArray() throws RaiMsgException {
    return GetFloatArray( this.fld );
  }
  private static native float [] GetFloatArray( long fld ) throws RaiMsgException;

  public double [] GetDoubleArray() throws RaiMsgException {
    return GetDoubleArray( this.fld );
  }
  private static native double [] GetDoubleArray( long fld ) throws RaiMsgException;

  public String [] GetStringArray() throws RaiMsgException {
    return GetStringArray( this.fld );
  }
  private static native String [] GetStringArray( long fld ) throws RaiMsgException;

  public Object GetHint() throws RaiMsgException {
    return GetHint( this.fld );
  }
  private static native Object GetHint( long fld ) throws RaiMsgException;

  public boolean Find( RaiMsg msg,  String name ) throws RaiMsgException {
    if ( ! Find( this.fld, msg.msg, name ) ) {
      this.msg = null;
      return false;
    }
    this.msg = msg;
    return true;
  }
  private static native boolean Find( long fld,  long msg,  String name ) throws RaiMsgException;

  public boolean First( RaiMsg msg ) throws RaiMsgException {
    if ( ! First( this.fld, msg.msg ) ) {
      this.msg = null;
      return false;
    }
    this.msg = msg;
    return true;
  }
  private static native boolean First( long fld,  long msg ) throws RaiMsgException;

  public boolean Next() throws RaiMsgException {
    if ( this.msg == null )
      return false;
    if ( ! Next( this.fld ) ) {
      this.msg = null;
      return false;
    }
    return true;
  }
  private static native boolean Next( long fld ) throws RaiMsgException;
}
