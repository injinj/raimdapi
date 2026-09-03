package com.rai.raiapi;

import com.rai.raimsg.*;

public interface RaiTimerCallback {
  public void  onTimer(RaiSession session, Object closure);
}


