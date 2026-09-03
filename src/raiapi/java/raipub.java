// raipub.java

import java.util.*;
import com.rai.raimsg.*;
import com.rai.raiapi.*;

public class raipub {

  static String subject       = "TEST.AAAA";
  static String formName      = "EQ";
  static String data          = "BID=12,ASK=14";
  static String dictSubject   = "_TIC.REPLY.SASS.DATA.DICTIONARY";
  static String msgType       = "INITIAL";
  static String protocol      = "RaiMsg";
  static String service       = null;
  static String network       = null;
  static String daemon        = null;
  static int counter          = 1;
  static int logLevel         = 2;  // 1-Major, 2-Minor, 3-Debug, 4-Trace, 5-All
  static boolean doCounter    = false;
  static boolean version      = false;
  static short formId         = 0;
  RaiSession session          = null;
  RaiPublish publisher        = null;
  RaiDict dataDict            = null;

  public raipub(String[] args) {

    if( readArgs(args)){
      init();
      mainLoop();
      close();
      System.exit(0);
    }
    else {
      if ( version == true ){
        System.out.println( RaiApi.RaiVersion() );
        System.out.println( RaiMsg.RaiMsgVersion() );
      }
      else {
        printArgs();
      }
      System.exit(1);
    }

  }

  public void pubMsg() {
    RaiMsg theMessage = null;
    String fname  = null;
    String fval   = null;
    int i         = 0;
    int theMsgType = RaiApi.INITIAL;

    /*
     * Check the session to see what sort of message we want to use.
     * If we are packing a SASS compatible QForm use the appropriate
     * message prototype
     */

    try {

      if ( msgType.compareTo( "VERIFY" ) == 0 )
        theMsgType = RaiApi.VERIFY;
      else if ( msgType.compareTo ( "UPDATE" ) == 0 )
        theMsgType = RaiApi.UPDATE;
      else
        theMsgType = RaiApi.INITIAL;

      if ( protocol.compareTo( "SASS" ) == 0 ) {
        System.out.println(" creating sass msg");
        if ( formId != 0 )
          theMessage = RaiApi.NewSASSMsg(theMsgType, formId );
        else
          theMessage = RaiApi.NewSASSMsg(theMsgType,formName);
      }
      else // msgtype==RaiMsg
        if ( formId != 0 )
          theMessage = RaiApi.NewRaiMsg(theMsgType,formId);
        else
          theMessage = RaiApi.NewRaiMsg(theMsgType,formName);


/*      if( publisher.GetUsesSass() )
        theMessage = new RaiMsg( RaiMsg.TIB_SASS_PROTO );
      else
        theMessage = new RaiMsg( RaiMsg.RAIMSG_PROTO );
*/

      /*
       * Take the data values provided on the command line and add them
       * to the message
       */

      String[] dataItems = data.split( "," );
      for( i = 0; i < dataItems.length; i++ ) {
        String[] item = dataItems[i].split( "=" );
        fname = item[0];
        fval  = item[1];

        /*
         * Add the field and data to the message. In this case
         * the message is not using a pre-defined record type so
         * any field can be specified.
         */

        if( logLevel >= RaiApi.Debug )
          System.out.println( "Setting field "+fname+" to "+fval );
        theMessage.Append( fname, fval );
      }
      if( logLevel >= RaiApi.Debug )
        theMessage.Print( System.out );

      publisher.Publish( subject, theMessage );
      theMessage.Release();

    } catch ( RaiMsgException e ) {
      System.out.println("caught a RaiMsg exception" );
      System.out.println(e.toString());
    } catch ( RaiException e ) {
      System.out.println("caught a RaiApi exception" );
      System.out.println(e.toString());
    }
  }

  public void init(){
     try {
      RaiApi.RaiOpen( RaiApi.RV7, RaiApi.SASS2 );
      RaiApi.SetLogLevel (logLevel);

      /*
       * When specifying the MSG_TYPE at creation you need to be careful if
       * you use UPDATE. Messages will be dropped by the Cache if there is not
       * already an existing record. It is safer to use INTIAL or VERIFY on
       * creation and use RaiPublish.Ioctl to change the MSG_TYPE after the
       * initial publish
       */

      session = new RaiSession( service, network, daemon );
      //publisher = new RaiPublish( session, formName, msgType );
      publisher = new RaiPublish( session );

      /*
       * Create a data dictionary object and Load it from the Cache
       */

      dataDict = new RaiDict();
      dataDict.Load( session, dictSubject );
    } catch ( RaiException e ) {
      System.out.println("Caught a RaiException");
      System.out.println(e.toString());
    }

  }

  public synchronized void mainLoop() {
    for( ;; ) {
      try {
        wait(1000);
        pubMsg();
        if(--counter <= 0 )
          break;
      } catch ( Exception e ) {
        System.out.println("Caught an exception in mainLoop");
        System.out.println(e.toString());
      }
    }
  }

  public void close() {

    try {
      RaiApi.RaiClose();
    } catch ( RaiException e ) {
      System.out.println("Caught a RaiException in close");
      System.out.println(e.toString());
    }
  }

  static boolean readArgs(String[] args)
  {
    boolean returnValue =true;
    for (int i=0; i < args.length; i++)
    {
      if (args[i].startsWith("-"))
      {           // flag expected
        if (args[i].toLowerCase().startsWith("-su")) subject = args[i+1];
        else if (args[i].toLowerCase().startsWith("-f")){
          formName = args[i+1];
          try {
            formId = (short) Integer.parseInt( args[i+1]);
          }
          catch (NumberFormatException n ) {
            System.out.println("cannot set formId, using formName " + args[i+1]);
          }
        }
        else if (args[i].toLowerCase().startsWith("-proto")) protocol = args[i+1];
        else if (args[i].toLowerCase().startsWith("-data")) data = args[i+1];
        else if (args[i].toLowerCase().startsWith("-m")) msgType = args[i+1];
        else if (args[i].toLowerCase().startsWith("-n")) network = args[i+1];
        else if (args[i].toLowerCase().startsWith("-service")) service = args[i+1];
        else if (args[i].toLowerCase().startsWith("-daem")) daemon = args[i+1];
        else if (args[i].toLowerCase().startsWith("-dict")) dictSubject = args[i+1];
        else if (args[i].toLowerCase().startsWith("-log")) {
          try {
            logLevel = Integer.parseInt( args[i+1]);
          }
          catch (NumberFormatException n ) {
            System.out.println("Ignoring invalid logLevel " + args[i+1]);
          }
        }
        else if (args[i].toLowerCase().startsWith("-c")) {
          try {
            counter = Integer.parseInt( args[i+1]);
            doCounter=true;
          }
          catch (NumberFormatException n ) {
            System.out.println("Ignoring invalid counter " + args[i+1]);
          }
        }
        else if (args[i].toLowerCase().startsWith("-h")) { returnValue = false;}
        else if (args[i].toLowerCase().startsWith("-v")) { version = true; returnValue = false;}
        else System.out.println("Flag '{"+args[i]+"}' ignored");
      }
    }
  return returnValue;
  }

  static void printArgs()
  {
    System.out.println("Usage:");
    System.out.println("  -subject            <subject>");
    System.out.println("  -formname           <formname>");
    System.out.println("  -protocol           <data representation SASS or RaiMsg>");
    System.out.println("  -data               <data>");
    System.out.println("  -msgType            <msgType>");
    System.out.println("  -network            <network>");
    System.out.println("  -service            <service>");
    System.out.println("  -daemon             <daemon>");
    System.out.println("  -counter            <counter>");
    System.out.println("  -logLevel           <logging 0-low 5-high>");
    System.out.println("  -dictionarySubject  <subject string>");
    System.out.println("  -version            <print API version>");
    System.out.println("  -help");

  }

  public static void main(String[] args) {

    new raipub(args);

  }

}
