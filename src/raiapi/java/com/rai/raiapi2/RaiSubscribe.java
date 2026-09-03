package com.rai.raiapi2;
import com.rai.raiexception.RaiException;

/** RaiSubscribe is the object which the application specifies interest in
 * subjects and wildcards.  A subject may have an image (SNAP) associated with
 * it and/or delta updates (UPDATE).
 * @see RaiQueue#CreateSubscribe
 * @see RaiMsgEvent
 * @see RaiMsgCallback */
public class RaiSubscribe {
  /** In Start(), if parm == UPDATE, then only updates are requested. */
  public final static int UPDATE = 1;
  /** In Start(), if parm == SNAP, then only an image is requested. */
  public final static int SNAP   = 2;
  /** In Start(), if parm == BOTH (or SNAP | UPDATE), then an image is
   * requested followed by updates. */
  public final static int BOTH   = 3;
  /** In Start(), if ( parm &amp; NO_PREIX ), then no prefix is used to
   * snapshot the subject.  In the case of the tibrv api, this is usually
   * "_SNAP.", but has no effect in other apis. */
  public final static int NO_PREFIX = 4;
  /** In Start(), if ( parm &amp; NO_COPY ), then the RaiMsg message objects
   * dispatched in the RaiMsgCallback are reused.  The same message object
   * will be used for every callback, with different data contents.  If the
   * used scope of the message is only used in callback, then use this option,
   * it is faster since java doesn't need to allocate and garbage collect
   * message objects. */
  public final static int NO_COPY = 8;

  /** The RaiSubscribe has not yet seen a message. */
  public final static int STATE_NO_MSG    = 0;
  /** The RaiSubscribe is a wildcard, no state transitions. */
  public final static int STATE_WILDCARD  = 1;
  /** The RaiSubscribe has seen a message, but there were no SASS control
   * fields. */
  public final static int STATE_NO_HDR    = 2;
  /** The RaiSubscribe has seen an INITIAL value. */
  public final static int STATE_INITIAL   = 3;
  /** The RaiSubscribe has seen an UPDATE value. */
  public final static int STATE_UPDATE    = 4;
  /** The RaiSubscribe has seen a NOT_FOUND status value. */
  public final static int STATE_NOTFOUND  = 5;
  /** The RaiSubscribe has seen a status value which causes the subscription
   * to be STALE. */
  public final static int STATE_STALE     = 6;
  /** The RaiSubscribe has seen a status value which causes the subscription
   * to be DROPPED. */
  public final static int STATE_DROPPED   = 7;

  /** State string values */
  final static String state_str[] = {
    "NO_MSG", "WILDCARD", "NO_HDR", "INITIAL",
    "UPDATE", "NOTFOUND", "STALE", "DROPPED"
  };

  long     subscribe,
           msgCb;
  RaiQueue queue;
  /** The current state.  This is modified when a messages arrive.  Each
   * message is classified with one of the STATE_* enumerations in this class.
   * The initial state is either NO_MSG, or WILDCARD when subject the Start()ed
   * is a wildcard.  The state transition map is:<p>
   * <table border="1">
   * <caption>The subscription state transitions</caption>
   * <tr><th colspan="2" rowspan="2"><th colspan="8">recv state:<th rowspan="2"></tr>
   * <tr><th>NO_MSG<th>WILDCARD<th>NO_HDR<th>INITIAL<th>UPDATE<th>NOTFOUND<th>STALE<th>DROPPED</tr>
   * <tr><th rowspan="8">old state:<th>NO_MSG<td>NO_MSG<td>WILDCARD<td>NO_HDR<td>INITIAL<td>UPDATE<td>NOTFOUND<td>STALE<td>DROPPED<th rowspan="8">:new state</tr>
   * <tr><th>WILDCARD<td>WILDCARD<td>WILDCARD<td>WILDCARD<td>WILDCARD<td>WILDCARD<td>WILDCARD<td>WILDCARD<td>WILDCARD</tr>
   * <tr><th>NO_HDR<td>NO_HDR<td>WILDCARD<td>NO_HDR<td>INITIAL<td>UPDATE<td>NOTFOUND<td>STALE<td>DROPPED</tr>
   * <tr><th>INITIAL<td>INITIAL<td>INITIAL<td>INITIAL<td>INITIAL<td>INITIAL<td>INITIAL<td>STALE<td>DROPPED</tr>
   * <tr><th>UPDATE<td>UPDATE<td>UPDATE<td>NO_HDR<td>INITIAL<td>UPDATE<td>UPDATE<td>STALE<td>DROPPED</tr>
   * <tr><th>NOTFOUND<td>NOTFOUND<td>NOTFOUND<td>NO_HDR<td>INITIAL<td>UPDATE<td>NOTFOUND<td>STALE<td>DROPPED</tr>
   * <tr><th>STALE<td>STALE<td>STALE<td>STALE<td>INITIAL<td>UPDATE<td>NOTFOUND<td>STALE<td>DROPPED</tr>
   * <tr><th>DROPPED<td>DROPPED<td>DROPPED<td>DROPPED<td>INITIAL<td>DROPPED<td>DROPPED<td>DROPPED<td>DROPPED</tr>
   * </table> */
  public int state;

  /** Internal constructor.
   * @see RaiQueue#CreateSubscribe */
  protected RaiSubscribe( long s,  long m,  RaiQueue q ) {
    this.subscribe = s;
    this.msgCb     = m;
    this.queue     = q;
    this.state     = STATE_NO_MSG;
  }
  /** Internal destructor */
  protected void finalize() {
    if ( this.msgCb != 0 )
      DeleteCB( this.msgCb );
    Delete( this.subscribe );
  } 
  private static native void DeleteCB( long msgCb );

  private static native void Delete( long subscribe );

  /** Convert a state enumeration to a string.  For example,
   * RaiSubscribe.StateToString( RaiSubscribe.NO_MSG ) = "NO_MSG". */
  public final static String StateToString( int state ) {
    return state_str[ state & 7 ];
  }
  /** Convert the current state enumeration to a string. */
  public String GetStateString() {
    return StateToString( this.state );
  }
  /** Start subscription or make the snapshot request, with no timeout, see
   * below.
   * */
  public void Start( String subject, int parm ) throws RaiException {
    this.Start( subject, parm, 0 );
  }
  /** Start subscription or make the snapshot request, with timeout.
   * @param subject The subject for the subscription.
   * @param parm Configures the subscription for a subscription, a snapshot, or
   * both.  This is a bit mask of SNAP, UPDATE, NO_PREFIX, NO_COPY.  BOTH is
   * equivalent to SNAP | UPDATE.  If UPDATE is present, then potentially many
   * update messages will stream as the cached message changes.  If NO_COPY is
   * used, then the RaiMsg message objects dispatched in the RaiMsgCallback are
   * reused.  The same message object will be used for every callback, with
   * different data contents.  If the used scope of the message is only used in
   * callback, then use this option, it is faster since java doesn't need to
   * allocate and garbage collect message objects.
   * @param timeoutMSecs Time to wait in milliseconds. 0 to wait
   * indefinitely.  This is only valid if ( parm &amp; SNAP ) is true.  When
   * a SNAP times out then a message with MSG_TYPE = TRANSIENT, and REC_STATUS
   * = STATUS_TIMEOUT is dispatched to the onMsg() callback.
   * */
  public native void Start( String subject,  int parm,
                            int timeoutMSecs )      throws RaiException;
  /** Cancel subscription or snapshot */
  public native void Cancel()                       throws RaiException;
  /** Refresh subscription by requesting initial or snapshot.
   * @param timeoutMSecs Time to wait in milliseconds for new snapshot. */
  public native void Refresh( int timeoutMSecs )    throws RaiException;
  /** The name of subject that is subscribed passed in Start() */
  public native String Subject();
  /** Determine whether the subscription is active. Will be set to true on
   * start(); Will be set to inactive when after a snapshot has been fulfilled
   * and when a cancel has been processed. */
  public native boolean InProgress();
  /** Get queue subscription is on */
  public RaiQueue GetQueue() {
    return this.queue;
  }
};
