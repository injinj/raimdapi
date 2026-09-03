package com.rai.raiexception;
import java.util.*;


public class RaiException extends java.lang.Exception
{
  public RaiException( String description,  Throwable cause ) {
    super( description, cause );
  }
  public RaiException( String description ) {
    super( description );
  }
  public RaiException() {}

  public String getModule() { /* Rai super specifics */
    return null;
  }
  public int getErrno() {
    return 0;
  }
  public String getReason() {
    return null;
  }
  public String toString() {
    if ( this.getModule() == null )
      return super.toString();
    return this.getModule() + "." +
           Integer.toString( this.getErrno() ) + "+" +
           this.getReason() + ": " + super.toString();
  }
}
