package com.rai.raiapi2;
import com.rai.raimsg.RaiMsg;

/** Interface for subscription notification.
 * @see RaiQueue#CreateInteractivePublish
 * @see RaiInteractivePublish */
public interface RaiSubscribeCallback {
  /** A subscription event occured. */
  public void onSubscribe( RaiSubscribeEvent event,  RaiMsg message,
                           Object cl );
}
