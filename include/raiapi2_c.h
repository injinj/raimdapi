/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 *
 * raiapi2_c.h -- flat C binding of the Rai API (raiapi2.h) and RaiMsg.
 *
 * This is the layer that language bindings without C++ access (for example
 * .NET P/Invoke) call.  It mirrors the Java binding in src/raiapi/java one to
 * one: every object is an opaque handle, every call that can throw in C++
 * returns a rai_err_t (NULL on success), results come back through out
 * parameters, and callbacks are C function pointers with a closure.
 *
 * Strings returned as const char * are owned by the api and valid until the
 * object they came from is destroyed or the next call that changes it.
 * Strings and buffers handed to a callback are valid only for the duration
 * of the callback.
 */
#ifndef __rai_raiapi2_c_h__
#define __rai_raiapi2_c_h__

#include <stdint.h>
#include <stddef.h>

#if defined( _WIN32 ) || defined( _WIN64 )
#  if defined( RAIAPI2_C_BUILD )
#    define RAIAPI2_C_EXP __declspec( dllexport )
#  else
#    define RAIAPI2_C_EXP __declspec( dllimport )
#  endif
#else
#  define RAIAPI2_C_EXP __attribute__(( visibility( "default" ) ))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- handles --------------------------------------------------------- */
typedef struct rai_api_s        * rai_api_t;
typedef struct rai_session_s    * rai_session_t;
typedef struct rai_queue_s      * rai_queue_t;
typedef struct rai_subscribe_s  * rai_subscribe_t;
typedef struct rai_publish_s    * rai_publish_t;
typedef struct rai_ipublish_s   * rai_ipublish_t; /* interactive publish */
typedef struct rai_timer_s      * rai_timer_t;
typedef struct rai_dict_s       * rai_dict_t;
typedef struct rai_entitle_s    * rai_entitle_t;
typedef struct rai_args_s       * rai_args_t;
typedef struct rai_msg_s        * rai_msg_t;
typedef struct rai_field_s      * rai_field_t;
typedef struct rai_callback_s   * rai_callback_t; /* opaque cb registration */
/* error: pointer to a static { status, reason, module } record, NULL = ok */
typedef const struct rai_error_s * rai_err_t;

RAIAPI2_C_EXP uint32_t     rai_err_status( rai_err_t e );
RAIAPI2_C_EXP const char * rai_err_reason( rai_err_t e );
RAIAPI2_C_EXP const char * rai_err_module( rai_err_t e );

/* ---- log levels (RaiApi.LVL_*) --------------------------------------- */
enum rai_log_level {
  RAI_LVL_DEVEL  = 0, RAI_LVL_FTRACE = 1, RAI_LVL_TRACE  = 2,
  RAI_LVL_DEBUG  = 3, RAI_LVL_MINOR  = 4, RAI_LVL_NORMAL = 5,
  RAI_LVL_ERROR  = 6
};

/* ---- output stream callback (RaiMsg.Print, Args.addDefaults) ---------- */
/* return bytes consumed; return 0 to signal a broken pipe */
typedef uint32_t (*rai_write_fn)( void *closure,  const uint8_t *buf,
                                  uint32_t len );

/* ---- RaiApi ----------------------------------------------------------- */
RAIAPI2_C_EXP rai_err_t rai_api_open( const char *api_name,  int argc,
                                      const char **argv,  rai_api_t *api );
RAIAPI2_C_EXP void      rai_api_close( rai_api_t api );   /* RaiApi.Close */
RAIAPI2_C_EXP void      rai_api_delete( rai_api_t api );  /* finalize */
RAIAPI2_C_EXP const char * rai_api_name( rai_api_t api );
RAIAPI2_C_EXP const char * rai_api_version( void );
RAIAPI2_C_EXP rai_err_t rai_api_get_args( rai_api_t api,  rai_args_t args );
RAIAPI2_C_EXP rai_err_t rai_api_get_dict_args( rai_args_t args );
RAIAPI2_C_EXP rai_err_t rai_api_parse_args( rai_api_t api,  rai_args_t args );
RAIAPI2_C_EXP rai_err_t rai_api_open_log_args( rai_args_t args,  int *opened );
RAIAPI2_C_EXP rai_err_t rai_api_open_log( const char *name,  int level,
                                          int verb );
/* err may be NULL; where/line identify the caller, may be NULL/0 */
RAIAPI2_C_EXP void      rai_api_print_log( rai_api_t api,  int level,
                                           rai_err_t err,  const char *where,
                                           int line,  const char *s );
RAIAPI2_C_EXP void      rai_api_log( int level,  rai_err_t err,
                                     const char *where,  int line,
                                     const char *s );
RAIAPI2_C_EXP rai_err_t rai_api_open_dict( rai_args_t args,  int *loaded );
RAIAPI2_C_EXP rai_err_t rai_api_create_session( rai_api_t api,
                                                rai_session_t *session );
RAIAPI2_C_EXP rai_err_t rai_api_new_msg( int proto,  uint16_t msg_type,
                                         uint16_t rec_type,  uint16_t seqno,
                                         uint16_t rec_status,  rai_msg_t *m );
RAIAPI2_C_EXP rai_err_t rai_api_new_msg_form( int proto,  uint16_t msg_type,
                                              const char *form_type,
                                              uint16_t seqno,
                                              uint16_t rec_status,
                                              rai_msg_t *m );
RAIAPI2_C_EXP int       rai_api_set_ioctl( rai_api_t api,  const char *parm,
                                           const char *value );
/* signal handler: SIGINT(2) SIGHUP(1) SIGTERM(15) -> fn( sig, closure ) */
typedef void (*rai_signal_fn)( int sig,  void *closure );
RAIAPI2_C_EXP rai_err_t rai_api_register_sig_handler( rai_signal_fn fn,
                                                      void *closure );

/* ---- events passed to callbacks --------------------------------------- */
typedef struct rai_msg_event_s {
  rai_subscribe_t subscribe;
  const char    * subject;     /* received subject (may be an _INBOX) */
  int             type;        /* 0 = SNAP, 1 = UPDATE */
  uint16_t        msg_type,
                  rec_status;
  int             old_state,   /* RaiSubscribe.STATE_* */
                  recv,
                  state;
  int64_t         pub_time,    /* ns, 0 if none */
                  route_time;
  uint32_t        counter,
                  old_counter;
} rai_msg_event_t;

typedef struct rai_subscribe_event_s {
  rai_ipublish_t  publish;
  const char    * subject,
                * reply;
  uint32_t        query_flags;
} rai_subscribe_event_t;

typedef struct rai_dataloss_event_s {
  rai_session_t   session;
  const char    * transport_name,
                * description;
  uint32_t        inbound_packet_loss,
                  outbound_packet_loss,
                  connection_count;
  int             connection_loss,
                  is_multicast;
} rai_dataloss_event_t;

typedef struct rai_connection_event_s {
  rai_session_t   session;
  const char    * transport_name,
                * description;
  uint32_t        connection_count;
  int             connection_oriented,
                  is_multicast;
} rai_connection_event_t;

typedef void (*rai_msg_fn)( void *closure,  const rai_msg_event_t *ev,
                            rai_msg_t msg );
typedef void (*rai_timer_fn)( void *closure,  rai_timer_t timer );
typedef void (*rai_subscribe_fn)( void *closure,
                                  const rai_subscribe_event_t *ev,
                                  rai_msg_t msg );
typedef void (*rai_dataloss_fn)( void *closure,
                                 const rai_dataloss_event_t *ev );
typedef void (*rai_connection_fn)( void *closure,
                                   const rai_connection_event_t *ev );

/* ---- RaiSession ------------------------------------------------------- */
RAIAPI2_C_EXP rai_err_t rai_session_start( rai_session_t s );
RAIAPI2_C_EXP rai_err_t rai_session_create_queue( rai_session_t s,
                                                  int direct,  rai_queue_t *q );
RAIAPI2_C_EXP rai_err_t rai_session_create_publish( rai_session_t s,
                                                    int auto_inc,
                                                    rai_publish_t *p );
RAIAPI2_C_EXP rai_err_t rai_session_create_dict( rai_session_t s,
                                                 rai_dict_t *d );
RAIAPI2_C_EXP rai_err_t rai_session_destroy( rai_session_t s );
RAIAPI2_C_EXP rai_err_t rai_session_login( rai_session_t s,  const char *user,
                                           rai_entitle_t *e );
/* cb registration lives until rai_callback_delete() (after session destroy) */
RAIAPI2_C_EXP rai_err_t rai_session_set_dataloss_cb( rai_session_t s,
                                                     rai_dataloss_fn loss_fn,
                                                     rai_connection_fn conn_fn,
                                                     void *closure,
                                                     rai_callback_t *cb );
RAIAPI2_C_EXP rai_err_t rai_session_notify_status( rai_session_t s,
                                                   uint16_t msg_type,
                                                   uint16_t rec_status );
RAIAPI2_C_EXP rai_api_t rai_session_get_api( rai_session_t s );
RAIAPI2_C_EXP rai_err_t rai_session_set_name( rai_session_t s,
                                              const char *name );
RAIAPI2_C_EXP const char * rai_session_get_name( rai_session_t s );
RAIAPI2_C_EXP void      rai_callback_delete( rai_callback_t cb );

/* ---- RaiQueue --------------------------------------------------------- */
RAIAPI2_C_EXP rai_err_t rai_queue_create_subscribe( rai_queue_t q,
                                                    rai_msg_fn fn,
                                                    void *closure,
                                                    rai_subscribe_t *sub,
                                                    rai_callback_t *cb );
RAIAPI2_C_EXP rai_err_t rai_queue_create_timer( rai_queue_t q,
                                                rai_timer_fn fn,  void *closure,
                                                rai_timer_t *t,
                                                rai_callback_t *cb );
RAIAPI2_C_EXP rai_err_t rai_queue_create_ipublish( rai_queue_t q,
                                                   rai_subscribe_fn fn,
                                                   void *closure,
                                                   rai_ipublish_t *p,
                                                   rai_callback_t *cb );
RAIAPI2_C_EXP rai_err_t rai_queue_notify_status( rai_queue_t q,
                                                 uint16_t msg_type,
                                                 uint16_t rec_status );
RAIAPI2_C_EXP rai_err_t rai_queue_mainloop( rai_queue_t q );
RAIAPI2_C_EXP rai_err_t rai_queue_timed_dispatch( rai_queue_t q,
                                                  uint32_t ival_ms );
RAIAPI2_C_EXP rai_err_t rai_queue_dispatch( rai_queue_t q );
RAIAPI2_C_EXP uint32_t  rai_queue_get_depth( rai_queue_t q );
RAIAPI2_C_EXP rai_err_t rai_queue_destroy( rai_queue_t q );
RAIAPI2_C_EXP rai_session_t rai_queue_get_session( rai_queue_t q );

/* ---- RaiSubscribe ----------------------------------------------------- */
enum rai_sub_parm { RAI_SUB_UPDATE = 1, RAI_SUB_SNAP = 2, RAI_SUB_BOTH = 3,
                    RAI_SUB_NO_PREFIX = 4, RAI_SUB_NO_COPY = 8 };
enum rai_sub_state { RAI_STATE_NO_MSG = 0, RAI_STATE_WILDCARD = 1,
                     RAI_STATE_NO_HDR = 2, RAI_STATE_INITIAL = 3,
                     RAI_STATE_UPDATE = 4, RAI_STATE_NOTFOUND = 5,
                     RAI_STATE_STALE = 6, RAI_STATE_DROPPED = 7 };
RAIAPI2_C_EXP rai_err_t rai_subscribe_start( rai_subscribe_t s,
                                             const char *subject,  int parm,
                                             uint32_t timeout_ms );
RAIAPI2_C_EXP rai_err_t rai_subscribe_cancel( rai_subscribe_t s );
RAIAPI2_C_EXP rai_err_t rai_subscribe_refresh( rai_subscribe_t s,
                                               uint32_t timeout_ms );
RAIAPI2_C_EXP const char * rai_subscribe_subject( rai_subscribe_t s );
RAIAPI2_C_EXP int       rai_subscribe_in_progress( rai_subscribe_t s );
RAIAPI2_C_EXP rai_queue_t rai_subscribe_get_queue( rai_subscribe_t s );
RAIAPI2_C_EXP const char * rai_subscribe_state_to_string( int state );

/* ---- RaiPublish / RaiInteractivePublish ------------------------------- */
RAIAPI2_C_EXP rai_err_t rai_publish_msg( rai_publish_t p,  const char *subject,
                                         rai_msg_t m,  int64_t stamp );
RAIAPI2_C_EXP rai_err_t rai_publish_buf( rai_publish_t p,  const char *subject,
                                         const void *buf,  uint32_t len,
                                         int64_t stamp );
RAIAPI2_C_EXP rai_err_t rai_publish_set_prefix( rai_publish_t p,
                                                const char *prefix );
RAIAPI2_C_EXP const char * rai_publish_get_prefix( rai_publish_t p );
RAIAPI2_C_EXP uint32_t  rai_publish_get_seqno( rai_publish_t p );
RAIAPI2_C_EXP void      rai_publish_set_seqno( rai_publish_t p,  uint32_t n );
RAIAPI2_C_EXP rai_err_t rai_publish_destroy( rai_publish_t p );
RAIAPI2_C_EXP rai_session_t rai_publish_get_session( rai_publish_t p );
/* an interactive publish is also a publish */
RAIAPI2_C_EXP rai_publish_t rai_ipublish_publish( rai_ipublish_t p );
RAIAPI2_C_EXP rai_err_t rai_ipublish_start( rai_ipublish_t p,
                                            const char *subject );
RAIAPI2_C_EXP rai_err_t rai_ipublish_cancel( rai_ipublish_t p );
RAIAPI2_C_EXP int       rai_ipublish_in_progress( rai_ipublish_t p );
RAIAPI2_C_EXP rai_queue_t rai_ipublish_get_queue( rai_ipublish_t p );

/* ---- RaiTimer --------------------------------------------------------- */
RAIAPI2_C_EXP rai_err_t rai_timer_start( rai_timer_t t );
RAIAPI2_C_EXP void      rai_timer_stop( rai_timer_t t );
RAIAPI2_C_EXP int64_t   rai_timer_get_interval( rai_timer_t t );
RAIAPI2_C_EXP void      rai_timer_set_interval( rai_timer_t t,  int64_t ms );
RAIAPI2_C_EXP rai_queue_t rai_timer_get_queue( rai_timer_t t );

/* ---- RaiDict ---------------------------------------------------------- */
RAIAPI2_C_EXP rai_err_t rai_dict_load( rai_dict_t d,  uint32_t timeout_secs,
                                       const char *dict_subject,
                                       int load_wait );
RAIAPI2_C_EXP int       rai_dict_have_dict( rai_dict_t d );
RAIAPI2_C_EXP int       rai_dict_in_progress( rai_dict_t d );
RAIAPI2_C_EXP rai_session_t rai_dict_get_session( rai_dict_t d );

/* ---- RaiEntitlement --------------------------------------------------- */
RAIAPI2_C_EXP void      rai_entitle_delete( rai_entitle_t e );

/* ---- Args ------------------------------------------------------------- */
enum rai_arg_flags {
  RAI_IGNORE_ARG = 0, RAI_RESOURCE_ARG = 1, RAI_COMMAND_ARG = 2,
  RAI_TIME_SEC_ARG = 4, RAI_TIME_MS_ARG = 8, RAI_MEM_ARG = 16,
  RAI_HELP_ARG = 32, RAI_VERSION_ARG = 64, RAI_PRINTRC_ARG = 128,
  RAI_RCFILE_ARG = 256, RAI_LIST_ARG = 512, RAI_NO_DEFAULT_VAL = 1024,
  RAI_BITS_ARG = 2048
};
RAIAPI2_C_EXP rai_args_t rai_args_create( void );
RAIAPI2_C_EXP void      rai_args_delete( rai_args_t a );
/* the strings are copied; def may be NULL for string args */
RAIAPI2_C_EXP rai_err_t rai_args_add_string( rai_args_t a,  const char *name,
                                             const char *def,
                                             const char *example,
                                             const char *descr,  int flags );
RAIAPI2_C_EXP rai_err_t rai_args_add_bool( rai_args_t a,  const char *name,
                                           int def,  const char *example,
                                           const char *descr,  int flags );
RAIAPI2_C_EXP rai_err_t rai_args_add_int( rai_args_t a,  const char *name,
                                          uint32_t def,  const char *example,
                                          const char *descr,  int flags );
RAIAPI2_C_EXP rai_err_t rai_args_add_double( rai_args_t a,  const char *name,
                                             double def,  const char *example,
                                             const char *descr,  int flags );
/* out_fn receives help/version output; NULL -> stderr */
RAIAPI2_C_EXP rai_err_t rai_args_add_defaults( rai_args_t a,  const char *vers,
                                               const char *prefix,
                                               rai_write_fn out_fn,
                                               void *out_cl,
                                               const char *argv0 );
/* returns 0 when -help / -version handled and the program should exit */
RAIAPI2_C_EXP rai_err_t rai_args_process( rai_args_t a,  int argc,
                                          const char **argv,  int *ok );
RAIAPI2_C_EXP uint32_t  rai_args_num_values( rai_args_t a,  const char *n );
RAIAPI2_C_EXP rai_err_t rai_args_get_string( rai_args_t a,  const char *n,
                                             uint32_t i,  const char **val );
RAIAPI2_C_EXP rai_err_t rai_args_get_bool( rai_args_t a,  const char *n,
                                           uint32_t i,  int *val );
RAIAPI2_C_EXP rai_err_t rai_args_get_int( rai_args_t a,  const char *n,
                                          uint32_t i,  uint32_t *val );
RAIAPI2_C_EXP rai_err_t rai_args_get_double( rai_args_t a,  const char *n,
                                             uint32_t i,  double *val );
RAIAPI2_C_EXP rai_err_t rai_args_set_string( rai_args_t a,  const char *n,
                                             const char *val );
RAIAPI2_C_EXP rai_err_t rai_args_set_bool( rai_args_t a,  const char *n,
                                           int val );
RAIAPI2_C_EXP rai_err_t rai_args_set_int( rai_args_t a,  const char *n,
                                          uint32_t val );
RAIAPI2_C_EXP rai_err_t rai_args_set_double( rai_args_t a,  const char *n,
                                             double val );
RAIAPI2_C_EXP int       rai_args_is_set( rai_args_t a,  const char *n );

/* ---- Time ------------------------------------------------------------- */
RAIAPI2_C_EXP int64_t   rai_time_current_ns( void );
RAIAPI2_C_EXP int64_t   rai_time_hires_ns( void );
RAIAPI2_C_EXP int64_t   rai_time_hires_to_ns( int64_t hires );
/* buf must be >= 64 bytes; ns == 0 -> now */
RAIAPI2_C_EXP const char * rai_time_ns_timestamp( int64_t ns,  int precision,
                                                  char *buf,  uint32_t len );
RAIAPI2_C_EXP const char * rai_time_ns_interval( int64_t ns,  char *buf,
                                                 uint32_t len );
enum rai_tz { RAI_TZ_LOCAL_TIME = 0, RAI_TZ_GM_TIME = 1 };
RAIAPI2_C_EXP const char * rai_time_strftime( int tz,  int64_t ms,
                                              const char *fmt,  char *buf,
                                              uint32_t len );
/* TimeRotate: state is carried by the caller (mirrors the Java class) */
typedef struct rai_time_rotate_s {
  int64_t time, period, last_time;
  int     day_or_week;  /* 0 unspecified, 1 daily, 2 weekly */
} rai_time_rotate_t;
RAIAPI2_C_EXP int rai_time_rotate_set_time( rai_time_rotate_t *r,
                                            const char *spec,  int rot_dow,
                                            int64_t rot_time );
RAIAPI2_C_EXP int rai_time_rotate_set_period( rai_time_rotate_t *r,
                                              const char *spec,
                                              int64_t rot_period );

/* ---- RaiMsg ----------------------------------------------------------- */
enum rai_msg_type {
  RAI_MSG_NODATA = 0, RAI_MSG_MESSAGE = 1, RAI_MSG_STRING = 2,
  RAI_MSG_OPAQUE = 3, RAI_MSG_BOOLEAN = 4, RAI_MSG_INT = 5, RAI_MSG_UINT = 6,
  RAI_MSG_REAL = 7, RAI_MSG_ARRAY = 8, RAI_MSG_PARTIAL = 9, RAI_MSG_IPDATA = 10
};
enum rai_msg_proto {
  RAI_RAIMSG_PROTO = 0, RAI_RV_SASS_PROTO = 1, RAI_TIB_SASS_PROTO = 2,
  RAI_TIB_SASS_FORM_PROTO = 3, RAI_RV_RAIMSG_PROTO = 4, RAI_XREP_PROTO = 5,
  RAI_RV_PROTO = 6, RAI_CISERVER_SASS_PROTO = 7,
  RAI_CISERVER_SASS_FORM_PROTO = 8
};
RAIAPI2_C_EXP const char * rai_msg_version( void );
RAIAPI2_C_EXP rai_err_t rai_msg_create( int proto,  rai_msg_t *m );
RAIAPI2_C_EXP void      rai_msg_delete( rai_msg_t m );
RAIAPI2_C_EXP rai_err_t rai_msg_reuse( rai_msg_t m,  int proto ); /* -1 keep */
RAIAPI2_C_EXP void      rai_msg_set_protocol( rai_msg_t m,  int proto );
RAIAPI2_C_EXP int       rai_msg_get_protocol( rai_msg_t m );
RAIAPI2_C_EXP const char * rai_msg_get_protocol_string( rai_msg_t m );
RAIAPI2_C_EXP rai_err_t rai_msg_clear_form( rai_msg_t m );
RAIAPI2_C_EXP rai_err_t rai_msg_release( rai_msg_t m );
RAIAPI2_C_EXP rai_err_t rai_msg_copy( rai_msg_t m,  rai_msg_t from );
/* sass header helpers: MSG_TYPE / REC_TYPE / REC_STATUS as strings */
RAIAPI2_C_EXP const char * rai_msg_type_to_string( uint16_t msg_type,
                                                   char *buf,  uint32_t len );
RAIAPI2_C_EXP uint16_t  rai_msg_string_to_type( const char *s );
RAIAPI2_C_EXP const char * rai_rec_status_to_string( uint16_t rec_status,
                                                     char *buf,  uint32_t len );
RAIAPI2_C_EXP uint16_t  rai_string_to_rec_status( const char *s );
RAIAPI2_C_EXP rai_err_t rai_msg_rec_type_to_string( uint16_t rec_type,
                                                    const char **s );
RAIAPI2_C_EXP rai_err_t rai_msg_string_to_rec_type( const char *s,
                                                    uint16_t *rec_type );
RAIAPI2_C_EXP rai_err_t rai_msg_get_hdr_string( rai_msg_t m,  const char *fname,
                                                const char **s );
RAIAPI2_C_EXP rai_err_t rai_msg_set_hdr_string( rai_msg_t m,  const char *fname,
                                                const char *s );
/* field lookup: fills fld (a rai_field_create()'d field), *found = 0/1 */
RAIAPI2_C_EXP rai_err_t rai_msg_get_field( rai_msg_t m,  const char *name,
                                           rai_field_t fld,  int *found );
/* typed get; error NOT_FOUND (9) when missing, BAD_CVT_* when wrong type */
RAIAPI2_C_EXP rai_err_t rai_msg_get_bool( rai_msg_t m,  const char *n, int *v );
RAIAPI2_C_EXP rai_err_t rai_msg_get_i8( rai_msg_t m,  const char *n, int8_t *v);
RAIAPI2_C_EXP rai_err_t rai_msg_get_i16( rai_msg_t m, const char *n,int16_t *v);
RAIAPI2_C_EXP rai_err_t rai_msg_get_i32( rai_msg_t m, const char *n,int32_t *v);
RAIAPI2_C_EXP rai_err_t rai_msg_get_i64( rai_msg_t m, const char *n,int64_t *v);
RAIAPI2_C_EXP rai_err_t rai_msg_get_f32( rai_msg_t m,  const char *n, float *v);
RAIAPI2_C_EXP rai_err_t rai_msg_get_f64( rai_msg_t m,  const char *n,double *v);
RAIAPI2_C_EXP rai_err_t rai_msg_get_string( rai_msg_t m,  const char *n,
                                            const char **s,  uint32_t *len );
RAIAPI2_C_EXP rai_err_t rai_msg_get_opaque( rai_msg_t m,  const char *n,
                                            const void **p,  uint32_t *len );
/* append / update: is_unsigned selects RAI_MSG_UINT for the int types */
RAIAPI2_C_EXP rai_err_t rai_msg_append_bool( rai_msg_t m,  const char *n,
                                             int v );
RAIAPI2_C_EXP rai_err_t rai_msg_append_i8( rai_msg_t m,  const char *n,
                                           int8_t v,  int is_unsigned );
RAIAPI2_C_EXP rai_err_t rai_msg_append_i16( rai_msg_t m,  const char *n,
                                            int16_t v,  int is_unsigned );
RAIAPI2_C_EXP rai_err_t rai_msg_append_i32( rai_msg_t m,  const char *n,
                                            int32_t v,  int is_unsigned );
RAIAPI2_C_EXP rai_err_t rai_msg_append_i64( rai_msg_t m,  const char *n,
                                            int64_t v,  int is_unsigned );
RAIAPI2_C_EXP rai_err_t rai_msg_append_f32( rai_msg_t m,  const char *n,
                                            float v );
RAIAPI2_C_EXP rai_err_t rai_msg_append_f64( rai_msg_t m,  const char *n,
                                            double v );
RAIAPI2_C_EXP rai_err_t rai_msg_append_string( rai_msg_t m,  const char *n,
                                               const char *s );
RAIAPI2_C_EXP rai_err_t rai_msg_append_opaque( rai_msg_t m,  const char *n,
                                               const void *p,  uint32_t len );
RAIAPI2_C_EXP rai_err_t rai_msg_append_partial( rai_msg_t m,  const char *n,
                                                const void *p,  uint32_t len,
                                                uint32_t offset );
RAIAPI2_C_EXP rai_err_t rai_msg_append_msg( rai_msg_t m,  const char *n,
                                            rai_msg_t sub );
RAIAPI2_C_EXP rai_err_t rai_msg_append_field( rai_msg_t m,  rai_field_t f );
RAIAPI2_C_EXP rai_err_t rai_msg_update_bool( rai_msg_t m,  const char *n,
                                             int v );
RAIAPI2_C_EXP rai_err_t rai_msg_update_i8( rai_msg_t m,  const char *n,
                                           int8_t v,  int is_unsigned );
RAIAPI2_C_EXP rai_err_t rai_msg_update_i16( rai_msg_t m,  const char *n,
                                            int16_t v,  int is_unsigned );
RAIAPI2_C_EXP rai_err_t rai_msg_update_i32( rai_msg_t m,  const char *n,
                                            int32_t v,  int is_unsigned );
RAIAPI2_C_EXP rai_err_t rai_msg_update_i64( rai_msg_t m,  const char *n,
                                            int64_t v,  int is_unsigned );
RAIAPI2_C_EXP rai_err_t rai_msg_update_f32( rai_msg_t m,  const char *n,
                                            float v );
RAIAPI2_C_EXP rai_err_t rai_msg_update_f64( rai_msg_t m,  const char *n,
                                            double v );
RAIAPI2_C_EXP rai_err_t rai_msg_update_string( rai_msg_t m,  const char *n,
                                               const char *s );
RAIAPI2_C_EXP rai_err_t rai_msg_update_opaque( rai_msg_t m,  const char *n,
                                               const void *p,  uint32_t len );
RAIAPI2_C_EXP rai_err_t rai_msg_update_field( rai_msg_t m,  rai_field_t f );
/* pack / unpack */
RAIAPI2_C_EXP rai_err_t rai_msg_unpack( rai_msg_t m,  const void *buf,
                                        uint32_t len );
RAIAPI2_C_EXP rai_err_t rai_msg_pack_size( rai_msg_t m,  uint32_t *size );
RAIAPI2_C_EXP rai_err_t rai_msg_pack( rai_msg_t m,  void *buf,  uint32_t len,
                                      uint32_t *size );
/* pointer to the packed message inside the msg, valid until next change */
RAIAPI2_C_EXP rai_err_t rai_msg_packed( rai_msg_t m,  const void **buf,
                                        uint32_t *size );
RAIAPI2_C_EXP rai_err_t rai_msg_activate( rai_msg_t m,  const char *n,
                                          int *ok );
RAIAPI2_C_EXP rai_err_t rai_msg_rename( rai_msg_t m,  const char *old_n,
                                        const char *new_n,  int *ok );
RAIAPI2_C_EXP rai_err_t rai_msg_remove( rai_msg_t m,  const char *n,  int *ok );
/* printing, through the write callback */
RAIAPI2_C_EXP rai_err_t rai_msg_print( rai_msg_t m,  rai_write_fn fn,
                                       void *cl,  int field_newlines,
                                       const char *fname_format,
                                       int print_op,  const char *debug_fmt,
                                       const char *debug_hfmt );
RAIAPI2_C_EXP rai_err_t rai_msg_print_hex( rai_msg_t m,  rai_write_fn fn,
                                           void *cl );
RAIAPI2_C_EXP rai_err_t rai_msg_print_hex_buf( const void *buf,  uint32_t len,
                                               rai_write_fn fn,  void *cl );
RAIAPI2_C_EXP rai_err_t rai_msg_print_xml( rai_msg_t m,  rai_write_fn fn,
                                           void *cl,  int attr_flags,
                                           int print_newlines );

/* ---- RaiField --------------------------------------------------------- */
RAIAPI2_C_EXP rai_field_t rai_field_create( void );
RAIAPI2_C_EXP void      rai_field_delete( rai_field_t f );
RAIAPI2_C_EXP const char * rai_field_name( rai_field_t f );
RAIAPI2_C_EXP int       rai_field_type( rai_field_t f );
RAIAPI2_C_EXP uint32_t  rai_field_size( rai_field_t f );
RAIAPI2_C_EXP int       rai_field_hint_type( rai_field_t f );
RAIAPI2_C_EXP uint32_t  rai_field_hint_size( rai_field_t f );
RAIAPI2_C_EXP int       rai_field_entry_type( rai_field_t f );  /* arrays */
RAIAPI2_C_EXP uint32_t  rai_field_entry_size( rai_field_t f );
RAIAPI2_C_EXP uint32_t  rai_field_num_entries( rai_field_t f );
RAIAPI2_C_EXP uint32_t  rai_field_offset( rai_field_t f );      /* partial */
RAIAPI2_C_EXP int       rai_field_fid( rai_field_t f,  uint16_t *fid );
/* raw data pointer (valid while the message is unchanged) */
RAIAPI2_C_EXP const void * rai_field_data( rai_field_t f );
RAIAPI2_C_EXP const void * rai_field_hint_data( rai_field_t f );
/* converting getters: BAD_CVT_* errors when incompatible */
RAIAPI2_C_EXP rai_err_t rai_field_get_bool( rai_field_t f,  int *v );
RAIAPI2_C_EXP rai_err_t rai_field_get_i8( rai_field_t f,  int8_t *v );
RAIAPI2_C_EXP rai_err_t rai_field_get_i16( rai_field_t f,  int16_t *v );
RAIAPI2_C_EXP rai_err_t rai_field_get_i32( rai_field_t f,  int32_t *v );
RAIAPI2_C_EXP rai_err_t rai_field_get_i64( rai_field_t f,  int64_t *v );
RAIAPI2_C_EXP rai_err_t rai_field_get_f32( rai_field_t f,  float *v );
RAIAPI2_C_EXP rai_err_t rai_field_get_f64( rai_field_t f,  double *v );
RAIAPI2_C_EXP rai_err_t rai_field_get_string( rai_field_t f,  const char **s,
                                              uint32_t *len );
RAIAPI2_C_EXP rai_err_t rai_field_get_msg( rai_field_t f,  rai_msg_t sub );
/* array element access, index < num_entries */
RAIAPI2_C_EXP rai_err_t rai_field_get_entry_i64( rai_field_t f,  uint32_t i,
                                                 int64_t *v );
RAIAPI2_C_EXP rai_err_t rai_field_get_entry_f64( rai_field_t f,  uint32_t i,
                                                 double *v );
RAIAPI2_C_EXP rai_err_t rai_field_get_entry_string( rai_field_t f,  uint32_t i,
                                                    const char **s,
                                                    uint32_t *len );
/* iteration over a message */
RAIAPI2_C_EXP rai_err_t rai_field_first( rai_field_t f,  rai_msg_t m,
                                         int *more );
RAIAPI2_C_EXP rai_err_t rai_field_next( rai_field_t f,  int *more );
RAIAPI2_C_EXP rai_err_t rai_field_find( rai_field_t f,  rai_msg_t m,
                                        const char *name,  int *found );
RAIAPI2_C_EXP const char * rai_field_type_string( int type );

#ifdef __cplusplus
}
#endif
#endif
