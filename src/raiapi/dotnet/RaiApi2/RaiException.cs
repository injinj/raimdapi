/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 * Mirrors com.rai.raiexception.RaiException, com.rai.raiapi2.RaiApiException
 * and com.rai.raimsg.RaiMsgException. */
using System;
using Com.Rai.Interop;

namespace Com.Rai.Raiexception {

/** Base exception of the Rai API, wraps the native error record when the
 * error came from the api (Module/Errno/Reason), or a message otherwise. */
public class RaiException : Exception {
  /* native rai_err_t, IntPtr.Zero when this is a managed-only exception */
  internal readonly IntPtr err;

  public RaiException( string msg ) : base( msg ) {}
  public RaiException( string msg,  Exception inner ) : base( msg, inner ) {}
  internal RaiException( IntPtr e ) : base( Describe( e ) ) { this.err = e; }

  /** The module where the error is defined, or null */
  public string getModule() {
    return this.err == IntPtr.Zero ? null
                                   : Native.Str( Native.rai_err_module( this.err ) );
  }
  /** The error status code, its domain is the module */
  public int getErrno() {
    return this.err == IntPtr.Zero ? 0 : (int) Native.rai_err_status( this.err );
  }
  /** The error string that errno maps to */
  public string getReason() {
    return this.err == IntPtr.Zero ? this.Message
                                   : Native.Str( Native.rai_err_reason( this.err ) );
  }
  internal IntPtr NativeErr { get { return this.err; } }

  static string Describe( IntPtr e ) {
    if ( e == IntPtr.Zero ) return "ok";
    return Native.Str( Native.rai_err_module( e ) ) + ": " +
           Native.Str( Native.rai_err_reason( e ) ) + " (" +
           Native.rai_err_status( e ) + ")";
  }

  /** throw when a native call returned an error */
  internal static void Check( IntPtr e ) {
    if ( e != IntPtr.Zero )
      throw Make( e );
  }
  internal static RaiException Make( IntPtr e ) {
    string mod = Native.Str( Native.rai_err_module( e ) );
    if ( mod == "RaiMsg" || mod == "RaiField" || mod == "RaiDict" )
      return new Com.Rai.Raimsg.RaiMsgException( e );
    return new Com.Rai.Raiapi2.RaiApiException( e );
  }
}
} /* namespace Com.Rai.Raiexception */

namespace Com.Rai.Raiapi2 {
/** Exception raised by the api layer (session, queue, args, ...) */
public class RaiApiException : Com.Rai.Raiexception.RaiException {
  internal RaiApiException( IntPtr e ) : base( e ) {}
  public RaiApiException( string msg ) : base( msg ) {}
}
}

namespace Com.Rai.Raimsg {
/** Exception raised by the message layer (RaiMsg, RaiField) */
public class RaiMsgException : Com.Rai.Raiexception.RaiException {
  internal RaiMsgException( IntPtr e ) : base( e ) {}
  public RaiMsgException( string msg ) : base( msg ) {}
}
}
