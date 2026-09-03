package com.rai.raiapi2;
import com.rai.raiexception.RaiException;

/** An interactive publisher filters publishing by subscriber interest.
 * @see RaiQueue#CreateInteractivePublish */
public class RaiInteractivePublish extends RaiPublish {
  long     interactive,
           subCb;
  RaiQueue queue;

  /** Constructor used internally.
   * @see RaiQueue#CreateInteractivePublish */
  protected RaiInteractivePublish( long i,  long cb,  RaiQueue q,
                                   long p ) {
    super( p, q.GetSession() );
    this.interactive = i;
    this.subCb       = cb;
    this.queue       = q;
  }
  /** Destructor used internally. */
  protected void finalize() {
    if ( this.subCb != 0 )
      DeleteCB( this.subCb );
    Delete( this.interactive );
  } 
  private static native void DeleteCB( long subCb );

  private static native void Delete( long interactive );

  /** Start interactive publish.  The subject string is usually a wildcard
   * which filters subscription interest.  When a subscription event occurs
   * which matches the subject started, then the
   * RaiSubscribeCallback.onSubscribe() is notified.  The wildcards are '*'
   * which matches any segment, and '&gt;' which matches any suffix.
   *
   * <p>Example:
   * <p>"TEST.*" matches TEST.STRING<br>
   * "TEST.*" does not match TEST, or TEST.ONE.TWO
   *
   * <p>"TEST.&gt;" matches TEST.STRING, TEST.ONE.TWO<br>
   * "TEST.&gt;" does not match TEST
   *
   * <p>"&gt;" matches everything<br>
   *
   * @param subject The subject to match, which can be a wildcard.
   * @see RaiQueue#CreateInteractivePublish
   * @see RaiSubscribeCallback */
  public native void InteractiveStart( String subject ) throws RaiException;
  /** Cancel interactive publish.  This stops subscription event notification.
   */
  public native void InteractiveCancel();
  /** Whether interactive publisher is in progress.  If a InteractiveStart()
   * was called and suceeded and InteractiveCancel() was not called, this is
   * true. */
  public native boolean InProgress();
  /** Get the queue the interactive publisher is on.  Subscription events are
   * dispatched through this queue. */
  public RaiQueue GetQueue() {
    return this.queue;
  }
}

