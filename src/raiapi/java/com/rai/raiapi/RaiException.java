

/******************************************************************************
 *
 * Error Interface
 *
 *****************************************************************************/
package com.rai.raiapi;

import java.util.*;
import com.rai.raimsg.*;


public class
RaiException extends com.rai.raiexception.RaiException
{

  /* Default return values */
  protected int     status  = UNKNOWN;
  protected String  reason  = "No details available";
  protected String  module  = "RaiApi";

  /* Status codes */
  //public static final int OK       = 0;
  //public static final int UNKNOWN  = 1;
  //public static final int BAD_MSG  = 4;
  //public static final int BAD_DICT = 5;
  //public static final int BAD_RV   = 6;

  public static final int BAD_SESSION  = 0;
  public static final int BAD_SUBJECT  = 1;
  public static final int BAD_RAIMSG  = 2;
  public static final int BAD_BUFFER  = 3;
  public static final int BAD_IOCTL_PARM  = 4;
  public static final int BAD_DICT  = 5;
  public static final int BAD_SASS_INIT  = 6;
  public static final int DICT_LOAD_PENDING  = 7;
  public static final int BAD_TRAN = 8;
  public static final int UNKNOWN  = 9;

  protected static final String errString[] = {
    "NULL or invalid session parameter",
    "NULL or invalid subject",
    "NULL or invalid RaiMsg",
    "NULL buffer",
    "Bad IOCTL parameter",
    "Error waiting for dictionary to load",
    "Error creating or initializing a RaiMsg",
    "Dictionary load pending",
    "Error in underlying transport",
    "Unknown Error"
  };

  public
  RaiException( int status, String reason, String module ) {
    super( reason );
    this.status = status;
    this.module = module;
    this.reason = reason;
  };

  public
  RaiException() {
    // just provide the default values as we are not given anything else
  };

  public
  RaiException( int status ) {

    this.status = status;
    this.reason = this.errString[ status ];
  };

  public
  RaiException( String reason ) {

  this.reason = reason;
  };

  public
  RaiException( int status, Throwable cause ){

  this.status = status;
  this.reason = this.errString[ status ];
  this.initCause( cause );
  };

  public
  RaiException( int status, String reason, Throwable cause ){

  this.status = status;
  this.reason = reason;
  this.initCause( cause );
  };

  public String getModule() {
    return this.module;
  }

  public int getErrno() {
    return this.status;
  }

  public String getReason() {
    return this.reason;
  }

  public String toString() {
    if ( this.module == null )
      return this.reason;
    return this.module + "+" + this.status + ": " + this.reason;
  }
}
