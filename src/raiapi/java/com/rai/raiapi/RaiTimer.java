
/******************************************************************************
 *
 * Timer Interface Class
 *
 *****************************************************************************/
package com.rai.raiapi;

import java.util.*;
import com.rai.raimsg.*;


public class RaiTimer {
  com.rai.raiapi2.RaiTimer timer;
  TimerHandle cl;

  public
  RaiTimer( RaiSession session, RaiTimerCallback callback,
            int interval, Object closure ) throws RaiException
  {
    try {
      this.cl = this.addTimer( callback, closure );
      this.timer = session.defaultQueue2.CreateTimer( session, cl );
      this.timer.SetInterval( interval );
      this.timer.Start();
    } catch ( com.rai.raiexception.RaiException e ) {
      throw RaiApi.getException( e, "CreateTimer" );
    }
  }

  protected TimerHandle
  addTimer( RaiTimerCallback callback, Object closure )
  {
    return new TimerHandle( callback, closure );
  }

  public int
  getInterval()
  {
    try {
      if ( this.timer != null )
        return (int) this.timer.GetInterval();
    } catch ( com.rai.raiexception.RaiException e ) {
    }
    return 0;
  }

  public void
  setInterval( int intervalMS ) throws RaiException
  {
    try {
      if ( this.timer != null )
        this.timer.SetInterval( intervalMS );
    } catch ( com.rai.raiexception.RaiException e ) {
    }
  }

  public void
  Destroy()
  {
    com.rai.raiapi2.RaiTimer t = this.timer;
    if ( t != null ) {
      this.timer = null;
      this.cl = null;
      t.Stop();
    }
  }
}
