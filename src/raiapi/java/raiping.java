/* 
 * raiping.java
 */

import java.util.*;
import com.rai.raimsg.*;
import com.rai.raiapi.*;

public class raiping implements RaiCallback, RaiTimerCallback
{
  static String service = null;
  static String network = null;
  static String daemon  = null;
  static String subject = "PING.RAI.TEST.NaE";
  static int     counter;
  static boolean doCount;

  RaiSession   session;
  RaiSubscribe sub;
  RaiPublish   pub;
  RaiTimer     timer;
  int          msgSent;

  public raiping() 
  {
  }

  public static void main(String[] args) 
  {
    try {
      if (ReadArgs(args))
      {
        raiping raiPing = new raiping();
        raiPing.init();
        RaiApi.RaiMainloop( raiPing.session );
        raiPing.Close();
      } 
      else
        PrintArgs();
    } catch ( RaiException e ) {
      System.out.println( "main() failed " + e.toString() );
    }
  }

  static boolean ReadArgs(String[] args)
  {
    boolean returnValue =true;
    for (int i=0; i < args.length; i++) 
    {
      if (args[i].startsWith("-")) 
      {           // flag expected
        if (args[i].startsWith("-su")) subject = args[i+1];
      // else if (args[i].startsWith("-f")) formName = args[i+1];
      //  else if (args[i].startsWith("-d")) data = args[i+1];
        else if (args[i].startsWith("-n")) network = args[i+1];
        else if (args[i].startsWith("-ser")) service = args[i+1];
        else if (args[i].startsWith("-da")) daemon = args[i+1];
        /*else if (args[i].startsWith("-di"))
          dictionarySubject = args[i+1];*/
        else if (args[i].startsWith("-c")) 
        {
          try {
            counter = Integer.valueOf( args[i+1] ); 
            doCount = true;
          } catch (Exception Ex) {
            System.out.println("Ignoring invalid counter " + args[i+1]);
          }
        }
        /*else if (args[i].startsWith("-per")) 
        {
          try {
            perSec = Integer.valueOf( args[i+1] );
          } catch (Exception Ex) {
            System.out.println("Ignoring invalid perSec " + args[i+1]);
          }
        }*/
        else if (args[i].startsWith("-h")) { returnValue = false;}
        else System.out.println("Flag '{"+args[i]+"}' ignored");
      } 
    } // i; getting arguments
    return returnValue;
  }

  static void PrintArgs()
  {
    System.out.println("Usage:");
    System.out.println("  -subject            <subject>"); 
    //System.out.println("  -formname           <formname>"); 
    //System.out.println("  -data               <data>");  
    System.out.println("  -network            <network>"); 
    System.out.println("  -service            <service>"); 
    System.out.println("  -daemon             <daemon>"); 
    System.out.println("  -counter            <counter>"); 
    //System.out.println("  -perSec             <messages/sec>");
    //System.out.println("  -dictionarySubject  <counter>"); 
    System.out.println("  -help"); 

  }

  public void DoPing() 
  {
    try {
      RaiMsg theMsg = new RaiMsg();
      theMsg.SetProtocol(RaiMsg.RV_PROTO);  
      long time = System.nanoTime();
      theMsg.Append("time",time);
      theMsg.Append("seqno",this.msgSent);
      this.pub.Publish( subject, theMsg );
      this.msgSent++;
      theMsg.Release();
    } catch ( RaiException Ex ) {
      System.out.println( "DoPing() failed " + Ex.toString() );
    } catch ( RaiMsgException Mex ) {
      System.out.println( "DoPing() failed " + Mex.toString() );
    }
  }


  public void
  onMsg( RaiEvent event, RaiMsg theMsg, Object closure )
  {
    try {
      /*System.out.println( "sub " + event.subscribedSubject + " recv " +
                          event.receivedSubject );
      theMsg.Print( System.out );*/
      long time = System.nanoTime();
      //QueryPerformanceCounter( out time);
      long sendTime = theMsg.GetLong("time");
      long seqNo = theMsg.GetLong("seqno");
      double d =  (double) ( time - sendTime ) / 1000000.0;
      System.out.println( subject + " cnt=" + seqNo + " latency=" + d + "ms" );
      //theMsg.Print( System.out );
    } catch ( RaiMsgException Mex ) {
      System.out.println( "onMsg() failed " + Mex.toString() );
    }
  }


  public void
  onTimer(RaiSession session, Object closure)
  {
    DoPing();
    if ( doCount && --counter <= 0 )
      Close();
  }

  public void init()
  {
    try {  
      RaiApi.SetLogLevel(2);
      RaiApi.RaiOpen( RaiApi.RV7, RaiApi.SASS2 );
      this.session = new RaiSession( service, network, daemon );
      this.sub     = new RaiSubscribe( this.session, subject, this, null,
                                       RaiApi.UPDATE );
      this.pub     = new RaiPublish( this.session, subject, true );
      this.timer   = new RaiTimer( this.session, this, 500, null );
    } catch ( RaiException Ex ) {
      System.out.println( "init() failed " + Ex.toString() );
    }
  }

  public void Close() 
  { 
    try {
      RaiApi.RaiClose();
    } catch ( RaiException Ex ) {
      System.out.println( "Close() failed " + Ex.toString() );
    }
  }

}
