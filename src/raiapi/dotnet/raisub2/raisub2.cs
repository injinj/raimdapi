/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 * .NET port of src/raiapi/java/raisub2.java */
using System;
using System.IO;
using System.Collections.Generic;
using System.Threading;
using Com.Rai.Raimsg;
using Com.Rai.Raiapi2;
using Com.Rai.Raiexception;

class raisub2Args {
  public StringArg subject_arg, output_arg, input_arg, save_arg, rotateTime_arg;
  public DoubleArg timeout_arg, rotateIval_arg;
  public BoolArg   snap_arg, listn_arg, retry_arg, noDict_arg, direct_arg,
                   wait_arg, quiet_arg, rate_arg, sched_arg, latency_arg;
  public IntArg    msgCnt_arg;

  public raisub2Args() {
    subject_arg = new StringArg( "subject", "-", "<subject> ...",
      "Subject name(s) to subscribe, use '-' to read subscriptions from stdin" );
    output_arg = new StringArg( "output", null, "<file>",
      "Output file name, otherwise uses stdout" );
    input_arg = new StringArg( "input", null, "<file>",
      "Input file name, otherwise uses stdin" );
    save_arg = new StringArg( "save", null, "<file>",
      "Save messages to file in replay format" );
    rotateTime_arg = new StringArg( "rotateTime", null, "<date>",
      "Rotate save messages file at this time" );
    timeout_arg = new DoubleArg( "timeout", 6.0, "<time>",
      "Timeout subscription after this period if no data is received, or " +
      "zero for no timeout" );
    rotateIval_arg = new DoubleArg( "rotateIval", 0.0, "<time>",
      "Rotate save messages file at this interval" );
    snap_arg = new BoolArg( "snap", false, "<bool>",
      "Get snapshot of subject instead of subscribe" );
    listn_arg = new BoolArg( "listen", false, "<bool>",
      "Listen to subject instead of subscribe, no initial value requested" );
    retry_arg = new BoolArg( "retry", false, "<bool>",
      "Retry subscriptions after timeout period" );
    noDict_arg = new BoolArg( "noDict", false, "<bool>",
      "Don't try to load dictionary" );
    direct_arg = new BoolArg( "direct", false, "<bool>",
      "Whether to dispatch messages directly from the recv threads (true) or " +
      "serialized on the queue thread (false)" );
    wait_arg = new BoolArg( "wait", false, "<bool>",
      "Causes program to keep running after snapshots have completed, use " +
      "this if multiple snapshot replies are expected" );
    quiet_arg = new BoolArg( "quiet", false, "<bool>",
      "Don't print the messages" );
    rate_arg = new BoolArg( "rate", false, "<bool>",
      "Print the rate of messages received" );
    sched_arg = new BoolArg( "sched", false, "<bool>",
      "Read scheduled subscribes and unsubscribes from stdin, useful for " +
      "generating subscription load test" );
    latency_arg = new BoolArg( "latency", false, "<bool>",
      "Track latency of messages and report at program end " );
    msgCnt_arg = new IntArg( "msgCount", 0, "<num>", "Quit after receiving num messages" );
  }

  public void getArgs( Args args ) {
    args.add( subject_arg, Args.COMMAND_ARG | Args.RESOURCE_ARG | Args.LIST_ARG );
    args.add( snap_arg );
    args.add( listn_arg );
    args.add( timeout_arg, Args.COMMAND_ARG | Args.RESOURCE_ARG | Args.TIME_SEC_ARG );
    args.add( retry_arg );
    args.add( noDict_arg );
    args.add( direct_arg );
    args.add( wait_arg );
    args.add( quiet_arg );
    args.add( rate_arg );
    args.add( sched_arg );
    args.add( latency_arg );
    args.add( msgCnt_arg );
    args.add( output_arg );
    args.add( input_arg );
    args.add( save_arg );
    args.add( rotateTime_arg );
    args.add( rotateIval_arg, Args.COMMAND_ARG | Args.RESOURCE_ARG | Args.TIME_SEC_ARG );
  }
}

public class raisub2 : RaiMsgCallback, RaiDataLossCallback, RaiTimerCallback {
  RaiApi       api;        /* the api handle */
  RaiDict      dataDict;   /* dictionary loader */
  RaiSession   session;    /* a session */
  RaiQueue     queue;      /* a queue for message and timer events */
  RaiTimer     rateTimer,  /* a timer used for -rate calculations */
               rotateTimer;/* a timer used to rotate -save output */
  TextReader   inp;        /* subjects read from here */
  TextWriter   outp;       /* messages written here */
  Stream       saveOut;    /* messages written in replay format */
  string       outName, saveName, inName,
               subjname,   /* first subject arg */
               inSource;   /* source name of subjects */
  long[]       msgsLat;    /* map of latency vals < 1 sec */
  long         latencyOverrun, latencyCnt, cumLatencySum, cumLatencyMin, cumLatencyMax;
  int          timeout,    /* if > 0, then timeout subscription starts */
               msgWaitCount; /* if > 0, then wait for N messages */
  long         msgEventCount, msgByteCount, msgTimeoutCount, subCount, unsubCount;
  long         lastTime, baseTime;
  long         lastMsgCount, lastByteCount, lastSubCount, lastUnsubCount;
  TimeRotate   fileRotate;
  Dictionary<string, RaiSubscribe> subHT;
  volatile bool quit;
  bool         doRetry, doQuiet, doSnapshot, doListen, doWait, doSched, doLatency;
  int          sigCaught;
  Thread       dispatchThread;
  readonly object outLock = new object(), saveLock = new object(), latLock = new object();
  const long   MAX_LAT = 1000000; /* 100ms */
  const long   MIN_INIT_LAT = 9999999;

  public raisub2() {
    this.subHT    = new Dictionary<string, RaiSubscribe>();
    this.baseTime = Time.currentTimeNanosecs(); /* -save time offset */
  }

  /* RaiDataLossCallback */
  public void onConnection( RaiConnectionEvent ev,  object cl ) {
    this.api.PrintLog( RaiApi.LVL_MINOR, "onConnection: " + ev.description );
  }
  public void onDataLoss( RaiDataLossEvent ev,  object closure ) {
    this.api.PrintLog( RaiApi.LVL_ERROR, "onDataLoss: " + ev.description );
    if ( ev.connectionLoss && ev.connectionCount == 0 ) {
      try {
        this.session.NotifyStatus( SassConst.TRANSIENT, SassConst.STATUS_TPT_DISCONNECTED );
      } catch ( RaiException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "NotifyStatus" );
      }
    }
  }

  /* RaiMsgCallback */
  public void onMsg( RaiMsgEvent ev,  RaiMsg raiMsg,  object closure ) {
    long ns = ( ev.pubTime != 0 || ev.routeTime != 0 ) ? Time.currentTimeNanosecs() : 0;
    int nBytes = 0;
    try {
      /* track latencies and summarize at program exit */
      if ( ns != 0 && this.msgsLat != null ) {
        if ( ev.routeTime != 0 ) {
          long i = ( ns - ev.routeTime ) / 100;
          nBytes = raiMsg.PackSize();
          /* if -direct = true, then multiple threads could callback onMsg() */
          lock ( this.latLock ) {
            if ( i < MAX_LAT ) {
              this.msgsLat[ (int) i ]++;
              this.cumLatencySum += i;
              this.latencyCnt++;
              if ( i < this.cumLatencyMin ) this.cumLatencyMin = i;
              if ( i > this.cumLatencyMax ) this.cumLatencyMax = i;
            }
            else {
              this.latencyOverrun++; /* may be cached messages, not deltas */
            }
            this.msgEventCount++;
            this.msgByteCount += nBytes;
          }
        }
      }
      /* print the message */
      if ( ! this.doQuiet ) {
        string s = ev.SubscribedSubject();
        lock ( this.outLock ) {
          this.outp.Write( "## Subject " + s + " (old state=" +
                      RaiSubscribe.StateToString( ev.oldState ) + ",new=" +
                      RaiSubscribe.StateToString( ev.state ) + ")" );
          /* if ev.subject is inbox name, or subscribed subject is wildcard
           * print the ev.subject */
          if ( s != ev.subject )
            this.outp.Write( " (" + ev.subject + ")" );
          if ( ev.counter != 0 )
            this.outp.Write( " c=" + ev.counter ); /* msg update count */
          this.outp.WriteLine();
          /* if message is timestamped, print times and latencies */
          if ( ns != 0 ) {
            this.outp.WriteLine( "# Receive " + Time.nsTimestamp( ns, 7 ) );
            if ( ev.pubTime != 0 )
              this.outp.WriteLine( "# Publish " + Time.nsTimestamp( ev.pubTime, 7 ) +
                " (lat=" + ( (double) ( ns - ev.pubTime ) / 1000000.0 ).ToString( "0.00000" ) + "ms)" );
            if ( ev.routeTime != 0 )
              this.outp.WriteLine( "# Route   " + Time.nsTimestamp( ev.routeTime, 7 ) +
                " (lat=" + ( (double) ( ns - ev.routeTime ) / 1000000.0 ).ToString( "0.00000" ) + "ms)" );
          }
          raiMsg.Print( this.outp );
          this.outp.Flush();
        }
      }
      if ( this.saveOut != null ) {
        /* if message is not internally generated, save it to replay file */
        if ( ev.recStatus != SassConst.STATUS_TIMEOUT && ev.msgType != SassConst.SERVICE_STATUS ) {
          lock ( this.saveLock ) {
            try {
              int size = raiMsg.PackSize();
              string s = ev.subject; /* use ev.subject unless inbox */
              if ( s.StartsWith( "_INBOX" ) )
                s = ev.SubscribedSubject();
              if ( ns == 0 )
                ns = Time.currentTimeNanosecs();
              if ( this.baseTime > ns )
                this.baseTime = ns;
              byte[] hdr = System.Text.Encoding.UTF8.GetBytes( s + "\n" + size + " " +
                 ( (double) ( ns - this.baseTime ) / 1000000000.0 ).ToString( "0.000000" ) + "\n" );
              this.saveOut.Write( hdr, 0, hdr.Length );
              this.saveOut.Write( raiMsg.Packed(), 0, size );
              this.saveOut.Flush();
            } catch ( Exception e ) {
              this.api.PrintLog( RaiApi.LVL_ERROR, e, "Saving message to \"" + this.saveName + "\"" );
            }
          }
        }
      }
      if ( ev.state == RaiSubscribe.STATE_STALE || ev.recStatus == SassConst.STATUS_TIMEOUT ) {
        if ( this.doRetry ) {
          this.api.PrintLog( RaiApi.LVL_MINOR, "Refreshing subject, " +
             ( ev.state == RaiSubscribe.STATE_STALE ? "stale" : "timeout" ) +
             ": \"" + ev.SubscribedSubject() + "\"" );
          ev.subscribe.Refresh( this.timeout );
          return; /* don't increment msgEventCount */
        }
        lock ( this ) { this.msgTimeoutCount++; }
        this.api.PrintLog( RaiApi.LVL_NORMAL, "Subject " +
           ( ev.state == RaiSubscribe.STATE_STALE ? "stale" : "timeout" ) +
           ": \"" + ev.SubscribedSubject() + "\"" );
        return;
      }
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Printing message" );
      this.outp.Flush();
    }
    /* if tracking latency, nBytes != 0 and these are already computed */
    if ( nBytes == 0 ) {
      try {
        nBytes = raiMsg.PackSize();
        lock ( this ) {
          this.msgEventCount++;
          this.msgByteCount += nBytes;
        }
      } catch ( RaiMsgException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "Pack size" );
      }
    }
  }

  /* RaiTimerCallback */
  public void onTimer( RaiTimer timer,  object closure ) {
    if ( timer == this.rateTimer ) {
      long curTime = Time.currentTimeMillis();
      double interval = (double) ( curTime - this.lastTime ) / 1000.0;
      this.lastTime = curTime;
      long count = this.msgEventCount, msgs = count - this.lastMsgCount;
      this.lastMsgCount = count;
      count = this.msgByteCount; long bytes = count - this.lastByteCount;
      this.lastByteCount = count;
      long cnt = this.subCount, subs = cnt - this.lastSubCount;
      this.lastSubCount = cnt;
      cnt = this.unsubCount; long unsubs = cnt - this.lastUnsubCount;
      this.lastUnsubCount = cnt;

      lock ( this.outLock ) {
        this.outp.WriteLine(
              ( (double) subs / interval ).ToString( "0.0" ) + " sub/s " +
              ( (double) unsubs / interval ).ToString( "0.0" ) + " unsub/s " +
              ( (double) msgs / interval ).ToString( "0.0" ) + " msg/s " +
              ( (double) bytes / 1024.0 / interval ).ToString( "0.0" ) + " kb/s " +
              ( (double) bytes * 8.0 / 1000.0 / 1000.0 / interval ).ToString( "0.00" ) + " mbit/s" );
        this.outp.Flush();
      }
    }
    else if ( timer == this.rotateTimer && this.fileRotate.checkRotate() ) {
      try {
        this.renameSaveFile();
        Stream tmpOut = this.saveOut;
        Stream newOut = new FileStream( this.saveName, FileMode.Create, FileAccess.Write );
        long[] times = this.fileRotate.nextRotate();
        long nextRotate = times[ 0 ];
        this.api.PrintLog( RaiApi.LVL_MINOR,
              "File \"" + this.saveName + "\" rotate: interval: " +
              Time.msIntervalTime( this.fileRotate.getInterval() ) +
              "; next: " + Time.msTimestamp( nextRotate, 0 ) );
        lock ( this.saveLock ) {
          this.baseTime = Time.currentTimeNanosecs();
          this.saveOut  = newOut;
        }
        tmpOut.Close();
      } catch ( Exception e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "Rotate file \"" + this.saveName + "\"" );
      }
    }
  }

  void renameSaveFile() {
    long curTime = Time.currentTimeMillis();
    string saveName2 = this.saveName + Time.strftime( Time.TZ_LOCAL_TIME, curTime, ".%Y-%m-%d_%H-%M-%S" );
    File.Move( this.saveName, saveName2 );
  }

  public bool init( RaiApi api,  Args args ) {
    this.api = api;
    try {
      raisub2Args subargs = new raisub2Args();

      bool noDictionary = args.getBoolean( subargs.noDict_arg.name );

      this.subjname     = args.getString( subargs.subject_arg.name );
      this.outName      = args.getString( subargs.output_arg.name );
      this.saveName     = args.getString( subargs.save_arg.name );
      this.inName       = args.getString( subargs.input_arg.name );
      this.doSnapshot   = args.getBoolean( subargs.snap_arg.name );
      this.doListen     = args.getBoolean( subargs.listn_arg.name );
      this.doWait       = args.getBoolean( subargs.wait_arg.name );
      this.doSched      = args.getBoolean( subargs.sched_arg.name );
      this.msgWaitCount = args.getInt( subargs.msgCnt_arg.name );
      this.doLatency    = args.getBoolean( subargs.latency_arg.name );

      if ( this.doLatency ) {
        this.msgsLat = new long[ (int) MAX_LAT ];
        this.cumLatencyMin = MIN_INIT_LAT;
      }

      long rotateIval = (long) ( args.getDouble( subargs.rotateIval_arg.name ) * 1000.0 + 0.5 );

      this.fileRotate = new TimeRotate();
      this.fileRotate.setRotateTime( args.getString( subargs.rotateTime_arg.name ) );
      this.fileRotate.setRotatePeriod( null, rotateIval );
      this.fileRotate.setLastTime( Time.currentTimeMillis() );
      long[] times = this.fileRotate.nextRotate();
      long nextRotate = times[ 0 ];

      if ( this.saveName != null ) {
        try {
          if ( nextRotate != 0 && File.Exists( this.saveName ) )
            this.renameSaveFile();
          this.saveOut = new FileStream( this.saveName, FileMode.Create, FileAccess.Write );
        } catch ( Exception e ) {
          this.api.PrintLog( RaiApi.LVL_ERROR, e, "Opening \"" + this.saveName + "\"" );
          throw new RaiException( "Open " + this.saveName, e );
        }
      }
      if ( this.outName != null ) {
        try {
          this.outp = new StreamWriter( new FileStream( this.outName, FileMode.Create, FileAccess.Write ) );
        } catch ( Exception e ) {
          this.api.PrintLog( RaiApi.LVL_ERROR, e, "Opening \"" + this.outName + "\"" );
          throw new RaiException( "Open " + this.outName, e );
        }
      }
      if ( this.outp == null )
        this.outp = Console.Out;

      RaiApi.OpenLog( args );
      /* if cfilePath specified on the command line */
      if ( ! noDictionary )
        noDictionary = RaiApi.OpenDict( args );
      this.api.ParseArgs( args );

      this.timeout = (int) ( args.getDouble( subargs.timeout_arg.name ) * 1000.0 );
      this.doRetry = args.getBoolean( subargs.retry_arg.name );
      this.doQuiet = args.getBoolean( subargs.quiet_arg.name );
      this.session = this.api.CreateSession();
      this.session.SetDataLossCB( this );
      this.session.Start();

      /* resolve dictionary */
      if ( ! noDictionary ) {
        this.dataDict = this.session.CreateDict();
        this.dataDict.Load( 3, null, false );
        while ( this.dataDict.InProgress() && ! this.quit )
          Thread.Yield();
        if ( this.quit )
          return false;
        if ( ! this.dataDict.HaveDict() ) {
          this.dataDict.Load( 10, null, false );
          while ( this.dataDict.InProgress() && ! this.quit )
            Thread.Yield();
          if ( this.quit )
            return false;
          if ( ! this.dataDict.HaveDict() ) {
            this.api.PrintLog( RaiApi.LVL_MINOR, "Dictionary load timed out" );
            this.outp.WriteLine( "Dictionary load timed out" );
            this.outp.Flush();
            return false;
          }
        }
        if ( ! doQuiet ) {
          this.outp.WriteLine( "Dictionary received" );
          this.outp.Flush();
        }
      }

      if ( subjname[ 0 ] != '-' ) {
        System.Text.StringBuilder b = new System.Text.StringBuilder();
        b.Append( subjname ).Append( '\n' );
        int n = args.getNumValues( subargs.subject_arg.name );
        for ( int i = 1; i < n; i++ )
          b.Append( args.getString( subargs.subject_arg.name, i ) ).Append( '\n' );
        this.inp = new StringReader( b.ToString() );
        this.inSource = "cmdline";
      }
      else if ( this.inName != null ) {
        try {
          this.inp = new StreamReader( this.inName );
          this.inSource = this.inName;
        } catch ( Exception e ) {
          this.api.PrintLog( RaiApi.LVL_ERROR, e, "Opening \"" + this.inName + "\"" );
          throw new RaiException( "Open " + this.inName, e );
        }
      }
      else {
        this.inp = Console.In;
        this.inSource = "stdin";
      }
      /* create queue, direct = true will cause messages to be dispatched
       * directly from the network instead of the queue */
      this.queue = this.session.CreateQueue( args.getBoolean( subargs.direct_arg.name ) );
      if ( args.getBoolean( subargs.rate_arg.name ) ) {
        /* print message rate every .5 secs in a timer */
        this.rateTimer = this.queue.CreateTimer( this );
        this.rateTimer.SetInterval( 500 );
        this.lastTime = Time.currentTimeMillis();
        this.rateTimer.Start();
      }
      if ( this.saveOut != null && nextRotate != 0 ) {
        this.api.PrintLog( RaiApi.LVL_MINOR,
              "File \"" + this.saveName + "\" rotate: interval: " +
              Time.msIntervalTime( this.fileRotate.getInterval() ) +
              "; next: " + Time.msTimestamp( nextRotate, 0 ) );
        this.rotateTimer = this.queue.CreateTimer( this );
        this.rotateTimer.SetInterval( 10000 );
        this.rotateTimer.Start();
      }
      return true;
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Not initialized, stopped" );
    }
    return false;
  }

  public bool subscribe( string subject,  int parm ) {
    try {
      RaiSubscribe newSub = this.queue.CreateSubscribe( this );
      if ( ! this.doQuiet ) {
        this.outp.WriteLine(
                  ( parm == RaiSubscribe.SNAP ? "Snapshot" :
                    parm == RaiSubscribe.UPDATE ? "Listening" : "Starting" ) +
                  " subject " + subject );
        this.outp.Flush();
      }
      lock ( this.subHT ) this.subHT[ subject ] = newSub;
      newSub.Start( subject, parm | RaiSubscribe.NO_COPY, this.timeout );
      this.subCount++;
      return true;
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Subscribe " + subject );
    }
    return false;
  }

  public bool unsubscribe( string subject ) {
    try {
      RaiSubscribe sub;
      lock ( this.subHT ) {
        if ( ! this.subHT.TryGetValue( subject, out sub ) )
          return false;
        this.subHT.Remove( subject );
      }
      if ( ! this.doQuiet ) {
        this.outp.WriteLine( "Unsubscribe subject " + subject );
        this.outp.Flush();
      }
      sub.Cancel();
      this.unsubCount++;
      return true;
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Unsubscribe " + subject );
    }
    return false;
  }

  void dispatchLoop() {
    while ( ! this.quit ) {
      try {
        this.queue.TimedDispatch( 100 );
      } catch ( RaiException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "In dispatchLoop" );
      }
    }
    this.api.PrintLog( RaiApi.LVL_MINOR, "Done dispatchLoop" );
  }

  /* thread from main() for dispatch loop */
  public void start() {
    this.dispatchThread = new Thread( this.dispatchLoop );
    this.dispatchThread.Start();
  }
  public void join() { if ( this.dispatchThread != null ) this.dispatchThread.Join(); }

  /* Subscribe subjects, then wait for quit (ctrl-c) or until message
   * count received */
  public void subLoop() {
    int parm = this.doSnapshot ? RaiSubscribe.SNAP :
               this.doListen   ? RaiSubscribe.UPDATE : RaiSubscribe.BOTH;
    int count = 0, missSub = 0, missUnsub = 0;

    if ( ! this.doSched ) {
      /* subscribe or snap subjects are read from stdin */
      for ( count = 0; ! this.quit; ) {
        string s = this.inp.ReadLine();
        if ( s == null ) /* end of input */
          break;
        if ( s.Length > 0 ) {
          if ( this.subscribe( s, parm ) ) count++;
          else missSub++;
        }
      }
      if ( ! this.doQuiet ) {
        this.outp.WriteLine( count + " subjects read on " + this.inSource );
        this.outp.Flush();
      }
    }
    else {
      /* read "SUB subject" and "UNSUB subject" commands from input,
       * if not a command and line is an integer, sleep */
      for ( count = 0; ! this.quit; ) {
        string s = this.inp.ReadLine();
        if ( s == null ) break;
        if ( s.Length > 0 ) {
          if ( s.StartsWith( "SUB " ) ) {
            if ( this.subscribe( s.Substring( 4 ), parm ) ) count++;
            else missSub++;
          }
          else if ( s.StartsWith( "UNSUB " ) ) {
            if ( this.unsubscribe( s.Substring( 6 ) ) ) --count;
            else missUnsub++;
          }
          else if ( s[ 0 ] >= '0' && s[ 0 ] <= '9' ) {
            int i = int.Parse( s );
            if ( i > 0 ) Thread.Sleep( i * 1000 );
          }
        }
      }
      this.outp.WriteLine( "Done reading SUB/UNSUB commands on stdin, waiting for " + count + " subs" );
      this.outp.Flush();
    }
    if ( this.msgWaitCount == 0 ) {
      if ( this.doSnapshot && ! this.doWait )
        this.msgWaitCount = this.subHT.Count;
    }
    if ( this.msgWaitCount > 0 ) {
      while ( ! this.quit ) {
        /* if all messages rcvd and/or timed out */
        if ( this.msgWaitCount > 0 &&
             this.msgWaitCount <= this.msgEventCount + this.msgTimeoutCount )
          this.quit = true;
        else
          Thread.Sleep( 100 );
      }
    }
    if ( this.doQuiet && this.msgTimeoutCount > 0 )
      this.api.PrintLog( RaiApi.LVL_ERROR, this.msgTimeoutCount + " subjects timeout (" + this.msgEventCount + " recv)" );
    if ( missSub > 0 )
      this.api.PrintLog( RaiApi.LVL_ERROR, missSub + " subjects did not subscribe" );
    if ( missUnsub > 0 )
      this.api.PrintLog( RaiApi.LVL_ERROR, missUnsub + " subjects did not unsubscribe" );
  }

  void finalLat() {
    double av;
    int    avg, j;
    long   cnt;
    int[]  stdDev = new int[ 4 ];
    long[] stdDevMsgs = new long[ 4 ];
    if ( this.latencyCnt == 0 ) av = 0;
    else av = this.cumLatencySum / (double) this.latencyCnt;
    avg = (int) av;
    if ( avg >= (int) MAX_LAT ) avg = (int) MAX_LAT - 1;
    cnt             = this.msgsLat[ avg ];
    stdDev[ 0 ]     = 1;
    stdDevMsgs[ 0 ] = 0;
    stdDevMsgs[ 1 ] = (long) ( this.latencyCnt * 0.682 );
    stdDevMsgs[ 2 ] = (long) ( this.latencyCnt * 0.955 );
    stdDevMsgs[ 3 ] = (long) ( this.latencyCnt * 0.997 );

    for ( j = 1; j < 4; j++ ) {
      for ( stdDev[ j ] = stdDev[ j - 1 ];
            stdDev[ j ] < MAX_LAT && cnt < stdDevMsgs[ j ]; stdDev[ j ]++ ) {
        if ( stdDev[ j ] <= avg )
          cnt += this.msgsLat[ avg - stdDev[ j ] ];
        if ( avg + stdDev[ j ] < MAX_LAT )
          cnt += this.msgsLat[ avg + stdDev[ j ] ];
      }
    }
    this.api.PrintLog( RaiApi.LVL_NORMAL,
       "av=" + ( av / 10000.0 ).ToString( "0.000000" ) +
      " min=" + ( (double) this.cumLatencyMin / 10000.0 ).ToString( "0.000000" ) +
      " max=" + ( (double) this.cumLatencyMax / 10000.0 ).ToString( "0.000000" ) +
      " stddev=(" + ( (double) stdDev[ 1 ] / 10000.0 ).ToString( "0.000000" ) +
              "," + ( (double) stdDev[ 2 ] / 10000.0 ).ToString( "0.000000" ) +
              "," + ( (double) stdDev[ 3 ] / 10000.0 ).ToString( "0.000000" ) +
              ") (68.2%,95.5%,99.7%)" );
    this.api.PrintLog( RaiApi.LVL_NORMAL, "latency overruns: " + this.latencyOverrun + " > " + ( MAX_LAT / 1000 / 10 ) + "ms" );
  }

  /* close everything */
  public void close() {
    if ( this.doLatency )
      this.finalLat();
    this.quit = true;
    if ( this.rateTimer != null ) this.rateTimer.Stop();
    if ( this.rotateTimer != null ) this.rotateTimer.Stop();

    if ( this.subHT != null ) {
      RaiSubscribe[] subs;
      lock ( this.subHT ) { subs = new RaiSubscribe[ this.subHT.Count ]; this.subHT.Values.CopyTo( subs, 0 ); this.subHT.Clear(); }
      foreach ( RaiSubscribe s in subs ) s.Cancel();
    }
    if ( this.queue != null ) this.queue.Destroy();
    if ( this.session != null ) this.session.Destroy();
    if ( this.api != null ) this.api.Close();
    if ( this.saveOut != null ) this.saveOut.Close();
  }

  public static raisub2 subTest;

  public static void sigHandler( int sig ) {
    /* be careful with locks in here, could deadlock if interrupt
     * happened while inside a critical section (for example, logging) */
    if ( subTest != null ) {
      subTest.sigCaught = sig;
      subTest.quit = true;
    }
    else
      Environment.Exit( 1 );
  }

  public static int Main( string[] argv ) {
    RaiApi      api = null;
    Args        args;
    raisub2Args subargs;

    try {
      /* traps SIGINT, SIGHUP, SIGTERM and calls sigHandler() */
      RaiApi.RegisterSigHandler( sigHandler );
      /* open log to stderr in case command line fails to parse, it may open
       * again if -log is specified on command line */
      RaiApi.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
      /* Open the api type from the command line, looks for -api <name> in
       * argv[] and loads that middleware.  Program could also pass "tibrv" or
       * some other api name in the first argument.  If neither are specfied
       * then the default api is loaded */
      api = RaiApi.RaiOpen( null, argv );
    } catch ( Exception e ) {
      Console.Error.WriteLine( "Unable to load Rai API: " + e );
      return 1;
    }

    try {
      args    = new Args();
      subargs = new raisub2Args();

      /* get the api's configuration arguments */
      api.GetArgs( args );
      /* get the subject and settings for the program */
      subargs.getArgs( args );
      /* get the arguments for the dictionary, useful for parsing dict files
       * locally in the filesystem instead of receiving it on the network */
      RaiApi.GetDictArgs( args );
      /* get the logging, version, help, rc arguments and sets error output */
      args.addDefaults( RaiApi.RaiVersion(), "rai_", Console.Error, "raisub2" );
      /* If -help or -version specified in argv[], then processArgs()
       * returns false and program exits without executing. */
      if ( args.processArgs( argv ) ) {
        subTest = new raisub2();
        /* create api elements and start the dispatch thread */
        if ( subTest.init( api, args ) ) {
          subTest.start();
          /* subscribes */
          subTest.subLoop();
          subTest.join(); /* join mainloop dispatch thread */
        }
        if ( subTest.sigCaught != 0 )
          RaiApi.Log( "Caught signal " + subTest.sigCaught + ", quitting" );
        /* stop all the subscribes, if any, close the api */
        subTest.close();
      }
    } catch ( Exception e ) {
      RaiApi.Log( RaiApi.LVL_ERROR, e, "Main" );
    }
    RaiApi.Log( RaiApi.LVL_MINOR, "Finished" );
    return 0;
  }
}
