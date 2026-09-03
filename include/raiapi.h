/* Copyright (c) 2004 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_raiapi__raiapi_h__
#define __rai_raiapi__raiapi_h__

#if ! defined(SWIG)
#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_raiapi__raiapi_h[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */
#endif

#include <string.h>

#if ! defined(SWIG)
  #include "msg/rai_msg.h"
#else
  #include "java/msg/rai_msg.h"
#endif

#include "base/mem.h"
#include "base/time.h"
#include "base/sys.h"
#include "stream/io_stream.h"

/* predefine classes that are called in this headder */

#ifdef SetForm
/* Windows has a function SetForm defined */
#undef SetForm
#endif
enum IoctlParameter {
  SetPubType = 1,
  SetForm    = 2,
  SetPubSubject = 3
};

enum SubParameter {
  BOTH    = 0,
  UPDATE  = 1,
  SNAP    = 2
};

enum RaiTransport {
  RV7  = 1,
  RV5  = 2,
  CI   = 3,
  MD   = 4,
  TCP  = 5,
  HTTP = 6
};

enum RaiProto {
  RDP   = 1,
  MDRV  = 2,
  SASS  = 3,
  SASS2 = 4,
  SASS3 = 5
};

namespace Sass {
  enum MsgType {
    VERIFY    = 0,
    UPDATE    = 1,
    CORRECT   = 2,
    CLOSING   = 3,
    DROP      = 4,
    INITIAL   = 8,
    TRANSIENT = 9,
    SNAPSHOT  = 13
   };

  enum RecStatus {
    OK              = 0,
    BAD_NAME        = 1,
    BAD_LINE        = 2,
    BAD_ACCESS      = 6,
    EXPIRED         = 10,
    FEED_DOWN       = 12,
    NOT_FOUND       = 17,
    STALE_VALUE     = 18,
    FEED_UP         = 65,
    FEED_SWITCHOVER = 73,
    DATA_SUSPECT    = 74,
    RECAP           = 75

  };
};

enum logLevel {
  Major = 1,
  Minor = 2,
  Debug = 3,
  Trace = 4,
  Full  = 5
};

enum RaiDqm {
  SetHbInterval = 1,
  GetHbInterval = 2,
  SetStatus     = 3,
  GetStatus     = 4

};

class RaiSession;
class RaiEvent;

typedef void (* RaiCallback) (RaiEvent * event, RaiMsg * message, void * closure );
typedef void (* RaiTimerCallback) ( RaiSession * session, void * closure);

struct RaiTimerImpl;

class RaiTimer {
 protected:
  RaiTimerImpl	* timerImpl;
 public:
  RaiTimer( RaiSession * session, RaiTimerCallback callback,
            rai::TimeMSecs interval, void * closure )           throw( RaiException );
  rai::TimeMSecs  GetInterval( void )                           throw( RaiException );
  void SetInterval(rai::TimeMSecs  interval)                    throw( RaiException );
  virtual ~RaiTimer( void );
};

struct RaiDictImpl;

class RaiDict {
  RaiDictImpl    * dictImpl;
 public:
  bool haveDictionary;
  bool isDispatcher;
  RaiDict( void )                                             throw( RaiException );
  void Load( RaiSession *session, char *dictSubject )         throw( RaiException );
  virtual ~RaiDict( void );
};

struct RaiSessionImpl;

class RaiSession {
 public:
  SYS_OPS( RaiSession );

  RaiSessionImpl        * sessionImpl;

  RaiSession( const char *svcname, const char *netname,
	      const char *dmnname )                         throw( RaiException );
  RaiSession( const char *svcname, const char *netname,
	      const char *dmnname, bool createDispatcher )  throw( RaiException );
    
  void Destroy();
  virtual ~RaiSession();
};

class RaiEvent {
  public:
    RaiSession * session;
    const char * receivedSubject;
    const char * subscribedSubject;

    RaiEvent();
    virtual ~RaiEvent();
};

class RaiPublish {
 protected:
  RaiSession    * session;
  Rai_u16         type;
  Rai_u16         form;
  char          * subject;
  char          * typenam;
  char          * formname;
  unsigned int    seqNo;
  bool            isComplex;
    
 public:
  bool usesSass;

  RaiPublish( RaiSession *session, const char *subject, bool isComplex = false )
                                                          throw( RaiException );
  RaiPublish( RaiSession *session, bool isComplex = false )
                                                          throw( RaiException );

  void Publish( RaiMsg * raiMsg )			                    throw( RaiException );
    
  void Publish( const char * subject, RaiMsg * raiMsg )   throw( RaiException );

  void Publish( byte * buffer, unsigned int size )        throw( RaiException );

  void Publish( const char * subject, byte * buffer, 
		unsigned int size )			                              throw( RaiException );

  void Ioctl( IoctlParameter parameter, void * value)     throw( RaiException );

  void Destroy();

  virtual ~RaiPublish();
};

struct RaiEntImpl;

class RaiApi {
  protected:
    static int    DictTimeOutSeconds;
    static bool   haveDictionary;
    static bool   dictionaryLoadInProgress;
    static bool   entitleLoginInProgress;
    static int    logLevel;
    static RaiEntImpl  entitleImpl;

    friend class RaiPublish;
    friend class RaiSubscribe;
    friend class RaiDict;
    friend struct RaiDictImpl;
    friend struct RaiEntImpl;

  public:

    static void RaiOpen( RaiTransport transport, RaiProto protocol ) throw( RaiException );

    static void RaiClose( void )                          throw( RaiException );

    static void RaiMainloop( RaiSession * session )       throw( RaiException );

    static void RaiTimedDispatch( RaiSession *session, unsigned int interval ) 
								 throw( RaiException );

    static void RaiDispatch( RaiSession *session )         throw( RaiException );

    static RaiMsg * NewSASSMsg( Sass::MsgType MsgType, short RecType,
                 short SeqNo = 0, Sass::RecStatus RecStatus = Sass::OK )
                                                           throw( RaiException );

    static RaiMsg * NewSASSMsg( Sass::MsgType MsgType, const char * RecType,
                 short SeqNo = 0, Sass::RecStatus RecStatus = Sass::OK )
                                                           throw( RaiException );

    static RaiMsg * NewRaiMsg( Sass::MsgType MsgType, short RecType,
                 short SeqNo = 0, Sass::RecStatus RecStatus  = Sass::OK )
                                                           throw( RaiException );
  
    static RaiMsg * NewRaiMsg( Sass::MsgType MsgType, const char * RecType, 
                 short SeqNo = 0, Sass::RecStatus RecStatus = Sass::OK) 
                                                           throw( RaiException );

    static const char * RaiVersion( void );

    static void RaiLogin( RaiSession *session, char *userDetails );

    static void SetDictTimeoutSec( int sec );

    static int GetDictTimeoutSec( void );

    static bool GetDictLoadInProgress( void );

    static bool GetHaveDict( void );

    static void SetLogLevel( int level );

    static int GetLogLevel( void );
 
    static void RaiIoctl( IoctlParameter parameter, void * value)    throw( RaiException );

};

class RaiSubscribe {
 protected:
   void * handle;

 public:

  RaiSubscribe( RaiSession * session, const char * subject,
		RaiCallback callback, void * closure, SubParameter parm = BOTH ) 
							       throw( RaiException );
  void Cancel();
  virtual ~RaiSubscribe();

};

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
    NO_PERMISSION     = 10
  };
  RaiException getErr( unsigned int status );
};

#endif
