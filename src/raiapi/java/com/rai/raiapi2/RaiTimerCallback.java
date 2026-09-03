package com.rai.raiapi2;

/** This is the interface for timer events.
 * @see RaiQueue#CreateTimer
 * @see RaiTimer */
public interface RaiTimerCallback {
  /** When a timer event expired, this will be called. */
  public void onTimer( RaiTimer timer,  Object cl );
};

