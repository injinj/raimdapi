/* Copyright (c) 2009 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_raiapi__raiapi2_h__
#define __rai_raiapi__raiapi2_h__

#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_raiapi__raiapi2_h[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */

#if ! defined( RAIAPI_DLL_EXP )
#if defined( RAI_DLL )
#define RAIAPI_DLL_EXP __declspec(dllimport)
#else
#define RAIAPI_DLL_EXP
#endif
#endif

#ifndef __rai_msg__rai_msg_h__
#include "msg/rai_msg.h"
#endif
#ifndef __rai_msg__mfeed_dict_h__
#include "msg/mfeed_dict.h"
#endif
#ifndef __rai_msg__sass_const_h__
#include "msg/sass_const.h"
#endif
#ifndef __rai_base__time_h__
#include "base/time.h"
#endif
#ifndef __rai_util__args_h__
#include "util/args.h"
#endif
#ifndef __rai_base__sys_h__
#include "base/sys.h"
#endif
#ifndef __rai_base__log_h__
#include "base/log.h"
#endif
#ifndef CI_SASS_TYPE_ID
#define CI_SASS_TYPE_ID       0x62669718 /* CI_SASS */
#endif
#ifndef CI_SASS_FORM_TYPE_ID
#define CI_SASS_FORM_TYPE_ID  0x9049fdf4 /* CI_SASS_FORM */
#endif
#ifndef TIB_SASS_TYPE_ID
#define TIB_SASS_TYPE_ID      0x179ca0f5 /* TIB_SASS */
#endif
#ifndef TIB_SASS_FORM_TYPE_ID
#define TIB_SASS_FORM_TYPE_ID 0xa08b0040 /* TIB_SASS_FORM */
#endif
#ifndef RVMSG_TYPE_ID
#define RVMSG_TYPE_ID         0xebf946be /* RVMSG */
#endif
#ifndef RAIMSG_TYPE_ID
#define RAIMSG_TYPE_ID        0x07344064 /* TIBMSG */
#endif

/* predefine classes that are called in this headder */
class RaiMsgEvent;
class RaiSubscribeEvent;
class RaiDataLossEvent;
class RaiConnectionEvent;
class RaiSession;
class RaiQueue;
class RaiTimer;
class RaiSubscribe;
class RaiPublish;
class RaiInteractivePublish;
class RaiDict;
class RaiEntitlement;
namespace rai {
  class Args;
}

/* default args set by RaiApi returned by api->GetArgs() */
static const char raiapi_api_arg[]    = "api",
                  raiapi_userid_arg[] = "userid",
                  raiapi_appid_arg[]  = "appid";
/* args set by RaiApi returned by api->GetDictArgs() */
static const char cfile_path_arg[]    = "cfilePath",
                  tss_records_arg[]   = "tssRecords",
                  tss_fields_arg[]    = "tssFields",
                  appendix_a_arg[]    = "appendixA",
                  enumtype_def_arg[]  = "enumtypeDef";

class RAIAPI_DLL_EXP RaiApi {
 protected:
  char * api,   /* values filled in by ParseArgs(), and/or SetIoctl() */
       * userid,
       * appid;
  RaiApi() : api( 0 ), userid( 0 ), appid( 0 ) {}

 public:
  /* Load api class, if apiname = NULL, the default (capr) will be used unless
     -api is an argment in argc/argv[] */
  static RaiApi *RaiOpen( const char *apiname,  int argc,  char *argv[] )
  /* Get the name of the api name of the protocol */;
  virtual const char *GetApiName( void ) = 0;
  /* Rai API Version */
  static const char *RaiVersion( void );
  /* UserId */
  const char *GetUserId( void ) const { return this->userid; }
  /* AppId */
  const char *GetAppId( void ) const { return this->appid; }
  /* Get the api Args, for example:  daemon, network, service */
  virtual void GetArgs( rai::Args &args );
  /* Get the SASS dictionary args:  cfilePath, tssRecords, tssFields... */
  static void GetDictArgs( rai::Args &args );
  /* Parse api Args, use after args.processArgs( argc, argv ) is called */
  virtual void ParseArgs( rai::Args &args ) = 0;
  /* If has log, logLevel, logVerb, logXml args, then open the log file,
   * otherwise return false */
  static bool OpenLog( rai::Args &args );
  /* Prints to log file if level >= Log::minLevel, using the log above */
  virtual void PrintLog( rai::Log::LogLevel level,  const char *where, int line,
                         RaiException err,  const char *fmt,  ... )
    #if defined( __GNUC__ )
        __attribute__((format(printf,6,7)));
    #else
        ;
    #endif
  /* If has cfilePath, tssRecords, tssFIelds, appendixA, enumtypeDef,
   * then open the SASS dictionary and marketfeed dictionary, otherwise
   * return false */
  static bool OpenDict( rai::Args &args );
  /* Create a new session */
  virtual RaiSession *CreateSession( void ) = 0;

  /* Create a SASS Qform class message, which is RaiMsg sass qform with 4
   * fields */
  static RaiMsg * NewSASSMsg( Rai_u16 MsgType, Rai_u16 RecType, Rai_u16 SeqNo=0,
                              Rai_u16 RecStatus = rai::SassConst::STATUS_OK )
 {
    return NewRaiMsg( TIB_SASS_PROTO, MsgType, RecType, SeqNo, RecStatus );
  }
  static RaiMsg * NewSASSMsg( Rai_u16 MsgType, const char * FormType,
                              Rai_u16 SeqNo = 0,
                              Rai_u16 RecStatus = rai::SassConst::STATUS_OK )
 {
    return NewRaiMsg( TIB_SASS_PROTO, MsgType, FormType, SeqNo, RecStatus );
  }
  /* Create a SASS RaiMsg class message, which is a RaiMsg self describing
   * format with 4 fields */
  static RaiMsg * NewRaiMsg( short MsgType, Rai_u16 RecType, Rai_u16 SeqNo = 0,
                             Rai_u16 RecStatus = rai::SassConst::STATUS_OK )
 {
    return NewRaiMsg( RAIMSG_PROTO, MsgType, RecType, SeqNo, RecStatus );
  }
  static RaiMsg * NewRaiMsg( Rai_u16 MsgType, const char *FormType, 
                             Rai_u16 SeqNo = 0,
                             Rai_u16 RecStatus = rai::SassConst::STATUS_OK )
 {
    return NewRaiMsg( RAIMSG_PROTO, MsgType, FormType, SeqNo, RecStatus );
  }
  /* Convenience function to create a message with 4 header fields, usually for
   * publishing */
  static RaiMsg * NewRaiMsg( RaiMsg_protocol proto,  Rai_u16 MsgType,
                             Rai_u16 RecType,  Rai_u16 SeqNo = 0,
                             Rai_u16 RecStatus = rai::SassConst::STATUS_OK )
;
  /* Convenience function to create a message and lookup a form type name,
   * using the current dictionary loaded */
  static RaiMsg * NewRaiMsg( RaiMsg_protocol proto,  Rai_u16 MsgType,
                             const char *FormType,  Rai_u16 SeqNo = 0,
                             Rai_u16 RecStatus = rai::SassConst::STATUS_OK ) 
  /* Close the api, should close sessions first */;
  virtual void Close( void ) = 0;
  /* Set parameters for controlling the api */
  virtual bool SetIoctl( const char *parameter,  const void *value )
  /* Get parameters for controlling the api */;
  virtual bool GetIoctl( const char *parameter,  void *value );

  /* entitlements set/check interface convenience methods */
  /* login to entitlement system */
  static void RaiLogin( RaiSession *session, const char *userDetails );
  /* check if allowed to subscribe on subject */
  static bool canSubscribe( const char *subject );
  /* check if allowed to publish on subject */
  static bool canPublish( const char *subject );
  /* Perform content entitle check against message */
  static bool contentEntitle( RaiMsg * raiMsg );
  /* If the entitlement has been loaded */
  static bool haveEntitle( void );

  virtual ~RaiApi() {}
};

/* Callback for messages recieved on subscriptions or snapshots */
class RAIAPI_DLL_EXP RaiMsgCallback {
 public:
  /* Event contains subscription info like the subject, message is the data */
  virtual void onMsg( RaiMsgEvent &event,  RaiMsg &message,  void *cl ) = 0;

  virtual ~RaiMsgCallback() {}
};

/* Callback for timer events */
class RAIAPI_DLL_EXP RaiTimerCallback {
 public:
  /* Timer event expired */
  virtual void onTimer( RaiTimer &timer,  void *cl ) = 0;

  virtual ~RaiTimerCallback() {}
};

/* Callback for subscribe events, sent to interactive publishers to notify
   when subscribers are starting, stopping, or snapshot */
class RAIAPI_DLL_EXP RaiSubscribeCallback {
 public:
  /* Event contains publisher */
  virtual void onSubscribe( RaiSubscribeEvent &event,  RaiMsg &message,
                            void *cl ) = 0;
  virtual ~RaiSubscribeCallback() {}
};

/* Callback when data loss occurs, connect loss or packet loss, these are
 * asynchronous events that are not associated with a RaiQueue and may be
 * dispatched on session event threads or network event threads */
class RAIAPI_DLL_EXP RaiDataLossCallback {
 public:
  /* Called when a connection is dropped or data packets are lost */
  virtual void onDataLoss( RaiDataLossEvent &event,  void *cl ) = 0;
  /* Called when a connection is established or a mcast network is joined */
  virtual void onConnection( RaiConnectionEvent &event,  void *cl );

  virtual ~RaiDataLossCallback() {}
};

/* Callback for application events */
class RAIAPI_DLL_EXP RaiAppCallback {
 public:
  virtual void onAppEvent( void *eventData,  Rai_i32 eventEnum ) = 0;

  virtual ~RaiAppCallback() {}
};

/* The session has all the associated structures for a pub/sub interface */
class RAIAPI_DLL_EXP RaiSession {
 public:
   /* Start the session by initializing the transports */
   virtual void Start( void );
  /* Create a queue.  Passing direct = true causes recv threads to dispatch
   * message callbacks, and passing direct = false causes the queue thread to
   * dispatch.  This feature is only available in protocols where this can be
   * controlled (capr, embedded).  Using direct = true minimizes copying and
   * context switches, but if callbacks are compute heavy, then multiple queues
   * and direct = false will be better because cpu will be offloaded onto queue
   * threads. */
  virtual RaiQueue *CreateQueue( bool direct = false ) = 0;
  /* A publisher, passing audoInc = true causes a msg sequence number update 
   * when Publish() is called:
   * RaiMsg::Update( "SEQ_NO", RaiPublish::nextSeqno++ ) */
  virtual RaiPublish *CreatePublish( bool autoInc = false )
  /* A dictionary loader */ = 0;
  virtual RaiDict *CreateDict( void ) = 0;
  /* Stop the session event processing */
  virtual void Destroy( void ) = 0;
  /* Login to entitlements facility, if user NULL, use RaiApi::userid */
  virtual RaiEntitlement *Login( const char *user = NULL )
  /* Data loss errors */ = 0;
  virtual void SetDataLossCB( RaiDataLossCallback *cb,  void *closure = NULL )
 = 0;
  /* Notify subscription state by sending a message to each subscription */
  virtual void NotifyStatus( Rai_u16 msgType = rai::SassConst::TRANSIENT,
                        Rai_u16 recStatus = rai::SassConst::STATUS_STALE_VALUE )
 = 0;
  /* Get the api interface this session was created from */
  virtual RaiApi *GetApi( void ) = 0;

  RaiSession();
  virtual ~RaiSession();
  /* Assigns a name for the session, which is used by embedded raicache
   * as a identifier for the service, the default name is the session memory
   * address */
  void setSessionName( const char* sessionName );
  /* Gets the session name set by the above function */
  const char* getSessionName() const;

 private:
  char* sessionName;
};

/* A queue is used for dispatching subscription callbacks and timer events */
class RAIAPI_DLL_EXP RaiQueue {
 public:
  /* A subscription */
  virtual RaiSubscribe *CreateSubscribe( RaiMsgCallback *cb,
                                         void *closure = NULL )
  /* A timer event */ = 0;
  virtual RaiTimer *CreateTimer( RaiTimerCallback *cb, void *closure = NULL )
  /* An interactive publisher */ = 0;
  virtual RaiInteractivePublish *CreateInteractivePublish(
                               RaiSubscribeCallback *cb,
                               void *closure = NULL ) = 0;
  /* Notify subscription state by sending a message to each sub on this queue */
  virtual void NotifyStatus( Rai_u16 msgType = rai::SassConst::TRANSIENT,
                       Rai_u16 recStatus = rai::SassConst::STATUS_STALE_VALUE )
 = 0;
  /* An application event, posts an event to a queue and dispatches to
   * RaiAppCallback; eventData and eventEnum are any values the application
   * defines;  eventPriority is 0->4, 0 is highest, timers use priority 2;
   * expireMSecs is amount of delay in milliseconds before event expires, use
   * 0 to fire the event immediately after transfering to the queue thread */
  static const Rai_u8 EV_PRIO_0 = 0;
  static const Rai_u8 EV_PRIO_1 = 1;
  static const Rai_u8 EV_PRIO_2 = 2;
  static const Rai_u8 EV_PRIO_3 = 3;
  static const Rai_u8 EV_PRIO_4 = 4;
  #define EV_PRIO_HIGH EV_PRIO_0
  #define EV_PRIO_LOW EV_PRIO_4
  #define EV_PRIO_NORM EV_PRIO_2
  virtual void QueueEvent( RaiAppCallback *cb,  void *eventData,
                           Rai_i32 eventEnum, Rai_u8 eventPriority = EV_PRIO_2,
                           Rai_u32 expireMSecs = 0 ) = 0;
  /* Dispatch forever, don't return */
  virtual void Mainloop( void ) = 0;
  /* Return if dispatched one event or intervalMSecs is timed out */
  virtual void TimedDispatch( Rai_u32 ivalMSecs ) = 0;
  /* Return after dispatching one event */
  virtual void Dispatch( void ) = 0;
  /* Return number of items in queue */
  virtual Rai_u32 GetDepth( void ) = 0;
  /* Get session of queue */
  virtual RaiSession *GetSession( void ) = 0;
  /* Stop the event processing */
  virtual void Destroy( void ) = 0;

  virtual ~RaiQueue() {}
};

/* A timer event, which calls TimerCallback::onTimer at interval millisecs,
   repeatedly until stopped  */
class RAIAPI_DLL_EXP RaiTimer {
 public:
  /* Start the timer at the current interval setting */
  virtual void Start( void ) = 0;
  /* Stop the timer processing */
  virtual void Stop( void ) = 0;
  /* Get the current timer interval setting in milliseconds */
  virtual rai::TimeMSecs GetInterval( void ) = 0;
  /* Set the interval in milliseconds */
  virtual void SetInterval( rai::TimeMSecs interval ) = 0;
  /* Get the queue which this timer is on */
  virtual RaiQueue *GetQueue( void ) = 0;

  virtual ~RaiTimer() {}
};

/* A dictionary loader */
class RAIAPI_DLL_EXP RaiDict {
 public:
  /* Load dictionary, if dictSubject is NULL, the protocol will use the
   * default name for the dictionary, if loadWait true then wait for dict */
  virtual void Load( Rai_u32 timeoutSecs = 3,  const char *dictSubject = 0,
                     bool loadWait = true ) = 0;
  /* If the dictionary has been loaded */
  virtual bool HaveDict( void ) = 0;
  /* If the dictionary hasn't timed out and hasn't been loaded yet */
  virtual bool InProgress( void ) = 0;
  /* Get the session which created this */
  virtual RaiSession *GetSession( void ) = 0;

  virtual ~RaiDict() {}
};

/* Entitlements interface */
class RAIAPI_DLL_EXP RaiEntitlement {
  public:
    /* Load entitlements, if called with no userId then use default Id.
     *  once entitlement object created then all pub and sub will check for 
     *  permissions */
    virtual void Load( RaiSession *session, const char *userDetails ) = 0;
    /* Load entitlement data from local filesystem */
    virtual bool LoadLocal( const char *userDetails ) = 0;
    /* If the entitlement has been loaded */
    virtual bool HaveEntitlements( void ) = 0;
    /* If the entitlment hasn't timed out and hasn't been loaded yet */
    virtual bool InProgress( void ) = 0;
    /* Perform subscription check against subject */
    virtual bool canSubscribe( const char * subject ) = 0;
    /* Perform publish check against subject */
    virtual bool canPublish( const char * subject ) = 0;
    /* Perform content entitle check against message */
    virtual bool contentEntitle( RaiMsg * raiMsg ) = 0;
    /* log subscription close */
    virtual void closeSubscribe( const char * subject ) = 0;
    /* Stop the entitlement processing */
    virtual void Destroy( void ) = 0;

    virtual ~RaiEntitlement() {}
};

/* Start and stop a subscription or snapshot on a subject */
class RAIAPI_DLL_EXP RaiSubscribe {
 public:
  enum RaiSubParameter {
    UPDATE  = 1,
    SNAP    = 2,
    BOTH    = 3,
    NO_PREFIX = 4
  };

  enum RaiSubState {
    STATE_NO_MSG   = 0, /* no message received for subscription yet */
    STATE_WILDCARD = 1, /* no state maintained for wildcard, always wildcard */
    STATE_NO_HDR   = 2, /* msg received which does not have state information */
    STATE_INITIAL  = 3, /* initial or verify value received, 0 or more updates*/
    STATE_UPDATE   = 4, /* update received, but no initial or verify */
    STATE_NOTFOUND = 5, /* not found received, no initial value */
    STATE_STALE    = 6, /* may have missing messages since last update */
    STATE_DROPPED  = 7  /* last update was dropped status */
#define MAX_RAI_SUB_STATE 8
  };
  /* State transitions, if bit of state_transitions[ old ][ recv ] == 0, then
   * recv is new state, otherwise old state remains (see NewState() below)
   * For example:
   *     old state       recv message      new state
   *   STATE_INITIAL  + STATE_UPDATE   = STATE_INITIAL  (no transition)
   *   STATE_UPDATE   + STATE_INITIAL  = STATE_INITIAL  (transition)
   *   STATE_NO_MSG   + STATE_NOTFOUND = STATE_NOTFOUND (transition)
   *   STATE_STALE    + STATE_UPDATE   = STATE_UPDATE   (transition)
   *   STATE_DROPPED  + STATE_UPDATE   = STATE_DROPPED  (no transition)
   *   STATE_WILDCARD + STATE_UPDATE   = STATE_WILDCARD (no transition)
   */
  static const Rai_u64 state_transitions =
#define Y( b, n ) ( ( b != 0 ) ? n : 0 )
#define X( B0, B1, B2, B3, B4, B5, B6, B7 ) \
  Y( STATE_ ## B0, 1 ) | Y( STATE_ ## B1, 2 ) | Y( STATE_ ## B2, 4 ) | \
  Y( STATE_ ## B3, 8 ) | Y( STATE_ ## B4, 16 ) | Y( STATE_ ## B5, 32 ) | \
  Y( STATE_ ## B6, 64 ) | Y( STATE_ ## B7, 128 )
#define U Rai_u64
#define Z( A0, A1, A2, A3, A4, A5, A6, A7 ) \
  ( ((U)A0) << 0 ) | ( ((U)A1) << 8 ) | ( ((U)A2) << 16 ) | ( ((U)A3) << 24 ) | \
  ( ((U)A4) << 32 ) | ( ((U)A5) << 40 ) | ( ((U)A6) << 48 ) | ( ((U)A7) << 56 )
#define STATE_RECV 0
/* rows are current state, columns are recv state (0 -> 7)
 *       0        1        2        3        4        5        6        7
 *    NO_MSG WILDCARD   NO_HDR  INITIAL   UPDATE NOTFOUND    STALE  DROPPED */
Z(
  X(  NO_MSG,    RECV,    RECV,    RECV,    RECV,    RECV,    RECV,    RECV),
  X(WILDCARD,WILDCARD,WILDCARD,WILDCARD,WILDCARD,WILDCARD,WILDCARD,WILDCARD),
  X(  NO_HDR,    RECV,  NO_HDR,    RECV,    RECV,    RECV,    RECV,    RECV),
  X( INITIAL, INITIAL, INITIAL, INITIAL, INITIAL, INITIAL,    RECV,    RECV),
  X(  UPDATE,  UPDATE,    RECV,    RECV,  UPDATE,  UPDATE,    RECV,    RECV),
  X(NOTFOUND,NOTFOUND,    RECV,    RECV,    RECV,NOTFOUND,    RECV,    RECV),
  X(   STALE,   STALE,   STALE,    RECV,    RECV,    RECV,   STALE,    RECV),
  X( DROPPED, DROPPED, DROPPED,    RECV, DROPPED, DROPPED, DROPPED, DROPPED)
);
#undef STATE_RECV
#undef Y
#undef X
#undef U
#undef Z

  static inline RaiSubState NewState( RaiSubState old,  RaiSubState recv ) {
    if ( ( RaiSubscribe::state_transitions &
           ( 1ULL << (old*MAX_RAI_SUB_STATE + recv) ) ) != 0 )
      return old;
    return recv;
  }
  /* convert a MSG_TYPE, REC_STATUS into a RaiSubState */
  static inline RaiSubState SassToSubState( Rai_u16 msgType,
                                            Rai_u16 recStatus ) {
    switch ( msgType ) {
      case rai::SassConst::VERIFY:    /* 0 */ return STATE_INITIAL;
      case rai::SassConst::UPDATE:    /* 1 */ return STATE_UPDATE;
      case rai::SassConst::CORRECT:   /* 2 */ return STATE_UPDATE;
      case rai::SassConst::CLOSING:   /* 3 */ return STATE_UPDATE;
      case rai::SassConst::DROP:      /* 4 */
        /* publisher sends this when there is no subscribers,
         * shouldn't recv it unless network conditions are such that
         * session status or reassert messages are not being recvd by pub */
        if ( recStatus == rai::SassConst::STATUS_NOSUBSCRIBERS )
          return STATE_STALE;
        /* other drop status causes DROPPED state:
             recStatus == SassConst::EXPIRED ||
             recStatus == SassConst::STATUS_ENTITLEMENT_DENIED */
        return STATE_DROPPED;
      case rai::SassConst::AGGREGATE: /* 5 */ return STATE_UPDATE;
      case rai::SassConst::STATUS:    /* 6 */ return STATE_INITIAL;
      case rai::SassConst::CANCEL:    /* 7 */ return STATE_UPDATE;
      case rai::SassConst::INITIAL:   /* 8 */ return STATE_INITIAL;
      case rai::SassConst::TRANSIENT: /* 9 */
      case rai::SassConst::SERVICE_STATUS: /* 19 */
        if ( recStatus == rai::SassConst::STATUS_FEED_UP ||
             recStatus == rai::SassConst::STATUS_OK )
          return STATE_UPDATE;
        if ( recStatus == rai::SassConst::STATUS_STALE_VALUE ||
             recStatus == rai::SassConst::STATUS_FEED_DOWN ||
             recStatus == rai::SassConst::STATUS_FEED_SWITCHOVER ||
             recStatus == rai::SassConst::STATUS_DATA_SUSPECT ||
             recStatus == rai::SassConst::STATUS_TPT_DISCONNECTED )
          return STATE_STALE;
        /* other transient status causes NOT_FOUND state:
             recStatus == SassConst::STATUS_TEMP_UNAVAIL ||
             recStatus == SassConst::STATUS_NOT_FOUND ||
             recStatus == SassConst::STATUS_NO_REPLY ||
             recStatus == SassConst::STATUS_TIMEOUT */
        return STATE_NOTFOUND;
      case rai::SassConst::DERIVED:   /* 10 */ return STATE_INITIAL;
      case rai::SassConst::DELETE:    /* 11 */ return STATE_DROPPED;
      case rai::SassConst::SUBREINIT: /* 12 */ return STATE_INITIAL;
      case rai::SassConst::SNAPSHOT:  /* 13 */ return STATE_INITIAL;
      case rai::SassConst::INITIAL_PASS_THRU:  /* 24 */ return STATE_INITIAL;
      case rai::SassConst::UPDATE_PASS_THRU:   /* 25 */ return STATE_UPDATE;
      case rai::SassConst::INITIAL_AGGREGATE:  /* 26 */ return STATE_INITIAL;
      case rai::SassConst::UPDATE_AGGREGATE:  /* 27 */  return STATE_UPDATE;
      case rai::SassConst::FINISH_AGGREGATE:  /* 28 */  return STATE_UPDATE;
      default:                                 return STATE_UPDATE;
      case rai::SassConst::MAX_TYPE:           return STATE_NO_HDR;
    }
  }
  static const char *StateToString( RaiSubState state );

  const char *GetStateString( void ) const {
    return StateToString( this->state );
  }

  RaiSubState state; /* current state */

  /* Start subscription or snapshot on subject */
  //virtual void Start( const char *subject,  RaiSubParameter parm = BOTH,
  virtual void Start( const char *subject,  RaiSubParameter parm = BOTH,
                      Rai_u32 timeoutMSecs = 0 ) = 0;
  /* Cancel subscription or snapshot */
  virtual void Cancel( void ) = 0;
  /* Refresh subscription by requesting initial or snapshot */
  virtual void Refresh( Rai_u32 timeoutMSecs = 0 ) = 0;
  /* The name of subject that is subscribed passed in Start() */
  virtual const char *Subject( void ) = 0;
  /* Whether started sub/snap hasn't been canceled or finished */
  virtual bool InProgress( void ) = 0;
  /* Get queue subscription is on */
  virtual RaiQueue *GetQueue( void ) = 0;
  /* Set extra parameters passed with subscription at Start(), null erases */
  virtual void SetExtra( RaiMsg *ex ) = 0;

  RaiSubscribe( RaiSubState init = STATE_NO_MSG ) : state( init ) {}
  virtual ~RaiSubscribe() {}
};

/* A publisher sends messages to the network */
class RAIAPI_DLL_EXP RaiPublish {
 protected:
   char  * prefix;    /* publish prefix, default is none */
   Rai_u32 prefixLen; /* strlen() of prefix */
   Rai_u32 nextSeqno; /* RaiMsg::Update( "SEQ_NO", nextSeqno++ ) when autoInc
                         is true, only 16 bits will be used when using the SASS
                         field definition */
   bool    autoInc;   /* if true, uses nextSeqno++ to update the field "SEQ_NO"
                         on Publish() using a RaiMsg */
   RaiPublish( bool autoIncrement = false );
 public:
   /* Publish a message on subject - assumes raiMsg object is only
    * valid for the duration of the call. May take a copy if
    * required.  TimeNSecs stamp can be used by receivers to calculate latency.
    * Will update SEQ_NO field when autoInc is true, otherwise RaiMsg is a
    * const value */
   virtual void Publish( const char *subject,  RaiMsg &raiMsg,
                         rai::TimeNSecs stamp = 0 );

   /* Publish a message on subject - takes ownership and will delete the
    * raiMsg.  TimeNSecs stamp can be used by receivers to calculate latency.
    * Will update SEQ_NO field when autoInc is true. */
   virtual void Publish( const char *subject,  RaiMsg* raiMsg,
                         rai::TimeNSecs stamp = 0 );

   /* Publish a message buffer on subject - assumes buffer is only
    * valid for the duration of the call.  MsgTypeId identifies the message,
    * it is a crc_c hash of the type name (some are defined in std_message.h).
    * May copy if required.  Will not update the SEQ_NO field (the buffer is
    * an opaque const). */
   virtual void Publish( const char *subject,  const void *buffer, Rai_u32 size,
                         rai::TimeNSecs stamp = 0,  Rai_u32 msgTypeId = 0 )
 = 0;
  /* On publish, add prefix to subject with this, sass2/rv often uses "_TIC." */
  virtual void SetPrefix( const char *prefix = NULL );
  /* Get the prefix value */
  virtual const char *GetPrefix( void );
  /* Get the nextSeqno value, when using SASS SEQ_NO field definition, it will
   * only use the lower 16 bits */
  virtual Rai_u32 GetSeqno( void );
  /* Set the nextSeqno value, when using SASS SEQ_NO field definition, it will
   * only use the lower 16 bits */
  virtual void SetSeqno( Rai_u32 newSeqno );
  /* Destroy publisher */
  virtual void Destroy( void ) = 0;
  /* Get parent session that publisher was created from */
  virtual RaiSession *GetSession( void ) = 0;

  virtual ~RaiPublish();
};

/* An interactive publisher filters publishing by subscriber interest */
class RAIAPI_DLL_EXP RaiInteractivePublish : public RaiPublish {
 public:
  /* Start interactive publish, listen for requests on subject */
  virtual void InteractiveStart( const char *subject ) = 0;
  /* Cancel interactive publish */
  virtual void InteractiveCancel( void ) = 0;
  /* Whether interactive publisher is in progress */
  virtual bool InProgress( void ) = 0;
  /* Get the queue the interactive publisher is on */
  virtual RaiQueue *GetQueue( void ) = 0;

  virtual ~RaiInteractivePublish() {}
};

/* Event passed to RaiMsgCallback::onMsg() when Subscribe recvs a message */
class RAIAPI_DLL_EXP RaiMsgEvent {
 public:
  enum RaiMsgEventType {
    SNAP    = 0,
    UPDATE  = 1
  };
  RaiSubscribe  & subscribe; /* the subscription that started the subject */
  const char    * subject;   /* the recieved subject (which may be an _INBOX) */
  RaiMsgEventType type;
  Rai_u16         msgType,
                  recStatus;

  RaiSubscribe::RaiSubState oldState, /* previous state */
                            recv,     /* state derived from msgType, recStatus*/
                            state;    /* state after msg {msgType, recStatus} */
  /* These are 64 bit unix timestamps in nanoseconds GMT since 1970, */
  /* equivalent to time_t time() * 10**9 + nanosecond offset */
  rai::TimeNSecs pubTime,/* if non-zero, when msg sent by publisher */
                 routeTime; /* if non-zero, when msg was created or cached */
  unsigned int counter,    /* sequence number of subject update, if supported */
               oldCounter; /* the previous counter */

  /* The subject that was used in subscribe.Start() */
  const char *SubscribedSubject( void ) {
    return this->subscribe.Subject();
  }
  RaiMsgEvent( RaiSubscribe &s,  const char *subj,  RaiMsgEventType t,
            Rai_u16 mtype,  Rai_u16 rstatus,  RaiSubscribe::RaiSubState ostate,
            RaiSubscribe::RaiSubState rstate, RaiSubscribe::RaiSubState nstate,
            rai::TimeNSecs ptm,  rai::TimeNSecs rtm,  unsigned int ctr,
            unsigned int oldCtr )
    : subscribe( s ), subject( subj ), type( t ), msgType( mtype ),
      recStatus( rstatus ), oldState( ostate ), recv( rstate ),
      state( nstate ), pubTime( ptm ), routeTime( rtm ), counter( ctr ),
      oldCounter( oldCtr ) {}
};

/* Event passed to RaiSubscribeCallback::onSubscribe() when interactive Publish
 * recvs a subscription notification */
class RAIAPI_DLL_EXP RaiSubscribeEvent {
 public:
  RaiInteractivePublish & publish;
  const char            * subject,
                        * reply;
  unsigned int            queryFlags; /* SassConst::SUBSCRIBE_FLAG, etc */

  RaiSubscribeEvent( RaiInteractivePublish &p,  const char *subj,
                     const char *reply, unsigned int fl )
    : publish( p ), subject( subj ), reply( reply ), queryFlags( fl ) {}
};

/* Event passed to RaiDataLossCallback::onDataLoss() */
class RAIAPI_DLL_EXP RaiDataLossEvent {
 public:
  RaiSession & session;
  const char * transportName, /* the transport name that loss occured */
             * description;  /* a description of the type of data loss */
  Rai_u32      inboundPacketLoss,  /* count of recv packets lost, if available*/
               outboundPacketLoss, /* count of sent packets lost, if available*/
               connectionCount; /* count of active connections */
  bool         connectionLoss, /* if connection oriented, this will be true
                                (but may be connection to daemon based mcast) */
               isMulticast;    /* true  = multicast connection,
                                  false = point to point connection */
  RaiDataLossEvent( RaiSession &s ) :
    session( s ), transportName( 0 ), description( 0 ), inboundPacketLoss( 0 ),
    outboundPacketLoss( 0 ), connectionCount( 0 ), connectionLoss( false ),
    isMulticast( false ) {}
};

/* Event passed to RaiDataLossCallback::onDataLoss() */
class RAIAPI_DLL_EXP RaiConnectionEvent {
 public:
  RaiSession & session;
  const char * transportName, /* the transport name that loss occured */
             * description;  /* a description of the connection */
  Rai_u32      connectionCount; /* count of active connections */
  bool         connectionOriented, /* if connection oriented, this will be true
                                (but may be connection to daemon based mcast) */
               isMulticast;    /* true  = multicast connection,
                                  false = point to point connection */
  RaiConnectionEvent( RaiSession &s ) :
    session( s ), transportName( 0 ), description( 0 ), connectionCount( 0 ),
    connectionOriented( false ), isMulticast( false ) {}
};

/* Errors in the API space */
namespace RaiApiErr {
  enum {
    BAD_SESSION       = 0,
    BAD_SUBJECT       = 1,
    BAD_RAIMSG        = 2,
    BAD_BUFFER        = 3,
    BAD_IOCTL_PARAM   = 4,
    BAD_DICT          = 5,
    BAD_SASS_INIT     = 6,
    DICT_LOAD_PENDING = 7,
    ENT_LOGIN_PENDING = 8,
    BAD_ENT           = 9,
    NO_PERMISSION     = 10,
    BAD_TRANSPORT     = 11,
    UNSUPPORTED_TSPT  = 12,
    UNSUPPORTED_MTHD  = 13,
    TSPT_DATALOSS     = 14,
    TSPT_RECV_ERR     = 15,
    BAD_PUBLISH       = 16,
    BAD_TIMER         = 17,
    BAD_SUBSCRIBE     = 18,
    BAD_MSG_TYPE      = 19
  };
  RAIAPI_DLL_EXP
  RaiException getErr( unsigned int status );
}

#endif
