package com.rai.raiapi2;
import com.rai.raimsg.RaiMsg;
import com.rai.raiexception.RaiException;

/** RaiApi is a handle to an underlying transport implementation,
 * as well as logging and dictionary management, and some convenience
 * functions. */
public class RaiApi {
  long api;

  /** The api argument switch controls the middleware module loaded.
   * @see #GetArgs */
  public static final String raiapi_api_arg    = "api";
  /** The userid argument switch identifies the user of the api to other
   * services.
   * @see #GetArgs */
  public static final String raiapi_userid_arg = "userid";
  /** The appid argument switch identifies the program using the api to other
   * services.
   * @see #GetArgs */
  public static final String raiapi_appid_arg  = "appid";
  /** The cfilePath argument can cause the dictionary to loaded locally from
   * the file system, if GetDictArgs() populates them and OpenDict() is
   * called.
   * @see #OpenDict */
  public static final String cfile_path_arg   = "cfilePath";
  /** The tssRecords argument identifies the root of the SASS configuration
   * for records, defaults to "tss_records.cf".
   * @see #OpenDict */
  public static final String tss_records_arg  = "tssRecords";
  /** The tssFields argument identifies the root of the SASS configuration
   * for fields, it defaults to "tss_fields.cf".
   * @see #OpenDict */
  public static final String tss_fields_arg   = "tssFields";
  /** The appendixA argument identifies the marketfeed configuration, it
   * defaults to "appendix_a".
   * @see #OpenDict */
  public static final String appendix_a_arg   = "appendixA";
  /** The enumtypeDef argument identifies the enumerations for the marketfeed
   * configuration, it defaults to "enumtype.def".
   * @see #OpenDict */
  public static final String enumtype_def_arg = "enumtypeDef";
  /** LVL_DEVEL logs everything.
   * @see #OpenLog */
  public static final int LVL_DEVEL  = 0;
  /** LVL_FTRACE is used for tracing functions during development.
   * @see #OpenLog */
  public static final int LVL_FTRACE = 1;
  /** LVL_TRACE is used for tracing during development.
   * @see #OpenLog */
  public static final int LVL_TRACE  = 2;
  /** LVL_DEBUG is used for logging useful debug information after deployment,
   * this isn't intended to be logged during normal operation.
   * @see #OpenLog */
  public static final int LVL_DEBUG  = 3;
  /** LVL_MINOR is used for logging rates and other periodic info which
   * shouldn't alter the performance of the program.
   * @see #OpenLog */
  public static final int LVL_MINOR  = 4;
  /** LVL_NORMAL is used for logging events which are important, but shouldn't
   * affect correctness.
   * @see #OpenLog */
  public static final int LVL_NORMAL = 5;
  /** LVL_ERROR is used for logging abnormal behavior which may affect the
   * correctness.
   * @see #OpenLog */
  public static final int LVL_ERROR  = 6;

  /** Internal constructor.
   *
   * @see RaiApi#RaiOpen
   */
  protected RaiApi( long a ) {
    this.api = a;
  }
  /** Initialization and configuration class for the Rai API.
   *
   * Call RaiApi.RaiOpen() to begin Rai API operations.
   * <p>If apiname is null, the default (capr) will be used unless -api
   * &lt;apiname&gt;is an argument in argv[].  This binds the RaiApi classes to
   * a specific implementation, loaded from the directory that the binary is
   * run. The dynamically linked library called rai_api_&lt;apiname&gt;.so or
   * .dll is loaded and the function
   * <p><pre>extern "C" RaiApi *RaiApi_RaiOpen_&lt;apiname&gt;( int argc,
   * char *argv[ ] )</pre>
   * <p>is called in initialize and create the RaiApi object.  Multiple RaiApi
   * handles may be created in order to bridge transports.
   *
   * @param apiName The protocol used by the api - for example,
   * "tibrv", "lbm", "embd", or "capr" (default);  Used for configuring common
   * API transport bindings
   * @param argv The argument array from main( String [] args)
   *
   * @return RaiApi The handle to an instance of an interface to the API
   * transport implementation
   */
  public static native RaiApi RaiOpen( String apiName,
                                       String[] argv ) throws RaiException;
  /** Internal destructor for the API handle */
  protected void finalize() {
    Delete( this.api );
  }
  private static native void Delete( long api );

  /** Get the name of the api name of the protocol, this will be the string
   * name resolved from RaiApi.RaiOpen()
   * @see RaiApi#RaiOpen */
  public native String GetApiName();
  /** Get the Rai API version string.   This will have a release string name,
   * a version number (major.minor), a patch number, and a build number. */
  public static native String RaiVersion();
  /** Retrieve the Args configuration parameters for the RaiAPI instantiation.
   * <p>Args will vary based on the underlying transport being used. For
   * example, for a TIBCO Rendezvous session, Args will include the daemon,
   * network and service parameters used to create the Rendezvous transport.
   * To see what these arguments are, use -help with the -api argument.
   * <p>Example:
   * <p><pre>$ java raisub2 -api tibrv -help</pre>
   * @param args An args object */
  public native void GetArgs( Args args ) throws RaiException;
  /** Get the SASS dictionary args:  cfilePath, tssRecords, tssFields,
   * appendixA, enumtypeDef.  These can be used with OpenDict() to load
   * a dictionary from the local filesystem.
   * @param args An args object
   * @see RaiApi#OpenDict
   * @see RaiSession#CreateDict */
  public static native void GetDictArgs( Args args ) throws RaiException;
  /** Parse api Args, use after args.processArgs( argc, argv ) is called */
  public native void ParseArgs( Args args ) throws RaiException;
  /** If args has log, logLevel, logVerb, logXml args, then open the log file,
   * otherwise return false.  These arguments are added to Args when
   * Args.addDefaults() is called and parsed when RaiApi.ParseArgs() is
   * called.
   **/
  public static native boolean OpenLog( Args args ) throws RaiException;
  /** Opens the log file.  If string is "-", then stderr is used for logging.
   * The logLevel argument filters the logging, if logLevel is minor (-logLevel
   * minor, or RaiApi.LVL_MINOR), then any logging LVL_MINOR and above
   * (specifically LVL_MINOR, LVL_NORMAL, LVL_ERROR) will appear in the log
   * file, but the levels below do not (LVL_DEBUG and below).  The logVerb is a
   * number between 0 and 5 which controls the verbosity of logging (1 nothing,
   * 5 everything).  The defaults are level LVL_MINOR and verbosity 4.
   * <p>Example of different verbosities:
   * <p><pre>
   * $ java raisub2 -logVerb 1
   * ^C$ 
   * $ java raisub2 -logVerb 2
   * ^C Caught signal 2, quitting
   * $ java raisub2 -logVerb 3
   * ^C2011-07-08 12:59:22.193  Caught signal 2, quitting
   * $ java raisub2 -logVerb 4
   * ^C2011-07-08 12:59:26.901 Minor:  Caught signal 2, quitting
   * $ java raisub2 -logVerb 5
   * ^C2011-07-08 12:59:35.514 Minor:  Caught signal 2, quitting (src/raiapi/java/com/rai/raiapi2/rai_api_jni.cpp:1292)
   * </pre>
   *
   * @param name The filesystem name of the log file, if '-' then stderr
   * @param logLevel The minimum log level that gets logged, for example
   * RaiApi.LVL_MINOR
   * @param logVerb The verbosity of logging
   */
  public static native void OpenLog( String name,  int logLevel,  int logVerb )
                                                    throws RaiException;
  /** Prints string to log file using level RaiApi.LVL_MINOR */
  public void PrintLog( String s ) {
    this.PrintLog( LVL_MINOR, null, s );
  }
  /** Prints string to log file with level specified  */
  public void PrintLog( int level,  String s ) {
    this.PrintLog( level, null, s );
  }
  /** Prints string and exception to log file with level specified  */
  public native void PrintLog( int level,  java.lang.Exception err,  String s );

  /** Prints string to log file using level RaiApi.LVL_MINOR, this is
   * equivalent to PrintLog() unless a module name is associated with the
   * RaiApi handle */
  public static void Log( String s ) {
    Log( LVL_MINOR, null, s );
  }
  /** Prints string to log file with level specified, this is equivalent
   * to PrintLog() unless a module name is associated with the RaiApi handle  */
  public static void Log( int level,  String s ) {
    Log( level, null, s );
  }
  /** Prints string and exception to log file with level specified, this is
   * equivalent to PrintLog() unless a module name is associated with the
   * RaiApi handle  */
  public static native void Log( int level,  java.lang.Exception err,
                                 String s );
  /** Load the dictionary from local filesystem.  If args has cfilePath,
   * tssRecords, tssFIelds, appendixA, enumtypeDef, then open the SASS
   * dictionary and marketfeed dictionary, otherwise return false.
   * <p>Example:
   * <p><pre>
   * public static void main( String [] argv ) {
   *   RaiApi api = RaiApi.RaiOpen( null, argv );
   *   Args args = new Args();
   *   args.GetArgs( args ); // Get api arguments
   *   args.GetDictArgs( args ); // Get dict arguments -cfilePath, etc
   *   if ( args.processArgs( argv ) { // populates values for args
   *     bool success = args.OpenDict( args ); // check if dictionary available
   *     if ( success ) {
   *       // Dictionary does not need to be loaded from the network with
   *       // RaiDict dict = session.CreateDict()
   *     }
   *   }
   * }
   * </pre>
   * @param args An args object
   * @return true when dictionary is loaded, false when it was not found
   * @see RaiApi#GetDictArgs
   * @see RaiSession#CreateDict
   */
  public static native boolean OpenDict( Args args ) throws RaiException;
  /** Create a new session.
   * <p>This uses the network parameters which have been set in the RaiApi
   * object using ParseArgs(). After successfully completing this call, any
   * transport objects, such as sockets, will have been connected to the
   * network or service. This object is unique, with its own session
   * identifier, so creating multiple sessions will create multiple transport
   * and session instances. It is possible to change the RaiApi parameters with
   * ParseArgs() and SetIoctl() in order to create another RaiSession object
   * using a different network.
   *
   * @return RaiSession The handle to a session */
  public native RaiSession CreateSession() throws RaiException;

  /** Create a SASS Qform class message, which is RaiMsg sass qform with 4
   * fields using the form defined by RecType */
  public static RaiMsg NewSASSMsg( short MsgType, short RecType, short SeqNo,
                                   short RecStatus ) throws RaiException {
    return NewRaiMsg( RaiMsg.TIB_SASS_PROTO, MsgType, RecType, SeqNo,
                      RecStatus );
  }
  /** Create a SASS Qform class message, which is RaiMsg sass qform with 4
   * fields, using the name of the form defined by FormType  */
  public static RaiMsg NewSASSMsg( short MsgType,  String FormType,
                           short SeqNo,  short RecStatus ) throws RaiException {
    return NewRaiMsg( RaiMsg.TIB_SASS_PROTO, MsgType, FormType, SeqNo,
                      RecStatus );
  }
  /** Create a SASS RaiMsg class message, which is a RaiMsg self describing
   * format with 4 fields */
  public static RaiMsg NewRaiMsg( short MsgType, short RecType, short SeqNo,
                                  short RecStatus ) throws RaiException {
    return NewRaiMsg( RaiMsg.RAIMSG_PROTO, MsgType, RecType, SeqNo, RecStatus );
  }
  /** Create a SASS RaiMsg class message, which is a RaiMsg self describing
   * format with 4 fields */
  public static RaiMsg NewRaiMsg( short MsgType, String FormType, short SeqNo,
                                  short RecStatus  ) throws RaiException {
    return NewRaiMsg( RaiMsg.RAIMSG_PROTO, MsgType, FormType, SeqNo, RecStatus );
  }
  /** Convenience function to create a message with 4 header fields */
  public static native RaiMsg NewRaiMsg( int proto,  short MsgType,
                             short RecType,  short SeqNo,  short RecStatus )
                                                      throws RaiException;
  /** Convenience function to create a message and lookup a form type name,
   * using the current dictionary loaded */
  public static native RaiMsg NewRaiMsg( int proto,  short MsgType,
                             String FormType,  short SeqNo,  short RecStatus ) 
                                                     throws RaiException;
  /** Closes the Rai API.  Close any open sessions first.
   * @see RaiApi#RaiOpen
   * @see RaiSession#Destroy */
  public native void Close();

  /** Value of signal SIGHUP(1) (loss of terminal) */
  public static final int SIGHUP  = 1;
  /** Value of signal SIGINT(2) (ctrl-c) */
  public static final int SIGINT  = 2;
  /** Value of signal SIGTERM(15) (default termination signal used with kill) */
  public static final int SIGTERM = 15;
  /** This function traps SIGINT (2), SIGHUP (1), SIGTERM (15) and calls
   * static void className.functionName( int sig ) when a signal is caught.
   *
   * <p>This may require LD_PRELOAD=libjsig.so and/or java option -Xrs (rs =
   * reduce signals).  With -Xrs, it does not capture these three signals.
   * This is a simpler version of Runtime.getRuntime().addShutdownHook(), which
   * will spawn a thread for shutdown tasks.  This does not spawn a thread, so
   * it has no thread concurrency, and it should not use synchronized
   * operations.
   * <p>This function is useful when cleanup, such as graceful network shutdown
   * and unsubscribing subjects are desirable when a signal is caught.
   * <p>Example:
   * <p><pre>
   * class X {
   *   public static void sigHandler( int sig ) {
   *     if ( sig != RaiApi.SIGHUP ) // ignore SIGHUP
   *       global_quit_flag = true;
   *   }
   *   public static void main( String [] args ) {
   *     RaiApi.RegisterSigHandler( X.class.getName(), "sigHandler" );
   *   }
   * }
   * </pre>
   **/
  public static native void RegisterSigHandler( String className,
                                      String functionName ) throws RaiException;

  /** Set parameters for controlling the api.  This is equivalent to 
   * Args processing except there is no value type checking, so this
   * should only be used in cases that Args processing is insufficient.
   * Internally, this is used when value is not a string, bool, double or
   * integer, but a complex object */
  public native boolean SetIoctl( String parameter,  Object value );

  /* Initializes C++ RaiApi Java type mapping */
  static native void initClasses();

  static boolean isInitialized = false;

  static synchronized void initRaiApi() {
    if ( ! isInitialized ) {
      try {
        System.loadLibrary( "jraiapi264" );
      } catch ( java.lang.LinkageError e ) {
        System.loadLibrary( "jraiapi2" );
      }
      initClasses();
      isInitialized = true;
    }
  }

  static {
    initRaiApi();
  }
};
