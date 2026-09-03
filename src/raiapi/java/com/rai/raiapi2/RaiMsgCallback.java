package com.rai.raiapi2;
import com.rai.raimsg.RaiMsg;

/** RaiMsgCallback is the interface for message events.
  * @see RaiQueue#CreateSubscribe */
public interface RaiMsgCallback {
  /** The message callback.
   * @see RaiQueue#CreateSubscribe */
  public void onMsg( RaiMsgEvent event,  RaiMsg msg,  Object closure );
}

