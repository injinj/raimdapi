#include <jni.h>
#include <string.h>

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <signal.h>
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#include "raiapi/java/com/rai/raiapi2/rai_api_exception_jni.h"
#include "raiapi/java/com/rai/raiapi2/rai_api_jni.h"
#include "raiapi/java/com/rai/raiapi2/args_jni.h"
#include "raiapi/java/com/rai/raiapi2/time_jni.h"
#include "raiapi/java/com/rai/raiapi2/time_rotate_jni.h"
#include "raiapi/java/com/rai/raiapi2/rai_dict_jni.h"
#include "raiapi/java/com/rai/raiapi2/rai_session_jni.h"
#include "raiapi/java/com/rai/raiapi2/rai_queue_jni.h"
#include "raiapi/java/com/rai/raiapi2/rai_subscribe_jni.h"
#include "raiapi/java/com/rai/raiapi2/rai_publish_jni.h"
#include "raiapi/java/com/rai/raiapi2/rai_timer_jni.h"
#include "raiapi/java/com/rai/raiapi2/rai_interactive_publish_jni.h"
#include "raiapi/java/com/rai/raiapi2/rai_entitlement_jni.h"
#include "raiapi2.h"
#include "stream/io_stream.h"
#include "util/str_util.h"
#include "base/thread.h"

#define JaRaiApiException(F) Java_com_rai_raiapi2_RaiApiException_ ## F
#define JaRaiApi(F)          Java_com_rai_raiapi2_RaiApi_ ## F
#define JaArgs(F)            Java_com_rai_raiapi2_Args_ ## F
#define JaTime(F)            Java_com_rai_raiapi2_Time_ ## F
#define JaTimeRotate(F)      Java_com_rai_raiapi2_TimeRotate_ ## F
#define JaRaiDict(F)         Java_com_rai_raiapi2_RaiDict_ ## F
#define JaRaiSession(F)      Java_com_rai_raiapi2_RaiSession_ ## F
#define JaRaiQueue(F)        Java_com_rai_raiapi2_RaiQueue_ ## F
#define JaRaiSubscribe(F)    Java_com_rai_raiapi2_RaiSubscribe_ ## F
#define JaRaiPublish(F)      Java_com_rai_raiapi2_RaiPublish_ ## F
#define JaRaiTimer(F)        Java_com_rai_raiapi2_RaiTimer_ ## F
#define JaRaiInteractivePublish(F) Java_com_rai_raiapi2_RaiInteractivePublish_ ## F
#define JaRaiApi2PATH        "com/rai/raiapi2/"
#define JaRaiMsgPATH         "com/rai/raimsg/"
#define JaRaiExcPATH         "com/rai/raiexception/"

static unsigned int raiJNIEnvKey = rai::Thread::NIL_KEY;

static JavaVM * raiJVM;

static jclass    RaiApiException_cls,
                 RaiMsgException_cls,
                 Exception_cls,
                 RaiApi_cls,
                 RaiDataLossEvent_cls,
                 RaiConnectionEvent_cls,
                 RaiDataLossCallback_cls,
                 RaiMsg_cls,
                 RaiSession_cls,
                 Args_cls,
                 RaiDict_cls,
                 RaiEntitlement_cls,
                 RaiPublish_cls,
                 RaiInteractivePublish_cls,
                 RaiSubscribeCallback_cls,
                 RaiSubscribeEvent_cls,
                 RaiQueue_cls,
                 RaiSubscribe_cls,
                 RaiMsgEvent_cls,
                 RaiMsgCallback_cls,
                 RaiTimer_cls,
                 RaiTimerCallback_cls,
                 StringArg_cls,
                 BoolArg_cls,
                 IntArg_cls,
                 DoubleArg_cls,
                 OutputStream_cls,
                 TimeRotate_cls,
                 String_cls,
                 Boolean_cls,
                 Byte_cls,
                 Short_cls,
                 Int_cls,
                 Long_cls,
                 Float_cls,
                 Double_cls;

static jmethodID RaiApiException_mid,
                 RaiApi_mid,
                 RaiDataLossEvent_mid,
                 RaiConnectionEvent_mid,
                 RaiDataLossCallback_mid,
                 RaiConnectionCallback_mid,
                 RaiMsg_mid,
                 RaiSession_mid,
                 RaiDict_mid,
                 RaiEntitlement_mid,
                 RaiPublish_mid,
                 RaiInteractivePublish_mid,
                 RaiSubscribeCallback_mid,
                 RaiSubscribeEvent_mid,
                 RaiQueue_mid,
                 RaiSubscribe_mid,
                 RaiMsgEvent_mid,
                 RaiMsgEvent_init_mid,
                 RaiMsgCallback_mid,
                 RaiTimer_mid,
                 RaiTimerCallback_mid,
                 write_mid_aB,
                 Exception_mid_toString,
                 boolValue_mid,
                 byteValue_mid,
                 shortValue_mid,
                 intValue_mid,
                 longValue_mid,
                 floatValue_mid,
                 doubleValue_mid;

static jfieldID  RaiApi_fid,
                 RaiMsg_fid,
                 RaiApiException_fid,
                 RaiMsgException_fid,
                 RaiSession_fid,
                 RaiSession_cb_fid,
                 Args_fid,
                 RaiDict_fid,
                 RaiEntitlement_fid,
                 RaiPublish_fid,
                 RaiInteractivePublish_fid,
                 RaiQueue_fid,
                 RaiSubscribe_fid,
                 RaiSubscribe_cb_fid,
                 RaiSubscribe_state_fid,
                 RaiTimer_fid,
                 RaiTimer_cb_fid,
                 StringArg_name_fid,
                 StringArg_defVal_fid,
                 StringArg_example_fid,
                 StringArg_description_fid,
                 BoolArg_name_fid,
                 BoolArg_defVal_fid,
                 BoolArg_example_fid,
                 BoolArg_description_fid,
                 IntArg_name_fid,
                 IntArg_defVal_fid,
                 IntArg_example_fid,
                 IntArg_description_fid,
                 DoubleArg_name_fid,
                 DoubleArg_defVal_fid,
                 DoubleArg_example_fid,
                 DoubleArg_description_fid,
                 TimeRotate_time_fid,
                 TimeRotate_period_fid,
                 TimeRotate_lastTime_fid,
                 TimeRotate_dayOrWeek_fid;

static inline jlong toLong( const void *p ) {
  return (jlong) (ulongptr) p;
}
static inline RaiException toError( jlong p ) {
  return (RaiException) (void *) (ulongptr) p;
}
static inline RaiException toError( JNIEnv *env, jthrowable p ) {
  jlong me;
  if ( env->IsInstanceOf( p, RaiApiException_cls ) )
    me = env->GetLongField( p, RaiApiException_fid );
  else if ( env->IsInstanceOf( p, RaiMsgException_cls ) )
    me = env->GetLongField( p, RaiMsgException_fid );
  else
    me = 0;
  return (RaiException) (void *) (ulongptr) me;
}
struct RaiApiJOutputStream;
struct RaiJArgsEx : public rai::Args {
  char *vs;
  char *ps;
  RaiApiJOutputStream *os;

  SYS_OPS( RaiJArgsEx );
  RaiJArgsEx() : vs( 0 ), ps( 0 ), os( 0 ) {}
  ~RaiJArgsEx();
};
static inline RaiJArgsEx *toArgs( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, Args_fid );
  return (RaiJArgsEx *) (void *) (ulongptr) me;
}
static inline RaiJArgsEx *toArgs( jlong me ) {
  return (RaiJArgsEx *) (void *) (ulongptr) me;
}
static inline RaiApi *toApi( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiApi_fid );
  return (RaiApi *) (void *) (ulongptr) me;
}
static inline RaiApi *toApi( jlong me ) {
  return (RaiApi *) (void *) (ulongptr) me;
}
static inline RaiMsg *toMsg( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiMsg_fid );
  return (RaiMsg *) (void *) (ulongptr) me;
}
static inline RaiSession *toSession( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiSession_fid );
  return (RaiSession *) (void *) (ulongptr) me;
}
static inline RaiSession *toSession( jlong me ) {
  return (RaiSession *) (void *) (ulongptr) me;
}
static inline RaiDict *toDict( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiDict_fid );
  return (RaiDict *) (void *) (ulongptr) me;
}
static inline RaiDict *toDict( jlong me ) {
  return (RaiDict *) (void *) (ulongptr) me;
}
static inline RaiEntitlement *toEntitlement( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiEntitlement_fid );
  return (RaiEntitlement *) (void *) (ulongptr) me;
}
static inline RaiEntitlement *toEntitlement( jlong me ) {
  return (RaiEntitlement *) (void *) (ulongptr) me;
}
static inline RaiPublish *toPublish( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiPublish_fid );
  return (RaiPublish *) (void *) (ulongptr) me;
}
static inline RaiPublish *toPublish( jlong me ) {
  return (RaiPublish *) (void *) (ulongptr) me;
}
static inline RaiInteractivePublish *toInteractivePublish( JNIEnv *env,
                                                           jobject p ) {
  jlong me = env->GetLongField( p, RaiInteractivePublish_fid );
  return (RaiInteractivePublish *) (void *) (ulongptr) me;
}
static inline RaiInteractivePublish *toInteractivePublish( jlong me ) {
  return (RaiInteractivePublish *) (void *) (ulongptr) me;
}
static inline RaiQueue *toQueue( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiQueue_fid );
  return (RaiQueue *) (void *) (ulongptr) me;
}
static inline RaiQueue *toQueue( jlong me ) {
  return (RaiQueue *) (void *) (ulongptr) me;
}
static inline RaiSubscribe *toSubscribe( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiSubscribe_fid );
  return (RaiSubscribe *) (void *) (ulongptr) me;
}
static inline RaiSubscribe *toSubscribe( jlong me ) {
  return (RaiSubscribe *) (void *) (ulongptr) me;
}
struct RaiApiJRaiMsgCallback;
static inline RaiApiJRaiMsgCallback *toMsgCb( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiSubscribe_cb_fid );
  return (RaiApiJRaiMsgCallback *) (void *) (ulongptr) me;
}
static inline RaiTimer *toTimer( JNIEnv *env, jobject p ) {
  jlong me = env->GetLongField( p, RaiTimer_fid );
  return (RaiTimer *) (void *) (ulongptr) me;
}
static inline RaiTimer *toTimer( jlong me ) {
  return (RaiTimer *) (void *) (ulongptr) me;
}

static void
javaException( JNIEnv *env,  const char *msg )
{
  jclass ex = env->FindClass( "java/lang/Exception" );
  if ( ex != NULL )
    env->ThrowNew( ex, msg );
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    initClasses
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiApi( initClasses )( JNIEnv *env, jclass /* me */ )
{
  static const char INIT[] = "<init>";
  jclass cls;

#if defined( _WIN32 ) || defined( _WIN64 )
  HMODULE dllp = ::LoadLibrary( "jraiapi264.dll" );
  if ( dllp == NULL ) {
    dllp = ::LoadLibrary( "jraiapi2.dll" );
  }
#else
  /* map it global, otherwise exceptions won't work */
  void *a = ::dlopen( "libjraiapi264.so", RTLD_NOW | RTLD_GLOBAL );
  if ( a == NULL ) {
    //fprintf( stderr, "dlerror %s\n", dlerror() );
    a = ::dlopen( "libjraiapi2.so", RTLD_NOW | RTLD_GLOBAL );
    //if ( a == NULL )
      //fprintf( stderr, "dlerror %s\n", dlerror() );
  }
#endif
  raiJNIEnvKey = rai::Thread::createSpecificKey();
  rai::Thread::putSpecific( raiJNIEnvKey, env );
  env->GetJavaVM( &raiJVM );

  if ( (cls = env->FindClass( JaRaiApi2PATH "RaiApiException" )) == NULL ||
       (RaiApiException_cls   = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiApiException_mid = env->GetMethodID( RaiApiException_cls, INIT, "(J)V" )) == NULL ||
       (RaiApiException_fid   = env->GetFieldID( RaiApiException_cls, "err", "J" )) == NULL )
    javaException( env, "Can't load RaiApiException class" );

  else if ( (cls = env->FindClass( JaRaiMsgPATH "RaiMsgException" )) == NULL ||
       (RaiMsgException_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiMsgException_fid = env->GetFieldID( RaiMsgException_cls, "err", "J" )) == NULL )
    javaException( env, "Can't load RaiMsgException class" );

  else if ( (cls = env->FindClass( "java/lang/Exception" )) == NULL ||
       (Exception_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (Exception_mid_toString = env->GetMethodID( Exception_cls, "toString", "()Ljava/lang/String;" )) == NULL )
    javaException( env, "Can't load Exception class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiApi" )) == NULL ||
       (RaiApi_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiApi_mid = env->GetMethodID( RaiApi_cls, INIT, "(J)V" )) == NULL ||
       (RaiApi_fid = env->GetFieldID( RaiApi_cls, "api", "J" )) == NULL )
    javaException( env, "Can't load RaiApi class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiDataLossEvent" )) == NULL ||
       (RaiDataLossEvent_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiDataLossEvent_mid = env->GetMethodID( RaiDataLossEvent_cls, INIT, "(Lcom/rai/raiapi2/RaiSession;Ljava/lang/String;Ljava/lang/String;JJJZZ)V" )) == NULL )
    javaException( env, "Can't load RaiDataLossEvent class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiConnectionEvent" )) == NULL ||
       (RaiConnectionEvent_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiConnectionEvent_mid = env->GetMethodID( RaiConnectionEvent_cls, INIT, "(Lcom/rai/raiapi2/RaiSession;Ljava/lang/String;Ljava/lang/String;JZZ)V" )) == NULL )
    javaException( env, "Can't load RaiConnectionEvent class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiDataLossCallback" )) == NULL ||
       (RaiDataLossCallback_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiDataLossCallback_mid = env->GetMethodID( RaiDataLossCallback_cls, "onDataLoss", "(Lcom/rai/raiapi2/RaiDataLossEvent;Ljava/lang/Object;)V" )) == NULL ||
       (RaiConnectionCallback_mid = env->GetMethodID( RaiDataLossCallback_cls, "onConnection", "(Lcom/rai/raiapi2/RaiConnectionEvent;Ljava/lang/Object;)V" )) == NULL )
    javaException( env, "Can't load RaiDataLossCallback class" );

  else if ( (cls = env->FindClass( JaRaiMsgPATH "RaiMsg" )) == NULL ||
       (RaiMsg_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiMsg_mid = env->GetMethodID( RaiMsg_cls, INIT, "(J)V" )) == NULL ||
       (RaiMsg_fid = env->GetFieldID( RaiMsg_cls, "msg", "J" )) == NULL )
    javaException( env, "Can't load RaiMsg class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiSession" )) == NULL ||
       (RaiSession_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiSession_mid = env->GetMethodID( RaiSession_cls, INIT, "(JLcom/rai/raiapi2/RaiApi;)V" )) == NULL ||
       (RaiSession_fid = env->GetFieldID( RaiSession_cls, "session", "J" )) == NULL ||
       (RaiSession_cb_fid = env->GetFieldID( RaiSession_cls, "dataLossCb", "J" )) == NULL )
    javaException( env, "Can't load RaiSession class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "Args" )) == NULL ||
       (Args_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (Args_fid = env->GetFieldID( Args_cls, "args", "J" )) == NULL )
    javaException( env, "Can't load Args class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiDict" )) == NULL ||
       (RaiDict_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiDict_mid = env->GetMethodID( RaiDict_cls, INIT, "(JLcom/rai/raiapi2/RaiSession;)V" )) == NULL ||
       (RaiDict_fid = env->GetFieldID( RaiDict_cls, "dict", "J" )) == NULL )
    javaException( env, "Can't load RaiDict class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiEntitlement" )) == NULL ||
       (RaiEntitlement_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiEntitlement_mid = env->GetMethodID( RaiEntitlement_cls, INIT, "(J)V" )) == NULL ||
       (RaiEntitlement_fid = env->GetFieldID( RaiEntitlement_cls, "entitle", "J" )) == NULL )
    javaException( env, "Can't load RaiEntitlement class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiPublish" )) == NULL ||
       (RaiPublish_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiPublish_mid = env->GetMethodID( RaiPublish_cls, INIT, "(JLcom/rai/raiapi2/RaiSession;)V" )) == NULL ||
       (RaiPublish_fid = env->GetFieldID( RaiPublish_cls, "publish", "J" )) == NULL )
    javaException( env, "Can't load RaiPublish class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiInteractivePublish" )) == NULL ||
       (RaiInteractivePublish_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiInteractivePublish_mid = env->GetMethodID( RaiInteractivePublish_cls, INIT, "(JJLcom/rai/raiapi2/RaiQueue;J)V" )) == NULL ||
       (RaiInteractivePublish_fid = env->GetFieldID( RaiInteractivePublish_cls, "interactive", "J" )) == NULL )
    javaException( env, "Can't load RaiInteractivePublish class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiSubscribeCallback" )) == NULL ||
       (RaiSubscribeCallback_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiSubscribeCallback_mid = env->GetMethodID( RaiSubscribeCallback_cls, "onSubscribe", "(Lcom/rai/raiapi2/RaiSubscribeEvent;Lcom/rai/raimsg/RaiMsg;Ljava/lang/Object;)V" )) == NULL )
    javaException( env, "Can't load RaiSubscribeCallback class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiSubscribeEvent" )) == NULL ||
       (RaiSubscribeEvent_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiSubscribeEvent_mid = env->GetMethodID( RaiSubscribeEvent_cls, INIT, "(Lcom/rai/raiapi2/RaiInteractivePublish;Ljava/lang/String;Ljava/lang/String;I)V" )) == NULL )
    javaException( env, "Can't load RaiSubscribeEvent class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiQueue" )) == NULL ||
       (RaiQueue_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiQueue_mid = env->GetMethodID( RaiQueue_cls, INIT, "(JLcom/rai/raiapi2/RaiSession;)V" )) == NULL ||
       (RaiQueue_fid = env->GetFieldID( RaiQueue_cls, "queue", "J" )) == NULL )
    javaException( env, "Can't load RaiQueue class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiSubscribe" )) == NULL ||
       (RaiSubscribe_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiSubscribe_mid = env->GetMethodID( RaiSubscribe_cls, INIT, "(JJLcom/rai/raiapi2/RaiQueue;)V" )) == NULL ||
       (RaiSubscribe_fid = env->GetFieldID( RaiSubscribe_cls, "subscribe", "J" )) == NULL ||
       (RaiSubscribe_cb_fid = env->GetFieldID( RaiSubscribe_cls, "msgCb", "J" )) == NULL ||
       (RaiSubscribe_state_fid = env->GetFieldID( RaiSubscribe_cls, "state", "I" )) == NULL )
    javaException( env, "Can't load RaiSubscribe class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiMsgEvent" )) == NULL ||
       (RaiMsgEvent_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiMsgEvent_mid = env->GetMethodID( RaiMsgEvent_cls, INIT, "(Lcom/rai/raiapi2/RaiSubscribe;Ljava/lang/String;ISSIIIJJJ)V" )) == NULL ||
       (RaiMsgEvent_init_mid = env->GetMethodID( RaiMsgEvent_cls, "init", "(Lcom/rai/raiapi2/RaiSubscribe;Ljava/lang/String;ISSIIIJJJ)V" )) == NULL )
    javaException( env, "Can't load RaiMsgEvent class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiMsgCallback" )) == NULL ||
       (RaiMsgCallback_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiMsgCallback_mid = env->GetMethodID( RaiMsgCallback_cls, "onMsg", "(Lcom/rai/raiapi2/RaiMsgEvent;Lcom/rai/raimsg/RaiMsg;Ljava/lang/Object;)V" )) == NULL )
    javaException( env, "Can't load RaiMsgCallback class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiTimer" )) == NULL ||
       (RaiTimer_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiTimer_mid = env->GetMethodID( RaiTimer_cls, INIT, "(JJLcom/rai/raiapi2/RaiQueue;)V" )) == NULL ||
       (RaiTimer_fid = env->GetFieldID( RaiTimer_cls, "timer", "J" )) == NULL ||
       (RaiTimer_cb_fid = env->GetFieldID( RaiTimer_cls, "timerCb", "J" )) == NULL )
    javaException( env, "Can't load RaiTimer class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "RaiTimerCallback" )) == NULL ||
       (RaiTimerCallback_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiTimerCallback_mid = env->GetMethodID( RaiTimerCallback_cls, "onTimer", "(Lcom/rai/raiapi2/RaiTimer;Ljava/lang/Object;)V" )) == NULL )
    javaException( env, "Can't load RaiTimerCallback class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "StringArg" )) == NULL ||
       (StringArg_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (StringArg_name_fid = env->GetFieldID( StringArg_cls, "name", "Ljava/lang/String;" )) == NULL ||
       (StringArg_defVal_fid = env->GetFieldID( StringArg_cls, "defVal", "Ljava/lang/String;" )) == NULL ||
       (StringArg_example_fid = env->GetFieldID( StringArg_cls, "example", "Ljava/lang/String;" )) == NULL ||
       (StringArg_description_fid = env->GetFieldID( StringArg_cls, "description", "Ljava/lang/String;" )) == NULL )
    javaException( env, "Can't load StringArg class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "BoolArg" )) == NULL ||
       (BoolArg_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (BoolArg_name_fid = env->GetFieldID( BoolArg_cls, "name", "Ljava/lang/String;" )) == NULL ||
       (BoolArg_defVal_fid = env->GetFieldID( BoolArg_cls, "defVal", "Z" )) == NULL ||
       (BoolArg_example_fid = env->GetFieldID( BoolArg_cls, "example", "Ljava/lang/String;" )) == NULL ||
       (BoolArg_description_fid = env->GetFieldID( BoolArg_cls, "description", "Ljava/lang/String;" )) == NULL )
    javaException( env, "Can't load BoolArg class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "IntArg" )) == NULL ||
       (IntArg_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (IntArg_name_fid = env->GetFieldID( IntArg_cls, "name", "Ljava/lang/String;" )) == NULL ||
       (IntArg_defVal_fid = env->GetFieldID( IntArg_cls, "defVal", "I" )) == NULL ||
       (IntArg_example_fid = env->GetFieldID( IntArg_cls, "example", "Ljava/lang/String;" )) == NULL ||
       (IntArg_description_fid = env->GetFieldID( IntArg_cls, "description", "Ljava/lang/String;" )) == NULL )
    javaException( env, "Can't load IntArg class" );

  else if ( (cls = env->FindClass( JaRaiApi2PATH "DoubleArg" )) == NULL ||
       (DoubleArg_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (DoubleArg_name_fid = env->GetFieldID( DoubleArg_cls, "name", "Ljava/lang/String;" )) == NULL ||
       (DoubleArg_defVal_fid = env->GetFieldID( DoubleArg_cls, "defVal", "D" )) == NULL ||
       (DoubleArg_example_fid = env->GetFieldID( DoubleArg_cls, "example", "Ljava/lang/String;" )) == NULL ||
       (DoubleArg_description_fid = env->GetFieldID( DoubleArg_cls, "description", "Ljava/lang/String;" )) == NULL )
    javaException( env, "Can't load DoubleArg class" );

  else if ( (cls = env->FindClass( "java/io/OutputStream" )) == NULL ||
       (OutputStream_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (write_mid_aB = env->GetMethodID( OutputStream_cls, "write", "([B)V" )) == NULL )
    javaException( env, "Can't load OutputStream class" );

  else if ( (cls = env->FindClass( "com/rai/raiapi2/TimeRotate" )) == NULL ||
       (TimeRotate_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (TimeRotate_time_fid = env->GetFieldID( TimeRotate_cls, "time", "J" )) == NULL ||
       (TimeRotate_period_fid = env->GetFieldID( TimeRotate_cls, "period", "J" )) == NULL ||
       (TimeRotate_lastTime_fid = env->GetFieldID( TimeRotate_cls, "lastTime", "J" )) == NULL ||
       (TimeRotate_dayOrWeek_fid = env->GetFieldID( TimeRotate_cls, "dayOrWeek", "I" )) == NULL )
    javaException( env, "Can't load TimeRotate class" );

  else if ( (cls = env->FindClass( "java/lang/String" )) == NULL ||
       (String_cls = (jclass) env->NewGlobalRef( cls )) == NULL )
    javaException( env, "Can't load String class" );

   else if ( (cls = env->FindClass( "java/lang/Boolean" )) == NULL ||
       (Boolean_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (boolValue_mid = env->GetMethodID( Boolean_cls, "booleanValue", "()Z" )) == NULL )
    javaException( env, "Can't load Boolean class" );

   else if ( (cls = env->FindClass( "java/lang/Byte" )) == NULL ||
       (Byte_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (byteValue_mid = env->GetMethodID( Byte_cls, "byteValue", "()B" )) == NULL )
    javaException( env, "Can't load Byte class" );

   else if ( (cls = env->FindClass( "java/lang/Short" )) == NULL ||
       (Short_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (shortValue_mid = env->GetMethodID( Short_cls, "shortValue", "()S" )) == NULL )
    javaException( env, "Can't load Short class" );

   else if ( (cls = env->FindClass( "java/lang/Integer" )) == NULL ||
       (Int_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (intValue_mid = env->GetMethodID( Int_cls, "intValue", "()I" )) == NULL )
    javaException( env, "Can't load Int class" );

   else if ( (cls = env->FindClass( "java/lang/Long" )) == NULL ||
       (Long_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (longValue_mid = env->GetMethodID( Long_cls, "longValue", "()J" )) == NULL )
    javaException( env, "Can't load Long class" );

   else if ( (cls = env->FindClass( "java/lang/Float" )) == NULL ||
       (Float_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (floatValue_mid = env->GetMethodID( Float_cls, "floatValue", "()F" )) == NULL )
    javaException( env, "Can't load Float class" );

   else if ( (cls = env->FindClass( "java/lang/Double" )) == NULL ||
       (Double_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (doubleValue_mid = env->GetMethodID( Double_cls, "doubleValue", "()D" )) == NULL )
    javaException( env, "Can't load Double class" );
}

static void
throwError( JNIEnv *env,  RaiException e )
{
  jthrowable thr;
  thr = (jthrowable) env->NewObject( RaiApiException_cls, RaiApiException_mid,
                                     toLong( e ) );
  env->Throw( thr );
}


struct DetachJNI : public rai::ThreadOnExit {
  DetachJNI() {}
  virtual ~DetachJNI() {}

  virtual void onExit( void ) {
    if ( raiJVM != NULL )
      raiJVM->DetachCurrentThread();
  }
};


static JNIEnv *
getJNIEnv( void )
{
  JNIEnv *env = (JNIEnv *) rai::Thread::getSpecific( raiJNIEnvKey );
  if ( env == NULL ) {
    static DetachJNI jniExit;
    if ( rai::Thread::self() == NULL )
      rai::Thread::createExternalThread( "rai-unnamed" );
    JavaVMAttachArgs args = {
      JNI_VERSION_1_4, rai::Thread::self()->name, NULL
    };
    raiJVM->AttachCurrentThreadAsDaemon( (void **) &env, (void *) &args );
    rai::Thread::putSpecific( raiJNIEnvKey, env );
    rai::Thread::onExit( &jniExit );
  }
  return env;
}


/*** RaiApiException ***/
/*
 * Class:     com_rai_raiapi2_RaiApiException
 * Method:    getModule
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiApiException( getModule )( JNIEnv *env, jclass /* me */, jlong err )
{
  if ( err == 0 )
    return NULL;
  return env->NewStringUTF( toError( err )->module );
}

/*
 * Class:     com_rai_raiapi2_RaiApiException
 * Method:    getErrno
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiApiException( getErrno )( JNIEnv */* env */, jclass /* me */, jlong err)
{
  if ( err == 0 )
    return 0;
  return toError( err )->status;
}

/*
 * Class:     com_rai_raiapi2_RaiApiException
 * Method:    getReason
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiApiException( getReason )( JNIEnv *env, jclass /* me */, jlong err )
{
  if ( err == 0 )
    return NULL;
  return env->NewStringUTF( toError( err )->reason );
}

/*** RaiApi ***/
/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiApi( Delete )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  RaiApi *api = toApi( me );
  if ( api != NULL )
    delete api;
}


static jclass    Interrupt_cls;
static jmethodID Interrupt_mid_handler;

#if defined( _WIN32 ) || defined( _WIN64 )
static BOOL
jniCtrlHandler( DWORD ctrlType )
{
  if ( Interrupt_cls != NULL &&
       Interrupt_mid_handler != NULL && raiJVM != NULL ) {
    JNIEnv *env = getJNIEnv();
    if ( env != NULL ) {
      env->CallStaticVoidMethod( Interrupt_cls, Interrupt_mid_handler,
                                 (jint) ctrlType );  
    }
  }
  return TRUE;
}
#else
static void
jniInterruptHandler( int sig )
{
  if ( Interrupt_cls != NULL &&
       Interrupt_mid_handler != NULL && raiJVM != NULL ) {
    JNIEnv *env = getJNIEnv();
    if ( env != NULL ) {
      env->CallStaticVoidMethod( Interrupt_cls, Interrupt_mid_handler,
                                 (jint) sig );  
    }
  }
}
#endif
/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    RegisterSigHandler
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiApi( RegisterSigHandler )( JNIEnv *env, jclass /* cls */,
                                jstring clsNm, jstring methNm )
{
  static const rai::ErrorRec jerr[] = {
    { 0, "Unable to load signal handler class or method", "JRaiApi" } };
  jclass cls2;
  RaiException e2 = NULL;
  try {
    rai::Sys::initialize();
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( e2 == NULL ) {
    const char *className = env->GetStringUTFChars( clsNm, NULL ),
               *methName  = env->GetStringUTFChars( methNm, NULL );
    if ( (cls2 = env->FindClass( className )) == NULL ||
         (Interrupt_cls = (jclass) env->NewGlobalRef( cls2 )) == NULL ||
         (Interrupt_mid_handler = env->GetStaticMethodID( Interrupt_cls,
                                                 methName, "(I)V" )) == NULL ) {
      e2 = &jerr[ 0 ];
    }
    else {
#if defined( _WIN32 ) || defined( _WIN64 )
     ::SetConsoleCtrlHandler( (PHANDLER_ROUTINE) jniCtrlHandler, TRUE );
#else
      struct sigaction nsa;

      /* do this before threads are created so that they inherit the sigs */
      ::memset( &nsa, 0, sizeof( nsa ) );
      ::sigemptyset( &nsa.sa_mask );
      nsa.sa_handler = ::jniInterruptHandler;
      ::sigaction( SIGHUP, &nsa, NULL );
      ::sigaction( SIGINT, &nsa, NULL );
      ::sigaction( SIGTERM, &nsa, NULL );
#endif
    }
    env->ReleaseStringUTFChars( clsNm, className );
    env->ReleaseStringUTFChars( methNm, methName );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    SetIoctl
 * Signature: (Ljava/lang/String;Ljava/lang/Object;)Z
 */
JNIEXPORT jboolean JNICALL
JaRaiApi( SetIoctl )( JNIEnv *env,  jobject me,  jstring parameter,
                      jobject value )
{
  const char *param = env->GetStringUTFChars( parameter, NULL );
  bool b = false;

  if ( env->IsInstanceOf( value, String_cls ) ) {
    const char *val = ( value ?
                       env->GetStringUTFChars( (jstring) value, NULL ) : NULL );
    b = toApi( env, me )->SetIoctl( param, val );
    if ( val != NULL )
      env->ReleaseStringUTFChars( (jstring) value, val );
  }
  else if ( env->IsInstanceOf( value, Boolean_cls ) ) {
    bool v = ( env->CallBooleanMethod( value, boolValue_mid ) != 0 );
    b = toApi( env, me )->SetIoctl( param, &v );
  }
  else if ( env->IsInstanceOf( value, Byte_cls ) ) {
    byte v = (byte) env->CallByteMethod( value, byteValue_mid );
    b = toApi( env, me )->SetIoctl( param, &v );
  }
  else if ( env->IsInstanceOf( value, Short_cls ) ) {
    short v = (short) env->CallShortMethod( value, shortValue_mid );
    b = toApi( env, me )->SetIoctl( param, &v );
  }
  else if ( env->IsInstanceOf( value, Int_cls ) ) {
    int v = (int) env->CallIntMethod( value, intValue_mid );
    b = toApi( env, me )->SetIoctl( param, &v );
  }
  else if ( env->IsInstanceOf( value, Long_cls ) ) {
    long v = (long) env->CallLongMethod( value, longValue_mid );
    b = toApi( env, me )->SetIoctl( param, &v );
  }
  else if ( env->IsInstanceOf( value, Float_cls ) ) {
    float v = (float) env->CallFloatMethod( value, floatValue_mid );
    b = toApi( env, me )->SetIoctl( param, &v );
  }
  else if ( env->IsInstanceOf( value, Double_cls ) ) {
    double v = (double) env->CallDoubleMethod( value, doubleValue_mid );
    b = toApi( env, me )->SetIoctl( param, &v );
  }

  env->ReleaseStringUTFChars( parameter, param );
  return b;
}


struct RaiApiJArgv {
  int     argc;
  char ** argv;
  void operator delete( void *p ) { FREE( p ); }
};


static RaiApiJArgv *
getArgv( JNIEnv *env,  jobjectArray argv )
{
  RaiApiJArgv * args;
  jstring       s;
  const char  * s2;
  char        * p;
  jsize         i,
                argc = ( argv == NULL ? 0 : env->GetArrayLength( argv ) ) + 1;
  unsigned int  len = sizeof( RaiApiJArgv ) + 5 +
                      sizeof( char * ) * ( argc + 1 );

  for ( i = 1; i < argc; i++ ) {
    env->PushLocalFrame( 2 );
    s = (jstring) env->GetObjectArrayElement( argv, i - 1 );
    if ( s != NULL ) {
      s2 = env->GetStringUTFChars( s, NULL );
      if ( s2 != NULL ) {
        len += ::strlen( s2 ) + 1;
        env->ReleaseStringUTFChars( s, s2 );
      }
    }
    env->PopLocalFrame( NULL );
    if ( env->ExceptionCheck() )
      return NULL;
  }
  argc = i;
  MALLOC( len, &args );
  args->argv = (char **) (void *) &args[ 1 ];
  p = (char *) (void *) &args->argv[ argc + 1 ];
  args->argv[ 0 ] = p;
  ::strcpy( p, "java" );
  p = &p[ 5 ];
  
  for ( i = 1; i < argc; i++ ) {
    args->argv[ i ] = NULL;
    env->PushLocalFrame( 2 );
    s = (jstring) env->GetObjectArrayElement( argv, i - 1 );
    if ( s != NULL ) {
      s2 = env->GetStringUTFChars( s, NULL );
      if ( s2 != NULL ) {
        args->argv[ i ] = p;
        ::strcpy( p, s2 );
        p = &p[ ::strlen( s2 ) + 1 ];
        env->ReleaseStringUTFChars( s, s2 );
      }
    }
    env->PopLocalFrame( NULL );
    if ( env->ExceptionCheck() ) {
      FREE( args );
      return NULL;
    }
  }
  args->argc = i;
  args->argv[ i ] = NULL;
  return args;
}


/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    RaiOpen
 * Signature: (Ljava/lang/String;[Ljava/lang/String;)Lcom/rai/raiapi2/RaiApi;
 */
JNIEXPORT jobject JNICALL
JaRaiApi( RaiOpen )( JNIEnv *env, jclass /* me */, jstring apiName,
                     jobjectArray argv )
{
  RaiApi      * api    = NULL;
  jobject       apiObj = NULL;
  RaiApiJArgv * args   = NULL;
  const char  * name   = NULL;
  RaiException  e2     = NULL;

  try {
    rai::Sys::initialize();
    args = getArgv( env, argv );
  } catch ( RaiException e ) {
    e2 = e;
  }

  if ( e2 == NULL ) {
    if ( apiName != NULL )
      name = env->GetStringUTFChars( apiName, NULL );
    try {
      api = RaiApi::RaiOpen( name, args->argc, args->argv );
    } catch ( RaiException e ) {
      e2 = e;
    }
    if ( name != NULL )
      env->ReleaseStringUTFChars( apiName, name );
  }

  if ( e2 == NULL )
    apiObj = env->NewObject( RaiApi_cls, RaiApi_mid, toLong( api ) );
  if ( args != NULL )
    delete args;
  /* not releasing args[] */
  if ( e2 != NULL )
    throwError( env, e2 );
  return apiObj;
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    GetApiName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiApi( GetApiName )( JNIEnv *env, jobject me )
{
  return env->NewStringUTF( toApi( env, me )->GetApiName() );
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    RaiVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiApi( RaiVersion )( JNIEnv *env, jclass /* me */ )
{
  return env->NewStringUTF( RaiApi::RaiVersion() );
}

/*
 * Class:     com_rai_raiapi2_Time
 * Method:    currentTimeNanosecs
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
JaTime( currentTimeNanosecs )( JNIEnv */* env */, jclass /* me */ )
{
  return (jlong) rai::Time::currentTimeNanosecs();
}

/*
 * Class:     com_rai_raiapi2_Time
 * Method:    nsTimestamp
 * Signature: (JI)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaTime( nsTimestamp )( JNIEnv *env, jclass /* me */, jlong ns, jint precision )
{
  char ts[ 32 ];
  rai::TimeNSecs n = (rai::TimeNSecs) ns;
  if ( n == 0 )
    n = rai::Time::currentTimeNanosecs();
  rai::Time::timestamp( n, (unsigned int) precision, ts, sizeof( ts ) );
  return env->NewStringUTF( ts );
}

/*
 * Class:     com_rai_raiapi2_Time
 * Method:    nsIntervalTime
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaTime( nsIntervalTime )( JNIEnv *env, jclass /* me */, jlong ns )
{
  char buf[ 32 ];
  rai::StrUtil::intToString( (llong) ns, buf, sizeof( buf ), rai::U_NANOSECS );
  return env->NewStringUTF( buf );
}


/*
 * Class:     com_rai_raiapi2_Time
 * Method:    hiresTimeNanosecs
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
JaTime( hiresTimeNanosecs )( JNIEnv */* env */, jclass /* me */ )
{
  double         cpms;
  rai::TimeHires h = rai::Time::getHiresTime( &cpms );

  if ( cpms == 1000000.0 )
    return h;
  else if ( cpms > 1000000.0 )
    return (jlong) ( (double) h / ( cpms / 1000000.0 ) );
  return (jlong) ( (double) h * ( 1000000.0 / cpms ) );
}

/*
 * Class:     com_rai_raiapi2_Time
 * Method:    hiresTimeToNsTimestamp
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
JaTime( hiresTimeToNsTimestamp )( JNIEnv */* env */, jclass /* me */, jlong h )
{
  double cpms = rai::Time::getCyclesPerMSec();

  if ( cpms == 1000000.0 )
    return rai::Time::hiresToNanosecs( (rai::TimeHires) h );
  else if ( cpms > 1000000.0 )
    return rai::Time::hiresToNanosecs(
      (rai::TimeHires) ( (double) h * ( cpms / 1000000.0 ) ) );
  return rai::Time::hiresToNanosecs(
    (rai::TimeHires) ( (double) h / ( 1000000.0 / cpms ) ) );
}


/*
 * Class:     com_rai_raiapi2_Time
 * Method:    strftime
 * Signature: (IJLjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaTime( strftime )( JNIEnv *env, jclass /* me */, jint tz, jlong ms,
                    jstring fmt )
{
  const char * sfmt = env->GetStringUTFChars( fmt, NULL );
  RaiException e2   = NULL;
  char         buf[ 1024 ];

  try {
    rai::Time::strftime( tz, (rai::TimeMSecs) ms, sfmt, buf, sizeof( buf ) );
  } catch ( RaiException e ) {
    e2 = e;
  }
  env->ReleaseStringUTFChars( fmt, sfmt );
  if ( e2 != NULL ) {
    throwError( env, e2 );
    return NULL;
  }
  return env->NewStringUTF( buf );
}

/*
 * Class:     com_rai_raiapi2_TimeRotate
 * Method:    setRotateTime
 * Signature: (Ljava/lang/String;IJ)Z
 */
JNIEXPORT jboolean JNICALL
JaTimeRotate( setRotateTime )(JNIEnv *env, jobject me, jstring spec,
                              jint rotDorW, jlong rotTime )
{
  jlong t = env->GetLongField( me, TimeRotate_time_fid ),
        p = env->GetLongField( me, TimeRotate_period_fid ),
        l = env->GetLongField( me, TimeRotate_lastTime_fid );
  jint  d = env->GetIntField( me, TimeRotate_dayOrWeek_fid );
  const char *timeSpec = ( spec ? env->GetStringUTFChars( spec, NULL ) : NULL );
  rai::TimeRotate r;
  r.time      = t;
  r.period    = p;
  r.lastTime  = l;
  r.dayOrWeek = (rai::TimeRotate::DayOrWeek) d;
  
  bool res = r.setRotateTime( timeSpec, (rai::TimeRotate::DayOrWeek) rotDorW,
                              (rai::TimeMSecs) rotTime );
  if ( spec != NULL )
    env->ReleaseStringUTFChars( spec, timeSpec );
  env->SetLongField( me, TimeRotate_time_fid, (jlong) r.time ),
  env->SetLongField( me, TimeRotate_period_fid, (jlong) r.period ),
  env->SetLongField( me, TimeRotate_lastTime_fid, (jlong) r.lastTime ),
  env->SetIntField( me, TimeRotate_dayOrWeek_fid, (jint) r.dayOrWeek );

  return (jboolean) res;
}

/*
 * Class:     com_rai_raiapi2_TimeRotate
 * Method:    setRotatePeriod
 * Signature: (Ljava/lang/String;J)Z
 */
JNIEXPORT jboolean JNICALL
JaTimeRotate( setRotatePeriod )(JNIEnv *env, jobject me, jstring spec,
                                jlong rotPer )
{
  jlong t = env->GetLongField( me, TimeRotate_time_fid ),
        p = env->GetLongField( me, TimeRotate_period_fid ),
        l = env->GetLongField( me, TimeRotate_lastTime_fid );
  jint  d = env->GetIntField( me, TimeRotate_dayOrWeek_fid );
  const char *timeSpec = ( spec ? env->GetStringUTFChars( spec, NULL ) : NULL );
  rai::TimeRotate r;
  r.time      = t;
  r.period    = p;
  r.lastTime  = l;
  r.dayOrWeek = (rai::TimeRotate::DayOrWeek) d;
  
  bool res = r.setRotatePeriod( timeSpec, (rai::TimeMSecs) rotPer );

  if ( spec != NULL )
    env->ReleaseStringUTFChars( spec, timeSpec );
  env->SetLongField( me, TimeRotate_time_fid, (jlong) r.time ),
  env->SetLongField( me, TimeRotate_period_fid, (jlong) r.period ),
  env->SetLongField( me, TimeRotate_lastTime_fid, (jlong) r.lastTime ),
  env->SetIntField( me, TimeRotate_dayOrWeek_fid, (jint) r.dayOrWeek );

  return (jboolean) res;
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    GetArgs
 * Signature: (Lcom/rai/raiapi2/Args;)V
 */
JNIEXPORT void JNICALL
JaRaiApi( GetArgs )( JNIEnv *env, jobject me, jobject args )
{
  try {
    toApi( env, me )->GetArgs( *toArgs( env, args ) );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    GetDictArgs
 * Signature: (Lcom/rai/raiapi2/Args;)V
 */
JNIEXPORT void JNICALL
JaRaiApi( GetDictArgs )( JNIEnv *env, jclass me, jobject args )
{
  try {
    toApi( env, me )->GetDictArgs( *toArgs( env, args ) );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    ParseArgs
 * Signature: (Lcom/rai/raiapi2/Args;)V
 */
JNIEXPORT void JNICALL
JaRaiApi( ParseArgs )( JNIEnv *env, jobject me, jobject args )
{
  try {
    toApi( env, me )->ParseArgs( *toArgs( env, args ) );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    OpenLog
 * Signature: (Lcom/rai/raiapi2/Args;)Z
 */
JNIEXPORT jboolean JNICALL
JaRaiApi( OpenLog__Lcom_rai_raiapi2_Args_2 )( JNIEnv *env, jclass /* me */, jobject args )
{
  bool res = false;
  try {
    res = RaiApi::OpenLog( *toArgs( env, args ) );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return res;
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    OpenLog
 * Signature: (Lcom/rai/raiapi2/Args;)Z
 */
JNIEXPORT void JNICALL
JaRaiApi( OpenLog__Ljava_lang_String_2II )( JNIEnv *env, jclass /* me */, jstring s,
                                            jint lvl, jint verb )
{
  const char * str = ( s ? env->GetStringUTFChars( s, NULL ) : NULL );
  RaiException e2  = NULL;
  try {
    rai::Sys::initialize();
    rai::Log::openLog( str, (rai::Log::LogLevel) lvl, verb );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( str != NULL )
    env->ReleaseStringUTFChars( s, str );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    PrintLog
 * Signature: (ILcom/rai/raiexception/RaiException;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiApi( PrintLog )( JNIEnv *env,  jobject me,  jint level,  jthrowable err,
                      jstring s )
{
  const char * str = ( s ? env->GetStringUTFChars( s, NULL ) : NULL );
  if ( err != NULL ) {
    RaiException e = toError( env, err );
    if ( e != NULL ) {
      toApi( env, me )->PrintLog( (rai::Log::LogLevel) (int) level,
                                   __FILE__, __LINE__, e, "%s", str );
      goto done;
    }
    else if ( env->IsInstanceOf( err, Exception_cls ) ) {
      jstring      s1 = NULL;
      const char * s2 = NULL;
      env->PushLocalFrame( 2 );
      s1 = (jstring)
        env->CallObjectMethodV( err, Exception_mid_toString, NULL );  
      if ( s1 != NULL ) {
        s2 = env->GetStringUTFChars( s1, NULL );
        if ( s2 != NULL ) {
          toApi( env, me )->PrintLog( (rai::Log::LogLevel) (int) level,
                                       __FILE__, __LINE__, NULL, "%s; %s",
                                       s2, str );
          env->ReleaseStringUTFChars( s1, s2 );
        }
      }
      env->PopLocalFrame( NULL );
      if ( s2 != NULL )
        goto done;
    }
  }
  toApi( env, me )->PrintLog( (rai::Log::LogLevel) (int) level,
                               __FILE__, __LINE__, NULL, "%s", str );
done:;
  if ( str != NULL )
    env->ReleaseStringUTFChars( s, str );
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    Log
 * Signature: (ILjava/lang/Exception;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiApi( Log )( JNIEnv *env, jclass /* cls */, jint level, jthrowable err, jstring s)
{
  const char * str = ( s ? env->GetStringUTFChars( s, NULL ) : NULL );
  if ( err != NULL ) {
    RaiException e = toError( env, err );
    if ( e != NULL ) {
      rai::Log::printLog( (rai::Log::LogLevel) (int) level,
                          __FILE__, __LINE__, e, "%s", str );
      goto done;
    }
    else if ( env->IsInstanceOf( err, Exception_cls ) ) {
      jstring      s1 = NULL;
      const char * s2 = NULL;
      env->PushLocalFrame( 2 );
      s1 = (jstring)
        env->CallObjectMethodV( err, Exception_mid_toString, NULL );  
      if ( s1 != NULL ) {
        s2 = env->GetStringUTFChars( s1, NULL );
        if ( s2 != NULL ) {
          rai::Log::printLog( (rai::Log::LogLevel) (int) level,
                              __FILE__, __LINE__, e, "%s; %s", s2, str );
          env->ReleaseStringUTFChars( s1, s2 );
        }
      }
      env->PopLocalFrame( NULL );
      if ( s2 != NULL )
        goto done;
    }
  }
  rai::Log::printLog( (rai::Log::LogLevel) (int) level,
                      __FILE__, __LINE__, NULL, "%s", str );
done:;
  if ( str != NULL )
    env->ReleaseStringUTFChars( s, str );
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    OpenDict
 * Signature: (Lcom/rai/raiapi2/Args;)Z
 */
JNIEXPORT jboolean JNICALL
JaRaiApi( OpenDict )( JNIEnv *env, jclass /* me */, jobject args )
{
  bool res = false;
  try {
    res = RaiApi::OpenDict( *toArgs( env, args ) );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return res;
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    CreateSession
 * Signature: ()Lcom/rai/raiapi2/RaiSession;
 */
JNIEXPORT jobject JNICALL
JaRaiApi( CreateSession )( JNIEnv *env, jobject me )
{
  RaiApi     & api  = *toApi( env, me );
  RaiSession * sess = NULL;
  
  try {
    sess = api.CreateSession();
    return env->NewObject( RaiSession_cls, RaiSession_mid, toLong( sess ), me );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return NULL;
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    NewRaiMsg
 * Signature: (ISSSS)Lcom/rai/raimsg/RaiMsg;
 */
JNIEXPORT jobject JNICALL
JaRaiApi( NewRaiMsg__ISSSS )( JNIEnv *env, jclass /* me */, jint proto,
                              jshort MsgType, jshort RecType, jshort SeqNo,
                              jshort RecStatus )
{
  RaiMsg * msg;
  try {
    msg = RaiApi::NewRaiMsg( (RaiMsg_protocol) proto, (Rai_u16) MsgType,
                             (Rai_u16) RecType, (Rai_u16) SeqNo,
                             (Rai_u16) RecStatus );
    return env->NewObject( RaiMsg_cls, RaiMsg_mid, toLong( msg ) );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return NULL;
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    NewRaiMsg
 * Signature: (ISLjava/lang/String;SS)Lcom/rai/raimsg/RaiMsg;
 */
JNIEXPORT jobject JNICALL
JaRaiApi( NewRaiMsg__ISLjava_lang_String_2SS )( JNIEnv *env, jclass /* me */,
                       jint proto, jshort MsgType, jstring FormType,
                       jshort SeqNo, jshort RecStatus )
{
  RaiMsg     * msg;
  jobject      obj = NULL;
  RaiException e2  = NULL;
  const char * formTypeString =
    ( FormType ? env->GetStringUTFChars( FormType, NULL ) : NULL );
  try {
    msg = RaiApi::NewRaiMsg( (RaiMsg_protocol) proto, (Rai_u16) MsgType,
                             formTypeString, (Rai_u16) SeqNo,
                             (Rai_u16) RecStatus );
    obj = env->NewObject( RaiMsg_cls, RaiMsg_mid, toLong( msg ) );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( formTypeString != NULL )
    env->ReleaseStringUTFChars( FormType, formTypeString );
  if ( e2 != NULL )
    throwError( env, e2 );
  return obj;
}

/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    Close
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiApi( Close )( JNIEnv *env, jobject me )
{
  RaiApi &api = *toApi( env, me );

  try {
    api.Close();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

struct RaiApiJOutputStream : public rai::OutputStream {
  jobject out;

  virtual unsigned int emptyBuf( const byte *buf,
                                 unsigned int bufLen ) {
    if ( this->out == NULL )
      return bufLen;

    RaiException e = NULL;
    JNIEnv     * env = getJNIEnv();
    jbyteArray   barr;

    env->PushLocalFrame( 1 );
    if ( (barr = env->NewByteArray( bufLen )) == NULL ) {
      e = rai::IOStreamErr::getErr( rai::IOStreamErr::BROKEN_PIPE );
    }
    else {
      env->SetByteArrayRegion( barr, 0, bufLen, (jbyte *) buf );
      env->CallVoidMethod( this->out, write_mid_aB, barr );
      if ( env->ExceptionCheck() ) {
        env->ExceptionClear();
        e = rai::IOStreamErr::getErr( rai::IOStreamErr::BROKEN_PIPE );
      }
    }
    env->PopLocalFrame( NULL );
    if ( e != NULL )
      throw e;
    return bufLen;
  }

  SYS_OPS( RaiApiJOutputStream );
  RaiApiJOutputStream( jobject out,  bool linebuff ) :
      rai::OutputStream( 1024, linebuff, false, 0 ) {
    this->out = out;
  }

  virtual ~RaiApiJOutputStream() {}

  void jniRelease( JNIEnv *env ) {
    if ( this->out != NULL ) {
      jobject obj = this->out;
      this->out = NULL;
      env->DeleteGlobalRef( obj );
    }
  }
};


/*** Args ***/
/*
 * Class:     com_rai_raiapi2_RaiApi
 * Method:    Delete
 * Signature: (J)V
 */
RaiJArgsEx::~RaiJArgsEx() {
  this->clear();
  if ( this->vs != NULL )
    FREE( this->vs );
  if ( this->ps != NULL )
    FREE( this->ps );
  if ( this->os != NULL )
    delete this->os;
}

JNIEXPORT void JNICALL
JaArgs( Delete )( JNIEnv *env, jclass /* cls */, jlong me )
{
  RaiJArgsEx *a = toArgs( me );
  if ( a != NULL ) {
    if ( a->os != NULL )
      a->os->jniRelease( env );
    delete a;
  }
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    Create
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
JaArgs( Create )( JNIEnv *env,  jclass /* me */ )
{
  RaiJArgsEx * a = NULL;
  try {
    a = NEW RaiJArgsEx();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return toLong( a );
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    add
 * Signature: (Lcom/rai/raiapi2/StringArg;I)V
 */
JNIEXPORT void JNICALL
JaArgs( add__Lcom_rai_raiapi2_StringArg_2I )( JNIEnv *env, jobject me,
                                              jobject a, jint flags )
{
  jstring name  = (jstring) env->GetObjectField( a, StringArg_name_fid ),
    defVal      = (jstring) env->GetObjectField( a, StringArg_defVal_fid ),
    example     = (jstring) env->GetObjectField( a, StringArg_example_fid ),
    description = (jstring) env->GetObjectField( a, StringArg_description_fid );
  RaiException e2 = NULL;

  rai::StringArg sa( ( name ? env->GetStringUTFChars( name, NULL ) : NULL ),
         ( defVal ? env->GetStringUTFChars( defVal, NULL ) : NULL ),
         ( example ? env->GetStringUTFChars( example, NULL ) : NULL ),
         ( description ? env->GetStringUTFChars( description, NULL ) : NULL ) );
  try {
    toArgs( env, me )->copy( sa, (unsigned int) flags );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( sa.name != NULL )
    env->ReleaseStringUTFChars( name, sa.name );
  if ( sa.defVal != NULL )
    env->ReleaseStringUTFChars( defVal, sa.defVal );
  if ( sa.example != NULL )
    env->ReleaseStringUTFChars( example, sa.example );
  if ( sa.description != NULL )
    env->ReleaseStringUTFChars( description, sa.description );

  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    add
 * Signature: (Lcom/rai/raiapi2/BoolArg;I)V
 */
JNIEXPORT void JNICALL
JaArgs( add__Lcom_rai_raiapi2_BoolArg_2I )( JNIEnv *env, jobject me,
                                            jobject a, jint flags )
{
  jstring name  = (jstring) env->GetObjectField( a, BoolArg_name_fid ),
    example     = (jstring) env->GetObjectField( a, BoolArg_example_fid ),
    description = (jstring) env->GetObjectField( a, BoolArg_description_fid );
  jboolean defVal = env->GetBooleanField( a, BoolArg_defVal_fid );
  RaiException e2 = NULL;

  rai::BoolArg ba( ( name ? env->GetStringUTFChars( name, NULL ) : NULL ),
         ( defVal != 0 ),
         ( example ? env->GetStringUTFChars( example, NULL ) : NULL ),
         ( description ? env->GetStringUTFChars( description, NULL ) : NULL ) );
  try {
    toArgs( env, me )->copy( ba, (unsigned int) flags );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( ba.name != NULL )
    env->ReleaseStringUTFChars( name, ba.name );
  if ( ba.example != NULL )
    env->ReleaseStringUTFChars( example, ba.example );
  if ( ba.description != NULL )
    env->ReleaseStringUTFChars( description, ba.description );

  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    add
 * Signature: (Lcom/rai/raiapi2/IntArg;I)V
 */
JNIEXPORT void JNICALL
JaArgs( add__Lcom_rai_raiapi2_IntArg_2I )( JNIEnv *env, jobject me,
                                           jobject a, jint flags )
{
  jstring name  = (jstring) env->GetObjectField( a, IntArg_name_fid ),
    example     = (jstring) env->GetObjectField( a, IntArg_example_fid ),
    description = (jstring) env->GetObjectField( a, IntArg_description_fid );
  jint defVal   = env->GetIntField( a, IntArg_defVal_fid );
  RaiException e2 = NULL;

  rai::UIntArg ia( ( name ? env->GetStringUTFChars( name, NULL ) : NULL ),
         (unsigned int) defVal,
         ( example ? env->GetStringUTFChars( example, NULL ) : NULL ),
         ( description ? env->GetStringUTFChars( description, NULL ) : NULL ) );
  try {
    toArgs( env, me )->copy( ia, (unsigned int) flags );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( ia.name != NULL )
    env->ReleaseStringUTFChars( name, ia.name );
  if ( ia.example != NULL )
    env->ReleaseStringUTFChars( example, ia.example );
  if ( ia.description != NULL )
    env->ReleaseStringUTFChars( description, ia.description );

  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    add
 * Signature: (Lcom/rai/raiapi2/DoubleArg;I)V
 */
JNIEXPORT void JNICALL
JaArgs( add__Lcom_rai_raiapi2_DoubleArg_2I )( JNIEnv *env, jobject me,
                                              jobject a, jint flags )
{
  jstring name  = (jstring) env->GetObjectField( a, DoubleArg_name_fid ),
    example     = (jstring) env->GetObjectField( a, DoubleArg_example_fid ),
    description = (jstring) env->GetObjectField( a, DoubleArg_description_fid );
  jdouble defVal = env->GetDoubleField( a, DoubleArg_defVal_fid );
  RaiException e2 = NULL;

  rai::DoubleArg da( ( name ? env->GetStringUTFChars( name, NULL ) : NULL ),
         (double) defVal,
         ( example ? env->GetStringUTFChars( example, NULL ) : NULL ),
         ( description ? env->GetStringUTFChars( description, NULL ) : NULL ) );
  try {
    toArgs( env, me )->copy( da, (unsigned int) flags );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( da.name != NULL )
    env->ReleaseStringUTFChars( name, da.name );
  if ( da.example != NULL )
    env->ReleaseStringUTFChars( example, da.example );
  if ( da.description != NULL )
    env->ReleaseStringUTFChars( description, da.description );

  if ( e2 != NULL )
    throwError( env, e2 );
}


/*
 * Class:     com_rai_raiapi2_Args
 * Method:    addDefaults
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/io/OutputStream;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaArgs( addDefaults )( JNIEnv *env, jobject me, jstring vers, jstring prefix,
                       jobject out, jstring argv0 )
{
  const char * vu = ( vers ? env->GetStringUTFChars( vers, NULL ) : NULL ),
             * pu = ( prefix ? env->GetStringUTFChars( prefix, NULL ) : NULL ),
             * au = ( argv0 ? env->GetStringUTFChars( argv0, NULL ) : NULL );
  RaiException e2 = NULL;
  RaiJArgsEx *a = toArgs( env, me );

  try {
    STRDUP( a->vs, vu );
    STRDUP( a->ps, pu );
    if ( out != NULL && a->os == NULL )
      a->os = NEW RaiApiJOutputStream( env->NewGlobalRef( out ), true );
    a->addDefaults( a->vs, a->ps, a->os, au );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( vu != NULL )
    env->ReleaseStringUTFChars( vers, vu );
  if ( pu != NULL )
    env->ReleaseStringUTFChars( prefix, pu );
  if ( au != NULL )
    env->ReleaseStringUTFChars( argv0, au );

  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    processArgs
 * Signature: ([Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
JaArgs( processArgs )( JNIEnv *env, jobject me,  jobjectArray argv )
{
  RaiApiJArgv * args = NULL;
  RaiException  e2   = NULL;
  bool          res  = false;

  try {
    args = getArgv( env, argv );
    if ( toArgs( env, me )->processArgs( args->argc, args->argv ) )
      res = true;
  } catch ( RaiException e ) {
    e2 = e;
  }

  if ( args != NULL )
    delete args;
  if ( e2 != NULL )
    throwError( env, e2 );
  return res;
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    getNumValues
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
JaArgs( getNumValues )( JNIEnv *env, jobject me, jstring n )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( n ? env->GetStringUTFChars( n, NULL ) : NULL );
  unsigned int num  = 0;
  RaiException e2   = NULL;

  if ( name != NULL ) {
    try {
      num = args.getNumValues( name );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( n, name );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return (jint) num;
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    getString
 * Signature: (Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaArgs( getString )( JNIEnv *env, jobject me, jstring n, jint num )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( n ? env->GetStringUTFChars( n, NULL ) : NULL ),
             * val  = NULL;
  RaiException e2   = NULL;

  if ( name != NULL ) {
    try {
      val = args.getString( name, (unsigned int) num );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( n, name );
  }
  if ( val != NULL )
    return env->NewStringUTF( val );
  if ( e2 != NULL )
    throwError( env, e2 );
  return NULL;
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    getBoolean
 * Signature: (Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL
JaArgs( getBoolean )( JNIEnv *env, jobject me, jstring n, jint num )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( n ? env->GetStringUTFChars( n, NULL ) : NULL );
  bool         val  = false;
  RaiException e2   = NULL;

  if ( name != NULL ) {
    try {
      val = args.getBoolean( name, (unsigned int) num );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( n, name );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return (jboolean) val;
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    getInt
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL
JaArgs( getInt )( JNIEnv *env, jobject me, jstring n, jint num )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( n ? env->GetStringUTFChars( n, NULL ) : NULL );
  unsigned int val  = 0;
  RaiException e2   = NULL;

  if ( name != NULL ) {
    try {
      val = args.getUInt( name, (unsigned int) num );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( n, name );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return (jint) val;
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    getDouble
 * Signature: (Ljava/lang/String;I)D
 */
JNIEXPORT jdouble JNICALL
JaArgs( getDouble )( JNIEnv *env, jobject me, jstring n, jint num )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( n ? env->GetStringUTFChars( n, NULL ) : NULL );
  double       val  = 0.0;
  RaiException e2   = NULL;

  if ( name != NULL ) {
    try {
      val = args.getDouble( name, (unsigned int) num );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( n, name );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return (jdouble) val;
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    setString
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaArgs( setString )( JNIEnv *env, jobject me, jstring n, jstring val )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( n ? env->GetStringUTFChars( n, NULL ) : NULL );
  const char * str  = ( val ? env->GetStringUTFChars( val, NULL ) : NULL );
  RaiException e2   = NULL;

  if ( name != NULL ) {
    try {
      args.setString( name, str );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( n, name );
  }
  if ( str != NULL )
    env->ReleaseStringUTFChars( val, str );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    setInt
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL
JaArgs( setInt )( JNIEnv *env, jobject me, jstring n, jint val )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( n ? env->GetStringUTFChars( n, NULL ) : NULL );
  RaiException e2   = NULL;

  if ( name != NULL ) {
    try {
      args.setUInt( name, (unsigned int) val );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( n, name );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    setBoolean
 * Signature: (Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL
JaArgs( setBoolean )( JNIEnv *env, jobject me, jstring n, jboolean val )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( n ? env->GetStringUTFChars( n, NULL ) : NULL );
  RaiException e2   = NULL;

  if ( name != NULL ) {
    try {
      args.setBoolean( name, (bool) ( val != 0 ) );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( n, name );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    setDouble
 * Signature: (Ljava/lang/String;D)V
 */
JNIEXPORT void JNICALL
JaArgs( setDouble )( JNIEnv *env, jobject me, jstring n, jdouble val )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( n ? env->GetStringUTFChars( n, NULL ) : NULL );
  RaiException e2   = NULL;

  if ( name != NULL ) {
    try {
      args.setDouble( name, (double) val );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( n, name );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    exists
 * Signature: ([Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
JaArgs( exists )( JNIEnv *env, jobject me,  jstring s )
{
  rai::Args  & args = *toArgs( env, me );
  const char * name = ( s ? env->GetStringUTFChars( s, NULL ) : NULL );
  RaiException e2   = NULL;
  bool         res  = false;

  if ( name != NULL ) {
    try {
      res = args.exists( name );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseStringUTFChars( s, name );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return res;
}

struct RaiApiJOutputStream2 : public rai::OutputStream {
  protected:
    JNIEnv  * env;
    jobject   out;

    virtual unsigned int emptyBuf( const byte *buf,
                                   unsigned int bufLen ) {
      RaiException e = NULL;
      jbyteArray   barr;

      this->env->PushLocalFrame( 1 );
      if ( (barr = this->env->NewByteArray( bufLen )) == NULL )
        e = rai::IOStreamErr::getErr( rai::IOStreamErr::BROKEN_PIPE );
      else {
        this->env->SetByteArrayRegion( barr, 0, bufLen, (jbyte *) buf );
        this->env->CallVoidMethod( this->out, write_mid_aB, barr );
        if ( this->env->ExceptionCheck() ) {
          this->env->ExceptionClear();
          e = rai::IOStreamErr::getErr( rai::IOStreamErr::BROKEN_PIPE );
        }
      }
      this->env->PopLocalFrame( NULL );
      if ( e != NULL )
        throw e;
      return bufLen;
    };
  public:
    RaiApiJOutputStream2( JNIEnv *env,  jobject out ) {
      this->env = env;
      this->out = out;
    };
};


/*
 * Class:     com_rai_raiapi2_Args
 * Method:    printRC
 * Signature: (Ljava/io/OutputStream;)V
 */
JNIEXPORT void JNICALL
JaArgs( printRC )( JNIEnv *env, jobject me, jobject out )
{
  rai::Args  & args = *toArgs( env, me );

  try {
    RaiApiJOutputStream2 jout( env, out );
    args.printRC( &jout );
    jout.flush();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_Args
 * Method:    printHelp
 * Signature: (Ljava/io/OutputStream;)V
 */
JNIEXPORT void JNICALL
JaArgs( printHelp )( JNIEnv *env, jobject me, jobject out )
{
  rai::Args  & args = *toArgs( env, me );

  try {
    RaiApiJOutputStream2 jout( env, out );
    args.printHelp( &jout );
    jout.flush();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}


/*** RaiDict ***/
/*
 * Class:     com_rai_raiapi2_RaiDict
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiDict( Delete )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  RaiDict *d = toDict( me );
  if ( d != NULL )
    delete d;
}

/*
 * Class:     com_rai_raiapi2_RaiDict
 * Method:    Load
 * Signature: (ILjava/lang/String;Z)V
 */
JNIEXPORT void JNICALL
JaRaiDict( Load )( JNIEnv *env, jobject me, jint timeoutSecs,
                   jstring dictSubject, jboolean loadWait )
{
  RaiDict    & dict = *toDict( env, me );
  const char * subj = dictSubject ?
                      env->GetStringUTFChars( dictSubject, NULL ) : NULL;
  RaiException e2   = NULL;
  
  try {
    dict.Load( (unsigned int) timeoutSecs, subj, (bool) ( loadWait != 0 ) );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( subj != NULL )
    env->ReleaseStringUTFChars( dictSubject, subj );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_RaiDict
 * Method:    HaveDict
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
JaRaiDict( HaveDict )( JNIEnv *env,  jobject me )
{
  RaiDict & dict = *toDict( env, me );
  bool      res  = false;
  try {
    res = dict.HaveDict();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return res;
}

/*
 * Class:     com_rai_raiapi2_RaiDict
 * Method:    InProgress
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
JaRaiDict( InProgress )( JNIEnv *env, jobject me )
{
  RaiDict & dict = *toDict( env, me );
  bool      res  = false;
  try {
    res = dict.InProgress();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return res;
}

/*** RaiSession ***/
/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    Start
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiSession( Start )( JNIEnv *env, jobject me )
{
  RaiSession & sess  = *toSession( env, me );
  try {
    sess.Start();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiSession( Delete )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  RaiSession * sess = toSession( me );
  if ( sess != NULL )
    delete sess;
}

struct RaiApiJDataLossCallback : public RaiDataLossCallback {
  jweak jcb, jcl, session;

  SYS_OPS( RaiApiJDataLossCallback );
  RaiApiJDataLossCallback( jweak cb,  jweak cl )
    : jcb( cb ), jcl( cl ), session( NULL ) {}

  void jniRelease( JNIEnv *env );

  virtual ~RaiApiJDataLossCallback() {}

  virtual void onDataLoss( RaiDataLossEvent &event,  void *cl );

  virtual void onConnection( RaiConnectionEvent &event,  void *cl );
};

void
RaiApiJDataLossCallback::jniRelease( JNIEnv *env )
{
  if ( this->jcb != NULL )
    env->DeleteWeakGlobalRef( this->jcb );
  if ( this->jcl != NULL )
    env->DeleteWeakGlobalRef( this->jcl );
  if ( this->session != NULL )
    env->DeleteWeakGlobalRef( this->session );
}

void
RaiApiJDataLossCallback::onDataLoss( RaiDataLossEvent &event,  void */* cl */ )
{
  static const rai::ErrorRec jerr[] = {
    { 1, "Exception creating dataloss event for callback", "JRaiApi" },
    { 2, "Memory exception creating dataloss event for callback", "JRaiApi" },
    { 3, "Exception in dataloss callback", "JRaiApi" }
  };
  jobject      obj;
  jstring      transport   = NULL,
               description = NULL;
  RaiException e           = NULL;
  JNIEnv     * env         = getJNIEnv();
  env->PushLocalFrame( 7 );
  jobject      lsession = env->NewLocalRef( this->session ),
               ljcb     = env->NewLocalRef( this->jcb ),
               ljcl     = this->jcl ? env->NewLocalRef( this->jcl ) : NULL;
  if ( lsession != NULL && ljcb != NULL ) {
    transport = env->NewStringUTF( event.transportName ?
                                   event.transportName : "(not-available)" );
    description = env->NewStringUTF( event.description ?
                                     event.description : "(not-available)" );
    obj = env->NewObject( RaiDataLossEvent_cls, RaiDataLossEvent_mid,
                      lsession, transport, description,
                      (jlong) event.inboundPacketLoss,
                      (jlong) event.outboundPacketLoss,
                      (jlong) event.connectionCount,
                      (jboolean) event.connectionLoss,
                      (jboolean) event.isMulticast );
    if ( obj == NULL || env->ExceptionCheck() ) {
      env->ExceptionDescribe();
      env->ExceptionClear();
      if ( obj == NULL )
        e = &jerr[ 0 ];
      else
        e = &jerr[ 1 ];
    }
    if ( e == NULL ) {
      env->CallVoidMethod( ljcb, RaiDataLossCallback_mid, obj, ljcl );
      if ( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        e = &jerr[ 2 ];
      }
    }
  }
  env->PopLocalFrame( NULL );
  if ( e != NULL )
    throw e;
}

void
RaiApiJDataLossCallback::onConnection( RaiConnectionEvent &event,  void */* cl */ )
{
  static const rai::ErrorRec jerr[] = {
    { 1, "Exception creating connection event for callback", "JRaiApi" },
    { 2, "Memory exception creating connection event for callback", "JRaiApi" },
    { 3, "Exception in connection callback", "JRaiApi" }
  };
  jobject      obj;
  jstring      transport   = NULL,
               description = NULL;
  RaiException e           = NULL;
  JNIEnv     * env         = getJNIEnv();
  env->PushLocalFrame( 7 );
  jobject      lsession = env->NewLocalRef( this->session ),
               ljcb     = env->NewLocalRef( this->jcb ),
               ljcl     = this->jcl ? env->NewLocalRef( this->jcl ) : NULL;
  if ( lsession != NULL && ljcb != NULL ) {
    transport = env->NewStringUTF( event.transportName ?
                                   event.transportName : "(not-available)" );
    description = env->NewStringUTF( event.description ?
                                     event.description : "(not-available)" );
    obj = env->NewObject( RaiConnectionEvent_cls, RaiConnectionEvent_mid,
                      lsession, transport, description,
                      (jlong) event.connectionCount,
                      (jboolean) event.connectionOriented,
                      (jboolean) event.isMulticast );
    if ( obj == NULL || env->ExceptionCheck() ) {
      env->ExceptionDescribe();
      env->ExceptionClear();
      if ( obj == NULL )
        e = &jerr[ 0 ];
      else
        e = &jerr[ 1 ];
    }
    if ( e == NULL ) {
      env->CallVoidMethod( ljcb, RaiConnectionCallback_mid, obj, ljcl );
      if ( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        e = &jerr[ 2 ];
      }
    }
  }
  env->PopLocalFrame( NULL );
  if ( e != NULL )
    throw e;
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    DeleteCB
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiSession( DeleteCB )( JNIEnv *env, jclass /* cls */, jlong me )
{
  RaiApiJDataLossCallback * cb = (RaiApiJDataLossCallback *) me;
  if ( cb != NULL ) {
    cb->jniRelease( env );
    delete cb;
  }
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    SetDataLossCB
 * Signature: (Lcom/rai/raiapi2/RaiDataLossCallback;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL
JaRaiSession( SetDataLossCB )( JNIEnv *env, jobject me, jobject cb, jobject cl )
{
  RaiSession              & sess  = *toSession( env, me );
  jweak                     jcb   = env->NewWeakGlobalRef( cb );
  jweak                     jcl   = cl ? env->NewWeakGlobalRef( cl ) : NULL;
  RaiException              e2    = NULL;
  RaiApiJDataLossCallback * lossCb = NULL;

  try {
    lossCb = NEW RaiApiJDataLossCallback( jcb, jcl );
    lossCb->session = env->NewWeakGlobalRef( me );
    env->SetLongField( me, RaiSession_cb_fid, (jlong) (void *) lossCb );
    sess.SetDataLossCB( lossCb );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( e2 != NULL ) {
    sess.SetDataLossCB( NULL );
    if ( lossCb != NULL )
      delete lossCb;
    else {
      env->DeleteWeakGlobalRef( jcb );
      if ( jcl != NULL )
        env->DeleteWeakGlobalRef( jcl );
    }
    throwError( env, e2 );
  }
}


/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    CreateQueue
 * Signature: (Z)Lcom/rai/raiapi2/RaiQueue;
 */
JNIEXPORT jobject JNICALL
JaRaiSession( CreateQueue )( JNIEnv *env, jobject me, jboolean direct )
{
  RaiSession & sess  = *toSession( env, me );
  RaiQueue   * queue;
  
  try {
    queue = sess.CreateQueue( (bool) ( direct != 0 ) );
    return env->NewObject( RaiQueue_cls, RaiQueue_mid, toLong( queue ), me );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return NULL;
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    CreatePublish
 * Signature: (Z)Lcom/rai/raiapi2/RaiPublish;
 */
JNIEXPORT jobject JNICALL
JaRaiSession( CreatePublish )( JNIEnv *env, jobject me, jboolean autoInc )
{
  RaiSession & sess = *toSession( env, me );
  RaiPublish * pub;
  try {
    pub = sess.CreatePublish( (bool) autoInc );
    return env->NewObject( RaiPublish_cls, RaiPublish_mid, toLong( pub ), me );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return NULL;
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    CreateDict
 * Signature: ()Lcom/rai/raiapi2/RaiDict;
 */
JNIEXPORT jobject JNICALL
JaRaiSession( CreateDict )( JNIEnv *env, jobject me )
{
  RaiSession & sess = *toSession( env, me );
  RaiDict    * dict;
  try {
    dict = sess.CreateDict();
    return env->NewObject( RaiDict_cls, RaiDict_mid, toLong( dict ), me );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return NULL;
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    Destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiSession( Destroy )( JNIEnv *env, jobject me )
{
  RaiSession & sess = *toSession( env, me );
  try {
    sess.Destroy();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    Login
 * Signature: (Ljava/lang/String;)Lcom/rai/raiapi2/RaiEntitlement;
 */
JNIEXPORT jobject JNICALL
JaRaiSession( Login )( JNIEnv *env, jobject me, jstring user )
{
  RaiSession     & sess = *toSession( env, me );
  const char     * usr  = ( user ?
                            env->GetStringUTFChars( user, NULL ) : NULL );
  RaiEntitlement * ent;
  jobject          obj  = NULL;
  RaiException     e2   = NULL;
  
  try {
    ent = sess.Login( usr );
    if ( ent != NULL )
      obj = env->NewObject( RaiEntitlement_cls, RaiEntitlement_mid,
                            toLong( ent ) );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( usr != NULL )
    env->ReleaseStringUTFChars( user, usr );
  if ( e2 != NULL )
    throwError( env, e2 );
  return obj;
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    NotifyStatus
 * Signature: (SS)V
 */
JNIEXPORT void JNICALL
JaRaiSession( NotifyStatus )( JNIEnv *env, jobject me, jshort msgType,
                              jshort recStatus )
{
  rai::Thread::putSpecific( raiJNIEnvKey, env ); 
  RaiSession & sess = *toSession( env, me );
  try {
    sess.NotifyStatus( (Rai_u16) msgType, (Rai_u16) recStatus );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    setSessionName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiSession( SetSessionName )( JNIEnv *env, jobject me, jstring n )
{
  RaiSession  & sess = *toSession( env, me );
  const char  * name = ( n ?  env->GetStringUTFChars( n, NULL ) : NULL );
  RaiException  e2   = NULL;
  
  try {
    sess.setSessionName( name );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( name != NULL )
    env->ReleaseStringUTFChars( n, name );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_RaiSession
 * Method:    getSessionName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiSession( GetSessionName )( JNIEnv *env, jobject me )
{
  RaiSession & sess = *toSession( env, me );
  const char *name = sess.getSessionName();
  if ( name == NULL )
    return NULL;
  return env->NewStringUTF( name );
}

/*** RaiQueue ***/
/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiQueue( Delete )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  RaiQueue * queue = toQueue( me );
  if ( queue != NULL )
    delete queue;
}


struct RaiApiJRaiMsgCallback : public RaiMsgCallback {
  jweak jcb, jcl, subscribe;
  jobject jev, jmsg;
  bool reuseMsg;

  SYS_OPS( RaiApiJRaiMsgCallback );
  RaiApiJRaiMsgCallback( jweak cb,  jweak cl )
    : jcb( cb ), jcl( cl ), subscribe( 0 ), jev( 0 ), jmsg( 0 ),
      reuseMsg( false ) {}
  virtual ~RaiApiJRaiMsgCallback() {}

  void jniRelease( JNIEnv *env );

  virtual void onMsg( RaiMsgEvent &event,  RaiMsg &message,  void *cl );
};


void
RaiApiJRaiMsgCallback::jniRelease( JNIEnv *env )
{
  if ( this->jcb != NULL )
    env->DeleteWeakGlobalRef( this->jcb );
  if ( this->jcl != NULL )
    env->DeleteWeakGlobalRef( this->jcl );
  if ( this->subscribe != NULL )
    env->DeleteWeakGlobalRef( this->subscribe );
  if ( this->jev != NULL )
    env->DeleteGlobalRef( this->jev );
  if ( this->jmsg != NULL ) {
    env->SetLongField( this->jmsg, RaiMsg_fid, (jlong) (void *) 0 );
    env->DeleteGlobalRef( this->jmsg );
  }
}


void
RaiApiJRaiMsgCallback::onMsg( RaiMsgEvent &event,  RaiMsg &message,  void */* cl */ )
{
  static const rai::ErrorRec jerr[] = {
    { 4, "Exception creating message event for callback", "JRaiApi" },
    { 5, "Memory exception creating message event for callback", "JRaiApi" },
    { 6, "Exception creating message for callback", "JRaiApi" },
    { 7, "Memory exception creating message for callback", "JRaiApi" },
    { 8, "Exception in java message callback", "JRaiApi" }
  };
  RaiException e   = NULL;
  JNIEnv     * env = getJNIEnv();

  env->PushLocalFrame( 7 );
  jobject lsubscribe = env->NewLocalRef( this->subscribe ),
          ljcb       = env->NewLocalRef( this->jcb ),
          ljcl       = this->jcl ? env->NewLocalRef( this->jcl ) : NULL;
  if ( lsubscribe == NULL || ljcb == NULL ) {
    env->PopLocalFrame( NULL );
    return;
  }
  if ( this->jev == NULL ) {
    jobject obj;
    obj = env->NewObject( RaiMsgEvent_cls, RaiMsgEvent_mid, lsubscribe,
                      env->NewStringUTF( event.subject ), (jint) event.type,
                      (jshort) event.msgType, (jshort) event.recStatus,
                      (jint) event.oldState, (jint) event.recv,
                      (jint) event.state, (jlong) event.pubTime,
                      (jlong) event.routeTime, (jlong) event.counter );
    this->jev = env->NewGlobalRef( obj );
  }
  else {
    env->CallVoidMethod( this->jev, RaiMsgEvent_init_mid, lsubscribe,
                      env->NewStringUTF( event.subject ), (jint) event.type,
                      (jshort) event.msgType, (jshort) event.recStatus,
                      (jint) event.oldState, (jint) event.recv,
                      (jint) event.state, (jlong) event.pubTime,
                      (jlong) event.routeTime, (jlong) event.counter );
  }
  if ( this->jev == NULL || env->ExceptionCheck() ) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    if ( this->jev != NULL )
      e = &jerr[ 0 ];
    else
      e = &jerr[ 1 ];
  }
  else if ( this->reuseMsg ) {
    if ( this->jmsg == NULL ) {
      jobject msg;
      msg = env->NewObject( RaiMsg_cls, RaiMsg_mid, (jlong) (void *) 0 );
      this->jmsg = env->NewGlobalRef( msg );
    }
    if ( this->jmsg == NULL || env->ExceptionCheck() ) {
      env->ExceptionDescribe();
      env->ExceptionClear();
      if ( this->jmsg != NULL )
        e = &jerr[ 2 ];
      else
        e = &jerr[ 3 ];
    }
    else {
      env->SetLongField( this->jmsg, RaiMsg_fid, toLong( &message ) );
      env->CallVoidMethod( ljcb, RaiMsgCallback_mid, this->jev, this->jmsg,
                           ljcl );
      if ( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        e = &jerr[ 4 ];
      }
    }
  }
  else {
    RaiMsg * copy = NULL;
    try {
      copy = NEW RaiMsg();
      copy->Copy( &message );
    } catch ( RaiException e2 ) {
      e = e2;
      if ( copy != NULL )
        delete copy;
    }
    if ( e == NULL ) {
      jobject msg;
      msg = env->NewObject( RaiMsg_cls, RaiMsg_mid, toLong( copy ) );

      if ( msg == NULL || env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        if ( msg == NULL )
          delete copy;
        if ( msg != NULL )
          e = &jerr[ 2 ];
        else
          e = &jerr[ 3 ];
      }
      if ( e == NULL ) {
        env->CallVoidMethod( ljcb, RaiMsgCallback_mid, this->jev, msg, ljcl );
        if ( env->ExceptionCheck() ) {
          env->ExceptionDescribe();
          env->ExceptionClear();
          e = &jerr[ 4 ];
        }
      }
    }
  }

  env->PopLocalFrame( NULL );
  if ( e != NULL )
    throw e;
}


/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    CreateSubscribe
 * Signature: (Lcom/rai/raiapi2/RaiMsgCallback;Ljava/lang/Object;)Lcom/rai/raiapi2/RaiSubscribe;
 */
JNIEXPORT jobject JNICALL
JaRaiQueue( CreateSubscribe )( JNIEnv *env, jobject me, jobject cb, jobject cl )
{
  RaiQueue              & queue = *toQueue( env, me );
  jweak                   jcb   = env->NewWeakGlobalRef( cb );
  jweak                   jcl   = cl ? env->NewWeakGlobalRef( cl ) : NULL;
  jobject                 obj   = NULL;
  RaiSubscribe          * sub   = NULL;
  RaiException            e2    = NULL;
  RaiApiJRaiMsgCallback * msgCb = NULL;

  try {
    msgCb = NEW RaiApiJRaiMsgCallback( jcb, jcl );
    sub   = queue.CreateSubscribe( msgCb );
    obj   = env->NewObject( RaiSubscribe_cls, RaiSubscribe_mid,
                            toLong( sub ), toLong( msgCb ), me );
    msgCb->subscribe = env->NewWeakGlobalRef( obj );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( obj == NULL ) {
    if ( msgCb != NULL )
      delete msgCb;
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return obj;
}


struct RaiApiJRaiTimerCallback : public RaiTimerCallback {
  jweak jcb, jcl, timer;

  SYS_OPS( RaiApiJRaiTimerCallback );
  RaiApiJRaiTimerCallback( jweak cb,  jweak cl )
    : jcb( cb ), jcl( cl ), timer( NULL ) {}

  void jniRelease( JNIEnv *env );

  virtual ~RaiApiJRaiTimerCallback() {}

  virtual void onTimer( RaiTimer &timer,  void *cl );
};


void
RaiApiJRaiTimerCallback::jniRelease( JNIEnv *env )
{
  if ( this->jcb != NULL )
    env->DeleteWeakGlobalRef( this->jcb );
  if ( this->jcl != NULL )
    env->DeleteWeakGlobalRef( this->jcl );
  if ( this->timer != NULL )
    env->DeleteWeakGlobalRef( this->timer );
}


void
RaiApiJRaiTimerCallback::onTimer( RaiTimer &/* timer */,  void */* cl */ )
{
  static const rai::ErrorRec jerr[] = {
    { 9, "Exception in java timer callback", "JRaiApi" }
  };
  RaiException e = NULL;
  JNIEnv     * env  = getJNIEnv();

  env->PushLocalFrame( 5 );
  jobject ltimer = env->NewLocalRef( this->timer ),
          ljcb   = env->NewLocalRef( this->jcb ),
          ljcl   = this->jcl ? env->NewLocalRef( this->jcl ) : NULL;
  if ( ltimer != NULL && ljcb != NULL ) {
    env->CallVoidMethod( ljcb, RaiTimerCallback_mid, ltimer, ljcl );
    if ( env->ExceptionCheck() ) {
      env->ExceptionDescribe();
      env->ExceptionClear();
      e = &jerr[ 0 ];
    }
  }
  env->PopLocalFrame( NULL );
  if ( e != NULL )
    throw e;
}


/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    CreateTimer
 * Signature: (Lcom/rai/raiapi2/RaiTimerCallback;Ljava/lang/Object;)Lcom/rai/raiapi2/RaiTimer;
 */
JNIEXPORT jobject JNICALL
JaRaiQueue( CreateTimer )( JNIEnv *env, jobject me, jobject cb, jobject cl )
{
  RaiQueue                & queue = *toQueue( env, me );
  jweak                     jcb   = env->NewWeakGlobalRef( cb );
  jweak                     jcl   = cl ? env->NewWeakGlobalRef( cl ) : NULL;
  jobject                   obj   = NULL;
  RaiTimer                * tim   = NULL;
  RaiException              e2    = NULL;
  RaiApiJRaiTimerCallback * timCb = NULL;

  try {
    timCb = NEW RaiApiJRaiTimerCallback( jcb, jcl );
    tim   = queue.CreateTimer( timCb );
    obj   = env->NewObject( RaiTimer_cls, RaiTimer_mid,
                            toLong( tim ), toLong( timCb ), me );
    timCb->timer = env->NewWeakGlobalRef( obj );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( obj == NULL ) {
    if ( timCb != NULL )
      delete timCb;
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return obj;
}

struct RaiApiJRaiSubscribeCallback : public RaiSubscribeCallback {
  jweak jcb, jcl, interactivePublish;

  SYS_OPS( RaiApiJRaiSubscribeCallback );
  RaiApiJRaiSubscribeCallback( jweak cb,  jweak cl )
    : jcb( cb ), jcl( cl ), interactivePublish( NULL ) {}

  void jniRelease( JNIEnv *env );

  virtual ~RaiApiJRaiSubscribeCallback() {}

  virtual void onSubscribe( RaiSubscribeEvent &event,  RaiMsg &message,
                            void *cl );
};


void
RaiApiJRaiSubscribeCallback::jniRelease( JNIEnv *env )
{
  if ( this->jcb != NULL )
    env->DeleteWeakGlobalRef( this->jcb );
  if ( this->jcl != NULL )
    env->DeleteWeakGlobalRef( this->jcl );
  if ( this->interactivePublish != NULL )
    env->DeleteWeakGlobalRef( this->interactivePublish );
}


void
RaiApiJRaiSubscribeCallback::onSubscribe( RaiSubscribeEvent &event,
                                          RaiMsg &message,  void */* cl */ )
{
  static const rai::ErrorRec jerr[] = {
    { 10, "Exception creating subscription event for callback", "JRaiApi" },
    { 11, "Memory exception creating subscription event for callback", "JRaiApi" },
    { 12, "Exception creating message for subscription callback", "JRaiApi" },
    { 13, "Memory exception creating message for subscription callback", "JRaiApi" },
    { 14, "Exception in java interactive subscribe callback", "JRaiApi" }
  };
  RaiException e       = NULL;
  JNIEnv     * env     = getJNIEnv();
  jstring      subject = NULL,
               reply   = NULL;
  jobject      obj,
               msg;
  RaiMsg     * copy    = NEW RaiMsg();

  copy->Copy( &message );
  env->PushLocalFrame( 9 );

  jobject linter = env->NewLocalRef( this->interactivePublish ),
          ljcb   = env->NewLocalRef( this->jcb ),
          ljcl   = this->jcl ? env->NewLocalRef( this->jcl ) : NULL;
  if ( linter != NULL && ljcb != NULL ) {
    if ( event.subject != NULL )
      subject = env->NewStringUTF( event.subject );
    if ( event.reply != NULL )
      reply = env->NewStringUTF( event.reply );

    obj = env->NewObject( RaiSubscribeEvent_cls, RaiSubscribeEvent_mid,
                          linter, subject, reply, (jint) event.queryFlags );
    if ( obj == NULL || env->ExceptionCheck() ) {
      env->ExceptionDescribe();
      env->ExceptionClear();
      delete copy;
      if ( obj != NULL )
        e = &jerr[ 0 ];
      else
        e = &jerr[ 1 ];
    }
    if ( e == NULL ) {
      msg = env->NewObject( RaiMsg_cls, RaiMsg_mid, toLong( copy ) );
      if ( msg == NULL || env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        delete copy;
        if ( msg != NULL )
          e = &jerr[ 2 ];
        else
          e = &jerr[ 3 ];
      }
      if ( e == NULL ) {
        env->CallVoidMethod( ljcb, RaiSubscribeCallback_mid, obj, msg, ljcl );
        if ( env->ExceptionCheck() ) {
          env->ExceptionDescribe();
          env->ExceptionClear();
          e = &jerr[ 4 ];
        }
      }
    }
  }
  env->PopLocalFrame( NULL );
  if ( e != NULL )
    throw e;
}


/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    CreateInteractivePublish
 * Signature: (Lcom/rai/raiapi2/RaiSubscribeCallback;Ljava/lang/Object;)Lcom/rai/raiapi2/RaiInteractivePublish;
 */
JNIEXPORT jobject JNICALL
JaRaiQueue( CreateInteractivePublish )( JNIEnv *env, jobject me, jobject cb,
                                        jobject cl )
{
  RaiQueue              & queue = *toQueue( env, me );
  jweak                   jcb   = env->NewWeakGlobalRef( cb );
  jweak                   jcl   = cl ? env->NewWeakGlobalRef( cl ) : NULL;
  jobject                 obj   = NULL;
  RaiInteractivePublish * pub   = NULL;
  RaiException            e2    = NULL;
  RaiApiJRaiSubscribeCallback * subCb = NULL;

  try {
    subCb = NEW RaiApiJRaiSubscribeCallback( jcb, jcl );
    pub   = queue.CreateInteractivePublish( subCb );
    obj   = env->NewObject( RaiInteractivePublish_cls,
                            RaiInteractivePublish_mid,
                            toLong( pub ), toLong( subCb ), me,
                            toLong( (RaiPublish *) pub ) );
    subCb->interactivePublish = env->NewWeakGlobalRef( obj );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( obj == NULL ) {
    if ( subCb != NULL )
      delete subCb;
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return obj;
}

/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    NotifyStatus
 * Signature: (SS)V
 */
JNIEXPORT void JNICALL
JaRaiQueue( NotifyStatus )( JNIEnv *env, jobject me, jshort msgType,
                            jshort recStatus )
{
  rai::Thread::putSpecific( raiJNIEnvKey, env ); 
  RaiQueue & queue = *toQueue( env, me );
  try {
    queue.NotifyStatus( (Rai_u16) msgType, (Rai_u16) recStatus );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    Mainloop
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiQueue( Mainloop )( JNIEnv *env, jobject me )
{
  rai::Thread::putSpecific( raiJNIEnvKey, env ); 
  RaiQueue & queue = *toQueue( env, me );
  try {
    queue.Mainloop();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    TimedDispatch
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
JaRaiQueue( TimedDispatch )( JNIEnv *env, jobject me, jint ivalMSecs )
{
  rai::Thread::putSpecific( raiJNIEnvKey, env ); 
  RaiQueue & queue = *toQueue( env, me );
  try {
    queue.TimedDispatch( (Rai_u32) ivalMSecs );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    Dispatch
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiQueue( Dispatch )( JNIEnv *env, jobject me )
{
  rai::Thread::putSpecific( raiJNIEnvKey, env ); 
  RaiQueue & queue = *toQueue( env, me );
  try {
    queue.Dispatch();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    GetDepth
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
JaRaiQueue( GetDepth )( JNIEnv *env, jobject me )
{
  RaiQueue & queue = *toQueue( env, me );
  Rai_u32    depth = 0;
  try {
    depth = queue.GetDepth();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return depth;
}

/*
 * Class:     com_rai_raiapi2_RaiQueue
 * Method:    Destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiQueue( Destroy )( JNIEnv *env, jobject me )
{
  RaiQueue & queue = *toQueue( env, me );
  try {
    queue.Destroy();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*** RaiSubscribe ***/
/*
 * Class:     com_rai_raiapi2_RaiSubscribe
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiSubscribe( Delete )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  RaiSubscribe * sub = toSubscribe( me );
  if ( sub != NULL )
    delete sub;
}


/*
 * Class:     com_rai_raiapi2_RaiSubscribe
 * Method:    DeleteCB
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiSubscribe( DeleteCB )( JNIEnv *env, jclass /* cls */, jlong me )
{
  RaiApiJRaiMsgCallback *msgCb = (RaiApiJRaiMsgCallback *) (void *) me;
  if ( msgCb != NULL ) {
    msgCb->jniRelease( env );
    delete msgCb;
  }
}


/*
 * Class:     com_rai_raiapi2_RaiSubscribe
 * Method:    Start
 * Signature: (Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL
JaRaiSubscribe( Start )( JNIEnv *env, jobject me, jstring subject, jint parm,
                         jint timeoutMSecs )
{
  RaiSubscribe          & sub = *toSubscribe( env, me );
  RaiApiJRaiMsgCallback & cb  = *toMsgCb( env, me );
  const char   * subj = ( subject ?
                          env->GetStringUTFChars( subject, NULL ) : NULL );
  RaiException   e2   = NULL;
  if ( ( (byte) parm & com_rai_raiapi2_RaiSubscribe_NO_COPY ) != 0 ) {
    cb.reuseMsg = true;
    parm &= ~com_rai_raiapi2_RaiSubscribe_NO_COPY;
  }
  else {
    cb.reuseMsg = false;
  }
  try {
    sub.Start( subj, (RaiSubscribe::RaiSubParameter) parm,
               (Rai_u32) timeoutMSecs );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( subj != NULL )
    env->ReleaseStringUTFChars( subject, subj );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_RaiSubscribe
 * Method:    Cancel
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiSubscribe( Cancel )( JNIEnv *env, jobject me )
{
  RaiSubscribe & sub = *toSubscribe( env, me );
  try {
    sub.Cancel();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiSubscribe
 * Method:    Refresh
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
JaRaiSubscribe( Refresh )( JNIEnv *env, jobject me, jint timeoutMSecs )
{
  RaiSubscribe & sub = *toSubscribe( env, me );
  try {
    sub.Refresh( (Rai_u32) timeoutMSecs );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiSubscribe
 * Method:    Subject
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiSubscribe( Subject )( JNIEnv *env, jobject me )
{
  RaiSubscribe & sub = *toSubscribe( env, me );
  try {
    const char * val = sub.Subject();
    if ( val != NULL )
      return env->NewStringUTF( val );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return NULL;
}

/*
 * Class:     com_rai_raiapi2_RaiSubscribe
 * Method:    InProgress
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
JaRaiSubscribe( InProgress )( JNIEnv *env, jobject me )
{
  RaiSubscribe & sub = *toSubscribe( env, me );
  try {
    return sub.InProgress();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return false;
}

/*** RaiPublish ***/
/*
 * Class:     com_rai_raiapi2_RaiPublish
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiPublish( Delete )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  RaiPublish * pub = toPublish( me );
  if ( pub != NULL )
    delete pub;
}

/*
 * Class:     com_rai_raiapi2_RaiPublish
 * Method:    Publish
 * Signature: (Ljava/lang/String;Lcom/rai/raimsg/RaiMsg;J)V
 */
JNIEXPORT void JNICALL
JaRaiPublish( Publish__Ljava_lang_String_2Lcom_rai_raimsg_RaiMsg_2J )(
         JNIEnv *env, jobject me, jstring subject, jobject raiMsg, jlong stamp )
{
  RaiPublish & pub  = *toPublish( env, me );
  const char * subj = ( subject ?
                        env->GetStringUTFChars( subject, NULL ) : NULL );
  RaiMsg     & msg  = *toMsg( env, raiMsg );
  RaiException e2   = NULL;
  try {
    pub.Publish( subj, msg, (rai::TimeNSecs) stamp );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( subj != NULL )
    env->ReleaseStringUTFChars( subject, subj );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_RaiPublish
 * Method:    Publish
 * Signature: (Ljava/lang/String;[BIIJ)V
 */
JNIEXPORT void JNICALL
JaRaiPublish( Publish__Ljava_lang_String_2_3BIIJ )(
         JNIEnv *env, jobject me, jstring subject,
         jbyteArray buffer, jint offset, jint length, jlong stamp )
{
  static const rai::ErrorRec jerr[] = {
    { 15, "Publishing null buffer", "JRaiApi" }
  };
  RaiException e2 = NULL;
  if ( buffer == NULL )
    e2 = &jerr[ 0 ];
  else {
    RaiPublish & pub  = *toPublish( env, me );
    const char * subj = ( subject ?
                          env->GetStringUTFChars( subject, NULL ) : NULL );
    jsize        len  = env->GetArrayLength( buffer );
    jbyte      * els  = (jbyte *) env->GetPrimitiveArrayCritical( buffer, NULL);
    if ( offset + length > len ) {
      if ( offset > len )
        length = 0;
      else
        length = len - offset;
    }
    try {
      pub.Publish( subj, &els[ offset ], length, (rai::TimeNSecs) stamp );
    } catch ( RaiException e ) {
      e2 = e;
    }
    if ( subj != NULL )
      env->ReleaseStringUTFChars( subject, subj );
    if ( els != NULL )
      env->ReleasePrimitiveArrayCritical( buffer, els, 0 );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_RaiPublish
 * Method:    SetPrefix
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiPublish( SetPrefix )( JNIEnv *env, jobject me, jstring subject )
{
  RaiPublish & pub = *toPublish( env, me );
  const char * subj = ( subject ?
                        env->GetStringUTFChars( subject, NULL ) : NULL );
  RaiException e2  = NULL;
  try {
    pub.SetPrefix( subj );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( subj != NULL )
    env->ReleaseStringUTFChars( subject, subj );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_RaiPublish
 * Method:    GetPrefix
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiPublish( GetPrefix )( JNIEnv *env, jobject me )
{
  RaiPublish & pub = *toPublish( env, me );
  const char * subj;
  subj = pub.GetPrefix();
  if ( subj != NULL )
    return env->NewStringUTF( subj );
  return NULL;
}

/*
 * Class:     com_rai_raiapi2_RaiPublish
 * Method:    GetSeqno
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
JaRaiPublish( GetSeqno )( JNIEnv *env, jobject me )
{
  RaiPublish & pub = *toPublish( env, me );
  return (jlong) pub.GetSeqno();
}

/*
 * Class:     com_rai_raiapi2_RaiPublish
 * Method:    SetSeqno
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiPublish( SetSeqno )( JNIEnv *env, jobject me, jlong newSeqno )
{
  RaiPublish & pub = *toPublish( env, me );
  pub.SetSeqno( (Rai_u32) newSeqno );
}

/*
 * Class:     com_rai_raiapi2_RaiPublish
 * Method:    Destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiPublish( Destroy )( JNIEnv *env, jobject me )
{
  RaiPublish & pub = *toPublish( env, me );
  try {
    pub.Destroy();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*** RaiTimer ***/
/*
 * Class:     com_rai_raiapi2_RaiTimer
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiTimer( Delete )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  RaiTimer * tim = toTimer( me );
  if ( tim != NULL )
    delete tim;
}

/*
 * Class:     com_rai_raiapi2_RaiTimer
 * Method:    DeleteCB
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiTimer( DeleteCB )( JNIEnv *env, jclass /* cls */, jlong me )
{
  RaiApiJRaiTimerCallback * timCb = (RaiApiJRaiTimerCallback *) (void *) me;

  if ( timCb != NULL ) {
    timCb->jniRelease( env );
    delete timCb;
  }
}



/*
 * Class:     com_rai_raiapi2_RaiTimer
 * Method:    Start
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiTimer( Start )( JNIEnv *env, jobject me )
{
  RaiTimer & tim = *toTimer( env, me );
  try {
    tim.Start();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiTimer
 * Method:    Stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiTimer( Stop )( JNIEnv *env, jobject me )
{
  RaiTimer & tim = *toTimer( env, me );
  try {
    tim.Stop();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiTimer
 * Method:    GetInterval
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
JaRaiTimer( GetInterval )( JNIEnv *env, jobject me )
{
  RaiTimer & tim = *toTimer( env, me );
  try {
    return (jlong) tim.GetInterval();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return 0;
}

/*
 * Class:     com_rai_raiapi2_RaiTimer
 * Method:    SetInterval
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiTimer( SetInterval )( JNIEnv *env, jobject me, jlong intervalMSecs )
{
  RaiTimer & tim = *toTimer( env, me );
  try {
    tim.SetInterval( (rai::TimeMSecs) intervalMSecs );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*** RaiInteractivePublisher ***/
/*
 * Class:     com_rai_raiapi2_RaiInteractivePublish
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiInteractivePublish( Delete )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  RaiInteractivePublish * ipub = toInteractivePublish( me );
  if ( ipub != NULL )
    delete ipub;
}

/*
 * Class:     com_rai_raiapi2_RaiInteractivePublish
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiInteractivePublish( DeleteCB )( JNIEnv *env, jclass /* cls */, jlong me )
{
  RaiApiJRaiSubscribeCallback * subCb = 
    (RaiApiJRaiSubscribeCallback *) (void *) me;
  if ( subCb != NULL ) {
    subCb->jniRelease( env );
    delete subCb;
  }
}

/*
 * Class:     com_rai_raiapi2_RaiInteractivePublish
 * Method:    InteractiveStart
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiInteractivePublish( InteractiveStart )( JNIEnv *env, jobject me,
                                            jstring subject )
{
  RaiInteractivePublish & ipub = *toInteractivePublish( env, me );
  const char * subj = ( subject ?
                        env->GetStringUTFChars( subject, NULL ) : NULL );
  RaiException e2   = NULL;
  try {
    ipub.InteractiveStart( subj );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( subj != NULL )
    env->ReleaseStringUTFChars( subject, subj );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raiapi2_RaiInteractivePublish
 * Method:    InteractiveCancel
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JaRaiInteractivePublish( InteractiveCancel )( JNIEnv *env, jobject me )
{
  RaiInteractivePublish & ipub = *toInteractivePublish( env, me );
  try {
    return ipub.InteractiveCancel();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     com_rai_raiapi2_RaiInteractivePublish
 * Method:    InProgress
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
JaRaiInteractivePublish( InProgress )( JNIEnv *env, jobject me )
{
  RaiInteractivePublish & ipub = *toInteractivePublish( env, me );
  try {
    return ipub.InProgress();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return false;
}

/*
 * Class:     com_rai_raiapi2_RaiEntitlement
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_rai_raiapi2_RaiEntitlement_Delete( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  RaiEntitlement * ent = toEntitlement( me );
  if ( ent != NULL )
    delete ent;
}


