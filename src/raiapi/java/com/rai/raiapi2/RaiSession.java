package com.rai.raiapi2;
import com.rai.raiexception.RaiException;

/** The RaiSession class maintains the structures associated with publishing
 * and subscribing. */
public class RaiSession {
  long   session,
         dataLossCb;
  RaiApi api;

  /** Internally used to initialize a session.
   * @see RaiApi#CreateSession */
  protected RaiSession( long s,  RaiApi a ) {
    this.session    = s;
    this.dataLossCb = 0;
    this.api        = a;
  }
  /** Internally used to delete a session. */
  protected void finalize() {
    if ( this.dataLossCb != 0 )
      DeleteCB( this.dataLossCb );
    Delete( this.session );
  } 
  /** Start the Session transports and event processing.
   * <p>This should be called after SetDataLossCallback() to initialize any
   * network or background event processing that may be needed by the session.
   * For some middleware apis it must be called, but it may do nothing in
   * others. */
  public native void Start() throws RaiException;

  private static native void DeleteCB( long dataLossCb );

  private static native void Delete( long session );

  /** Create a queue with direct = false, see below.
   * @return RaiQueue, an object for dispatching timers and messages */ 
  public RaiQueue CreateQueue() throws RaiException {
    return this.CreateQueue( false );
  }
  /** Create a queue for dispatching messages and timer events.
   * <p>Passing direct = true causes the receive thread to dispatch message
   * callbacks.
   * <p>Passing direct = false (default) causes the queue thread to dispatch
   * message callbacks.
   * <p>This feature is only available in protocols where this can be
   * controlled (capr, embd), otherwise direct has no effect.  Using direct =
   * true minimizes copying and context switches, but if callbacks are compute
   * heavy, then multiple queues with direct = false allows processing to be
   * offloaded on to queue threads.  When direct is false, the thread that is
   * dispatching events using the RaiQueue.Dispatch() methods (other variants
   * are TimedDispatch() and Mainloop()) is the same thread that will execute
   * the the callback. When direct is true, then the thread that received a
   * message is used to execute the callback, rather than the thread that is
   * dispatching events in the RaiQueue.
   * <p>A RaiQueue object provides dispatching methods to filter by subject
   * using RaiSubscribe and execute callbacks for messages that are received by
   * a RaiSession. It has a consumer relationship with RaiSession, which
   * produces messages for multiple queues and the threads which are
   * dispatching these queues.
   * @return RaiQueue, an object for dispatching timers and messages */ 
  public native RaiQueue CreateQueue( boolean direct ) throws RaiException;
  /** Create a publisher without auto increment of SEQ_NO field, see below. */
  public RaiPublish CreatePublish() throws RaiException {
    return this.CreatePublish( false );
  }
  /** Create a publisher.
   * <p>Passing autoInc = true causes the the publish() method to increment
   * the SEQ_NO field, if it exists.
   * <p>A publisher provides methods to send messages to the network using the
   * RaiSession transport. It can be reused to send multiple messages as long
   * as the RaiSession object is open.
   *
   * <p>Example:
   * <p><pre>
   *  import com.rai.raimsg.*;
   *  import com.rai.raiapi2.*;
   *  import com.rai.raiexception.RaiException;
   *
   *  public class test {
   *    public static void main( String [] args ) {
   *      try {
   *        RaiApi api = RaiApi.RaiOpen( "tibrv", args );
   *        api.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
   *        RaiSession session = api.CreateSession();
   *        RaiPublish pub = session.CreatePublish( false );
   *        RaiMsg msg = RaiApi.NewRaiMsg( RaiMsg.RV_PROTO, SassConst.INITIAL, 
   *                              (short) 0, (short) 0, (short) SassConst.STATUS_OK );
   *        msg.Append( "hello", "world" );
   *        pub.Publish( "TEST.SUBJECT", msg );
   *      } catch ( RaiException e ) {
   *        System.err.println( "Failed: " + e );
   *      }
   *    }
   *  }
   * </pre>
   * @return RaiPublish, an object for publishing messages */
  public native RaiPublish CreatePublish( boolean autoInc ) throws RaiException;
  /** Create a dictionary loader.
   * <p>Dictionaries are used to pack and unpack messages such as Qform
   * messages that use a data dictionary to compress a messages.
   * Self-describing messages do not need to use dictionaries since the field
   * name, type of data, and data value are included in each message.
   *
   * <p>Example:
   * <p><pre>
   *  import com.rai.raiapi2.*;
   *  import com.rai.raiexception.RaiException;
   *
   *  public static void main( String [] args ) {
   *    try {
   *      RaiApi api = RaiApi.RaiOpen( null, args );
   *      api.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
   *      RaiSession session = api.CreateSession();
   *      RaiDict dict = session.CreateDict();
   *      dict.Load( 3, null, false );
   *      while ( dict.InProgress() )
   *        ;
   *      if ( ! dict.HaveDict() )
   *        api.PrintLog( RaiApi.LVL_MINOR, "Dictionary load timed out" );
   *    } catch ( RaiException e ) {
   *      System.err.println( "Failed: " + e );
   *    }
   *  }
   * </pre>
   * @return RaiDict, an object for obtaining a dictionary from the network
   * @see RaiApi#OpenDict */
  public native RaiDict CreateDict()                   throws RaiException;
  /** Stop the session event processing, shutdown the transport.
   *  <p>No more messages will be received. If any RaiQueue or RaiSubscribe are
   *  still open, they will only dispatch callbacks for messages which are
   *  currently in memory. Messages sent with a RaiPublish after
   *  RaiSession.Destroy() will be discarded.  */
  public native void Destroy();
  /** Login to entitlements facility.
   * <p>The RaiEntitlements object can be used in any RaiApi and any RaiSession
   * so that entitlements do not need to be on the same network as the data.
   * The userid and appid set in the RaiApi object will be used to retrieve the
   * entitlements.
   * <p>This may not have any affect if entitlements are not checked locally
   * @return RaiEntitlement, an object that processes entitlements */
  public native RaiEntitlement Login( String user )    throws RaiException;
  /** Set callback for data loss errors, with null closure, see below. */
  public void SetDataLossCB( RaiDataLossCallback cb ) throws RaiException {
    this.SetDataLossCB( cb, null );
  }
  /** Set callback for data loss errors, with a closure.
   * <p>When a connection occurs, the
   * cb.onConnection( RaiConnectionEvent event, Object cl ) will be used to
   * notify the event.
   * <p>If message loss or network is disconnected, the callback
   * cb.onDataLoss( RaiDataLossEvent event, Object cl ) will be used to notify
   * the application. The application can then notify the subscriptions with
   * RaiSession.NotifyStatus() if desired. The RaiSession will also take action
   * to reconnect the session to the network whether or not there is a
   * RaiDataLossCallback defined.  Defining this callback function is optional.
   * <p>These events are asynchronous and may occur on threads which are not
   * dispatching a queue, but on a session event thread or a network event
   * thread.
   *
   * <p>Example:
   * <p><pre>
   *  import com.rai.raiapi2.*;
   *  import com.rai.raimsg.*;
   *  import com.rai.raiexception.RaiException;
   *
   *  public class test implements RaiDataLossCallback {
   *    RaiApi api;
   *    RaiSession session;
   *    RaiQueue queue;
   *
   *    public void onConnection( RaiConnectionEvent event,  Object closure ) {
   *      this.api.PrintLog( RaiApi.LVL_ERROR, "onConnection: " + event.description );
   *    }
   *
   *    public void onDataLoss( RaiDataLossEvent event,  Object closure ) {
   *      this.api.PrintLog( RaiApi.LVL_ERROR, "onDataLoss: " + event.description );
   *      if ( event.connectionLoss &amp;&amp; event.connectionCount == 0 ) {
   *        try {
   *          this.session.NotifyStatus( SassConst.TRANSIENT,
   *                                     SassConst.STATUS_TPT_DISCONNECTED );
   *        } catch ( RaiException e ) {
   *          this.api.PrintLog( RaiApi.LVL_ERROR, e, "NotifyStatus" );
   *        }
   *      }
   *    }
   *
   *    public static void main( String [] argv ) {
   *      try {
   *        Args args = new Args();
   *        test me = new test();
   *        me.api = RaiApi.RaiOpen( null, argv );
   *        me.api.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
   *        me.api.GetArgs( args );
   *        if ( args.processArgs( argv ) ) {
   *          me.api.ParseArgs( args );
   *          me.session = me.api.CreateSession();
   *          me.session.SetDataLossCB( me );
   *          me.queue = me.session.CreateQueue();
   *          me.queue.Mainloop();
   *        }
   *      } catch ( RaiException e ) {
   *        System.err.println( "Failed: " + e );
   *      }
   *    }
   *  }
   * </pre>
   * <p>Running this with tcp connection loss:
   * <p><pre>
   *  $ javac test.java
   *  $ java test -address tcp:localhost:4000
   *  2011-07-08 22:52:24.051 Error: Socket.40+Unable to connect socket, remote refused connection; Connecting to: tcp/127.0.0.1:4000
   *  2011-07-08 22:52:24.051 Error: RaiApi.14+Transport lost data or connection; Connection to address tcp/127.0.0.1:4000, reconnecting in 3.0 seconds
   *  2011-07-08 22:52:27.052 Error:  onDataLoss: Connection to address tcp/127.0.0.1:4000, reconnecting in 3.0 seconds
   *  2011-07-08 22:52:30.053 Error:  onDataLoss: Connection to address tcp/127.0.0.1:4000, reconnecting in 3.0 seconds
   *  2011-07-08 22:52:33.054 Error:  onDataLoss: Connection to address tcp/127.0.0.1:4000, reconnecting in 3.0 seconds
   * </pre>
   * <p>With multicast packet loss, the log would look like this at the
   * publisher:
   * <p><pre>
   * $ java raiping2 -address localhost -noSub -perSec 100000
   * 2011-07-08 23:02:35.090 Minor:  Publishing PING.TEST.REC.XXX
   * 2011-07-08 23:02:57.550 Error:  onDataLoss: Connection to multicast address pgmoudp/127.0.0.1;226.6.6.6:1;228.8.8.8:8866, transport 127.0.0.1:52491 &lt;-&gt; 228.8.8.8:8866, host at 127.0.0.1, outbound 4074 packets lost
   * </pre>
   * <p>The multicast packet loss at the subscriber:
   * <p><pre>
   * $ java raisub2 -address 'localhost;228.8.8.8;226.6.6.6' -subject PING.TEST.REC.XXX -rate -quiet -noDict
   * 0.0 sub/s 0.0 unsub/s 1739.5 msg/s 113.8 kb/s 0.93 mbit/s
   * 2011-07-08 23:02:57.482 Error:  onDataLoss: Connection to multicast address pgmoudp/127.0.0.1;228.8.8.8:45084;226.6.6.6:8866, transport 127.0.0.1:59552 &lt;-&gt; 226.6.6.6:8866, host at 127.0.0.1, inbound 123870 packets lost
   * </pre>
   *
   * @param cb Callback for notification of data loss and connection events.
   * @param closure The closure which is passed through to cb.onDataLoss() and
   * cb.onConnection().  This RaiSession object keeps a "weak global reference"
   * to the closure, so there must be another reference to keep it alive,
   * otherwise it will be equal to null after the GC reclaims it.
   */
  public native void SetDataLossCB( RaiDataLossCallback cb,  Object closure )
                                                       throws RaiException;
  /** Notify subscription state by sending a message to each subscription.
   * This is useful to notify RaiSubscribe callbacks that the value is stale,
   * or that the transport is disconnected.
   * @param msgType The MSG_TYPE in the RaiMsg status sent to open
   * subscriptions.
   * @param recStatus The REC_STATUS in the RaiMsg.
   * @see com.rai.raimsg.SassConst
   * @see RaiQueue#NotifyStatus
   * @see #SetDataLossCB */
  public native void NotifyStatus( short msgType,
                                   short recStatus )   throws RaiException;
  /** Get the api interface this session was created from. */
  public RaiApi GetApi() {
    return this.api;
  }
  /** Set the name of the session, used to identify it uniquely when multiple
   * sessions are active. */
  public native void SetSessionName( String sessionName ) throws RaiException;
  /** Get the name assigned to this session object.  If none are set, then
   * a unique ID is returned, currently this is the memory address of the
   * underlying session object. */
  public native String GetSessionName()                   throws RaiException;
}
