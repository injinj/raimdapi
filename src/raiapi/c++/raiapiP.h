/* Copyright (c) 2004 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_raiapip__raiapiP_h__
#define __rai_raiapip__raiapiP_h__

#if ! defined(SWIG)
#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_raiapi__raiapiP_h[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */
#endif

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <regex.h>
#endif
#include "base/log.h"
#include "base/time.h"
#include "base/thread.h"
#include "util/args.h"
#include "util/str_util.h"
#include "raiapi.h"

using namespace rai;

#include <sassrv/rv7api.h>

RaiException badRvStatus( tibrv_status status );

struct RaiEntImpl {
  friend class    RaiPublish;
  tibrvId         rvD;
  tibrvId         rvD2;
  tibrvQueue      rvQ;
  tibrvQueue      rvQ2;
  //RaiDict       * me;
  bool            haveLoggedIn;
  bool            haveAnnounce;
  const char    * userCredentials;
  const char    * userLogin;
  RaiSession    * session;
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  regex_t         regexBuf;
#endif
  static Mutex  * lock;

  RaiEntImpl( void ) { // RaiEnt * ent ) : me( ent ) {
    if( ! lock ) {
      lock = Mutex::create();
    }
    this->haveLoggedIn = false;
    this->haveAnnounce = false;
    this->userCredentials = NULL;
    this->userLogin = NULL;
    this->session = NULL;
  };

  static void EntAnnOnMsg( tibrvId I, tibrvMsg msg, void *cl )  throw( RaiException );
  static void EntOnMsg( tibrvId I, tibrvMsg msg, void *cl )  throw( RaiException );
  void Login( RaiSession *session, char *entSubject )        throw( RaiException );
  void Load( RaiSession *session, char *entSubject )         throw( RaiException );
  void TCPLoad( RaiSession *session, char *entSubject )      throw( RaiException );
  void RVLoad( RaiSession *session, char *entSubject )       throw( RaiException );
  void WaitForEnt(RaiSession *session )                      throw( RaiException );
  bool match( const char *subject );
  bool canPublish( const char *subject )                     throw( RaiException );
  bool canSubscribe( const char *subject )                   throw( RaiException );
  SYS_OPS( RaiEntImpl );
  //RaiEntImpl() {};
};

struct RaiDictImpl {
  friend class RaiDict;
  tibrvId       rvD;
  tibrvQueue    rvQ;
  RaiDict       * me;
  static Mutex  * lock;

  RaiDictImpl( RaiDict * dict ) : me( dict ) {
    if( ! lock ) {
      lock = Mutex::create();
    }
  };

  static void DictOnMsg( tibrvId I, tibrvMsg msg, void *cl )  throw( RaiException );
  void Load( RaiSession *session, char *dictSubject )         throw( RaiException );
  void TCPLoad( RaiSession *session, char *dictSubject )      throw( RaiException );
  void RVLoad( RaiSession *session, char *dictSubject )       throw( RaiException );
  void WaitForDict(RaiSession *session )                      throw( RaiException );
  SYS_OPS( RaiDictImpl );
  RaiDictImpl() {};
};

struct TimerHandle {
  unsigned int            Id;
  RaiTimerCallback        callback;
  RaiSession            * session;
  void                  * arg;
  SYS_OPS( TimerHandle );
  TimerHandle() {};
};

struct RaiTimerImpl {
  friend class            RaiTimer;
  RaiTimer              * me;
  tibrvId                 TimerEvent;
  RaiTimerCallback        callback;

  RaiTimerImpl( RaiTimer * timer, RaiSession * session, RaiTimerCallback callback,
                TimeMSecs interval, void * closure )            throw( RaiException );

  static void RV_callback( tibrvId I, tibrvMsg msg, void *cl );
  TimerHandle * AddTimer( RaiSession * session, RaiTimerCallback callback,
                          void * closure)                       throw( RaiException );
  SYS_OPS( RaiTimerImpl );
  RaiTimerImpl() {};
  ~RaiTimerImpl();
};

struct SubHandle {
  tibrvEvent      subId;
  tibrvEvent      subId2;
  RaiCallback     callback;
  RaiSession    * session;
  void          * arg;
  char          * subSubject;

  SYS_OPS( SubHandle );
  SubHandle() { 
    subId       = TIBRV_INVALID_ID;
    subId2      = TIBRV_INVALID_ID;
    callback    = NULL;
    session     = NULL;
    arg         = NULL;
    subSubject  = NULL;
  };
    
  ~SubHandle(){
    tibrvEvent_Destroy( subId );
    tibrvEvent_Destroy( subId2 );
    if ( subSubject != NULL )
      FREE( subSubject );
  }; 
};

struct RaiSessionImpl {
  protected:
/*   friend class RaiPublish; */
/*   friend class RaiSession; */
/*   friend class RaiTimerImpl; */
/*   friend class RaiDict; */
/*   friend void RaiMainLoop( RaiSession * session ); */
/*   friend void RaiTimedDispatch(RaiSession *session, unsigned int interval ); */
  public:
  RaiSession     * me;
  RaiTransport     transport;

  tibrvTransport   rvT;
  tibrvQueue       rvQ;
  tibrvId          rvD, rvI, rvI2;
  tibrvDispatcher  rvDispatcher;
  bool             receivedSubject,
                   subscribedSubject;

  public:
  SYS_OPS( RaiSessionImpl );
  RaiSessionImpl( RaiSession * session, const char *svcname, const char *netname,
                  const char *dmnname )                         throw( RaiException );
  static void onMsg( tibrvId rvI, tibrvMsg msg, void * closure );
  SubHandle * AddSubscriber(RaiCallback callback, void *closure )      throw( RaiException );

  ~RaiSessionImpl();
};

#endif
