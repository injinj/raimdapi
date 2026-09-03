package com.rai.raiapi2;
import com.rai.raiexception.RaiException;

/** A timer object, which invokes the onTimer() in RaiTimerCallback at interval
 * millisecs, repeatedly until stopped.
 *
 * @see RaiQueue#CreateTimer */
public class RaiTimer {
  long     timer,
           timerCb;
  RaiQueue queue;

  /** Internal constructor
   * @see RaiQueue#CreateTimer */
  protected RaiTimer( long t,  long cb,  RaiQueue q ) {
    this.timer   = t;
    this.timerCb = cb;
    this.queue   = q;
  }
  /** Internal destructor */
  protected void finalize() {
    if ( this.timerCb != 0 )
      DeleteCB( this.timerCb );
    Delete( this.timer );
  } 
  private static native void DeleteCB( long timerCb );

  private static native void Delete( long timer );

  /** Start the timer at the current interval setting */
  public native void Start()                              throws RaiException;
  /** Stop the timer processing */
  public native void Stop();
  /** Get the current timer interval setting in milliseconds */
  public native long GetInterval()                        throws RaiException;
  /** Set the interval in milliseconds */
  public native void SetInterval( long intervalMSecs )    throws RaiException;
  /** Get the queue which this timer is on */
  public RaiQueue GetQueue() {
    return this.queue;
  }
}

