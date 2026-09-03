package com.rai.raiapi2;

/** Args provides a store of name value pairs as well as example and
* a short description of what a switch does.  This class processes command
* line, resource file configurations, and an array of name[]/value[] pairs.
* Each argument has a type, some attributes, a default value, an example,
* and a short description.
*
* <p>Example:
*
* <p><pre>
import com.rai.raiapi2.*;
import com.rai.raiexception.RaiException;

public class argtest {
  public static void main( String [] args ) {
    Args a = new Args();
    StringArg s = new StringArg( "description", null, "&lt;string&gt;",
                        "A description of the program" );
    DoubleArg d = new DoubleArg( "rate", 1000, "&lt;bits&gt;",
                        "The message rage" );

    try {
      a.add( s );
      a.add( d, Args.BITS_ARG | Args.COMMAND_ARG | Args.RESOURCE_ARG );
      a.addDefaults( "1.0", "argtest_", System.err, "argtest" );
      if ( a.processArgs( args ) ) {
        System.out.println( "description = " + a.getString( s.name ) );
        System.out.println( "rate = " + a.getDouble( d.name ) );
      }
    } catch ( RaiException e ) {
      System.err.println( e );
    }
  }
}

$ javac argtest.java

$ java argtest -help
Usage:
  -description &lt;string&gt;       A description of the program
  -rate &lt;bits&gt;                   The message rage (default: 1.000kbit)
  -log &lt;file&gt;                    Log filename, use '-' for stderr (default: -)
  -logLevel &lt;dbg|min|norm|err&gt;   Error level of logging (default: minor)
  -logVerb &lt;1-5&gt;              Verbosity level of logging (default: 4)
  -logXml                        Use XML log format (default: no)
  -help                          Display help and exit
  -version                       Display version and exit
  -printRC                       Print arguments in resource format and exit
  -rcFile &lt;file&gt;                 Load arguments from resource file (default:
                                 argtest.ini)

$ java argtest -description "hello world" -rate 1 mbit
description = hello world
rate = 1000000.0

$ argtest_description="hello world2" argtest_rate="5 kbits" java argtest
description = hello world2
rate = 5000.0

$ java argtest -printRC 2&gt;&amp;1 | sed 's/# description=/description=hello world3/' &gt; argtest.ini
$ java argtest -rate 1 mbyte
description = hello world3
rate = 8388608.0
* </pre>
*
* @see StringArg
* @see BoolArg
* @see IntArg
* @see DoubleArg
*
*/

public class Args {
  long args;
  boolean ownArgs;

  /** If flags passed to add() are bit mask = 0, then argument is ignored and
   * not parsed. */
  public static final int IGNORE_ARG      = 0;
  /** If flags passed to add() &amp; RESOURCE_ARG, then arg can be present in
   * -rcFile. */
  public static final int RESOURCE_ARG    = 1;
  /** If flags passed to add() &amp; COMMAND_ARG, then arg can be present on
   * the command line.  */
  public static final int COMMAND_ARG     = 2;
  /** If flags passed to add() &amp; TIME_SEC_ARG, then arg is processed with
   * an optional time specifier (seconds, minutes, etc) then converted to
   * seconds, so -t "1 minute" would be parsed as 60 seconds. */
  public static final int TIME_SEC_ARG    = 4;
  /** If flags passed to add() &amp; TIME_MS_ARG, then arg is processed with an
   * optional time specifier (milliseconds, seconds, minutes, etc) then
   * converted to milliseconds, so -t "1 minute" would be parsed as 60000
   * milliseconds. */
  public static final int TIME_MS_ARG     = 8;
  /** If flags passed to add() &amp; MEM_ARG, then arg is processed with an
   * optional memory specifier (kb, mb, gb) then converted to bytes, so -m 1kb
   * would be parsed as 1024 bytes.  */
  public static final int MEM_ARG         = 16;
  /** If flags passed to add() &amp; HELP_ARG, then program prints option help
   * on the command line, this attribute is used with the -help argument. */
  public static final int HELP_ARG        = 32;
  /** If flags passed to add() &amp; VERSION_ARG, then program prints the
   * version on the command line, this attribute is used with the -version
   * argument. */
  public static final int VERSION_ARG     = 64;
  /** If flags passed to add() &amp; PRINTRC_ARG, then program prints an
   * rcFile.ini style option list on the command line, this attribute is used
   * with the -printRC argument, it is intended for use with -rcFile: prog
   * -printRC 2&gt; args.ini ; edit args.ini ; prog -rcFile args.init */
  public static final int PRINTRC_ARG     = 128;
  /** If flags passed to add() &amp; RCFILE_ARG, then program loads arguments
   * from a file, this attribute is used with -rcFile.  */
  public static final int RCFILE_ARG      = 256;
  /** If flags passed to add() &amp; LIST_ARG, then option can hold a list of
   * arguments:
   * -option val1 val2 val3 */
  public static final int LIST_ARG        = 512;
  /** If flag passed to add() &amp; NO_DEFAULT_VAL, the option does not have a
   * default value */
  public static final int NO_DEFAULT_VAL  = 1024;
  /** If flags passed to add() &amp; BITS_ARG, then arg is processed with an
   * optional memory specifier (mbits, gbits, kb, mb, gb) then converted to
   * bits, so -m 1mbit would be parsed as 1000000 bits.  */
  public static final int BITS_ARG        = 2048;

  /** Args provides a store of name value pairs as well as example and
   * a short description of what a switch does.
   */
  public Args() {
    this.args = Create();
    this.ownArgs = true;
  }
  protected Args( long a ) {
    this.args = a;
    this.ownArgs = false;
  }
  protected static native long Create();

  protected void finalize() {
    if ( this.ownArgs )
      Delete( this.args );
  }
  private static native void Delete( long args );

  /** Add an arg of type String with flags (COMMAND_ARG | RESOURCE_ARG) */
  public void add( StringArg a ) throws RaiApiException {
    this.add( a, COMMAND_ARG | RESOURCE_ARG );
  }
  /** Add an arg of type Bool with flags (COMMAND_ARG | RESOURCE_ARG) */
  public void add( BoolArg a ) throws RaiApiException {
    this.add( a, COMMAND_ARG | RESOURCE_ARG );
  }
  /** Add an arg of type Int with flags (COMMAND_ARG | RESOURCE_ARG) */
  public void add( IntArg a ) throws RaiApiException {
    this.add( a, COMMAND_ARG | RESOURCE_ARG );
  }
  /** Add an arg of type Double with flags (COMMAND_ARG | RESOURCE_ARG) */
  public void add( DoubleArg a ) throws RaiApiException {
    this.add( a, COMMAND_ARG | RESOURCE_ARG );
  }
  /** Add an arg of type String, with flags specifying attributes  */
  public native void add( StringArg a,  int flags ) throws RaiApiException;

  /** Add an arg of type Bool , with flags specifying attributes  */
  public native void add( BoolArg a,  int flags ) throws RaiApiException;

  /** Add an arg of type Int , with flags specifying attributes  */
  public native void add( IntArg a,  int flags ) throws RaiApiException;

  /** Add an arg of type Double , with flags specifying attributes  */
  public native void add( DoubleArg a,  int flags ) throws RaiApiException;

  /** Add -version (set as vers arg), -log, -help, -version, -rc arguments. */
  public native void addDefaults( String vers,  String prefix,
                                  java.io.OutputStream out,
                                  String argv0 ) throws RaiApiException;
  /** Sets the arguments in command line format: -subject TEST,
   * then tests whether -help or -rcFile, or -version is specified and
   * performs the action, if program should continue, this returns true. */
  public native boolean processArgs( String[] argv ) throws RaiApiException;

 /** Return the number of items associated with arg "n", will be 0
  * if arg has no value, 1 if arg has a value, N if arg has flags
  * LIST_ARG set and more than 1 value associated with it. */
  public native int getNumValues( String n ) throws RaiApiException;

  /** Get the string value of the arg named "n" */
  public String getString( String n ) throws RaiApiException {
    return this.getString( n, 0 );
  }
  /** Get the string value of the arg named "n", if num &gt; 0, then get the
   * nth item, use getNumValues() to determine the number of items, the type of
   * arg must be a string, the getType() functions don't convert args. */
  public native String getString( String n,  int num ) throws RaiApiException;

  /** Get the boolean value of arg named "n", type must be BoolArg. */
  public boolean getBoolean( String n ) throws RaiApiException {
    return this.getBoolean( n, 0 );
  }
  /** Get the boolean value of arg named "n", type must be BoolArg, num
   * indicates the nth item in a list. */
  public native boolean getBoolean( String n,  int num ) throws RaiApiException;

  /** Get the int value of arg named "n", type must be IntArg. */
  public int getInt( String n ) throws RaiApiException {
    return this.getInt( n, 0 );
  }
  /** Get the int value of arg the named "n", if num &gt; 0, then get the nth
   * item, use getNumValues() to determine the number of items. */
  public native int getInt( String n,  int num ) throws RaiApiException;

  /** Get the double value of arg named "n", type must be DoubleArg. */
  public double getDouble( String n ) throws RaiApiException {
    return this.getDouble( n, 0 );
  }
  /** Get the double value of arg named "n", type must be DoubleArg, num
   * indicates the nth item in a list. */
  public native double getDouble( String n,  int num ) throws RaiApiException;

 /** Set the value of the arg named "n", if arg is not a string type, then this
  * function attepts to parse the argument and convert it to int, double,
  * boolean. */
  public native void setString( String n,  String val ) throws RaiApiException;

  /** Set the value of arg "n", type must be IntArg. */
  public native void setInt( String n,  int val ) throws RaiApiException;

  /** Set the value of arg "n", type must be BoolArg. */
  public native void setBoolean( String n,  boolean val ) throws RaiApiException;

  /** Set the value of arg "n", type must be DoubleArg. */
  public native void setDouble( String n,  double val ) throws RaiApiException;

  /** Test whether an argument exists  */
  public native boolean exists( String s ) throws RaiApiException;

  public native void printRC( java.io.OutputStream out ) throws RaiApiException;

  public native void printHelp( java.io.OutputStream out )
                                                         throws RaiApiException;
  static {
    RaiApi.initRaiApi();
  }
};

