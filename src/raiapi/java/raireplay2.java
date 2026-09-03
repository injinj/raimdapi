/* Copyright (c) 2011 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

import java.io.*;
import java.text.DecimalFormat;
import com.rai.raimsg.*;
import com.rai.raiapi2.*;
import com.rai.raiexception.RaiException;

/* raicache will look for rai_service_* classes in the CLASSPATH
 * and run them when they are configured in cache.xml, for example:
 *
 * <service-jraireplay2 subject="PING.TEST" perSec="1000" />
 *
 * This would cause and instance of class raireplay2 to be run with arguments
 * configured in the service-jraireplay2 XML tag.
 */
class rai_service_jraireplay2 extends RaiServiceFactory {
  final static String svcname = "jraireplay2";

  public String getName() throws RaiException {
    return this.svcname;
  }

  public void getArgs( Args args ) throws RaiException {
    raireplay2Args pingargs = new raireplay2Args();
    pingargs.getArgs( args );
  }

  public RaiService newService() throws RaiException {
    return new raireplay2();
  }
}

class raireplay2Args {
  StringArg fileName_arg;
  IntArg    perSec_arg,
            msgCount_arg;
  BoolArg   publishOnce_arg;
  StringArg prefix_arg;
  BoolArg   rate_arg;
  DoubleArg realtime_arg;

  raireplay2Args() {
    fileName_arg    = new StringArg( "fileName", null, "<file> [<file> ...]",
                               "Replay file name(s)" );
    perSec_arg      = new IntArg( "perSec", 1, "<num>",
                               "Number of msgs to replay per second" );
    msgCount_arg    = new IntArg( "msgCount", 0, "<num>",
                               "Number of msgs to publish, 0 for infinite" );
    publishOnce_arg = new BoolArg( "once", false, null,
                          "Don't rewind files, publish records only one time" );
    prefix_arg      = new StringArg( "prefix", null, "<subject>", 
                               "Publish subject prefix, usually set to " +
                               "_TIC. if using SASS/RV" );
    rate_arg        = new BoolArg( "rate", false, null,
                               "Display publish rate info" );
    realtime_arg    = new DoubleArg( "realtime", 0.0, "<speed>",
                               "Replay messages at the speed that they " +
                               "were recorded (0 = use -perSec, " +
                              "1 = 1x record speed, 2.5 = 2.5x record speed)" );
  }

  void getArgs( Args args ) throws RaiException {
    args.add( fileName_arg, Args.COMMAND_ARG | Args.RESOURCE_ARG |
                            Args.LIST_ARG );
    args.add( perSec_arg );
    args.add( msgCount_arg );
    args.add( publishOnce_arg );
    args.add( prefix_arg );
    args.add( rate_arg );
    args.add( realtime_arg );
  }
}

public class raireplay2 implements RaiTimerCallback, RaiService {
  RaiApi        api;
  RaiSession    session;
  RaiQueue      pubQueue;
  RaiPublish    pub;
  RaiTimer      pubTimer,
                printTimer,
                startTimer;
  int           msgCount;
  long          msgsClocked,
                msgsSent,
                bytesSent,
                msgsPrint,
                ivalBytesSent;
  int           msgsPerSec,
                fileNum,
                fileCount,
                errCount;
  InputStream   in;
  long          startTime,
                currentTime,
                timer;
  String     [] files;
  byte       [] msgBuf;
  int           bufOff,
                bufLen,
                msgOff,
                msgSize;
  String        subject;
  int           msgTypeId,
                prefixLen;
  long          ivalMSecs,
                intervalStart,
                baseTime;
  double        realtimeSpeed,
                msgDelta;
  int           sigCaught;
  boolean       publishOnce,
                quit,
                printRate;
  final DecimalFormat zeroF,
                      oneF,
                      twoF;
  public raireplay2() {
    this.ivalMSecs = 500;
    this.msgBuf    = new byte[ 8 * 1024 ];
    this.zeroF     = new DecimalFormat( "########0" );
    this.oneF      = new DecimalFormat( "########0.0" );
    this.twoF      = new DecimalFormat( "########0.00" );
  }

  public void close() throws RaiException {
    this.quit = true;
    if ( this.pubTimer != null )
      this.pubTimer.Stop();
    if ( this.printTimer != null )
      this.printTimer.Stop();
    if ( this.startTimer != null )
      this.startTimer.Stop();
    if ( this.pub != null )
      this.pub.Destroy();
    if ( this.pubQueue != null )
      this.pubQueue.Destroy();
    if ( this.session != null )
      this.session.Destroy();
    if ( this.api != null )
      this.api.Close();
    try {
      this.closeInput();
    } catch ( Exception e ) {
      throw new RaiException( "closeInput()", e );
    }
  }

  public boolean init( RaiApi api,  Args args ) {
    this.api = api;
    try {
      int i, timeout;
      raireplay2Args repargs = new raireplay2Args();

      this.fileCount = args.getNumValues( repargs.fileName_arg.name );
      if ( this.fileCount == 0 )
        throw new RaiException( "No file, -fileName required" );

      RaiApi.OpenLog( args );
      this.api.ParseArgs( args );

      /* alloc space for the file names */
      this.files = new String[ this.fileCount ];
      for ( i = 0; i < this.fileCount; i++ )
	this.files[ i ] = args.getString( repargs.fileName_arg.name, i);
      this.api.PrintLog( RaiApi.LVL_MINOR, "Found " + this.fileCount +
                         " files to replay" );
      /* subject prefix */
      String prefix = args.getString( repargs.prefix_arg.name );

      /* number of messages to publish */
      this.msgCount   = args.getInt( repargs.msgCount_arg.name );
      /* how fast to publish them */
      this.msgsPerSec = args.getInt( repargs.perSec_arg.name );
      this.realtimeSpeed = args.getDouble( repargs.realtime_arg.name );

      if ( this.msgsPerSec >= 100 || this.realtimeSpeed != 0.0 ) {
        timeout = 1;
        if ( this.realtimeSpeed != 0.0 )
          /* normalize per nanosecond */
          this.realtimeSpeed = 1000000000.0 / this.realtimeSpeed;
      }
      else {
        timeout = 100 / this.msgsPerSec;
      }
      /* if only publish each file one time */
      this.publishOnce = args.getBoolean( repargs.publishOnce_arg.name );
      /* print rate of publish to stdout */
      this.printRate   = args.getBoolean( repargs.rate_arg.name );
      this.openInput();

      this.session  = this.api.CreateSession();
      this.session.Start();
      this.pubQueue = this.session.CreateQueue();
      this.pub      = this.session.CreatePublish();
      this.pub.SetPrefix( prefix );

      this.startTimer = this.pubQueue.CreateTimer( this );
      this.startTimer.SetInterval( 1000 ); /* delay start 1 second */
      this.startTimer.Start();

      /* publish messages on a timer */
      this.pubTimer = this.pubQueue.CreateTimer( this );
      this.pubTimer.SetInterval( timeout );
      //this.pubTimer.Start(); /* delayed 1 second */

      /* print rate every second */
      if ( this.printRate ) {
        this.printTimer = this.pubQueue.CreateTimer( this );
        this.printTimer.SetInterval( 1000 );
        //this.printTimer.Start();
      }
      return true;
    } catch ( Exception e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Not initialized, stopped" );
      return false;
    }
  }

  public void serviceRun() {
    this.dispatchLoop();
  }

  void dispatchLoop() {
    while ( ! this.quit ) {
      try {
        this.pubQueue.TimedDispatch( 100 );
      } catch ( RaiException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "pubQueue dispatch" );
      }
    }
  }

  long updateClock() {
    long t;

    t = Time.hiresTimeNanosecs();
    if ( this.startTime == 0 ) {
      this.startTime   = t;
      this.msgsClocked = 0;
    }
    else if ( t > this.startTime ) {
      this.msgsClocked = (long)
        ( (double) ( t - this.startTime ) / 1000000000.0 *
          (double) this.msgsPerSec );
    }
    return t;
  }

  boolean readMsg() throws Exception {
    int     i, j, n, size = 0;
    boolean haveSubject = false,
            haveSize = false;

    for (;;) {
      if ( ! haveSubject ) {
        for ( i = this.bufOff; i < this.bufLen; i++ ) {
          if ( this.msgBuf[ i ] == '\n' ) {
            this.subject = new String( this.msgBuf, this.bufOff,
                                       i - this.bufOff );
            haveSubject = true;
            this.bufOff = i + 1;
            break;
          }
        }
      }
      if ( haveSubject && ! haveSize ) {
        i = this.bufOff;
        j = this.bufOff;
        for ( ; j < this.bufLen; j++ ) {
          if ( this.msgBuf[ j ] == '\n' ) {
            for ( ; this.msgBuf[ i ] >= '0' && this.msgBuf[ i ] <= '9'; i++ )
              size = size * 10 + this.msgBuf[ i ] - '0';
            if ( size == 0 )
              throw new RaiException( "Invalid size, not a number" );
            haveSize = true;
            this.bufOff = j + 1;
            if ( this.msgBuf[ i++ ] == ' ' && this.realtimeSpeed != 0.0 ) {
              double  fraction = 10.0, delta = 0.0;
              for ( ; this.msgBuf[ i ] >= '0' && this.msgBuf[ i ] <= '9'; i++ )
                delta = ( delta * 10 ) + (double) ( this.msgBuf[ i ] - '0' );
              if ( this.msgBuf[ i++ ] == '.' ) {
                for ( ; this.msgBuf[ i ] >= '0' && this.msgBuf[ i ] <= '9';
                      i++ ) {
                  delta += (double) ( this.msgBuf[ i ] - '0' ) / fraction;
                  fraction *= 10.0;
                }
              }
              this.msgDelta = delta;
            }
            break;
          }
        }
      }
      if ( haveSubject && haveSize ) {
        if ( this.bufOff + size <= this.bufLen ) {
          this.msgSize = size;
          this.msgOff  = this.bufOff;
          this.bufOff += size;
          return true;
        }
      }

      if ( this.bufOff > 0 ) {
        System.arraycopy( this.msgBuf, this.bufOff, this.msgBuf,
                          0, this.bufLen - this.bufOff );
        this.bufLen -= this.bufOff;
        this.bufOff = 0;
      }
      if ( this.bufLen == this.msgBuf.length ) {
        byte [] msgBuf2 = new byte[ this.msgBuf.length * 2 ];
        System.arraycopy( this.msgBuf, 0, msgBuf2, 0, this.msgBuf.length );
        this.msgBuf = msgBuf2;
      }
      n = this.in.read( this.msgBuf, this.bufLen,
                        this.msgBuf.length - this.bufLen );
      if ( n > 0 )
        this.bufLen += n;
      else if ( this.bufLen > 0 )
        throw new RaiException( "Message truncated at end of file" );
      else
        break;
    }
    return false;
  }

  public void onTimer( RaiTimer timer,  Object cl ) {
    if ( timer == this.pubTimer )
      this.doPub();
    else if ( timer == this.printTimer )
      this.doPrint();
    else if ( timer == this.startTimer ) {
      this.startTimer.Stop();
      try {
        this.pubTimer.Start();
        if ( this.printTimer != null )
          this.printTimer.Start();
      } catch ( RaiException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "Starting timer" );
        this.quit = true;
      }
    }
  }

  void doPub() {
    try {
      /* replay at realtime rate */
      if ( this.realtimeSpeed != 0.0 ) {
        for (;;) {
          if ( this.quit )
            return;
          if ( this.msgSize == 0 ) {
            if ( ! this.readMsg() )
              this.rotateInput();
          }
          if ( this.msgSize != 0 ) {
            if ( this.msgDelta == 0 ) /* if no delta in replay file */
              break;
            /* compare stamps */
            this.currentTime = Time.hiresTimeNanosecs();
            if ( ( (double) ( this.currentTime - this.baseTime ) /
                  this.realtimeSpeed ) <= this.msgDelta )
              return;
            this.publishMsg();
          }
        }
      }
      /* calculate how many messages should be sent at -perSec rate */
      this.currentTime = this.updateClock();

      /* replay at constant perSec rate */
      while ( this.msgsSent < this.msgsClocked && ! this.quit ) {
        if ( this.msgSize == 0 ) {
          if ( ! this.readMsg() )
            this.rotateInput();
        }
        if ( this.msgSize != 0 ) {
          this.publishMsg();
          this.currentTime = Time.hiresTimeNanosecs();
        }
      }
    } catch ( Exception e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Publishing msg" );
      if ( this.errCount++ > 300 ) {
        this.pubTimer.Stop();
        this.quit = true;
        return;
      }
    }
  }

  void publishMsg() throws RaiException {
    this.pub.Publish( this.subject, this.msgBuf, this.msgOff,
                this.msgSize, Time.hiresTimeToNsTimestamp( this.currentTime ) );
    this.msgsSent++;
    this.bytesSent += this.msgSize;
    this.msgSize = 0;

    if ( this.msgCount != 0 && --this.msgCount == 0 ) {
      this.pubTimer.Stop();
      this.quit = true;
    }
  }

  void rotateInput() throws Exception {
    this.closeInput();

    if ( ++this.fileNum >= this.fileCount ) {
      this.fileNum = 0;
      if ( this.publishOnce ) {
        this.pubTimer.Stop();
        this.quit = true;
        return;
      }
    }
    this.openInput();
  }

  void openInput() throws Exception {
    this.in = new FileInputStream( this.files[ this.fileNum ] );
    this.baseTime = Time.hiresTimeNanosecs();

    if ( ! this.printRate )
      this.api.PrintLog( RaiApi.LVL_MINOR, "File: " +
                         this.files[ this.fileNum ] );
  }

  void closeInput() throws Exception {
    if ( this.in != null ) {
      this.in.close();
      this.in = null;
    }
  }

  void doPrint() {
    long   curTime;
    double interval,
           rate;
    long   bs;

    curTime  = Time.hiresTimeNanosecs();
    interval = (double) ( curTime - this.intervalStart ) / 1000000000.0;
    if ( interval >= 0.100 ) {
      this.intervalStart = curTime;

      rate = (double) ( this.msgsSent - this.msgsPrint );
      this.msgsPrint = this.msgsSent;
      rate /= interval;
      bs = this.bytesSent;

      String suffix = "";
      if ( rate >= 950.0 ) {
        rate /= 1000.0;
        suffix = "k";
      }

      DecimalFormat digits =
        ( rate >= 10000.0 ? zeroF : ( rate >= 1000.0 ? oneF : twoF ) );
      System.out.println( "msgs=" + digits.format( rate ) + suffix +
                    "/s data=" + oneF.format(
                      (double) ( bs - this.ivalBytesSent ) /
                      1000.0 / 1000.0 * 8.0 / interval ) + "mbit/s" );
      this.ivalBytesSent = bs;
    }
  }

  public static raireplay2 replay;

  public static void sigHandler( int sig ) {
    /* be careful with locks in here, could deadlock if interrupt
     * happened while inside a critical section (for example, logging) */
    if ( replay != null ) {
      replay.sigCaught = sig;
      replay.quit = true;
    }
    else
      System.exit( 1 );
  }

  public static void main( String[] argv ) {
    RaiApi         api = null;
    Args           args;
    raireplay2Args repargs;

    try {
      /* traps SIGINT, SIGHUP, SIGTERM and calls sigHandler(),
       * this may require LD_PRELOAD=libjsig.so and/or
       * java option -Xrs (rs = reduce signals) */
      RaiApi.RegisterSigHandler( raireplay2.class.getName(), "sigHandler" );
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
      repargs = new raireplay2Args();
      /* get the api's configuration arguments */
      api.GetArgs( args );
      /* get the subject and settings for the program */
      repargs.getArgs( args );
      /* get the logging, version, help, rc arguments and sets error output */
      args.addDefaults( api.RaiVersion(), "rai_", System.err, "raireplay2" );

      try {
        if ( args.processArgs( argv ) ) {
          replay = new raireplay2();
          /* create api elements and start the dispatch thread */
          if ( replay.init( api, args ) )
            replay.dispatchLoop();
          /* stop publishers, if any, close the api */
          if ( replay.sigCaught != 0 )
            api.Log( "Caught signal " + replay.sigCaught + ", quitting" );
          replay.close();
        }
      } catch ( Exception e ) {
        RaiApi.Log( RaiApi.LVL_ERROR, e, "Main" );
      }
    } catch ( RaiException e ) {
      RaiApi.Log( RaiApi.LVL_ERROR, e, "Unable to load Rai API" );
    }
    RaiApi.Log( RaiApi.LVL_MINOR, "Finished" );
  }
}
