package com.rai.raiapi2;

/** An Entitlement object holds information about subject and content
 * capabilities
 * @see RaiSession#Login */
public class RaiEntitlement {
  long entitle;

  /** Constructor used internally.
   * @see RaiSession#Login */
  protected RaiEntitlement( long e ) {
    this.entitle = e;
  }
  protected void finalize() {
    Delete( this.entitle );
  } 
  private static native void Delete( long entitle );
}
