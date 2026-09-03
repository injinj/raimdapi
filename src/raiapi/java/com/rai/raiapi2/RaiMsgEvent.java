package com.rai.raiapi2;

/** Event passed to RaiMsgCallback.onMsg() when a subject matches the interest
 * of a RaiSubscribe subscription.
 * @see RaiQueue#CreateSubscribe
 * @see RaiSubscribe
 * @see RaiMsgCallback */
public class RaiMsgEvent {
  public static final int SNAP   = 0;
  public static final int UPDATE = 1;
  /** The subscription that started the subject */
  public RaiSubscribe subscribe;
  /** The recieved subject, which in the case of tibrv, may be an _INBOX
   * subject */
  public String       subject;
  /** The type of message event: SNAP, or UPDATE */
  public int          type;
  /** The MSG_TYPE of the message, for example: INITIAL, UPDATE, SNAPSHOT
   * @see com.rai.raimsg.SassConst */
  public short        msgType;
  /** The REC_STATUS of the message, for example: STATUS_OK, STATUS_TEMP_UNAVAIL
   * @see com.rai.raimsg.SassConst */
  public short        recStatus;

  /** The previous state. */
  public int oldState;
  /** The state derived from msgType, recStatus of the message. */
  public int recv;
  /** The state new state derived from oldState + recv = state.
   * These constants are defined in RaiSubscribe.  The state transition map is:<p>
   * <table border="1">
   *   <caption>The subscription state transitions</caption>
   *   <tr>
   *     <th rowspan="2" colspan="2"></th>
   *     <th colspan="8">recv state:</th>
   *     <th rowspan="2"></th>
   *   </tr>
   *   <tr> <th>NO_MSG</th> <th>WILDCARD</th> <th>NO_HDR</th> <th>INITIAL</th> <th>UPDATE</th> <th>NOTFOUND</th> <th>STALE</th> <th>DROPPED</th> </tr>
   *   <tr>
   *     <th rowspan="8">old state:</th>
   *     <th>NO_MSG</th> <td>NO_MSG</td> <td>WILDCARD</td> <td>NO_HDR</td> <td>INITIAL</td> <td>UPDATE</td> <td>NOTFOUND</td> <td>STALE</td> <td>DROPPED</td>
   *     <th rowspan="8">:new state</th>
   *   </tr>
   *   <tr> <th>WILDCARD</th> <td>WILDCARD</td> <td>WILDCARD</td> <td>WILDCARD</td> <td>WILDCARD</td> <td>WILDCARD</td> <td>WILDCARD</td> <td>WILDCARD</td> <td>WILDCARD</td> </tr>
   *   <tr> <th>NO_HDR</th>   <td>NO_HDR</td>   <td>WILDCARD</td> <td>NO_HDR</td>   <td>INITIAL</td>  <td>UPDATE</td>   <td>NOTFOUND</td> <td>STALE</td>    <td>DROPPED</td> </tr>
   *   <tr> <th>INITIAL</th>  <td>INITIAL</td>  <td>INITIAL</td>  <td>INITIAL</td>  <td>INITIAL</td>  <td>INITIAL</td>  <td>INITIAL</td>  <td>STALE</td>    <td>DROPPED</td> </tr>
   *   <tr> <th>UPDATE</th>   <td>UPDATE</td>   <td>UPDATE</td>   <td>NO_HDR</td>   <td>INITIAL</td>  <td>UPDATE</td>   <td>UPDATE</td>   <td>STALE</td>    <td>DROPPED</td> </tr>
   *   <tr> <th>NOTFOUND</th> <td>NOTFOUND</td> <td>NOTFOUND</td> <td>NO_HDR</td>   <td>INITIAL</td>  <td>UPDATE</td>   <td>NOTFOUND</td> <td>STALE</td>    <td>DROPPED</td> </tr>
   *   <tr> <th>STALE</th>    <td>STALE</td>    <td>STALE</td>    <td>STALE</td>    <td>INITIAL</td>  <td>UPDATE</td>   <td>NOTFOUND</td> <td>STALE</td>    <td>DROPPED</td> </tr>
   *   <tr> <th>DROPPED</th>  <td>DROPPED</td>  <td>DROPPED</td>  <td>DROPPED</td>  <td>INITIAL</td>  <td>DROPPED</td>  <td>DROPPED</td>  <td>DROPPED</td>  <td>DROPPED</td> </tr>
   * </table>
   * @see RaiSubscribe
   */
  public int state;
  /** If supported, it will be non-zero;  It is when msg sent over the
   * transport by publisher.  Timestamp is nanoseconds GMT since 1970,
   * equivalent to time_t time() * 10**9 + nanosecond offset.  This is set to a
   * new value for each hop. */
  public long pubTime;
  /** If supported, it be non-zero;  It is when msg update was created or
   * cached.  Timestamp is nanoseconds GMT since 1970, equivalent to time_t
   * time() * 10**9 + nanosecond offset.  This is cached and passes through
   * from the upstream feed.  For an initial value (RaiMsgEvent.type = SNAP),
   * this is when the message was last updated. */
  public long routeTime;
  /** The sequence number of subject update, it will be non-zero if supported.
   * This is a counter that changes when an update is applied to a cached
   * image.  This wraps around 32 bits integer. */
  public long counter;

  /** The subject that was used in subscribe.Start().
   * @see RaiSubscribe#Start */
  public String SubscribedSubject() {
    return this.subscribe.Subject();
  }
  /** Constructor used internally. */
  protected RaiMsgEvent( RaiSubscribe s,  String subj,  int t,  short mtype,
                         short rstatus,  int ostate,  int rstate,  int nstate,
                         long ptm,  long rtm,  long ctr ) {
    this.init( s, subj, t, mtype, rstatus, ostate, rstate, nstate, ptm, rtm,
               ctr );
  }
  /** Initializer used internally. */
  protected void init( RaiSubscribe s,  String subj,  int t,  short mtype,
                       short rstatus,  int ostate,  int rstate,  int nstate,
                       long ptm,  long rtm,  long ctr ) {
    this.subscribe = s;
    this.subject   = subj;
    this.type      = t;
    this.msgType   = mtype;
    this.recStatus = rstatus;
    this.oldState  = ostate;
    this.recv      = rstate;
    this.state     = nstate;
    this.pubTime   = ptm;
    this.routeTime = rtm;
    this.counter   = ctr;
  }
}
