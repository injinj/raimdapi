package com.rai.raiapi;
import com.rai.raimsg.*;

public class SubHandle {
  public RaiCallback cb;
  public Object arg;
  public String theSubject;

  public SubHandle( RaiCallback tcallback, Object targ, String ttheSubject )
  {
    cb = tcallback;
    arg = targ;
    theSubject = ttheSubject;
  }
}
