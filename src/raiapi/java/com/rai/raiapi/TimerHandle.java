package com.rai.raiapi;


public class TimerHandle {
  public RaiTimerCallback cb;
  public Object           arg;

  public TimerHandle( RaiTimerCallback tcb,  Object targ ) {
    this.cb  = tcb;
    this.arg = targ;
  }
}
