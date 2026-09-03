/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 * .NET port of src/raiapi/java/raireplay2.java */
using System;
using System.IO;
using System.Text;
using Com.Rai.Raimsg;
using Com.Rai.Raiapi2;
using Com.Rai.Raiexception;

class raireplay2Args {
  public StringArg fileName_arg, prefix_arg;
  public IntArg    perSec_arg, msgCount_arg;
  public BoolArg   publishOnce_arg, rate_arg;
  public DoubleArg realtime_arg;

  public raireplay2Args() {
    fileName_arg    = new StringArg( "fileName", null, "<file> [<file> ...]", "Replay file name(s)" );
    perSec_arg      = new IntArg( "perSec", 1, "<num>", "Number of msgs to replay per second" );
    msgCount_arg    = new IntArg( "msgCount", 0, "<num>", "Number of msgs to publish, 0 for infinite" );
    publishOnce_arg = new BoolArg( "once", false, null, "Don't rewind files, publish records only one time" );
    prefix_arg      = new StringArg( "prefix", null, "<subject>",
                               "Publish subject prefix, usually set to _TIC. if using SASS/RV" );
    rate_arg        = new BoolArg( "rate", false, null, "Display publish rate info" );
    realtime_arg    = new DoubleArg( "realtime", 0.0, "<speed>",
                               "Replay messages at the speed that they were recorded (0 = use -perSec, " +
                               "1 = 1x record speed, 2.5 = 2.5x record speed)" );
  }

  public void getArgs( Args args ) {
    args.add( fileName_arg, Args.COMMAND_ARG | Args.RESOURCE_ARG | Args.LIST_ARG );
    args.add( perSec_arg );
    args.add( msgCount_arg );
    args.add( publishOnce_arg );
    args.add( prefix_arg );
    args.add( rate_arg );
    args.add( realtime_arg );
  }
}

public class raireplay2 : RaiTimerCallback {
  RaiApi        api;
  RaiSession    session;
  RaiQueue      pubQueue;
  RaiPublish    pub;
  RaiTimer      pubTimer, printTimer, startTimer;
  int           msgCount;
  long          msgsClocked, msgsSent, bytesSent, msgsPrint, ivalBytesSent;
  int           msgsPerSec, fileNum, fileCount, errCount;
  Stream        inp;
  long          startTime, currentTime;
  string[]      files;
  byte[]        msgBuf;
  int           bufOff, bufLen, msgOff, msgSize;
  string        subject;
  long          intervalStart, baseTime;
  double        realtimeSpeed, msgDelta;
  int           sigCaught;
  bool          publishOnce, printRate;
  volatile bool quit;

  public raireplay2() {
    this.msgBuf    = new byte[ 8 * 1024 ];
  }

  public void close() {
    this.quit = true;
    if ( this.pubTimer != null ) this.pubTimer.Stop();
    if ( this.printTimer != null ) this.printTimer.Stop();
    if ( this.startTimer != null ) this.startTimer.Stop();
    if ( this.pub != null ) this.pub.Destroy();
    if ( this.pubQueue != null ) this.pubQueue.Destroy();
    if ( this.session != null ) this.session.Destroy();
    if ( this.api != null ) this.api.Close();
    try { this.closeInput(); }
    catch ( Exception e ) { throw new RaiException( "closeInput()", e ); }
  }

  public bool init( RaiApi api,  Args args ) {
    this.api = api;
    try {
      int timeout;
      raireplay2Args repargs = new raireplay2Args();

      this.fileCount = args.getNumValues( repargs.fileName_arg.name );
      if ( this.fileCount == 0 )
        throw new RaiException( "No file, -fileName required" );

      RaiApi.OpenLog( args );
      this.api.ParseArgs( args );

      this.files = new string[ this.fileCount ];
      for ( int i = 0; i < this.fileCount; i++ )
        this.files[ i ] = args.getString( repargs.fileName_arg.name, i );
      this.api.PrintLog( RaiApi.LVL_MINOR, "Found " + this.fileCount + " files to replay" );
      string prefix = args.getString( repargs.prefix_arg.name );

      this.msgCount      = args.getInt( repargs.msgCount_arg.name );
      this.msgsPerSec    = args.getInt( repargs.perSec_arg.name );
      this.realtimeSpeed = args.getDouble( repargs.realtime_arg.name );

      if ( this.msgsPerSec >= 100 || this.realtimeSpeed != 0.0 ) {
        timeout = 1;
        if ( this.realtimeSpeed != 0.0 )
          this.realtimeSpeed = 1000000000.0 / this.realtimeSpeed; /* per nanosecond */
      }
      else {
        timeout = 100 / this.msgsPerSec;
      }
      this.publishOnce = args.getBoolean( repargs.publishOnce_arg.name );
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

      /* publish messages on a timer, started by startTimer */
      this.pubTimer = this.pubQueue.CreateTimer( this );
      this.pubTimer.SetInterval( timeout );

      if ( this.printRate ) {
        this.printTimer = this.pubQueue.CreateTimer( this );
        this.printTimer.SetInterval( 1000 );
      }
      return true;
    } catch ( Exception e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Not initialized, stopped" );
      return false;
    }
  }

  void dispatchLoop() {
    while ( ! this.quit ) {
      try { this.pubQueue.TimedDispatch( 100 ); }
      catch ( RaiException e ) { this.api.PrintLog( RaiApi.LVL_ERROR, e, "pubQueue dispatch" ); }
    }
  }

  long updateClock() {
    long t = Time.hiresTimeNanosecs();
    if ( this.startTime == 0 ) {
      this.startTime   = t;
      this.msgsClocked = 0;
    }
    else if ( t > this.startTime ) {
      this.msgsClocked = (long) ( (double) ( t - this.startTime ) / 1000000000.0 * (double) this.msgsPerSec );
    }
    return t;
  }

  /* replay format: "subject\n" "size [delta]\n" packed-message */
  bool readMsg() {
    int  i, j, n, size = 0;
    bool haveSubject = false, haveSize = false;

    for (;;) {
      if ( ! haveSubject ) {
        for ( i = this.bufOff; i < this.bufLen; i++ ) {
          if ( this.msgBuf[ i ] == '\n' ) {
            this.subject = Encoding.UTF8.GetString( this.msgBuf, this.bufOff, i - this.bufOff );
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
              double fraction = 10.0, delta = 0.0;
              for ( ; this.msgBuf[ i ] >= '0' && this.msgBuf[ i ] <= '9'; i++ )
                delta = ( delta * 10 ) + (double) ( this.msgBuf[ i ] - '0' );
              if ( this.msgBuf[ i++ ] == '.' ) {
                for ( ; this.msgBuf[ i ] >= '0' && this.msgBuf[ i ] <= '9'; i++ ) {
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
        Array.Copy( this.msgBuf, this.bufOff, this.msgBuf, 0, this.bufLen - this.bufOff );
        this.bufLen -= this.bufOff;
        this.bufOff = 0;
      }
      if ( this.bufLen == this.msgBuf.Length ) {
        byte[] msgBuf2 = new byte[ this.msgBuf.Length * 2 ];
        Array.Copy( this.msgBuf, 0, msgBuf2, 0, this.msgBuf.Length );
        this.msgBuf = msgBuf2;
      }
      n = this.inp.Read( this.msgBuf, this.bufLen, this.msgBuf.Length - this.bufLen );
      if ( n > 0 )
        this.bufLen += n;
      else if ( this.bufLen > 0 )
        throw new RaiException( "Message truncated at end of file" );
      else
        break;
    }
    return false;
  }

  public void onTimer( RaiTimer timer,  object cl ) {
    if ( timer == this.pubTimer ) this.doPub();
    else if ( timer == this.printTimer ) this.doPrint();
    else if ( timer == this.startTimer ) {
      this.startTimer.Stop();
      try {
        this.pubTimer.Start();
        if ( this.printTimer != null ) this.printTimer.Start();
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
          if ( this.quit ) return;
          if ( this.msgSize == 0 ) {
            if ( ! this.readMsg() ) this.rotateInput();
          }
          if ( this.msgSize != 0 ) {
            if ( this.msgDelta == 0 ) /* if no delta in replay file */
              break;
            this.currentTime = Time.hiresTimeNanosecs();
            if ( ( (double) ( this.currentTime - this.baseTime ) / this.realtimeSpeed ) <= this.msgDelta )
              return;
            this.publishMsg();
          }
        }
      }
      /* calculate how many messages should be sent at -perSec rate */
      this.currentTime = this.updateClock();
      while ( this.msgsSent < this.msgsClocked && ! this.quit ) {
        if ( this.msgSize == 0 ) {
          if ( ! this.readMsg() ) this.rotateInput();
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
      }
    }
  }

  void publishMsg() {
    this.pub.Publish( this.subject, this.msgBuf, this.msgOff, this.msgSize,
                      Time.hiresTimeToNsTimestamp( this.currentTime ) );
    this.msgsSent++;
    this.bytesSent += this.msgSize;
    this.msgSize = 0;
    if ( this.msgCount != 0 && --this.msgCount == 0 ) {
      this.pubTimer.Stop();
      this.quit = true;
    }
  }

  void rotateInput() {
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

  void openInput() {
    this.inp = new FileStream( this.files[ this.fileNum ], FileMode.Open, FileAccess.Read );
    this.baseTime = Time.hiresTimeNanosecs();
    if ( ! this.printRate )
      this.api.PrintLog( RaiApi.LVL_MINOR, "File: " + this.files[ this.fileNum ] );
  }

  void closeInput() {
    if ( this.inp != null ) { this.inp.Close(); this.inp = null; }
  }

  void doPrint() {
    long   curTime  = Time.hiresTimeNanosecs();
    double interval = (double) ( curTime - this.intervalStart ) / 1000000000.0;
    if ( interval >= 0.100 ) {
      this.intervalStart = curTime;
      double rate = (double) ( this.msgsSent - this.msgsPrint );
      this.msgsPrint = this.msgsSent;
      rate /= interval;
      long bs = this.bytesSent;
      string suffix = "";
      if ( rate >= 950.0 ) { rate /= 1000.0; suffix = "k"; }
      string digits = rate >= 10000.0 ? "0" : ( rate >= 1000.0 ? "0.0" : "0.00" );
      Console.WriteLine( "msgs=" + rate.ToString( digits ) + suffix + "/s data=" +
                    ( (double) ( bs - this.ivalBytesSent ) / 1000.0 / 1000.0 * 8.0 / interval ).ToString( "0.0" ) + "mbit/s" );
      this.ivalBytesSent = bs;
    }
  }

  public static raireplay2 replay;

  public static void sigHandler( int sig ) {
    if ( replay != null ) {
      replay.sigCaught = sig;
      replay.quit = true;
    }
    else
      Environment.Exit( 1 );
  }

  public static int Main( string[] argv ) {
    RaiApi         api = null;
    Args           args;
    raireplay2Args repargs;

    try {
      RaiApi.RegisterSigHandler( sigHandler );
      RaiApi.OpenLog( "-", RaiApi.LVL_MINOR, 4 );
      api = RaiApi.RaiOpen( null, argv );
    } catch ( Exception e ) {
      Console.Error.WriteLine( "Unable to load Rai API: " + e );
      return 1;
    }

    try {
      args    = new Args();
      repargs = new raireplay2Args();
      api.GetArgs( args );
      repargs.getArgs( args );
      args.addDefaults( RaiApi.RaiVersion(), "rai_", Console.Error, "raireplay2" );

      try {
        if ( args.processArgs( argv ) ) {
          replay = new raireplay2();
          if ( replay.init( api, args ) )
            replay.dispatchLoop();
          if ( replay.sigCaught != 0 )
            RaiApi.Log( "Caught signal " + replay.sigCaught + ", quitting" );
          replay.close();
        }
      } catch ( Exception e ) {
        RaiApi.Log( RaiApi.LVL_ERROR, e, "Main" );
      }
    } catch ( RaiException e ) {
      RaiApi.Log( RaiApi.LVL_ERROR, e, "Unable to load Rai API" );
    }
    RaiApi.Log( RaiApi.LVL_MINOR, "Finished" );
    return 0;
  }
}
