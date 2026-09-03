package com.rai.raiapi;

import com.rai.raimsg.*;

public interface RaiCallback {
  public void  onMsg(RaiEvent event, RaiMsg raiMsg, Object closure);
}


