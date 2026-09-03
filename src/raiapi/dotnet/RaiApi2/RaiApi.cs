/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 * Mirrors com.rai.raiapi2: RaiApi, RaiSession, RaiQueue, RaiSubscribe,
 * RaiPublish, RaiInteractivePublish, RaiTimer, RaiDict, RaiEntitlement,
 * the event classes and the callback interfaces. */
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Com.Rai.Interop;
using Com.Rai.Raimsg;
using Com.Rai.Raiexception;

namespace Com.Rai.Raiapi2 {

/* ---- callback interfaces ------------------------------------------------ */
public interface RaiMsgCallback {
  void onMsg( RaiMsgEvent ev,  RaiMsg msg,  object closure );
}
public interface RaiTimerCallback {
  void onTimer( RaiTimer timer,  object closure );
}
public interface RaiSubscribeCallback {
  void onSubscribe( RaiSubscribeEvent ev,  RaiMsg message,  object closure );
}
public interface RaiDataLossCallback {
  void onDataLoss( RaiDataLossEvent ev,  object closure );
  void onConnection( RaiConnectionEvent ev,  object closure );
}

/* ---- events ------------------------------------------------------------- */
public class RaiMsgEvent {
  public const int SNAP   = 0;
  public const int UPDATE = 1;
  public RaiSubscribe subscribe;
  public string       subject;   /* received subject, may be an _INBOX */
  public int          type;      /* SNAP or UPDATE */
  public short        msgType;
  public short        recStatus;
  public int          oldState;  /* RaiSubscribe.STATE_* */
  public int          recv;
  public int          state;
  public long         pubTime;   /* ns, 0 if none */
  public long         routeTime;
  public long         counter;
  public string SubscribedSubject() { return this.subscribe.Subject(); }
}
public class RaiSubscribeEvent {
  public RaiInteractivePublish publish;
  public string subject;
  public string reply;
  public int    queryFlags;
}
public class RaiConnectionEvent {
  public RaiSession session;
  public string     transportName;
  public string     description;
  public long       connectionCount;
  public bool       connectionOriented;
  public bool       isMulticast;
}
public class RaiDataLossEvent {
  public RaiSession session;
  public string     transportName;
  public string     description;
  public long       inboundPacketLoss;
  public long       outboundPacketLoss;
  public long       connectionCount;
  public bool       connectionLoss;
  public bool       isMulticast;
}

/* keeps a native callback registration + the managed delegate alive */
internal sealed class CallbackHandle {
  internal IntPtr    cb;        /* rai_callback_t */
  internal object    keepAlive; /* the marshalled delegate(s) */
  internal GCHandle  self;
  internal object    target;    /* the managed callback object */
  internal object    closure;
  internal CallbackHandle( object t,  object cl ) {
    this.target = t; this.closure = cl; this.self = GCHandle.Alloc( this );
  }
  internal IntPtr Ptr { get { return GCHandle.ToIntPtr( this.self ); } }
  internal static CallbackHandle From( IntPtr p ) {
    return (CallbackHandle) GCHandle.FromIntPtr( p ).Target;
  }
  internal void Release() {
    if ( this.cb != IntPtr.Zero ) { Native.rai_callback_delete( this.cb ); this.cb = IntPtr.Zero; }
    if ( this.self.IsAllocated ) this.self.Free();
  }
}

/** RaiApi is a handle to an underlying transport implementation, as well as
 * logging and dictionary management, and some convenience functions.
 * Call RaiApi.RaiOpen() to begin. */
public class RaiApi : IDisposable {
  internal IntPtr api;

  public const string raiapi_api_arg    = "api";
  public const string raiapi_userid_arg = "userid";
  public const string raiapi_appid_arg  = "appid";
  public const string cfile_path_arg    = "cfilePath";
  public const string tss_records_arg   = "tssRecords";
  public const string tss_fields_arg    = "tssFields";
  public const string appendix_a_arg    = "appendixA";
  public const string enumtype_def_arg  = "enumtypeDef";

  public const int LVL_DEVEL  = 0;
  public const int LVL_FTRACE = 1;
  public const int LVL_TRACE  = 2;
  public const int LVL_DEBUG  = 3;
  public const int LVL_MINOR  = 4;
  public const int LVL_NORMAL = 5;
  public const int LVL_ERROR  = 6;

  public const int SIGHUP  = 1;
  public const int SIGINT  = 2;
  public const int SIGTERM = 15;

  protected RaiApi( IntPtr a ) { this.api = a; }
  ~RaiApi() { this.Dispose( false ); }
  public void Dispose() { this.Dispose( true ); GC.SuppressFinalize( this ); }
  void Dispose( bool d ) {
    if ( this.api != IntPtr.Zero ) { Native.rai_api_delete( this.api ); this.api = IntPtr.Zero; }
  }

  /** Open the api.  apiName selects the transport module ("tibrv", ... or
   * null for the default / -api argument).  argv is the program's argument
   * array (without the program name, as Main() receives it). */
  public static RaiApi RaiOpen( string apiName,  string[] argv ) {
    string[] av = Argv( argv );
    IntPtr a;
    RaiException.Check( Native.rai_api_open( apiName, av.Length, av, out a ) );
    return new RaiApi( a );
  }
  internal static string[] Argv( string[] argv ) {
    string[] av = new string[ ( argv == null ? 0 : argv.Length ) + 1 ];
    av[ 0 ] = "dotnet";
    if ( argv != null ) Array.Copy( argv, 0, av, 1, argv.Length );
    return av;
  }

  public string GetApiName() { return Native.Str( Native.rai_api_name( this.api ) ); }
  public static string RaiVersion() { return Native.Str( Native.rai_api_version() ); }
  public void GetArgs( Args args ) { RaiException.Check( Native.rai_api_get_args( this.api, args.args ) ); }
  public static void GetDictArgs( Args args ) { RaiException.Check( Native.rai_api_get_dict_args( args.args ) ); }
  public void ParseArgs( Args args ) { RaiException.Check( Native.rai_api_parse_args( this.api, args.args ) ); }
  /** Open the log from -log/-logLevel/-logVerb args, false if not present */
  public static bool OpenLog( Args args ) {
    int o; RaiException.Check( Native.rai_api_open_log_args( args.args, out o ) ); return o != 0;
  }
  /** Open the log file, "-" is stderr */
  public static void OpenLog( string name,  int logLevel,  int logVerb ) {
    RaiException.Check( Native.rai_api_open_log( name, logLevel, logVerb ) );
  }
  public void PrintLog( string s ) { this.PrintLog( LVL_MINOR, null, s ); }
  public void PrintLog( int level,  string s ) { this.PrintLog( level, null, s ); }
  public void PrintLog( int level,  Exception err,  string s ) {
    IntPtr e = ErrOf( err, ref s );
    Native.rai_api_print_log( this.api, level, e, null, 0, s );
  }
  public static void Log( string s ) { Log( LVL_MINOR, null, s ); }
  public static void Log( int level,  string s ) { Log( level, null, s ); }
  public static void Log( int level,  Exception err,  string s ) {
    IntPtr e = ErrOf( err, ref s );
    Native.rai_api_log( level, e, null, 0, s );
  }
  /* an api error logs its record, any other exception is prefixed as text */
  static IntPtr ErrOf( Exception err,  ref string s ) {
    if ( err == null ) return IntPtr.Zero;
    RaiException re = err as RaiException;
    if ( re != null && re.NativeErr != IntPtr.Zero ) return re.NativeErr;
    s = err.ToString() + "; " + s;
    return IntPtr.Zero;
  }
  /** Load the dictionary from the local filesystem if -cfilePath etc are set */
  public static bool OpenDict( Args args ) {
    int l; RaiException.Check( Native.rai_api_open_dict( args.args, out l ) ); return l != 0;
  }
  public RaiSession CreateSession() {
    IntPtr s; RaiException.Check( Native.rai_api_create_session( this.api, out s ) );
    return new RaiSession( s, this );
  }
  public static RaiMsg NewSASSMsg( short MsgType,  short RecType,  short SeqNo,  short RecStatus ) {
    return NewRaiMsg( RaiMsg.TIB_SASS_PROTO, MsgType, RecType, SeqNo, RecStatus );
  }
  public static RaiMsg NewSASSMsg( short MsgType,  string FormType,  short SeqNo,  short RecStatus ) {
    return NewRaiMsg( RaiMsg.TIB_SASS_PROTO, MsgType, FormType, SeqNo, RecStatus );
  }
  public static RaiMsg NewRaiMsg( short MsgType,  short RecType,  short SeqNo,  short RecStatus ) {
    return NewRaiMsg( RaiMsg.RAIMSG_PROTO, MsgType, RecType, SeqNo, RecStatus );
  }
  public static RaiMsg NewRaiMsg( short MsgType,  string FormType,  short SeqNo,  short RecStatus ) {
    return NewRaiMsg( RaiMsg.RAIMSG_PROTO, MsgType, FormType, SeqNo, RecStatus );
  }
  public static RaiMsg NewRaiMsg( int proto,  short MsgType,  short RecType,  short SeqNo,  short RecStatus ) {
    IntPtr m;
    RaiException.Check( Native.rai_api_new_msg( proto, (ushort) MsgType, (ushort) RecType, (ushort) SeqNo, (ushort) RecStatus, out m ) );
    return new RaiMsg( m, true );
  }
  public static RaiMsg NewRaiMsg( int proto,  short MsgType,  string FormType,  short SeqNo,  short RecStatus ) {
    IntPtr m;
    RaiException.Check( Native.rai_api_new_msg_form( proto, (ushort) MsgType, FormType, (ushort) SeqNo, (ushort) RecStatus, out m ) );
    return new RaiMsg( m, true );
  }
  /** Close the api; close any open sessions first */
  public void Close() { Native.rai_api_close( this.api ); }

  /** Trap SIGINT, SIGHUP, SIGTERM and call handler( sig ).  The handler runs
   * in signal context: set a flag, do not lock. */
  static Native.SignalFn sigFn;
  static Action<int>     sigHandler;
  public static void RegisterSigHandler( Action<int> handler ) {
    sigHandler = handler;
    if ( sigFn == null ) {
      sigFn = ( sig, cl ) => { Action<int> h = sigHandler; if ( h != null ) h( sig ); };
      RaiException.Check( Native.rai_api_register_sig_handler( sigFn, IntPtr.Zero ) );
    }
  }
  public bool SetIoctl( string parameter,  string value ) {
    return Native.rai_api_set_ioctl( this.api, parameter, value ) != 0;
  }
}

/** A session is a connection to the network / service */
public class RaiSession {
  internal IntPtr session;
  readonly RaiApi api;
  CallbackHandle  dataLossCb;

  internal RaiSession( IntPtr s,  RaiApi a ) { this.session = s; this.api = a; }

  public void Start() { RaiException.Check( Native.rai_session_start( this.session ) ); }
  public RaiQueue CreateQueue() { return this.CreateQueue( false ); }
  public RaiQueue CreateQueue( bool direct ) {
    IntPtr q; RaiException.Check( Native.rai_session_create_queue( this.session, direct ? 1 : 0, out q ) );
    return new RaiQueue( q, this );
  }
  public RaiPublish CreatePublish() { return this.CreatePublish( false ); }
  public RaiPublish CreatePublish( bool autoInc ) {
    IntPtr p; RaiException.Check( Native.rai_session_create_publish( this.session, autoInc ? 1 : 0, out p ) );
    return new RaiPublish( p, this );
  }
  public RaiDict CreateDict() {
    IntPtr d; RaiException.Check( Native.rai_session_create_dict( this.session, out d ) );
    return new RaiDict( d, this );
  }
  public void Destroy() {
    RaiException.Check( Native.rai_session_destroy( this.session ) );
    if ( this.dataLossCb != null ) { this.dataLossCb.Release(); this.dataLossCb = null; }
  }
  public RaiEntitlement Login( string user ) {
    IntPtr e; RaiException.Check( Native.rai_session_login( this.session, user, out e ) );
    return new RaiEntitlement( e );
  }
  public void SetDataLossCB( RaiDataLossCallback cb ) { this.SetDataLossCB( cb, null ); }
  public void SetDataLossCB( RaiDataLossCallback cb,  object closure ) {
    CallbackHandle h = new CallbackHandle( cb, closure );
    Native.DataLossFn lf = ( cl, evp ) => {
      CallbackHandle hh = CallbackHandle.From( cl );
      Native.DataLossEvent ne = Marshal.PtrToStructure<Native.DataLossEvent>( evp );
      RaiDataLossEvent ev = new RaiDataLossEvent();
      ev.session = this; ev.transportName = Native.Str( ne.transport_name );
      ev.description = Native.Str( ne.description );
      ev.inboundPacketLoss = ne.inbound_packet_loss; ev.outboundPacketLoss = ne.outbound_packet_loss;
      ev.connectionCount = ne.connection_count; ev.connectionLoss = ne.connection_loss != 0;
      ev.isMulticast = ne.is_multicast != 0;
      ( (RaiDataLossCallback) hh.target ).onDataLoss( ev, hh.closure );
    };
    Native.ConnectionFn cf = ( cl, evp ) => {
      CallbackHandle hh = CallbackHandle.From( cl );
      Native.ConnectionEvent ne = Marshal.PtrToStructure<Native.ConnectionEvent>( evp );
      RaiConnectionEvent ev = new RaiConnectionEvent();
      ev.session = this; ev.transportName = Native.Str( ne.transport_name );
      ev.description = Native.Str( ne.description ); ev.connectionCount = ne.connection_count;
      ev.connectionOriented = ne.connection_oriented != 0; ev.isMulticast = ne.is_multicast != 0;
      ( (RaiDataLossCallback) hh.target ).onConnection( ev, hh.closure );
    };
    h.keepAlive = new Delegate[] { lf, cf };
    IntPtr ncb;
    RaiException.Check( Native.rai_session_set_dataloss_cb( this.session, lf, cf, h.Ptr, out ncb ) );
    h.cb = ncb;
    if ( this.dataLossCb != null ) this.dataLossCb.Release();
    this.dataLossCb = h;
  }
  public void NotifyStatus( short msgType,  short recStatus ) {
    RaiException.Check( Native.rai_session_notify_status( this.session, (ushort) msgType, (ushort) recStatus ) );
  }
  public RaiApi GetApi() { return this.api; }
  public void SetSessionName( string name ) { RaiException.Check( Native.rai_session_set_name( this.session, name ) ); }
  public string GetSessionName() { return Native.Str( Native.rai_session_get_name( this.session ) ); }
}

/** A queue serializes message and timer events for dispatch */
public class RaiQueue {
  internal IntPtr queue;
  readonly RaiSession session;
  /* live objects created on this queue, so callbacks map handles back */
  internal readonly Dictionary<IntPtr, RaiSubscribe> subs = new Dictionary<IntPtr, RaiSubscribe>();
  internal readonly Dictionary<IntPtr, RaiTimer> timers = new Dictionary<IntPtr, RaiTimer>();
  internal readonly Dictionary<IntPtr, RaiInteractivePublish> ipubs = new Dictionary<IntPtr, RaiInteractivePublish>();

  internal RaiQueue( IntPtr q,  RaiSession s ) { this.queue = q; this.session = s; }

  public RaiSubscribe CreateSubscribe( RaiMsgCallback cb ) { return this.CreateSubscribe( cb, null ); }
  public RaiSubscribe CreateSubscribe( RaiMsgCallback cb,  object closure ) {
    CallbackHandle h = new CallbackHandle( cb, closure );
    RaiSubscribe sub = null;
    Native.MsgFn fn = ( cl, evp, mp ) => {
      CallbackHandle hh = CallbackHandle.From( cl );
      Native.MsgEvent ne = Marshal.PtrToStructure<Native.MsgEvent>( evp );
      RaiMsgEvent ev = new RaiMsgEvent();
      ev.subscribe = sub; ev.subject = Native.Str( ne.subject ); ev.type = ne.type;
      ev.msgType = (short) ne.msg_type; ev.recStatus = (short) ne.rec_status;
      ev.oldState = ne.old_state; ev.recv = ne.recv; ev.state = ne.state;
      ev.pubTime = ne.pub_time; ev.routeTime = ne.route_time; ev.counter = ne.counter;
      RaiMsg m = new RaiMsg( mp, false ); /* owned by the api, valid in callback */
      try { ( (RaiMsgCallback) hh.target ).onMsg( ev, m, hh.closure ); }
      finally { m.Dispose(); }
    };
    h.keepAlive = fn;
    IntPtr s, ncb;
    RaiException.Check( Native.rai_queue_create_subscribe( this.queue, fn, h.Ptr, out s, out ncb ) );
    h.cb = ncb;
    sub = new RaiSubscribe( s, h, this );
    lock ( this.subs ) this.subs[ s ] = sub;
    return sub;
  }
  public RaiTimer CreateTimer( RaiTimerCallback cb ) { return this.CreateTimer( cb, null ); }
  public RaiTimer CreateTimer( RaiTimerCallback cb,  object closure ) {
    CallbackHandle h = new CallbackHandle( cb, closure );
    RaiTimer timer = null;
    Native.TimerFn fn = ( cl, tp ) => {
      CallbackHandle hh = CallbackHandle.From( cl );
      ( (RaiTimerCallback) hh.target ).onTimer( timer, hh.closure );
    };
    h.keepAlive = fn;
    IntPtr t, ncb;
    RaiException.Check( Native.rai_queue_create_timer( this.queue, fn, h.Ptr, out t, out ncb ) );
    h.cb = ncb;
    timer = new RaiTimer( t, h, this );
    lock ( this.timers ) this.timers[ t ] = timer;
    return timer;
  }
  public RaiInteractivePublish CreateInteractivePublish( RaiSubscribeCallback cb ) { return this.CreateInteractivePublish( cb, null ); }
  public RaiInteractivePublish CreateInteractivePublish( RaiSubscribeCallback cb,  object closure ) {
    CallbackHandle h = new CallbackHandle( cb, closure );
    RaiInteractivePublish ip = null;
    Native.SubscribeFn fn = ( cl, evp, mp ) => {
      CallbackHandle hh = CallbackHandle.From( cl );
      Native.SubscribeEvent ne = Marshal.PtrToStructure<Native.SubscribeEvent>( evp );
      RaiSubscribeEvent ev = new RaiSubscribeEvent();
      ev.publish = ip; ev.subject = Native.Str( ne.subject ); ev.reply = Native.Str( ne.reply );
      ev.queryFlags = (int) ne.query_flags;
      RaiMsg m = new RaiMsg( mp, false );
      try { ( (RaiSubscribeCallback) hh.target ).onSubscribe( ev, m, hh.closure ); }
      finally { m.Dispose(); }
    };
    h.keepAlive = fn;
    IntPtr p, ncb;
    RaiException.Check( Native.rai_queue_create_ipublish( this.queue, fn, h.Ptr, out p, out ncb ) );
    h.cb = ncb;
    ip = new RaiInteractivePublish( p, h, this );
    lock ( this.ipubs ) this.ipubs[ p ] = ip;
    return ip;
  }
  public void NotifyStatus( short msgType,  short recStatus ) {
    RaiException.Check( Native.rai_queue_notify_status( this.queue, (ushort) msgType, (ushort) recStatus ) );
  }
  /** Dispatch events until the queue is destroyed */
  public void Mainloop() { RaiException.Check( Native.rai_queue_mainloop( this.queue ) ); }
  /** Dispatch events for up to ivalMSecs */
  public void TimedDispatch( int ivalMSecs ) { RaiException.Check( Native.rai_queue_timed_dispatch( this.queue, (uint) ivalMSecs ) ); }
  /** Dispatch pending events, return immediately */
  public void Dispatch() { RaiException.Check( Native.rai_queue_dispatch( this.queue ) ); }
  public int GetDepth() { return (int) Native.rai_queue_get_depth( this.queue ); }
  public void Destroy() {
    RaiException.Check( Native.rai_queue_destroy( this.queue ) );
    lock ( this.subs ) { foreach ( RaiSubscribe s in this.subs.Values ) s.ReleaseCB(); this.subs.Clear(); }
    lock ( this.timers ) { foreach ( RaiTimer t in this.timers.Values ) t.ReleaseCB(); this.timers.Clear(); }
    lock ( this.ipubs ) { foreach ( RaiInteractivePublish p in this.ipubs.Values ) p.ReleaseCB(); this.ipubs.Clear(); }
  }
  public RaiSession GetSession() { return this.session; }
}

/** A subscription to a subject, created on a queue */
public class RaiSubscribe {
  public const int UPDATE    = 1;
  public const int SNAP      = 2;
  public const int BOTH      = 3;
  public const int NO_PREFIX = 4;
  public const int NO_COPY   = 8;

  public const int STATE_NO_MSG   = 0;
  public const int STATE_WILDCARD = 1;
  public const int STATE_NO_HDR   = 2;
  public const int STATE_INITIAL  = 3;
  public const int STATE_UPDATE   = 4;
  public const int STATE_NOTFOUND = 5;
  public const int STATE_STALE    = 6;
  public const int STATE_DROPPED  = 7;

  internal IntPtr subscribe;
  CallbackHandle  cb;
  readonly RaiQueue queue;
  public int state;

  internal RaiSubscribe( IntPtr s,  CallbackHandle h,  RaiQueue q ) { this.subscribe = s; this.cb = h; this.queue = q; }
  internal void ReleaseCB() { if ( this.cb != null ) { this.cb.Release(); this.cb = null; } }

  public static string StateToString( int state ) { return Native.Str( Native.rai_subscribe_state_to_string( state ) ); }
  public string GetStateString() { return StateToString( this.state ); }
  public void Start( string subject,  int parm ) { this.Start( subject, parm, 0 ); }
  /** Start the subscription, parm is UPDATE / SNAP / BOTH | NO_PREFIX | NO_COPY,
   * timeoutMSecs > 0 generates a STATUS_TIMEOUT if no message arrives */
  public void Start( string subject,  int parm,  int timeoutMSecs ) {
    RaiException.Check( Native.rai_subscribe_start( this.subscribe, subject, parm, (uint) timeoutMSecs ) );
  }
  /** Cancel the subscription; the object is released, do not use it after */
  public void Cancel() {
    RaiException.Check( Native.rai_subscribe_cancel( this.subscribe ) );
    lock ( this.queue.subs ) this.queue.subs.Remove( this.subscribe );
    this.ReleaseCB();
  }
  public void Refresh( int timeoutMSecs ) { RaiException.Check( Native.rai_subscribe_refresh( this.subscribe, (uint) timeoutMSecs ) ); }
  public string Subject() { return Native.Str( Native.rai_subscribe_subject( this.subscribe ) ); }
  public bool InProgress() { return Native.rai_subscribe_in_progress( this.subscribe ) != 0; }
  public RaiQueue GetQueue() { return this.queue; }
}

/** Publisher of messages to subjects */
public class RaiPublish {
  internal IntPtr publish;
  readonly RaiSession session;

  internal RaiPublish( IntPtr p,  RaiSession s ) { this.publish = p; this.session = s; }

  public void Publish( string subject,  RaiMsg raiMsg ) { this.Publish( subject, raiMsg, 0 ); }
  /** Publish a message, stamp is a ns timestamp or 0 */
  public void Publish( string subject,  RaiMsg raiMsg,  long stamp ) {
    RaiException.Check( Native.rai_publish_msg( this.publish, subject, raiMsg.msg, stamp ) );
  }
  public void Publish( string subject,  byte[] buffer,  int offset,  int length ) { this.Publish( subject, buffer, offset, length, 0 ); }
  /** Publish a packed message buffer */
  public void Publish( string subject,  byte[] buffer,  int offset,  int length,  long stamp ) {
    GCHandle h = GCHandle.Alloc( buffer, GCHandleType.Pinned );
    try {
      RaiException.Check( Native.rai_publish_buf( this.publish, subject, IntPtr.Add( h.AddrOfPinnedObject(), offset ), (uint) length, stamp ) );
    } finally { h.Free(); }
  }
  public void SetPrefix( string prefix ) { RaiException.Check( Native.rai_publish_set_prefix( this.publish, prefix ) ); }
  public string GetPrefix() { return Native.Str( Native.rai_publish_get_prefix( this.publish ) ); }
  public long GetSeqno() { return Native.rai_publish_get_seqno( this.publish ); }
  public void SetSeqno( long newSeqno ) { Native.rai_publish_set_seqno( this.publish, (uint) newSeqno ); }
  public virtual void Destroy() { RaiException.Check( Native.rai_publish_destroy( this.publish ) ); }
  public RaiSession GetSession() { return this.session; }
}

/** A publisher that is told when subscriptions start / stop (interactive) */
public class RaiInteractivePublish : RaiPublish {
  internal IntPtr interactive;
  CallbackHandle  cb;
  readonly RaiQueue queue;

  internal RaiInteractivePublish( IntPtr i,  CallbackHandle h,  RaiQueue q )
    : base( Native.rai_ipublish_publish( i ), q.GetSession() ) {
    this.interactive = i; this.cb = h; this.queue = q;
  }
  internal void ReleaseCB() { if ( this.cb != null ) { this.cb.Release(); this.cb = null; } }
  public void InteractiveStart( string subject ) { RaiException.Check( Native.rai_ipublish_start( this.interactive, subject ) ); }
  public void InteractiveCancel() {
    RaiException.Check( Native.rai_ipublish_cancel( this.interactive ) );
    lock ( this.queue.ipubs ) this.queue.ipubs.Remove( this.interactive );
    this.ReleaseCB();
  }
  public bool InProgress() { return Native.rai_ipublish_in_progress( this.interactive ) != 0; }
  public RaiQueue GetQueue() { return this.queue; }
}

/** A timer dispatched on a queue */
public class RaiTimer {
  internal IntPtr timer;
  CallbackHandle  cb;
  readonly RaiQueue queue;

  internal RaiTimer( IntPtr t,  CallbackHandle h,  RaiQueue q ) { this.timer = t; this.cb = h; this.queue = q; }
  internal void ReleaseCB() { if ( this.cb != null ) { this.cb.Release(); this.cb = null; } }

  public void Start() { RaiException.Check( Native.rai_timer_start( this.timer ) ); }
  public void Stop() { Native.rai_timer_stop( this.timer ); }
  public long GetInterval() { return Native.rai_timer_get_interval( this.timer ); }
  public void SetInterval( long intervalMSecs ) { Native.rai_timer_set_interval( this.timer, intervalMSecs ); }
  public RaiQueue GetQueue() { return this.queue; }
}

/** Dictionary loader */
public class RaiDict {
  internal IntPtr dict;
  readonly RaiSession session;
  internal RaiDict( IntPtr d,  RaiSession s ) { this.dict = d; this.session = s; }
  /** Load the dictionary from the network, loadWait blocks until done */
  public void Load( int timeoutSecs,  string dictSubject,  bool loadWait ) {
    RaiException.Check( Native.rai_dict_load( this.dict, (uint) timeoutSecs, dictSubject, loadWait ? 1 : 0 ) );
  }
  public bool HaveDict() { return Native.rai_dict_have_dict( this.dict ) != 0; }
  public bool InProgress() { return Native.rai_dict_in_progress( this.dict ) != 0; }
  public RaiSession GetSession() { return this.session; }
}

public class RaiEntitlement {
  internal IntPtr entitle;
  internal RaiEntitlement( IntPtr e ) { this.entitle = e; }
  public void Destroy() { if ( this.entitle != IntPtr.Zero ) { Native.rai_entitle_delete( this.entitle ); this.entitle = IntPtr.Zero; } }
}

} /* namespace */
