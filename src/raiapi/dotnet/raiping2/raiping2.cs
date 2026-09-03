/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 * .NET port of src/raiapi/java/raiping2.java */
using System;
using System.Threading;
using Com.Rai.Raimsg;
using Com.Rai.Raiapi2;
using Com.Rai.Raiexception;

class raiping2Args {
  public IntArg    perSec_arg, msgCount_arg;
  public StringArg prefix_arg, subject_arg;
  public BoolArg   direct_arg, noSub_arg, noPub_arg;

  public raiping2Args() {
    perSec_arg   = new IntArg( "perSec", 10, "<num>", "Number of msgs per sec" );
    msgCount_arg = new IntArg( "msgCount", 0, "<num>", "Number of msgs to publish, 0 for infinite" );
    prefix_arg   = new StringArg( "prefix", null, "<subject>",
        "Subject to prefix publish subject with, usually set to _TIC. if using SASS/RV" );
    subject_arg  = new StringArg( "subject", "PING.TEST.REC.XXX", "<subject>", "Subject to ping" );
    direct_arg   = new BoolArg( "direct", false, "<bool>",
        "Whether to dispatch messages directly from the recv threads (true) or " +
        "serialized on the queue thread (false)" );
    noSub_arg    = new BoolArg( "noSub", false, "<bool>", "Don't subscribe, only publish pings" );
    noPub_arg    = new BoolArg( "noPub", false, "<bool>", "Don't publish, only subscribe pings" );
  }

  public void getArgs( Args args ) {
    args.add( perSec_arg );
    args.add( msgCount_arg );
    args.add( prefix_arg );
    args.add( subject_arg );
    args.add( direct_arg );
    args.add( noSub_arg );
    args.add( noPub_arg );
  }
}

public class raiping2 : RaiTimerCallback, RaiMsgCallback, RaiDataLossCallback {
  RaiApi       api;
  RaiSession   session;
  RaiQueue     subQueue, pubQueue;
  RaiSubscribe subscriber;
  RaiPublish   publisher;
  RaiTimer     publishTimer, printTimer;
  /* set when a RAIPING form is available in the dictionary (see the java
   * version); null means the plain MSG_TYPE/SEQ_NO/REC_STATUS/time header */
  string       msgTypeField = null, recTypeField = null, seqNoField = null,
               recStatusField = null, timeField = null;
  string       subject, prefix, publishSubject;
  int          msgsPerSec;
  long         msgsPublished, msgsClocked, msgCount, msgSent, msgRecvd, lastMsgRecvd;
  long         intervalStart, startTime;
  double       latencySum, latencyMin, latencyMax, cumLatencySum, cumLatencyMin, cumLatencyMax;
  short        recType = 0;
  bool         direct, noSub, noPub;
  volatile bool quit;
  int          sigCaught;
  const long   MAX_LAT = 1000000; /* 100ms */
  const long   MIN_INIT_LAT = 9999999;
  long[]       msgsLat; /* 1us .. 100ms */
  Thread       pubThread;
  RaiMsg       pingMsg;

  public raiping2() {
    this.msgsLat       = new long[ (int) MAX_LAT ];
    this.latencyMin    = MIN_INIT_LAT;
    this.cumLatencyMin = MIN_INIT_LAT;
    this.msgsPerSec    = 10;
  }

  public void close() {
    this.finalLat();
    this.quit = true;
    if ( this.pubThread != null ) this.pubThread.Join();
    if ( this.publishTimer != null ) this.publishTimer.Stop();
    if ( this.printTimer != null ) this.printTimer.Stop();
    if ( this.publisher != null ) this.publisher.Destroy();
    if ( this.subscriber != null ) this.subscriber.Cancel();
    if ( this.subQueue != null ) this.subQueue.Destroy();
    if ( this.pubQueue != null ) this.pubQueue.Destroy();
    if ( this.session != null ) this.session.Destroy();
    if ( this.api != null ) this.api.Close();
  }

  public bool init( RaiApi api,  Args args ) {
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

      this.publishSubject = this.prefix == null ? this.subject : this.prefix + this.subject;
      if ( ! this.noPub && ! this.noSub )
        this.api.PrintLog( RaiApi.LVL_MINOR, "Publishing " + this.publishSubject + " subscribe " + this.subject );
      else if ( this.noPub )
        this.api.PrintLog( RaiApi.LVL_MINOR, "Subscribe " + this.subject );
      else if ( this.noSub )
        this.api.PrintLog( RaiApi.LVL_MINOR, "Publishing " + this.publishSubject );
      this.session  = this.api.CreateSession();
      this.session.SetDataLossCB( this );
      this.session.Start();
      this.subQueue = this.session.CreateQueue( this.direct );
      this.pubQueue = this.session.CreateQueue( this.direct );

      if ( ! this.noSub ) {
        this.subscriber = this.subQueue.CreateSubscribe( this );
        this.subscriber.Start( this.subject, RaiSubscribe.UPDATE | RaiSubscribe.NO_COPY );
      }
      if ( ! this.noPub ) {
        this.publisher = this.session.CreatePublish();
        /* timer is 10 times faster than rate since pub rate is controlled by
         * updateClock(), not by this timer */
        timeout = this.msgsPerSec >= 100 ? 1 : 100 / this.msgsPerSec;
        this.publishTimer = this.pubQueue.CreateTimer( this );
        this.publishTimer.SetInterval( timeout );
        this.publishTimer.Start();
      }
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
  public void onConnection( RaiConnectionEvent ev,  object cl ) {
    this.api.PrintLog( RaiApi.LVL_MINOR, "onConnection: " + ev.description );
  }
  public void onDataLoss( RaiDataLossEvent ev,  object cl ) {
    this.api.PrintLog( RaiApi.LVL_ERROR, "onDataLoss: " + ev.description );
  }

  long updateClock() {
    long currentTime = Time.hiresTimeNanosecs();
    if ( this.startTime == 0 ) {
      this.startTime   = currentTime;
      this.msgsClocked = 0;
    }
    else if ( currentTime > this.startTime ) {
      this.msgsClocked = (long) ( (double) ( currentTime - this.startTime ) / 1000000000.0 * (double) this.msgsPerSec );
    }
    return currentTime;
  }

  /* RaiMsgCallback */
  public void onMsg( RaiMsgEvent ev,  RaiMsg raiMsg,  object closure ) {
    long sendTime;
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
    long   curTime   = Time.hiresTimeNanosecs();
    double latencyMS = (double) ( curTime - sendTime ) / 1000000.0;
    this.latencySum += latencyMS;
    this.cumLatencySum += latencyMS;
    int usecsIndex = (int) ( latencyMS * 1000.0 );
    if ( usecsIndex < 0 ) usecsIndex = 0;
    else if ( usecsIndex >= MAX_LAT ) usecsIndex = (int) ( MAX_LAT - 1 );
    this.msgsLat[ usecsIndex ]++;

    if ( latencyMS > this.latencyMax ) {
      this.latencyMax = latencyMS;
      if ( latencyMS > this.cumLatencyMax ) this.cumLatencyMax = latencyMS;
    }
    if ( latencyMS < this.latencyMin ) {
      this.latencyMin = latencyMS;
      if ( latencyMS < this.cumLatencyMin ) this.cumLatencyMin = latencyMS;
    }
    this.msgRecvd++;

    if ( this.printTimer == null )
      Console.WriteLine( ev.subject + " cnt=" + this.msgRecvd + " latency=" + latencyMS.ToString( "0.000" ) );
    if ( this.msgRecvd == this.msgCount )
      this.quit = true;
  }

  /* RaiTimerCallback */
  public void onTimer( RaiTimer timer,  object closure ) {
    if ( timer == this.publishTimer ) this.publish();
    else if ( timer == this.printTimer ) this.print();
  }

  void publish() {
    try {
      long curTime = this.updateClock();
      for ( ; this.msgsPublished < this.msgsClocked; this.msgsPublished++ ) {
        if ( this.pingMsg == null ) {
          this.pingMsg = new RaiMsg();
          if ( this.timeField != null ) {
            this.pingMsg.AppendUShort( this.msgTypeField, (short) SassConst.UPDATE );
            this.pingMsg.AppendUShort( this.recTypeField, this.recType );
            this.pingMsg.AppendUShort( this.seqNoField, (short) this.msgSent );
            this.pingMsg.AppendUShort( this.recStatusField, (short) 0 );
            this.pingMsg.AppendULong( this.timeField, curTime );
          }
          else {
            this.pingMsg.AppendUShort( "MSG_TYPE", (short) SassConst.UPDATE );
            this.pingMsg.AppendUShort( "SEQ_NO", (short) this.msgSent );
            this.pingMsg.AppendUShort( "REC_STATUS", (short) 0 );
            this.pingMsg.AppendULong( "time", curTime );
          }
        }
        else {
          if ( this.timeField != null ) {
            this.pingMsg.UpdateUShort( this.seqNoField, (short) this.msgSent );
            this.pingMsg.UpdateULong( this.timeField, curTime );
          }
          else {
            this.pingMsg.UpdateUShort( "SEQ_NO", (short) this.msgSent );
            this.pingMsg.UpdateULong( "time", curTime );
          }
        }
        this.publisher.Publish( this.publishSubject, this.pingMsg );
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
    long   curTime  = Time.hiresTimeNanosecs();
    double interval = (double) ( curTime - this.intervalStart ) / 1000000000.0;
    if ( interval >= 0.100 ) {
      if ( this.msgRecvd > this.lastMsgRecvd ) {
        this.intervalStart = curTime;
        double rate = (double) ( this.msgRecvd - this.lastMsgRecvd );
        this.lastMsgRecvd = this.msgRecvd;
        double ms = this.latencySum / rate;
        rate /= interval;
        double min = this.latencyMin, max = this.latencyMax;
        this.latencySum = 0;
        this.latencyMin = MIN_INIT_LAT;
        this.latencyMax = 0;

        if ( rate < 30.0 ) {
          if ( rate <= 1.1 )
            Console.WriteLine( this.subject + ": " + ms.ToString( "0.000" ) + " ms" );
          else
            Console.WriteLine( this.subject + ": rate=" + rate.ToString( "0.0" ) + "/s av=" + ms.ToString( "0.000" ) +
                               " min=" + min.ToString( "0.000" ) + " max=" + max.ToString( "0.000" ) + "ms" );
        }
        else {
          string suffix = "";
          if ( rate >= 950.0 ) { rate /= 1000.0; suffix = "k"; }
          string digits = rate >= 10000.0 ? "0" : ( rate >= 1000.0 ? "0.0" : "0.00" );
          Console.WriteLine( this.subject + ": r=" + rate.ToString( digits ) + suffix + "/s av=" + ms.ToString( "0.000" ) +
                             " min=" + min.ToString( "0.000" ) + " max=" + max.ToString( "0.000" ) + "ms" );
        }
      }
    }
  }

  void finalLat() {
    if ( this.noSub ) return;
    double av = this.msgRecvd == 0 ? 0 : this.cumLatencySum / (double) this.msgRecvd;
    int avg = (int) ( av * 1000.0 );
    if ( avg >= MAX_LAT ) avg = (int) ( MAX_LAT - 1 );
    int[]  stdDev = new int[ 4 ];
    long[] stdDevMsgs = new long[ 4 ];
    long cnt = this.msgsLat[ avg ];
    stdDev[ 0 ] = 1;
    stdDevMsgs[ 0 ] = 0;
    stdDevMsgs[ 1 ] = (long) ( this.msgRecvd * 0.682 );
    stdDevMsgs[ 2 ] = (long) ( this.msgRecvd * 0.955 );
    stdDevMsgs[ 3 ] = (long) ( this.msgRecvd * 0.997 );
    for ( int j = 1; j < 4; j++ ) {
      for ( stdDev[ j ] = stdDev[ j - 1 ]; stdDev[ j ] < MAX_LAT && cnt < stdDevMsgs[ j ]; stdDev[ j ]++ ) {
        if ( stdDev[ j ] <= avg ) cnt += this.msgsLat[ avg - stdDev[ j ] ];
        if ( avg + stdDev[ j ] < MAX_LAT ) cnt += this.msgsLat[ avg + stdDev[ j ] ];
      }
    }
    Console.WriteLine(
      "av=" + av.ToString( "0.000000" ) +
      " min=" + this.cumLatencyMin.ToString( "0.000000" ) +
      " max=" + this.cumLatencyMax.ToString( "0.000000" ) +
      " stddev=(" + ( (double) stdDev[ 1 ] / 1000.0 ).ToString( "0.000000" ) +
            "," + ( (double) stdDev[ 2 ] / 1000.0 ).ToString( "0.000000" ) +
            "," + ( (double) stdDev[ 3 ] / 1000.0 ).ToString( "0.000000" ) +
            ") (68.2%,95.5%,99.7%)" );
    if ( av > (double) MAX_LAT / 1000.0 )
      Console.WriteLine( "stddev not accurate" );
  }

  /* use two threads, one for publishing / printing and one for subscribe */
  void publoop() {
    while ( ! this.quit ) {
      try { this.pubQueue.TimedDispatch( 100 ); }
      catch ( RaiException e ) { this.api.PrintLog( RaiApi.LVL_ERROR, e, "pubQueue dispatch" ); }
    }
  }
  void dispatchLoop() {
    this.pubThread = new Thread( this.publoop );
    this.pubThread.Start();
    while ( ! this.quit ) {
      try { this.subQueue.TimedDispatch( 100 ); }
      catch ( RaiException e ) { this.api.PrintLog( RaiApi.LVL_ERROR, e, "subQueue dispatch" ); }
    }
  }

  static raiping2 ping;

  public static void sigHandler( int sig ) {
    if ( ping != null ) {
      ping.sigCaught = sig;
      ping.quit = true;
    }
    else
      Environment.Exit( 1 );
  }

  public static int Main( string[] argv ) {
    RaiApi       api = null;
    Args         args;
    raiping2Args pargs;

    try {
      RaiApi.RegisterSigHandler( sigHandler );
      RaiApi.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
      api = RaiApi.RaiOpen( null, argv );
    } catch ( Exception e ) {
      Console.Error.WriteLine( "Unable to load Rai API: " + e );
      return 1;
    }

    try {
      args  = new Args();
      pargs = new raiping2Args();
      /* get the api's configuration arguments */
      api.GetArgs( args );
      /* get the ping subject and rate */
      pargs.getArgs( args );
      /* get the logging, version, help, rc arguments and sets error output */
      args.addDefaults( RaiApi.RaiVersion(), "rai_", Console.Error, "raiping2" );

      try {
        /* match command line args, if -help or -version, returns false */
        if ( args.processArgs( argv ) ) {
          ping = new raiping2();
          /* initialize ping subscriptions, publisher, queue */
          if ( ping.init( api, args ) )
            ping.dispatchLoop(); /* run ping queue dispatch */
          /* print ping latency summary and close api handles */
          ping.close();
        }
      } catch ( RaiException e ) {
        RaiApi.Log( RaiApi.LVL_ERROR, e, "Main" );
      }
    } catch ( RaiException e ) {
      RaiApi.Log( RaiApi.LVL_ERROR, e, "Unable to load Rai API" );
    }
    return 0;
  }
}
