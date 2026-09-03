/* Copyright (c) 2009 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_api__raisub2_cpp[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */

/* for keyboard interrupt (ctrl-c) */
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <signal.h>
#else
#include <windows.h>
#endif
#include "raiapi2.h"
/* for Sys::out->printf() */
#include "stream/io_stream.h"
#include "stream/file_stream.h"
#include "stream/byte_array_stream.h"
#include "base/thread.h"
#include "base/file.h"
#include "util/str_util.h"
#include "util/atomic.h"
#include "msg/string_to_msg.h"


namespace { /* hide classes if dll */

/* the arguments for raisub2 */
struct RaiSub2Args {
  const rai::StringArg subject_arg, output_arg, input_arg, save_arg,
                       rotateTime_arg, snapMsg_arg;
  const rai::DoubleArg timeout_arg, rotateIval_arg, rateIval_arg;
  const rai::BoolArg   snap_arg, listn_arg, retry_arg, noDict_arg,
                       direct_arg, wait_arg, quiet_arg, rate_arg,
                       sched_arg, latency_arg;
  const rai::UIntArg   msgCnt_arg;

  RaiSub2Args() :
    subject_arg("subject", "-", "<subject> ...",
                "Subject name(s) to subscribe, use '-' to read "
                "subscriptions from stdin" ),
    output_arg( "output", NULL, "<file>",
               "Output file name, otherwise uses stdout" ),
    input_arg( "input", NULL, "<file>",
               "Input file name, otherwise uses stdin" ),
    save_arg(   "save", NULL, "<file>",
                "Save messages to file in replay format" ),
    rotateTime_arg( "rotateTime", NULL, "<date>",
                "Rotate save messages file at this time" ),
    snapMsg_arg( "snapMsg", NULL, "<field=val,...>", "Snapshot fields "
                "sent with sub which are used to transform the message at "
                "raicache" ),
    timeout_arg("timeout", 6.0, "<time>",
                "Timeout subscription after this period "
                "if no data is received, or zero for no "
                "timeout" ),
    rotateIval_arg( "rotateIval", 0.0, "<time>",
                "Rotate save messages file at this interval" ),
    rateIval_arg( "rateIval", 0.5, "<time>",
                "Interval to print rate" ),
    snap_arg(   "snap", false, "<bool>",
                "Get snapshot of subject instead of subscribe" ),
    listn_arg(  "listen", false, "<bool>",
                "Listen to subject instead of subscribe, "
                "no initial value requested" ),
    retry_arg(  "retry", false, "<bool>",
                "Retry subscriptions after timeout period" ),
    noDict_arg( "noDict", false, "<bool>",
                "Don't try to load dictionary" ),
    direct_arg( "direct", false, "<bool>",
                "Whether to dispatch messages directly from the recv "
                "threads (true) or serialized on the queue thread (false)" ),
    wait_arg(   "wait", false, "<bool>",
                "Causes program to keep running after snapshots have "
                "completed, use this if multiple snapshot replies are "
                "expected" ),
    quiet_arg(  "quiet", false, "<bool>",
                "Don't print the messages" ),
    rate_arg(   "rate", false, "<bool>",
                "Print the rate of messages received" ),
    sched_arg(  "sched", false, "<bool>",
                "Read scheduled subscribes and unsubscribes from "
                "stdin, useful for generating subscription load test" ),
    latency_arg( "latency", false, "<bool>",
                "Track latency of messages and report at program end " ),
    msgCnt_arg( "msgCount", 0, "<num>",
                "Quit after receiving num messages" ) {}

  void getArgs( rai::Args &args ) const {
    args.add( &subject_arg, rai::COMMAND_ARG | rai::RESOURCE_ARG |
                            rai::LIST_ARG );
    args.add( &snap_arg );
    args.add( &listn_arg );
    args.add( &timeout_arg, rai::COMMAND_ARG | rai::RESOURCE_ARG |
                            rai::TIME_SEC_ARG );
    args.add( &retry_arg );
    args.add( &noDict_arg );
    args.add( &direct_arg );
    args.add( &wait_arg );
    args.add( &quiet_arg );
    args.add( &rate_arg );
    args.add( &rateIval_arg, rai::COMMAND_ARG | rai::RESOURCE_ARG |
                             rai::TIME_SEC_ARG );
    args.add( &sched_arg );
    args.add( &latency_arg );
    args.add( &msgCnt_arg );
    args.add( &output_arg );
    args.add( &input_arg );
    args.add( &save_arg );
    args.add( &snapMsg_arg );
    args.add( &rotateTime_arg );
    args.add( &rotateIval_arg, rai::COMMAND_ARG | rai::RESOURCE_ARG |
                               rai::TIME_SEC_ARG );
  }
};

/* map subjects and RaiSubscribe handles */
struct RaiSub2HT {
  RaiSubscribe ** subHT; 
  unsigned int    nSubs,
                  htSize;

  RaiSub2HT() : subHT( 0 ), nSubs( 0 ), htSize( 0 ) {}

  void releaseHT( void ) {
    if ( this->subHT != NULL ) {
      for ( unsigned int i = 0; i < this->htSize; i++ )
        if ( this->subHT[ i ] != NULL )
          delete this->subHT[ i ];
      FREE( this->subHT );
      this->subHT  = NULL;
      this->htSize = 0;
      this->nSubs  = 0;
    }
  }

  static unsigned int hash( const char *s ) {
    unsigned int key = 0;
    while ( *s != '\0' )
      key = ( (unsigned int) (unsigned char) *s++ ) ^ ( ( key << 5 ) + key );
    return key;
  }

  void resizeHT( void ) {
    unsigned int i, n = this->htSize, newSize = ( n == 0 ? 8 : n * 2 );
    REALLOC( newSize * sizeof( this->subHT[ 0 ] ), &this->subHT );
    ::memset( &this->subHT[ n ], 0, sizeof( this->subHT[ 0 ] ) * ( newSize -n));
    for ( i = 0; i < n || this->subHT[ i ] != NULL; i++ ) {
      RaiSubscribe * sub = this->subHT[ i ];
      if ( sub != NULL ) {
        unsigned int h = RaiSub2HT::hash( sub->Subject() ),
                     j = h & ( newSize - 1 );
        if ( i != j ) {
          this->subHT[ i ] = NULL;
          while ( this->subHT[ j ] != NULL )
            j = ( j + 1 ) & ( newSize - 1 );
          this->subHT[ j ] = sub;
        }
      }
    }
    this->htSize = newSize;
  }

  void addHT( const char *subject,
              RaiSubscribe *newSub ) {
    unsigned int h = RaiSub2HT::hash( subject ),
                 n = this->nSubs;
    if ( n == this->htSize / 2 + this->htSize / 4 )
      this->resizeHT();
    for ( h &= ( this->htSize - 1 ); this->subHT[ h ] != NULL; )
      h = ( h + 1 ) & ( this->htSize - 1 );
    this->subHT[ h ] = newSub;
    this->nSubs = n + 1;
  }

  RaiSubscribe *removeHT( const char *subject ) {
    if ( this->nSubs > 0 ) {
      unsigned int j, h = RaiSub2HT::hash( subject ) & ( this->htSize - 1 );
      for ( ; ; h = ( h + 1 ) & ( this->htSize - 1 ) ) {
        RaiSubscribe * sub = this->subHT[ h ];
        if ( sub == NULL )
          break;
        if ( ::strcmp( subject, sub->Subject() ) == 0 ) {
          this->subHT[ h ] = NULL;
          for (;;) {
            h = ( h + 1 ) & ( this->htSize - 1 );
            if ( this->subHT[ h ] == NULL )
              break;
            RaiSubscribe * p = this->subHT[ h ];
            j = ( hash( p->Subject() ) & ( this->htSize - 1 ) );
            if ( j != h ) {
              this->subHT[ h ] = NULL;
              while ( this->subHT[ j ] != NULL )
                j = ( j + 1 ) & ( this->htSize - 1 );
              this->subHT[ j ] = p;
            }
          }
          this->nSubs--;
          return sub;
        }
      }
    }
    return NULL;
  }
};


struct RaiSub2 : public RaiMsgCallback, public RaiDataLossCallback,
                public RaiTimerCallback, public RaiSub2HT, public rai::Thread {
  RaiApi            * api;      /* the api handle */
  RaiDict           * dataDict; /* dictionary loader */
  RaiSession        * session;  /* a session */
  RaiQueue          * queue;    /* a queue for message and timer events */
  RaiTimer          * rateTimer, /* a timer used for -rate calculations */
                    * rotateTimer; /* a timer used to rotate -save output */
  rai::InputStream  * in;       /* subjects read from here */
  rai::OutputStream * out,      /* messages written here */
                    * saveOut;  /* messages written in replay format */
  rai::Mutex        * lock;     /* onMsg lock for stats */
  char              * outName,  /* file name of above streams */
                    * saveName,
                    * inName,
                    * subjname; /* first subject arg */
  const char        * inSource; /* source name of subjects */
  Rai_u64           * msgsLat, /* map of latency vals < 1 sec */
                      latencyOverrun, /* latency >= 1 sec */
                      latencyCnt,     /* count messages in cumLatencySum */
                      cumLatencySum,  /* cumulative latency in usecs */
                      cumLatencyMin,  /* latency minimum */
                      cumLatencyMax;  /* latency maximum */
  unsigned int        timeout,  /* if > 0, then timeout subscription starts */
                      msgWaitCount;  /* if > 0, then wait for N messages */
  Rai_u64             msgEventCount, /* how many message events */
                      msgByteCount,  /* how message bytes */
                      msgTimeoutCount, /* how subscription timeouts */
                      subCount,        /* how many subscription starts */
                      unsubCount;   /* how many subscription stops */
  rai::TimeHires      lastTime,     /* interval time since last timer event */
                      startTime;    /* first time */
  Rai_u64             lastMsgCount, /* interval stats, for rate calculations */
                      lastByteCount,
                      lastSubCount,
                      lastUnsubCount;
  rai::TimeRotate     fileRotate;
  rai::TimeNSecs      baseTime;
  RaiMsg            * snapMsg;
  bool                quit,
                      doRetry,    /* if subscriptions timeout, retry them */
                      doQuiet,    /* don't be chatty, just subscribe */
                      doSnapshot, /* snapshot subscriptions */
                      doListen,   /* listen subscriptions (just updates) */
                      doWait,     /* wait after all snapshots have been recvd */
                      doSched,    /* use subscribe and unsubscribe script */
                      doLatency,  /* use msgsLat[] to track latency */
                      doRate;     /* print the rate of messages */
  static const unsigned int MAX_LAT = 1000000; /* 100ms */
  #define MIN_INIT_LAT 9999999
  
  RaiSub2() : rai::Thread( "raisub2-dispatch" ),
              api( 0 ), dataDict( 0 ), session( 0 ), queue( 0 ), rateTimer( 0 ),
              rotateTimer( 0 ), in( 0 ), out( 0 ), saveOut( 0 ), lock( 0 ),
              outName( 0 ), saveName( 0 ), inName( 0 ), subjname( 0 ),
              inSource( 0 ), msgsLat( 0 ), latencyOverrun( 0 ), latencyCnt( 0 ),
              cumLatencySum( 0 ), cumLatencyMin( MIN_INIT_LAT ),
              cumLatencyMax( 0 ), timeout( 3000 ), msgWaitCount( 0 ),
              msgEventCount( 0 ), msgByteCount( 0 ), msgTimeoutCount( 0 ),
              subCount( 0 ), unsubCount( 0 ), lastTime( 0 ), startTime( 0 ),
              lastMsgCount( 0 ), lastByteCount( 0 ), lastSubCount( 0 ),
              lastUnsubCount( 0 ), snapMsg( 0 ),
              quit( false ), doRetry( false ), doQuiet( false ),
              doSnapshot( false ), doListen( false ), doWait( false ),
              doSched( false ), doLatency( false ), doRate( false ),
              subthr( *this ) {
    this->baseTime = rai::Time::currentTimeNanosecs(); /* -save time offset */
    this->fileRotate.setLastTime( rai::Time::currentTimeMillisecs() );
  }
  ~RaiSub2() {
    this->releaseHT();
    if ( this->dataDict != NULL )
      delete this->dataDict;
    if ( this->rateTimer != NULL )
      delete this->rateTimer;
    if ( this->rotateTimer != NULL )
      delete this->rotateTimer;
    if ( this->queue != NULL )
      delete this->queue;
    if ( this->session != NULL )
      delete this->session;
    if ( this->outName != NULL )
      FREE( this->outName );
    if ( this->saveName != NULL )
      FREE( this->saveName );
    if ( this->inName != NULL )
      FREE( this->inName );
    if ( this->subjname != NULL )
      FREE( this->subjname );
    if ( this->msgsLat != NULL )
      FREE( this->msgsLat );
    if ( this->out != NULL && this->out != rai::Sys::out )
      delete this->out;
    if ( this->in != NULL && this->in != rai::Sys::in )
      delete this->in;
    if ( this->saveOut != NULL )
      delete this->saveOut;
    if ( this->lock != NULL )
      delete this->lock;
    if ( this->api != NULL )
      delete this->api;
    if ( this->snapMsg != NULL )
      delete this->snapMsg;
  }

  /* RaiDataLossCallback, connection success event, log it */
  virtual void onConnection( RaiConnectionEvent &event,  void *cl ) {
    this->api->PrintLog( LMINOR, "%s", event.description );
  }

  /* RaiDataLossCallback, log it then notify subscriptions */
  virtual void onDataLoss( RaiDataLossEvent &event,  void *cl ) {
    RaiException e = RaiApiErr::getErr( RaiApiErr::TSPT_DATALOSS );
    this->api->PrintLog( LERROR, e, "%s", event.description );
    /* notify subscriptions if connection oriented and no secondary exists */
    if ( event.connectionLoss && event.connectionCount == 0 ) {
      try {
        this->session->NotifyStatus( rai::SassConst::TRANSIENT,
                                     rai::SassConst::STATUS_TPT_DISCONNECTED );
      } catch ( RaiException e ) {
        this->api->PrintLog( LERROR, e, "NotifyStatus" );
      }
    }
  }
  /* RaiMsgCallback, print the message */
  virtual void onMsg( RaiMsgEvent &event,  RaiMsg &raiMsg,  void *closure ) {
    rai::TimeNSecs ns = ( event.pubTime != 0 || event.routeTime != 0 ) ?
                        rai::Time::currentTimeNanosecs() : 0;
    unsigned int nBytes = 0;
    try {
      /* track latencies and sumerize at program exit */
      if ( ns != 0 && this->msgsLat != NULL ) {
        if ( event.routeTime != 0 ) {
          rai::TimeUSecs i = ( ns - event.routeTime ) / 100;
          nBytes = raiMsg.PackSize();
          this->lock->lock();
          if ( i < MAX_LAT ) {
            this->msgsLat[ i ]++;
            this->cumLatencySum += i;
            this->latencyCnt++;
            if ( i < this->cumLatencyMin )
              this->cumLatencyMin = i;
            if ( i > this->cumLatencyMax )
              this->cumLatencyMax = i;
          }
          else {
            this->latencyOverrun++; /* may be cached messages, not deltas */
          }
          this->msgEventCount++;
          this->msgByteCount += nBytes;
          this->lock->unlock();
        }
      }
      /* print the message */
      if ( ! this->doQuiet ) {
        char         ts[ 32 ];
        const char * s = event.SubscribedSubject();
        this->lock->lock();
        try {
          this->out->printf( "## Subject %s (old state=%s,new=%s)", s,
                            RaiSubscribe::StateToString( event.oldState ),
                            RaiSubscribe::StateToString( event.state ) );
          /* if event.subject is inbox name, or subscribed subject is wildcard
           * print the event.subject */
          if ( ::strcmp( s, event.subject ) != 0 )
            this->out->printf( " (%s)", event.subject );
          if ( event.counter != 0 || event.oldCounter != 0 ) {
            this->out->printf( " cnt=%u", event.counter ); /* msg update count*/
            if ( event.oldCounter + 1 != event.counter &&
                 event.oldCounter != 0 ) /* display if different than count+1 */
              this->out->printf( " old cnt=%u", event.oldCounter );
          }
          this->out->puts( "\n" );
          /* if message is timestamped, print times and latencies */
          if ( ns != 0 ) {
            this->out->printf( "# Receive %s\n", 
                  rai::Time::timestamp( ns, 7, ts, sizeof( ts ) ) );
            if ( event.pubTime != 0 )
              this->out->printf( "# Publish %s (lat=%.5fms)\n", 
                rai::Time::timestamp( event.pubTime, 7, ts, sizeof( ts ) ),
                (double) ( (llong) ( ns - event.pubTime ) ) / 1000000.0 );
            if ( event.routeTime != 0 )
              this->out->printf( "# Route   %s (lat=%.5fms)\n",
                rai::Time::timestamp( event.routeTime, 7, ts, sizeof( ts ) ),
                (double) ( (llong) ( ns - event.routeTime ) ) / 1000000.0 );
          }
          raiMsg.Print( this->out );
          this->out->flush();
          this->lock->unlock();
        } catch ( ... ) {
          this->lock->unlock();
          throw;
        }
      }
      if ( this->saveOut != NULL ) {
        /* if message is not internally generated, save it to replay file */
        if ( event.recStatus != rai::SassConst::STATUS_TIMEOUT &&
             event.msgType != rai::SassConst::SERVICE_STATUS ) {
          this->lock->lock();
          try {
            RaiMsg_size size = raiMsg.PackSize();
            const char * s = event.subject; /* use event.subject unless inbox */
            if ( s[ 0 ] == '_' && s[ 1 ] == 'I' &&
                 ::strncmp( s, "_INBOX", 6 ) == 0 )
              s = event.SubscribedSubject();
            if ( ns == 0 )
              ns = rai::Time::currentTimeNanosecs();
            if ( this->baseTime > ns )
              this->baseTime = ns;
            this->saveOut->printf( "%s\n%u %.6f\n", s, size,
                             (double) ( ns - this->baseTime ) / 1000000000.0 );
            this->saveOut->writeBytes( (const byte *) raiMsg.Packed(), size );
            this->saveOut->flush();
            this->lock->unlock();
          } catch ( RaiException e ) {
            this->lock->unlock();
            this->api->PrintLog( LERROR, e, "Saving message to \"%s\"",
                                 this->saveName );
            throw;
          }
        }
      }
      if ( event.state == RaiSubscribe::STATE_STALE ||
           event.recStatus == rai::SassConst::STATUS_TIMEOUT ) {
        if ( this->doRetry ) {
          this->api->PrintLog( LMINOR, "Refreshing subject, %s: \"%s\"",
             ( event.state == RaiSubscribe::STATE_STALE ? "stale" : "timeout" ),
               event.SubscribedSubject() );
          event.subscribe.Refresh( this->timeout );
          return; // Don't increment msgEventCount
        }
        this->lock->lock();
        this->msgTimeoutCount++;
        this->lock->unlock();
        this->api->PrintLog( LNORMAL, NULL, "Subject %s: \"%s\"",
             ( event.state == RaiSubscribe::STATE_STALE ? "stale" : "timeout" ),
               event.SubscribedSubject() );
        return;
      }
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "Printing message" );
      this->out->flush();
    }
    if ( nBytes == 0 ) {
      nBytes = raiMsg.PackSize();
      this->lock->lock();
      /*rai::AtomicULong::addv( &this->msgEventCount, 1 ); */
      this->msgEventCount++;
      /*rai::AtomicULong::addv( &this->msgByteCount, nBytes ); */
      this->msgByteCount += nBytes;
      this->lock->unlock();
    }
  } 
  
  /* RaiTimerCallback, print the message rate (if -rate) */
  virtual void onTimer( RaiTimer &timer,  void *closure ) {
    if ( &timer == this->rateTimer ) {
      double       interval;
      Rai_u64      msgs,
                   bytes,
                   count;
      double       cpms;
      unsigned int subs,
                   unsubs,
                   cnt;
      rai::TimeHires curTime = rai::Time::getHiresTime( &cpms );

      interval             = (double) ( curTime - this->lastTime ) /
                             (double) cpms / 1000.0;
      this->lastTime       = curTime;
      count                = this->msgEventCount;
      msgs                 = count - this->lastMsgCount;
      this->lastMsgCount   = count;
      count                = this->msgByteCount;
      bytes                = count - this->lastByteCount;
      this->lastByteCount  = count;
      cnt                  = this->subCount;
      subs                 = cnt - this->lastSubCount;
      this->lastSubCount   = cnt;
      cnt                  = this->unsubCount;
      unsubs               = cnt - this->lastUnsubCount;
      this->lastUnsubCount = cnt;

      this->lock->lock();
      try {
        this->out->printf(
            "%6.1f sub/s %6.1f unsub/s %8.1f msg/s %8.1f kb/s %8.2f mbit/s\n",
                          (double) subs / interval,
                          (double) unsubs / interval,
                          (double) msgs / interval,
                          (double) bytes / 1024.0 / interval,
                          (double) bytes * 8.0 / 1000.0 / 1000.0 / interval );
        this->out->flush();
      } catch ( ... ) {
      }
      this->lock->unlock();
    }
    else if ( &timer == this->rotateTimer && this->fileRotate.checkRotate() ) {
      char buf1[ 64 ], tsbuf[ 64 ];
      rai::TimeMSecs nextRotate, diffTime;
      try {
        this->renameSaveFile();

        rai::OutputStream *tmpOut = this->saveOut;
        rai::OutputStream *newOut =
          rai::FileOutputStream::open( this->saveName );

        nextRotate = this->fileRotate.nextRotate( &diffTime );
        this->api->PrintLog( LMINOR,
              "File \"%s\" rotate: interval: %s; next: %s", this->saveName,
              rai::StrUtil::intToString( this->fileRotate.getInterval(),
                            buf1, sizeof( buf1 ), rai::U_MILLISECS, false ),
              rai::Time::timestamp( nextRotate, tsbuf, sizeof( tsbuf ) ) );

        this->lock->lock();
        this->baseTime = rai::Time::currentTimeNanosecs();
        this->saveOut = newOut;
        this->lock->unlock();

        tmpOut->close();
        delete tmpOut;
      } catch ( RaiException e ) {
        this->api->PrintLog( LERROR, e, "Rotate file \"%s\"", this->saveName );
        throw e;
      }
    }
  }

  void renameSaveFile( void ) {
    char saveName2[ 1024 ];

    ::strncpy( saveName2, this->saveName, sizeof( saveName2 ) );

    unsigned int olen = ::strlen( saveName2 );
    rai::TimeMSecs curTime = rai::Time::currentTimeMillisecs();
    rai::Time::strftime( rai::Time::TZ_LOCAL_TIME, curTime, ".%Y-%m-%d_%H-%M-%S",
                    &saveName2[ olen ], sizeof( saveName2 ) - olen );
    rai::File::renameFile( this->saveName, saveName2 );
  }

  /* get arg values for subs, dictionary, setup timers and in/out streams */
  bool init( RaiApi *apip,  rai::Args &args ) {
    this->api = apip;
    try {
      RaiSub2Args subargs;
      rai::TimeMSecs diffTime, nextRotate, rotateIval;
      bool noDictionary = args.getBoolean( subargs.noDict_arg.name );

      this->lock = rai::Mutex::create();
      STRDUP( this->subjname, args.getString( subargs.subject_arg.name ) );
      STRDUP( this->outName, args.getString( subargs.output_arg.name ) );
      STRDUP( this->saveName, args.getString( subargs.save_arg.name ) );
      STRDUP( this->inName, args.getString( subargs.input_arg.name ) );
      this->doSnapshot   = args.getBoolean( subargs.snap_arg.name );
      this->doListen     = args.getBoolean( subargs.listn_arg.name );
      this->doWait       = args.getBoolean( subargs.wait_arg.name );
      this->doSched      = args.getBoolean( subargs.sched_arg.name );
      this->doLatency    = args.getBoolean( subargs.latency_arg.name );
      this->doRate       = args.getBoolean( subargs.rate_arg.name );
      this->msgWaitCount = args.getUInt( subargs.msgCnt_arg.name );
      rotateIval = (rai::TimeMSecs) (
                 args.getDouble( subargs.rotateIval_arg.name ) * 1000.0 + 0.5 );
      this->fileRotate.setRotateTime(
                  args.getString( subargs.rotateTime_arg.name ) );
      this->fileRotate.setRotatePeriod( NULL, rotateIval );
      this->fileRotate.setLastTime( rai::Time::currentTimeMillisecs() );
      nextRotate = this->fileRotate.nextRotate( &diffTime );

      if ( this->saveName != NULL ) {
        try {
          if ( nextRotate != 0 && rai::File::fileExists( this->saveName ) )
            this->renameSaveFile();
          this->saveOut = rai::FileOutputStream::open( this->saveName );
        } catch ( RaiException e ) {
          this->api->PrintLog( LERROR, e, "Opening \"%s\"", this->saveName );
          throw e;
        }
      }
      if ( this->outName != NULL ) {
        try {
          this->out = rai::FileOutputStream::open( this->outName );
        } catch ( RaiException e ) {
          this->api->PrintLog( LERROR, e, "Opening \"%s\"", this->outName );
          throw e;
        }
      }
      if ( this->out == NULL )
        this->out = rai::Sys::out;

      RaiApi::OpenLog( args );
      /* if cfilePath specified on the command line */
      if ( ! noDictionary )
        noDictionary = RaiApi::OpenDict( args );
      this->api->ParseArgs( args );

      this->timeout = (unsigned int)
                      ( args.getDouble( subargs.timeout_arg.name ) * 1000.0 );
      this->doRetry = args.getBoolean( subargs.retry_arg.name );
      this->doQuiet = args.getBoolean( subargs.quiet_arg.name );
      this->setSnapMsg( args.getString( subargs.snapMsg_arg.name ) );
      this->session = this->api->CreateSession();
      this->session->SetDataLossCB( this );
      this->session->Start();

      /* resolve dictionary */
      if ( ! noDictionary ) {
        this->dataDict = this->session->CreateDict();
        this->dataDict->Load( 5, NULL, false );
        while ( this->dataDict->InProgress() && ! this->quit )
          ;
        if ( this->quit )
          return false;
        if ( ! this->dataDict->HaveDict() ) {
          for (;;) {
            this->dataDict->Load( 10, NULL, false );
            while ( this->dataDict->InProgress() && ! this->quit )
              ;
            if ( this->quit )
              return false;
            if ( this->dataDict->HaveDict() )
              break;
            this->api->PrintLog( LMINOR, "Dictionary load timed out, retrying");
            this->out->printf( "Dictionary load timed out, retrying\n" );
            this->out->flush();
          }
        }
        if ( ! doQuiet ) {
          this->out->printf( "Dictionary received\n" );
          this->out->flush();
        }
      }

      if ( this->subjname[ 0 ] != '-' ) {
        unsigned int i, n, dataLen;
        byte *data;
        rai::ByteArrayOutputStream bout( NULL, 1024, true );
        bout.printf( "%s\n", this->subjname );
        n = args.getNumValues( subargs.subject_arg.name );
        for ( i = 1; i < n; i++ )
          bout.printf( "%s\n", args.getString( subargs.subject_arg.name, i ) );
        bout.flush();
        dataLen = bout.getData( &data );
        bout.reset();
        this->in = NEW rai::ByteArrayInputStream( data, dataLen, false, true );
        this->inSource = "cmdline";
      }
      else if ( this->inName != NULL ) {
        try {
          this->in = rai::FileInputStream::open( this->inName );
          this->inSource = this->inName;
        } catch ( RaiException e ) {
          this->api->PrintLog( LERROR, e, "Opening \"%s\"", this->inName );
          throw e;
        }
      }
      else {
        this->in = rai::Sys::in;
        this->inSource = "stdin";
      }
      /* create queue, direct = true will cause messages to be dispatched
       * directly from the network instead of the queue */
      this->queue = this->session->CreateQueue(
                                   args.getBoolean( subargs.direct_arg.name ) );
      if ( this->doRate ) {
        /* print message rate every rateIval secs in a timer */
        this->rateTimer = this->queue->CreateTimer( this );
        this->rateTimer->SetInterval( (rai::TimeMSecs) (
          args.getDouble( subargs.rateIval_arg.name ) * 1000.0 ) );
        this->lastTime = rai::Time::getHiresTime();
        this->startTime = this->lastTime;
        this->rateTimer->Start();
      }
      if ( this->saveOut != NULL && nextRotate != 0 ) {
        char buf1[ 64 ], tsbuf[ 64 ];
        this->api->PrintLog( LMINOR,
              "File \"%s\" rotate: interval: %s; next: %s", this->saveName,
              rai::StrUtil::intToString( this->fileRotate.getInterval(),
                            buf1, sizeof( buf1 ), rai::U_MILLISECS, false ),
              rai::Time::timestamp( nextRotate, tsbuf, sizeof( tsbuf ) ) );
        this->rotateTimer = this->queue->CreateTimer( this );
        this->rotateTimer->SetInterval( 10000 );
        this->rotateTimer->Start();
      }
      if ( this->doLatency ) {
        MALLOC( MAX_LAT * sizeof( this->msgsLat[ 0 ] ), &this->msgsLat );
        ::memset( this->msgsLat, 0, MAX_LAT * sizeof( this->msgsLat[ 0 ] ) );
      }
      return true;
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "Not initialized, stopped" );
    }
    return false;
  }

  void setSnapMsg( const char *str ) {
    if ( str != NULL ) {
      rai::StringToMsg strToMsg;
      this->snapMsg = NEW RaiMsg( RAIMSG_PROTO );
      strToMsg.addFields( *this->snapMsg, str );
    }
  }

  /* start a subscribe to a subject */
  bool subscribe( const char *subject,  RaiSubscribe::RaiSubParameter parm ) {
    try {
      RaiSubscribe * newSub = this->queue->CreateSubscribe( this );
      if ( this->snapMsg != NULL )
        newSub->SetExtra( this->snapMsg );
      if ( ! this->doQuiet ) {
        this->out->printf( "%s subject %s\n",
                  parm == RaiSubscribe::SNAP ? "Snapshot" :
                  parm == RaiSubscribe::UPDATE ? "Listening" : "Starting",
                  subject );
        this->out->flush();
      }
      this->addHT( subject, newSub );
      newSub->Start( subject, parm, this->timeout );
      this->subCount++;
      return true;
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "Subscribe subject %s", subject );
    }
    return false;
  }

  /* stop a previously started subscribe */
  bool unsubscribe( const char *subject ) {
    try {
      RaiSubscribe * sub = this->removeHT( subject );
      if ( sub == NULL )
        return false;
      if ( ! this->doQuiet ) {
        this->out->printf( "Unsubscribe subject %s\n", subject );
        this->out->flush();
      }
      sub->Cancel();
      delete sub;
      this->unsubCount++;
      return true;
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "Unsubscribe subject %s", subject );
    }
    return false;
  }

  void finalLatency( void ) {
    double       av;
    unsigned int avg, j, stdDev[ 4 ];
    Rai_u64      cnt,
                 stdDevMsgs[ 4 ];
    if ( this->latencyCnt == 0 )
      av = 0;
    else
      av = this->cumLatencySum / (double) this->latencyCnt;
    avg = (unsigned int) av;
    if ( avg >= MAX_LAT )
      avg = MAX_LAT - 1;
    cnt             = this->msgsLat[ avg ];
    stdDev[ 0 ]     = 1;
    stdDevMsgs[ 0 ] = 0;
    stdDevMsgs[ 1 ] = (Rai_u64) ( this->latencyCnt * 0.682 );
    stdDevMsgs[ 2 ] = (Rai_u64) ( this->latencyCnt * 0.955 );
    stdDevMsgs[ 3 ] = (Rai_u64) ( this->latencyCnt * 0.997 );

    for ( j = 1; j < 4; j++ ) {
      for ( stdDev[ j ] = stdDev[ j - 1 ];
            stdDev[ j ] < MAX_LAT && cnt < stdDevMsgs[ j ]; stdDev[ j ]++ ) {
        if ( stdDev[ j ] <= avg )
          cnt += this->msgsLat[ avg - stdDev[ j ] ];
        if ( avg + stdDev[ j ] < MAX_LAT )
          cnt += this->msgsLat[ avg + stdDev[ j ] ];
      }
    }
    this->api->PrintLog( LNORMAL, NULL, "av=%.06f min=%.06f max=%.06fms "
                      "stddev=(%.06f,%.06f,%.06f) (68.2%%,95.5%%,99.7%%)",
                 av / 10000.0, (double) this->cumLatencyMin / 10000.0,
                 (double) this->cumLatencyMax / 10000.0,
                 (double) stdDev[ 1 ] / 10000.0, (double) stdDev[ 2 ] / 10000.0,
                 (double) stdDev[ 3 ] / 10000.0 );
    this->api->PrintLog( LNORMAL, NULL, "latency overruns: %qu > %ums",
                       this->latencyOverrun, MAX_LAT / 1000 / 10 );
  }

  void finalRate( void ) {
    double       interval;
    Rai_u64      msgs,
                 bytes;
    double       cpms;
    unsigned int subs,
                 unsubs;
    rai::TimeHires curTime = rai::Time::getHiresTime( &cpms );

    interval             = (double) ( curTime - this->startTime ) /
                           (double) cpms / 1000.0;
    msgs                 = this->msgEventCount;
    bytes                = this->msgByteCount;
    subs                 = this->subCount;
    unsubs               = this->unsubCount;

    this->lock->lock();
    try {
      this->out->printf( "\nTotal rate:\n"
          "%6.1f sub/s %6.1f unsub/s %8.1f msg/s %8.1f kb/s %8.2f mbit/s\n",
                        (double) subs / interval,
                        (double) unsubs / interval,
                        (double) msgs / interval,
                        (double) bytes / 1024.0 / interval,
                        (double) bytes * 8.0 / 1000.0 / 1000.0 / interval );
      this->out->flush();
    } catch ( ... ) {
    }
    this->lock->unlock();
  }

  /* close everything */
  void close( void ) {
    if ( this->doRate )
      this->finalRate();
    if ( this->doLatency )
      this->finalLatency();
    this->quit = true;
    if ( this->rateTimer != NULL )
      this->rateTimer->Stop();
    if ( this->rotateTimer != NULL )
      this->rotateTimer->Stop();
    if ( ! this->subthr.isThreadJoined() )
      this->subthr.join();
    for ( unsigned int i = 0; i < this->htSize; i++ ) {
      if ( this->subHT[ i ] != NULL )
        this->subHT[ i ]->Cancel();
    }
    if ( this->queue != NULL )
      this->queue->Destroy();
    if ( this->session != NULL )
      this->session->Destroy();
    if ( this->api != NULL )
      this->api->Close();
  }

  /* dispatch queue events */
  void dispatchLoop( void ) {
    while ( ! this->quit ) {
      try {
        this->queue->TimedDispatch( 100 );
      } catch ( RaiException e ) {
        this->api->PrintLog( LERROR, e, "In dispatchLoop" );
      }
    }
  }

  /* thread from service for dispatch loop */
  void serviceRun( void ) {
    this->subthr.start();
    this->dispatchLoop();
  }

  /* thread from main() for dispatch loop */
  virtual void run( void ) {
    this->dispatchLoop();
  }

  struct SubThread : public rai::Thread {
    RaiSub2 &me;

    SubThread( RaiSub2 &m ) : rai::Thread( "raisub2-subscribe" ), me( m ) {}

    virtual void run( void ) {
      try {
        this->me.subLoop();
      } catch ( RaiException e ) {
        this->me.api->PrintLog( LERROR, e, "In subLoop" );
      }
    }
  } subthr;

  /* Subscribe subjects, then wait for quit (ctrl-c) or until message 
   * count received */
  void subLoop( void ) {
    RaiSubscribe::RaiSubParameter parm =
      this->doSnapshot ? RaiSubscribe::SNAP :
      this->doListen   ? RaiSubscribe::UPDATE : RaiSubscribe::BOTH;
    unsigned int n, count = 0, missSub = 0, missUnsub = 0;
    char         buf[ rai::SassConst::MAX_SUBJECT_LEN ];

    if ( ! this->doSched ) {
      /* subscribe or snap subjects are read fron stdin */
      for ( count = 0; ! this->quit; ) {
        n = this->in->gets( buf, sizeof( buf ) );
        if ( n == 0 ) /* end of input */
          break;
        /* strip end of line whitespace */
        while ( n > 0 && ( buf[ n-1 ] == ' ' || buf[ n-1 ] == '\n' ||
                           buf[ n-1 ] == '\r' ) )
          buf[ --n ] = '\0';
        if ( n > 0 ) {
          if ( this->subscribe( buf, parm ) )
            count++;
          else
            missSub++;
        }
      }
      if ( ! this->doQuiet ) {
        this->out->printf( "%u subjects read on %s\n", count, this->inSource );
        this->out->flush();
      }
    }
    else {
      /* read "SUB subject" and "UNSUB subject" commands from input,
       * if not a command and line is an integer, sleep */
      for ( count = 0; ! this->quit; ) {
        n = this->in->gets( buf, sizeof( buf ) );
        if ( n == 0 ) /* end of input */
          break;
        /* strip end of line whitespace */
        while ( n > 0 && ( buf[ n-1 ] == ' ' || buf[ n-1 ] == '\n' ||
                           buf[ n-1 ] == '\r' ) )
          buf[ --n ] = '\0';
        if ( n > 0 ) {
          if ( ::strncmp( "SUB", buf, 3 ) == 0 &&
               ( buf[ 3 ] == ' ' || buf[ 3 ] == '=' ) ) {
            if ( this->subscribe( &buf[ 4 ], parm ) )
              count++;
            else
              missSub++;
          }
          else if ( ::strncmp( "UNSUB", buf, 5 ) == 0 &&
                    ( buf[ 5 ] == ' ' || buf[ 5 ] == '=' ) ) {
            if ( this->unsubscribe( &buf[ 6 ] ) )
              --count;
            else
              missUnsub++;
          }
          else if ( buf[ 0 ] >= '0' && buf[ 0 ] <= '9' ) {
            int i = atoi( buf );
            if ( i > 0 )
              rai::Time::sleepMillisecs( i * 1000 );
          }
        }
      }
      //if ( ! doQuiet ) {
        this->out->printf( "Done reading SUB/UNSUB commands on stdin, "
                           "waiting for %u subs\n", count );
        this->out->flush();
      //}
    }
    if ( this->msgWaitCount == 0 ) {
      if ( this->doSnapshot && ! this->doWait )
        this->msgWaitCount = this->nSubs;
    }
    if ( this->msgWaitCount > 0 ) {
      while ( ! this->quit ) {
        /* if all messages rcvd and/or timed out */
        if ( this->msgWaitCount > 0 &&
             this->msgWaitCount <= this->msgEventCount + this->msgTimeoutCount )
          this->quit = true;
        else
          rai::Time::sleepMillisecs( 100 );
      }
    }
    if ( this->doQuiet && this->msgTimeoutCount > 0 ) {
      this->api->PrintLog( LERROR, NULL, "%qu subjects timeout (%qu recv)",
                     this->msgTimeoutCount, this->msgEventCount );
    }
    if ( missSub > 0 ) {
      this->api->PrintLog( LERROR, NULL, "%u subjects did not subscribe",
                     missSub );
    }
    if ( missUnsub > 0 ) {
      this->api->PrintLog( LERROR, NULL, "%u subjects did not unsubscribe",
                     missUnsub );
    }
  }
};
} // namespace

#ifdef RAI_DLL_EMBEDDED
/* this version can be loaded into raicache */

#include "raiapi2_service.h"

/* dll entry point, determined by service file name: rai_service_raisub2.so */
extern "C" RAI_DLL_EXPORT void
RAISUB2_ServiceInitialize( void )
{
  rai::ServiceFactory * fact = NEW
    T_RaiApiServiceFactory< RaiSub2Args,
      T_RaiApiServiceProto< RaiSub2 > >( "raisub2" );
  rai::ServiceFactory::installService( fact );
}

#else
/* this is cmdline main for static or dynamic loading middlewares */
static RaiSub2 * raisub2 = NULL; /* for signal handlers to set quit = true */

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
extern "C" void myInterruptHandler( int );
#else
extern "C" BOOL myCtrlHandler( DWORD );
#endif

int
main( int argc, char *argv[] )
{
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  struct sigaction nsa;

  /* do this before threads are created so that they inherit the sigs */
  ::memset( &nsa, 0, sizeof( nsa ) );
  ::sigemptyset( &nsa.sa_mask );
  nsa.sa_handler = ::myInterruptHandler;
  ::sigaction( SIGHUP, &nsa, NULL );
  ::sigaction( SIGINT, &nsa, NULL );
  ::sigaction( SIGTERM, &nsa, NULL );
#else
  ::SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ::myCtrlHandler, TRUE );
#endif
  RaiApi    * api = NULL;
  rai::Args   args;
  RaiSub2Args subargs;

  /* initialize rai::Time, rai::Sys::in, rai::Sys::out, rai::Sys::err,
   * and calls rai::Hash32::selftest() */
  rai::Sys::initialize();
  /* open log to stderr in case command line fails to parse, it may open
   * again if -log is specified on command line */
  rai::Log::openLog( "-", rai::Log::LVL_MINOR, 4 );

  try {
    /* Open the api type from the command line, looks for -api <name> in
     * argc/argv[] and loads that middleware.  Program could also pass "tibrv"
     * or some other api name in the first argument.  If neither are specfied
     * then the default api is loaded (capr) */
    api = RaiApi::RaiOpen( NULL, argc, argv );
    /* get the api's configuration arguments */
    api->GetArgs( args );
    /* get the subject and settings for the program */
    subargs.getArgs( args );
    /* get the arguments for the dictionary, useful for parsing dict files
     * locally in the filesystem instead of receiving it on the network */
    RaiApi::GetDictArgs( args );
    /* get the logging, version, help, rc arguments and sets error output */
    args.addDefaults( api->RaiVersion(), "rai_", rai::Sys::err, argv[ 0 ] );

    try {
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
      if ( args.processArgs( argc, argv ) ) {
        raisub2 = new RaiSub2();
        /* create api elements and start the dispatch thread */
        if ( raisub2->init( api, args ) ) {
          raisub2->start(); /* start thread dispatching queue */
          /* subscribes */
          raisub2->subLoop();
          /* either done or waiting for signal */
          raisub2->join(); /* join mainloop dispatch thread */
        }
        /* stop all the subscribes, if any, close the api */
        raisub2->close();
        delete raisub2;
      }
    } catch ( RaiException e ) {
      logError( LERROR, e, "Main" );
    }
  } catch ( RaiException e ) {
    logError( LERROR, e, "Unable to load Rai API" );
  }
  rai::Log::closeLog();
  rai::Sys::terminate();

  return 0;
}

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
void
myInterruptHandler( int sig ) {
  if ( sig == SIGHUP )
    return;
  rai::Sys::err->printf( "Caught signal %d event, shutting down\n", sig );
  if ( raisub2 == NULL )
    exit( 1 );
  raisub2->quit = true;
}
#else
BOOL
myCtrlHandler( DWORD fdwCtrlType )
{
  const char *s = NULL;
  switch ( fdwCtrlType ) {
    case CTRL_C_EVENT:      s = "ctrl-c"; break;
    case CTRL_CLOSE_EVENT:  s = "close"; break;
    case CTRL_BREAK_EVENT:  s = "ctrl-break"; break;
    case CTRL_LOGOFF_EVENT: s = "logoff"; break;
    case CTRL_SHUTDOWN_EVENT:
    default:                s = "shutdown"; break;
  }
  rai::Sys::err->printf( "Caught %s event, shutting down\n", s );
  if ( raisub2 == NULL )
    exit( 1 );
  if ( raisub2 != NULL )
    raisub2->quit = true;
  return TRUE;
}
#endif
#endif
