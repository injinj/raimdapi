package com.rai.raiapi2;
import com.rai.raiexception.RaiException;

/** Utilities for manipulating nanosecond resolution timestamps */
public class Time {
  /** Current time in nanoseconds, UTC time seconds * 10^9 + nanosecs */
  public static native long currentTimeNanosecs();

  /** Convert nanoseconds UTC into string "stamp + .sub-second-precision",
   * for example: "2011-06-20 12:37:32.162" */
  public static native String nsTimestamp( long ns,  int precision );

  /** Convert microseconds UTC into string, see above. */
  public static String usTimestamp( long us,  int precision ) {
    return nsTimestamp( us * 1000, precision );
  }
  /** Convert milliseconds UTC into string, see above. */
  public static String msTimestamp( long ms,  int precision ) {
    return nsTimestamp( ms * 1000000, precision );
  }
  /** Format nanoseconds as a string, example: "1 second", when ns is
   * a divisible factor of 60*10^9 */
  public static native String nsIntervalTime( long ns );

  /** Format microseconds as a string, example: "10 microsecond" when us = 10 */
  public static String usIntervalTime( long us ) {
    return nsIntervalTime( us * 1000 );
  }
  /** Format milliseconds as a string, example: "90 milliseconds" when ms = 90
   * */
  public static String msIntervalTime( long ms ) {
    return nsIntervalTime( ms * 1000000 );
  }
  /** Monotonic timestamp in nanoseconds, does not go backwards like
   * UTC based times can when the clock drifts */
  public static native long hiresTimeNanosecs();

  /** Convert a monotonic timestamp to a UTC timestamp.  This avoids the
   * overhead of a syscall for nsTimestamp() */
  public static native long hiresTimeToNsTimestamp( long h );

  /** Format time string in local time zone */
  public final static int TZ_LOCAL_TIME = 0;
  /** Format time string in GM time zone */
  public final static int TZ_GM_TIME    = 1;

  /** Same as 'C' version of strftime, except first option is to change
   * timezone to UTC (TZ_GM_TIME) */
  public static native String strftime( int tz,  long ms,  String fmt )
                                                         throws RaiException;
  static {
    RaiApi.initRaiApi();
  }
};

