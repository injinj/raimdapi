package com.rai.raiapi2;
import com.rai.raiexception.RaiException;

/** The RaiApiException is thrown from within the raiapi2 package.
 *
 * @see com.rai.raimsg.RaiMsgException
 * @see RaiException
 */
public class RaiApiException extends RaiException {
  protected long err;
  public RaiApiException( long err ) {
    this.err = err;
  }
  /** Get the module name that the error occured. */
  public String getModule() {
    if ( this.err == 0 )
      return super.getModule();
    return getModule( this.err );
  }
  private static native String getModule( long err );

  /** Get the error status of the error occured, this is numbered 0 to N per
   * module. */
  public int getErrno() {
    if ( this.err == 0 )
      return super.getErrno();
    return getErrno( this.err );
  }
  private static native int getErrno( long err );

  /** Get the reason of the error, this is assigned per error status number. */
  public String getReason() {
    if ( this.err == 0 )
      return super.getReason();
    return getReason( this.err );
  }
  private static native String getReason( long err );
};
