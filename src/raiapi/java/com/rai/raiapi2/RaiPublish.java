package com.rai.raiapi2;
import com.rai.raiexception.RaiException;
import com.rai.raimsg.RaiMsg;

/** RaiPublish is the object which allows publishing messages to subjects.
 * @see RaiSession#CreatePublish */
public class RaiPublish {
  long       publish;
  RaiSession session;

  /** Internally used constructor
   * @see RaiSession#CreatePublish */
  protected RaiPublish( long p,  RaiSession s ) {
    this.publish = p;
    this.session = s;
  }
  /** Internally used destructor */
  protected void finalize() {
    Delete( this.publish );
  } 
  private static native void Delete( long publish );

  /** Publish a message on a subject, with no timestamp, see below. */
  public void Publish( String subject,  RaiMsg raiMsg ) throws RaiException {
    this.Publish( subject, raiMsg, 0 );
  }
  /** Publish a message on a subject, stamp is a nanoseconds UTC based timestamp
   * which is available in Time.nsTimestamp(), if it is zero, it will be
   * assigned by the Publish() method. This is the same timestamp that appears
   * to subscribers in RaiMsgEvent.pubTime.
   * @see RaiMsgEvent#pubTime */
  public native void Publish( String subject,  RaiMsg raiMsg,  long stamp )
                                                        throws RaiException;
  /** Publish a message buffer on a subject, with no timestamp, see below. */
  public void Publish( String subject,  byte[] buffer,  int offset,
                       int length ) throws RaiException {
    this.Publish( subject, buffer, offset, length, 0 );
  }
  /** Publish a message buffer on a subject, stamp is a nanoseconds UTC based
   * timestamp which is available in Time.nsTimestamp(), if it is zero, it will
   * be assigned by the Publish() method.  This is the same timestamp that
   * appears to subscribers in RaiMsgEvent.pubTime.
   * @see RaiMsgEvent#pubTime */
  public native void Publish( String subject,  byte[] buffer,
                              int offset,  int length,  long stamp )
                                                        throws RaiException;
  /** On publish, add prefix to subject with this, the tibrv api (sass2/rv)
   * often uses "_TIC." */
  public native void SetPrefix( String prefix )         throws RaiException;
  /** Get the prefix value */
  public native String GetPrefix();
  /** Get the nextSeqno value, when using SASS SEQ_NO field definition, it will
   * only use the lower 16 bits */
  public native long GetSeqno();
  /** Set the nextSeqno value, when using SASS SEQ_NO field definition, it will
   * only use the lower 16 bits */
  public native void SetSeqno( long newSeqno );
  /** Destroy publisher */
  public native void Destroy();
  /** Get parent session that publisher was created from */
  public RaiSession GetSession() {
    return this.session;
  }
}
