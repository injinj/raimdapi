/* Copyright (c) 2011 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

import java.io.*;
import java.util.Hashtable;
import java.text.DecimalFormat;
import com.rai.raimsg.*;
import com.rai.raiapi2.*;
import com.rai.raiexception.RaiException;

class raisub2Args {
  StringArg subject_arg, output_arg, input_arg, save_arg,
            rotateTime_arg;
  DoubleArg timeout_arg, rotateIval_arg;
  BoolArg   snap_arg, listn_arg, retry_arg, noDict_arg,
            direct_arg, wait_arg, quiet_arg, rate_arg,
            sched_arg, latency_arg;
  IntArg    msgCnt_arg;

  raisub2Args() {
    subject_arg = new StringArg(
      "subject", "-", "<subject> ...",
      "Subject name(s) to subscribe, use '-' to read subscriptions from stdin");
    output_arg = new StringArg(
      "output", null, "<file>",
      "Output file name, otherwise uses stdout" );
    input_arg = new StringArg(
      "input", null, "<file>",
      "Input file name, otherwise uses stdin" );
    save_arg = new StringArg(
      "save", null, "<file>",
      "Save messages to file in replay format" );
    rotateTime_arg = new StringArg(
      "rotateTime", null, "<date>",
      "Rotate save messages file at this time" );
    timeout_arg = new DoubleArg(
      "timeout", 6.0, "<time>",
      "Timeout subscription after this period if no data is received, or " +
      "zero for no timeout" );
    rotateIval_arg = new DoubleArg(
      "rotateIval", 0.0, "<time>",
      "Rotate save messages file at this interval" );
    snap_arg = new BoolArg(
      "snap", false, "<bool>",
      "Get snapshot of subject instead of subscribe" );
    listn_arg = new BoolArg(
      "listen", false, "<bool>",
      "Listen to subject instead of subscribe, no initial value requested" );
    retry_arg = new BoolArg(
      "retry", false, "<bool>",
      "Retry subscriptions after timeout period" );
    noDict_arg = new BoolArg(
      "noDict", false, "<bool>",
      "Don't try to load dictionary" );
    direct_arg = new BoolArg(
      "direct", false, "<bool>",
      "Whether to dispatch messages directly from the recv threads (true) or " +
      "serialized on the queue thread (false)" );
    wait_arg = new BoolArg(
      "wait", false, "<bool>",
      "Causes program to keep running after snapshots have completed, use " +
      "this if multiple snapshot replies are expected" );
    quiet_arg = new BoolArg(
      "quiet", false, "<bool>",
      "Don't print the messages" );
    rate_arg = new BoolArg(   "rate", false, "<bool>",
      "Print the rate of messages received" );
    sched_arg = new BoolArg(
      "sched", false, "<bool>",
      "Read scheduled subscribes and unsubscribes from stdin, useful for " +
      "generating subscription load test" );
    latency_arg = new BoolArg(
      "latency", false, "<bool>",
      "Track latency of messages and report at program end " );
    msgCnt_arg = new IntArg(
      "msgCount", 0, "<num>", "Quit after receiving num messages" );
  }

  void getArgs( Args args ) throws RaiException {
    args.add( subject_arg, Args.COMMAND_ARG | Args.RESOURCE_ARG |
                           Args.LIST_ARG );
    args.add( snap_arg );
    args.add( listn_arg );
    args.add( timeout_arg, Args.COMMAND_ARG | Args.RESOURCE_ARG |
                           Args.TIME_SEC_ARG );
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
    args.add( rotateIval_arg, Args.COMMAND_ARG | Args.RESOURCE_ARG |
                              Args.TIME_SEC_ARG );
  }
}


public class raisub2 extends Thread
  implements RaiMsgCallback, RaiDataLossCallback, RaiTimerCallback {
  RaiApi       api;      /* the api handle */
  RaiDict      dataDict; /* dictionary loader */
  RaiSession   session;  /* a session */
  RaiQueue     queue;    /* a queue for message and timer events */
  RaiTimer     rateTimer, /* a timer used for -rate calculations */
               rotateTimer; /* a timer used to rotate -save output */
  InputStream  in;       /* subjects read from here */
  PrintStream  out,      /* messages written here */
               saveOut;  /* messages written in replay format */
  String       outName,  /* file name of above streams */
               saveName,
               inName,
               subjname, /* first subject arg */
               inSource; /* source name of subjects */
  long      [] msgsLat; /* map of latency vals < 1 sec */
  long         latencyOverrun, /* latency >= 1 sec */
               latencyCnt,     /* count messages in cumLatencySum */
               cumLatencySum,  /* cumulative latency in usecs */
               cumLatencyMin,  /* latency minimum */
               cumLatencyMax;  /* latency maximum */
  int          timeout,  /* if > 0, then timeout subscription starts */
               msgWaitCount;  /* if > 0, then wait for N messages */
  long         msgEventCount, /* how many message events */
               msgByteCount,  /* how message bytes */
               msgTimeoutCount, /* how subscription timeouts */
               subCount,        /* how many subscription starts */
               unsubCount;   /* how many subscription stops */
  long         lastTime,     /* interval time since last timer event */
               baseTime;
  long         lastMsgCount, /* interval stats, for rate calculations */
               lastByteCount,
               lastSubCount,
               lastUnsubCount;
  TimeRotate   fileRotate;
  Hashtable    subHT;
  boolean      quit,
               doRetry,    /* if subscriptions timeout, retry them */
               doQuiet,    /* don't be chatty, just subscribe */
               doSnapshot, /* snapshot subscriptions */
               doListen,   /* listen subscriptions (just updates) */
               doWait,     /* wait after all snapshots have been recvd */
               doSched,    /* use subscribe and unsubscribe script */
               doLatency;  /* use msgsLat[] to track latency */
  int          sigCaught;
  final DecimalFormat fiveF,
                      sixF,
                      oneF,
                      twoF;
  static final long MAX_LAT = 1000000; /* 100ms */
  static final long MIN_INIT_LAT = 9999999;

  public raisub2() {
    this.subHT    = new Hashtable();
    this.fiveF    = new DecimalFormat( "########0.00000" );
    this.sixF     = new DecimalFormat( "########0.000000" );
    this.oneF     = new DecimalFormat( "########0.0" );
    this.twoF     = new DecimalFormat( "########0.00" );
    this.baseTime = Time.currentTimeNanosecs(); /* -save time offset */
  }

  /* RaiDataLossCallback */
  public void onConnection( RaiConnectionEvent event,  Object cl ) {
    this.api.PrintLog( RaiApi.LVL_MINOR, "onConnection: " + event.description );
  }

  public void onDataLoss( RaiDataLossEvent event,  Object closure ) {
    this.api.PrintLog( RaiApi.LVL_ERROR, "onDataLoss: " + event.description );
    if ( event.connectionLoss && event.connectionCount == 0 ) {
      try {
        this.session.NotifyStatus( SassConst.TRANSIENT,
                                   SassConst.STATUS_TPT_DISCONNECTED );
      } catch ( RaiException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "NotifyStatus" );
      }
    }
  }

  /* RaiMsgCallback */
  public void onMsg( RaiMsgEvent event,  RaiMsg raiMsg,  Object closure ) {
    long ns = ( event.pubTime != 0 || event.routeTime != 0 ) ?
                Time.currentTimeNanosecs() : 0;
    int nBytes = 0;
    try {
      /* track latencies and sumerize at program exit */
      if ( ns != 0 && this.msgsLat != null ) {
        if ( event.routeTime != 0 ) {
          long i = ( ns - event.routeTime ) / 100;
          nBytes = raiMsg.PackSize();
          /* if -direct = true, then multiple threads could callback onMsg()
           * so need to synchronize counters in that case */
          synchronized ( this.msgsLat ) {
            if ( i < MAX_LAT ) {
              this.msgsLat[ (int) i ]++;
              this.cumLatencySum += i;
              this.latencyCnt++;
              if ( i < this.cumLatencyMin )
                this.cumLatencyMin = i;
              if ( i > this.cumLatencyMax )
                this.cumLatencyMax = i;
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
        String s = event.SubscribedSubject();
        synchronized ( this.out ) {
          this.out.print( "## Subject " + s + " (old state=" +
                      RaiSubscribe.StateToString( event.oldState ) + ",new=" +
                      RaiSubscribe.StateToString( event.state ) + ")" );
          /* if event.subject is inbox name, or subscribed subject is wildcard
           * print the event.subject */
          if ( ! s.equals( event.subject ) )
            this.out.print( " (" + event.subject + ")" );
          if ( event.counter != 0 )
            this.out.print( " c=" + event.counter ); /* msg update count */
          this.out.println();
          /* if message is timestamped, print times and latencies */
          if ( ns != 0 ) {
            this.out.println( "# Receive " + Time.nsTimestamp( ns, 7 ) );
            if ( event.pubTime != 0 )
              this.out.println( "# Publish " +
                Time.nsTimestamp( event.pubTime, 7 ) +
                " (lat=" +
                fiveF.format(
                ( (double) ( (long) ( ns - event.pubTime ) ) / 1000000.0 ) ) +
                "ms)" );
            if ( event.routeTime != 0 )
              this.out.println( "# Route   " +
                Time.nsTimestamp( event.routeTime, 7 ) +
                " (lat=" +
                fiveF.format(
                ( (double) ( (long) ( ns - event.routeTime ) ) / 1000000.0 ) ) +
                "ms)" );
          }
          raiMsg.Print( this.out );
          this.out.flush();
        }
      }
      if ( this.saveOut != null ) {
        /* if message is not internally generated, save it to replay file */
        if ( event.recStatus != SassConst.STATUS_TIMEOUT &&
             event.msgType != SassConst.SERVICE_STATUS ) {

          synchronized ( this.saveOut ) {
            try {
              int size = raiMsg.PackSize();
              String s = event.subject; /* use event.subject unless inbox */
              if ( s.charAt( 0 ) == '_' && s.charAt( 1 ) == 'I' &&
                   s.startsWith( "_INBOX" ) )
                s = event.SubscribedSubject();
              if ( ns == 0 )
                ns = Time.currentTimeNanosecs();
              if ( this.baseTime > ns )
                this.baseTime = ns;
              this.saveOut.print( s + "\n" + size + " " +
                 sixF.format( (double) ( ns - this.baseTime ) / 1000000000.0 ) +
                 "\n" );
              this.saveOut.write( raiMsg.Packed(), 0, size );
              this.saveOut.flush();
              if ( this.saveOut.checkError() )
                throw new IOException();
            } catch ( Exception e ) {
              this.api.PrintLog( RaiApi.LVL_ERROR, e, "Saving message to \"" +
                                 this.saveName + "\"" );
            }
          }
        }
      }
      if ( event.state == RaiSubscribe.STATE_STALE ||
           event.recStatus == SassConst.STATUS_TIMEOUT ) {
        if ( this.doRetry ) {
          this.api.PrintLog( RaiApi.LVL_MINOR, "Refreshing subject, " +
             ( event.state == RaiSubscribe.STATE_STALE ? "stale" : "timeout" ) +
             ": \"" + event.SubscribedSubject() + "\"" );
          event.subscribe.Refresh( this.timeout );
          return; // Don't increment msgEventCount
        }
        synchronized ( this ) {
          this.msgTimeoutCount++;
        }
        this.api.PrintLog( RaiApi.LVL_NORMAL, "Subject " +
           ( event.state == RaiSubscribe.STATE_STALE ? "stale" : "timeout" ) +
           ": \"" + event.SubscribedSubject() + "\"" );
        return;
      }
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Printing message" );
      this.out.flush();
    }
    /* if tracking latency, nBytes != 0 and these are already computed */
    if ( nBytes == 0 ) {
      try {
        nBytes = raiMsg.PackSize();
        /* if -direct = true, then multiple threads could callback onMsg()
         * so need to synchronize counters in that case */
        synchronized( this ) {
          this.msgEventCount++;
          this.msgByteCount += nBytes;
        }
      } catch ( RaiMsgException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "Pack size" );
      }
    }
  }

  /* RaiTimerCallback */
  public void onTimer( RaiTimer timer,  Object closure ) {
    if ( timer == this.rateTimer ) {
      double interval;
      long   msgs,
             bytes,
             count,
             subs,
             unsubs,
             cnt;
      long curTime = System.currentTimeMillis();

      interval            = (double) ( curTime - this.lastTime ) / 1000.0;
      this.lastTime       = curTime;
      count               = this.msgEventCount;
      msgs                = count - this.lastMsgCount;
      this.lastMsgCount   = count;
      count               = this.msgByteCount;
      bytes               = count - this.lastByteCount;
      this.lastByteCount  = count;
      cnt                 = this.subCount;
      subs                = cnt - this.lastSubCount;
      this.lastSubCount   = cnt;
      cnt                 = this.unsubCount;
      unsubs              = cnt - this.lastUnsubCount;
      this.lastUnsubCount = cnt;

      synchronized ( this.out ) {
        this.out.println(
              oneF.format( (double) subs / interval ) + " sub/s " +
              oneF.format( (double) unsubs / interval ) + " unsub/s " +
              oneF.format( (double) msgs / interval ) + " msg/s " +
              oneF.format(
              (double) bytes / 1024.0 / interval ) + " kb/s " +
              twoF.format(
              (double) bytes * 8.0 / 1000.0 / 1000.0 / interval ) + " mbit/s" );
        this.out.flush();
      }
    }
    else if ( timer == this.rotateTimer && this.fileRotate.checkRotate() ) {
      try {
        this.renameSaveFile();

        PrintStream tmpOut = this.saveOut;
        PrintStream newOut = new PrintStream(
                               new FileOutputStream( this.saveName ) );
        long [] times = this.fileRotate.nextRotate();
        long nextRotate = times[ 0 ];
        long diffTime   = times[ 1 ];
        this.api.PrintLog( RaiApi.LVL_MINOR,
              "File \"" + this.saveName + "\" rotate: interval: " +
              Time.msIntervalTime( this.fileRotate.getInterval() ) +
              "; next: " + Time.msTimestamp( nextRotate, 0 ) );

        synchronized ( this.saveOut ) {
          this.baseTime = Time.currentTimeNanosecs();
          this.saveOut  = newOut;
        }
        tmpOut.close();
      } catch ( Exception e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e,
                           "Rotate file \"" + this.saveName + "\"");
      }
    }
  }

  void renameSaveFile() throws Exception {
    long curTime = System.currentTimeMillis();
    String saveName2 = this.saveName +
      Time.strftime( Time.TZ_LOCAL_TIME, curTime, ".%Y-%m-%d_%H-%M-%S" );
    new File( this.saveName ).renameTo( new File( saveName2 ) );
  }

  /* RaiService and main uses it too */
  public boolean init( RaiApi api, Args args ) throws RaiException {
    this.api = api;
    try {
      raisub2Args subargs = new raisub2Args();

      boolean noDictionary = args.getBoolean( subargs.noDict_arg.name );

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

      long rotateIval = (long) (
                 args.getDouble( subargs.rotateIval_arg.name ) * 1000.0 + 0.5 );

      this.fileRotate = new TimeRotate();
      this.fileRotate.setRotateTime(
                  args.getString( subargs.rotateTime_arg.name ) );
      this.fileRotate.setRotatePeriod( null, rotateIval );
      this.fileRotate.setLastTime( System.currentTimeMillis() );
      long [] times = this.fileRotate.nextRotate();
      long nextRotate = times[ 0 ];
      long diffTime   = times[ 1 ];

      if ( this.saveName != null ) {
        try {
          if ( nextRotate != 0 && new File( this.saveName ).exists() )
            this.renameSaveFile();
          this.saveOut = new PrintStream(
            new FileOutputStream( this.saveName ) );
        } catch ( Exception e ) {
          this.api.PrintLog( RaiApi.LVL_ERROR, e,
                             "Opening \"" + this.saveName + "\"" );
          throw new RaiException( "Open " + this.saveName, e );
        }
      }
      if ( this.outName != null ) {
        try {
          this.out = new PrintStream(
            new FileOutputStream( this.outName ) );
        } catch ( Exception e ) {
          this.api.PrintLog( RaiApi.LVL_ERROR, e,
                             "Opening \"" + this.outName + "\"" );
          throw new RaiException( "Open " + this.outName, e );
        }
      }
      if ( this.out == null )
        this.out = System.out;

      RaiApi.OpenLog( args );
      /* if cfilePaath specified on the command line */
      if ( ! noDictionary )
        noDictionary = RaiApi.OpenDict( args );
      this.api.ParseArgs( args );

      this.timeout = (int)
                     ( args.getDouble( subargs.timeout_arg.name ) * 1000.0 );
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
          ;
        if ( this.quit )
          return false;
        if ( ! this.dataDict.HaveDict() ) {
          this.dataDict.Load( 10, null, false );
          while ( this.dataDict.InProgress() && ! this.quit )
            ;
          if ( this.quit )
            return false;
          if ( ! this.dataDict.HaveDict() ) {
            this.api.PrintLog( RaiApi.LVL_MINOR, "Dictionary load timed out" );
            this.out.println( "Dictionary load timed out" );
            this.out.flush();
            return false;
          }
        }
        if ( ! doQuiet ) {
          this.out.println( "Dictionary received" );
          this.out.flush();
        }
      }

      if ( subjname.charAt( 0 ) != '-' ) {
        int i, n;
        byte [] data;
        ByteArrayOutputStream bstr = new ByteArrayOutputStream();
        PrintStream           bout = new PrintStream( bstr );

        bout.println( subjname );
        n = args.getNumValues( subargs.subject_arg.name );
        for ( i = 1; i < n; i++ )
          bout.println( args.getString( subargs.subject_arg.name, i ) );
        bout.flush();
        data = bstr.toByteArray();
        this.in = new ByteArrayInputStream( data );
        this.inSource = "cmdline";
      }
      else if ( this.inName != null ) {
        try {
          this.in = new FileInputStream( this.inName );
          this.inSource = this.inName;
        } catch ( Exception e ) {
          this.api.PrintLog( RaiApi.LVL_ERROR, e,
                             "Opening \"" + this.inName + "\"" );
          throw new RaiException( "Open " + this.inName, e );
        }
      }
      else {
        this.in = System.in;
        this.inSource = "stdin";
      }
      /* create queue, direct = true will cause messages to be dispatched
       * directly from the network instead of the queue */
      this.queue = this.session.CreateQueue(
                                   args.getBoolean( subargs.direct_arg.name ) );
      if ( args.getBoolean( subargs.rate_arg.name ) ) {
        /* print message rate every .5 secs in a timer */
        this.rateTimer = this.queue.CreateTimer( this );
        this.rateTimer.SetInterval( 500 );
        this.lastTime = System.currentTimeMillis();
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
      subthr = new SubThread( this );
      return true;
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Not initialized, stopped" );
    }
    return false;
  }

  public boolean subscribe( String subject,  int parm ) {
    try {
      RaiSubscribe newSub = this.queue.CreateSubscribe( this );
      if ( ! this.doQuiet ) {
        this.out.println(
                  ( parm == RaiSubscribe.SNAP ? "Snapshot" :
                    parm == RaiSubscribe.UPDATE ? "Listening" : "Starting" ) +
                  " subject " + subject );
        this.out.flush();

      }
      this.subHT.put( subject, newSub );
      newSub.Start( subject, parm | RaiSubscribe.NO_COPY, this.timeout );
      this.subCount++;
      return true;
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Subscribe " + subject );
    }
    return false;
  }

  public boolean unsubscribe( String subject ) {
    try {
      RaiSubscribe sub;
      sub = (RaiSubscribe) this.subHT.remove( subject );
      if ( sub == null )
        return false;
      if ( ! this.doQuiet ) {
        this.out.println( "Unsubscribe subject " + subject );
        this.out.flush();
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

  /* RaiService, thread from raicache to service */
  public void serviceRun() {
    this.subthr.start();
    this.dispatchLoop();
  }

  /* thread from main() for dispatch loop */
  public void run() {
    this.dispatchLoop();
  }

  class SubThread extends Thread {
    raisub2 me;

    SubThread( raisub2 m ) { me = m; }

    public void run() {
      try {
        this.me.subLoop();
      } catch ( Exception e ) {
        this.me.api.PrintLog( RaiApi.LVL_ERROR, e, "In subLoop" );
      }
      this.me.api.PrintLog( RaiApi.LVL_MINOR, "Done run" );
    }
  }
  SubThread subthr;

  /* Subscribe subjects, then wait for quit (ctrl-c) or until message 
   * count received */
  void subLoop() throws Exception {
    int parm = this.doSnapshot ? RaiSubscribe.SNAP :
               this.doListen   ? RaiSubscribe.UPDATE : RaiSubscribe.BOTH;
    int n, count = 0, missSub = 0, missUnsub = 0;
    BufferedReader rd = new BufferedReader( new InputStreamReader( this.in ) );

    if ( ! this.doSched ) {
      /* subscribe or snap subjects are read fron stdin */
      for ( count = 0; ! this.quit; ) {
        String s = rd.readLine();
        if ( s == null ) /* end of input */
          break;
        if ( s.length() > 0 ) {
          if ( this.subscribe( s, parm ) )
            count++;
          else
            missSub++;
        }
      }
      if ( ! this.doQuiet ) {
        this.out.println( count + " subjects read on " + this.inSource );
        this.out.flush();
      }
    }
    else {
      /* read "SUB subject" and "UNSUB subject" commands from input,
       * if not a command and line is an integer, sleep */
      for ( count = 0; ! this.quit; ) {
        String s = rd.readLine();
        if ( s == null ) /* end of input */
          break;
        if ( s.length() > 0 ) {
          if ( s.startsWith( "SUB " ) ) {
            if ( this.subscribe( s.substring( 4 ), parm ) )
              count++;
            else
              missSub++;
          }
          else if ( s.startsWith( "UNSUB " ) ) {
            if ( this.unsubscribe( s.substring( 6 ) ) )
              --count;
            else
              missUnsub++;
          }
          else if ( s.charAt( 0 ) >= '0' && s.charAt( 0 ) <= '9' ) {
            int i = Integer.parseInt( s );
            if ( i > 0 )
              Thread.sleep( i * 1000 );
          }
        }
      }
      //if ( ! doQuiet ) {
        this.out.println( "Done reading SUB/UNSUB commands on stdin, " +
                           "waiting for " + count + " subs" );
        this.out.flush();
      //}
    }
    if ( this.msgWaitCount == 0 ) {
      if ( this.doSnapshot && ! this.doWait )
        this.msgWaitCount = this.subHT.size();
    }
    if ( this.msgWaitCount > 0 ) {
      while ( ! this.quit ) {
        /* if all messages rcvd and/or timed out */
        if ( this.msgWaitCount > 0 &&
             this.msgWaitCount <= this.msgEventCount + this.msgTimeoutCount )
          this.quit = true;
        else
          Thread.sleep( 100 );
      }
    }
    if ( this.doQuiet && this.msgTimeoutCount > 0 ) {
      this.api.PrintLog( RaiApi.LVL_ERROR,
             this.msgTimeoutCount + " subjects timeout (" +
             this.msgEventCount + " recv)" );
    }
    if ( missSub > 0 ) {
      this.api.PrintLog( RaiApi.LVL_ERROR,
                         missSub + " subjects did not subscribe" );
    }
    if ( missUnsub > 0 ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, 
                         missUnsub + " subjects did not unsubscribe" );
    }
  }

  void finalLat() {
    double  av;
    int     avg, j;
    long    cnt;
    int  [] stdDev = new int[ 4 ];
    long [] stdDevMsgs = new long[ 4 ];
    if ( this.latencyCnt == 0 )
      av = 0;
    else
      av = this.cumLatencySum / (double) this.latencyCnt;
    avg = (int) av;
    if ( avg >= (int) MAX_LAT )
      avg = (int) MAX_LAT - 1;
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
       "av=" + sixF.format( av / 10000.0 ) +
      " min=" + sixF.format( (double) this.cumLatencyMin / 10000.0 ) +
      " max=" + sixF.format( (double) this.cumLatencyMax / 10000.0 ) +
      " stddev=(" + sixF.format( (double) stdDev[ 1 ] / 10000.0 ) +
              "," + sixF.format( (double) stdDev[ 2 ] / 10000.0 ) +
              "," + sixF.format( (double) stdDev[ 3 ] / 10000.0 ) +
              ") (68.2%%,95.5%%,99.7%%)" );
    this.api.PrintLog( RaiApi.LVL_NORMAL, "latency overruns: " +
                       this.latencyOverrun + " > " +
                       ( MAX_LAT / 1000 / 10 ) + "ms" );
  }

  /* RaiService and main, close everything */
  public void close() throws RaiException {
    if ( this.doLatency )
      this.finalLat();
    this.quit = true;
    if ( this.rateTimer != null )
      this.rateTimer.Stop();
    if ( this.rotateTimer != null )
      this.rotateTimer.Stop();
    if ( this.subthr != null ) {
      while ( this.subthr.isAlive() ) {
        try {
          this.subthr.join();
        } catch ( InterruptedException i ) {
        }
      }
    }

    if ( this.subHT != null ) {
      Object [] subs = this.subHT.values().toArray();
      if ( subs != null ) {
        for ( int i = 0; i < subs.length; i++ ) {
          ((RaiSubscribe) subs[ i ]).Cancel();
        }
      }
      this.subHT.clear();
    }
    if ( this.queue != null )
      this.queue.Destroy();
    if ( this.session != null )
      this.session.Destroy();
    if ( this.api != null )
      this.api.Close();
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
      System.exit( 1 );
  }

  public static void main( String[] argv ) {
    RaiApi      api = null;
    Args        args;
    raisub2Args subargs;

    try {
      /* traps SIGINT, SIGHUP, SIGTERM and calls sigHandler(),
       * this may require LD_PRELOAD=libjsig.so and/or
       * java option -Xrs (rs = reduce signals) */
      RaiApi.RegisterSigHandler( raisub2.class.getName(), "sigHandler" );
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
      args    = new Args();
      subargs = new raisub2Args();

      /* get the api's configuration arguments */
      api.GetArgs( args );
      /* get the subject and settings for the program */
      subargs.getArgs( args );
      /* get the arguments for the dictionary, useful for parsing dict files
       * locally in the filesystem instead of receiving it on the network */
      api.GetDictArgs( args );
      /* get the logging, version, help, rc arguemtns and sets error output */
      args.addDefaults( api.RaiVersion(), "rai_", System.err, "raisub2" );
      /* Separate arguments on the command line, process the arg types.
       * This causes the args to contain values specfied on the command line,
       * the init() procedure extracts these arg values.
       * If -help or -version specified in argc/argv[], then processArgs()
       * returns false and program exits without executing.
       * It is also possible to set the args with:
       * args.setString( "subject", "TEST" );
       * args.setBoolean( "rate", true );
       * args.setDouble( "timeout", 4.0 ); // seconds, or
       * args.setString( "timeout", "1 minute" );
       * args.setString( "quiet", "true" );
       * instead of parsing command line with processArgs() */
      if ( args.processArgs( argv ) ) {
        subTest = new raisub2();
        /* create api elements and start the dispatch thread */
        if ( subTest.init( api, args ) ) {
          subTest.start();
          /* subscribes */
          subTest.subLoop();
          while ( subTest.isAlive() ) {
            try {
              subTest.join(); /* join mainloop dispatch thread */
            } catch ( InterruptedException i ) {
              api.PrintLog( RaiApi.LVL_NORMAL, i, "Quitting..." );
              subTest.quit = true;
            }
          }
        }
        if ( subTest.sigCaught != 0 )
          api.Log( "Caught signal " + subTest.sigCaught + ", quitting" );
        /* stop all the subscribes, if any, close the api */
        subTest.close();
      }
    } catch ( Exception e ) {
      RaiApi.Log( RaiApi.LVL_ERROR, e, "Main" );
    }
    RaiApi.Log( RaiApi.LVL_MINOR, "Finished" );
  }
}
