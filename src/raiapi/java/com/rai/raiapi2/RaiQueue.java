package com.rai.raiapi2;
import com.rai.raiexception.RaiException;

/** The object for serializing events and dispatching them.  All events which
 * can be dispatched on a queue are created here:<br>
 * RaiQueue.CreateSubscribe() for RaiMsgEvent<br>
 * RaiQueue.CreateTimer() for RaiTimerEvent<br>
 * RaiQueue.CreateInteractivePublish() for RaiSubscribeEvent<br>
 *
 * @see RaiSession#CreateQueue
 * @see RaiSubscribe
 * @see RaiTimer
 * @see RaiInteractivePublish
 */
public class RaiQueue {
  long       queue;
  RaiSession session;

  /** Constructor used internally.
   * @see RaiSession#CreateQueue */
  protected RaiQueue( long q,  RaiSession s ) {
    this.queue   = q;
    this.session = s;
  }
  /** Destructor used internally. */
  protected void finalize() {
    Delete( this.queue );
  } 
  private static native void Delete( long queue );

  /** Create a subscription with a null closure, see below. */
  public RaiSubscribe CreateSubscribe( RaiMsgCallback cb ) throws RaiException {
    return this.CreateSubscribe( cb, null );
  }
  /** Creates a RaiSubscribe object, which associates a queue and a callback
   * with subject interest for subscriptions and snapshots.  The RaiSubscribe
   * object can be used with one subject including subject wildcards.  Multiple
   * RaiSubscribe objects should be created for multiple subject interests.
   * The RaiQueue which created the RaiSubscribe object will also be used to
   * execute the message callbacks when events are dispatched with Dispatch(),
   * TimedDispatch(), or Mainloop().  If RaiQueue is created with direct = true,
   * then messages can be dispatched without calling Dispatch(), since it is
   * possible that the transport layer threads will call the RaiMsgCallback
   * directly instead of queueing the message for dispatching.
   *
   * <p>Example:
   * <p><pre>
   * import com.rai.raiapi2.*;
   * import com.rai.raimsg.*;
   * import com.rai.raiexception.RaiException;
   *
   * public class tests implements RaiMsgCallback {
   *   RaiApi api;
   *   RaiSession session;
   *   RaiQueue queue;
   *   RaiSubscribe sub;
   *
   *   public void onMsg( RaiMsgEvent event,  RaiMsg raiMsg,  Object closure ) {
   *     System.out.println( "subject: " + event.subject );
   *     System.out.println( "SubscribedSubject: " + event.SubscribedSubject() );
   *     System.out.println( "oldState: " +
   *                         RaiSubscribe.StateToString( event.oldState ) +
   *                         " (" + event.oldState + ")" );
   *     System.out.println( "state: " + RaiSubscribe.StateToString( event.state ) +
   *                         " (" + event.state + ")" );
   *     if ( event.counter != 0 )
   *       System.out.println( "counter: " + event.counter );
   *     if ( event.pubTime != 0 )
   *       System.out.println( "pubTime: " + Time.nsTimestamp( event.pubTime, 3 ) );
   *     if ( event.routeTime != 0 )
   *       System.out.println( "routeTime: " +
   *                           Time.nsTimestamp( event.routeTime, 3 ) );
   *
   *     System.out.println( "msg: " );
   *     try {
   *       RaiField field = new RaiField();
   *       if ( field.First( raiMsg ) ) {
   *         do {
   *           System.out.println(
   *             field.Name() + " [ " +
   *               RaiMsg.TypeStr( field.Type() ) + " " + field.Size() + " ] : " +
   *                 field.Get() );
   *         } while ( field.Next() );
   *       }
   *     } catch ( RaiMsgException e ) {
   *       this.api.PrintLog( RaiApi.LVL_ERROR, e, "Printing message" );
   *     }
   *     System.out.println( "----" );
   *   }
   *
   *   public static void main( String [] argv ) {
   *     try {
   *       Args args = new Args();
   *       tests me = new tests();
   *       me.api = RaiApi.RaiOpen( null, argv );
   *       me.api.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
   *       me.api.GetArgs( args );
   *       args.addDefaults( me.api.RaiVersion(), "rai_", System.err, "test" );
   *       if ( args.processArgs( argv ) ) {
   *         me.api.ParseArgs( args );
   *         RaiApi.OpenLog( args );
   *         me.session = me.api.CreateSession();
   *         me.queue = me.session.CreateQueue();
   *         me.sub = me.queue.CreateSubscribe( me );
   *         me.sub.Start( "TEST.SUBJ", RaiSubscribe.UPDATE, 0 );
   *         me.queue.Mainloop();
   *       }
   *     } catch ( RaiException e ) {
   *       System.err.println( "Failed: " + e );
   *     }
   *   }
   * }
   * </pre>
   *
   * <p>Compile and run:
   * <p><pre>
   * $ javac tests.java
   * $ java tests -address '192.168.1.0;224.2.2.2:7222'
   * subject: TEST.SUBJ
   * SubscribedSubject: TEST.SUBJ
   * oldState: NO_MSG (0)
   * state: INITIAL (3)
   * pubTime: 2011-07-19 17:18:20.305
   * msg: 
   * MSG_TYPE [ UINT 2 ] : 8
   * REC_TYPE [ STRING 3 ] : EQ
   * SEQ_NO [ UINT 2 ] : 1
   * REC_STATUS [ UINT 2 ] : -1
   * ASK [ STRING 5 ] : 11.0
   * BID [ STRING 5 ] : 10.5
   * ----
   * </pre>
   *
   * <p>The above output is the result of running "raipub2":
   * <p><pre>
   * $ java raipub2 -address '192.168.1.0;224.2.2.2:7222' -subject TEST.SUBJ
   * </pre>
   *
   * @param cb Callback for messages on subscription, this class implements an
   * onMsg() method which RaiQueue calls when it dispatches message events.
   * @param closure The closure which is passed through to cb.onMsg().  This
   * RaiSubscribe object keeps a "weak global reference" to the closure, so
   * there must be another reference to keep it alive, otherwise it will be
   * equal to null after the GC reclaims it.
   */
  public native RaiSubscribe CreateSubscribe( RaiMsgCallback cb,
                                              Object closure )
                                                          throws RaiException;
  /** Create a RaiTimer event with a null closure, see below. */
  public RaiTimer CreateTimer( RaiTimerCallback cb ) throws RaiException {
    return this.CreateTimer( cb, null );
  }
  /** Creates a RaiTimer object, which associates a queue and a callback with a
   * timer interval.  Each RaiTimer object can be used for one interval. Create
   * multiple RaiTimer objects for multiple timer intervals.  A RaiTimer
   * schedules a callback to be called repeatedly at an interval when
   * RaiTimer.Start() is called and continues until RaiTimer.Stop() is called.
   * The RaiQueue which created the RaiTimer will execute the callbacks when
   * events are dispatched.
   *
   * <p>Example:
   * <p><pre>
   * import com.rai.raiapi2.*;
   * import com.rai.raiexception.RaiException;

   * public class testt implements RaiTimerCallback {
   *   RaiApi api;
   *   RaiSession session;
   *   RaiQueue queue;
   *   RaiTimer timer;
   *   int count;
   *
   *   public void onTimer( RaiTimer t,  Object cl ) {
   *     System.out.println( "Timer: " + System.currentTimeMillis() );
   *     if ( ++this.count == 3 )
   *       this.queue.Destroy();
   *   }
   *
   *   public static void main( String [] argv ) {
   *     try {
   *       Args args = new Args();
   *       testt me = new testt();
   *       me.api = RaiApi.RaiOpen( null, argv );
   *       me.api.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
   *       me.api.GetArgs( args );
   *       args.addDefaults( me.api.RaiVersion(), "rai_", System.err, "test" );
   *       if ( args.processArgs( argv ) ) {
   *         me.api.ParseArgs( args );
   *         RaiApi.OpenLog( args );
   *         me.session = me.api.CreateSession();
   *         me.queue = me.session.CreateQueue();
   *         me.timer = me.queue.CreateTimer( me );
   *         me.timer.SetInterval( 1000 );
   *         me.timer.Start();
   *         me.queue.Mainloop();
   *       }
   *     } catch ( RaiException e ) {
   *       System.err.println( "Failed: " + e );
   *     }
   *   }
   * }
   * </pre>
   *
   * <p>Compile and run:
   * <p><pre>
   * $ javac testt.java
   * $ java testt -api tibrv
   * Timer: 1310979565603
   * Timer: 1310979566603
   * Timer: 1310979567603
   * </pre>
   *
   * @param cb Callback for the timer, this class implements an onTimer()
   * method which RaiQueue calls when it dispatches timer events.
   * @param closure The closure which is passed through to cb.onTimer().
   * This RaiTimer object keeps a "weak global reference" to the closure, so
   * there must be another reference to keep it alive, otherwise it will be
   * equal to null after the GC reclaims it.
   */
  public native RaiTimer CreateTimer( RaiTimerCallback cb, Object closure )
                                                          throws RaiException;
  /** An interactive publisher with a null closure, see below. */
  public RaiInteractivePublish CreateInteractivePublish( RaiSubscribeCallback cb ) throws RaiException {
    return this.CreateInteractivePublish( cb, null );
  }
  /** An interactive publisher.  This is a mechanism to be notified that
   * a subscription to a subject and to publish to the subscriptions.  It
   * inherits the publishing methods from RaiPublish.  This allows the
   * application to publish messages when there is subscription interest and
   * to filter out messages which are not wanted by downstream applications.
   *
   * <p>Example:
   * <p><pre>
   * import com.rai.raiapi2.*;
   * import com.rai.raimsg.*;
   * import com.rai.raiexception.RaiException;
   *
   * public class testi implements RaiSubscribeCallback {
   *   RaiApi api;
   *   RaiSession session;
   *   RaiQueue queue;
   *   RaiInteractivePublish ipub;
   *
   *   public void onSubscribe( RaiSubscribeEvent event,  RaiMsg m,  Object cl ) {
   *     try {
   *       System.out.println( "subject: " + event.subject );
   *       System.out.println( "reply: " + event.reply );
   *       System.out.println( "queryFlags: " + event.queryFlags );
   *       m.Print( System.out );
   *       System.out.flush();
   *       if ( ( event.queryFlags &amp; ( SassConst.SNAPSHOT_FLAG |
   *                                   SassConst.REFRESH_FLAG |
   *                                  SassConst.INITIAL_VALUES_FLAG ) ) != 0 ) {
   *         RaiMsg msg = this.api.NewRaiMsg( RaiMsg.RV_PROTO,
   *             SassConst.INITIAL, (short) 0, (short) 0, SassConst.STATUS_OK );
   *         msg.Append( "hello", "world" );
   *         this.ipub.Publish(
   *           event.reply != null ? event.reply : event.subject, msg );
   *       }
   *     } catch ( RaiException e ) {
   *       api.PrintLog( RaiApi.LVL_ERROR, e, "onSubscribe" );
   *     }
   *   }
   *
   *   public static void main( String [] argv ) {
   *     try {
   *       Args args = new Args();
   *       testi me = new testi();
   *       me.api = RaiApi.RaiOpen( null, argv );
   *       me.api.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
   *       me.api.GetArgs( args );
   *       args.addDefaults( me.api.RaiVersion(), "rai_", System.err, "test" );
   *       if ( args.processArgs( argv ) ) {
   *         me.api.ParseArgs( args );
   *         RaiApi.OpenLog( args );
   *         me.session = me.api.CreateSession();
   *         me.queue = me.session.CreateQueue();
   *         me.ipub = me.queue.CreateInteractivePublish( me );
   *         me.ipub.InteractiveStart( "TEST.&gt;" );
   *         me.queue.Mainloop();
   *       }
   *     } catch ( RaiException e ) {
   *       System.err.println( "Failed: " + e );
   *     }
   *   }
   * }
   * </pre>
   *
   * <p>Compile and run:
   * <p><pre>
   * $ javac testi.java
   * $ java testi -api tibrv -network '192.168.1.0;224.2.2.2' -service 7222
   * </pre>
   *
   * <p>After a subscription event:
   * <pre>
   * $ java raisub2 -api tibrv -network '192.168.1.0;224.2.2.2' -service 7222 -subject TEST.SUBJECT -noDict
   * Starting subject TEST.SUBJECT
   * 1 subjects read on cmdline
   * ## Subject TEST.SUBJECT (old state=NO_MSG,new=INITIAL) (_INBOX.C0A801FE.1D704E23F153A0E8578.1)
   * MSG_TYPE       : UINT      2 : 8  [INITIAL]
   * REC_TYPE       : UINT      2 : 0
   * SEQ_NO         : UINT      2 : 0
   * REC_STATUS     : UINT      2 : 0  [OK]
   * hello          : STRING    6 : "world"
   * ^C2011-07-18 01:42:08.159 Minor:  Done dispatchLoop
   * 2011-07-18 01:42:08.159 Minor:  Caught signal 2, quitting
   * 2011-07-18 01:42:08.163 Minor:  Finished
   * </pre>
   *
   * <p>This output should appear from "java testi" when raisub2 starts:
   * <p><pre>
   * subject: TEST.SUBJECT
   * reply: _INBOX.C0A801FE.1D704E23F153A0E8578.1
   * queryFlags: 16390
   * flags          : UINT      2 : 16390
   * subject: TEST.SUBJECT
   * reply: null
   * queryFlags: 2
   * ADV_CLASS      : STRING    5 : "INFO"
   * ADV_SOURCE     : STRING    7 : "SYSTEM"
   * ADV_NAME       : STRING   26 : "LISTEN.START.TEST.SUBJECT"
   * id             : STRING   43 : "C0A801FE.DAEMON.46E54E23F1360X7FBCFC081210"
   * sub            : STRING   13 : "TEST.SUBJECT"
   * refcnt         : INT       4 : 1
   * </pre>
   *
   * <p>This output should appear when raisub2 stops:
   * <p><pre>
   * subject: TEST.SUBJECT
   * reply: null
   * queryFlags: 8
   * ADV_CLASS      : STRING    5 : "INFO"
   * ADV_SOURCE     : STRING    7 : "SYSTEM"
   * ADV_NAME       : STRING   25 : "LISTEN.STOP.TEST.SUBJECT"
   * id             : STRING   43 : "C0A801FE.DAEMON.46E54E23F1360X7FBCFC081210"
   * sub            : STRING   13 : "TEST.SUBJECT"
   * refcnt         : INT       4 : 0
   * </pre>
   *
   * @param cb The interface for subscription notifications, onSubscribe().
   * @param closure The closure passed to onSubscribe().
   * @see RaiPublish */
  public native RaiInteractivePublish CreateInteractivePublish(
                                        RaiSubscribeCallback cb,
                                        Object closure ) throws RaiException;
  /** Notify subscription state by sending a message to each subscription on
   * this queue.  This is useful when a network event causes all subscriptions
   * to become in a stale state.
   * @param msgType The MSG_TYPE in the RaiMsg status sent to open
   * subscriptions.
   * @param recStatus The REC_STATUS in the RaiMsg.
   * @see com.rai.raimsg.SassConst
   * @see RaiSession#SetDataLossCB
   * @see RaiSession#NotifyStatus */
  public native void NotifyStatus( short msgType,  short recStatus )
                                                         throws RaiException;
  /** Dispatch messages off of the queue without returning.  This continually
   * dispatches events until the session or queue is destroyed, or until an
   * exception occurs.
   * @see RaiQueue#Dispatch */
  public native void Mainloop()                          throws RaiException;
  /** Return after dispatching one event.  Timeout (return without dispatching)
   * if no events to dispatch after ivalMSecs milliseconds. 
   * @see RaiQueue#Dispatch */
  public native void TimedDispatch( int ivalMSecs )      throws RaiException;
  /** Dispatch an event.  This call will block until at least one event is
   * dispatched, then return.  An event is any callback mechanism created
   * from this queue:  onTimer(), onMsg(), onSubscribe().  If the queue
   * was created with direct = true, then onMsg() events can bypass the
   * queue dispatch and would not be sent through the queue, otherwise the
   * thread which calls Dispatch(), TimedDispatch(), or Mainloop() is the
   * same thread which will call onTimer(), onMsg(), or onSubscribe().
   * If multiple threads call this queue's Dispatch() functions, then 
   * only one thread will execute the callbacks at a time, there isn't
   * any parallelism gained by this, since queues are inherently serialized,
   * instead use multiple queues with for each queue. */
  public native void Dispatch()                          throws RaiException;
  /** Return number of items in queue waiting to be dispatched.  Canceling
   * subscriptions or stopping timers may also cancel these events.  Since
   * there are threads producting events while GetDepth() is called, there may
   * be more items added to the queue after the function returns but before the
   * result is examined. */
  public native int GetDepth();
  /** Stop the event processing.  If there are threads processing callbacks at
   * the time Destroy() is called, this doesn't wait for them to complete, it
   * signals that no more events should be processed afterwards. */
  public native void Destroy();
  /** Get the queue's session.  This is a constant for the queue. */
  public RaiSession GetSession() {
    return this.session;
  }
};
