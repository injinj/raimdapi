#include <jni.h>
#include <string.h>
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#include "raiapi/java/com/rai/raimsg/rai_msg_jni.h"
#include "raiapi/java/com/rai/raimsg/rai_field_jni.h"
#include "raiapi/java/com/rai/raimsg/rai_msg_exception_jni.h"
#include "msg/rai_msg.h"
#include "msg/sass_const.h"
#include "stream/io_stream.h"
#include "util/str_util.h"
#include "base/log.h"

#define JaRaiMsg(F)          Java_com_rai_raimsg_RaiMsg_ ## F
#define JaRaiField(F)        Java_com_rai_raimsg_RaiField_ ## F
#define JaRaiMsgException(F) Java_com_rai_raimsg_RaiMsgException_ ## F
#define JaRaiMsgPATH         "com/rai/raimsg/"

static inline jlong toLong( const void *p ) {
  return (jlong) (ulongptr) p;
}
static inline RaiException toError( jlong p ) {
  return (RaiException) (void *) (ulongptr) p;
}
static inline RaiMsg *toRaiMsg( jlong p ) {
  return (RaiMsg *) (void *) (ulongptr) p;
}
static inline RaiField *toRaiField( jlong p ) {
  return (RaiField *) (void *) (ulongptr) p;
}

static jclass    RaiMsgException_cls,
                 RaiMsg_cls,
                 RaiField_cls,
                 Partial_cls,
                 String_cls,
                 Boolean_cls,
                 Byte_cls,
                 Short_cls,
                 Integer_cls,
                 Long_cls,
                 Float_cls,
                 Double_cls,
                 OutputStream_cls,
                 Class_cls,
                 boolean_cls,
                 byte_cls,
                 short_cls,
                 int_cls,
                 long_cls,
                 float_cls,
                 double_cls;

static jmethodID RaiMsgException_mid_J,
                 RaiMsg_mid_J,
                 RaiField_mid_J,
                 Partial_mid_aBI,
                 Boolean_mid_Z,
                 Byte_mid_B,
                 Short_mid_S,
                 Integer_mid_I,
                 Long_mid_J,
                 Float_mid_F,
                 Double_mid_D,
                 write_mid_aB,
                 getData_mid,
                 getOffset_mid,
                 booleanValue_mid,
                 byteValue_mid,
                 shortValue_mid,
                 intValue_mid,
                 longValue_mid,
                 floatValue_mid,
                 doubleValue_mid,
                 getMsgHandle_mid,
                 getFieldHandle_mid,
                 isArray_mid,
                 getComponentType_mid;

static void
javaException( JNIEnv *env,  const char *msg )
{
  jclass ex = env->FindClass( "java/lang/Exception" );
  if ( ex != NULL )
    env->ThrowNew( ex, msg );
}


JNIEXPORT void JNICALL
JaRaiMsg( initClasses )( JNIEnv *env,  jclass /* me */,
                         jclass Z, jclass B, jclass S, jclass I, jclass J,
                         jclass F, jclass D )
{
  static const char INIT[] = "<init>";
  jclass cls;

#if defined( _WIN32 ) || defined( _WIN64 )
  HMODULE dllp = ::LoadLibrary( "jraimsg.dll" );
  if ( dllp == NULL ) {
    dllp = ::LoadLibrary( "jraimsg64.dll" );
  }
#else
  /* map it global, otherwise exceptions won't work */
  void *a = ::dlopen( "libjraimsg.so", RTLD_NOW | RTLD_GLOBAL );
  if ( a == NULL ) {
    //fprintf( stderr, "dlerror %s\n", dlerror() );
    a = ::dlopen( "libjraimsg64.so", RTLD_NOW | RTLD_GLOBAL );
    //if ( a == NULL )
    //fprintf( stderr, "dlerror %s\n", dlerror() );
  }
#endif
  if ( (cls = env->FindClass( JaRaiMsgPATH "RaiMsgException" )) == NULL ||
       (RaiMsgException_cls   = (jclass) env->NewGlobalRef( cls )) == NULL ||
       (RaiMsgException_mid_J = env->GetMethodID( RaiMsgException_cls, INIT,
                                                  "(J)V" )) == NULL )
    javaException( env, "Can't load RaiMsgException class" );

  else if ( (cls = env->FindClass( JaRaiMsgPATH "RaiMsg" )) == NULL ||
            (RaiMsg_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (RaiMsg_mid_J = env->GetMethodID( RaiMsg_cls, INIT,
                                              "(J)V" )) == NULL ||
            (getMsgHandle_mid = env->GetMethodID( RaiMsg_cls, "getMsgHandle",
                                                  "()J" )) == NULL )
    javaException( env, "Can't load RaiMsg class" );

  else if ( (cls = env->FindClass( JaRaiMsgPATH "RaiField" )) == NULL ||
            (RaiField_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (RaiField_mid_J = env->GetMethodID( RaiField_cls, INIT,
                                                "(J)V" )) == NULL ||
            (getFieldHandle_mid = env->GetMethodID( RaiField_cls,
                                                    "getFieldHandle",
                                                    "()J" )) == NULL )
    javaException( env, "Can't load RaiField class" );

  else if ( (cls = env->FindClass( JaRaiMsgPATH "Partial" )) == NULL ||
            (Partial_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (Partial_mid_aBI = env->GetMethodID( Partial_cls, INIT,
                                                 "([BI)V" )) == NULL ||
            (getData_mid = env->GetMethodID( Partial_cls, "getData",
                                             "()[B" )) == NULL ||
            (getOffset_mid = env->GetMethodID( Partial_cls, "getOffset",
                                               "()I" )) == NULL )
    javaException( env, "Can't load Partial class" );

  else if ( (cls = env->FindClass( "java/lang/String" )) == NULL ||
            (String_cls = (jclass) env->NewGlobalRef( cls )) == NULL )
    javaException( env, "Can't load String class" );

  else if ( (cls = env->FindClass( "java/lang/Boolean" )) == NULL ||
            (Boolean_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (Boolean_mid_Z = env->GetMethodID( Boolean_cls, INIT,
                                               "(Z)V" )) == NULL ||
            (booleanValue_mid = env->GetMethodID( Boolean_cls, "booleanValue",
                                                  "()Z" )) == NULL )
    javaException( env, "Can't load Boolean class" );

  else if ( (cls = env->FindClass( "java/lang/Byte" )) == NULL ||
            (Byte_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (Byte_mid_B = env->GetMethodID( Byte_cls, INIT, "(B)V" )) == NULL ||
            (byteValue_mid = env->GetMethodID( Byte_cls, "byteValue",
                                              "()B" )) == NULL )
    javaException( env, "Can't load Byte class" );

  else if ( (cls = env->FindClass( "java/lang/Short" )) == NULL ||
            (Short_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (Short_mid_S = env->GetMethodID( Short_cls, INIT,
                                             "(S)V" )) == NULL ||
            (shortValue_mid = env->GetMethodID( Short_cls, "shortValue",
                                                "()S" )) == NULL )
    javaException( env, "Can't load Short class" );

  else if ( (cls = env->FindClass( "java/lang/Integer" )) == NULL ||
            (Integer_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (Integer_mid_I = env->GetMethodID( Integer_cls, INIT,
                                               "(I)V" )) == NULL ||
            (intValue_mid = env->GetMethodID( Integer_cls, "intValue",
                                              "()I" )) == NULL )
    javaException( env, "Can't load Integer class" );

  else if ( (cls = env->FindClass( "java/lang/Long" )) == NULL ||
            (Long_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (Long_mid_J = env->GetMethodID( Long_cls, INIT, "(J)V" )) == NULL ||
            (longValue_mid = env->GetMethodID( Long_cls, "longValue",
                                              "()J" )) == NULL )
    javaException( env, "Can't load Long class" );

  else if ( (cls = env->FindClass( "java/lang/Float" )) == NULL ||
            (Float_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (Float_mid_F = env->GetMethodID( Float_cls, INIT,
                                             "(F)V" )) == NULL ||
            (floatValue_mid = env->GetMethodID( Float_cls, "floatValue",
                                                "()F" )) == NULL )
    javaException( env, "Can't load Float class" );

  else if ( (cls = env->FindClass( "java/lang/Double" )) == NULL ||
            (Double_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (Double_mid_D = env->GetMethodID( Double_cls, INIT,
                                              "(D)V" )) == NULL ||
            (doubleValue_mid = env->GetMethodID( Double_cls, "doubleValue",
                                                 "()D" )) == NULL )
    javaException( env, "Can't load Double class" );

  else if ( (cls = env->FindClass( "java/io/OutputStream" )) == NULL ||
            (OutputStream_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (write_mid_aB = env->GetMethodID( OutputStream_cls, "write",
                                              "([B)V" )) == NULL )
    javaException( env, "Can't load OutputStream class" );

  else if ( (cls = env->FindClass( "java/lang/Class" )) == NULL ||
            (Class_cls = (jclass) env->NewGlobalRef( cls )) == NULL ||
            (isArray_mid = env->GetMethodID( Class_cls, "isArray",
                                             "()Z" )) == NULL ||
            (getComponentType_mid = env->GetMethodID( Class_cls,
                                                      "getComponentType",
                                            "()Ljava/lang/Class;" )) == NULL )
    javaException( env, "Can't load Class class" );

  else if ( (boolean_cls = (jclass) env->NewGlobalRef( Z )) == NULL ||
            (byte_cls    = (jclass) env->NewGlobalRef( B )) == NULL ||
            (short_cls   = (jclass) env->NewGlobalRef( S )) == NULL ||
            (int_cls     = (jclass) env->NewGlobalRef( I )) == NULL ||
            (long_cls    = (jclass) env->NewGlobalRef( J )) == NULL ||
            (float_cls   = (jclass) env->NewGlobalRef( F )) == NULL ||
            (double_cls  = (jclass) env->NewGlobalRef( D )) == NULL )
    javaException( env, "Can't create global refs" );
}


static void
throwError( JNIEnv *env,  RaiException e )
{
  jthrowable thr;
  thr = (jthrowable) env->NewObject( RaiMsgException_cls, RaiMsgException_mid_J,
                                     toLong( e ) );
  env->Throw( thr );
}


jobject
boxField( JNIEnv *env,  RaiField &field )
{
  jobject     obj = NULL;
  RaiMsg_data fdata;
  RaiMsg_size fsize;

  switch ( field.Type() ) {

    case RAIMSG_MESSAGE: {
      RaiMsg  * subMsg,
              * newMsg = NULL;
      subMsg = (RaiMsg *) field.Data();
      newMsg = NEW RaiMsg();

      try {
        newMsg->Copy( subMsg );
        obj = env->NewObject( RaiMsg_cls, RaiMsg_mid_J, toLong( newMsg ) );
      } catch ( ... ) {
        if ( newMsg != NULL )
          delete newMsg;
        throw;
      }
      break;
    }

    case RAIMSG_STRING:
      if ( (fdata = field.Data()) != NULL ) {
        fsize = field.Size();
        if ( rai::StrUtil::strnlen( (const char *) fdata, fsize ) == fsize ) {
          char * tmp;
          MALLOC( fsize + 1, &tmp );
          ::memcpy( tmp, fdata, fsize );
          tmp[ fsize ] = '\0';
          obj = (jobject) env->NewStringUTF( tmp ); 
          FREE( tmp );
        }
        else {
          obj = (jobject) env->NewStringUTF( (char *) fdata ); 
        }
      }
      break;

    case RAIMSG_OPAQUE: {
      jbyteArray barr;
      fdata = field.Data();
      fsize = field.Size();
      barr = env->NewByteArray( fsize );
      env->SetByteArrayRegion( barr, 0, fsize, (jbyte *) fdata );
      obj = (jobject) barr;
      break;
    }

    case RAIMSG_BOOLEAN: {
      bool b;
      field.Get( b );
      obj = env->NewObject( Boolean_cls, Boolean_mid_Z, b );
      break;
    }

    case RAIMSG_UINT:
    case RAIMSG_INT: {
      fsize = field.Size();
      if ( fsize == 2 ) {
        Rai_i16 i16;
        field.Get( i16 );
        obj = env->NewObject( Short_cls, Short_mid_S, i16 );
      }
      else if ( fsize == 4 ) {
        Rai_i32 i32;
        field.Get( i32 );
        obj = env->NewObject( Integer_cls, Integer_mid_I, i32 );
      }
      else if ( fsize == 1 ) {
        Rai_i8 i8;
        field.Get( i8 );
        obj = env->NewObject( Byte_cls, Byte_mid_B, i8 );
      }
      else {
        Rai_i64 i64;
        field.Get( i64 );
        obj = env->NewObject( Long_cls, Long_mid_J, i64 );
      }
      break;
    }

    case RAIMSG_REAL:
      fsize = field.Size();
      if ( fsize == 4 ) {
        Rai_f32 f32;
        field.Get( f32 );
        obj = env->NewObject( Float_cls, Float_mid_F, f32 );
      }
      else {
        Rai_f64 f64;
        field.Get( f64 );
        obj = env->NewObject( Double_cls, Double_mid_D, f64 );
      }
      break;

    case RAIMSG_IPDATA: {
      char buf[ 32 ];
      field.Convert( buf, sizeof( buf ) );
      obj = (jobject) env->NewStringUTF( buf ); 
      break;
    }

    case RAIMSG_ARRAY: {
      unsigned int numEntries,
                   entrySize;
      numEntries = field.NumEntries();
      if ( numEntries > 0 ) {
        fdata     = field.Data();
        entrySize = field.EntrySize();

        switch ( field.EntryType() ) {

          case RAIMSG_STRING: {
            jobjectArray sArray;
            jstring      s;
            unsigned int i;
            char       * tmp = NULL;

            sArray = env->NewObjectArray( numEntries, String_cls, NULL );
            for ( i = 0; i < numEntries; i++ ) {
              if ( rai::StrUtil::strnlen( (char *) fdata, entrySize ) ==
                   entrySize ){
                if ( tmp == NULL )
                  MALLOC( entrySize + 1, &tmp );
                ::memcpy( tmp, fdata, entrySize );
                tmp[ entrySize ] = '\0';
                s = env->NewStringUTF( tmp );
              }
              else {
                s = env->NewStringUTF( (char *) fdata );
              }
              env->SetObjectArrayElement( sArray, (jsize) i, (jobject) s );
              fdata = &((char *) fdata)[ entrySize ];
            }
            if ( tmp != NULL )
              FREE( tmp );
            obj = (jobject) sArray;
            break;
          }

          case RAIMSG_OPAQUE: {
            jbyteArray bArray = env->NewByteArray( numEntries * entrySize );
            env->SetByteArrayRegion( bArray, 0, numEntries * entrySize,
                                     (jbyte *) fdata );
            obj = (jobject) bArray;
            break;
          }

          case RAIMSG_BOOLEAN: {
            jbooleanArray bArray = env->NewBooleanArray( numEntries );
            env->SetBooleanArrayRegion( bArray, 0, numEntries,
                                        (jboolean *) fdata );
            obj = (jobject) bArray;
            break;
          }

          case RAIMSG_UINT:
          case RAIMSG_INT: {
            if ( entrySize == 2 ) {
              jshortArray iArray = env->NewShortArray( numEntries );
              env->SetShortArrayRegion( iArray, 0, numEntries,
                                        (jshort *) fdata );
              obj = (jobject) iArray;
            }
            else if ( entrySize == 4 ) {
              jintArray iArray = env->NewIntArray( numEntries );
              env->SetIntArrayRegion( iArray, 0, numEntries, (jint *) fdata );
              obj = (jobject) iArray;
            }
            else if ( entrySize == 1 ) {
              jbyteArray iArray = env->NewByteArray( numEntries );
              env->SetByteArrayRegion( iArray, 0, numEntries, (jbyte *) fdata );
              obj = (jobject) iArray;
            }
            else {
              jlongArray iArray = env->NewLongArray( numEntries );
              env->SetLongArrayRegion( iArray, 0, numEntries, (jlong *) fdata );
              obj = (jobject) iArray;
            }
            break;
          }

          case RAIMSG_REAL: {
            if ( entrySize == 4 ) {
              jfloatArray fArray = env->NewFloatArray( numEntries );
              env->SetFloatArrayRegion( fArray, 0, numEntries,
                                        (jfloat *) fdata );
              obj = fArray;
            }
            else {
              jdoubleArray dArray = env->NewDoubleArray( numEntries );
              env->SetDoubleArrayRegion( dArray, 0, numEntries,
                                        (jdouble *) fdata );
              obj = dArray;
            }
            break;
          }

          case RAIMSG_IPDATA: {
            jobjectArray sArray;
            jstring      s;
            unsigned int i;
            char         buf[ 32 ];
            sArray = env->NewObjectArray( numEntries, String_cls, NULL );
            for ( i = 0; i < numEntries; i++ ) {
              RaiField::Convert( RAIMSG_STRING, sizeof( buf ),
                                   (RaiMsg_data) buf, RAIMSG_IPDATA,
                                   entrySize, fdata );
              s = env->NewStringUTF( buf );
              env->SetObjectArrayElement( sArray, (jsize) i, (jobject) s );
              fdata = &((char *) fdata)[ entrySize ];
            }
            obj = (jobject) sArray;
            break;
          }

          default:
            throw RaiMsgErr::getErr( RaiMsgErr::NO_FIELD );
        }
      }
      break;
    }
    case RAIMSG_PARTIAL: {
      fdata = field.Data();
      fsize = field.Size();
      jbyteArray bArray = env->NewByteArray( fsize );
      env->SetByteArrayRegion( bArray, 0, fsize, (jbyte *) fdata );
      obj = env->NewObject( Partial_cls, Partial_mid_aBI, bArray,
                            (int) field.Offset() );
      break;
    }
    default:
      throw RaiMsgErr::getErr( RaiMsgErr::NO_FIELD );
  }
  return obj;
}


jobject
boxFieldHint( JNIEnv *env,  RaiField &field )
{
  jobject     obj = NULL;
  RaiMsg_data fdata;
  RaiMsg_size fsize;

  switch ( field.HintType() ) {
    case RAIMSG_STRING:
      if ( (fdata = field.HintData()) != NULL ) {
        fsize = field.HintSize();
        if ( rai::StrUtil::strnlen( (const char *) fdata, fsize ) == fsize ) {
          char * tmp;
          MALLOC( fsize + 1, &tmp );
          ::memcpy( tmp, fdata, fsize );
          tmp[ fsize ] = '\0';
          obj = (jobject) env->NewStringUTF( tmp ); 
          FREE( tmp );
        }
        else {
          obj = (jobject) env->NewStringUTF( (char *) fdata ); 
        }
      }
      break;
    case RAIMSG_OPAQUE: {
      jbyteArray barr;
      fdata = field.HintData();
      fsize = field.HintSize();
      barr = env->NewByteArray( fsize );
      env->SetByteArrayRegion( barr, 0, fsize, (jbyte *) fdata );
      obj = (jobject) barr;
      break;
    }
    case RAIMSG_BOOLEAN: {
      bool b;
      field.GetHint( b );
      obj = env->NewObject( Boolean_cls, Boolean_mid_Z, b );
      break;
    }
    case RAIMSG_UINT:
    case RAIMSG_INT: {
      fsize = field.HintSize();
      if ( fsize == 2 ) {
        Rai_i16 i16;
        field.GetHint( i16 );
        obj = env->NewObject( Short_cls, Short_mid_S, i16 );
      }
      else if ( fsize == 4 ) {
        Rai_i32 i32;
        field.GetHint( i32 );
        obj = env->NewObject( Integer_cls, Integer_mid_I, i32 );
      }
      else if ( fsize == 1 ) {
        Rai_i8 i8;
        field.GetHint( i8 );
        obj = env->NewObject( Byte_cls, Byte_mid_B, i8 );
      }
      else {
        Rai_i64 i64;
        field.GetHint( i64 );
        obj = env->NewObject( Long_cls, Long_mid_J, i64 );
      }
      break;
    }
    case RAIMSG_REAL:
      fsize = field.Size();
      if ( fsize == 4 ) {
        Rai_f32 f32;
        field.GetHint( f32 );
        obj = env->NewObject( Float_cls, Float_mid_F, f32 );
      }
      else {
        Rai_f64 f64;
        field.GetHint( f64 );
        obj = env->NewObject( Double_cls, Double_mid_D, f64 );
      }
      break;
    case RAIMSG_IPDATA: {
      char buf[ 32 ];
      field.HintConvert( buf, sizeof( buf ) );
      obj = (jobject) env->NewStringUTF( buf ); 
      break;
    }
    case RAIMSG_MESSAGE:
    case RAIMSG_ARRAY:
    case RAIMSG_PARTIAL:
    default:
      throw RaiMsgErr::getErr( RaiMsgErr::NO_FIELD );
  }
  return obj;
}


void
updateOrAppendHint( JNIEnv *env,  RaiMsg &msg,  const char *fname,
                    RaiMsg_type ftype,  RaiMsg_size fsize,  RaiMsg_data fdata,
                    jobject obj,  bool isAppend )
{
  RaiField     field;
  RaiException e2;

  e2 = NULL;
  /* RAIMSG_STRING */
  if ( env->IsInstanceOf( obj, String_cls ) ) {
    const char *sval = env->GetStringUTFChars( (jstring) obj, NULL );
    if ( ! env->ExceptionCheck() ) {
      try {
        field.Update( fname, ftype, fsize, fdata, RAIMSG_STRING,
                      ( sval == NULL ? 0 : ::strlen( sval ) + 1 ),
                      (RaiMsg_data) sval );
        if ( isAppend )
          msg.Append( &field );
        else
          msg.Update( &field );
      } catch ( RaiException e ) {
        e2 = e;
      }
      env->ReleaseStringUTFChars( (jstring) obj, sval );
    }
  }
  /* RAIMSG_INT 2 */
  else if ( env->IsInstanceOf( obj, Short_cls ) ) {
    jshort v = env->CallShortMethod( obj, shortValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        field.Update( fname, ftype, fsize, fdata, RAIMSG_INT, sizeof( Rai_i16 ),
                      (RaiMsg_data) &v );
        if ( isAppend )
          msg.Append( &field );
        else
          msg.Update( &field );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_REAL 4 */
  else if ( env->IsInstanceOf( obj, Float_cls ) ) {
    jfloat v = env->CallFloatMethod( obj, floatValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        field.Update( fname, ftype, fsize, fdata, RAIMSG_REAL,
                      sizeof( Rai_f32 ), (RaiMsg_data) &v );
        if ( isAppend )
          msg.Append( &field );
        else
          msg.Update( &field );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_INT 4 */
  else if ( env->IsInstanceOf( obj, Integer_cls ) ) {
    jint v = env->CallIntMethod( obj, intValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        field.Update( fname, ftype, fsize, fdata, RAIMSG_INT, sizeof( Rai_i32 ),
                      (RaiMsg_data) &v );
        if ( isAppend )
          msg.Append( &field );
        else
          msg.Update( &field );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_INT 1 */
  else if ( env->IsInstanceOf( obj, Byte_cls ) ) {
    jbyte v = env->CallByteMethod( obj, byteValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        field.Update( fname, ftype, fsize, fdata, RAIMSG_INT, sizeof( Rai_i8 ),
                      (RaiMsg_data) &v );
        if ( isAppend )
          msg.Append( &field );
        else
          msg.Update( &field );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_REAL 8 */
  else if ( env->IsInstanceOf( obj, Double_cls ) ) {
    jdouble v = env->CallDoubleMethod( obj, doubleValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        field.Update( fname, ftype, fsize, fdata, RAIMSG_REAL,
                      sizeof( Rai_f64 ), (RaiMsg_data) &v );
        if ( isAppend )
          msg.Append( &field );
        else
          msg.Update( &field );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_BOOLEAN */
  else if ( env->IsInstanceOf( obj, Boolean_cls ) ) {
    bool v = env->CallBooleanMethod( obj, booleanValue_mid ) ? true : false;
    if ( ! env->ExceptionCheck() ) {
      try {
        field.Update( fname, ftype, fsize, fdata, RAIMSG_BOOLEAN,
                      sizeof( bool ), (RaiMsg_data) &v );
        if ( isAppend )
          msg.Append( &field );
        else
          msg.Update( &field );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_INT 8 */
  else if ( env->IsInstanceOf( obj, Long_cls ) ) {
    jlong v = env->CallLongMethod( obj, longValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        field.Update( fname, ftype, fsize, fdata, RAIMSG_INT, sizeof( Rai_i64 ),
                      (RaiMsg_data) &v );
        if ( isAppend )
          msg.Append( &field );
        else
          msg.Update( &field );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }

  if ( e2 != NULL )
    throwError( env, e2 );
}


void
updateOrAppend( JNIEnv *env,  RaiMsg &msg,  jstring name,  jobject obj,
                jobject hintObj,  bool isAppend,  bool isUnsigned )
{
  const char * fname;
  RaiException e2 = NULL;

  fname = ( name == NULL ? NULL : env->GetStringUTFChars( name, NULL ) );

  /* RAIMSG_STRING */
  if ( env->IsInstanceOf( obj, String_cls ) ) {
    const char *sval = env->GetStringUTFChars( (jstring) obj, NULL );
    if ( ! env->ExceptionCheck() ) {
      try {
        if ( hintObj != NULL )
          updateOrAppendHint( env, msg, fname, RAIMSG_STRING, 
                              ( sval == NULL ? 0 : ::strlen( sval ) + 1 ),
                              (RaiMsg_data) sval, hintObj, isAppend );
        else if ( isAppend )
          msg.Append( fname, sval );
        else
          msg.Update( fname, sval );
      } catch ( RaiException e ) {
        e2 = e;
      }
      env->ReleaseStringUTFChars( (jstring) obj, sval );
    }
  }
  /* RAIMSG_INT 2 */
  else if ( env->IsInstanceOf( obj, Short_cls ) ) {
    jshort v = env->CallShortMethod( obj, shortValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        if ( isUnsigned ) {
          if ( hintObj != NULL )
            updateOrAppendHint( env, msg, fname, RAIMSG_UINT, sizeof( Rai_u16 ),
                                (RaiMsg_data) &v, hintObj, isAppend );
          else if ( isAppend )
            msg.Append( fname, (Rai_u16) v );
          else
            msg.Update( fname, (Rai_u16) v );
        }
        else {
          if ( hintObj != NULL )
            updateOrAppendHint( env, msg, fname, RAIMSG_INT, sizeof( Rai_i16 ),
                                (RaiMsg_data) &v, hintObj, isAppend );
          else if ( isAppend )
            msg.Append( fname, (Rai_i16) v );
          else
            msg.Update( fname, (Rai_i16) v );
        }
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_REAL 4 */
  else if ( env->IsInstanceOf( obj, Float_cls ) ) {
    jfloat v = env->CallFloatMethod( obj, floatValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        if ( hintObj != NULL )
          updateOrAppendHint( env, msg, fname, RAIMSG_REAL, sizeof( Rai_f32 ),
                              (RaiMsg_data) &v, hintObj, isAppend );
        else if ( isAppend )
          msg.Append( fname, v );
        else
          msg.Update( fname, v );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_INT 4 */
  else if ( env->IsInstanceOf( obj, Integer_cls ) ) {
    jint v = env->CallIntMethod( obj, intValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        if ( isUnsigned ) {
          if ( hintObj != NULL )
            updateOrAppendHint( env, msg, fname, RAIMSG_UINT, sizeof( Rai_u32 ),
                                (RaiMsg_data) &v, hintObj, isAppend );
          else if ( isAppend )
            msg.Append( fname, (Rai_u32) v );
          else
            msg.Update( fname, (Rai_u32) v );
        }
        else {
          if ( hintObj != NULL )
            updateOrAppendHint( env, msg, fname, RAIMSG_INT, sizeof( Rai_i32 ),
                                (RaiMsg_data) &v, hintObj, isAppend );
          else if ( isAppend )
            msg.Append( fname, (Rai_i32) v );
          else
            msg.Update( fname, (Rai_i32) v );
        }
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_INT 1 */
  else if ( env->IsInstanceOf( obj, Byte_cls ) ) {
    jbyte v = env->CallByteMethod( obj, byteValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        if ( isUnsigned ) {
          if ( hintObj != NULL )
            updateOrAppendHint( env, msg, fname, RAIMSG_UINT, sizeof( Rai_u8 ),
                                (RaiMsg_data) &v, hintObj, isAppend );
          else if ( isAppend )
            msg.Append( fname, (Rai_u8) v );
          else
            msg.Update( fname, (Rai_u8) v );
        }
        else {
          if ( hintObj != NULL )
            updateOrAppendHint( env, msg, fname, RAIMSG_INT, sizeof( Rai_i8 ),
                                (RaiMsg_data) &v, hintObj, isAppend );
          else if ( isAppend )
            msg.Append( fname, (Rai_i8) v );
          else
            msg.Update( fname, (Rai_i8) v );
        }
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_REAL 8 */
  else if ( env->IsInstanceOf( obj, Double_cls ) ) {
    jdouble v = env->CallDoubleMethod( obj, doubleValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        if ( hintObj != NULL )
          updateOrAppendHint( env, msg, fname, RAIMSG_REAL, sizeof( Rai_f64 ),
                              (RaiMsg_data) &v, hintObj, isAppend );
        else if ( isAppend )
          msg.Append( fname, v );
        else
          msg.Update( fname, v );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_MESSAGE */
  else if ( env->IsInstanceOf( obj, RaiMsg_cls ) ) {
    RaiMsg * m;
    if ( (m = toRaiMsg(
               env->CallLongMethod( obj, getMsgHandle_mid ) )) != NULL ) {
      try {
        if ( isAppend )
          msg.Append( fname, m );
        else
          msg.Update( fname, m );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_PARTIAL */
  else if ( env->IsInstanceOf( obj, Partial_cls ) ) {
    jbyteArray barr;
    if ( (barr = (jbyteArray) env->CallObjectMethod( obj,
                                                     getData_mid )) != NULL ) {
      jint off = env->CallIntMethod( obj, getOffset_mid );
      if ( ! env->ExceptionCheck() ) {
        jbyte * buf;
        if ( (buf = env->GetByteArrayElements( barr, NULL )) != NULL ) {
          jsize len = env->GetArrayLength( barr );
          try {
            if ( isAppend )
              msg.Append( fname, (RaiMsg_data) buf, (RaiMsg_size) len,
                          (RaiMsg_size) off );
            else
              msg.Update( fname, (RaiMsg_data) buf, (RaiMsg_size) len,
                          (RaiMsg_size) off );
          } catch ( RaiException e ) {
            e2 = e;
          }
          env->ReleaseByteArrayElements( barr, buf, JNI_ABORT );
        }
      }
    }
  }
  /* RAIMSG_BOOLEAN */
  else if ( env->IsInstanceOf( obj, Boolean_cls ) ) {
    bool v = env->CallBooleanMethod( obj, booleanValue_mid ) ? true : false;
    if ( ! env->ExceptionCheck() ) {
      try {
        if ( hintObj != NULL )
          updateOrAppendHint( env, msg, fname, RAIMSG_BOOLEAN, sizeof( bool ),
                              (RaiMsg_data) &v, hintObj, isAppend );
        else if ( isAppend )
          msg.Append( fname, v );
        else
          msg.Update( fname, v );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_INT 8 */
  else if ( env->IsInstanceOf( obj, Long_cls ) ) {
    jlong v = env->CallLongMethod( obj, longValue_mid );
    if ( ! env->ExceptionCheck() ) {
      try {
        if ( isUnsigned ) {
          if ( hintObj != NULL )
            updateOrAppendHint( env, msg, fname, RAIMSG_UINT, sizeof( Rai_u64 ),
                                (RaiMsg_data) &v, hintObj, isAppend );
          else if ( isAppend )
            msg.Append( fname, (Rai_u64) v );
          else
            msg.Update( fname, (Rai_u64) v );
        }
        else {
          if ( hintObj != NULL )
            updateOrAppendHint( env, msg, fname, RAIMSG_INT, sizeof( Rai_i64 ),
                                (RaiMsg_data) &v, hintObj, isAppend );
          else if ( isAppend )
            msg.Append( fname, (Rai_i64) v );
          else
            msg.Update( fname, (Rai_i64) v );
        }
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  /* RAIMSG_ARRAY */
  else {
    jclass cls = env->GetObjectClass( obj );

    /* check if isArray() */
    if ( env->CallBooleanMethod( cls, isArray_mid ) ) {
      jsize len = env->GetArrayLength( (jarray) obj );
      RaiMsg_type utype = ( isUnsigned ? RAIMSG_UINT : RAIMSG_INT );

      if ( ! env->ExceptionCheck() && len > 0 &&
           (cls = (jclass) env->CallObjectMethod( cls,
                                            getComponentType_mid )) != NULL ) {

        /* RAIMSG_ARRAY / RAIMSG_INT 1 */
        if ( env->IsSameObject( (jobject) cls, (jobject) byte_cls ) ) {
          jbyteArray arr = (jbyteArray) obj;
          jbyte    * els = env->GetByteArrayElements( arr, NULL );
          if ( els != NULL ) {
            try {
              msg.Append( fname, els, len, utype, sizeof( els[ 0 ] ) );
            } catch ( RaiException e ) {
              e2 = e;
            }
            env->ReleaseByteArrayElements( arr, els, JNI_ABORT );
          }
        }
        /* RAIMSG_ARRAY / RAIMSG_INT 2 */
        else if ( env->IsSameObject( (jobject) cls, (jobject) short_cls ) ) {
          jshortArray arr = (jshortArray) obj;
          jshort    * els = env->GetShortArrayElements( arr, NULL );
          if ( els != NULL ) {
            try {
              msg.Append( fname, els, len, utype, sizeof( els[ 0 ] ) );
            } catch ( RaiException e ) {
              e2 = e;
            }
            env->ReleaseShortArrayElements( arr, els, JNI_ABORT );
          }
        }
        /* RAIMSG_ARRAY / RAIMSG_INT 4 */
        else if ( env->IsSameObject( (jobject) cls, (jobject) int_cls ) ) {
          jintArray arr = (jintArray) obj;
          jint    * els = env->GetIntArrayElements( arr, NULL );
          if ( els != NULL ) {
            try {
              msg.Append( fname, els, len, utype, sizeof( els[ 0 ] ) );
            } catch ( RaiException e ) {
              e2 = e;
            }
            env->ReleaseIntArrayElements( arr, els, JNI_ABORT );
          }
        }
        /* RAIMSG_ARRAY / RAIMSG_INT 8 */
        else if ( env->IsSameObject( (jobject) cls, (jobject) long_cls ) ) {
          jlongArray arr = (jlongArray) obj;
          jlong    * els = env->GetLongArrayElements( arr, NULL );
          if ( els != NULL ) {
            try {
              msg.Append( fname, els, len, utype, sizeof( els[ 0 ] ) );
            } catch ( RaiException e ) {
              e2 = e;
            }
            env->ReleaseLongArrayElements( arr, els, JNI_ABORT );
          }
        }
        /* RAIMSG_ARRAY / RAIMSG_REAL 4 */
        else if ( env->IsSameObject( (jobject) cls, (jobject) float_cls ) ) {
          jfloatArray arr = (jfloatArray) obj;
          jfloat    * els = env->GetFloatArrayElements( arr, NULL );
          if ( els != NULL ) {
            try {
              msg.Append( fname, els, len, RAIMSG_REAL, sizeof( els[ 0 ] ) );
            } catch ( RaiException e ) {
              e2 = e;
            }
            env->ReleaseFloatArrayElements( arr, els, JNI_ABORT );
          }
        }
        /* RAIMSG_ARRAY / RAIMSG_REAL 8 */
        else if ( env->IsSameObject( (jobject) cls, (jobject) double_cls ) ) {
          jdoubleArray arr = (jdoubleArray) obj;
          jdouble    * els = env->GetDoubleArrayElements( arr, NULL );
          if ( els != NULL ) {
            try {
              msg.Append( fname, els, len, RAIMSG_REAL, sizeof( els[ 0 ] ) );
            } catch ( RaiException e ) {
              e2 = e;
            }
            env->ReleaseDoubleArrayElements( arr, els, JNI_ABORT );
          }
        }
        /* RAIMSG_ARRAY / RAIMSG_STRING */
        else if ( env->IsSameObject( (jobject) cls, (jobject) String_cls ) ) {
          jobjectArray arr = (jobjectArray) obj;
          char       * strings;
          jstring      s;
          const char * s2;
          unsigned int i,
                       len2,
                       maxLen;
          strings = NULL;
          maxLen  = 1;

          for ( i = 0; i < (unsigned int) len; i++ ) {
            env->PushLocalFrame( 2 );
            s = (jstring) env->GetObjectArrayElement( arr, i );
            if ( s != NULL ) {
              s2 = env->GetStringUTFChars( s, NULL );
              if ( s2 != NULL ) {
                len2 = ::strlen( s2 ) + 1;
                if ( len2 > maxLen )
                  maxLen = len2;
                env->ReleaseStringUTFChars( s, s2 );
              }
            }
            env->PopLocalFrame( NULL );
            if ( env->ExceptionCheck() )
              goto has_exception;
          }

          MALLOC( len * maxLen, &strings );
          ::memset( strings, 0, len * maxLen );

          for ( i = 0; i < (unsigned int) len; i++ ) {
            env->PushLocalFrame( 2 );
            s = (jstring) env->GetObjectArrayElement( arr, i );
            if ( s != NULL ) {
              s2 = env->GetStringUTFChars( s, NULL );
              if ( s2 != NULL ) {
                ::strcpy( &strings[ i * maxLen ], s2 );
                env->ReleaseStringUTFChars( s, s2 );
              }
            }
            env->PopLocalFrame( NULL );
            if ( env->ExceptionCheck() )
              goto has_exception;
          }

          try {
            msg.Append( fname, strings, len, RAIMSG_STRING, maxLen );
          } catch ( RaiException e ) {
            e2 = e;
          }
        has_exception:;
          if ( strings != NULL )
            FREE( strings );
        }
        /* RAIMSG_ARRAY / RAIMSG_BOOLEAN */
        else if ( env->IsSameObject( (jobject) cls, (jobject) boolean_cls ) ) {
          jbooleanArray arr = (jbooleanArray) obj;
          jboolean    * els = env->GetBooleanArrayElements( arr, NULL );
          if ( els != NULL ) {
            msg.Append( fname, els, len, RAIMSG_BOOLEAN, sizeof( els[ 0 ] ) );
            env->ReleaseBooleanArrayElements( arr, els, JNI_ABORT );
          }
        }
        else {
          e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
        }
      }
    }
    else {
      e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
    }
  }

  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
}


void
updateOrAppendField( JNIEnv *env,  RaiMsg &msg,  jobject obj,  bool isAppend )
{
  RaiException e2 = NULL;

  if ( env->IsInstanceOf( obj, RaiField_cls ) ) {
    RaiField * f;
    if ( (f = toRaiField(
               env->CallLongMethod( obj, getFieldHandle_mid ) )) != NULL ) {
      try {
        if ( isAppend )
          msg.Append( f );
        else
          msg.Update( f );
      } catch ( RaiException e ) {
        e2 = e;
      }
    }
  }
  else {
    e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     RaiMsg
 * Method:    Create
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
JaRaiMsg( Create )( JNIEnv *env, jclass /* cls */,  jint proto )
{
  RaiMsg * msg = NULL;
  try {
    if ( ! RaiMsg::isValidProto( (RaiMsg_protocol) proto ) )
      throw RaiMsgErr::getErr( RaiMsgErr::BAD_PROTO );
    msg = NEW RaiMsg( (RaiMsg_protocol) proto );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return toLong( msg );
}

/*
 * Class:     RaiMsg
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( Delete )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  if ( me != 0 )
    delete toRaiMsg( me );
}


/*
 * Class:     RaiMsg
 * Method:    ReUse
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( ReUse__JI )( JNIEnv *env, jclass /* cls */, jlong me, jint proto )
{
  try {
    toRaiMsg( me )->ReUse( (RaiMsg_protocol) proto );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     RaiMsg
 * Method:    ReUse
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( ReUse__J )( JNIEnv *env, jclass /* cls */, jlong me )
{
  try {
    toRaiMsg( me )->ReUse();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     RaiMsg
 * Method:    GetProtocol
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiMsg( GetProtocol )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  return (jint) toRaiMsg( me )->GetProtocol();
}

/*
 * Class:     RaiMsg
 * Method:    GetProtocolString
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsg( GetProtocolString )( JNIEnv *env, jclass /* cls */, jlong me )
{
  const char *s = toRaiMsg( me )->GetProtocolString();
  return env->NewStringUTF( s );
}

/*
 * Class:     RaiMsg
 * Method:    MsgTypeToString
 * Signature: (S)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsg( MsgTypeToString )( JNIEnv *env, jclass /* cls */, jshort msgType )
{
  char buf[ 16 ];
  return env->NewStringUTF( rai::SassConst::msgTypeToString(
                                             (unsigned short) msgType, buf ) );
}

/*
 * Class:     RaiMsg
 * Method:    StringToMsgType
 * Signature: (Ljava/lang/String;)S
 */
JNIEXPORT jshort JNICALL
JaRaiMsg( StringToMsgType )( JNIEnv *env, jclass /* cls */, jstring s )
{
  const char   * s2      = ( s == NULL ? NULL :
                           env->GetStringUTFChars( s, NULL ) );
  unsigned short msgType = rai::SassConst::stringToMsgType( s2 );
  if ( s2 != NULL )
    env->ReleaseStringUTFChars( s, s2 );
  return (jshort) msgType;
}

/*
 * Class:     RaiMsg
 * Method:    GetMsgTypeString
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsg( GetMsgTypeString )( JNIEnv *env, jclass /* cls */, jlong me )
{
  jstring      val = NULL;
  RaiException e2  = NULL;
  try {
    RaiField field;
    if ( toRaiMsg( me )->Get( "MSG_TYPE", field ) ) {
      if ( field.Type() == RAIMSG_STRING ) {
        const char *s;
        field.Get( s );
        val = env->NewStringUTF( s );
      }
      else {
        char    buf[ 16 ];
        Rai_u16 msgType;
        field.Get( msgType );
        val = env->NewStringUTF(
          rai::SassConst::msgTypeToString( msgType, buf ) );
      }
    }
    else {
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
    }
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    SetMsgTypeString
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( SetMsgTypeString )( JNIEnv *env, jclass /* cls */, jlong me, jstring s )
{
  const char   * s2      = ( s == NULL ? NULL :
                           env->GetStringUTFChars( s, NULL ) );
  unsigned short msgType = rai::SassConst::stringToMsgType( s2 );
  if ( s2 != NULL )
    env->ReleaseStringUTFChars( s, s2 );
  try {
    toRaiMsg( me )->Update( "MSG_TYPE", msgType );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     RaiMsg
 * Method:    RecStatusToString
 * Signature: (S)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsg( RecStatusToString )( JNIEnv *env, jclass /* cls */, jshort recStatus )
{
  char buf[ 16 ];
  return env->NewStringUTF( rai::SassConst::recStatusToString(
                                            (unsigned short) recStatus, buf ) );
}

/*
 * Class:     RaiMsg
 * Method:    StringToRecStatus
 * Signature: (Ljava/lang/String;)S
 */
JNIEXPORT jshort JNICALL
JaRaiMsg( StringToRecStatus )( JNIEnv *env, jclass /* cls */, jstring s )
{
  const char   * s2        = ( s == NULL ? NULL :
                             env->GetStringUTFChars( s, NULL ) );
  unsigned short recStatus = rai::SassConst::stringToRecStatus( s2 );
  if ( s2 != NULL )
    env->ReleaseStringUTFChars( s, s2 );
  return (jshort) recStatus;
}

/*
 * Class:     RaiMsg
 * Method:    GetRecStatusString
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsg( GetRecStatusString )( JNIEnv *env, jclass /* cls */, jlong me )
{
  jstring      val = NULL;
  RaiException e2  = NULL;
  try {
    RaiField field;
    if ( toRaiMsg( me )->Get( "REC_STATUS", field ) ) {
      if ( field.Type() == RAIMSG_STRING ) {
        const char *s;
        field.Get( s );
        val = env->NewStringUTF( s );
      }
      else {
        char    buf[ 16 ];
        Rai_u16 recStatus;
        field.Get( recStatus );
        val = env->NewStringUTF(
                          rai::SassConst::recStatusToString( recStatus, buf ) );
      }
    }
    else {
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
    }
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    SetRecStatustring
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( SetRecStatusString )( JNIEnv *env, jclass /* cls */, jlong me, jstring s )
{
  const char   * s2        = ( s == NULL ? NULL :
                             env->GetStringUTFChars( s, NULL ) );
  unsigned short recStatus = rai::SassConst::stringToRecStatus( s2 );
  if ( s2 != NULL )
    env->ReleaseStringUTFChars( s, s2 );
  try {
    toRaiMsg( me )->Update( "REC_STATUS", recStatus );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}


/*
 * Class:     RaiMsg
 * Method:    RecTypeToString
 * Signature: (S)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsg( RecTypeToString )( JNIEnv *env, jclass /* cls */, jshort recType )
{
  const RaiMsg_form * form;
  RaiException e2;

  if ( rai::DataDictionary == NULL )
    e2 = RaiMsgErr::getErr( RaiMsgErr::NO_DICTIONARY );
  else if ( (form = rai::DataDictionary->getForm( recType )) == NULL )
    e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS );
  else {
    return env->NewStringUTF( form->entry->fname );
  }
  throwError( env, e2 );
  return (jstring) NULL;
}

/*
 * Class:     RaiMsg
 * Method:    StringToRecType
 * Signature: (Ljava/lang/String;)S
 */
JNIEXPORT jshort JNICALL
JaRaiMsg( StringToRecType )( JNIEnv *env, jclass /* cls */, jstring s )
{
  const char        * s2 = ( s == NULL ? NULL :
                      env->GetStringUTFChars( s, NULL ) );
  RaiException        e2 = NULL;
  const RaiMsg_form * form;
  unsigned short      recType = 0;

  try {
    if ( s2 != NULL ) {
      if ( rai::DataDictionary == NULL )
        e2 = RaiMsgErr::getErr( RaiMsgErr::NO_DICTIONARY );
      else if ( (form = rai::DataDictionary->getForm( s2 )) == NULL )
        e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS );
      else
        recType = form->entry->fid;
    }
    else {
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
    }
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( s2 != NULL )
    env->ReleaseStringUTFChars( s, s2 );
  if ( e2 != NULL )
    throwError( env, e2 );
  return (jshort) recType;
}

/*
 * Class:     RaiMsg
 * Method:    GetRecTypeString
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsg( GetRecTypeString )( JNIEnv *env, jclass /* cls */, jlong me )
{
  jstring      val = NULL;
  RaiException e2  = NULL;
  try {
    RaiField field;
    if ( toRaiMsg( me )->Get( "REC_TYPE", field ) ) {
      if ( field.Type() == RAIMSG_STRING ) {
        const char *s;
        field.Get( s );
        val = env->NewStringUTF( s );
      }
      else {
        char                buf[ 16 ];
        Rai_u16             recType;
        const RaiMsg_form * form;
        field.Get( recType );
        if ( rai::DataDictionary != NULL &&
             (form = rai::DataDictionary->getForm( recType )) != NULL ) {
          val = env->NewStringUTF( form->entry->fname );
        }
        else {
          field.Get( buf, sizeof( buf ) );
          val = env->NewStringUTF( buf );
        }
      }
    }
    else {
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
    }
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    SetRecTypetring
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( SetRecTypeString )( JNIEnv *env, jclass /* cls */, jlong me, jstring s )
{
  const char        * s2 = ( s == NULL ? NULL :
                           env->GetStringUTFChars( s, NULL ) );
  RaiException        e2 = NULL;
  const RaiMsg_form * form;

  try {
    if ( s2 != NULL ) {
      if ( rai::DataDictionary != NULL &&
           (form = rai::DataDictionary->getForm( s2 )) != NULL ) {
        toRaiMsg( me )->Update( "REC_TYPE", form->entry->fid );
      }
      else {
        toRaiMsg( me )->Update( "REC_TYPE", s2 );
      }
    }
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( s2 != NULL )
    env->ReleaseStringUTFChars( s, s2 );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     RaiMsg
 * Method:    ClearForm
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( ClearForm )( JNIEnv *env, jclass /* cls */, jlong me )
{
  RaiMsg            * _this = toRaiMsg( me );
  const RaiMsg_form * form      = NULL;
  unsigned short      msgType   = 0,
                      recType   = 0,
                      recStatus = 0,
                      seqNo     = 0;
  RaiException        e2        = NULL;

  try {
    if ( ! _this->isSass() )
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_SASS_FORM );
    else if ( ! _this->Get( "REC_TYPE", recType ) )
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
    else if ( rai::DataDictionary == NULL )
      e2 = RaiMsgErr::getErr( RaiMsgErr::NO_DICTIONARY );
    else if ( (form = rai::DataDictionary->getForm( recType )) == NULL )
      e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_FORM_CLASS );

    if ( e2 == NULL ) {
      if ( form->msgType != NULL )
        _this->Get( form->msgType, msgType );
      if ( form->recStatus != NULL )
        _this->Get( form->recStatus, recStatus );
      if ( form->seqNo != NULL )
        _this->Get( form->seqNo, seqNo );
      
      _this->UpgradeToForm();
      _this->ClearForm( form );

      if ( form->msgType != NULL )
        _this->Update( form->msgType, msgType );
      if ( form->recType != NULL )
        _this->Update( form->recType, recType );
      if ( form->seqNo != NULL )
        _this->Update( form->seqNo, seqNo );
      if ( form->recStatus != NULL )
        _this->Update( form->recStatus, recStatus );
    }
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     RaiMsg
 * Method:    Release
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( Release )( JNIEnv */* env */, jclass /* cls */, jlong me )
{
  toRaiMsg( me )->Release();
}

/*
 * Class:     RaiMsg
 * Method:    Get
 * Signature: (JLjava/lang/String;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL
JaRaiMsg( Get )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  RaiMsg     * _this = toRaiMsg( me );
  RaiField     field;
  jobject      obj;
  RaiException e2;
  const char * fname;

  obj   = NULL;
  e2    = NULL;
  fname = ( name == NULL ? NULL : env->GetStringUTFChars( name, NULL ) );

  try {
    if ( _this->Get( fname, field ) )
      obj = boxField( env, field );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );

  return obj;
}


#if 0
template <class RetType, class ValType>
static RetType
GetVal( JNIEnv *env, jlong me, jstring name )
{
  RaiMsg     * _this = toRaiMsg( me );
  ValType      val   = (ValType) 0;
  RaiException e2    = NULL;
  const char * fname = ( name == NULL ? NULL :
                         env->GetStringUTFChars( name, NULL ) );
  try {
    RaiField field;
    if ( _this->Get( fname, field ) )
      field.Get( val );
    else
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
  return (RetType) val;
}
#endif

#define BADGCC_GET_VAL2( RetType, ValType, ENV, ME, NAME ) \
{ \
  RaiMsg     * _this = toRaiMsg( ME ); \
  ValType      val   = (ValType) 0; \
  RaiException e2    = NULL; \
  const char * fname = ( NAME == NULL ? NULL : \
                         env->GetStringUTFChars( NAME, NULL ) ); \
  try { \
    RaiField field; \
    if ( _this->Get( fname, field ) ) \
      field.Get( val ); \
    else \
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND ); \
  } catch ( RaiException e ) { \
    e2 = e; \
  } \
  if ( fname != NULL ) \
    env->ReleaseStringUTFChars( NAME, fname ); \
  if ( e2 != NULL ) \
    throwError( env, e2 ); \
  return (RetType) val; \
}

/*
 * Class:     RaiMsg
 * Method:    GetBoolean
 * Signature: (JLjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
JaRaiMsg( GetBoolean )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  BADGCC_GET_VAL2( jboolean, bool, env, me, name )
  /*return GetVal<jboolean, bool>( env, me, name );*/
}

/*
 * Class:     RaiMsg
 * Method:    GetByte
 * Signature: (JLjava/lang/String;)B
 */
JNIEXPORT jbyte JNICALL
JaRaiMsg( GetByte )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  BADGCC_GET_VAL2( jbyte, Rai_i8, env, me, name )
  /*return GetVal<jbyte, Rai_i8>( env, me, name );*/
}

/*
 * Class:     RaiMsg
 * Method:    GetShort
 * Signature: (JLjava/lang/String;)S
 */
JNIEXPORT jshort JNICALL
JaRaiMsg( GetShort )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  BADGCC_GET_VAL2( jshort, Rai_i16, env, me, name )
  /*return GetVal<jshort, Rai_i16>( env, me, name );*/
}

/*
 * Class:     RaiMsg
 * Method:    GetInt
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL
JaRaiMsg( GetInt )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  BADGCC_GET_VAL2( jint, Rai_i32, env, me, name )
  /*return GetVal<jint, Rai_i32>( env, me, name );*/
}

/*
 * Class:     RaiMsg
 * Method:    GetLong
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL
JaRaiMsg( GetLong )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  BADGCC_GET_VAL2( jlong, Rai_i64, env, me, name )
  /*return GetVal<jlong, Rai_i64>( env, me, name );*/
}

/*
 * Class:     RaiMsg
 * Method:    GetFloat
 * Signature: (JLjava/lang/String;)F
 */
JNIEXPORT jfloat JNICALL
JaRaiMsg( GetFloat )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  BADGCC_GET_VAL2( jfloat, Rai_f32, env, me, name )
  /*return GetVal<jfloat, Rai_f32>( env, me, name );*/
}

/*
 * Class:     RaiMsg
 * Method:    GetDouble
 * Signature: (JLjava/lang/String;)D
 */
JNIEXPORT jdouble JNICALL
JaRaiMsg( GetDouble )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  BADGCC_GET_VAL2( jdouble, Rai_f64, env, me, name )
  /*return GetVal<jdouble, Rai_f64>( env, me, name );*/
}


static jstring
GetString( JNIEnv *env,  RaiField &field )
{
  const char * s   = NULL;
  jstring      val = NULL;
  RaiException e2  = NULL;

  try {
    if ( field.Type() == RAIMSG_STRING || field.Type() == RAIMSG_OPAQUE ) {
      field.Get( s );
      if ( s != NULL ) {
        RaiMsg_size fsize = field.Size();
        if ( rai::StrUtil::strnlen( s, fsize ) == fsize ) {
          char * tmp;
          MALLOC( fsize + 1, &tmp );
          ::memcpy( tmp, s, fsize );
          tmp[ fsize ] = '\0';
          val = env->NewStringUTF( tmp ); 
          FREE( tmp );
        }
        else {
          val = env->NewStringUTF( s );
        }
      }
    }
    else {
      char buf[ 256 ];
      field.Get( buf, 256 );
      val = env->NewStringUTF( buf );
    }
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}


/*
 * Class:     RaiMsg
 * Method:    GetString
 * Signature: (JLjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsg( GetString )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  RaiMsg     * _this = toRaiMsg( me );
  jstring      val   = NULL;
  RaiException e2    = NULL;
  const char * fname = ( name == NULL ? NULL :
                         env->GetStringUTFChars( name, NULL ) );
  try {
    RaiField field;
    if ( _this->Get( fname, field ) )
      val = GetString( env, field );
    else
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}


static jbyteArray
GetOpaque( JNIEnv *env,  RaiField &field )
{
  jbyteArray   val = NULL;
  const char * s   = NULL;
  RaiException e2  = NULL;

  try {
    char        buf[ 256 ];
    RaiMsg_size len = 0;

    if ( field.Type() == RAIMSG_ARRAY ||
         field.Type() == RAIMSG_PARTIAL ||
         field.Type() == RAIMSG_OPAQUE ) {
      len = field.Size();
      s   = (const char *) field.Data();
    }
    else {
      if ( field.Type() == RAIMSG_STRING ) {
        field.Get( s );
        if ( s != NULL )
          len = rai::StrUtil::strnlen( s, field.Size() );
      }
      else {
        field.Get( buf, 256 );
        s   = buf;
        len = ::strlen( buf );
      }
    }
    if ( s != NULL ) {
      if ( (val = env->NewByteArray( len )) != NULL )
        env->SetByteArrayRegion( val, 0, len, (jbyte *) s );
    }
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    GetOpaque
 * Signature: (JLjava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL
JaRaiMsg( GetOpaque )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  RaiMsg     * _this = toRaiMsg( me );
  jbyteArray   val   = NULL;
  RaiException e2    = NULL;
  const char * fname = ( name == NULL ? NULL :
                         env->GetStringUTFChars( name, NULL ) );
  try {
    RaiField field;
    if ( _this->Get( fname, field ) )
      val = GetOpaque( env, field );
    else
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}


static jobject
GetPartial( JNIEnv *env,  RaiField &field )
{
  RaiException e2  = NULL;
  jobject      val = NULL;

  if ( field.Type() == RAIMSG_PARTIAL ) {
    try {
      RaiMsg_data fdata = field.Data();
      RaiMsg_size fsize = field.Size();
      jbyteArray bArray = env->NewByteArray( fsize );
      if ( bArray != NULL ) {
        env->SetByteArrayRegion( bArray, 0, fsize, (jbyte *) fdata );
        val = env->NewObject( Partial_cls, Partial_mid_aBI, bArray,
                              (int) field.Offset() );
      }
    } catch ( RaiException e ) {
      e2 = e;
    }
  }
  else {
    e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_ARG );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}


/*
 * Class:     RaiMsg
 * Method:    GetPartial
 * Signature: (JLjava/lang/String;)LPartial;
 */
JNIEXPORT jobject JNICALL
JaRaiMsg( GetPartial )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  RaiMsg     * _this = toRaiMsg( me );
  jobject      val   = NULL;
  RaiException e2    = NULL;
  const char * fname = ( name == NULL ? NULL :
                         env->GetStringUTFChars( name, NULL ) );
  try {
    RaiField field;
    if ( _this->Get( fname, field ) )
      val = GetPartial( env, field );
    else
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}


#define GET_ARRAY_VAL_DEF1( Val, RetType, ElemType, DestType, DestType2, cvterr, \
                            env, me, name, NewArray, SetArrayRegion ) \
{ \
  RaiMsg     * _this = toRaiMsg( me ); \
  RaiException e2    = NULL; \
  const char * fname = ( name == NULL ? NULL : \
                          env->GetStringUTFChars( name, NULL ) ); \
  try { \
    RaiField field; \
    if ( _this->Get( fname, field ) ) { \
      GET_ARRAY_VAL_DEF2( Val, RetType, ElemType, DestType, DestType2, cvterr, \
                          env, field, NewArray, SetArrayRegion ); \
 \
    } \
    else { \
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND ); \
    } \
  } catch ( RaiException e ) { \
    e2 = e; \
  } \
  if ( fname != NULL ) \
    env->ReleaseStringUTFChars( name, fname ); \
  if ( e2 != NULL ) \
    throwError( env, e2 ); \
}

#define GET_ARRAY_VAL_DEF2( Val, RetType, ElemType, DestType, DestType2, cvterr, \
                            env, field, NewArray, SetArrayRegion ) \
{ \
  RaiException e2  = NULL; \
  ElemType  * tmp = NULL; \
  RaiMsg_size len; \
  RaiMsg_data data; \
  RaiMsg_size sz; \
  RaiMsg_type tp; \
 \
  try { \
    switch ( field.Type() ) { \
      case RAIMSG_OPAQUE: \
      case RAIMSG_STRING: \
      case RAIMSG_PARTIAL: \
        len  = field.Size(); \
        data = field.Data(); \
        sz   = 1; \
        tp   = RAIMSG_UINT; \
        if ( 0 ) { \
      case RAIMSG_ARRAY: \
          len  = field.NumEntries(); \
          data = field.Data(); \
          sz   = field.EntrySize(); \
          tp   = field.EntryType(); \
        } \
        try { \
          if ( sz == sizeof( tmp[ 0 ] ) && \
               ( tp == DestType || tp == DestType2 ) ) \
            tmp = (ElemType *) data; \
          else { \
            unsigned int i, off; \
            MALLOC( len * sizeof( tmp[ 0 ] ), &tmp ); \
            off = 0; \
            for ( i = 0; i < len; i++, off += sz ) \
              RaiField::Convert( DestType, sizeof( tmp[ 0 ] ), &tmp[ i ], \
                                 tp, sz, &((byte *) data)[ off ] ); \
          } \
 \
          if ( (Val = (env->NewArray)( len )) != NULL ) \
            (env->SetArrayRegion)( Val, 0, len, tmp ); \
        } catch ( RaiException e ) { \
          e2 = e; \
        } \
        if ( tmp != (ElemType *) data ) \
          FREE( tmp ); \
        break; \
 \
      default: \
        e2 = RaiMsgErr::getErr( cvterr ); \
        break; \
    } \
  } catch ( RaiException e ) { \
    e2 = e; \
  } \
 \
  if ( e2 != NULL ) \
    throwError( env, e2 ); \
}

/*
 * Class:     RaiMsg
 * Method:    GetBooleanArray
 * Signature: (JLjava/lang/String;)[Z
 */
JNIEXPORT jbooleanArray JNICALL
JaRaiMsg( GetBooleanArray )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  /*return GetArrayVal<jbooleanArray, jboolean, RAIMSG_BOOLEAN, RAIMSG_BOOLEAN,
                     RaiMsgErr::BAD_CVT_BOOL>
                     ( env, me, name, &JNIEnv_::NewBooleanArray,
                       &JNIEnv_::SetBooleanArrayRegion );*/
  jbooleanArray val = NULL;
  GET_ARRAY_VAL_DEF1( val, jbooleanArray, jboolean, RAIMSG_BOOLEAN,
                      RAIMSG_BOOLEAN, RaiMsgErr::BAD_CVT_BOOL ,
                      env, me, name, NewBooleanArray, SetBooleanArrayRegion );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    GetByteArray
 * Signature: (JLjava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL
JaRaiMsg( GetByteArray )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  jbyteArray val = NULL;
  GET_ARRAY_VAL_DEF1( val, jbyteArray, jbyte, RAIMSG_UINT, RAIMSG_INT,
                      RaiMsgErr::BAD_CVT_INT, env, me, name, NewByteArray,
                      SetByteArrayRegion );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    GetShortArray
 * Signature: (JLjava/lang/String;)[S
 */
JNIEXPORT jshortArray JNICALL
JaRaiMsg( GetShortArray )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  jshortArray val = NULL;
  GET_ARRAY_VAL_DEF1( val, jshortArray, jshort, RAIMSG_UINT, RAIMSG_INT,
                      RaiMsgErr::BAD_CVT_INT, env, me, name, NewShortArray,
                      SetShortArrayRegion );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    GetIntArray
 * Signature: (JLjava/lang/String;)[I
 */
JNIEXPORT jintArray JNICALL
JaRaiMsg( GetIntArray )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  jintArray val = NULL;
  GET_ARRAY_VAL_DEF1( val, jintArray, jint, RAIMSG_UINT, RAIMSG_INT,
                      RaiMsgErr::BAD_CVT_INT, env, me, name, NewIntArray,
                      SetIntArrayRegion );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    GetLongArray
 * Signature: (JLjava/lang/String;)[J
 */
JNIEXPORT jlongArray JNICALL
JaRaiMsg( GetLongArray )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  jlongArray val = NULL;
  GET_ARRAY_VAL_DEF1( val, jlongArray, jlong, RAIMSG_UINT, RAIMSG_INT,
                      RaiMsgErr::BAD_CVT_INT, env, me, name, NewLongArray,
                      SetLongArrayRegion );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    GetFloatArray
 * Signature: (JLjava/lang/String;)[F
 */
JNIEXPORT jfloatArray JNICALL
JaRaiMsg( GetFloatArray )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  jfloatArray val = NULL;
  GET_ARRAY_VAL_DEF1( val, jfloatArray, jfloat, RAIMSG_REAL, RAIMSG_REAL,
                      RaiMsgErr::BAD_CVT_REAL, env, me, name, NewFloatArray,
                      SetFloatArrayRegion );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    GetDoubleArray
 * Signature: (JLjava/lang/String;)[D
 */
JNIEXPORT jdoubleArray JNICALL
JaRaiMsg( GetDoubleArray )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  jdoubleArray val = NULL;
  GET_ARRAY_VAL_DEF1( val, jdoubleArray, jdouble, RAIMSG_REAL, RAIMSG_REAL,
                      RaiMsgErr::BAD_CVT_REAL, env, me, name, NewDoubleArray,
                      SetDoubleArrayRegion );
  return val;
}


static jobjectArray
GetStringArray( JNIEnv *env,  RaiField &field )
{
  jobjectArray val = NULL;
  RaiException e2  = NULL;

  if ( field.Type() == RAIMSG_ARRAY ) {
    try {
      RaiMsg_size len  = field.NumEntries();
      RaiMsg_data data = field.Data();
      RaiMsg_size sz   = field.EntrySize();
      RaiMsg_type tp   = field.EntryType();
      unsigned int i, off;
      jstring      s;

      val = env->NewObjectArray( len, String_cls, NULL );
      if ( tp == RAIMSG_STRING || tp == RAIMSG_OPAQUE ) {
        off = 0;
        for ( i = 0; i < len; i++, off += sz ) {
          const char * as = &((char *) data)[ off ];
          if ( rai::StrUtil::strnlen( as, sz ) == sz ) {
            char * tmp;
            MALLOC( sz + 1, &tmp );
            ::memcpy( tmp, as, sz );
            tmp[ sz ] = '\0';
            s = env->NewStringUTF( tmp ); 
            FREE( tmp );
          }
          else {
            s = env->NewStringUTF( as );
          }
          env->SetObjectArrayElement( val, (jsize) i, (jobject) s );
        }
      }
      else {
        char buf[ 256 ];
        off = 0;
        for ( i = 0; i < len; i++, off += sz ) {
          RaiField::Convert( RAIMSG_STRING, sizeof( buf ), buf,
                             tp, sz, &((byte *) data)[ off ] );
          s = env->NewStringUTF( buf );
          env->SetObjectArrayElement( val, (jsize) i, (jobject) s );
        }
      }
    } catch ( RaiException e ) {
      e2 = e;
    }
  }
  else {
    e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_CVT_STRING );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}


/*
 * Class:     RaiMsg
 * Method:    GetStringArray
 * Signature: (JLjava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
JaRaiMsg( GetStringArray )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  RaiMsg     * _this = toRaiMsg( me );
  jobjectArray val   = NULL;
  RaiException e2    = NULL;
  const char * fname = ( name == NULL ? NULL :
                          env->GetStringUTFChars( name, NULL ) );
  try {
    RaiField field;
    if ( _this->Get( fname, field ) )
      val = GetStringArray( env, field );
    else
      e2 = RaiMsgErr::getErr( RaiMsgErr::NOT_FOUND );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
  return val;
}

/*
 * Class:     RaiMsg
 * Method:    Append
 * Signature: (JLjava/lang/String;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( Append__JLjava_lang_String_2Ljava_lang_Object_2 )( JNIEnv *env,
                               jclass /* cls */, jlong me, jstring name, jobject val )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppend( env, *_this, name, val, NULL, true, false );
}

/*
 * Class:     RaiMsg
 * Method:    AppendWithHint
 * Signature: (JLjava/lang/String;Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendWithHint )(JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                           jobject val, jobject hintVal )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppend( env, *_this, name, val, hintVal, true, false );
}


static void
updateOrAppendOpaque( JNIEnv *env,  RaiMsg &msg,  jstring name,
                      jbyteArray buf,  jint off,  jint len,  bool isAppend )
{
  RaiException e2 = NULL;
  jbyte     * els = env->GetByteArrayElements( buf, NULL );

  if ( els != NULL ) {
    const char * fname = ( name == NULL ? NULL :
                            env->GetStringUTFChars( name, NULL ) );
    try {
      if ( isAppend )
        msg.Append( fname, RAIMSG_OPAQUE, len, &els[ off ] );
      else
        msg.Update( fname, RAIMSG_OPAQUE, len, &els[ off ] );
    } catch ( RaiException e ) {
      e2 = e;
    }
    env->ReleaseByteArrayElements( buf, els, JNI_ABORT );
    if ( fname != NULL )
      env->ReleaseStringUTFChars( name, fname );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     RaiMsg
 * Method:    AppendOpaque
 * Signature: (JLjava/lang/String;[BII)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendOpaque )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                          jbyteArray buf, jint off, jint len )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppendOpaque( env, *_this, name, buf, off, len, true );
}

/*
 * Class:     RaiMsg
 * Method:    AppendUnsigned
 * Signature: (JLjava/lang/String;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendUnsigned )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                            jobject val )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppend( env, *_this, name, val, NULL, true, true );
}

/*
 * Class:     RaiMsg
 * Method:    Append
 * Signature: (JLcom_rai_raimsg_RaiField;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( Append__JLcom_rai_raimsg_RaiField_2 )( JNIEnv *env, jclass /* cls */,
                                                 jlong me, jobject val )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppendField( env, *_this, val, true );
}


template<class JType, class RaiType>
static void
append( JNIEnv *env,  RaiMsg &msg,  jstring name,  JType val )
{
  const char * fname = ( name == NULL ? NULL :
                          env->GetStringUTFChars( name, NULL ) );
  RaiException e2    = NULL;

  try {
    msg.Append( fname, (RaiType) val );
  } catch ( RaiException e ) {
    e2 = e;
  }

  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    AppendBoolean
 * Signature: (JLjava/lang/String;Z)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendBoolean )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                           jboolean val )
{
  RaiMsg * _this = toRaiMsg( me );
  append<jboolean, bool>( env, *_this, name, val != 0 );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    AppendByte
 * Signature: (JLjava/lang/String;BZ)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendByte )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                        jbyte val, jboolean isUnsigned )
{
  RaiMsg * _this = toRaiMsg( me );
  if ( isUnsigned )
    append<jbyte, Rai_u8>( env, *_this, name, val );
  else
    append<jbyte, Rai_i8>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    AppendShort
 * Signature: (JLjava/lang/String;SZ)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendShort )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                         jshort val, jboolean isUnsigned )
{
  RaiMsg * _this = toRaiMsg( me );
  if ( isUnsigned )
    append<jshort, Rai_u16>( env, *_this, name, val );
  else
    append<jshort, Rai_i16>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    AppendInt
 * Signature: (JLjava/lang/String;IZ)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendInt )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                       jint val, jboolean isUnsigned )
{
  RaiMsg * _this = toRaiMsg( me );
  if ( isUnsigned )
    append<jint, Rai_u32>( env, *_this, name, val );
  else
    append<jint, Rai_i32>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    AppendLong
 * Signature: (JLjava/lang/String;JZ)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendLong )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                        jlong val, jboolean isUnsigned )
{
  RaiMsg * _this = toRaiMsg( me );
  if ( isUnsigned )
    append<jlong, Rai_u64>( env, *_this, name, val );
  else
    append<jlong, Rai_i64>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    AppendFloat
 * Signature: (JLjava/lang/String;F)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendFloat )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                         jfloat val )
{
  RaiMsg * _this = toRaiMsg( me );
  append<jfloat, Rai_f32>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    AppendDouble
 * Signature: (JLjava/lang/String;D)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( AppendDouble )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                          jdouble val )
{
  RaiMsg * _this = toRaiMsg( me );
  append<jdouble, Rai_f64>( env, *_this, name, val );
}

/*
 * Class:     RaiMsg
 * Method:    Update
 * Signature: (JLjava/lang/String;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( Update__JLjava_lang_String_2Ljava_lang_Object_2 )( JNIEnv *env,
                               jclass /* cls */, jlong me, jstring name, jobject val )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppend( env, *_this, name, val, NULL, false, false );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    UpdateWithHint
 * Signature: (JLjava/lang/String;Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateWithHint )(JNIEnv *env, jclass /* cls */, jlong me, jstring name,
          jobject val, jobject hintVal )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppend( env, *_this, name, val, hintVal, false, false );
}

/*
 * Class:     RaiMsg
 * Method:    UpdateOpaque
 * Signature: (JLjava/lang/String;[BII)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateOpaque )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                          jbyteArray buf, jint off, jint len )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppendOpaque( env, *_this, name, buf, off, len, false );
}

/*
 * Class:     RaiMsg
 * Method:    UpdateUnsigned
 * Signature: (JLjava/lang/String;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateUnsigned )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                            jobject val )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppend( env, *_this, name, val, NULL, false, true );
}

/*
 * Class:     RaiMsg
 * Method:    Update
 * Signature: (JLcom_rai_raimsg_RaiField;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( Update__JLcom_rai_raimsg_RaiField_2 )( JNIEnv *env, jclass /* cls */,
                                                 jlong me, jobject val )
{
  RaiMsg * _this = toRaiMsg( me );
  updateOrAppendField( env, *_this, val, false );
}


template<class JType, class RaiType>
static void
update( JNIEnv *env,  RaiMsg &msg,  jstring name,  JType val )
{
  const char * fname = ( name == NULL ? NULL :
                          env->GetStringUTFChars( name, NULL ) );
  RaiException e2    = NULL;

  try {
    msg.Update( fname, (RaiType) val );
  } catch ( RaiException e ) {
    e2 = e;
  }

  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    UpdateBoolean
 * Signature: (JLjava/lang/String;Z)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateBoolean )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                           jboolean val )
{
  RaiMsg * _this = toRaiMsg( me );
  update<jboolean, bool>( env, *_this, name, val != 0 );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    UpdateByte
 * Signature: (JLjava/lang/String;BZ)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateByte )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                        jbyte val, jboolean isUnsigned )
{
  RaiMsg * _this = toRaiMsg( me );
  if ( isUnsigned )
    update<jbyte, Rai_u8>( env, *_this, name, val );
  else
    update<jbyte, Rai_i8>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    UpdateShort
 * Signature: (JLjava/lang/String;SZ)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateShort )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                         jshort val, jboolean isUnsigned )
{
  RaiMsg * _this = toRaiMsg( me );
  if ( isUnsigned )
    update<jshort, Rai_u16>( env, *_this, name, val );
  else
    update<jshort, Rai_i16>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    UpdateInt
 * Signature: (JLjava/lang/String;IZ)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateInt )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                       jint val, jboolean isUnsigned )
{
  RaiMsg * _this = toRaiMsg( me );
  if ( isUnsigned )
    update<jint, Rai_u32>( env, *_this, name, val );
  else
    update<jint, Rai_i32>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    UpdateLong
 * Signature: (JLjava/lang/String;JZ)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateLong )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                        jlong val, jboolean isUnsigned )
{
  RaiMsg * _this = toRaiMsg( me );
  if ( isUnsigned )
    update<jlong, Rai_u64>( env, *_this, name, val );
  else
    update<jlong, Rai_i64>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    UpdateFloat
 * Signature: (JLjava/lang/String;F)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateFloat )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                         jfloat val )
{
  RaiMsg * _this = toRaiMsg( me );
  update<jfloat, Rai_f32>( env, *_this, name, val );
}

/*
 * Class:     com_rai_raimsg_RaiMsg
 * Method:    UpdateDouble
 * Signature: (JLjava/lang/String;D)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UpdateDouble )( JNIEnv *env, jclass /* cls */, jlong me, jstring name,
                          jdouble val )
{
  RaiMsg * _this = toRaiMsg( me );
  update<jdouble, Rai_f64>( env, *_this, name, val );
}

/*
 * Class:     RaiMsg
 * Method:    UnPack
 * Signature: (J[BII)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( UnPack )( JNIEnv *env, jclass /* cls */, jlong me, jbyteArray msgBuf,
                    jint off, jint len )
{
  RaiMsg        * _this = toRaiMsg( me );
  RaiMsg_size     off2;
  RaiMsg_protocol proto;
  byte          * mem = NULL;
  RaiException    e2  = NULL;

  try {
    if ( len == 0 )
      e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_MAGIC_NUMBER );
    else {
      MALLOC( len, &mem );
      env->GetByteArrayRegion( msgBuf, off, len, (jbyte *) mem );
      if ( ! RaiMsg::ExtractProtocolEx( mem, len, proto, off2 ) )
        e2 = RaiMsgErr::getErr( RaiMsgErr::BAD_MAGIC_NUMBER );
      else {
        _this->UnPack( proto, mem, len, RAIMSG_MEMORY_DYNAMIC, off2 );
        mem = NULL;
      }
    }
  } catch ( RaiException e ) {
    e2 = e;
  }

  if ( mem != NULL )
    FREE( mem );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     RaiMsg
 * Method:    Pack
 * Signature: (J[BII)I
 */
JNIEXPORT jint JNICALL
JaRaiMsg( Pack__J_3BII )( JNIEnv *env, jclass /* cls */, jlong me, jbyteArray msgBuf,
                          jint off, jint len )
{
  RaiMsg    * _this = toRaiMsg( me );
  RaiMsg_size size;

  try {
    size = _this->PackSize();
    if ( (jint) size < len )
      len = (jint) size;
    env->SetByteArrayRegion( msgBuf, off, len, (jbyte *) _this->Packed() );
  } catch ( RaiException e ) {
    throwError( env, e );
  }

  return len;
}

/*
 * Class:     RaiMsg
 * Method:    Pack
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL
JaRaiMsg( Pack__J )( JNIEnv *env, jclass /* cls */, jlong me )
{
  RaiMsg    * _this  = toRaiMsg( me );
  jbyteArray  msgBuf = NULL;
  RaiMsg_size size;

  try {
    size = _this->PackSize();
    if ( (msgBuf = env->NewByteArray( size )) != NULL )
      env->SetByteArrayRegion( msgBuf, 0, size, (jbyte *) _this->Packed() );
  } catch ( RaiException e ) {
    throwError( env, e );
  }

  return msgBuf;
}

/*
 * Class:     RaiMsg
 * Method:    PackSize
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiMsg( PackSize )( JNIEnv *env, jclass /* cls */, jlong me )
{
  RaiMsg    * _this = toRaiMsg( me );
  RaiMsg_size size  = 0;

  try {
    size = _this->PackSize();
  } catch ( RaiException e ) {
    throwError( env, e );
  }

  return (jint) size;
}

/*
 * Class:     RaiMsg
 * Method:    Activate
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT jboolean JNICALL
JaRaiMsg( Activate )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  RaiMsg     * _this = toRaiMsg( me );
  RaiException e2;
  const char * fname;
  bool         b = false;

  e2    = NULL;
  fname = ( name == NULL ? NULL : env->GetStringUTFChars( name, NULL ) );

  try {
    b = _this->Activate( fname );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
  return (jboolean) b;
}

/*
 * Class:     RaiMsg
 * Method:    Rename
 * Signature: (JLjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT jboolean JNICALL
JaRaiMsg( Rename )( JNIEnv *env, jclass /* cls */, jlong me, jstring oldName,
                    jstring newName )
{
  RaiMsg     * _this = toRaiMsg( me );
  const char * fname1,
             * fname2;
  bool         b = false;
  RaiException e2 = NULL;

  fname1 = ( oldName == NULL ? NULL : env->GetStringUTFChars( oldName, NULL ) );
  fname2 = ( newName == NULL ? NULL : env->GetStringUTFChars( newName, NULL ) );

  try {
    b = _this->Rename( fname1, fname2 );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname1 != NULL )
    env->ReleaseStringUTFChars( oldName, fname1 );
  if ( fname2 != NULL )
    env->ReleaseStringUTFChars( newName, fname2 );
  if ( e2 != NULL )
    throwError( env, e2 );
  return (jboolean) b;
}

/*
 * Class:     RaiMsg
 * Method:    Remove
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT jboolean JNICALL
JaRaiMsg( Remove )( JNIEnv *env, jclass /* cls */, jlong me, jstring name )
{
  RaiMsg     * _this = toRaiMsg( me );
  const char * fname;
  bool         b = false;
  RaiException e2 = NULL;

  fname = ( name == NULL ? NULL : env->GetStringUTFChars( name, NULL ) );

  try {
    b = _this->Remove( fname );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
  return (jboolean) b;
}

struct RaiMsgJOutputStream : public rai::OutputStream {
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
    RaiMsgJOutputStream( JNIEnv *env,  jobject out ) {
      this->env = env;
      this->out = out;
    };
};

/*
 * Class:     RaiMsg
 * Method:    Print
 * Signature: (JLjava/io/OutputStream;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( Print )( JNIEnv *env, jclass /* cls */, jlong me, jobject out,
                   jboolean field_nl,  jstring fname_fmt,  jboolean print_op,
                   jstring dbg_format,  jstring dbg_hformat )
{
  RaiMsg     * _this     = toRaiMsg( me );
  const char * fname_format = NULL,
             * debug_fmt    = NULL,
             * debug_hfmt   = NULL;
  RaiException e2           = NULL;

  fname_format = ( fname_fmt == NULL ? NULL :
                   env->GetStringUTFChars( fname_fmt, NULL ) );
  debug_fmt    = ( dbg_format == NULL ? NULL :
                   env->GetStringUTFChars( dbg_format, NULL ) );
  debug_hfmt   = ( dbg_hformat == NULL ? NULL :
                   env->GetStringUTFChars( dbg_hformat, NULL ) );
  try {
    RaiMsgJOutputStream jout( env, out );
    _this->Print( &jout, (bool) ( field_nl != 0 ), fname_format,
                  (bool) ( print_op != 0 ), debug_fmt, debug_hfmt );
    jout.flush();
  } catch ( RaiException e ) {
    e2 = e;
  }

  if ( fname_format != NULL )
    env->ReleaseStringUTFChars( fname_fmt, fname_format );
  if ( debug_fmt != NULL )
    env->ReleaseStringUTFChars( dbg_format, debug_fmt );
  if ( debug_hfmt != NULL )
    env->ReleaseStringUTFChars( dbg_hformat, debug_hfmt );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     RaiMsg
 * Method:    PrintHex
 * Signature: (JLjava/io/OutputStream;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( PrintHex__JLjava_io_OutputStream_2 )( JNIEnv *env, jclass /* cls */,
                                                jlong me, jobject out )
{
  RaiMsg * _this = toRaiMsg( me );

  try {
    RaiMsgJOutputStream jout( env, out );
    _this->PrintHex( &jout );
    jout.flush();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
}

/*
 * Class:     RaiMsg
 * Method:    PrintHex
 * Signature: (Ljava/io/OutputStream;[BII)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( PrintHex__Ljava_io_OutputStream_2_3BII )( JNIEnv *env, jclass /* cls */,
                          jobject out, jbyteArray msgBuf, jint off, jint len )
{
  jbyte * buf;

  if ( (buf = env->GetByteArrayElements( msgBuf, NULL )) != NULL ) {
    try {
      RaiMsgJOutputStream jout( env, out );
      RaiMsg::PrintHex( &jout, (Rai_u8 *) &buf[ off ], len );
      jout.flush();
    } catch ( RaiException e ) {
      throwError( env, e );
    }
    env->ReleaseByteArrayElements( msgBuf, buf, JNI_ABORT );
  }
}

/*
 * Class:     RaiMsg
 * Method:    PrintXML
 * Signature: (JLjava/io/OutputStream;IZLjava/lang/String;[Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( PrintXML )( JNIEnv *env, jclass /* cls */, jlong me, jobject out,
                      jint attr_flags, jboolean field_nl, jstring msg_name,
                      jobjectArray msg_atts )
{
  RaiMsg     * _this = toRaiMsg( me );
  RaiException e2    = NULL;
  const char * name  = NULL,
             * atts[ 101 ];
  jstring      jatts[ 101 ];
  unsigned int i;

  atts[ 0 ]  = NULL;
  jatts[ 0 ] = NULL;
  name = ( msg_name == NULL ? NULL :
           env->GetStringUTFChars( msg_name, NULL ) );
  if ( msg_atts != NULL ) {
    jsize len = env->GetArrayLength( msg_atts );
    for ( i = 0; i < 100 && i < (unsigned int) len; i++ ) {
      jatts[ i + 1 ] = NULL;
      atts[ i + 1 ]  = NULL;

      jatts[ i ] = (jstring) env->GetObjectArrayElement( msg_atts, i );
      if ( jatts[ i ] == NULL )
        break;
      atts[ i ] = env->GetStringUTFChars( jatts[ i ], NULL );
      if ( atts[ i ] == NULL )
        break;
    }
  }

  if ( ! env->ExceptionCheck() ) {
    try {
      RaiMsgJOutputStream jout( env, out );
      _this->PrintXML( &jout, attr_flags, field_nl ? 1 : 0, name, atts );
      jout.flush();
    } catch ( RaiException e ) {
      e2 = e;
    }
  }

  if ( name != NULL )
    env->ReleaseStringUTFChars( msg_name, name );
  if ( atts[ 0 ] != NULL ) {
    for ( i = 0; i < 100 && atts[ i ] != NULL; i++ )
      env->ReleaseStringUTFChars( jatts[ i ], atts[ i ] );
  }
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     RaiMsg
 * Method:    StrType
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
JaRaiMsg( StrType )( JNIEnv *env, jclass /* cls */, jstring name )
{
  const char * tname;
  RaiMsg_type  type;

  tname = ( name == NULL ? NULL : env->GetStringUTFChars( name, NULL ) );
  type  = RaiMsg::StrType( tname );
  if ( tname != NULL )
    env->ReleaseStringUTFChars( name, tname );

  return (jint) type;
}

/*
 * Class:     RaiMsg
 * Method:    TypeStr
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsg( TypeStr )( JNIEnv *env, jclass /* cls */, jint typ )
{
  return env->NewStringUTF( RaiMsg::TypeStr( (RaiMsg_type) typ ) );
}

/*
 * Class:     RaiMsg
 * Method:    ReadDataDictionary
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;C)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( ReadDataDictionary )( JNIEnv *env, jclass /* cls */, jstring fields_cf,
                                jstring records_cf, jstring cfile_path,
                                jchar path_sep )
{
  RaiMsg_config * dict,
                * curDict;
  const char    * fields,
                * records,
                * path;
  RaiException    e2;

  e2 = NULL;
  fields  = ( fields_cf == NULL ? NULL :
             env->GetStringUTFChars( fields_cf, NULL ) );
  records = ( records_cf == NULL ? NULL :
              env->GetStringUTFChars( records_cf, NULL ) );
  path    = ( cfile_path == NULL ? NULL :
              env->GetStringUTFChars( cfile_path, NULL ) );

  try {
    dict = RaiMsg_config::parseDictionary( fields, records, path,
                                           (char) path_sep, NULL );
    curDict = RaiMsg::GetDataDictionary();
    RaiMsg::SetDataDictionary( dict );
    if ( curDict != NULL )
      RaiMsg_config::release( curDict );
  } catch ( RaiException e ) {
    e2 = e;
  }

  if ( fields != NULL )
    env->ReleaseStringUTFChars( fields_cf, fields );
  if ( records_cf != NULL )
    env->ReleaseStringUTFChars( records_cf, records );
  if ( path != NULL )
    env->ReleaseStringUTFChars( cfile_path, path );
  if ( e2 != NULL )
    throwError( env, e2 );
}

/*
 * Class:     RaiMsg
 * Method:    SetDataDictionary
 * Signature: (LRaiMsg;)V
 */
JNIEXPORT void JNICALL
JaRaiMsg( SetDataDictionary )( JNIEnv *env, jclass cls, jobject msg )
{
  RaiMsg_config * dict,
                * curDict;
  RaiMsg        * dictMsg;
  jfieldID        fid;

  if ( msg == NULL ) {
    curDict = RaiMsg::GetDataDictionary();
    RaiMsg::SetDataDictionary( NULL );
    if ( curDict != NULL )
      RaiMsg_config::release( curDict );
  }
  else {
    fid     = env->GetFieldID( cls, "msg", "J" );
    dictMsg = toRaiMsg( env->GetLongField( msg, fid ) );
    try {
      dict    = RaiMsg_config::unpackDataDictionary( *dictMsg );
      curDict = RaiMsg::GetDataDictionary();
      RaiMsg::SetDataDictionary( dict );
      if ( curDict != NULL )
        RaiMsg_config::release( curDict );
    } catch ( RaiException e ) {
      throwError( env, e );
    }
  }
}

/*
 * Class:     RaiMsg
 * Method:    GetDataDictionary
 * Signature: ()LRaiMsg;
 */
JNIEXPORT jobject JNICALL
JaRaiMsg( GetDataDictionary )( JNIEnv *env, jclass /* cls */ )
{
  jobject         obj;
  RaiMsg_config * curDict;
  RaiMsg        * dictMsg;

  obj     = NULL;
  dictMsg = NULL;
  try {
    curDict = RaiMsg::GetDataDictionary();
    if ( curDict != NULL ) {
      dictMsg = NEW RaiMsg();
      curDict->packDataDictionary( *dictMsg );
      obj = env->NewObject( RaiMsg_cls, RaiMsg_mid_J, toLong( dictMsg ) );
    }
  } catch ( RaiException e ) {
    if ( dictMsg != NULL )
      delete dictMsg;
    throwError( env, e );
  }
  return obj;
}

/*
 * Class:     RaiField
 * Method:    Create
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
JaRaiField( Create )( JNIEnv *env, jclass /* cls */ )
{
  RaiField * field = NULL;
  try {
    field = NEW RaiField();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return toLong( field );
}

/*
 * Class:     RaiField
 * Method:    Delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JaRaiField( Delete )( JNIEnv */* env */, jclass /* cls */, jlong fld )
{
  if ( fld != 0 )
    delete toRaiField( fld );
}

/*
 * Class:     RaiField
 * Method:    Name
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiField( Name )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  RaiField * _this = toRaiField( fld );
  jstring    s     = NULL;

  if ( _this->name != NULL )
    s = env->NewStringUTF( _this->name );

  return s;
}

/*
 * Class:     RaiField
 * Method:    Type
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiField( Type )( JNIEnv */* env */, jclass /* cls */, jlong fld )
{
  return (jint) toRaiField( fld )->Type();
}

/*
 * Class:     RaiField
 * Method:    Size
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiField( Size )( JNIEnv */* env */, jclass /* cls */, jlong fld )
{
  return (jint) toRaiField( fld )->Size();
}

/*
 * Class:     RaiField
 * Method:    HintType
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiField( HintType )( JNIEnv */* env */, jclass /* cls */, jlong fld )
{
  return (jint) toRaiField( fld )->HintType();
}

/*
 * Class:     RaiField
 * Method:    HintSize
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiField( HintSize )( JNIEnv */* env */, jclass /* cls */, jlong fld )
{
  return (jint) toRaiField( fld )->HintSize();
}

/*
 * Class:     RaiField
 * Method:    EntryType
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiField( EntryType )( JNIEnv */* env */, jclass /* cls */, jlong fld )
{
  return (jint) toRaiField( fld )->EntryType();
}

/*
 * Class:     RaiField
 * Method:    EntrySize
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiField( EntrySize )( JNIEnv */* env */, jclass /* cls */, jlong fld )
{
  return (jint) toRaiField( fld )->EntrySize();
}

/*
 * Class:     RaiField
 * Method:    NumEntries
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiField( NumEntries )( JNIEnv */* env */, jclass /* cls */, jlong fld )
{
  return (jint) toRaiField( fld )->NumEntries();
}

/*
 * Class:     RaiField
 * Method:    Get
 * Signature: (J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL
JaRaiField( Get )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  RaiField * _this = toRaiField( fld );
  jobject    obj   = NULL;
  try {
    obj = boxField( env, *_this );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return obj;
}

/* gcc badness... */
#if 0
template <class RetType, class ValType>
static RetType
GetVal( JNIEnv *env, RaiField &field )
{
  ValType val = (ValType) 0;
  try {
    field.Get( val );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return (RetType) val;
}
#endif
#define BADGCC_GET_VAL( RetType, ValType, ENV, FLD ) { \
  RaiField &field = FLD; \
  ValType val = (ValType) 0; \
  try { \
    field.Get( val ); \
  } catch ( RaiException e ) { \
    throwError( ENV, e ); \
  } \
  return (RetType) val; \
}

/*
 * Class:     RaiField
 * Method:    GetBoolean
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
JaRaiField( GetBoolean )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  BADGCC_GET_VAL( jboolean, bool, env, *(toRaiField( fld )) )
  /*return GetVal<jboolean, bool>( env, *(toRaiField( fld )) );*/
}

/*
 * Class:     RaiField
 * Method:    GetByte
 * Signature: (J)B
 */
JNIEXPORT jbyte JNICALL
JaRaiField( GetByte )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  BADGCC_GET_VAL( jbyte, Rai_i8, env, *(toRaiField( fld )) )
  /*return GetVal<jbyte, Rai_i8>( env, *(toRaiField( fld )) );*/
}

/*
 * Class:     RaiField
 * Method:    GetShort
 * Signature: (J)S
 */
JNIEXPORT jshort JNICALL
JaRaiField( GetShort )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  BADGCC_GET_VAL( jshort, Rai_i16, env, *(toRaiField( fld )) )
  /*return GetVal<jshort, Rai_i16>( env, *(toRaiField( fld )) );*/
}

/*
 * Class:     RaiField
 * Method:    GetInt
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiField( GetInt )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  BADGCC_GET_VAL( jint, Rai_i32, env, *(toRaiField( fld )) )
  /*return GetVal<jint, Rai_i32>( env, *(toRaiField( fld )) );*/
}

/*
 * Class:     RaiField
 * Method:    GetLong
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
JaRaiField( GetLong )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  BADGCC_GET_VAL( jlong, Rai_i64, env, *(toRaiField( fld )) )
  /*return GetVal<jlong, Rai_i64>( env, *(toRaiField( fld )) );*/
}

void
bad_gcc_my_getf32( RaiField &field, Rai_f32 &val )
{
  field.Get( val );
}

/*
 * Class:     RaiField
 * Method:    GetFloat
 * Signature: (J)F
 */
JNIEXPORT jfloat JNICALL
JaRaiField( GetFloat )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  RaiField &field = *(toRaiField( fld ));
  Rai_f32 val;
  try {
    bad_gcc_my_getf32( field, val );
  } catch ( RaiException e ) {
    throwError( env, e );
    val = 0;
  }
  return val;
}

void
bad_gcc_my_getf64( RaiField &field, Rai_f64 &val )
{
  field.Get( val );
}

/*
 * Class:     RaiField
 * Method:    GetDouble
 * Signature: (J)D
 */
JNIEXPORT jdouble JNICALL
JaRaiField( GetDouble )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  RaiField &field = *(toRaiField( fld ));
  Rai_f64 val;
  try {
    bad_gcc_my_getf64( field, val );
  } catch ( RaiException e ) {
    throwError( env, e );
    val = 0;
  }
  return val;
}

/*
 * Class:     RaiField
 * Method:    GetString
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiField( GetString )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  return GetString( env, *(toRaiField( fld )) );
}

/*
 * Class:     RaiField
 * Method:    GetOpaque
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL
JaRaiField( GetOpaque )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  return GetOpaque( env,  *(toRaiField( fld )) );
}

/*
 * Class:     RaiField
 * Method:    GetPartial
 * Signature: (J)LPartial;
 */
JNIEXPORT jobject JNICALL
JaRaiField( GetPartial )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  return GetPartial( env,  *(toRaiField( fld )) );
}

/*
 * Class:     RaiField
 * Method:    GetBooleanArray
 * Signature: (J)[Z
 */
JNIEXPORT jbooleanArray JNICALL
JaRaiField( GetBooleanArray )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  jbooleanArray val = NULL;
  RaiField &field = *(toRaiField( fld ));
  GET_ARRAY_VAL_DEF2( val, jbooleanArray, jboolean, RAIMSG_BOOLEAN,
                      RAIMSG_BOOLEAN, RaiMsgErr::BAD_CVT_BOOL,
                      env, field, NewBooleanArray, SetBooleanArrayRegion );
  return val;
}

/*
 * Class:     RaiField
 * Method:    GetByteArray
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL
JaRaiField( GetByteArray )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  jbyteArray val = NULL;
  RaiField &field = *(toRaiField( fld ));
  GET_ARRAY_VAL_DEF2( val, jbyteArray, jbyte, RAIMSG_UINT, RAIMSG_INT,
                      RaiMsgErr::BAD_CVT_INT, env, field, NewByteArray,
                      SetByteArrayRegion );
  return val;
}

/*
 * Class:     RaiField
 * Method:    GetShortArray
 * Signature: (J)[S
 */
JNIEXPORT jshortArray JNICALL
JaRaiField( GetShortArray )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  jshortArray val = NULL;
  RaiField &field = *(toRaiField( fld ));
  GET_ARRAY_VAL_DEF2( val, jshortArray, jshort, RAIMSG_UINT, RAIMSG_INT,
                      RaiMsgErr::BAD_CVT_INT,  env, field, NewShortArray,
                      SetShortArrayRegion );
  return val;
}

/*
 * Class:     RaiField
 * Method:    GetIntArray
 * Signature: (J)[I
 */
JNIEXPORT jintArray JNICALL
JaRaiField( GetIntArray )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  jintArray val = NULL;
  RaiField &field = *(toRaiField( fld ));
  GET_ARRAY_VAL_DEF2( val, jintArray, jint, RAIMSG_UINT, RAIMSG_INT,
                      RaiMsgErr::BAD_CVT_INT, env, field, NewIntArray,
                      SetIntArrayRegion );
  return val;
}

/*
 * Class:     RaiField
 * Method:    GetLongArray
 * Signature: (J)[J
 */
JNIEXPORT jlongArray JNICALL
JaRaiField( GetLongArray )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  jlongArray val = NULL;
  RaiField &field = *(toRaiField( fld ));
  GET_ARRAY_VAL_DEF2( val, jlongArray, jlong, RAIMSG_UINT, RAIMSG_INT,
                      RaiMsgErr::BAD_CVT_INT, env, field, NewLongArray,
                      SetLongArrayRegion );
  return val;
}

/*
 * Class:     RaiField
 * Method:    GetFloatArray
 * Signature: (J)[F
 */
JNIEXPORT jfloatArray JNICALL
JaRaiField( GetFloatArray )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  jfloatArray val = NULL;
  RaiField &field = *(toRaiField( fld ));
  GET_ARRAY_VAL_DEF2( val, jfloatArray, jfloat, RAIMSG_REAL, RAIMSG_REAL,
                      RaiMsgErr::BAD_CVT_REAL, env, field, NewFloatArray,
                      SetFloatArrayRegion );
  return val;
}

/*
 * Class:     RaiField
 * Method:    GetDoubleArray
 * Signature: (J)[D
 */
JNIEXPORT jdoubleArray JNICALL
JaRaiField( GetDoubleArray )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  jdoubleArray val = NULL;
  RaiField &field = *(toRaiField( fld ));
  GET_ARRAY_VAL_DEF2( val, jdoubleArray, jdouble, RAIMSG_REAL, RAIMSG_REAL,
                      RaiMsgErr::BAD_CVT_REAL, env, field, NewDoubleArray,
                      SetDoubleArrayRegion );
  return val;
}

/*
 * Class:     RaiField
 * Method:    GetStringArray
 * Signature: (J)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
JaRaiField( GetStringArray )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  return GetStringArray( env, *(toRaiField( fld )) );
}

/*
 * Class:     RaiField
 * Method:    GetHint
 * Signature: (J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL
JaRaiField( GetHint )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  RaiField * _this = toRaiField( fld );
  jobject    obj   = NULL;
  try {
    obj = boxFieldHint( env, *_this );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return obj;
}

/*
 * Class:     RaiField
 * Method:    Find
 * Signature: (JJLjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
JaRaiField( Find )( JNIEnv *env, jclass /* cls */, jlong fld, jlong msg,
                    jstring name )
{
  RaiField   * _this  = toRaiField( fld );
  RaiMsg     * m      = toRaiMsg( msg );
  const char * fname;
  RaiException e2     = NULL;
  bool         result = false;

  fname = ( name == NULL ? NULL : env->GetStringUTFChars( name, NULL ) );
  try {
    result = _this->Find( m, fname );
  } catch ( RaiException e ) {
    e2 = e;
  }
  if ( fname != NULL )
    env->ReleaseStringUTFChars( name, fname );
  if ( e2 != NULL )
    throwError( env, e2 );
  return result;
}

/*
 * Class:     RaiField
 * Method:    First
 * Signature: (JJ)Z
 */
JNIEXPORT jboolean JNICALL
JaRaiField( First )( JNIEnv *env, jclass /* cls */, jlong fld, jlong msg )
{
  RaiField * _this  = toRaiField( fld );
  RaiMsg   * m      = toRaiMsg( msg );
  bool       result = false;

  try {
    result = _this->First( m );
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return result;
}

/*
 * Class:     RaiField
 * Method:    Next
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
JaRaiField( Next )( JNIEnv *env, jclass /* cls */, jlong fld )
{
  RaiField * _this  = toRaiField( fld );
  bool       result = false;

  try {
    result = _this->Next();
  } catch ( RaiException e ) {
    throwError( env, e );
  }
  return result;
}

/*
 * Class:     RaiMsgException
 * Method:    getModule
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsgException( getModule )( JNIEnv *env, jclass /* cls */, jlong err )
{
  if ( err == 0 )
    return NULL;
  return env->NewStringUTF( toError( err )->module );
}

/*
 * Class:     RaiMsgException
 * Method:    getErrno
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JaRaiMsgException( getErrno )( JNIEnv */* env */, jclass /* cls */, jlong err )
{
  if ( err == 0 )
    return 0;
  return toError( err )->status;
}

/*
 * Class:     RaiMsgException
 * Method:    getReason
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JaRaiMsgException( getReason )( JNIEnv *env, jclass /* cls */, jlong err )
{
  if ( err == 0 )
    return NULL;
  return env->NewStringUTF( toError( err )->reason );
}
