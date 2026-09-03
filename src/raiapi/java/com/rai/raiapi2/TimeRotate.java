package com.rai.raiapi2;

/** Utility for tracking time when rotating log files or other output files.
 * It uses a base time and an interval to calculate when the timer expires.
 * This is different from a timer in that it uses the wall clock time to
 * determine the expiration instead of an offset from the current time.
 *
 * <p>Example:
 * <p><pre>
 * import com.rai.raiapi2.*;
 * import com.rai.raiexception.RaiException;
 *
 * public class testr implements RaiTimerCallback {
 *   RaiApi api;
 *   RaiSession session;
 *   RaiQueue queue;
 *   RaiTimer timer;
 *   TimeRotate rotate;
 *
 *   public void onTimer( RaiTimer t,  Object cl ) {
 *     if ( rotate.checkRotate() )
 *       System.out.println( "Rotate expired: " +  
 *         Time.msTimestamp( System.currentTimeMillis(), 0 ) );
 *   }
 *
 *   public static void main( String [] argv ) {
 *     try {
 *       Args args = new Args();
 *       testr me = new testr();
 *       me.api = RaiApi.RaiOpen( null, argv );
 *       me.api.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
 *       me.api.GetArgs( args );
 *       args.addDefaults( me.api.RaiVersion(), "rai_", System.err, "test" );
 *       if ( args.processArgs( argv ) ) {
 *         me.api.ParseArgs( args );
 *         RaiApi.OpenLog( args );
 *         me.session = me.api.CreateSession();
 *         me.queue = me.session.CreateQueue();
 *         me.timer = me.queue.CreateTimer( me );
 *         me.rotate = new TimeRotate();
 *         me.rotate.setRotateTime( "12:00" );
 *         me.rotate.setRotatePeriod( "15 seconds", 0 );
 *         me.rotate.setLastTime( System.currentTimeMillis() );
 *         me.timer.SetInterval( 1000 );
 *         me.timer.Start();
 *         me.queue.Mainloop();
 *       }
 *     } catch ( RaiException e ) {
 *       System.err.println( "Failed: " + e );
 *     }
 *   }
 * }
 * </pre>
 * <p>Compile and run:
 * <pre>
 * $ javac testr.java
 * $ java testr
 * Rotate expired: 2011-07-20 12:27:00
 * Rotate expired: 2011-07-20 12:27:15
 * Rotate expired: 2011-07-20 12:27:30
 * Rotate expired: 2011-07-20 12:27:45
 * Rotate expired: 2011-07-20 12:28:00
 * Rotate expired: 2011-07-20 12:28:15
 * </pre>
 */
public class TimeRotate {
  /** No dayOrWeek specified */
  static final public int ROTATE_UNSPECIFIED = 0;
  /** Rotate daily */
  static final public int ROTATE_DAILY       = 1;
  /** Rotate weekly */
  static final public int ROTATE_WEEKLY      = 2;
  /** Milliseconds in second */
  static final public long MSECS_IN_SEC      = 1000;
  /** Milliseconds in day */
  static final public long MSECS_IN_DAY      = MSECS_IN_SEC * 24 * 60;
  /** Milliseconds in week */
  static final public long MSECS_IN_WEEK     = 7 * MSECS_IN_DAY;

  long time,
       period,
       lastTime;
  int  dayOrWeek;

  public TimeRotate() {
    this.init();
  }
  /** Reset to zero */
  public void init() {
    this.time      = 0;
    this.period    = 0;
    this.lastTime  = 0;
    this.dayOrWeek = ROTATE_UNSPECIFIED;
  }
  /** Begin calculating at this timestamp. */
  public void setLastTime( long lt ) {
    this.lastTime = lt;
  }
  /** The wall clock time that the timer should fire. */
  public boolean setRotateTime( String timeSpec ) {
    return this.setRotateTime( timeSpec, 0, 0 );
  }
  public native boolean setRotateTime( String timeSpec, int rotDorW,
                                       long rotTime );
  /** The interval that the timer should fire. */
  public native boolean setRotatePeriod( String periodSpec,
                                         long rotatePeriod );
  /** Check whether the timer expired. */
  public boolean checkRotate() {
    long    currTime, per;
    boolean doRotate = false;

    if ( this.time != 0 ) {
      if ( this.period != 0 ) {
        per = this.period;
      }
      else {
        if ( this.dayOrWeek == ROTATE_WEEKLY )
          per = MSECS_IN_WEEK;
        else if ( this.dayOrWeek == ROTATE_DAILY )
          per = MSECS_IN_DAY;
        else
          per = 0;
      }

      if ( per != 0 ) {
        currTime = System.currentTimeMillis();
        /* lastTime check is to rotate from a previous run */
        if ( this.time > this.lastTime ||
             this.time + per <= currTime ) {
          doRotate = true;
          while ( this.time > currTime )
            this.time -= per;
          while ( this.time + per <= currTime )
            this.time += per;
        }
        this.lastTime = currTime;
      }
    }
    return doRotate;
  }

  /** Return an array of the next rotate timestamp and the delta time until
   * then. */
  public long [] nextRotate() {
    long currTime, diffTime, nextTime;

    if ( this.time == 0 ) {
      diffTime = 0;
      nextTime = 0;
    }
    else {
      if ( this.period != 0 ) {
        nextTime = this.period;
      }
      else {
        if ( this.dayOrWeek == ROTATE_WEEKLY )
          nextTime = MSECS_IN_WEEK;
        else if ( this.dayOrWeek == ROTATE_DAILY )
          nextTime = MSECS_IN_DAY;
        else
          nextTime = 0;
      }

      if ( nextTime != 0 ) {
        currTime = System.currentTimeMillis();
        /* lastTime check is to rotate from a previous run */
        if ( this.time > this.lastTime )
          nextTime = currTime;
        else {
          nextTime += this.time;
          if ( nextTime < currTime )
            nextTime = currTime;
        }
        diffTime = nextTime - currTime;
      }
      else {
        diffTime = 0;
      }
    }
    long [] t = new long[ 2 ];
    t[ 0 ] = nextTime;
    t[ 1 ] = diffTime;
    return t;
  }

  /** Return the current timer interval. */
  public long getInterval() {
    if ( this.time == 0 )
      return 0;
    if ( this.period != 0 )
      return this.period;
    if ( this.dayOrWeek == ROTATE_WEEKLY )
      return MSECS_IN_WEEK;
    if ( this.dayOrWeek == ROTATE_DAILY )
      return MSECS_IN_DAY;
    return 0;
  }
};

