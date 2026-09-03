package com.rai.raiapi2;

/** Event passed to RaiSubscribeCallback.onSubscribe() when an
 * RaiInteractivePublish recvs a subscription notification.
 * @see RaiQueue#CreateInteractivePublish
 * @see RaiInteractivePublish
 * @see RaiSubscribeCallback */
public class RaiSubscribeEvent {
  public RaiInteractivePublish publish;
  public String                subject;
  public String                reply;
  public int                   queryFlags;

  protected RaiSubscribeEvent( RaiInteractivePublish p,  String subj,
                               String rep,  int fl ) {
    this.publish    = p;
    this.subject    = subj;
    this.reply      = rep;
    this.queryFlags = fl;
  }
};

