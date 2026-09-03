// raisub.java

import java.util.*;
import com.rai.raimsg.*;
import com.rai.raiapi.*;

public class raisub implements RaiCallback, RaiTimerCallback {

  static String service = null;
  static String network = null;
  static String daemon  = null;
  static String subject = "TEST.AAAA";
  static String dictSubject = "_TIC.REPLY.SASS.DATA.DICTIONARY";
  static boolean autoDispatch = false;
  static boolean printRate = false;
  static int logLevel = 2;  // 1-Major, 2-Minor, 3-Debug, 4-Trace, 5-All
  RaiSession session = null;
  RaiSession session2 = null;
  RaiSubscribe sub = null;
  RaiDict dataDict = null;
  RaiTimer timer = null;
  long msgCount = 0, lastCount = 0;

  public raisub(String[] args) {
    if(readArgs(args)) {
      init();
      mainLoop();
      close();
      System.exit(0);
    }
    else {
      printArgs();
      System.exit(1);
    }
  }

  public void
  onMsg( RaiEvent event, RaiMsg theMessage, Object closure ) {
    try {
      this.msgCount++;
      if ( ! printRate ) {
        System.out.println("received   subject: "+event.receivedSubject);
        System.out.println("subscribed subject: "+event.subscribedSubject); // note that for inbox messages this will return the actual subject and not the inbox
        theMessage.Print(System.out);
        System.out.flush();
      }
    } catch (RaiMsgException e ) {
      System.out.println(" caught a RaiMsg exception");
      System.out.println(e.toString());
    }
  }

  public void
  onTimer( RaiSession session, Object closure ) {
    long val = this.msgCount - this.lastCount;
    this.lastCount = this.msgCount;

    if ( val > 0 )
      System.out.println( ((Long) val).toString() + " msgs/sec" );
  }

  public void subscribe(String subject) {
    Object closure = null;

    try {
      this.sub = new RaiSubscribe( session, subject, this, closure);
    } catch ( RaiException e ) {
      System.out.println(" caught a RaiApi exception in subscribe");
      System.out.println(e.toString());
    }
  }

  public void init() {
    try {
      RaiApi.SetLogLevel(logLevel);//anything greater than 0
      if (autoDispatch) // rv
        System.out.println( "with RVDispatcher" );
      RaiApi.RaiOpen( RaiApi.RV7, RaiApi.SASS2 );
      this.session = new RaiSession( service, network, daemon, autoDispatch ); 
      this.dataDict = new RaiDict();
      // dictionary only needs to be loaded ONCE no matter how many sessions or
      // subscriptions
      this.dataDict.Load( session, dictSubject );
      if ( printRate ) {
        System.out.println( "Starting rate timer" );
        this.timer = new RaiTimer( this.session, this, 1000, null );
      }
      System.out.println( "Subscribing: " + subject );
      this.subscribe( subject );

    } catch ( RaiException e ) {
      System.out.println("Caught a RaiException in init");
      System.out.println(e.toString());
    }
  }

  public synchronized void mainLoop() {
    try {
        if (autoDispatch) {
          wait();// just receive messages automatically by dispatcher
        }
        else {
         for( ;; ) {
           RaiApi.RaiTimedDispatch( session, 1 ); //if not auto dispatch
        //System.out.print("#");
        }
      }
    }
    catch ( InterruptedException ie ) {
      System.out.println("Caught a InterruptedException in mainLoop");
      System.out.println(ie.toString());
    }
    catch ( RaiException e ) {
      System.out.println("Caught an Exception in mainLoop");
      System.out.println(e.toString());
    }
  }

  public void close() {
    try {
      RaiApi.RaiClose();
    } catch (RaiException e ) {
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
        else if (args[i].toLowerCase().startsWith("-n")) network = args[i+1];
        else if (args[i].toLowerCase().startsWith("-service")) service = args[i+1];
        else if (args[i].toLowerCase().startsWith("-daem")) daemon = args[i+1];
        else if (args[i].toLowerCase().startsWith("-di")) dictSubject = args[i+1];
        else if (args[i].toLowerCase().startsWith("-h")) { returnValue = false;}
        else if (args[i].toLowerCase().startsWith("-auto")) autoDispatch = true;
        else if (args[i].toLowerCase().startsWith("-rate")) printRate = true;
        else if (args[i].toLowerCase().startsWith("-log")) {
          try {
            logLevel = Integer.parseInt( args[i+1]);
          }
          catch (NumberFormatException n ) {
            System.out.println("Ignoring invalid logLevel " + args[i+1]);
          }
        }
        else System.out.println("Flag '{"+args[i]+"}' ignored");
      }
    }
  return returnValue;
  }

  static void printArgs()
  {
    System.out.println("Usage:");
    System.out.println("  -subject            <subject>");
    System.out.println("  -network            <network>");
    System.out.println("  -service            <service>");
    System.out.println("  -daemon             <daemon>");
    System.out.println("  -dictionarySubject  <counter>");
    System.out.println("  -auto               //create RV Dispatcher");
    System.out.println("  -logLevel           <logging 0-low 5-high>");
    System.out.println("  -help");
  }


  public static void main(String[] args) {
    new raisub(args);
  }
}
