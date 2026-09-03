/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 *
 * P/Invoke declarations for libraiapi2c (include/raiapi2_c.h).  Everything
 * here is internal; the public surface mirrors the Java binding in
 * com.rai.raiapi2 / com.rai.raimsg and lives in the other files.
 */
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Com.Rai.Interop {

/* a rai_err_t is a pointer to a static { status, reason, module } record */
internal static class Native {
  internal const string Lib = "raiapi2c";
  internal const CallingConvention CC = CallingConvention.Cdecl;

  [UnmanagedFunctionPointer( CC )]
  internal delegate uint WriteFn( IntPtr cl,  IntPtr buf,  uint len );
  [UnmanagedFunctionPointer( CC )]
  internal delegate void SignalFn( int sig,  IntPtr cl );
  [UnmanagedFunctionPointer( CC )]
  internal delegate void MsgFn( IntPtr cl,  IntPtr ev,  IntPtr msg );
  [UnmanagedFunctionPointer( CC )]
  internal delegate void TimerFn( IntPtr cl,  IntPtr timer );
  [UnmanagedFunctionPointer( CC )]
  internal delegate void SubscribeFn( IntPtr cl,  IntPtr ev,  IntPtr msg );
  [UnmanagedFunctionPointer( CC )]
  internal delegate void DataLossFn( IntPtr cl,  IntPtr ev );
  [UnmanagedFunctionPointer( CC )]
  internal delegate void ConnectionFn( IntPtr cl,  IntPtr ev );

  /* events (must match raiapi2_c.h layout) */
  [StructLayout( LayoutKind.Sequential )]
  internal struct MsgEvent {
    public IntPtr subscribe;
    public IntPtr subject;
    public int    type;
    public ushort msg_type, rec_status;
    public int    old_state, recv, state;
    public long   pub_time, route_time;
    public uint   counter, old_counter;
  }
  [StructLayout( LayoutKind.Sequential )]
  internal struct SubscribeEvent {
    public IntPtr publish;
    public IntPtr subject, reply;
    public uint   query_flags;
  }
  [StructLayout( LayoutKind.Sequential )]
  internal struct DataLossEvent {
    public IntPtr session;
    public IntPtr transport_name, description;
    public uint   inbound_packet_loss, outbound_packet_loss, connection_count;
    public int    connection_loss, is_multicast;
  }
  [StructLayout( LayoutKind.Sequential )]
  internal struct ConnectionEvent {
    public IntPtr session;
    public IntPtr transport_name, description;
    public uint   connection_count;
    public int    connection_oriented, is_multicast;
  }
  [StructLayout( LayoutKind.Sequential )]
  internal struct TimeRotateState {
    public long time, period, last_time;
    public int  day_or_week;
  }

  internal static string Str( IntPtr p ) {
    return p == IntPtr.Zero ? null : Marshal.PtrToStringAnsi( p );
  }
  internal static string Str( IntPtr p,  uint len ) {
    if ( p == IntPtr.Zero ) return null;
    if ( len == 0 ) return "";
    byte[] b = new byte[ len ];
    Marshal.Copy( p, b, 0, (int) len );
    int n = (int) len;
    while ( n > 0 && b[ n - 1 ] == 0 ) n--;  /* strings carry the NUL */
    return Encoding.UTF8.GetString( b, 0, n );
  }
  internal static byte[] Bytes( IntPtr p,  uint len ) {
    byte[] b = new byte[ len ];
    if ( len > 0 ) Marshal.Copy( p, b, 0, (int) len );
    return b;
  }

  /* ---- errors ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern uint rai_err_status( IntPtr e );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_err_reason( IntPtr e );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_err_module( IntPtr e );

  /* ---- RaiApi ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_open( string api_name,  int argc,  string[] argv,  out IntPtr api );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_api_close( IntPtr api );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_api_delete( IntPtr api );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_name( IntPtr api );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_version();
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_get_args( IntPtr api,  IntPtr args );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_get_dict_args( IntPtr args );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_parse_args( IntPtr api,  IntPtr args );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_open_log_args( IntPtr args,  out int opened );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_open_log( string name,  int level,  int verb );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_api_print_log( IntPtr api,  int level,  IntPtr err,  string where,  int line,  string s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_api_log( int level,  IntPtr err,  string where,  int line,  string s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_open_dict( IntPtr args,  out int loaded );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_create_session( IntPtr api,  out IntPtr session );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_new_msg( int proto,  ushort msg_type,  ushort rec_type,  ushort seqno,  ushort rec_status,  out IntPtr m );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_new_msg_form( int proto,  ushort msg_type,  string form_type,  ushort seqno,  ushort rec_status,  out IntPtr m );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_api_set_ioctl( IntPtr api,  string parm,  string value );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_api_register_sig_handler( SignalFn fn,  IntPtr cl );

  /* ---- RaiSession ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_start( IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_create_queue( IntPtr s,  int direct,  out IntPtr q );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_create_publish( IntPtr s,  int auto_inc,  out IntPtr p );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_create_dict( IntPtr s,  out IntPtr d );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_destroy( IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_login( IntPtr s,  string user,  out IntPtr e );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_set_dataloss_cb( IntPtr s,  DataLossFn loss,  ConnectionFn conn,  IntPtr cl,  out IntPtr cb );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_notify_status( IntPtr s,  ushort msg_type,  ushort rec_status );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_get_api( IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_set_name( IntPtr s,  string name );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_session_get_name( IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_callback_delete( IntPtr cb );

  /* ---- RaiQueue ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_queue_create_subscribe( IntPtr q,  MsgFn fn,  IntPtr cl,  out IntPtr sub,  out IntPtr cb );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_queue_create_timer( IntPtr q,  TimerFn fn,  IntPtr cl,  out IntPtr t,  out IntPtr cb );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_queue_create_ipublish( IntPtr q,  SubscribeFn fn,  IntPtr cl,  out IntPtr p,  out IntPtr cb );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_queue_notify_status( IntPtr q,  ushort msg_type,  ushort rec_status );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_queue_mainloop( IntPtr q );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_queue_timed_dispatch( IntPtr q,  uint ival_ms );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_queue_dispatch( IntPtr q );
  [DllImport( Lib, CallingConvention = CC )] internal static extern uint rai_queue_get_depth( IntPtr q );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_queue_destroy( IntPtr q );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_queue_get_session( IntPtr q );

  /* ---- RaiSubscribe ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_subscribe_start( IntPtr s,  string subject,  int parm,  uint timeout_ms );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_subscribe_cancel( IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_subscribe_refresh( IntPtr s,  uint timeout_ms );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_subscribe_subject( IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_subscribe_in_progress( IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_subscribe_get_queue( IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_subscribe_state_to_string( int state );

  /* ---- RaiPublish ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_publish_msg( IntPtr p,  string subject,  IntPtr m,  long stamp );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_publish_buf( IntPtr p,  string subject,  IntPtr buf,  uint len,  long stamp );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_publish_set_prefix( IntPtr p,  string prefix );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_publish_get_prefix( IntPtr p );
  [DllImport( Lib, CallingConvention = CC )] internal static extern uint rai_publish_get_seqno( IntPtr p );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_publish_set_seqno( IntPtr p,  uint n );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_publish_destroy( IntPtr p );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_publish_get_session( IntPtr p );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_ipublish_publish( IntPtr p );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_ipublish_start( IntPtr p,  string subject );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_ipublish_cancel( IntPtr p );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_ipublish_in_progress( IntPtr p );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_ipublish_get_queue( IntPtr p );

  /* ---- RaiTimer ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_timer_start( IntPtr t );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_timer_stop( IntPtr t );
  [DllImport( Lib, CallingConvention = CC )] internal static extern long rai_timer_get_interval( IntPtr t );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_timer_set_interval( IntPtr t,  long ms );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_timer_get_queue( IntPtr t );

  /* ---- RaiDict / RaiEntitlement ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_dict_load( IntPtr d,  uint timeout_secs,  string dict_subject,  int load_wait );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_dict_have_dict( IntPtr d );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_dict_in_progress( IntPtr d );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_dict_get_session( IntPtr d );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_entitle_delete( IntPtr e );

  /* ---- Args ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_create();
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_args_delete( IntPtr a );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_add_string( IntPtr a,  string name,  string def,  string example,  string descr,  int flags );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_add_bool( IntPtr a,  string name,  int def,  string example,  string descr,  int flags );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_add_int( IntPtr a,  string name,  uint def,  string example,  string descr,  int flags );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_add_double( IntPtr a,  string name,  double def,  string example,  string descr,  int flags );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_add_defaults( IntPtr a,  string vers,  string prefix,  WriteFn out_fn,  IntPtr out_cl,  string argv0 );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_process( IntPtr a,  int argc,  string[] argv,  out int ok );
  [DllImport( Lib, CallingConvention = CC )] internal static extern uint rai_args_num_values( IntPtr a,  string n );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_get_string( IntPtr a,  string n,  uint i,  out IntPtr val );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_get_bool( IntPtr a,  string n,  uint i,  out int val );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_get_int( IntPtr a,  string n,  uint i,  out uint val );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_get_double( IntPtr a,  string n,  uint i,  out double val );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_set_string( IntPtr a,  string n,  string val );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_set_bool( IntPtr a,  string n,  int val );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_set_int( IntPtr a,  string n,  uint val );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_args_set_double( IntPtr a,  string n,  double val );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_args_is_set( IntPtr a,  string n );

  /* ---- Time ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern long rai_time_current_ns();
  [DllImport( Lib, CallingConvention = CC )] internal static extern long rai_time_hires_ns();
  [DllImport( Lib, CallingConvention = CC )] internal static extern long rai_time_hires_to_ns( long hires );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_time_ns_timestamp( long ns,  int precision,  StringBuilder buf,  uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_time_ns_interval( long ns,  StringBuilder buf,  uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_time_strftime( int tz,  long ms,  string fmt,  StringBuilder buf,  uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_time_rotate_set_time( ref TimeRotateState r,  string spec,  int rot_dow,  long rot_time );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_time_rotate_set_period( ref TimeRotateState r,  string spec,  long rot_period );

  /* ---- RaiMsg ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_version();
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_create( int proto,  out IntPtr m );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_msg_delete( IntPtr m );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_reuse( IntPtr m,  int proto );
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_msg_set_protocol( IntPtr m,  int proto );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_msg_get_protocol( IntPtr m );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_protocol_string( IntPtr m );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_clear_form( IntPtr m );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_release( IntPtr m );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_copy( IntPtr m,  IntPtr from );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_type_to_string( ushort msg_type,  StringBuilder buf,  uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern ushort rai_msg_string_to_type( string s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_rec_status_to_string( ushort rec_status,  StringBuilder buf,  uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern ushort rai_string_to_rec_status( string s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_rec_type_to_string( ushort rec_type,  out IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_string_to_rec_type( string s,  out ushort rec_type );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_hdr_string( IntPtr m,  string fname,  out IntPtr s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_set_hdr_string( IntPtr m,  string fname,  string s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_field( IntPtr m,  string name,  IntPtr fld,  out int found );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_bool( IntPtr m,  string n,  out int v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_i8( IntPtr m,  string n,  out sbyte v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_i16( IntPtr m,  string n,  out short v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_i32( IntPtr m,  string n,  out int v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_i64( IntPtr m,  string n,  out long v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_f32( IntPtr m,  string n,  out float v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_f64( IntPtr m,  string n,  out double v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_string( IntPtr m,  string n,  out IntPtr s,  out uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_get_opaque( IntPtr m,  string n,  out IntPtr p,  out uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_bool( IntPtr m,  string n,  int v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_i8( IntPtr m,  string n,  sbyte v,  int is_unsigned );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_i16( IntPtr m,  string n,  short v,  int is_unsigned );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_i32( IntPtr m,  string n,  int v,  int is_unsigned );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_i64( IntPtr m,  string n,  long v,  int is_unsigned );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_f32( IntPtr m,  string n,  float v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_f64( IntPtr m,  string n,  double v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_string( IntPtr m,  string n,  string s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_opaque( IntPtr m,  string n,  byte[] p,  uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_partial( IntPtr m,  string n,  byte[] p,  uint len,  uint offset );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_msg( IntPtr m,  string n,  IntPtr sub );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_append_field( IntPtr m,  IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_bool( IntPtr m,  string n,  int v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_i8( IntPtr m,  string n,  sbyte v,  int is_unsigned );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_i16( IntPtr m,  string n,  short v,  int is_unsigned );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_i32( IntPtr m,  string n,  int v,  int is_unsigned );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_i64( IntPtr m,  string n,  long v,  int is_unsigned );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_f32( IntPtr m,  string n,  float v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_f64( IntPtr m,  string n,  double v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_string( IntPtr m,  string n,  string s );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_opaque( IntPtr m,  string n,  byte[] p,  uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_update_field( IntPtr m,  IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_unpack( IntPtr m,  IntPtr buf,  uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_pack_size( IntPtr m,  out uint size );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_pack( IntPtr m,  IntPtr buf,  uint len,  out uint size );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_packed( IntPtr m,  out IntPtr buf,  out uint size );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_activate( IntPtr m,  string n,  out int ok );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_rename( IntPtr m,  string old_n,  string new_n,  out int ok );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_remove( IntPtr m,  string n,  out int ok );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_print( IntPtr m,  WriteFn fn,  IntPtr cl,  int field_newlines,  string fname_format,  int print_op,  string debug_fmt,  string debug_hfmt );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_print_hex( IntPtr m,  WriteFn fn,  IntPtr cl );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_print_hex_buf( IntPtr buf,  uint len,  WriteFn fn,  IntPtr cl );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_msg_print_xml( IntPtr m,  WriteFn fn,  IntPtr cl,  int attr_flags,  int print_newlines );

  /* ---- RaiField ---- */
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_create();
  [DllImport( Lib, CallingConvention = CC )] internal static extern void rai_field_delete( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_name( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_field_type( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern uint rai_field_size( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_field_hint_type( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern uint rai_field_hint_size( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_field_entry_type( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern uint rai_field_entry_size( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern uint rai_field_num_entries( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern uint rai_field_offset( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern int rai_field_fid( IntPtr f,  out ushort fid );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_data( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_hint_data( IntPtr f );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_bool( IntPtr f,  out int v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_i8( IntPtr f,  out sbyte v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_i16( IntPtr f,  out short v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_i32( IntPtr f,  out int v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_i64( IntPtr f,  out long v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_f32( IntPtr f,  out float v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_f64( IntPtr f,  out double v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_string( IntPtr f,  out IntPtr s,  out uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_msg( IntPtr f,  IntPtr sub );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_entry_i64( IntPtr f,  uint i,  out long v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_entry_f64( IntPtr f,  uint i,  out double v );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_get_entry_string( IntPtr f,  uint i,  out IntPtr s,  out uint len );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_first( IntPtr f,  IntPtr m,  out int more );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_next( IntPtr f,  out int more );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_find( IntPtr f,  IntPtr m,  string name,  out int found );
  [DllImport( Lib, CallingConvention = CC )] internal static extern IntPtr rai_field_type_string( int type );
}

/* Adapts a System.IO.Stream to the rai_write_fn callback used by Print() and
 * Args.addDefaults().  Keep the delegate alive while native code may call. */
internal sealed class WriteAdapter : IDisposable {
  readonly System.IO.Stream stream;
  readonly System.IO.TextWriter writer;
  internal readonly Native.WriteFn Fn;
  GCHandle self;

  internal WriteAdapter( System.IO.Stream s ) {
    this.stream = s; this.Fn = this.Write; this.self = GCHandle.Alloc( this );
  }
  internal WriteAdapter( System.IO.TextWriter w ) {
    this.writer = w; this.Fn = this.Write; this.self = GCHandle.Alloc( this );
  }
  internal IntPtr Closure { get { return GCHandle.ToIntPtr( this.self ); } }

  uint Write( IntPtr cl,  IntPtr buf,  uint len ) {
    try {
      byte[] b = Native.Bytes( buf, len );
      if ( this.stream != null )
        this.stream.Write( b, 0, b.Length );
      else
        this.writer.Write( Encoding.UTF8.GetString( b ) );
      return len;
    } catch ( Exception ) {
      return 0; /* broken pipe */
    }
  }
  public void Dispose() {
    if ( this.stream != null ) this.stream.Flush(); else this.writer.Flush();
    if ( this.self.IsAllocated ) this.self.Free();
  }
}

} /* namespace */
