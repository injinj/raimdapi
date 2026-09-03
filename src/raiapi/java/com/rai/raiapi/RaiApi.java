

/******************************************************************************
 *
 * General Interface
 *
 *****************************************************************************/
package com.rai.raiapi;

import java.util.*;
import com.rai.raimsg.*;
//import com.rai.raiapi2.RaiApi;
//import com.rai.raiexception.RaiException;


public class RaiApi {

  protected static com.rai.raiapi2.RaiApi api2;
  protected static RaiSession defaultSession;

  protected static int refCount = 0;
  /* SubParameter */
  public static final int BOTH   = 0;
  public static final int UPDATE = 1; // defined again later for msg types
  public static final int SNAP   = 2;
  /* RaiTransport */
  public static final int CI   = 1;
  public static final int RV5  = 2;
  public static final int RV7  = 3;
  public static final int MD   = 4;
  public static final int TCP  = 5;
  public static final int HTTP = 6;
  public static final int CAPR = 7;
  /* RaiProto */
  public static final int RDP    = 0;
  public static final int MDRV   = 1;
  public static final int SASS   = 2;
  public static final int SASS2  = 3;
  public static final int SASS3  = 4;
  public static final int RaiMsg = 5;

  public static final int tibrvApi = ( RV7 << 8 ) | SASS2;   /* tibrv */
  public static final int sass2Api = ( RV5 << 8 ) | SASS2;   /* sass2 */
  public static final int sass3Api = ( RV5 << 8 ) | SASS3;   /* sass3 */
  public static final int caprApi  = ( CAPR << 8 ) | RaiMsg; /* capr */
  public static int apiType;

  /* IoctlParameter */
  public static final int SetPubType = 1;
  public static final int SetForm    = 2;
  public static final int SetSubject = 3;
  /* PubType */
  public static final int VERIFY    =  0;
  // update already defined above - wish we had 5.1 enums
  // public static final int UPDATE    =  1;
  public static final int DROP      =  4;
  public static final int INITIAL   =  8;
  public static final int TRANSIENT =  9;
  public static final int SNAPSHOT  = 13;

  static int logLevel = 0;
  public static void SetLogLevel(int value)
  {
    int lvl;
    logLevel = value;
    try {
      switch ( value ) {
        default:
        case 0: lvl = com.rai.raiapi2.RaiApi.LVL_ERROR; break;
        case 1: lvl = com.rai.raiapi2.RaiApi.LVL_NORMAL; break;
        case 2: lvl = com.rai.raiapi2.RaiApi.LVL_MINOR; break;
        case 3: lvl = com.rai.raiapi2.RaiApi.LVL_DEBUG; break;
        case 4: lvl = com.rai.raiapi2.RaiApi.LVL_TRACE; break;
        case 5: lvl = com.rai.raiapi2.RaiApi.LVL_DEVEL; break;
      }
      com.rai.raiapi2.RaiApi.OpenLog( "-", lvl, ( value < 3 ? 4 : 5 ) );
    } catch ( com.rai.raiexception.RaiException e ) {
    }
  }

  public static int GetLogLevel()
  {
    return logLevel;
  }

  public static final int Major = 1;
  public static final int Minor = 2;
  public static final int Debug = 3;
  public static final int Trace = 4;
  public static final int Full  = 5;

  static boolean haveDictionary = false;
  public static boolean GetHaveDict()
  {
    return haveDictionary;
  }

  static int DictTimeoutSeconds = 10;

  public static void SetDictTimeoutSeconds(int value)
  {
    DictTimeoutSeconds = value;
  }
  public static int GetDictTimeoutSeconds()
  {
    return DictTimeoutSeconds;
  }


  static boolean dictionaryLoadInProgress = false;
  public static boolean GetDictLoadInProgress()
  {
    return dictionaryLoadInProgress;
  }

  public static void
  RaiOpen() throws RaiException
  {
    //RaiOpen( RaiApi.RV7, RaiApi.SASS2 );
  }

  protected static RaiException
  getException( com.rai.raiexception.RaiException e,  String str )
  {
    if ( e.getModule() == null )
      return new RaiException( str );
    return new RaiException( e.getErrno(), e.getReason() + " (" + str + ")",
                             e.getModule() );
  }

  public static void
  RaiOpen( int transport, int protocol ) throws RaiException
  {
  }

  public static void
  RaiOpen2( String apiName ) throws RaiException
  {
    if ( refCount == 0 ) {
      int whichApi;
      if ( apiName == null ) {
        apiName  = "tibrv";
        whichApi = tibrvApi; /* default */
      }
      else if ( apiName.equals( "capr" ) )
        whichApi = caprApi;
      else if ( apiName.equals( "sass3" ) )
        whichApi = sass3Api;
      else if ( apiName.equals( "sass2" ) ) {
        apiName = "sass3";
        whichApi = sass2Api;
      }
      else {
        apiName  = "tibrv";
        whichApi = tibrvApi;
      }
      try {
         api2     = com.rai.raiapi2.RaiApi.RaiOpen( apiName, null );
         apiType  = whichApi;
         refCount = 1;
      } catch ( com.rai.raiexception.RaiException e ) {
        throw getException( e, "RaiOpen" );
      }
    }
    else { //refCount !=0
      refCount++;
    }
  }

  public static void
  RaiClose() throws RaiException
  {
    /* loop through open sessions and close off the resources */
    if ( refCount == 1 ) {
      RaiSession s = defaultSession;
      if ( s != null ) {
        defaultSession = null;
        s.Destroy();
      }
      api2.Close();
      api2 = null;
      refCount = 0;
    }
    else if (refCount > 1 )
      refCount--;
    else
      refCount = 0;

  }

  public static String
  RaiVersion()
  {
    String version = null;
    if ( api2 != null )
      version = api2.RaiVersion();
    return version;
  }

  public static void
  RaiIoctl( int parameter, String value ) throws RaiException
  {
    try {
      switch ( parameter ) {
        default:
          break;
      }
    }
    catch (Throwable e) {
      RaiException re =
        new RaiException( RaiException.BAD_IOCTL_PARM, e.toString(), e );
      throw re;
    }
  }

  public static void
  RaiMainloop( RaiSession session ) throws RaiException
  {
    try {
      session.defaultQueue2.Mainloop();
    } catch ( com.rai.raiexception.RaiException e ) {
      throw getException( e, "Mainloop" );
    }
  }

  public static void
  RaiDispatch(RaiSession session) throws RaiException
  {
    try {
      session.defaultQueue2.Dispatch();
    } catch ( com.rai.raiexception.RaiException e ) {
      throw getException( e, "Dispatch" );
    }
  }

  public static void
  RaiTimedDispatch( RaiSession session,
                    double interval ) throws RaiException
  {
    try {
      session.defaultQueue2.TimedDispatch( (int) ( interval * 1000.0 ) );
    } catch ( com.rai.raiexception.RaiException e ) {
      throw getException( e, "TimedDispatch" );
    }
  }


  public static RaiMsg
  NewSASSMsg(int MsgType, short RecType) throws RaiException
  {
    try {
      RaiMsg raiMsg = new RaiMsg();
      raiMsg.SetProtocol(raiMsg.TIB_SASS_PROTO);
      raiMsg.AppendUShort( "MSG_TYPE",(short) MsgType );
      raiMsg.AppendUShort( "REC_TYPE",  (short) RecType );
      raiMsg.AppendUShort( "SEQ_NO", (short) 0 ); //set this when we send
      raiMsg.AppendUShort( "REC_STATUS", (short) 0 );

      return raiMsg;
    }
    catch (Throwable e) {
      RaiException re = new RaiException(RaiException.BAD_RAIMSG,
        "An error has occured attempting to create a SASS type RaiMsg. "
        + e.toString(), e );
      throw (re);
    }
  }

  public static RaiMsg
  NewSASSMsg(int MsgType, String RecType) throws RaiException
  {
    try
    {
      RaiMsg raiMsg = new RaiMsg();
      raiMsg.SetProtocol(raiMsg.TIB_SASS_PROTO);
      raiMsg.AppendUShort( "MSG_TYPE",(short) MsgType );
      raiMsg.SetRecTypeString(RecType);
      raiMsg.AppendUShort( "SEQ_NO", (short) 0 ); //set this when we send
      raiMsg.AppendUShort( "REC_STATUS", (short) 0 );

      return raiMsg;
    }
    catch (Throwable e) {
      RaiException re = new RaiException(RaiException.BAD_RAIMSG,
        "An error has occured attempting to create a SASS type RaiMsg. "
        + e.toString(), e );
      throw (re);
    }
  }

  public static RaiMsg
  NewSASSMsg(int MsgType, short RecType,short SeqNo, short RecStatus) throws RaiException
  {
     try {
       RaiMsg raiMsg = new RaiMsg();
       raiMsg.SetProtocol(raiMsg.TIB_SASS_PROTO);
       raiMsg.AppendUShort( "MSG_TYPE",(short) MsgType );
       raiMsg.AppendUShort( "REC_TYPE",  (short) RecType );
       raiMsg.AppendUShort( "SEQ_NO", SeqNo); //set this when we send
       raiMsg.AppendUShort( "REC_STATUS", RecStatus );

       return raiMsg;
     }
     catch (Throwable e) {
       RaiException re = new RaiException(RaiException.BAD_RAIMSG,
         "An error has occured attempting to create a SASS type RaiMsg. "
         + e.toString(), e );
       throw (re);
     }
  }

  public static RaiMsg
  NewSASSMsg(int MsgType, String RecType,short SeqNo, short RecStatus) throws RaiException
  {
     try
     {
      RaiMsg raiMsg = new RaiMsg();
      raiMsg.SetProtocol(raiMsg.TIB_SASS_PROTO);
      raiMsg.AppendUShort( "MSG_TYPE",(short) MsgType );
      raiMsg.SetRecTypeString(RecType);
      raiMsg.AppendUShort( "SEQ_NO", SeqNo ); //set this when we send
      raiMsg.AppendUShort( "REC_STATUS", RecStatus);

       return raiMsg;
     }
     catch (Throwable e) {
       RaiException re = new RaiException(RaiException.BAD_RAIMSG,
         "An error has occured attempting to create a SASS type RaiMsg. "
         + e.toString(), e );
       throw (re);
     }
  }

  public static RaiMsg
  NewRaiMsg(int MsgType, short RecType) throws RaiException
  {
    try {
      RaiMsg raiMsg = new RaiMsg( );
      raiMsg.SetProtocol(raiMsg.RAIMSG_PROTO);
      raiMsg.AppendUShort( "MSG_TYPE",(short) MsgType );
      raiMsg.AppendUShort( "REC_TYPE",  RecType );
      raiMsg.AppendUShort( "SEQ_NO", (short) 0 ); //set this when we send
      raiMsg.AppendUShort( "REC_STATUS", (short) 0 );
      return raiMsg;
    }
    catch (Throwable e) {
      RaiException re = new RaiException(RaiException.BAD_RAIMSG,
        "An error has occured attempting to create and initialize a RaiMsg. "
        + e.toString(), e );
      throw (re);
    }
  }

  public static RaiMsg
  NewRaiMsg(int MsgType, String RecType) throws RaiException
  {
    try {
      RaiMsg raiMsg = new RaiMsg();
      raiMsg.SetProtocol(raiMsg.RAIMSG_PROTO);
      raiMsg.AppendUShort( "MSG_TYPE",(short) MsgType );
      raiMsg.SetRecTypeString(RecType);
      raiMsg.AppendUShort( "SEQ_NO", (short) 0 ); //set this when we send
      raiMsg.AppendUShort( "REC_STATUS", (short) 0 );
      return raiMsg;
    }
    catch (Throwable e) {
      RaiException re = new RaiException(RaiException.BAD_RAIMSG,
        "An error has occured attempting to create and initialize a RaiMsg. "
        + e.toString(), e );
      throw (re);
    }
  }

  public static RaiMsg
  NewRaiMsg(int MsgType, short RecType, short SeqNo, short RecStatus) throws RaiException
  {
    try {
      RaiMsg raiMsg = new RaiMsg();
      raiMsg.SetProtocol(raiMsg.RAIMSG_PROTO);
      raiMsg.AppendUShort( "MSG_TYPE",(short) MsgType );
      raiMsg.AppendUShort( "REC_TYPE",  RecType );
      raiMsg.AppendUShort( "SEQ_NO", SeqNo ); //set this when we send
      raiMsg.AppendUShort( "REC_STATUS", RecStatus);
      return raiMsg;
    }
    catch (Throwable e) {
      RaiException re = new RaiException(RaiException.BAD_RAIMSG,
        "An error has occured attempting to create and initialize a RaiMsg. "
        + e.toString(), e );
      throw (re);
    }
  }

  public static RaiMsg
  NewRaiMsg(int MsgType, String RecType, short SeqNo, short RecStatus) throws RaiException
  {
    try {
      RaiMsg raiMsg = new RaiMsg();
      raiMsg.SetProtocol(raiMsg.RAIMSG_PROTO);
      raiMsg.AppendUShort( "MSG_TYPE",(short) MsgType );
      raiMsg.SetRecTypeString(RecType);
      raiMsg.AppendUShort( "SEQ_NO", SeqNo ); //set this when we send
      raiMsg.AppendUShort( "REC_STATUS", RecStatus);
      return raiMsg;
    }
    catch (Throwable e) {
      RaiException re = new RaiException(RaiException.BAD_RAIMSG,
        "An error has occured attempting to create and initialize a RaiMsg. "
        + e.toString(), e );
      throw (re);
    }
  }
}
