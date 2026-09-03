/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 * Mirrors com.rai.raiapi2.Args, StringArg, BoolArg, IntArg, DoubleArg,
 * Time and TimeRotate. */
using System;
using System.IO;
using System.Text;
using Com.Rai.Interop;
using Com.Rai.Raiexception;

namespace Com.Rai.Raiapi2 {

public class StringArg {
  public string name, defVal, example, description;
  public StringArg( string name,  string defVal,  string example,  string description ) {
    this.name = name; this.defVal = defVal; this.example = example; this.description = description;
  }
}
public class BoolArg {
  public string name; public bool defVal; public string example, description;
  public BoolArg( string name,  bool defVal,  string example,  string description ) {
    this.name = name; this.defVal = defVal; this.example = example; this.description = description;
  }
}
public class IntArg {
  public string name; public int defVal; public string example, description;
  public IntArg( string name,  int defVal,  string example,  string description ) {
    this.name = name; this.defVal = defVal; this.example = example; this.description = description;
  }
}
public class DoubleArg {
  public string name; public double defVal; public string example, description;
  public DoubleArg( string name,  double defVal,  string example,  string description ) {
    this.name = name; this.defVal = defVal; this.example = example; this.description = description;
  }
}

/** Command line / rc file / environment argument processing.  Add the args
 * with defaults, then processArgs( argv ) populates the values, -help prints
 * them. */
public class Args : IDisposable {
  public const int IGNORE_ARG     = 0;
  public const int RESOURCE_ARG   = 1;
  public const int COMMAND_ARG    = 2;
  public const int TIME_SEC_ARG   = 4;
  public const int TIME_MS_ARG    = 8;
  public const int MEM_ARG        = 16;
  public const int HELP_ARG       = 32;
  public const int VERSION_ARG    = 64;
  public const int PRINTRC_ARG    = 128;
  public const int RCFILE_ARG     = 256;
  public const int LIST_ARG       = 512;
  public const int NO_DEFAULT_VAL = 1024;
  public const int BITS_ARG       = 2048;

  internal IntPtr args;
  WriteAdapter    outAdapter;

  public Args() {
    this.args = Native.rai_args_create();
    if ( this.args == IntPtr.Zero ) throw new RaiApiException( "unable to create args" );
  }
  ~Args() { this.Dispose( false ); }
  public void Dispose() { this.Dispose( true ); GC.SuppressFinalize( this ); }
  void Dispose( bool d ) {
    if ( this.args != IntPtr.Zero ) { Native.rai_args_delete( this.args ); this.args = IntPtr.Zero; }
    if ( this.outAdapter != null ) { this.outAdapter.Dispose(); this.outAdapter = null; }
  }

  public void add( StringArg a ) { this.add( a, COMMAND_ARG | RESOURCE_ARG ); }
  public void add( BoolArg a ) { this.add( a, COMMAND_ARG | RESOURCE_ARG ); }
  public void add( IntArg a ) { this.add( a, COMMAND_ARG | RESOURCE_ARG ); }
  public void add( DoubleArg a ) { this.add( a, COMMAND_ARG | RESOURCE_ARG ); }
  public void add( StringArg a,  int flags ) {
    RaiException.Check( Native.rai_args_add_string( this.args, a.name, a.defVal, a.example, a.description, flags ) );
  }
  public void add( BoolArg a,  int flags ) {
    RaiException.Check( Native.rai_args_add_bool( this.args, a.name, a.defVal ? 1 : 0, a.example, a.description, flags ) );
  }
  public void add( IntArg a,  int flags ) {
    RaiException.Check( Native.rai_args_add_int( this.args, a.name, (uint) a.defVal, a.example, a.description, flags ) );
  }
  public void add( DoubleArg a,  int flags ) {
    RaiException.Check( Native.rai_args_add_double( this.args, a.name, a.defVal, a.example, a.description, flags ) );
  }
  /** Add -log, -logLevel, -logVerb, -help, -version, -rcFile, -printRC;
   * help and version output is written to out (usually Console.Error) */
  public void addDefaults( string vers,  string prefix,  TextWriter out_,  string argv0 ) {
    if ( this.outAdapter == null && out_ != null )
      this.outAdapter = new WriteAdapter( out_ );
    RaiException.Check( Native.rai_args_add_defaults( this.args, vers, prefix,
      this.outAdapter == null ? null : this.outAdapter.Fn,
      this.outAdapter == null ? IntPtr.Zero : this.outAdapter.Closure, argv0 ) );
  }
  /** Parse argv (as Main() receives it); false when -help or -version was
   * handled and the program should exit */
  public bool processArgs( string[] argv ) {
    string[] av = RaiApi.Argv( argv );
    int ok;
    RaiException.Check( Native.rai_args_process( this.args, av.Length, av, out ok ) );
    if ( this.outAdapter != null ) out_flush();
    return ok != 0;
  }
  void out_flush() { try { Console.Error.Flush(); } catch ( Exception ) {} }

  public int getNumValues( string n ) { return (int) Native.rai_args_num_values( this.args, n ); }
  public string getString( string n ) { return this.getString( n, 0 ); }
  public string getString( string n,  int num ) {
    IntPtr s; RaiException.Check( Native.rai_args_get_string( this.args, n, (uint) num, out s ) );
    return Native.Str( s );
  }
  public bool getBoolean( string n ) { return this.getBoolean( n, 0 ); }
  public bool getBoolean( string n,  int num ) {
    int v; RaiException.Check( Native.rai_args_get_bool( this.args, n, (uint) num, out v ) ); return v != 0;
  }
  public int getInt( string n ) { return this.getInt( n, 0 ); }
  public int getInt( string n,  int num ) {
    uint v; RaiException.Check( Native.rai_args_get_int( this.args, n, (uint) num, out v ) ); return (int) v;
  }
  public double getDouble( string n ) { return this.getDouble( n, 0 ); }
  public double getDouble( string n,  int num ) {
    double v; RaiException.Check( Native.rai_args_get_double( this.args, n, (uint) num, out v ) ); return v;
  }
  public void setString( string n,  string val ) { RaiException.Check( Native.rai_args_set_string( this.args, n, val ) ); }
  public void setBoolean( string n,  bool val ) { RaiException.Check( Native.rai_args_set_bool( this.args, n, val ? 1 : 0 ) ); }
  public void setInt( string n,  int val ) { RaiException.Check( Native.rai_args_set_int( this.args, n, (uint) val ) ); }
  public void setDouble( string n,  double val ) { RaiException.Check( Native.rai_args_set_double( this.args, n, val ) ); }
  public bool isSet( string n ) { return Native.rai_args_is_set( this.args, n ) != 0; }
}

/** Time functions of the api: nanosecond clocks and formatting */
public static class Time {
  public const int TZ_LOCAL_TIME = 0;
  public const int TZ_GM_TIME    = 1;

  /** Wall clock in nanoseconds since the epoch */
  public static long currentTimeNanosecs() { return Native.rai_time_current_ns(); }
  /** Format a ns timestamp with precision fractional digits, 0 = now */
  public static string nsTimestamp( long ns,  int precision ) {
    StringBuilder b = new StringBuilder( 80 );
    return Native.Str( Native.rai_time_ns_timestamp( ns, precision, b, 80 ) );
  }
  public static string usTimestamp( long us,  int precision ) { return nsTimestamp( us * 1000, precision ); }
  public static string msTimestamp( long ms,  int precision ) { return nsTimestamp( ms * 1000000, precision ); }
  /** Format a ns interval as "1.5s", "20ms" ... */
  public static string nsIntervalTime( long ns ) {
    StringBuilder b = new StringBuilder( 80 );
    return Native.Str( Native.rai_time_ns_interval( ns, b, 80 ) );
  }
  public static string usIntervalTime( long us ) { return nsIntervalTime( us * 1000 ); }
  public static string msIntervalTime( long ms ) { return nsIntervalTime( ms * 1000000 ); }
  /** High resolution monotonic clock, normalized to nanoseconds */
  public static long hiresTimeNanosecs() { return Native.rai_time_hires_ns(); }
  /** Convert a hires time to a wall clock ns timestamp */
  public static long hiresTimeToNsTimestamp( long h ) { return Native.rai_time_hires_to_ns( h ); }
  /** strftime of a ms timestamp in local or GM time */
  public static string strftime( int tz,  long ms,  string fmt ) {
    StringBuilder b = new StringBuilder( 256 );
    return Native.Str( Native.rai_time_strftime( tz, ms, fmt, b, 256 ) );
  }
  /** Current wall clock in ms (java System.currentTimeMillis()) */
  public static long currentTimeMillis() { return currentTimeNanosecs() / 1000000; }
}

/** File rotation schedule: at a time of day / week or on a period */
public class TimeRotate {
  public const int  ROTATE_UNSPECIFIED = 0;
  public const int  ROTATE_DAILY       = 1;
  public const int  ROTATE_WEEKLY      = 2;
  public const long MSECS_IN_SEC       = 1000;
  public const long MSECS_IN_DAY       = MSECS_IN_SEC * 24 * 60 * 60;
  public const long MSECS_IN_WEEK      = 7 * MSECS_IN_DAY;

  Native.TimeRotateState st;

  public TimeRotate() { this.init(); }
  public void init() { this.st = new Native.TimeRotateState(); }
  public long time { get { return this.st.time; } }
  public long period { get { return this.st.period; } }
  public long lastTime { get { return this.st.last_time; } }
  public int dayOrWeek { get { return this.st.day_or_week; } }
  public void setLastTime( long lt ) { this.st.last_time = lt; }
  /** Parse a rotate time spec, for example "00:00" (daily) or "Sun 00:00" */
  public bool setRotateTime( string timeSpec ) { return this.setRotateTime( timeSpec, ROTATE_UNSPECIFIED, 0 ); }
  public bool setRotateTime( string timeSpec,  int rotDorW,  long rotTime ) {
    return Native.rai_time_rotate_set_time( ref this.st, timeSpec, rotDorW, rotTime ) != 0;
  }
  /** Parse a period spec ("1 hour") or use rotatePeriod ms when spec is null */
  public bool setRotatePeriod( string periodSpec,  long rotatePeriod ) {
    return Native.rai_time_rotate_set_period( ref this.st, periodSpec, rotatePeriod ) != 0;
  }
  long Per() {
    if ( this.st.period != 0 ) return this.st.period;
    if ( this.st.day_or_week == ROTATE_WEEKLY ) return MSECS_IN_WEEK;
    if ( this.st.day_or_week == ROTATE_DAILY ) return MSECS_IN_DAY;
    return 0;
  }
  /** True when the rotate time has passed; advances time by the period */
  public bool checkRotate() {
    bool doRotate = false;
    if ( this.st.time != 0 ) {
      long per = this.Per();
      if ( per != 0 ) {
        long currTime = Time.currentTimeMillis();
        /* lastTime check is to rotate from a previous run */
        if ( this.st.time > this.st.last_time || this.st.time + per <= currTime ) {
          doRotate = true;
          while ( this.st.time > currTime ) this.st.time -= per;
          while ( this.st.time + per <= currTime ) this.st.time += per;
        }
        this.st.last_time = currTime;
      }
    }
    return doRotate;
  }
  /** [ next rotate time ms, ms until then ], zeros when not rotating */
  public long[] nextRotate() {
    long diffTime = 0, nextTime = 0;
    if ( this.st.time != 0 ) {
      nextTime = this.Per();
      if ( nextTime != 0 ) {
        long currTime = Time.currentTimeMillis();
        if ( this.st.time > this.st.last_time )
          nextTime = currTime;
        else {
          nextTime += this.st.time;
          if ( nextTime < currTime ) nextTime = currTime;
        }
        diffTime = nextTime - currTime;
      }
    }
    return new long[] { nextTime, diffTime };
  }
  public long getInterval() { return this.st.time == 0 ? 0 : this.Per(); }
}

} /* namespace */
