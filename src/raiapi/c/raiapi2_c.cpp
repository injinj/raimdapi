/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 *
 * Flat C binding of raiapi2 + RaiMsg, see include/raiapi2_c.h.  This mirrors
 * the Java binding (src/raiapi/java/com/rai/raiapi2/rai_api_jni.cpp and
 * raimsg/rai_msg_jni.cpp): every C++ exception is caught and returned as a
 * rai_err_t, callbacks are adapted to C function pointers.
 */
#define RAIAPI2_C_BUILD 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <signal.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#include "base/sys.h"
#include "base/mem.h"
#include "base/log.h"
#include "base/time.h"
#include "util/args.h"
#include "util/str_util.h"
#include "stream/io_stream.h"
#include "stream/stdio_stream.h"
#include "msg/msg.h"
#include "msg/field.h"
#include "msg/dict.h"
#include "msg/sass_const.h"
#include "raiapi2.h"
#include "raiapi2_c.h"

using namespace rai;

/* the error record is rai::ErrorRec, rai_err_t is a const pointer to it */
static inline rai_err_t toErr( RaiException e ) {
  return (rai_err_t) (const void *) e;
}
#define TRY_BEGIN  try {
#define TRY_END    } catch ( RaiException e ) { return toErr( e ); } \
                   return NULL;

static const ErrorRec c_err[] = {
  { 1, "Null handle", "RaiApiC" },
  { 2, "Field not found", "RaiApiC" },
  { 3, "Not an array field", "RaiApiC" },
  { 4, "Array index out of range", "RaiApiC" },
  { 5, "Bad argument", "RaiApiC" }
};
#define ERR_NULL   ( (rai_err_t) &c_err[ 0 ] )
#define ERR_NOTFND ( (rai_err_t) &c_err[ 1 ] )
#define ERR_NOTARR ( (rai_err_t) &c_err[ 2 ] )
#define ERR_RANGE  ( (rai_err_t) &c_err[ 3 ] )
#define ERR_BADARG ( (rai_err_t) &c_err[ 4 ] )

#define H( T, h ) ( (T *) (void *) (h) )
#define API( h )  H( RaiApi, h )
#define SESS( h ) H( RaiSession, h )
#define QUE( h )  H( RaiQueue, h )
#define SUB( h )  H( RaiSubscribe, h )
#define PUB( h )  H( RaiPublish, h )
#define IPUB( h ) H( RaiInteractivePublish, h )
#define TMR( h )  H( RaiTimer, h )
#define DICT( h ) H( RaiDict, h )
#define ENT( h )  H( RaiEntitlement, h )
#define MSG( h )  H( RaiMsg, h )
#define FLD( h )  H( RaiField, h )
template<class T> static inline T *ret( void *p ) { return (T *) p; }

extern "C" {

/* ---- errors ----------------------------------------------------------- */
uint32_t rai_err_status( rai_err_t e ) {
  return e == NULL ? 0 : ( (const ErrorRec *) e )->status;
}
const char * rai_err_reason( rai_err_t e ) {
  return e == NULL ? "ok" : ( (const ErrorRec *) e )->reason;
}
const char * rai_err_module( rai_err_t e ) {
  return e == NULL ? "" : ( (const ErrorRec *) e )->module;
}

} /* extern C, resumed below after the helper classes */

/* ---- output stream over a C write callback ---------------------------- */
struct CWriteStream : public OutputStream {
  rai_write_fn fn;
  void       * cl;
  CWriteStream( rai_write_fn f,  void *c )
    : OutputStream( 4096, false, false ), fn( f ), cl( c ) {}
  virtual ~CWriteStream() {}
  virtual unsigned int emptyBuf( const byte *buf,  unsigned int bufLen ) {
    if ( bufLen == 0 )
      return 0;
    uint32_t n = this->fn( this->cl, buf, bufLen );
    if ( n == 0 )
      throw IOStreamErr::getErr( IOStreamErr::BROKEN_PIPE );
    return n;
  }
};

/* ---- args with owned strings (Args keeps pointers) --------------------- */
struct CArgs : public Args {
  char         * vs,      /* version string copy */
               * ps;      /* prefix copy */
  CWriteStream * os;
  SYS_OPS( CArgs );
  CArgs() : vs( 0 ), ps( 0 ), os( 0 ) {}
  ~CArgs() {
    this->clear();
    if ( this->vs != NULL ) FREE( this->vs );
    if ( this->ps != NULL ) FREE( this->ps );
    if ( this->os != NULL ) delete this->os;
  }
};
static inline CArgs *ARGS( rai_args_t a ) { return (CArgs *) (void *) a; }

/* argv copy for RaiOpen / processArgs, which want char ** */
struct CArgv {
  int     argc;
  char ** argv;
  char  * buf;
  CArgv( int c,  const char **v ) : argc( c ), argv( 0 ), buf( 0 ) {
    size_t len = 0;
    int i;
    for ( i = 0; i < c; i++ )
      len += ( v[ i ] ? ::strlen( v[ i ] ) : 0 ) + 1;
    this->argv = (char **) ::malloc( sizeof( char * ) * ( c + 1 ) );
    this->buf  = (char *) ::malloc( len + 1 );
    char *p = this->buf;
    for ( i = 0; i < c; i++ ) {
      this->argv[ i ] = p;
      const char *s = v[ i ] ? v[ i ] : "";
      size_t n = ::strlen( s ) + 1;
      ::memcpy( p, s, n );
      p += n;
    }
    this->argv[ c ] = NULL;
  }
  ~CArgv() { ::free( this->argv ); ::free( this->buf ); }
};

/* ---- callback adapters ------------------------------------------------- */
struct CCallback {
  virtual ~CCallback() {}
};

struct CMsgCallback : public RaiMsgCallback, public CCallback {
  rai_msg_fn fn;
  void     * cl;
  SYS_OPS( CMsgCallback );
  CMsgCallback( rai_msg_fn f,  void *c ) : fn( f ), cl( c ) {}
  virtual ~CMsgCallback() {}
  virtual void onMsg( RaiMsgEvent &ev,  RaiMsg &m,  void * ) {
    rai_msg_event_t e;
    e.subscribe   = ret<rai_subscribe_s>( &ev.subscribe );
    e.subject     = ev.subject;
    e.type        = (int) ev.type;
    e.msg_type    = ev.msgType;
    e.rec_status  = ev.recStatus;
    e.old_state   = (int) ev.oldState;
    e.recv        = (int) ev.recv;
    e.state       = (int) ev.state;
    e.pub_time    = (int64_t) ev.pubTime;
    e.route_time  = (int64_t) ev.routeTime;
    e.counter     = ev.counter;
    e.old_counter = ev.oldCounter;
    this->fn( this->cl, &e, ret<rai_msg_s>( &m ) );
  }
};

struct CTimerCallback : public RaiTimerCallback, public CCallback {
  rai_timer_fn fn;
  void       * cl;
  SYS_OPS( CTimerCallback );
  CTimerCallback( rai_timer_fn f,  void *c ) : fn( f ), cl( c ) {}
  virtual ~CTimerCallback() {}
  virtual void onTimer( RaiTimer &t,  void * ) {
    this->fn( this->cl, ret<rai_timer_s>( &t ) );
  }
};

struct CSubscribeCallback : public RaiSubscribeCallback, public CCallback {
  rai_subscribe_fn fn;
  void           * cl;
  SYS_OPS( CSubscribeCallback );
  CSubscribeCallback( rai_subscribe_fn f,  void *c ) : fn( f ), cl( c ) {}
  virtual ~CSubscribeCallback() {}
  virtual void onSubscribe( RaiSubscribeEvent &ev,  RaiMsg &m,  void * ) {
    rai_subscribe_event_t e;
    e.publish     = ret<rai_ipublish_s>( &ev.publish );
    e.subject     = ev.subject;
    e.reply       = ev.reply;
    e.query_flags = ev.queryFlags;
    this->fn( this->cl, &e, ret<rai_msg_s>( &m ) );
  }
};

struct CDataLossCallback : public RaiDataLossCallback, public CCallback {
  rai_dataloss_fn   loss_fn;
  rai_connection_fn conn_fn;
  void            * cl;
  SYS_OPS( CDataLossCallback );
  CDataLossCallback( rai_dataloss_fn l,  rai_connection_fn c,  void *x )
    : loss_fn( l ), conn_fn( c ), cl( x ) {}
  virtual ~CDataLossCallback() {}
  virtual void onDataLoss( RaiDataLossEvent &ev,  void * ) {
    if ( this->loss_fn == NULL )
      return;
    rai_dataloss_event_t e;
    e.session              = ret<rai_session_s>( &ev.session );
    e.transport_name       = ev.transportName;
    e.description          = ev.description;
    e.inbound_packet_loss  = ev.inboundPacketLoss;
    e.outbound_packet_loss = ev.outboundPacketLoss;
    e.connection_count     = ev.connectionCount;
    e.connection_loss      = ev.connectionLoss ? 1 : 0;
    e.is_multicast         = ev.isMulticast ? 1 : 0;
    this->loss_fn( this->cl, &e );
  }
  virtual void onConnection( RaiConnectionEvent &ev,  void * ) {
    if ( this->conn_fn == NULL )
      return;
    rai_connection_event_t e;
    e.session             = ret<rai_session_s>( &ev.session );
    e.transport_name      = ev.transportName;
    e.description         = ev.description;
    e.connection_count    = ev.connectionCount;
    e.connection_oriented = ev.connectionOriented ? 1 : 0;
    e.is_multicast        = ev.isMulticast ? 1 : 0;
    this->conn_fn( this->cl, &e );
  }
};

/* ---- signal handler ---------------------------------------------------- */
static rai_signal_fn sig_fn = NULL;
static void        * sig_cl = NULL;
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
static void c_signal_handler( int sig ) {
  if ( sig_fn != NULL )
    sig_fn( sig, sig_cl );
}
#else
static BOOL WINAPI c_ctrl_handler( DWORD type ) {
  if ( sig_fn != NULL )
    sig_fn( type == CTRL_C_EVENT ? 2 : 15, sig_cl );
  return TRUE;
}
#endif

extern "C" {

/* ---- RaiApi ----------------------------------------------------------- */
rai_err_t
rai_api_open( const char *api_name,  int argc,  const char **argv,
              rai_api_t *api )
{
  *api = NULL;
  TRY_BEGIN
    Sys::initialize();
    CArgv av( argc, argv );
    *api = ret<rai_api_s>( RaiApi::RaiOpen( api_name, av.argc, av.argv ) );
  TRY_END
}
void rai_api_close( rai_api_t api ) {
  if ( api != NULL ) API( api )->Close();
}
void rai_api_delete( rai_api_t api ) {
  if ( api != NULL ) delete API( api );
}
const char * rai_api_name( rai_api_t api ) {
  return api == NULL ? NULL : API( api )->GetApiName();
}
const char * rai_api_version( void ) {
  return RaiApi::RaiVersion();
}
rai_err_t rai_api_get_args( rai_api_t api,  rai_args_t args ) {
  if ( api == NULL || args == NULL ) return ERR_NULL;
  TRY_BEGIN API( api )->GetArgs( *ARGS( args ) ); TRY_END
}
rai_err_t rai_api_get_dict_args( rai_args_t args ) {
  if ( args == NULL ) return ERR_NULL;
  TRY_BEGIN RaiApi::GetDictArgs( *ARGS( args ) ); TRY_END
}
rai_err_t rai_api_parse_args( rai_api_t api,  rai_args_t args ) {
  if ( api == NULL || args == NULL ) return ERR_NULL;
  TRY_BEGIN API( api )->ParseArgs( *ARGS( args ) ); TRY_END
}
rai_err_t rai_api_open_log_args( rai_args_t args,  int *opened ) {
  if ( args == NULL ) return ERR_NULL;
  *opened = 0;
  TRY_BEGIN *opened = RaiApi::OpenLog( *ARGS( args ) ) ? 1 : 0; TRY_END
}
rai_err_t rai_api_open_log( const char *name,  int level,  int verb ) {
  TRY_BEGIN
    Sys::initialize();
    /* same as RaiApi::OpenLog( Args ) does after parsing -log etc */
    Log::openLog( name, (Log::LogLevel) level, (unsigned int) verb );
  TRY_END
}
void
rai_api_print_log( rai_api_t api,  int level,  rai_err_t err,
                   const char *where,  int line,  const char *s )
{
  if ( api == NULL ) {
    rai_api_log( level, err, where, line, s );
    return;
  }
  API( api )->PrintLog( (Log::LogLevel) level, where ? where : __FILE__,
                        where ? line : __LINE__, (RaiException) err, "%s",
                        s ? s : "" );
}
void
rai_api_log( int level,  rai_err_t err,  const char *where,  int line,
             const char *s )
{
  Log::printLog( (Log::LogLevel) level, where ? where : __FILE__,
                 where ? line : __LINE__, (RaiException) err, "%s",
                 s ? s : "" );
}
rai_err_t rai_api_open_dict( rai_args_t args,  int *loaded ) {
  if ( args == NULL ) return ERR_NULL;
  *loaded = 0;
  TRY_BEGIN *loaded = RaiApi::OpenDict( *ARGS( args ) ) ? 1 : 0; TRY_END
}
rai_err_t rai_api_create_session( rai_api_t api,  rai_session_t *session ) {
  *session = NULL;
  if ( api == NULL ) return ERR_NULL;
  TRY_BEGIN
    *session = ret<rai_session_s>( API( api )->CreateSession() );
  TRY_END
}
rai_err_t
rai_api_new_msg( int proto,  uint16_t msg_type,  uint16_t rec_type,
                 uint16_t seqno,  uint16_t rec_status,  rai_msg_t *m )
{
  *m = NULL;
  TRY_BEGIN
    *m = ret<rai_msg_s>( RaiApi::NewRaiMsg( (RaiMsg_protocol) proto, msg_type,
                                            rec_type, seqno, rec_status ) );
  TRY_END
}
rai_err_t
rai_api_new_msg_form( int proto,  uint16_t msg_type,  const char *form_type,
                      uint16_t seqno,  uint16_t rec_status,  rai_msg_t *m )
{
  *m = NULL;
  TRY_BEGIN
    *m = ret<rai_msg_s>( RaiApi::NewRaiMsg( (RaiMsg_protocol) proto, msg_type,
                                            form_type, seqno, rec_status ) );
  TRY_END
}
int rai_api_set_ioctl( rai_api_t api,  const char *parm,  const char *value ) {
  if ( api == NULL ) return 0;
  return API( api )->SetIoctl( parm, value ) ? 1 : 0;
}
rai_err_t rai_api_register_sig_handler( rai_signal_fn fn,  void *closure ) {
  TRY_BEGIN
    Sys::initialize();
    sig_fn = fn;
    sig_cl = closure;
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
    struct sigaction nsa;
    ::memset( &nsa, 0, sizeof( nsa ) );
    ::sigemptyset( &nsa.sa_mask );
    nsa.sa_handler = c_signal_handler;
    ::sigaction( SIGHUP, &nsa, NULL );
    ::sigaction( SIGINT, &nsa, NULL );
    ::sigaction( SIGTERM, &nsa, NULL );
#else
    ::SetConsoleCtrlHandler( c_ctrl_handler, TRUE );
#endif
  TRY_END
}

/* ---- RaiSession ------------------------------------------------------- */
rai_err_t rai_session_start( rai_session_t s ) {
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN SESS( s )->Start(); TRY_END
}
rai_err_t rai_session_create_queue( rai_session_t s,  int direct,
                                    rai_queue_t *q ) {
  *q = NULL;
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN *q = ret<rai_queue_s>( SESS( s )->CreateQueue( direct != 0 ) );
  TRY_END
}
rai_err_t rai_session_create_publish( rai_session_t s,  int auto_inc,
                                      rai_publish_t *p ) {
  *p = NULL;
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN
    *p = ret<rai_publish_s>( SESS( s )->CreatePublish( auto_inc != 0 ) );
  TRY_END
}
rai_err_t rai_session_create_dict( rai_session_t s,  rai_dict_t *d ) {
  *d = NULL;
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN *d = ret<rai_dict_s>( SESS( s )->CreateDict() ); TRY_END
}
rai_err_t rai_session_destroy( rai_session_t s ) {
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN SESS( s )->Destroy(); TRY_END
}
rai_err_t rai_session_login( rai_session_t s,  const char *user,
                             rai_entitle_t *e ) {
  *e = NULL;
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN *e = ret<rai_entitle_s>( SESS( s )->Login( user ) ); TRY_END
}
rai_err_t
rai_session_set_dataloss_cb( rai_session_t s,  rai_dataloss_fn loss_fn,
                             rai_connection_fn conn_fn,  void *closure,
                             rai_callback_t *cb )
{
  *cb = NULL;
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN
    CDataLossCallback *c = new CDataLossCallback( loss_fn, conn_fn, closure );
    SESS( s )->SetDataLossCB( c, NULL );
    *cb = ret<rai_callback_s>( (CCallback *) c );
  TRY_END
}
rai_err_t rai_session_notify_status( rai_session_t s,  uint16_t msg_type,
                                     uint16_t rec_status ) {
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN SESS( s )->NotifyStatus( msg_type, rec_status ); TRY_END
}
rai_api_t rai_session_get_api( rai_session_t s ) {
  return s == NULL ? NULL : ret<rai_api_s>( SESS( s )->GetApi() );
}
rai_err_t rai_session_set_name( rai_session_t s,  const char *name ) {
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN SESS( s )->setSessionName( name ); TRY_END
}
const char * rai_session_get_name( rai_session_t s ) {
  return s == NULL ? NULL : SESS( s )->getSessionName();
}
void rai_callback_delete( rai_callback_t cb ) {
  if ( cb != NULL ) delete (CCallback *) (void *) cb;
}

/* ---- RaiQueue --------------------------------------------------------- */
rai_err_t
rai_queue_create_subscribe( rai_queue_t q,  rai_msg_fn fn,  void *closure,
                            rai_subscribe_t *sub,  rai_callback_t *cb )
{
  *sub = NULL; *cb = NULL;
  if ( q == NULL || fn == NULL ) return ERR_NULL;
  TRY_BEGIN
    CMsgCallback *c = new CMsgCallback( fn, closure );
    RaiSubscribe *s = QUE( q )->CreateSubscribe( c, NULL );
    if ( s == NULL ) { delete c; return ERR_NULL; }
    *sub = ret<rai_subscribe_s>( s );
    *cb  = ret<rai_callback_s>( (CCallback *) c );
  TRY_END
}
rai_err_t
rai_queue_create_timer( rai_queue_t q,  rai_timer_fn fn,  void *closure,
                        rai_timer_t *t,  rai_callback_t *cb )
{
  *t = NULL; *cb = NULL;
  if ( q == NULL || fn == NULL ) return ERR_NULL;
  TRY_BEGIN
    CTimerCallback *c = new CTimerCallback( fn, closure );
    RaiTimer *tm = QUE( q )->CreateTimer( c, NULL );
    if ( tm == NULL ) { delete c; return ERR_NULL; }
    *t  = ret<rai_timer_s>( tm );
    *cb = ret<rai_callback_s>( (CCallback *) c );
  TRY_END
}
rai_err_t
rai_queue_create_ipublish( rai_queue_t q,  rai_subscribe_fn fn,  void *closure,
                           rai_ipublish_t *p,  rai_callback_t *cb )
{
  *p = NULL; *cb = NULL;
  if ( q == NULL || fn == NULL ) return ERR_NULL;
  TRY_BEGIN
    CSubscribeCallback *c = new CSubscribeCallback( fn, closure );
    RaiInteractivePublish *ip = QUE( q )->CreateInteractivePublish( c, NULL );
    if ( ip == NULL ) { delete c; return ERR_NULL; }
    *p  = ret<rai_ipublish_s>( ip );
    *cb = ret<rai_callback_s>( (CCallback *) c );
  TRY_END
}
rai_err_t rai_queue_notify_status( rai_queue_t q,  uint16_t msg_type,
                                   uint16_t rec_status ) {
  if ( q == NULL ) return ERR_NULL;
  TRY_BEGIN QUE( q )->NotifyStatus( msg_type, rec_status ); TRY_END
}
rai_err_t rai_queue_mainloop( rai_queue_t q ) {
  if ( q == NULL ) return ERR_NULL;
  TRY_BEGIN QUE( q )->Mainloop(); TRY_END
}
rai_err_t rai_queue_timed_dispatch( rai_queue_t q,  uint32_t ival_ms ) {
  if ( q == NULL ) return ERR_NULL;
  TRY_BEGIN QUE( q )->TimedDispatch( ival_ms ); TRY_END
}
rai_err_t rai_queue_dispatch( rai_queue_t q ) {
  if ( q == NULL ) return ERR_NULL;
  TRY_BEGIN QUE( q )->Dispatch(); TRY_END
}
uint32_t rai_queue_get_depth( rai_queue_t q ) {
  return q == NULL ? 0 : QUE( q )->GetDepth();
}
rai_err_t rai_queue_destroy( rai_queue_t q ) {
  if ( q == NULL ) return ERR_NULL;
  TRY_BEGIN QUE( q )->Destroy(); TRY_END
}
rai_session_t rai_queue_get_session( rai_queue_t q ) {
  return q == NULL ? NULL : ret<rai_session_s>( QUE( q )->GetSession() );
}

/* ---- RaiSubscribe ----------------------------------------------------- */
rai_err_t rai_subscribe_start( rai_subscribe_t s,  const char *subject,
                               int parm,  uint32_t timeout_ms ) {
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN
    SUB( s )->Start( subject, (RaiSubscribe::RaiSubParameter) parm,
                     timeout_ms );
  TRY_END
}
rai_err_t rai_subscribe_cancel( rai_subscribe_t s ) {
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN SUB( s )->Cancel(); TRY_END
}
rai_err_t rai_subscribe_refresh( rai_subscribe_t s,  uint32_t timeout_ms ) {
  if ( s == NULL ) return ERR_NULL;
  TRY_BEGIN SUB( s )->Refresh( timeout_ms ); TRY_END
}
const char * rai_subscribe_subject( rai_subscribe_t s ) {
  return s == NULL ? NULL : SUB( s )->Subject();
}
int rai_subscribe_in_progress( rai_subscribe_t s ) {
  return s == NULL ? 0 : ( SUB( s )->InProgress() ? 1 : 0 );
}
rai_queue_t rai_subscribe_get_queue( rai_subscribe_t s ) {
  return s == NULL ? NULL : ret<rai_queue_s>( SUB( s )->GetQueue() );
}
const char * rai_subscribe_state_to_string( int state ) {
  return RaiSubscribe::StateToString( (RaiSubscribe::RaiSubState) state );
}

/* ---- RaiPublish ------------------------------------------------------- */
rai_err_t rai_publish_msg( rai_publish_t p,  const char *subject,  rai_msg_t m,
                           int64_t stamp ) {
  if ( p == NULL || m == NULL ) return ERR_NULL;
  TRY_BEGIN PUB( p )->Publish( subject, *MSG( m ), (TimeNSecs) stamp ); TRY_END
}
rai_err_t rai_publish_buf( rai_publish_t p,  const char *subject,
                           const void *buf,  uint32_t len,  int64_t stamp ) {
  if ( p == NULL ) return ERR_NULL;
  TRY_BEGIN PUB( p )->Publish( subject, buf, len, (TimeNSecs) stamp ); TRY_END
}
rai_err_t rai_publish_set_prefix( rai_publish_t p,  const char *prefix ) {
  if ( p == NULL ) return ERR_NULL;
  TRY_BEGIN PUB( p )->SetPrefix( prefix ); TRY_END
}
const char * rai_publish_get_prefix( rai_publish_t p ) {
  return p == NULL ? NULL : PUB( p )->GetPrefix();
}
uint32_t rai_publish_get_seqno( rai_publish_t p ) {
  return p == NULL ? 0 : PUB( p )->GetSeqno();
}
void rai_publish_set_seqno( rai_publish_t p,  uint32_t n ) {
  if ( p != NULL ) PUB( p )->SetSeqno( n );
}
rai_err_t rai_publish_destroy( rai_publish_t p ) {
  if ( p == NULL ) return ERR_NULL;
  TRY_BEGIN PUB( p )->Destroy(); TRY_END
}
rai_session_t rai_publish_get_session( rai_publish_t p ) {
  return p == NULL ? NULL : ret<rai_session_s>( PUB( p )->GetSession() );
}
rai_publish_t rai_ipublish_publish( rai_ipublish_t p ) {
  return p == NULL ? NULL : ret<rai_publish_s>( (RaiPublish *) IPUB( p ) );
}
rai_err_t rai_ipublish_start( rai_ipublish_t p,  const char *subject ) {
  if ( p == NULL ) return ERR_NULL;
  TRY_BEGIN IPUB( p )->InteractiveStart( subject ); TRY_END
}
rai_err_t rai_ipublish_cancel( rai_ipublish_t p ) {
  if ( p == NULL ) return ERR_NULL;
  TRY_BEGIN IPUB( p )->InteractiveCancel(); TRY_END
}
int rai_ipublish_in_progress( rai_ipublish_t p ) {
  return p == NULL ? 0 : ( IPUB( p )->InProgress() ? 1 : 0 );
}
rai_queue_t rai_ipublish_get_queue( rai_ipublish_t p ) {
  return p == NULL ? NULL : ret<rai_queue_s>( IPUB( p )->GetQueue() );
}

/* ---- RaiTimer --------------------------------------------------------- */
rai_err_t rai_timer_start( rai_timer_t t ) {
  if ( t == NULL ) return ERR_NULL;
  TRY_BEGIN TMR( t )->Start(); TRY_END
}
void rai_timer_stop( rai_timer_t t ) {
  if ( t != NULL ) TMR( t )->Stop();
}
int64_t rai_timer_get_interval( rai_timer_t t ) {
  return t == NULL ? 0 : (int64_t) TMR( t )->GetInterval();
}
void rai_timer_set_interval( rai_timer_t t,  int64_t ms ) {
  if ( t != NULL ) TMR( t )->SetInterval( (TimeMSecs) ms );
}
rai_queue_t rai_timer_get_queue( rai_timer_t t ) {
  return t == NULL ? NULL : ret<rai_queue_s>( TMR( t )->GetQueue() );
}

/* ---- RaiDict ---------------------------------------------------------- */
rai_err_t rai_dict_load( rai_dict_t d,  uint32_t timeout_secs,
                         const char *dict_subject,  int load_wait ) {
  if ( d == NULL ) return ERR_NULL;
  TRY_BEGIN DICT( d )->Load( timeout_secs, dict_subject, load_wait != 0 );
  TRY_END
}
int rai_dict_have_dict( rai_dict_t d ) {
  return d == NULL ? 0 : ( DICT( d )->HaveDict() ? 1 : 0 );
}
int rai_dict_in_progress( rai_dict_t d ) {
  return d == NULL ? 0 : ( DICT( d )->InProgress() ? 1 : 0 );
}
rai_session_t rai_dict_get_session( rai_dict_t d ) {
  return d == NULL ? NULL : ret<rai_session_s>( DICT( d )->GetSession() );
}
void rai_entitle_delete( rai_entitle_t e ) {
  if ( e != NULL ) ENT( e )->Destroy();
}

/* ---- Args ------------------------------------------------------------- */
rai_args_t rai_args_create( void ) {
  try {
    Sys::initialize();
    return ret<rai_args_s>( new CArgs() );
  } catch ( RaiException ) {
    return NULL;
  }
}
void rai_args_delete( rai_args_t a ) {
  if ( a != NULL ) delete ARGS( a );
}
rai_err_t
rai_args_add_string( rai_args_t a,  const char *name,  const char *def,
                     const char *example,  const char *descr,  int flags )
{
  if ( a == NULL || name == NULL ) return ERR_NULL;
  TRY_BEGIN
    StringArg sa( name, def, example, descr );
    ARGS( a )->copy( sa, (unsigned int) flags ); /* copies the strings */
  TRY_END
}
rai_err_t
rai_args_add_bool( rai_args_t a,  const char *name,  int def,
                   const char *example,  const char *descr,  int flags )
{
  if ( a == NULL || name == NULL ) return ERR_NULL;
  TRY_BEGIN
    BoolArg ba( name, def != 0, example, descr );
    ARGS( a )->copy( ba, (unsigned int) flags );
  TRY_END
}
rai_err_t
rai_args_add_int( rai_args_t a,  const char *name,  uint32_t def,
                  const char *example,  const char *descr,  int flags )
{
  if ( a == NULL || name == NULL ) return ERR_NULL;
  TRY_BEGIN
    UIntArg ia( name, def, example, descr );
    ARGS( a )->copy( ia, (unsigned int) flags );
  TRY_END
}
rai_err_t
rai_args_add_double( rai_args_t a,  const char *name,  double def,
                     const char *example,  const char *descr,  int flags )
{
  if ( a == NULL || name == NULL ) return ERR_NULL;
  TRY_BEGIN
    DoubleArg da( name, def, example, descr );
    ARGS( a )->copy( da, (unsigned int) flags );
  TRY_END
}
rai_err_t
rai_args_add_defaults( rai_args_t a,  const char *vers,  const char *prefix,
                       rai_write_fn out_fn,  void *out_cl,  const char *argv0 )
{
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN
    CArgs *ca = ARGS( a );
    if ( vers != NULL && ca->vs == NULL )
      STRDUP( ca->vs, vers );
    if ( prefix != NULL && ca->ps == NULL )
      STRDUP( ca->ps, prefix );
    if ( out_fn != NULL && ca->os == NULL )
      ca->os = new CWriteStream( out_fn, out_cl );
    ca->addDefaults( ca->vs, ca->ps, ca->os, argv0 );
  TRY_END
}
rai_err_t rai_args_process( rai_args_t a,  int argc,  const char **argv,
                            int *ok ) {
  *ok = 0;
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN
    CArgv av( argc, argv );
    *ok = ARGS( a )->processArgs( (unsigned int) av.argc, av.argv ) ? 1 : 0;
    if ( ARGS( a )->os != NULL )
      ARGS( a )->os->flush();
  TRY_END
}
uint32_t rai_args_num_values( rai_args_t a,  const char *n ) {
  return a == NULL ? 0 : ARGS( a )->getNumValues( n );
}
rai_err_t rai_args_get_string( rai_args_t a,  const char *n,  uint32_t i,
                               const char **val ) {
  *val = NULL;
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN *val = ARGS( a )->getString( n, i ); TRY_END
}
rai_err_t rai_args_get_bool( rai_args_t a,  const char *n,  uint32_t i,
                             int *val ) {
  *val = 0;
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN *val = ARGS( a )->getBoolean( n, i ) ? 1 : 0; TRY_END
}
rai_err_t rai_args_get_int( rai_args_t a,  const char *n,  uint32_t i,
                            uint32_t *val ) {
  *val = 0;
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN *val = ARGS( a )->getUInt( n, i ); TRY_END
}
rai_err_t rai_args_get_double( rai_args_t a,  const char *n,  uint32_t i,
                               double *val ) {
  *val = 0;
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN *val = ARGS( a )->getDouble( n, i ); TRY_END
}
rai_err_t rai_args_set_string( rai_args_t a,  const char *n,  const char *v ) {
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN ARGS( a )->setString( n, v ); TRY_END
}
rai_err_t rai_args_set_bool( rai_args_t a,  const char *n,  int v ) {
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN ARGS( a )->setBoolean( n, v != 0 ); TRY_END
}
rai_err_t rai_args_set_int( rai_args_t a,  const char *n,  uint32_t v ) {
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN ARGS( a )->setUInt( n, v ); TRY_END
}
rai_err_t rai_args_set_double( rai_args_t a,  const char *n,  double v ) {
  if ( a == NULL ) return ERR_NULL;
  TRY_BEGIN ARGS( a )->setDouble( n, v ); TRY_END
}
int rai_args_is_set( rai_args_t a,  const char *n ) {
  return a == NULL ? 0 : ( ARGS( a )->isSet( n ) ? 1 : 0 );
}

/* ---- Time ------------------------------------------------------------- */
int64_t rai_time_current_ns( void ) {
  return (int64_t) Time::currentTimeNanosecs();
}
int64_t rai_time_hires_ns( void ) {
  /* same normalization as the java binding: hires ticks -> nanoseconds */
  double    cpms;
  TimeHires h = Time::getHiresTime( &cpms );
  if ( cpms == 1000000.0 )
    return (int64_t) h;
  if ( cpms > 1000000.0 )
    return (int64_t) ( (double) h / ( cpms / 1000000.0 ) );
  return (int64_t) ( (double) h * ( 1000000.0 / cpms ) );
}
int64_t rai_time_hires_to_ns( int64_t h ) {
  double cpms = Time::getCyclesPerMSec();
  if ( cpms == 1000000.0 )
    return (int64_t) Time::hiresToNanosecs( (TimeHires) h );
  if ( cpms > 1000000.0 )
    return (int64_t) Time::hiresToNanosecs(
      (TimeHires) ( (double) h * ( cpms / 1000000.0 ) ) );
  return (int64_t) Time::hiresToNanosecs(
    (TimeHires) ( (double) h / ( 1000000.0 / cpms ) ) );
}
const char * rai_time_ns_timestamp( int64_t ns,  int precision,  char *buf,
                                    uint32_t len ) {
  TimeNSecs n = (TimeNSecs) ns;
  if ( n == 0 )
    n = Time::currentTimeNanosecs();
  return Time::timestamp( n, (unsigned int) precision, buf, len );
}
const char * rai_time_ns_interval( int64_t ns,  char *buf,  uint32_t len ) {
  return StrUtil::intToString( (llong) ns, buf, len, U_NANOSECS );
}
const char * rai_time_strftime( int tz,  int64_t ms,  const char *fmt,
                                char *buf,  uint32_t len ) {
  return Time::strftime( tz, (TimeMSecs) ms, fmt, buf, len );
}
int rai_time_rotate_set_time( rai_time_rotate_t *r,  const char *spec,
                              int rot_dow,  int64_t rot_time ) {
  TimeRotate t;
  t.time = (TimeMSecs) r->time; t.period = (TimeMSecs) r->period;
  t.lastTime = (TimeMSecs) r->last_time;
  t.dayOrWeek = (TimeRotate::DayOrWeek) r->day_or_week;
  bool res = t.setRotateTime( spec, (TimeRotate::DayOrWeek) rot_dow,
                              (TimeMSecs) rot_time );
  r->time = t.time; r->period = t.period; r->last_time = t.lastTime;
  r->day_or_week = (int) t.dayOrWeek;
  return res ? 1 : 0;
}
int rai_time_rotate_set_period( rai_time_rotate_t *r,  const char *spec,
                                int64_t rot_period ) {
  TimeRotate t;
  t.time = (TimeMSecs) r->time; t.period = (TimeMSecs) r->period;
  t.lastTime = (TimeMSecs) r->last_time;
  t.dayOrWeek = (TimeRotate::DayOrWeek) r->day_or_week;
  bool res = t.setRotatePeriod( spec, (TimeMSecs) rot_period );
  r->time = t.time; r->period = t.period; r->last_time = t.lastTime;
  r->day_or_week = (int) t.dayOrWeek;
  return res ? 1 : 0;
}

/* ---- RaiMsg ----------------------------------------------------------- */
const char * rai_msg_version( void ) {
  return RaiApi::RaiVersion();
}
rai_err_t rai_msg_create( int proto,  rai_msg_t *m ) {
  *m = NULL;
  TRY_BEGIN
    Sys::initialize();
    *m = ret<rai_msg_s>( proto < 0 ? new RaiMsg()
                                   : new RaiMsg( (RaiMsg_protocol) proto ) );
  TRY_END
}
void rai_msg_delete( rai_msg_t m ) {
  if ( m != NULL ) delete MSG( m );
}
rai_err_t rai_msg_reuse( rai_msg_t m,  int proto ) {
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN
    if ( proto < 0 ) MSG( m )->ReUse();
    else MSG( m )->ReUse( (RaiMsg_protocol) proto );
  TRY_END
}
void rai_msg_set_protocol( rai_msg_t m,  int proto ) {
  if ( m != NULL ) MSG( m )->SetProtocol( (RaiMsg_protocol) proto );
}
int rai_msg_get_protocol( rai_msg_t m ) {
  return m == NULL ? -1 : (int) MSG( m )->GetProtocol();
}
const char * rai_msg_get_protocol_string( rai_msg_t m ) {
  return m == NULL ? NULL : MSG( m )->GetProtocolString();
}
rai_err_t rai_msg_clear_form( rai_msg_t m ) {
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->ClearForm( NULL ); TRY_END
}
rai_err_t rai_msg_release( rai_msg_t m ) {
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->Release(); TRY_END
}
rai_err_t rai_msg_copy( rai_msg_t m,  rai_msg_t from ) {
  if ( m == NULL || from == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->Copy( MSG( from ) ); TRY_END
}
const char * rai_msg_type_to_string( uint16_t msg_type,  char *buf,
                                     uint32_t len ) {
  if ( len < 16 ) return NULL;
  return SassConst::msgTypeToString( msg_type, buf );
}
uint16_t rai_msg_string_to_type( const char *s ) {
  return SassConst::stringToMsgType( s );
}
const char * rai_rec_status_to_string( uint16_t rec_status,  char *buf,
                                       uint32_t len ) {
  if ( len < 16 ) return NULL;
  return SassConst::recStatusToString( rec_status, buf );
}
uint16_t rai_string_to_rec_status( const char *s ) {
  return SassConst::stringToRecStatus( s );
}
rai_err_t rai_msg_rec_type_to_string( uint16_t rec_type,  const char **s ) {
  *s = NULL;
  const RaiMsg_form *form;
  if ( DataDictionary == NULL )
    return toErr( RaiMsgErr::getErr( RaiMsgErr::NO_DICTIONARY ) );
  if ( (form = DataDictionary->getForm( rec_type )) == NULL )
    return toErr( RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS ) );
  *s = form->entry->fname;
  return NULL;
}
rai_err_t rai_msg_string_to_rec_type( const char *s,  uint16_t *rec_type ) {
  *rec_type = 0;
  const RaiMsg_form *form;
  if ( s == NULL )
    return toErr( RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND ) );
  if ( DataDictionary == NULL )
    return toErr( RaiMsgErr::getErr( RaiMsgErr::NO_DICTIONARY ) );
  if ( (form = DataDictionary->getForm( s )) == NULL )
    return toErr( RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS ) );
  *rec_type = form->entry->fid;
  return NULL;
}
/* MSG_TYPE / REC_TYPE / REC_STATUS header fields as strings, like the java
 * Get{MsgType,RecType,RecStatus}String() */
rai_err_t rai_msg_get_hdr_string( rai_msg_t m,  const char *fname,
                                  const char **s ) {
  static char buf[ 4 ][ 32 ]; /* rotating, small strings */
  static unsigned int bi = 0;
  *s = NULL;
  if ( m == NULL || fname == NULL ) return ERR_NULL;
  TRY_BEGIN
    RaiField field;
    if ( ! MSG( m )->Get( fname, field ) )
      return toErr( RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND ) );
    if ( field.Type() == RAIMSG_STRING ) {
      const char *str;
      field.Get( str );
      *s = str;
      return NULL;
    }
    Rai_u16 v;
    field.Get( v );
    char *b = buf[ bi++ % 4 ];
    if ( ::strcmp( fname, "MSG_TYPE" ) == 0 )
      *s = SassConst::msgTypeToString( v, b );
    else if ( ::strcmp( fname, "REC_STATUS" ) == 0 )
      *s = SassConst::recStatusToString( v, b );
    else {
      const RaiMsg_form *form;
      if ( DataDictionary != NULL &&
           (form = DataDictionary->getForm( v )) != NULL )
        *s = form->entry->fname;
      else {
        ::snprintf( b, 32, "%u", (unsigned int) v );
        *s = b;
      }
    }
  TRY_END
}
rai_err_t rai_msg_set_hdr_string( rai_msg_t m,  const char *fname,
                                  const char *s ) {
  if ( m == NULL || fname == NULL || s == NULL ) return ERR_NULL;
  TRY_BEGIN
    if ( ::strcmp( fname, "MSG_TYPE" ) == 0 )
      MSG( m )->Update( fname, SassConst::stringToMsgType( s ) );
    else if ( ::strcmp( fname, "REC_STATUS" ) == 0 )
      MSG( m )->Update( fname, SassConst::stringToRecStatus( s ) );
    else {
      const RaiMsg_form *form;
      if ( DataDictionary != NULL &&
           (form = DataDictionary->getForm( s )) != NULL )
        MSG( m )->Update( fname, form->entry->fid );
      else
        MSG( m )->Update( fname, s );
    }
  TRY_END
}
rai_err_t rai_msg_get_field( rai_msg_t m,  const char *name,  rai_field_t fld,
                             int *found ) {
  *found = 0;
  if ( m == NULL || fld == NULL ) return ERR_NULL;
  TRY_BEGIN *found = MSG( m )->Get( name, *FLD( fld ) ) ? 1 : 0; TRY_END
}

#define MSG_GET( FN, CT, RT ) \
rai_err_t FN( rai_msg_t m,  const char *n,  CT *v ) { \
  *v = (CT) 0; \
  if ( m == NULL ) return ERR_NULL; \
  TRY_BEGIN \
    RaiField field; \
    if ( ! MSG( m )->Get( n, field ) ) \
      return toErr( RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND ) ); \
    RT val; field.Get( val ); *v = (CT) val; \
  TRY_END \
}
rai_err_t rai_msg_get_bool( rai_msg_t m,  const char *n,  int *v ) {
  *v = 0;
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN
    RaiField field;
    if ( ! MSG( m )->Get( n, field ) )
      return toErr( RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND ) );
    bool b; field.Get( b ); *v = b ? 1 : 0;
  TRY_END
}
MSG_GET( rai_msg_get_i8,  int8_t,  Rai_i8 )
MSG_GET( rai_msg_get_i16, int16_t, Rai_i16 )
MSG_GET( rai_msg_get_i32, int32_t, Rai_i32 )
MSG_GET( rai_msg_get_i64, int64_t, Rai_i64 )
MSG_GET( rai_msg_get_f32, float,   Rai_f32 )
MSG_GET( rai_msg_get_f64, double,  Rai_f64 )
#undef MSG_GET

rai_err_t rai_msg_get_string( rai_msg_t m,  const char *n,  const char **s,
                              uint32_t *len ) {
  *s = NULL; *len = 0;
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN
    RaiField field;
    if ( ! MSG( m )->Get( n, field ) )
      return toErr( RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND ) );
    const char *str; field.Get( str );
    *s = str; *len = field.Size();
  TRY_END
}
rai_err_t rai_msg_get_opaque( rai_msg_t m,  const char *n,  const void **p,
                              uint32_t *len ) {
  *p = NULL; *len = 0;
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN
    RaiField field;
    if ( ! MSG( m )->Get( n, field ) )
      return toErr( RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND ) );
    *p = field.Data(); *len = field.Size();
  TRY_END
}

#define MSG_PUT( FN, OP, CT, ST, UT ) \
rai_err_t FN( rai_msg_t m,  const char *n,  CT v,  int is_unsigned ) { \
  if ( m == NULL ) return ERR_NULL; \
  TRY_BEGIN \
    if ( is_unsigned ) MSG( m )->OP( n, (UT) v ); \
    else MSG( m )->OP( n, (ST) v ); \
  TRY_END \
}
MSG_PUT( rai_msg_append_i8,  Append, int8_t,  Rai_i8,  Rai_u8 )
MSG_PUT( rai_msg_append_i16, Append, int16_t, Rai_i16, Rai_u16 )
MSG_PUT( rai_msg_append_i32, Append, int32_t, Rai_i32, Rai_u32 )
MSG_PUT( rai_msg_append_i64, Append, int64_t, Rai_i64, Rai_u64 )
MSG_PUT( rai_msg_update_i8,  Update, int8_t,  Rai_i8,  Rai_u8 )
MSG_PUT( rai_msg_update_i16, Update, int16_t, Rai_i16, Rai_u16 )
MSG_PUT( rai_msg_update_i32, Update, int32_t, Rai_i32, Rai_u32 )
MSG_PUT( rai_msg_update_i64, Update, int64_t, Rai_i64, Rai_u64 )
#undef MSG_PUT
#define MSG_PUT1( FN, OP, CT, RT ) \
rai_err_t FN( rai_msg_t m,  const char *n,  CT v ) { \
  if ( m == NULL ) return ERR_NULL; \
  TRY_BEGIN MSG( m )->OP( n, (RT) v ); TRY_END \
}
MSG_PUT1( rai_msg_append_f32, Append, float,  Rai_f32 )
MSG_PUT1( rai_msg_append_f64, Append, double, Rai_f64 )
MSG_PUT1( rai_msg_update_f32, Update, float,  Rai_f32 )
MSG_PUT1( rai_msg_update_f64, Update, double, Rai_f64 )
MSG_PUT1( rai_msg_append_string, Append, const char *, const char * )
MSG_PUT1( rai_msg_update_string, Update, const char *, const char * )
#undef MSG_PUT1
rai_err_t rai_msg_append_bool( rai_msg_t m,  const char *n,  int v ) {
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->Append( n, (bool) ( v != 0 ) ); TRY_END
}
rai_err_t rai_msg_update_bool( rai_msg_t m,  const char *n,  int v ) {
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->Update( n, (bool) ( v != 0 ) ); TRY_END
}
rai_err_t rai_msg_append_opaque( rai_msg_t m,  const char *n,  const void *p,
                                 uint32_t len ) {
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN
    RaiField f;
    f.InitRaw( n, n ? (RaiMsg_size) ::strlen( n ) + 1 : 0, RAIMSG_OPAQUE,
               len, (RaiMsg_data) p );
    MSG( m )->Append( &f );
  TRY_END
}
rai_err_t rai_msg_update_opaque( rai_msg_t m,  const char *n,  const void *p,
                                 uint32_t len ) {
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN
    RaiField f;
    f.InitRaw( n, n ? (RaiMsg_size) ::strlen( n ) + 1 : 0, RAIMSG_OPAQUE,
               len, (RaiMsg_data) p );
    MSG( m )->Update( &f );
  TRY_END
}
rai_err_t rai_msg_append_partial( rai_msg_t m,  const char *n,  const void *p,
                                  uint32_t len,  uint32_t offset ) {
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->Append( n, (RaiMsg_data) p, len, offset ); TRY_END
}
rai_err_t rai_msg_append_msg( rai_msg_t m,  const char *n,  rai_msg_t sub ) {
  if ( m == NULL || sub == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->Append( n, MSG( sub ) ); TRY_END
}
rai_err_t rai_msg_append_field( rai_msg_t m,  rai_field_t f ) {
  if ( m == NULL || f == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->Append( FLD( f ) ); TRY_END
}
rai_err_t rai_msg_update_field( rai_msg_t m,  rai_field_t f ) {
  if ( m == NULL || f == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->Update( FLD( f ) ); TRY_END
}
rai_err_t rai_msg_unpack( rai_msg_t m,  const void *buf,  uint32_t len ) {
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN MSG( m )->UnPack( (RaiMsg_data) buf, len ); TRY_END
}
rai_err_t rai_msg_pack_size( rai_msg_t m,  uint32_t *size ) {
  *size = 0;
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN *size = MSG( m )->PackSize(); TRY_END
}
rai_err_t rai_msg_pack( rai_msg_t m,  void *buf,  uint32_t len,
                        uint32_t *size ) {
  *size = 0;
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN
    RaiMsg_size sz = MSG( m )->PackSize();
    if ( sz > len )
      return toErr( RaiMsgErr::getErr( RaiMsgErr::BAD_ARG ) );
    MSG( m )->Pack( (RaiMsg_data) buf );
    *size = sz;
  TRY_END
}
rai_err_t rai_msg_packed( rai_msg_t m,  const void **buf,  uint32_t *size ) {
  *buf = NULL; *size = 0;
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN
    *size = MSG( m )->PackSize();
    *buf  = MSG( m )->Packed();
  TRY_END
}
rai_err_t rai_msg_activate( rai_msg_t m,  const char *n,  int *ok ) {
  *ok = 0;
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN *ok = MSG( m )->Activate( n ) ? 1 : 0; TRY_END
}
rai_err_t rai_msg_rename( rai_msg_t m,  const char *o,  const char *nw,
                          int *ok ) {
  *ok = 0;
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN *ok = MSG( m )->Rename( o, nw ) ? 1 : 0; TRY_END
}
rai_err_t rai_msg_remove( rai_msg_t m,  const char *n,  int *ok ) {
  *ok = 0;
  if ( m == NULL ) return ERR_NULL;
  TRY_BEGIN *ok = MSG( m )->Remove( n ) ? 1 : 0; TRY_END
}
rai_err_t
rai_msg_print( rai_msg_t m,  rai_write_fn fn,  void *cl,  int field_newlines,
               const char *fname_format,  int print_op,  const char *debug_fmt,
               const char *debug_hfmt )
{
  if ( m == NULL || fn == NULL ) return ERR_NULL;
  TRY_BEGIN
    CWriteStream out( fn, cl );
    MSG( m )->Print( &out, field_newlines ? 1 : 0,
                     fname_format ? fname_format : "%-14s : ",
                     print_op ? 1 : 0, debug_fmt ? debug_fmt : "%-7s %3d : ",
                     debug_hfmt );
    out.flush();
  TRY_END
}
rai_err_t rai_msg_print_hex( rai_msg_t m,  rai_write_fn fn,  void *cl ) {
  if ( m == NULL || fn == NULL ) return ERR_NULL;
  TRY_BEGIN
    CWriteStream out( fn, cl );
    MSG( m )->PrintHex( &out );
    out.flush();
  TRY_END
}
rai_err_t rai_msg_print_hex_buf( const void *buf,  uint32_t len,
                                 rai_write_fn fn,  void *cl ) {
  if ( buf == NULL || fn == NULL ) return ERR_NULL;
  TRY_BEGIN
    CWriteStream out( fn, cl );
    RaiMsg::PrintHex( &out, (Rai_u8 *) buf, len, 0 );
    out.flush();
  TRY_END
}
rai_err_t rai_msg_print_xml( rai_msg_t m,  rai_write_fn fn,  void *cl,
                             int attr_flags,  int print_newlines ) {
  if ( m == NULL || fn == NULL ) return ERR_NULL;
  TRY_BEGIN
    CWriteStream out( fn, cl );
    MSG( m )->PrintXML( &out, (Rai_u32) attr_flags, print_newlines ? 1 : 0 );
    out.flush();
  TRY_END
}

/* ---- RaiField --------------------------------------------------------- */
rai_field_t rai_field_create( void ) {
  try {
    Sys::initialize();
    return ret<rai_field_s>( new RaiField() );
  } catch ( RaiException ) {
    return NULL;
  }
}
void rai_field_delete( rai_field_t f ) {
  if ( f != NULL ) delete FLD( f );
}
const char * rai_field_name( rai_field_t f ) {
  return f == NULL ? NULL : FLD( f )->Name();
}
int rai_field_type( rai_field_t f ) {
  return f == NULL ? 0 : (int) FLD( f )->Type();
}
uint32_t rai_field_size( rai_field_t f ) {
  return f == NULL ? 0 : FLD( f )->Size();
}
int rai_field_hint_type( rai_field_t f ) {
  return f == NULL ? 0 : (int) FLD( f )->HintType();
}
uint32_t rai_field_hint_size( rai_field_t f ) {
  return f == NULL ? 0 : FLD( f )->HintSize();
}
int rai_field_entry_type( rai_field_t f ) {
  return f == NULL ? 0 : (int) FLD( f )->EntryType();
}
uint32_t rai_field_entry_size( rai_field_t f ) {
  return f == NULL ? 0 : FLD( f )->EntrySize();
}
uint32_t rai_field_num_entries( rai_field_t f ) {
  return f == NULL ? 0 : FLD( f )->NumEntries();
}
uint32_t rai_field_offset( rai_field_t f ) {
  return f == NULL ? 0 : FLD( f )->Offset();
}
int rai_field_fid( rai_field_t f,  uint16_t *fid ) {
  *fid = 0;
  return f == NULL ? 0 : ( FLD( f )->Fid( *fid ) ? 1 : 0 );
}
const void * rai_field_data( rai_field_t f ) {
  return f == NULL ? NULL : (const void *) FLD( f )->Data();
}
const void * rai_field_hint_data( rai_field_t f ) {
  return f == NULL ? NULL : (const void *) FLD( f )->HintData();
}
#define FLD_GET( FN, CT, RT ) \
rai_err_t FN( rai_field_t f,  CT *v ) { \
  *v = (CT) 0; \
  if ( f == NULL ) return ERR_NULL; \
  TRY_BEGIN RT val; FLD( f )->Get( val ); *v = (CT) val; TRY_END \
}
rai_err_t rai_field_get_bool( rai_field_t f,  int *v ) {
  *v = 0;
  if ( f == NULL ) return ERR_NULL;
  TRY_BEGIN bool b; FLD( f )->Get( b ); *v = b ? 1 : 0; TRY_END
}
FLD_GET( rai_field_get_i8,  int8_t,  Rai_i8 )
FLD_GET( rai_field_get_i16, int16_t, Rai_i16 )
FLD_GET( rai_field_get_i32, int32_t, Rai_i32 )
FLD_GET( rai_field_get_i64, int64_t, Rai_i64 )
FLD_GET( rai_field_get_f32, float,   Rai_f32 )
FLD_GET( rai_field_get_f64, double,  Rai_f64 )
#undef FLD_GET
rai_err_t rai_field_get_string( rai_field_t f,  const char **s,
                                uint32_t *len ) {
  *s = NULL; *len = 0;
  if ( f == NULL ) return ERR_NULL;
  TRY_BEGIN
    const char *str; FLD( f )->Get( str );
    *s = str; *len = FLD( f )->Size();
  TRY_END
}
rai_err_t rai_field_get_msg( rai_field_t f,  rai_msg_t sub ) {
  if ( f == NULL || sub == NULL ) return ERR_NULL;
  TRY_BEGIN FLD( f )->Get( *MSG( sub ) ); TRY_END
}
/* array entries: elements are packed machine types of entry_size bytes */
static rai_err_t
array_entry( RaiField *f,  uint32_t i,  const uint8_t *&p,  RaiMsg_type &t,
             RaiMsg_size &sz )
{
  if ( f->Type() != RAIMSG_ARRAY )
    return ERR_NOTARR;
  if ( i >= f->NumEntries() )
    return ERR_RANGE;
  t  = f->EntryType();
  sz = f->EntrySize();
  p  = (const uint8_t *) f->Data() + (size_t) i * sz;
  return NULL;
}
rai_err_t rai_field_get_entry_i64( rai_field_t f,  uint32_t i,  int64_t *v ) {
  *v = 0;
  if ( f == NULL ) return ERR_NULL;
  const uint8_t *p; RaiMsg_type t; RaiMsg_size sz;
  rai_err_t e = array_entry( FLD( f ), i, p, t, sz );
  if ( e != NULL ) return e;
  TRY_BEGIN
    Rai_i64 val;
    RaiField::Convert( RAIMSG_INT, 8, (RaiMsg_data) &val, t, sz,
                       (RaiMsg_data) p );
    *v = val;
  TRY_END
}
rai_err_t rai_field_get_entry_f64( rai_field_t f,  uint32_t i,  double *v ) {
  *v = 0;
  if ( f == NULL ) return ERR_NULL;
  const uint8_t *p; RaiMsg_type t; RaiMsg_size sz;
  rai_err_t e = array_entry( FLD( f ), i, p, t, sz );
  if ( e != NULL ) return e;
  TRY_BEGIN
    Rai_f64 val;
    RaiField::Convert( RAIMSG_REAL, 8, (RaiMsg_data) &val, t, sz,
                       (RaiMsg_data) p );
    *v = val;
  TRY_END
}
rai_err_t rai_field_get_entry_string( rai_field_t f,  uint32_t i,
                                      const char **s,  uint32_t *len ) {
  *s = NULL; *len = 0;
  if ( f == NULL ) return ERR_NULL;
  const uint8_t *p; RaiMsg_type t; RaiMsg_size sz;
  rai_err_t e = array_entry( FLD( f ), i, p, t, sz );
  if ( e != NULL ) return e;
  if ( t != RAIMSG_STRING && t != RAIMSG_OPAQUE )
    return toErr( RaiMsgErr::getErr( RaiMsgErr::BAD_CVT_STRING ) );
  *s = (const char *) p; *len = sz;
  return NULL;
}
rai_err_t rai_field_first( rai_field_t f,  rai_msg_t m,  int *more ) {
  *more = 0;
  if ( f == NULL || m == NULL ) return ERR_NULL;
  TRY_BEGIN *more = FLD( f )->First( MSG( m ) ) ? 1 : 0; TRY_END
}
rai_err_t rai_field_next( rai_field_t f,  int *more ) {
  *more = 0;
  if ( f == NULL ) return ERR_NULL;
  TRY_BEGIN *more = FLD( f )->Next() ? 1 : 0; TRY_END
}
rai_err_t rai_field_find( rai_field_t f,  rai_msg_t m,  const char *name,
                          int *found ) {
  *found = 0;
  if ( f == NULL || m == NULL ) return ERR_NULL;
  TRY_BEGIN *found = FLD( f )->Find( MSG( m ), name ) ? 1 : 0; TRY_END
}
const char * rai_field_type_string( int type ) {
  return RaiMsg::TypeStr( (RaiMsg_type) type );
}

} /* extern "C" */
