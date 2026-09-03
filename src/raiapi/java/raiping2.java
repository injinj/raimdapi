/* Copyright (c) 2011 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

import java.io.*;
import java.util.Hashtable;
import java.text.DecimalFormat;
import com.rai.raimsg.*;
import com.rai.raiapi2.*;
import com.rai.raiexception.RaiException;

class raiping2Args {
  IntArg    perSec_arg;
  IntArg    msgCount_arg;
  StringArg prefix_arg;
  StringArg subject_arg;
  BoolArg   direct_arg;
  BoolArg   noSub_arg;
  BoolArg   noPub_arg;

  raiping2Args() {
    perSec_arg   = new IntArg( "perSec", 10, "<num>",
        "Number of msgs per sec" );
    msgCount_arg = new IntArg( "msgCount", 0, "<num>",
        "Number of msgs to publish, 0 for infinite" );
    prefix_arg = new StringArg( "prefix", null, "<subject>",
        "Subject to prefix publish subject with, usually set to " +
        "_TIC. if using SASS/RV" );
    subject_arg = new StringArg( "subject", "PING.TEST.REC.XXX", "<subject>", 
        "Subject to ping" );
    direct_arg  = new BoolArg( "direct", false, "<bool>", 
        "Whether to dispatch messages directly from the recv " +
        "threads (true) or serialized on the queue thread " +
        "(false)" );
    noSub_arg   = new BoolArg( "noSub", false, "<bool>", 
        "Don't subscribe, only publish pings" );
    noPub_arg   = new BoolArg( "noPub", false, "<bool>", 
        "Don't publish, only subscribe pings" );
  }

  void getArgs( Args args ) throws RaiException {
    args.add( perSec_arg );
    args.add( msgCount_arg );
    args.add( prefix_arg );
    args.add( subject_arg );
    args.add( direct_arg );
    args.add( noSub_arg );
    args.add( noPub_arg );
  }
}


public class raiping2
  implements RaiTimerCallback, RaiMsgCallback, RaiDataLossCallback, RaiService {
  RaiApi       api;
  RaiSession   session;
  RaiQueue     subQueue,
               pubQueue;
  RaiSubscribe subscriber;
  RaiPublish   publisher;
  RaiTimer     publishTimer,
               printTimer;
  String       msgTypeField,
               recTypeField,
               seqNoField,
               recStatusField,
               timeField;
  String       subject,
               prefix,
               publishSubject;
  int          msgsPerSec;
  long         msgsPublished,
               msgsClocked,
               msgCount,
               msgSent,
               msgRecvd,
               lastMsgRecvd;
  long         intervalStart,
               startTime;
  double       latencySum,
               latencyMin,
               latencyMax,
               cumLatencySum,
               cumLatencyMin,
               cumLatencyMax;
  short        recType;
  boolean      direct,
               quit,
               noSub,
               noPub;
  int          sigCaught;
  static final long MAX_LAT = 1000000; /* 100ms */
  static final long MIN_INIT_LAT = 9999999;
  long      [] msgsLat; /* 1us . 100ms */
  final DecimalFormat zeroF,
                      oneF,
                      twoF,
                      threeF,
                      sixF;

  public raiping2() {
    this.msgsLat       = new long[ (int) MAX_LAT ];
    this.latencyMin    = MIN_INIT_LAT;
    this.cumLatencyMin = MIN_INIT_LAT;
    this.msgsPerSec    = 10;
    this.zeroF  = new DecimalFormat( "########0" );
    this.oneF   = new DecimalFormat( "########0.0" );
    this.twoF   = new DecimalFormat( "########0.00" );
    this.threeF = new DecimalFormat( "########0.000" );
    this.sixF   = new DecimalFormat( "########0.000000" );
  }

  public void close() throws RaiException {
    this.finalLat();
    this.quit = true;
    if ( this.pubThread != null ) {
      while ( this.pubThread.isAlive() ) {
        try {
          this.pubThread.join();
        } catch ( InterruptedException i ) {
        }
      }
    }
    if ( this.publishTimer != null )
      this.publishTimer.Stop();
    if ( this.printTimer != null )
      this.printTimer.Stop();
    if ( this.publisher != null )
      this.publisher.Destroy();
    if ( this.subscriber != null )
      this.subscriber.Cancel();
    if ( this.subQueue != null )
      this.subQueue.Destroy();
    if ( this.pubQueue != null )
      this.pubQueue.Destroy();
    if ( this.session != null )
      this.session.Destroy();
    if ( this.api != null )
      this.api.Close();
  }

  public boolean init( RaiApi api,  Args args ) {
    this.api = api;
    try {
      raiping2Args pargs = new raiping2Args();
      int timeout;

      RaiApi.OpenLog( args );
      this.api.ParseArgs( args );

      this.subject    = args.getString( pargs.subject_arg.name );
      this.prefix     = args.getString( pargs.prefix_arg.name );
      this.msgCount   = args.getInt( pargs.msgCount_arg.name );
      this.msgsPerSec = args.getInt( pargs.perSec_arg.name );
      this.direct     = args.getBoolean( pargs.direct_arg.name );
      this.noSub      = args.getBoolean( pargs.noSub_arg.name );
      this.noPub      = args.getBoolean( pargs.noPub_arg.name );

      if ( this.prefix == null )
        this.publishSubject = this.subject;
      else
        this.publishSubject = this.prefix + this.subject;
      if ( ! this.noPub && ! this.noSub )
        this.api.PrintLog( RaiApi.LVL_MINOR, "Publishing " +
                           this.publishSubject + " subscribe " + this.subject );
      else if ( this.noPub )
        this.api.PrintLog( RaiApi.LVL_MINOR, "Subscribe " + this.subject );
      else if ( this.noSub )
        this.api.PrintLog( RaiApi.LVL_MINOR, "Publishing " +
                           this.publishSubject );
      this.session    = this.api.CreateSession();
      this.session.SetDataLossCB( this );
      this.session.Start();
      this.subQueue   = this.session.CreateQueue( this.direct );
      this.pubQueue   = this.session.CreateQueue( this.direct );

      /*if ( DataDictionary != null ) {
        const RaiMsg_form *form;
        if ( (form = DataDictionary.getForm( "RAIPING" )) != null ) {
          this.recType = form.entry.fid;
          if ( (this.msgTypeField   = form.msgType) != null &&
               (this.recTypeField   = form.recType) != null &&
               (this.seqNoField     = form.seqNo) != null &&
               (this.recStatusField = form.recStatus) != null ) {
             this.timeField = form.getEntry( "time" );
             if ( this.timeField == null )
               this.timeField = form.getEntry( "TIME" );
             if ( this.timeField != null )
               this.api.PrintLog( RaiApi.LVL_MINOR, "Using TIB_SASS form RAIPING" );
          }
        }
      }*/
      if ( ! this.noSub ) {
        this.subscriber = this.subQueue.CreateSubscribe( this );
        this.subscriber.Start( this.subject, RaiSubscribe.UPDATE |
                                             RaiSubscribe.NO_COPY );
      }
      if ( ! this.noPub ) {
        this.publisher = this.session.CreatePublish();
        /* timer is 10 times faster than rate since pub rate is controlled by
         * updateClock(), not by this timer */
        if ( this.msgsPerSec >= 100 )
          timeout = 1;
        else
          timeout = 100 / this.msgsPerSec;

        this.publishTimer = this.pubQueue.CreateTimer( this );
        this.publishTimer.SetInterval( timeout );
        this.publishTimer.Start();
      }
      /* this.noPub == true */
      else {
        /* since updateClock() is never called, and onMsg() tests if messages
         * are published before calculating latency */
        this.startTime = 1;
      }
      if ( this.msgsPerSec > 15 || this.noPub ) {
        this.printTimer = this.pubQueue.CreateTimer( this );
        this.printTimer.SetInterval( 500 ); /* ms */
        this.intervalStart = Time.hiresTimeNanosecs();
        this.printTimer.Start();
      }
      return true;
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Not initialized, stopped" );
      return false;
    }
  }

  /* RaiDataLossCallback */
  public void onConnection( RaiConnectionEvent event,  Object cl ) {
    this.api.PrintLog( RaiApi.LVL_MINOR, "onConnection: " + event.description );
  }

  public void onDataLoss( RaiDataLossEvent event,  Object cl ) {
    this.api.PrintLog( RaiApi.LVL_ERROR, "onDataLoss: " + event.description );
  }

  long updateClock() {
    long currentTime;

    currentTime = Time.hiresTimeNanosecs();
    if ( this.startTime == 0 ) {
      this.startTime   = currentTime;
      this.msgsClocked = 0;
    }
    else if ( currentTime > this.startTime ) {
      this.msgsClocked = (long)
        ( (double) ( currentTime - this.startTime ) / 1000000000.0 *
          (double) this.msgsPerSec );
    }
    return currentTime;
  }

  /* RaiMsgCallback */
  public void onMsg( RaiMsgEvent event,  RaiMsg raiMsg,  Object closure ) {
    double latencyMS;
    long   sendTime,
           curTime;

    try {
      sendTime = raiMsg.GetLong( "time" );
    } catch ( RaiMsgException e ) {
      if ( e.getErrno() == RaiMsg.NOT_FOUND )
        return;
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Getting time field" );
      return;
    }
    if ( sendTime < this.startTime || this.startTime == 0 ) /* old ping value */
      return;
    curTime   = Time.hiresTimeNanosecs();
    latencyMS = (double) ( curTime - sendTime ) / (double) 1000000.0;
    this.latencySum += latencyMS;
    this.cumLatencySum += latencyMS;
    int usecsIndex = (int) ( latencyMS * 1000.0 );
    if ( usecsIndex < 0 )
      usecsIndex = 0;
    else if ( usecsIndex >= MAX_LAT )
      usecsIndex = (int) ( MAX_LAT - 1 );
    this.msgsLat[ usecsIndex ]++;

    if ( latencyMS > this.latencyMax ) {
      this.latencyMax = latencyMS;
      if ( latencyMS > this.cumLatencyMax )
        this.cumLatencyMax = latencyMS;
    }
    if ( latencyMS < this.latencyMin ) {
      this.latencyMin = latencyMS;
      if ( latencyMS < this.cumLatencyMin )
        this.cumLatencyMin = latencyMS;
    }
    this.msgRecvd++;

    if ( this.printTimer == null ) {
      System.out.println( event.subject + " cnt=" + this.msgRecvd +
                          " latency=" + threeF.format( latencyMS ) );
    }
    if ( this.msgRecvd == this.msgCount )
      this.quit = true;
  }

  /* RaiTimerCallback */
  public void onTimer( RaiTimer timer, Object closure ) {
    if ( timer == this.publishTimer )
      this.publish();
    else if ( timer == this.printTimer )
      this.print();
  }

  void publish() {
    long curTime;
    RaiMsg msg = null;

    try {
      curTime = this.updateClock();
      for ( ; this.msgsPublished < this.msgsClocked;
            this.msgsPublished++ ) {
        if ( msg == null ) {
          msg = new RaiMsg();
          if ( this.timeField != null ) {
            msg.AppendUShort( this.msgTypeField, (short) SassConst.UPDATE );
            msg.AppendUShort( this.recTypeField, (short) this.recType );
            msg.AppendUShort( this.seqNoField, (short) this.msgSent );
            msg.AppendUShort( this.recStatusField, (short) 0 );
            msg.AppendULong( this.timeField, curTime );
          }
          else {
            msg.AppendUShort( "MSG_TYPE", (short) SassConst.UPDATE );
            msg.AppendUShort( "SEQ_NO", (short) this.msgSent );
            msg.AppendUShort( "REC_STATUS", (short) 0 );
            msg.AppendULong( "time", curTime );
          }
        }
        else {
          if ( this.timeField != null ) {
            msg.UpdateUShort( this.seqNoField, (short) this.msgSent );
            msg.UpdateULong( this.timeField, curTime );
          }
          else {
            msg.UpdateUShort( "SEQ_NO", (short) this.msgSent );
            msg.UpdateULong( "time", curTime );
          }
        }

        this.publisher.Publish( this.publishSubject, msg );
        if ( ++this.msgSent == this.msgCount ) {
          this.publishTimer.Stop();
          return;
        }
        curTime = Time.hiresTimeNanosecs();
      }
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "publish" );
    }
  }

  void print() {
    long   curTime;
    double ms,
           interval,
           rate,
           min,
           max;

    curTime  = Time.hiresTimeNanosecs();
    interval = (double) ( curTime - this.intervalStart ) /
               (double) 1000000000.0;
    if ( interval >= 0.100 ) {
      if ( this.msgRecvd > this.lastMsgRecvd ) {
        this.intervalStart = curTime; 

        rate = (double) ( this.msgRecvd - this.lastMsgRecvd );
        this.lastMsgRecvd = this.msgRecvd;
        ms    = this.latencySum / rate;
        rate /= interval;
        min   = this.latencyMin;
        max   = this.latencyMax;
        this.latencySum = 0;
        this.latencyMin = MIN_INIT_LAT;
        this.latencyMax = 0;

        if ( rate < 30.0 ) {
          if ( rate <= 1.1 ) {
            System.out.println( this.subject + ": " +
                                threeF.format( ms ) + " ms" );
          }
          else {
            System.out.println( this.subject + ": rate=" +
                          oneF.format( rate ) + "/s av=" + threeF.format( ms ) +
                          " min=" + threeF.format( min ) +
                          " max=" + threeF.format( max ) + "ms" );
          }
        }
        else {
          String suffix = "";
          if ( rate >= 950.0 ) {
            rate /= 1000.0;
            suffix = "k";
          }
          DecimalFormat digits =
            ( rate >= 10000.0 ? zeroF : ( rate >= 1000.0 ? oneF : twoF ) );
          System.out.println( this.subject + ": r=" + digits.format( rate ) +
                              suffix + "/s av=" + threeF.format( ms ) +
                              " min=" + threeF.format( min ) +
                              " max=" + threeF.format( max ) + "ms" );
        }
      }
    }
  }

  void finalLat() {
    double       av;
    int          avg, j;
    int       [] stdDev = new int[ 4 ];
    long         cnt;
    long      [] stdDevMsgs = new long[ 4 ];
    if ( ! this.noSub ) {
      if ( this.msgRecvd == 0 )
        av = 0;
      else
        av = this.cumLatencySum / (double) this.msgRecvd;
      avg = (int) ( av * 1000.0 );
      if ( avg >= MAX_LAT )
        avg = (int) ( MAX_LAT - 1 );
      cnt             = this.msgsLat[ avg ];
      stdDev[ 0 ]     = 1;
      stdDevMsgs[ 0 ] = 0;
      stdDevMsgs[ 1 ] = (long) ( this.msgRecvd * 0.682 );
      stdDevMsgs[ 2 ] = (long) ( this.msgRecvd * 0.955 );
      stdDevMsgs[ 3 ] = (long) ( this.msgRecvd * 0.997 );

      for ( j = 1; j < 4; j++ ) {
        for ( stdDev[ j ] = stdDev[ j - 1 ];
              stdDev[ j ] < MAX_LAT && cnt < stdDevMsgs[ j ]; stdDev[ j ]++ ) {
          if ( stdDev[ j ] <= avg )
            cnt += this.msgsLat[ avg - stdDev[ j ] ];
          if ( avg + stdDev[ j ] < MAX_LAT )
            cnt += this.msgsLat[ avg + stdDev[ j ] ];
        }
      }
      System.out.println(
        "av=" + sixF.format( av ) +
        " min=" + sixF.format( (double) this.cumLatencyMin ) +
        " max=" + sixF.format( (double) this.cumLatencyMax ) +
        " stddev=(" + sixF.format( (double) stdDev[ 1 ] / 1000.0 ) +
              "," + sixF.format( (double) stdDev[ 2 ] / 1000.0 ) +
              "," + sixF.format( (double) stdDev[ 3 ] / 1000.0 ) +
              ") (68.2%%,95.5%%,99.7%%)" );
      if ( av > (double) MAX_LAT / 1000.0 ) {
        System.out.println( "stddev not accurate" );
      }
    }
  }

  /* use two threads, one for publishing / printing and one for subscribe */
  class PubThread extends Thread {
    raiping2 me;

    PubThread( raiping2 m ) {
      this.me = m;
    }
    void publoop() {
      while ( ! this.me.quit ) {
        try {
          this.me.pubQueue.TimedDispatch( 100 );
        } catch ( RaiException e ) {
          this.me.api.PrintLog( RaiApi.LVL_ERROR, e, "pubQueue dispatch" );
        }
      }
    }
    public void run() {
      this.publoop();
    }
  }
  PubThread pubThread;

  public void serviceRun() {
    this.dispatchLoop();
  }

  void dispatchLoop() {
    this.pubThread = new PubThread( this );
    this.pubThread.start();
    while ( ! this.quit ) {
      try {
        this.subQueue.TimedDispatch( 100 );
      } catch ( RaiException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "subQueue dispatch" );
      }
    }
  }

  static raiping2 ping;

  public static void sigHandler( int sig ) {
    /* be careful with locks in here, could deadlock if interrupt
     * happened while inside a critical section (for example, logging) */
    if ( ping != null ) {
      ping.sigCaught = sig;
      ping.quit = true;
    }
    else
      System.exit( 1 );
  }

  public static void main( String[] argv ) {
    RaiApi       api = null;
    Args         args;
    raiping2Args pargs;

    try {
      /* traps SIGINT, SIGHUP, SIGTERM and calls sigHandler(),
       * this may require LD_PRELOAD=libjsig.so and/or
       * java option -Xrs (rs = reduce signals) */
      RaiApi.RegisterSigHandler( raiping2.class.getName(), "sigHandler" );
      /* open log to stderr in case command line fails to parse, it may open
       * again if -log is specified on command line */
      RaiApi.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
      /* Open the api type from the command line, looks for -api <name> in
       * argv[] and loads that middleware.  Program could also pass "tibrv" or
       * some other api name in the first argument.  If neither are specfied
       * then the default api is loaded (capr) */
      api = RaiApi.RaiOpen( null, argv );
    } catch ( Exception e ) {
      System.err.println( "Unable to load Rai API: " + e.toString() );
      System.exit( 1 );
    }

    try {
      args  = new Args();
      pargs = new raiping2Args();

      /* get the api's configuration arguments */
      api.GetArgs( args );
      /* get the ping subject and rate */
      pargs.getArgs( args );
      /* get the logging, version, help, rc arguments and sets error output */
      args.addDefaults( api.RaiVersion(), "rai_", System.err, "raiping2" );

      try {
        /* match command line args, if -help or -version, returns false */
        if ( args.processArgs( argv ) ) {
          ping = new raiping2();
          /* initialize ping subscriptions, publisher, queue */
          if ( ping.init( api, args ) )
            /* run ping queue dispatch */
            ping.dispatchLoop();
          /* print ping latency summary and close api handles */
          ping.close();
        }
      } catch ( RaiException e ) {
        RaiApi.Log( RaiApi.LVL_ERROR, e, "Main" );
      }
    } catch ( RaiException e ) {
      RaiApi.Log( RaiApi.LVL_ERROR, e, "Unable to load Rai API" );
    }
  }
}
