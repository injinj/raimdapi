/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 *
 * raiapi2_nats.cpp -- the RaiApi over the natsapi (natsmd/natsapi.h), the
 * way raiapi2_tibrv.cpp sits on the tibrv 7 api and raiapi2_omm.cpp on
 * ommapi.  Selected with -api nats.
 *
 * NATS carries the sassrv subject schema, so the message flow is the tibrv
 * one: the dictionary is requested on _TIC.REPLY.SASS.DATA.DICTIONARY with
 * an inbox reply, a snapshot / initial image is requested on _SNAP.<subject>
 * (-snapPrefix) with an inbox reply and the sass flags, updates arrive on
 * the subject itself (the publisher prefixes _TIC. or not; -prefix on the
 * publish side).  A payload is whatever the publisher wrote -- a raw TIBMSG /
 * TIB_SASS / RVMSG, or an RVMSG { _data_ : <msg> } the way an RV daemon
 * encapsulates it -- RaiMsg::ExtractProtocolEx() sorts that out.  Publish()
 * sends the RaiMsg's packed bytes as they are, with its codec id.
 *
 * Not in v1: interactive publish (NATS has no _RV.INFO.SYSTEM.LISTEN
 * notifications), raises UNSUPPORTED_MTHD. */

#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_api__raiapi2_nats_cpp[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */

#include <natsmd/natsapi.h>
#include "raiapi2.h"
#include "base/log.h"
#include "base/rai_factory.h"
#include "base/thread.h"
#include "util/args.h"
#include "msg/sass_const.h"
#include "msg/mfeed_dict.h"
#include "stream/byte_array_stream.h"
#include "util/str_util.h"

using namespace rai;

extern "C" RAI_DLL_EXPORT RaiApi *RaiApi_RaiOpen_nats( int argc,  char *argv[] );

struct RaiQueue_nats;
struct RaiSubscribe_nats;

struct RaiApi_nats : public RaiApi {
  char * daemon,     /* host[:port] */
       * appName,    /* CONNECT name */
       * user,       /* CONNECT user / pass / auth_token */
       * pass,
       * token,
       * snapPrefix, /* _SNAP. */
       * queueGroup; /* NATS queue group for listeners, NULL = none */

  SYS_OPS( RaiApi_nats );
  RaiApi_nats() : daemon( 0 ), appName( 0 ), user( 0 ), pass( 0 ),
    token( 0 ), snapPrefix( 0 ), queueGroup( 0 ) {}
  virtual ~RaiApi_nats();

  virtual const char *GetApiName( void ) { return "nats"; }
  virtual void GetArgs( Args &args );
  virtual void ParseArgs( Args &args );
  virtual RaiSession *CreateSession( void );
  virtual void Close( void );
  virtual bool SetIoctl( const char *parameter,  const void *value );
  virtual bool GetIoctl( const char *parameter,  void *value );

  static RaiException getNatsErr( nats_status status );
};

struct RaiSession_nats : public RaiSession, public Thread {
  RaiApi_nats         & api;
  natsTransport         tport;
  natsQueue             xQ;        /* transport state events, sub timers,
                                    * the dictionary reply */
  natsEvent             stateEv;
  Mutex               * lock;
  RaiQueue_nats       * qHd, * qTl;
  char                * transportName;
  RaiDataLossCallback * dataLossCb;
  void                * dataLossCl;
  bool                  quit;

  SYS_OPS( RaiSession_nats );
  RaiSession_nats( RaiApi_nats *a ) : Thread( "natsqueue" ),
    api( *a ), tport( 0 ), xQ( 0 ), stateEv( NATS_INVALID_ID ), lock( 0 ),
    qHd( 0 ), qTl( 0 ), transportName( 0 ), dataLossCb( 0 ), dataLossCl( 0 ),
    quit( false ) {}
  virtual ~RaiSession_nats();

  /* RaiSession */
  virtual void Start( void );
  virtual RaiQueue *CreateQueue( bool direct );
  virtual RaiPublish *CreatePublish( bool autoInc );
  virtual RaiDict *CreateDict( void );
  virtual void Destroy( void );
  virtual RaiEntitlement *Login( const char *user );
  virtual void SetDataLossCB( RaiDataLossCallback *cb,  void *closure );
  virtual void NotifyStatus( Rai_u16 msgType,  Rai_u16 recStatus );
  virtual RaiApi *GetApi( void ) { return &this->api; }

  /* Thread */
  virtual void run( void );

  static void state_onMsg( natsEvent ev,  natsMsg msg,  void *cl );
  static void quit_onTimer( natsEvent ev,  natsMsg msg,  void *cl );

  void addQueueList( RaiQueue_nats *q );
  void rmQueueList( RaiQueue_nats *q );
  /* a listener on subject, in the api's queue group when one is set */
  nats_status createListener( natsEvent *ev,  natsQueue q,
                              const char *subject,  natsEventCallback cb,
                              const void *cl );
};

struct RaiQueue_nats : public RaiQueue {
  RaiSession_nats    & session;
  natsQueue            natsQ;
  RaiQueue_nats      * next, * back;
  RaiSubscribe_nats  * subHd, * subTl;
  Mutex              * apiLock;
  unsigned int         matchClock,
                       subClock;
  bool                 inList,
                       isDestroyed;

  SYS_OPS( RaiQueue_nats );
  RaiQueue_nats( RaiSession_nats &s,  natsQueue q )
    : session( s ), natsQ( q ), next( 0 ), back( 0 ), subHd( 0 ), subTl( 0 ),
      apiLock( 0 ), matchClock( 0 ), subClock( 0 ), inList( false ),
      isDestroyed( false ) {}
  virtual ~RaiQueue_nats();

  /* RaiQueue */
  virtual RaiTimer *CreateTimer( RaiTimerCallback *cb,  void *closure );
  virtual RaiSubscribe *CreateSubscribe( RaiMsgCallback *cb,  void *closure );
  virtual RaiInteractivePublish *CreateInteractivePublish(
                                      RaiSubscribeCallback *cb,
                                      void *closure );
  virtual void NotifyStatus( Rai_u16 msgType,  Rai_u16 recStatus );
  struct AppEvent {
    natsEvent        ev;
    RaiAppCallback * cb;
    void           * eventData;
    Rai_i32          eventEnum;

    SYS_OPS( AppEvent );
    AppEvent( RaiAppCallback *c,  void *ed,  Rai_i32 en )
      : ev( NATS_INVALID_ID ), cb( c ), eventData( ed ), eventEnum( en ) {}
  };
  virtual void QueueEvent( RaiAppCallback *cb,  void *eventData,
                           Rai_i32 eventEnum,  Rai_u8 eventPriority,
                           Rai_u32 expireMSecs );
  RaiException sendStatusMsg( RaiMsg &msg, RaiSubscribe::RaiSubState recvState,
                              Rai_u16 msgType,  Rai_u16 recStatus );
  virtual void Mainloop( void );
  virtual void TimedDispatch( Rai_u32 ivalMSecs );
  virtual void Dispatch( void );
  virtual Rai_u32 GetDepth( void );
  virtual RaiSession *GetSession( void ) { return &this->session; }
  virtual void Destroy( void );

  void addSubList( RaiSubscribe_nats *s );
  void rmSubList( RaiSubscribe_nats *s );

  static void onAppTimer( natsEvent ev,  natsMsg msg,  void *cl );
};

struct RaiDict_nats : public RaiDict {
  RaiSession_nats & session;
  natsEvent         inboxEv;   /* listener on the reply inbox */
  TimeMSecs         startTime;
  Rai_u32           timeoutSecs;
  bool              complete;

  SYS_OPS( RaiDict_nats );
  RaiDict_nats( RaiSession_nats &s ) : session( s ),
    inboxEv( NATS_INVALID_ID ), startTime( 0 ), timeoutSecs( 0 ),
    complete( false ) {}
  virtual ~RaiDict_nats();

  /* RaiDict: request on dictSubject with an inbox reply, like tibrv */
  virtual void Load( Rai_u32 timeoutSecs,  const char *dictSubject,
                     bool loadWait );
  virtual bool HaveDict( void ) { return this->complete; }
  virtual bool InProgress( void );
  virtual RaiSession *GetSession( void ) { return &this->session; }

  static void onMsg( natsEvent ev,  natsMsg msg,  void *cl );
};

struct RaiTimer_nats : public RaiTimer {
  RaiQueue_nats    & q;
  RaiTimerCallback & cb;
  TimeMSecs          ival;
  void             * cl;
  natsEvent          ev;

  SYS_OPS( RaiTimer_nats );
  RaiTimer_nats( RaiQueue_nats &qu,  RaiTimerCallback *c,  void *closure )
    : q( qu ), cb( *c ), ival( 0 ), cl( closure ), ev( NATS_INVALID_ID ) {}
  virtual ~RaiTimer_nats();

  virtual void Start( void );
  virtual void Stop( void );
  virtual TimeMSecs GetInterval( void );
  virtual void SetInterval( TimeMSecs interval );
  virtual RaiQueue *GetQueue( void ) { return &this->q; }

  static void onTimer( natsEvent ev,  natsMsg msg,  void *cl );
};

struct RaiSubscribe_nats : public RaiSubscribe {
  RaiQueue_nats      & q;
  RaiMsgCallback     & cb;
  void               * cl;
  char               * subject;
  const char         * reply;
  RaiSubscribe_nats  * next, * back;
  natsEvent            snapId,    /* listener on the snapshot reply inbox */
                       listenId;  /* listener on the subject */
  RaiMsg             * extra;     /* request payload, SetExtra() */
  unsigned int         match,
                       count;
  byte                 subType;
  bool                 inList,
                       hasTimer,
                       noPrefix;  /* NO_PREFIX: subject already has _SNAP. */

  SYS_OPS( RaiSubscribe_nats );
  RaiSubscribe_nats( RaiQueue_nats &qu,  RaiMsgCallback *c,  void *closure )
    : q( qu ), cb( *c ), cl( closure ), subject( 0 ), reply( 0 ),
      next( 0 ), back( 0 ), snapId( NATS_INVALID_ID ),
      listenId( NATS_INVALID_ID ), extra( 0 ), match( 0 ), count( 0 ),
      subType( 0xff ), inList( false ), hasTimer( false ),
      noPrefix( false ) {}
  virtual ~RaiSubscribe_nats();

  virtual void Start( const char *subject,  RaiSubParameter parm,
                      Rai_u32 timeoutMSecs );
  void sendSubRequest( void );
  void startTimer( Rai_u32 timeoutMSecs );
  virtual void Cancel( void );
  virtual void Refresh( Rai_u32 timeoutMSecs );
  virtual const char *Subject( void );
  virtual bool InProgress( void );
  virtual RaiQueue *GetQueue( void ) { return &this->q; }
  /* Set extra parameters passed with the snapshot request, null erases */
  virtual void SetExtra( RaiMsg *ex );

  static void onMsg( natsEvent ev,  natsMsg msg,  void *cl );
};

struct RaiSubscribeTimer_nats {
  RaiSession_nats   & session;
  RaiQueue_nats     & q;
  RaiSubscribe_nats & sub;
  unsigned int        count;

  SYS_OPS( RaiSubscribeTimer_nats );
  RaiSubscribeTimer_nats( RaiSubscribe_nats & s ) : session( s.q.session ),
     q( s.q ), sub( s ), count( s.count ) {}

  static void onTimer( natsEvent ev,  natsMsg msg,  void *cl );
};

struct RaiPublish_nats : public RaiPublish {
  RaiSession_nats & session;

  SYS_OPS( RaiPublish_nats );
  RaiPublish_nats( RaiSession_nats &s,  bool autoInc )
    : RaiPublish( autoInc ), session( s ) {}
  virtual ~RaiPublish_nats() {}

  virtual void Publish( const char *subject,  const void *buffer,
                        Rai_u32 size,  TimeNSecs stamp,  Rai_u32 msgTypeId );
  virtual void Destroy( void ) {}
  virtual RaiSession *GetSession( void ) { return &this->session; }
};

/* unpack a NATS payload: a raw TIBMSG / TIB_SASS / RVMSG, or an RVMSG that
 * encapsulates the message in a _data_ / _QFORM / _TIBMSG / _RAIMSG opaque
 * (what an RV daemon or the sassrv bridge wraps around it) */
static void
unpackNatsPayload( RaiMsg &raiMsg,  const void *bufp,  uint32_t msgSize )
{
  RaiMsg_protocol proto;
  RaiMsg_size     off = 0;

  if ( ! RaiMsg::ExtractProtocolEx( (RaiMsg_data) bufp, msgSize, proto, off ) )
    throw RaiMsgErr::getErr( RaiMsgErr::BAD_MAGIC_NUMBER );
  raiMsg.UnPack( proto, (RaiMsg_data) &((const byte *) bufp)[ off ],
                 msgSize - off, RAIMSG_MEMORY_STATIC );
  if ( proto == RV_PROTO ) {
    /* the opaque is the first of several fields: the tibrv transport's
     * check, ExtractProtocolEx() only unwraps a lone opaque */
    RaiField field;
    if ( field.First( &raiMsg ) && field.NameSize() > 4 &&
         field.Name()[ 0 ] == '_' ) {
      switch ( field.Name()[ 1 ] ) {
        case 'd': if ( ! field.isNamed( "_data_" ) ) return; break;
        case 'Q': if ( ! field.isNamed( "_QFORM" ) ) return; break;
        case 'T': if ( ! field.isNamed( "_TIBMSG" ) ) return; break;
        case 'R': if ( ! field.isNamed( "_RAIMSG" ) ) return; break;
        default: return;
      }
      raiMsg.UnPack( (RaiMsg_data) field.Data(), field.Size() );
    }
  }
}

/* ---- api ------------------------------------------------------------------ */

RAI_DLL_EXPORT RaiApi *
RaiApi_RaiOpen_nats( int /* argc */,  char * /* argv */[] )
{
  nats_status status = nats_Open();
  if ( status != NATS_OK ) {
    Error e = RaiApi_nats::getNatsErr( status );
    logError( LERROR, e, "nats_Open() failed" );
  }
  return NEW RaiApi_nats();
}

RaiApi_nats::~RaiApi_nats()
{
  if ( this->daemon != NULL )
    FREE( this->daemon );
  if ( this->appName != NULL )
    FREE( this->appName );
  if ( this->user != NULL )
    FREE( this->user );
  if ( this->pass != NULL )
    FREE( this->pass );
  if ( this->token != NULL )
    FREE( this->token );
  if ( this->snapPrefix != NULL )
    FREE( this->snapPrefix );
  if ( this->queueGroup != NULL )
    FREE( this->queueGroup );
}

static const char daemon_arg[]      = "daemon",
                  app_name_arg[]    = "appName",
                  user_arg[]        = "user",
                  pass_arg[]        = "pass",
                  token_arg[]       = "token",
                  snap_prefix_arg[] = "snapPrefix",
                  queue_group_arg[] = "queueGroup";

static const char * prefix_null_check( const char *value ) {
  if ( value != NULL &&
       ( value[ 0 ] == '\0' || ::strcmp( value, "\"\"" ) == 0  ||
         ::strcmp( value, "-" ) == 0 ) )
    value = NULL;
  return value;
}

void
RaiApi_nats::GetArgs( Args &args )
{
  static StringArg daemon( daemon_arg, NULL, "<host[:port]>",
                           "NATS server to connect to (127.0.0.1:4222)" );
  static StringArg appName( app_name_arg, NULL, "<name>",
                            "NATS CONNECT name" );
  static StringArg user( user_arg, NULL, "<name>",
                         "NATS CONNECT user (default: -userid)" );
  static StringArg pass( pass_arg, NULL, "<string>",
                         "NATS CONNECT password" );
  static StringArg token( token_arg, NULL, "<string>",
                          "NATS CONNECT auth token" );
  static StringArg snapPref( snap_prefix_arg, "_SNAP.", "<string>",
                             "Prefix to use for snapshot and initial values" );
  static StringArg queueGroup( queue_group_arg, NULL, "<name>",
                               "Subscribe as a member of this NATS queue "
                               "group" );

  this->RaiApi::GetArgs( args );
  args.add( &daemon, COMMAND_ARG | RESOURCE_ARG );
  args.add( &appName, COMMAND_ARG | RESOURCE_ARG );
  args.add( &user, COMMAND_ARG | RESOURCE_ARG );
  args.add( &pass, COMMAND_ARG | RESOURCE_ARG );
  args.add( &token, COMMAND_ARG | RESOURCE_ARG );
  args.add( &snapPref, COMMAND_ARG | RESOURCE_ARG );
  args.add( &queueGroup, COMMAND_ARG | RESOURCE_ARG );
}

void
RaiApi_nats::ParseArgs( Args &args )
{
  const char *s;

  this->RaiApi::ParseArgs( args );
  s = args.getString( daemon_arg );
  if ( s != NULL )
    STRDUP( this->daemon, s );
  s = args.getString( app_name_arg );
  if ( s != NULL )
    STRDUP( this->appName, s );
  s = args.getString( user_arg );
  if ( s != NULL )
    STRDUP( this->user, s );
  s = args.getString( pass_arg );
  if ( s != NULL )
    STRDUP( this->pass, s );
  s = args.getString( token_arg );
  if ( s != NULL )
    STRDUP( this->token, s );
  s = prefix_null_check( args.getString( snap_prefix_arg ) );
  if ( s != NULL )
    STRDUP( this->snapPrefix, s );
  s = args.getString( queue_group_arg );
  if ( s != NULL )
    STRDUP( this->queueGroup, s );
}

void
RaiApi_nats::Close( void )
{
  nats_Close();
}

bool
RaiApi_nats::SetIoctl( const char *parameter,  const void *value )
{
  if ( ::strcmp( parameter, daemon_arg ) == 0 )
    STRDUP( this->daemon, (const char *) value );
  else if ( ::strcmp( parameter, app_name_arg ) == 0 )
    STRDUP( this->appName, (const char *) value );
  else if ( ::strcmp( parameter, user_arg ) == 0 )
    STRDUP( this->user, (const char *) value );
  else if ( ::strcmp( parameter, pass_arg ) == 0 )
    STRDUP( this->pass, (const char *) value );
  else if ( ::strcmp( parameter, token_arg ) == 0 )
    STRDUP( this->token, (const char *) value );
  else if ( ::strcmp( parameter, snap_prefix_arg ) == 0 )
    STRDUP( this->snapPrefix, prefix_null_check( (const char *) value ) );
  else if ( ::strcmp( parameter, queue_group_arg ) == 0 )
    STRDUP( this->queueGroup, (const char *) value );
  else
    return this->RaiApi::SetIoctl( parameter, value );
  return true;
}

bool
RaiApi_nats::GetIoctl( const char *parameter,  void *value )
{
  if ( ::strcmp( parameter, daemon_arg ) == 0 )
    *(char **) value = this->daemon;
  else if ( ::strcmp( parameter, app_name_arg ) == 0 )
    *(char **) value = this->appName;
  else if ( ::strcmp( parameter, user_arg ) == 0 )
    *(char **) value = this->user;
  else if ( ::strcmp( parameter, pass_arg ) == 0 )
    *(char **) value = this->pass;
  else if ( ::strcmp( parameter, token_arg ) == 0 )
    *(char **) value = this->token;
  else if ( ::strcmp( parameter, snap_prefix_arg ) == 0 )
    *(char **) value = this->snapPrefix;
  else if ( ::strcmp( parameter, queue_group_arg ) == 0 )
    *(char **) value = this->queueGroup;
  else
    return this->RaiApi::GetIoctl( parameter, value );
  return true;
}

/* ---- session -------------------------------------------------------------- */

RaiSession *
RaiApi_nats::CreateSession( void )
{
  RaiSession_nats * session;
  nats_status       status;
  char              buf[ 1024 ];
  ByteArrayOutputStream bout( (byte *) buf, sizeof( buf ) - 1 );

  session = NEW RaiSession_nats( this );
  session->lock = Mutex::create();
  status = natsQueue_Create( &session->xQ );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );

  bout.printf( "nats=%s", this->daemon != NULL ? this->daemon
                                               : "127.0.0.1:4222" );
  buf[ bout.length() ] = '\0';
  STRDUP( session->transportName, buf );
  return session;
}

/* transport state events (connected / disconnected) on the session queue */
void
RaiSession_nats::state_onMsg( natsEvent,  natsMsg msg,  void *cl )
{
  RaiSession_nats * session = (RaiSession_nats *) cl;
  natsMsgClass      cls = NATS_MSG_DATA;
  const char      * text = "";

  natsMsg_GetMsgClass( msg, &cls );
  natsMsg_GetStatusText( msg, &text );
  if ( cls == NATS_MSG_TRANSPORT_UP ) {
    logDebug( LDEBUG, "nats transport up: %s", text );
    if ( session->dataLossCb != NULL ) {
      RaiConnectionEvent event( *session );
      event.transportName      = session->transportName;
      event.description        = text;
      event.connectionOriented = true;
      event.isMulticast        = false;
      event.connectionCount    = 1;
      session->dataLossCb->onConnection( event, session->dataLossCl );
    }
  }
  else if ( cls == NATS_MSG_TRANSPORT_DOWN ) {
    logError( LERROR, NULL, "nats transport down: %s", text );
    if ( session->dataLossCb != NULL ) {
      RaiDataLossEvent event( *session );
      event.transportName      = session->transportName;
      event.description        = text;
      event.inboundPacketLoss  = 0;
      event.outboundPacketLoss = 0;
      event.connectionLoss     = true;
      event.isMulticast        = false;
      event.connectionCount    = 0;
      session->dataLossCb->onDataLoss( event, session->dataLossCl );
    }
  }
}

void
RaiSession_nats::Start( void )
{
  natsTransportParams parms;
  nats_status         status;
  const char        * user = NULL, * appId = NULL;

  this->api.GetIoctl( raiapi_userid_arg, &user );
  this->api.GetIoctl( raiapi_appid_arg, &appId );

  nats_InitTransportParams( &parms );
  parms.daemon     = this->api.daemon;
  parms.name       = this->api.appName != NULL ? this->api.appName : appId;
  parms.user       = this->api.user != NULL ? this->api.user : user;
  parms.password   = this->api.pass;
  parms.auth_token = this->api.token;

  status = natsTransport_Create( &this->tport, &parms );
  if ( status != NATS_OK ) {
    RaiException e = RaiApi_nats::getNatsErr( status );
    logError( LERROR, e, "Error in natsTransport_Create()" );
    throw e;
  }
  status = natsTransport_SetStateListener( this->tport, this->xQ,
                                           RaiSession_nats::state_onMsg, this,
                                           &this->stateEv );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );

  this->Thread::start();

  /* like the rv transport, Start() returns connected; the api keeps
   * reconnecting if the server is not there yet */
  status = natsTransport_WaitConnected( this->tport, 10.0 );
  if ( status != NATS_OK ) {
    RaiException e = RaiApi_nats::getNatsErr( status );
    logError( LERROR, e, "Waiting for nats connect to %s",
              this->transportName );
  }
}

nats_status
RaiSession_nats::createListener( natsEvent *ev,  natsQueue q,
                                 const char *subject,  natsEventCallback cb,
                                 const void *cl )
{
  if ( this->api.queueGroup != NULL )
    return natsEvent_CreateQueueListener( ev, q, this->tport, subject,
                                          this->api.queueGroup,
                                          NATS_LISTEN_DEFAULT, cb, cl );
  return natsEvent_CreateListener( ev, q, this->tport, subject,
                                   NATS_LISTEN_DEFAULT, cb, cl );
}

void
RaiSession_nats::addQueueList( RaiQueue_nats *q )
{
  this->lock->lock();
  if ( ! q->inList ) {
    q->inList = true;
    if ( (q->back = this->qTl) != NULL )
      this->qTl->next = q;
    else
      this->qHd = q;
    this->qTl = q;
  }
  this->lock->unlock();
}

void
RaiSession_nats::rmQueueList( RaiQueue_nats *q )
{
  this->lock->lock();
  if ( q->inList ) {
    q->inList = false;
    if ( q->next != NULL )
      q->next->back = q->back;
    else
      this->qTl = q->back;
    if ( q->back != NULL )
      q->back->next = q->next;
    else
      this->qHd = q->next;
    q->next = q->back = NULL;
  }
  this->lock->unlock();
}

void
RaiSession_nats::quit_onTimer( natsEvent ev,  natsMsg,  void *cl )
{
  RaiSession_nats *me = (RaiSession_nats *) cl;
  natsEvent_Destroy( ev );
  me->quit = true;
}

void
RaiSession_nats::run( void )
{
  nats_status status;
  while ( ! this->quit ) {
    status = natsQueue_TimedDispatch( this->xQ, 1.0 );
    if ( status != NATS_OK && status != NATS_TIMEOUT ) {
      RaiException e = RaiApi_nats::getNatsErr( status );
      logError( LERROR, e, "RaiSession dispatch thread" );
      break;
    }
  }
  this->exit();
}

RaiSession_nats::~RaiSession_nats()
{
  if ( this->transportName != NULL )
    FREE( this->transportName );
  if ( this->lock != NULL )
    delete this->lock;
}

RaiQueue *
RaiSession_nats::CreateQueue( bool /* direct */ )
{
  RaiQueue_nats * q;
  natsQueue       nq;
  nats_status     status;

  status = natsQueue_Create( &nq );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );
  q = NEW RaiQueue_nats( *this, nq );
  q->apiLock = Mutex::create( Mutex::RECURSIVE_LOCK );
  this->addQueueList( q );
  return q;
}

RaiPublish *
RaiSession_nats::CreatePublish( bool autoInc )
{
  return NEW RaiPublish_nats( *this, autoInc );
}

RaiDict *
RaiSession_nats::CreateDict( void )
{
  return NEW RaiDict_nats( *this );
}

void
RaiSession_nats::Destroy( void )
{
  if ( ! this->quit && this->isThreadRunning() ) {
    natsEvent timerId;
    natsEvent_CreateTimer( &timerId, this->xQ, RaiSession_nats::quit_onTimer,
                           0.0, this );
  }
  if ( ! this->isThreadJoined() )
    this->join();
  if ( this->stateEv != NATS_INVALID_ID ) {
    natsEvent_Destroy( this->stateEv );
    this->stateEv = NATS_INVALID_ID;
  }
  if ( this->xQ != 0 ) {
    natsQueue_Destroy( this->xQ );
    this->xQ = 0;
  }
  if ( this->tport != 0 ) {
    natsTransport_Destroy( this->tport );
    this->tport = 0;
  }
}

RaiEntitlement *
RaiSession_nats::Login( const char * /* user */ )
{
  return NULL;
}

void
RaiSession_nats::SetDataLossCB( RaiDataLossCallback *cb,  void *closure )
{
  this->dataLossCl = closure;
  this->dataLossCb = cb;
}

/* ---- queue ---------------------------------------------------------------- */

void
RaiQueue_nats::addSubList( RaiSubscribe_nats *s )
{
  if ( ! s->inList ) {
    s->inList = true;
    if ( (s->back = this->subTl) != NULL )
      this->subTl->next = s;
    else
      this->subHd = s;
    this->subTl = s;
  }
}

void
RaiQueue_nats::rmSubList( RaiSubscribe_nats *s )
{
  if ( s->inList ) {
    s->inList = false;
    if ( s->next != NULL )
      s->next->back = s->back;
    else
      this->subTl = s->back;
    if ( s->back != NULL )
      s->back->next = s->next;
    else
      this->subHd = s->next;
    s->next = s->back = NULL;
  }
}

RaiTimer *
RaiQueue_nats::CreateTimer( RaiTimerCallback *cb,  void *closure )
{
  return NEW RaiTimer_nats( *this, cb, closure );
}

RaiSubscribe *
RaiQueue_nats::CreateSubscribe( RaiMsgCallback *cb,  void *closure )
{
  return NEW RaiSubscribe_nats( *this, cb, closure );
}

/* no _RV.INFO.SYSTEM.LISTEN.START/STOP equivalent on NATS */
RaiInteractivePublish *
RaiQueue_nats::CreateInteractivePublish( RaiSubscribeCallback *,  void * )
{
  throw RaiApiErr::getErr( RaiApiErr::UNSUPPORTED_MTHD );
}

void
RaiQueue_nats::Destroy( void )
{
  this->apiLock->lock();
  this->isDestroyed = true;
  this->apiLock->unlock();
  if ( this->natsQ != 0 ) {
    natsQueue_Destroy( this->natsQ );
    this->natsQ = 0;
  }
  this->session.rmQueueList( this );
}

RaiQueue_nats::~RaiQueue_nats()
{
  if ( this->natsQ != 0 )
    this->Destroy();
  if ( this->apiLock != NULL )
    delete this->apiLock;
}

void
RaiQueue_nats::Mainloop( void )
{
  nats_status status;
  for (;;) {
    status = natsQueue_Dispatch( this->natsQ );
    if ( this->isDestroyed )
      return;
    if ( status != NATS_OK )
      throw RaiApi_nats::getNatsErr( status );
  }
}

void
RaiQueue_nats::TimedDispatch( Rai_u32 ivalMSecs )
{
  nats_status status;
  status = natsQueue_TimedDispatch( this->natsQ, (double) ivalMSecs / 1000.0 );
  if ( this->isDestroyed )
    return;
  if ( status != NATS_OK && status != NATS_TIMEOUT )
    throw RaiApi_nats::getNatsErr( status );
}

void
RaiQueue_nats::Dispatch( void )
{
  nats_status status;
  status = natsQueue_Dispatch( this->natsQ );
  if ( this->isDestroyed )
    return;
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );
}

Rai_u32
RaiQueue_nats::GetDepth( void )
{
  uint32_t    count;
  nats_status status = natsQueue_GetCount( this->natsQ, &count );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );
  return count;
}

/* a status message the way the tibrv transport builds it: sass header
 * fields from the dictionary when one is loaded */
static inline void
create_status_msg( RaiMsg &msg,  RaiSubscribe::RaiSubState &recvState,
                   Rai_u16 msgType,  Rai_u16 recStatus )
{
  recvState = RaiSubscribe::SassToSubState( msgType, recStatus );

  if ( DataDictionary != NULL &&
       DataDictionary->msgType != NULL &&
       DataDictionary->recStatus != NULL ) {
    msg.SetProtocol( TIB_SASS_PROTO );
    msg.Append( DataDictionary->msgType, msgType );
    msg.Append( DataDictionary->recStatus, recStatus );
  }
  else {
    msg.Append( "MSG_TYPE", msgType );
    msg.Append( "REC_STATUS", recStatus );
  }
}

void
RaiQueue_nats::NotifyStatus( Rai_u16 msgType,  Rai_u16 recStatus )
{
  RaiMsg msg;
  Error  e2;
  RaiSubscribe::RaiSubState recvState;

  create_status_msg( msg, recvState, msgType, recStatus );
  e2 = this->sendStatusMsg( msg, recvState, msgType, recStatus );
  if ( e2 != NULL )
    throw e2;
}

void
RaiQueue_nats::QueueEvent( RaiAppCallback *cb,  void *eventData,
                           Rai_i32 eventEnum,  Rai_u8 /* eventPriority */,
                           Rai_u32 expireMSecs )
{
  nats_status status;
  AppEvent  * p = NEW AppEvent( cb, eventData, eventEnum );
  double      ival = ( expireMSecs != 0 ? (double) expireMSecs / 1000.0 : 0 );

  status = natsEvent_CreateTimer( &p->ev, this->natsQ,
                                  RaiQueue_nats::onAppTimer, ival, p );
  if ( status != NATS_OK ) {
    delete p;
    throw RaiApi_nats::getNatsErr( status );
  }
}

void
RaiQueue_nats::onAppTimer( natsEvent,  natsMsg,  void *cl )
{
  AppEvent * p = (AppEvent *) cl;
  try {
    p->cb->onAppEvent( p->eventData, p->eventEnum );
  } catch ( Error e ) {
    logError( LERROR, e, "onAppEvent()" );
  }
  natsEvent_Destroy( p->ev );
  delete p;
}

void
RaiSession_nats::NotifyStatus( Rai_u16 msgType,  Rai_u16 recStatus )
{
  RaiMsg msg;
  Error  e, e2;
  RaiSubscribe::RaiSubState recvState;
  RaiQueue_nats * q;

  create_status_msg( msg, recvState, msgType, recStatus );
  e2 = NULL;
  this->lock->lock();
  for ( q = this->qHd; q != NULL; q = q->next ) {
    e = q->sendStatusMsg( msg, recvState, msgType, recStatus );
    if ( e != NULL )
      e2 = e;
  }
  this->lock->unlock();
  if ( e2 != NULL )
    throw e2;
}

RaiException
RaiQueue_nats::sendStatusMsg( RaiMsg &msg, RaiSubscribe::RaiSubState recvState,
                              Rai_u16 msgType,  Rai_u16 recStatus )
{
  RaiSubscribe_nats * sub;
  unsigned int        subClock;
  RaiException        e2 = NULL;

  this->apiLock->lock();
  if ( ++this->matchClock == 0 ) {
    this->matchClock = 1;
    for ( sub = this->subHd; sub != NULL; sub = sub->next )
      sub->match = 0;
  }
  try {
    subClock = this->subClock; /* if list loses or gains a subject */
  rescan_list:;
    for ( sub = this->subHd; sub != NULL; sub = sub->next ) {
      if ( sub->match != this->matchClock ) {
        sub->match = this->matchClock;
        RaiSubscribe::RaiSubState oldState = sub->state;
        sub->state = RaiSubscribe::NewState( oldState, recvState );
        RaiMsgEvent ev( *sub, sub->subject, RaiMsgEvent::UPDATE, msgType,
                       recStatus, oldState, recvState, sub->state, 0, 0, 0, 0 );
        sub->cb.onMsg( ev, msg, sub->cl );
        if ( this->subClock != subClock ) {
          subClock = this->subClock;
          goto rescan_list;
        }
      }
    }
  } catch ( RaiException e ) {
    logError( LERROR, e, "Notify %s", sub->subject );
    e2 = e;
  }
  this->apiLock->unlock();
  return e2;
}

/* ---- dict ----------------------------------------------------------------- */

RaiDict_nats::~RaiDict_nats()
{
  if ( this->inboxEv != NATS_INVALID_ID )
    natsEvent_Destroy( this->inboxEv );
}

/* the dictionary reply on the inbox: a packed sass dictionary (a TIBMSG),
 * raw or inside an RVMSG _data_ opaque, the same as the tibrv transport */
void
RaiDict_nats::onMsg( natsEvent,  natsMsg msg,  void *cl )
{
  RaiDict_nats  & me = *(RaiDict_nats *) cl;
  RaiMsg          raiMsg;
  const void    * bufp = NULL;
  RaiMsg_config * dict,
                * dict2;
  RaiMfeed_dict * mdict,
                * mdict2;
  uint32_t        msgSize = 0;
  natsMsgClass    cls = NATS_MSG_DATA;
  nats_status     status;

  natsMsg_GetMsgClass( msg, &cls );
  if ( cls != NATS_MSG_DATA )
    return;
  status = natsMsg_GetData( msg, &bufp, &msgSize );
  if ( status != NATS_OK ) {
    RaiException e = RaiApi_nats::getNatsErr( status );
    logError( LERROR, e, "Getting dictionary message bytes" );
    return;
  }
  /* see if we can unpack the dictionary */
  dict2  = RaiMsg::GetDataDictionary();
  mdict2 = RaiMfeed_dict::GetMfeedDictionary();
  try {
    unpackNatsPayload( raiMsg, bufp, msgSize );
    dict = RaiMsg_config::unpackDataDictionary( raiMsg );
    RaiMsg::SetDataDictionary( dict );
    if ( (mdict = RaiMfeed_dict::unpackDataDictionary2( raiMsg )) != NULL ) {
      RaiMfeed_dict::SetMfeedDictionary( mdict );
      mdict->indexSass();
    }
    else {
      RaiMfeed_dict::SetMfeedDictionary( NULL );
    }
  } catch ( Error e ) {
    logError( LERROR, e, "Unpack data dictionary failed" );
    return;
  }
  logDebug( LDEBUG, "dictionary received" );

  if ( dict2 != NULL )
    RaiMsg_config::release( dict2 );
  if ( mdict2 != NULL )
    RaiMfeed_dict::release( mdict2 );

  natsEvent_Destroy( me.inboxEv );
  me.inboxEv  = NATS_INVALID_ID;
  me.complete = true;
}

/* request: publish an RVMSG { flags : 1 } on dictSubject with the reply set
 * to an inbox of this transport, listen on the inbox for the reply */
void
RaiDict_nats::Load( Rai_u32 timeoutSecs,  const char *dictSubject,
                    bool loadWait )
{
  nats_status status;
  char        dictInBox[ NATS_MAX_INBOX_LEN ];
  RaiMsg      m( RV_PROTO );

  this->timeoutSecs = timeoutSecs;
  this->complete    = false;
  if ( dictSubject == NULL )
    dictSubject = "_TIC.REPLY.SASS.DATA.DICTIONARY";
  logDebug( LDEBUG, "Requesting dictionary over NATS, subject %s",
            dictSubject );

  if ( this->inboxEv != NATS_INVALID_ID ) {
    natsEvent_Destroy( this->inboxEv );
    this->inboxEv = NATS_INVALID_ID;
  }
  status = natsTransport_CreateInbox( this->session.tport, dictInBox,
                                      sizeof( dictInBox ) );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );
  status = natsEvent_CreateListener( &this->inboxEv, this->session.xQ,
                                     this->session.tport, dictInBox,
                                     NATS_LISTEN_DEFAULT, RaiDict_nats::onMsg,
                                     this );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );

  m.Append( "flags", (Rai_u32) 1 );
  status = natsTransport_Send( this->session.tport, dictSubject, dictInBox,
                               m.Packed(), m.PackSize(), RVMSG_TYPE_ID );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );

  this->startTime = Time::currentTimeMillisecs();

  if ( loadWait ) {
    while ( this->InProgress() )
      ;
    if ( ! this->HaveDict() )
      throw RaiApiErr::getErr( RaiApiErr::BAD_DICT );
  }
}

bool
RaiDict_nats::InProgress( void )
{
  if ( this->complete )
    return false;
  Time::sleepMillisecs( 10 );
  return this->startTime + (TimeMSecs) ( this->timeoutSecs * 1000 ) >
         Time::currentTimeMillisecs();
}

/* ---- timer ---------------------------------------------------------------- */

RaiTimer_nats::~RaiTimer_nats()
{
  if ( this->ev != NATS_INVALID_ID )
    natsEvent_Destroy( this->ev );
}

void
RaiTimer_nats::onTimer( natsEvent,  natsMsg,  void *cl )
{
  RaiTimer_nats & me = *(RaiTimer_nats *) cl;
  try {
    me.cb.onTimer( me, me.cl );
  } catch ( RaiException e ) {
    logError( LERROR, e, "Timer callback" );
  }
}

void
RaiTimer_nats::Start( void )
{
  nats_status status;
  if ( this->ev != NATS_INVALID_ID ) {
    natsEvent_Destroy( this->ev );
    this->ev = NATS_INVALID_ID;
  }
  status = natsEvent_CreateTimer( &this->ev, this->q.natsQ,
                                  RaiTimer_nats::onTimer,
                                  (double) this->ival / 1000.0, this );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );
}

void
RaiTimer_nats::Stop( void )
{
  if ( this->ev != NATS_INVALID_ID ) {
    natsEvent_Destroy( this->ev );
    this->ev = NATS_INVALID_ID;
  }
}

TimeMSecs
RaiTimer_nats::GetInterval( void )
{
  return this->ival;
}

void
RaiTimer_nats::SetInterval( TimeMSecs interval )
{
  this->ival = interval;
  if ( this->ev != NATS_INVALID_ID ) {
    nats_status status = natsEvent_ResetTimerInterval( this->ev,
                                                  (double) interval / 1000.0 );
    if ( status != NATS_OK )
      throw RaiApi_nats::getNatsErr( status );
  }
}

/* ---- subscribe ------------------------------------------------------------ */

RaiSubscribe_nats::~RaiSubscribe_nats()
{
  if ( this->inList ) {
    this->q.apiLock->lock();
    this->q.rmSubList( this );
    this->q.subClock++;
    this->q.apiLock->unlock();
  }
  if ( this->snapId != NATS_INVALID_ID )
    natsEvent_Destroy( this->snapId );
  if ( this->listenId != NATS_INVALID_ID )
    natsEvent_Destroy( this->listenId );
  if ( this->subject != NULL )
    FREE( this->subject );
  if ( this->extra != NULL )
    delete this->extra;
}

/* a message on the subject (update) or on the snapshot inbox (snap) */
void
RaiSubscribe_nats::onMsg( natsEvent id,  natsMsg msg,  void *cl )
{
  RaiSubscribe_nats & me = *(RaiSubscribe_nats *) cl;
  const void        * bufp = NULL;
  const char        * subj = "";
  uint32_t            msgSize = 0;
  nats_status         status;

  status = natsMsg_GetData( msg, &bufp, &msgSize );
  if ( status != NATS_OK ) {
    RaiException e = RaiApi_nats::getNatsErr( status );
    logError( LERROR, e, "Getting message bytes" );
    return;
  }
  status = natsMsg_GetSubject( msg, &subj );
  if ( status != NATS_OK ) {
    RaiException e = RaiApi_nats::getNatsErr( status );
    logError( LERROR, e, "Getting send subject" );
    return;
  }
  status = natsMsg_GetReplySubject( msg, &me.reply );
  if ( status != NATS_OK || me.reply == NULL || me.reply[ 0 ] == '\0' )
    me.reply = NULL;
  try {
    Rai_u16  msgType   = SassConst::MAX_TYPE,
             recType   = 0,
             seqNo     = 0,
             recStatus = SassConst::MAX_STATUS;
    RaiMsg   raiMsg;
    unpackNatsPayload( raiMsg, bufp, msgSize );
    raiMsg.GetSassHeader( msgType, recType, seqNo, recStatus );
    me.q.apiLock->lock();
    RaiSubscribe::RaiSubState ostate = me.state,
                              rstate = SassToSubState( msgType, recStatus );
    me.state = NewState( ostate, rstate );
    RaiMsgEvent event( me, subj,
                  ( id == me.snapId ? RaiMsgEvent::SNAP : RaiMsgEvent::UPDATE ),
                    msgType, recStatus, ostate, rstate, me.state, 0, 0, 0, 0 );
    me.count++;
    if ( RaiApi::contentEntitle( &raiMsg ) == false ) {
      logDebug( LDEBUG, "failed content entitlement test" );
    }
    else {
      try {
        me.cb.onMsg( event, raiMsg, me.cl );
      } catch ( Error e ) {
        logError( LERROR, e, "Callback exception" );
      }
    }
    me.q.apiLock->unlock();
  } catch ( Error e ) {
    logError( LERROR, e, "Unpack message failed" );
  }
}

void
RaiSubscribe_nats::startTimer( Rai_u32 timeoutMSecs )
{
  if ( timeoutMSecs != 0 && this->subType != (byte) UPDATE &&
       this->state != STATE_WILDCARD && ! this->hasTimer ) {
    RaiSubscribeTimer_nats *t = NEW RaiSubscribeTimer_nats( *this );
    natsEvent timerId;
    this->hasTimer = true;
    natsEvent_CreateTimer( &timerId, this->q.session.xQ,
                           RaiSubscribeTimer_nats::onTimer,
                           (double) timeoutMSecs / 1000.0, t );
  }
}

void
RaiSubscribe_nats::Start( const char *subject,  RaiSubParameter parm,
                          Rai_u32 timeoutMSecs )
{
  nats_status  status;
  const char * p;

  if ( subject == NULL )
    throw RaiApiErr::getErr( RaiApiErr::BAD_SUBJECT );
  if ( this->InProgress() )
    this->Cancel();

  STRDUP( this->subject, subject );
  this->noPrefix = ( ( (byte) parm & NO_PREFIX ) != 0 );
  parm = (RaiSubParameter) ( (byte) parm & BOTH );
  this->subType = parm;
  /* check if wildcard present (NATS: A.*.C, A.>) */
  this->state = STATE_NO_MSG;
  if ( (p = ::strchr( subject, '>' )) != NULL ) {
    if ( p == subject || *(p - 1) == '.' ) {
      if ( *(p + 1) == '\0' )
        this->state = STATE_WILDCARD;
    }
  }
  if ( this->state != STATE_WILDCARD &&
       (p = ::strchr( subject, '*' )) != NULL ) {
    if ( p == subject || *(p - 1) == '.' ) {
      if ( *(p + 1) == '.' || *(p + 1) == '\0' )
        this->state = STATE_WILDCARD;
    }
  }
  if ( RaiApi::canSubscribe( subject ) == false ) {
    logDebug( LDEBUG, "entitle check failed" );
    throw RaiApiErr::getErr( RaiApiErr::NO_PERMISSION );
  }

  this->q.apiLock->lock();
  try {
    this->q.addSubList( this );
    this->q.subClock++;
    this->startTimer( timeoutMSecs );
    this->sendSubRequest();

    if ( parm != SNAP ) {
      /* listen for subject */
      status = this->q.session.createListener( &this->listenId,
                                               this->q.natsQ, subject,
                                               RaiSubscribe_nats::onMsg, this );
      if ( status != NATS_OK )
        throw RaiApi_nats::getNatsErr( status );
    } /* all cases except SNAP */
  } catch ( RaiException e ) {
    logError( LERROR, e, "%s subject \"%s\"",
              parm == BOTH ? "Subscribe" :
              ( parm == SNAP ? "Snapshot" : "Listen" ), subject );
    this->q.apiLock->unlock();
    throw e;
  }
  this->q.apiLock->unlock();
}

static void
convertToRvMsg( RaiMsg &rvmsg,  RaiMsg &raimsg )
{
  RaiField field;
  if ( field.First( &raimsg ) ) {
    do {
      if ( field.Type() != RAIMSG_MESSAGE )
        rvmsg.Append( &field );
      else {
        RaiMsg rvmsg2( RV_PROTO );
        convertToRvMsg( rvmsg2, *(RaiMsg *) field.Data() );
        rvmsg.Append( field.Name(), &rvmsg2 );
      }
    } while ( field.Next() );
  }
}

/* the snapshot / initial image request: an RVMSG { [extra fields,] flags }
 * published on <snapPrefix><subject> with the reply set to an inbox of
 * this transport; the image comes back on the inbox */
void
RaiSubscribe_nats::sendSubRequest( void )
{
  char            snapSubjectName[ SassConst::MAX_SUBJECT_LEN ],
                  inboxName[ NATS_MAX_INBOX_LEN ];
  RaiSubParameter parm = (RaiSubParameter) this->subType;
  RaiMsg          m( RV_PROTO );
  nats_status     status;

  if ( parm == UPDATE )
    return;

  /* create a snap/initial reply inbox */
  status = natsTransport_CreateInbox( this->q.session.tport, inboxName,
                                      sizeof( inboxName ) );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );
  if ( this->snapId != NATS_INVALID_ID ) {
    natsEvent_Destroy( this->snapId );
    this->snapId = NATS_INVALID_ID;
  }
  status = natsEvent_CreateListener( &this->snapId, this->q.natsQ,
                                     this->q.session.tport, inboxName,
                                     NATS_LISTEN_DEFAULT,
                                     RaiSubscribe_nats::onMsg, this );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );

  if ( this->extra != NULL )
    convertToRvMsg( m, *this->extra );

  if ( ! this->noPrefix && this->q.session.api.snapPrefix != NULL ) {
    unsigned int len = ::strlen( this->q.session.api.snapPrefix );
    if ( len >= sizeof( snapSubjectName ) )
      len = sizeof( snapSubjectName ) - 1;
    ::memcpy( snapSubjectName, this->q.session.api.snapPrefix, len );
    str_copy( &snapSubjectName[ len ], this->subject,
              sizeof( snapSubjectName ) - len );
  }
  else {
    str_copy( snapSubjectName, this->subject, sizeof( snapSubjectName ) );
  }
  Rai_u16 flags = ( parm == BOTH ? ( SassConst::SUBSCRIBE_FLAG |
                                     SassConst::INITIAL_VALUES_FLAG )
                                 : SassConst::SNAPSHOT_FLAG );
  m.Append( "flags", flags );

  status = natsTransport_Send( this->q.session.tport, snapSubjectName,
                               inboxName, m.Packed(), m.PackSize(),
                               RVMSG_TYPE_ID );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );
}

void
RaiSubscribe_nats::Refresh( Rai_u32 timeoutMSecs )
{
  RaiException e2 = NULL;

  this->q.apiLock->lock();
  if ( this->snapId != NATS_INVALID_ID ) {
    natsEvent_Destroy( this->snapId );
    this->snapId = NATS_INVALID_ID;
  }
  try {
    this->startTimer( timeoutMSecs );
    this->sendSubRequest();
  } catch ( RaiException e ) {
    logError( LERROR, e, "Refresh subject \"%s\"", this->subject );
    e2 = e;
  }
  this->q.apiLock->unlock();
  if ( e2 != NULL )
    throw e2;
}

void
RaiSubscribeTimer_nats::onTimer( natsEvent ev,  natsMsg,  void *cl )
{
  RaiSubscribeTimer_nats * me = (RaiSubscribeTimer_nats *) cl;
  RaiQueue_nats     * q;
  RaiSubscribe_nats * s = NULL;

  natsEvent_Destroy( ev );

  me->session.lock->lock();
  for ( q = me->session.qHd; q != NULL; q = q->next )
    if ( q == &me->q )
      break;
  me->session.lock->unlock();

  if ( q != NULL ) {
    me->q.apiLock->lock();
    for ( s = me->q.subHd; s != NULL; s = s->next )
      if ( s == &me->sub )
        break;
    if ( s != NULL ) {
      s->hasTimer = false;
      if ( s->count != me->count )
        s = NULL;
    }
    if ( s != NULL ) {
      RaiSubscribe::RaiSubState recvState;
      static const Rai_u16 msgType   = SassConst::TRANSIENT,
                           recStatus = SassConst::STATUS_TIMEOUT;
      RaiMsg msg;
      try {
        create_status_msg( msg, recvState, msgType, recStatus );
        RaiSubscribe::RaiSubState oldState = s->state;
        s->state = RaiSubscribe::NewState( oldState, recvState );
        RaiMsgEvent ev( *s, s->subject, RaiMsgEvent::UPDATE, msgType,
                        recStatus, oldState, recvState, s->state, 0, 0, 0, 0 );
        s->cb.onMsg( ev, msg, s->cl );
      } catch ( Error e ) {
        logError( LERROR, e, "Subscribe onTimer" );
      }
    }
    me->q.apiLock->unlock();
  }
  delete me;
}

void
RaiSubscribe_nats::Cancel( void )
{
  this->q.apiLock->lock();
  this->q.rmSubList( this );
  this->q.subClock++;
  if ( this->snapId != NATS_INVALID_ID ) {
    natsEvent_Destroy( this->snapId );
    this->snapId = NATS_INVALID_ID;
  }
  if ( this->listenId != NATS_INVALID_ID ) {
    natsEvent_Destroy( this->listenId );
    this->listenId = NATS_INVALID_ID;
  }
  this->q.apiLock->unlock();
}

const char *
RaiSubscribe_nats::Subject( void )
{
  return this->subject;
}

bool
RaiSubscribe_nats::InProgress( void )
{
  return this->snapId != NATS_INVALID_ID || this->listenId != NATS_INVALID_ID;
}

void
RaiSubscribe_nats::SetExtra( RaiMsg *ex )
{
  if ( this->extra != NULL ) {
    delete this->extra;
    this->extra = NULL;
  }
  if ( ex != NULL ) {
    RaiMsg *x = NEW RaiMsg();
    x->Copy( ex );
    this->extra = x;
  }
}

/* ---- publish -------------------------------------------------------------- */

/* the packed bytes go out as they are (a TIBMSG, TIB_SASS or RVMSG), no RV
 * opaque wrapper: NATS payloads are raw and the subscribers unwrap either */
void
RaiPublish_nats::Publish( const char *subject,  const void *buffer,
                          Rai_u32 size,  TimeNSecs /* stamp */,
                          Rai_u32 msgTypeId )
{
  nats_status  status;
  unsigned int len;
  char         subjBuf[ SassConst::MAX_SUBJECT_LEN ];

  if ( this->prefix != NULL ) {
    ::memcpy( subjBuf, this->prefix, this->prefixLen );
    len = ::strlen( subject ) + 1;
    if ( len > sizeof( subjBuf ) - this->prefixLen )
      len = sizeof( subjBuf ) - this->prefixLen;
    ::memcpy( &subjBuf[ this->prefixLen ], subject, len );
    subjBuf[ sizeof( subjBuf ) - 1 ] = '\0';
    subject = subjBuf;
  }
  if ( RaiApi::canPublish( subject ) == false ) {
    logDebug( LDEBUG, "entitle check failed" );
    throw RaiApiErr::getErr( RaiApiErr::NO_PERMISSION );
  }
  status = natsTransport_Send( this->session.tport, subject, NULL, buffer,
                               size, msgTypeId );
  if ( status != NATS_OK )
    throw RaiApi_nats::getNatsErr( status );
}

/* ---- errors --------------------------------------------------------------- */

RaiException
RaiApi_nats::getNatsErr( nats_status status )
{
  static const char mod[] = "RaiApi_nats";
  static ErrorRec e[ 32 ];
  static const unsigned int numErrs = sizeof( e ) / sizeof( e[ 0 ] ) - 1;

  if ( status >= 0 && (unsigned int) status < numErrs ) {
    if ( e[ status ].module == NULL ) {
      e[ status ].status = (unsigned int) status;
      e[ status ].reason = natsStatus_GetText( status );
      if ( e[ status ].reason == NULL )
        e[ status ].reason = "null";
      e[ status ].module = mod;
    }
    return &e[ status ];
  }
  if ( e[ numErrs ].module == NULL ) {
    e[ numErrs ].status = 999;
    e[ numErrs ].reason = "NATS api call failed";
    e[ numErrs ].module = mod;
  }
  return &e[ numErrs ];
}
