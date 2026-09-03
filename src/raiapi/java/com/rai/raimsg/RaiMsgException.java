package com.rai.raimsg;
import com.rai.raiexception.RaiException;

/** An exception that occurs within the raimsg package. */
public class RaiMsgException extends RaiException {
  protected long err;
  public RaiMsgException( long err ) {
    this.err = err;
  }
  public String getModule() {
    return getModule( this.err );
  }
  private static native String getModule( long err );

  public int getErrno() {
    return getErrno( this.err );
  }
  private static native int getErrno( long err );

  public String getReason() {
    return getReason( this.err );
  }
  private static native String getReason( long err );
};
