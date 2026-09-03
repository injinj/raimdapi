/* Copyright (c) 2013 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

/*
 * This program snaps a wildcard subject and outputs a CSV table
 * to a file on an interval.
 *
 * Configuration is as follows:
 *
 * service-msg2csv interval="5 seconds" filter="RDF.REC.*.O"
 *                 fields="TRDPRC_1 TRDVOL_1 TRDTIM_MS" output="msg.csv" />
 *
 * Every 5 seconds, a msg.csv.tmp file is created and filled with values
 * resulting from a snapshot of RDF.REC.*.O.  Then it renames the msg.csv.tmp
 * file to msg.csv so that msg.csv is always the last result and will be a
 * complete result.
 *
 * It looks like this:
 *
 * SUBJECT, TRDPRC_1, TRDVOL_1, TRDTIM_MS
 * "RDF.REC.AGII.O", 29.81, 100.0, 66087033.0
 * "RDF.REC.ACNB.O", 14.91, 100.0, 71716300.0
 * "RDF.REC.AAME.O", 1.82, 100.0, 70127353.0
 * "RDF.REC.AFFM.O", 1.45, 100.0, 64599633.0
 * "RDF.REC.AERL.O", 6.089, 100.0, 71767870.0
 *
 * The msg.csv file may be empty if no subjects exist.  If a message is missing
 * one of the fields requested, it will be ignored.  If a message has a
 * MSG_TYPE field with a TRANSIENT or a DROP, then it is also ignored.
 */

/* for keyboard interrupt (ctrl-c) */
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <signal.h>
#else
#include <windows.h>
#endif

#include <string.h>
#include <math.h>

#include "raiapi2.h"
#include "msg/subject.h"
#include "util/hash_util.h"
#include "util/str_util.h"
#include "base/thread.h"
#include "base/file.h"
#include "stream/file_stream.h"

namespace {

static const char INPUT_STR[]  = "input",
                  OUTPUT_STR[] = "output",
                  FILTER_STR[] = "filter",
                  FIELDS_STR[] = "fields",
                  IVAL_STR[]   = "interval";

struct Msg2CsvArgs {
  const rai::StringArg input;
  const rai::StringArg output;
  const rai::StringArg filter;
  const rai::StringArg fields;
  const rai::DoubleArg ival;

  Msg2CsvArgs() :
      input(   INPUT_STR, "-", "<file>",
                    "File to read messages from (- for stdin)" ),
      output(  OUTPUT_STR, "-", "<file>",
                    "File to write output to (- for stdout)" ),
      filter(  FILTER_STR, NULL, "<subject>",
                    "Filter messages with a wildcard, for example TEST.>" ),
      fields(  FIELDS_STR, NULL, "<name> ...",
                    "The field names to output" ),
      ival( IVAL_STR, 0.0, "<time>", "Time between subject snapshots" ) {}

  void getArgs( rai::Args &args ) const throw( RaiException ) {
    args.add( &input );
    args.add( &output );
    args.add( &filter );
    args.add( &fields, rai::COMMAND_ARG | rai::RESOURCE_ARG | rai::LIST_ARG );
    args.add( &ival, rai::COMMAND_ARG | rai::RESOURCE_ARG | rai::TIME_SEC_ARG );
  }
};

/* SEP separates the csv values, NL terminates an entry */
static const byte SEP[] = { ',', ' ' },
                  NL[]  = { '\n' };

/* A csv column */
struct FieldBuf {
  FieldBuf   * next;     /* list of csv columns */
  char       * name,     /* name of the field */
             * buf;      /* field buffer */
  unsigned int bufLen,   /* amount allocated to buf */
               len,      /* length of buffer used */
               nameSize, /* strlen( name ) + 1 */
               nameHash; /* hash of name */
  bool         hasValue; /* whether this field is present in the message */

  SYS_OPS( FieldBuf );
  FieldBuf() : next( 0 ), name( 0 ), buf( 0 ),
               bufLen( 0 ), len( 0 ), nameSize( 0 ), nameHash( 0 ),
               hasValue( false ) {}

  ~FieldBuf() {
    if ( this->name != NULL )
      FREE( this->name );
    if ( this->buf != NULL )
      FREE( this->buf );
  }
};

/* convert a stream of messages to a csv file */
struct Msg2Csv : public RaiMsgCallback, public RaiTimerCallback {
  static const unsigned int FB_HASH_SZ = 256;
  RaiApi            * api;        /* the api handle */
  RaiSession        * session;    /* the sesion handle */
  RaiQueue          * queue;      /* the queue that dispatches the timers A*/
  RaiTimer          * snapTimer;  /* when the wild matches are published */
  RaiSubscribe      * snap;       /* the snap handle */
  rai::Mutex        * lock;
  rai::InputStream  * in;
  rai::OutputStream * out;
  char              * inFile,
                    * tmpOutFile,
                    * outFile,
                    * filter;
  RaiSubject        * filterSubj;     /* filter subjects which match */
  FieldBuf          * fbList,
                    * fbLast,
                    * fbHash[ FB_HASH_SZ ];
  double              ival;
  unsigned int        fbCount;        /* how many fields are in fbList */
  bool                haveHeaderLine, /* set to true when header is output */
                      quit;

  SYS_OPS( Msg2Csv );
  Msg2Csv() : api( 0 ), session( 0 ), queue( 0 ), snapTimer( 0 ), snap( 0 ),
              lock( 0 ), in( 0 ), out( 0 ), inFile( 0 ),
              tmpOutFile( 0 ), outFile( 0 ), filter( 0 ),
              filterSubj( 0 ), fbList( 0 ), fbLast( 0 ),
              ival( 0 ), fbCount( 0 ), haveHeaderLine( false ),
              quit( false ) {
    ::memset( this->fbHash, 0, sizeof( this->fbHash ) );
  }
  virtual ~Msg2Csv() {
    if ( this->snapTimer != NULL )
      delete this->snapTimer;
    if ( this->snap != NULL )
      delete this->snap;
    if ( this->queue != NULL )
      delete this->queue;
    if ( this->session != NULL )
      delete this->session;
    if ( this->api != NULL )
      delete this->api;
    if ( this->filter != NULL )
      FREE( this->filter );
    if ( this->filterSubj != NULL )
      delete this->filterSubj;
    while ( this->fbList != NULL ) {
      FieldBuf * fb = this->fbList;
      this->fbList = fb->next;
      delete fb;
    }
    if ( this->lock != NULL )
      delete this->lock;
  }


  /* process args, open in and out files */
  bool init( RaiApi *apip,  rai::Args &args ) throw( RaiException ) {
    this->api = apip;
    try {
      STRDUP( this->inFile, args.getString( INPUT_STR ) );
      STRDUP( this->outFile, args.getString( OUTPUT_STR ) );
      /* concat .tmp to outFile */
      if ( this->outFile != NULL && this->outFile[ 0 ] != '-' ) {
        char tmpPath[ 1024 ];
        ::memset( tmpPath, 0, sizeof( tmpPath ) );
        ::strncpy( tmpPath, this->outFile, sizeof( tmpPath ) - 1 );
        ::strncpy( &tmpPath[ ::strlen( tmpPath ) ], ".tmp",
                   sizeof( tmpPath ) - ( ::strlen( tmpPath ) + 1 ) );
        STRDUP( this->tmpOutFile, tmpPath );
      }

      /* if a filter is defined */
      STRDUP( this->filter, args.getString( FILTER_STR ) );
      if ( this->filter != NULL ) {
        this->filterSubj = NEW RaiSubject();
        this->filterSubj->encode( this->filter );
      }

      try {
        if ( this->inFile == NULL || this->inFile[ 0 ] == '-' )
          this->in = rai::Sys::in;
        else
          this->in = rai::FileInputStream::open( this->inFile );
      } catch ( RaiException e ) {
        this->api->PrintLog( LERROR, e, "Input file \"%s\"", this->inFile );
        throw e;
      }
      this->api->ParseArgs( args );
      RaiApi::OpenDict( args );

      unsigned int n = args.getNumValues( FIELDS_STR );
      if ( n > 0 ) {
        for ( unsigned int i = 0; i < n; i++ ) {
          const char *name = args.getString( FIELDS_STR, i );
          this->addFieldBuf( name, ::strlen( name ) + 1 );
        }
      }

      this->ival = args.getDouble( IVAL_STR ) * 1000.0;
      /* get from snapshots */
      if ( this->ival != 0 ) {
        this->lock      = rai::Mutex::create();
        this->session   = this->api->CreateSession();
        this->session->Start();
        this->queue     = this->session->CreateQueue( true );
        this->snap      = this->queue->CreateSubscribe( this );
        this->snapTimer = this->queue->CreateTimer( this );

        this->snapTimer->SetInterval( (unsigned int) ( this->ival + 0.5 ) );
        this->snapTimer->Start(); /* execute snapshot on timer interval */
      }
      else {
        if ( ! this->openOutput() ) /* process() called */
          return false;
      }
      return true;
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "Initializing" );
    }
    return false;
  }

  bool openOutput( void ) throw( RaiException ) {
    if ( this->lock != NULL )
      this->lock->lock();

    try {
      if ( this->out != NULL ) {
        this->out->flush();
        if ( this->out != rai::Sys::out ) {
          this->out->close();
          delete this->out;
        }
        this->out = NULL;

        if ( this->tmpOutFile != NULL &&
             rai::File::fileExists( this->tmpOutFile ) )
          rai::File::renameFile( this->tmpOutFile, this->outFile );
      }
      if ( this->outFile == NULL || this->outFile[ 0 ] == '-' )
        this->out = rai::Sys::out;
      else
        this->out = rai::FileOutputStream::open( this->tmpOutFile );
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "Output file \"%s\"", this->tmpOutFile );
      this->out = NULL;
      if ( this->lock != NULL )
        this->lock->unlock();
      return false;
    }
    if ( this->lock != NULL )
      this->lock->unlock();
    return true;
  }


  /* RaiMsgCallback */
  virtual void onMsg( RaiMsgEvent &event,  RaiMsg &msg,  void *closure ) {
    try {
      if ( event.state == RaiSubscribe::STATE_STALE ||
           event.recStatus == rai::SassConst::STATUS_TIMEOUT ) {
        this->api->PrintLog( LERROR, NULL, "Timeout occured" );
        return;
      }
      switch ( event.msgType ) {
        case rai::SassConst::TRANSIENT:
        case rai::SassConst::DROP: /* ignore not found, or transient */
          break;

        default:
          this->lock->lock();
          if ( ! this->haveHeaderLine ) {
            this->outputCsvHeader( msg );
            this->haveHeaderLine = true;
          }
          this->outputCsvMsg( event.subject, msg );
          this->lock->unlock();
          break;
      }
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "onMsg" );
    }
  } 


  /* RaiTimerCallback */
  virtual void onTimer( RaiTimer &tmr,  void *closure ) {
    const char * subj = this->filter;
    if ( subj == NULL )
      subj = ">";

    if ( this->openOutput() ) {
      this->haveHeaderLine = false;
      if ( this->snap->InProgress() )
        this->snap->Refresh();
      else
        this->snap->Start( subj, RaiSubscribe::SNAP );
    }
  }

  /* lookup a field in the output csv */
  FieldBuf *getFieldBuf( const char *name,  unsigned int nameSize ) {
    unsigned int nameHash = rai::Hash32::crc_c( (const byte *) name, nameSize );
    for ( unsigned int i = nameHash % FB_HASH_SZ; ;
          i = ( i + 1 ) % FB_HASH_SZ ) {
      if ( this->fbHash[ i ] == NULL )
        return NULL;
      if ( nameSize == this->fbHash[ i ]->nameSize &&
           ::memcmp( this->fbHash[ i ]->name, name, nameSize ) == 0 )
        return this->fbHash[ i ];
    }
  }

  /* add a field to the output csv */
  void addFieldBuf( const char *name,  unsigned int nameSize ) {
    FieldBuf * fb = NEW FieldBuf();
    STRDUP( fb->name, name );
    fb->nameSize = nameSize;
    fb->nameHash = rai::Hash32::crc_c( (const byte *) name, nameSize );

    for ( unsigned int i = fb->nameHash % FB_HASH_SZ; ;
          i = ( i + 1 ) % FB_HASH_SZ ) {
      if ( this->fbHash[ i ] == NULL ) {
        this->fbHash[ i ] = fb;
        break;
      }
    }
    if ( this->fbLast == NULL )
      this->fbList = fb;
    else
      this->fbLast->next = fb;
    this->fbLast = fb;
    this->fbCount++;
    this->api->PrintLog( LMINOR, "Adding field %s to csv table", name );
  }

  /* read messages from input and write csv */
  void process( void ) throw( RaiException ) {
    char       subj[ rai::SassConst::MAX_SUBJECT_LEN ],
               size[ 80 ];
    byte     * buf = NULL;
    RaiMsg     msg;
    RaiSubject subject;
    unsigned int i, j, k, sz, bufSize = 0;

    /* read replay file input and output csv file */
    while ( ! this->quit &&
            (i = this->in->gets( subj, sizeof( subj ) )) > 0 &&
            (j = this->in->gets( size, sizeof( size ) )) > 0 &&
            (sz = atoi( size )) > 0 ) {
      if ( sz > bufSize ) {
        REALLOC( sz, &buf );
        bufSize = sz;
      }
      k = this->in->readBytes( buf, sz );
      if ( sz != k )
        break;
      while ( i > 0 && ( subj[ i - 1 ] == '\n' || subj[ i - 1 ] == '\r' ) )
        subj[ --i ] = '\0';
      subject.encode( subj );

      /* if subject matches */
      if ( this->filterSubj == NULL || this->filterSubj->matches( subject ) ) {
        msg.UnPack( buf, sz );

        switch ( getMsgType( msg ) ) {
          case rai::SassConst::TRANSIENT:
          case rai::SassConst::DROP: /* ignore not found, or transient */
            break;

          default:
            if ( ! this->haveHeaderLine ) {
              this->outputCsvHeader( msg );
              this->haveHeaderLine = true;
            }

            this->outputCsvMsg( subj, msg );
            break;
        }
      }
    }
  }

  /* get the msg type from the message, if it has one */
  static Rai_u16 getMsgType( RaiMsg &msg ) throw( RaiException ) {
    RaiField     field;
    const char * msgTypeName;
    Rai_u16      msgType = rai::SassConst::MAX_TYPE;

    if ( field.FindEx( &msg, "MSG_TYPE", sizeof( "MSG_TYPE" ) ) ) {
      switch ( field.Type() ) {
        case RAIMSG_REAL:
        case RAIMSG_UINT:
        case RAIMSG_INT:
          field.Get( msgType );
          break;
        case RAIMSG_STRING:
          field.Get( msgTypeName );
          msgType = rai::SassConst::stringToMsgType( msgTypeName );
          break;
        default:
          break;
      }
    }
    return msgType;
  }

  /* print the csv header line, SUBJECT, FIELD1, FIELD2 ... */
  void outputCsvHeader( RaiMsg &msg ) throw( RaiException ) {
    RaiField field;

    /* if no fields defined, the first message defines them */
    if ( this->fbList == NULL ) {
      if ( field.First( &msg ) ) {
        do {
          this->addFieldBuf( field.Name(), field.NameSize() );
        } while ( field.Next() );
      }
    }
    /* the first column is always the subject */
    this->out->puts( "SUBJECT" );
    for ( FieldBuf *fb = this->fbList; fb != NULL; fb = fb->next ) {
      this->out->writeBytes( SEP, sizeof( SEP ) );
      if ( fb->nameSize != 0 )
        this->out->writeBytes( (byte *) fb->name, fb->nameSize - 1 );
    }
    this->out->writeBytes( NL, sizeof( NL ) );
  }

  /* extract the field values and print the csv values */
  void outputCsvMsg( const char *subj,  RaiMsg &msg ) throw( RaiException ) {
    RaiField     field;
    FieldBuf   * fb;
    unsigned int count = 0;

    /* clear the field buffers */
    for ( fb = this->fbList; fb != NULL; fb = fb->next ) {
      fb->hasValue = false;
      fb->len = 0;
    }
    /* convert the fields to field buffers */
    if ( field.First( &msg ) ) {
      do {
        fb = this->getFieldBuf( field.Name(), field.NameSize() );
        if ( fb != NULL ) {
          const bool isNew = ! fb->hasValue;
          this->outputCsvField( field, *fb );
          fb->hasValue = true;
          if ( isNew && ++count == this->fbCount )
            break;
        }
      } while ( field.Next() );
    }

    /* if all fields present */
    if ( count == this->fbCount ) {
      /* print the csv table to the output file */
      this->out->printf( "\"%s\"", subj );
      for ( fb = this->fbList; fb != NULL; fb = fb->next ) {
        this->out->writeBytes( SEP, sizeof( SEP ) );
        if ( fb->len > 0 )
          this->out->writeBytes( (byte *) fb->buf, fb->len );
      }
      this->out->writeBytes( NL, sizeof( NL ) );
    }
  }

  void outputCsvField( RaiField &field,  FieldBuf &fb ) throw( RaiException ) {
    if ( fb.bufLen == 0 ) {
      MALLOC( 256, &fb.buf );
      fb.bufLen = 256;
    }
    switch ( field.Type() ) {
      case RAIMSG_MESSAGE:
      case RAIMSG_ARRAY:
      default:
        return;

      case RAIMSG_OPAQUE:
      case RAIMSG_PARTIAL:
      case RAIMSG_STRING: {
        char       * data = (char *) field.Data();
        unsigned int sz   = field.Size();

        /* if concatenating previous string value from multiple fields with 
         * the same name */
        if ( fb.len > 0 && fb.buf[ fb.len - 1 ] == '"' )
          fb.len--;
        else
          fb.buf[ fb.len++ ] = '"'; /* new string value */
        for ( unsigned int i = 0; i < sz; i++ ) {
          if ( data[ i ] == '\0' )
            break;
          if ( data[ i ] < ' ' || data[ i ] > '~' )
            fb.buf[ fb.len++ ] = '.';
          else if ( data[ i ] == '"' ) {
            fb.buf[ fb.len++ ] = '"';
            fb.buf[ fb.len++ ] = '"';
          }
          else {
            fb.buf[ fb.len++ ] = data[ i ];
          }
          if ( fb.len + 3 > fb.bufLen ) {
            fb.bufLen *= 2;
            REALLOC( fb.bufLen, &fb.buf );
          }
        }
        fb.buf[ fb.len++ ] = '"';
        return;
      }
      case RAIMSG_REAL: {
        double d;
        char * end;

        field.Get( d );
        rai::StrUtil::floatToString( d, &fb.buf[ fb.len ], fb.bufLen - fb.len,
                        rai::StrUtil::UNTIL_ZERO, rai::U_DECIMAL, false, &end );
        fb.len = (unsigned int) ( end - fb.buf );
        return;
      }
      case RAIMSG_BOOLEAN:
      case RAIMSG_INT:
      case RAIMSG_UINT:
      case RAIMSG_IPDATA:
        field.Convert( fb.buf, fb.bufLen );
        fb.len = ::strlen( fb.buf );
        return;
    }
  }

  void close( void ) throw( RaiException ) {
    this->quit = true;

    if ( this->in != rai::Sys::in ) {
      this->in->close();
      delete this->in;
    }
    this->in = NULL;

    if ( this->lock != NULL )
      this->lock->lock();

    this->out->flush();
    if ( this->out != rai::Sys::out ) {
      this->out->close();
      delete this->out;
    }
    this->out = NULL;
    if ( this->tmpOutFile != NULL &&
         rai::File::fileExists( this->tmpOutFile ) )
      rai::File::renameFile( this->tmpOutFile, this->outFile );

    if ( this->lock != NULL )
      this->lock->unlock();
  }


  /* dispatch queue events */
  void dispatchLoop( void ) {
    if ( this->queue == NULL )
      this->process(); /* read input and write output */
    else {
      while ( ! this->quit ) { /* execute snapshots */
        try {
          this->queue->TimedDispatch( 100 );
        } catch ( RaiException e ) {
          this->api->PrintLog( LERROR, e, "In dispatchLoop" );
        }
      }
    }
  }

  /* thread from service for dispatch loop */
  void serviceRun( void ) {
    try {
      this->dispatchLoop();
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "Processing" );
    }
  }
};
}

#ifdef RAI_DLL_EMBEDDED
/* this version can be loaded into raicache */

#include "raiapi2_service.h"

/* dll entry point, determined by service file name: rai_service_msg2csv.so */
extern "C" RAI_DLL_EXPORT void
MSG2CSV_ServiceInitialize( void ) throw( RaiException )
{
  rai::ServiceFactory * fact = NEW
    T_RaiApiServiceFactory< Msg2CsvArgs,
      T_RaiApiServiceProto< Msg2Csv > >( "msg2csv" );
  rai::ServiceFactory::installService( fact );
}

#else
/* this is cmdline main for static or dynamic loading middlewares */
Msg2Csv * msg2csv = NULL;

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
  Msg2CsvArgs csvargs;

  /* initialize rai::Time, rai::Sys::in, rai::Sys::out, rai::Sys::err */
  rai::Sys::initialize();
  /* open log to stderr in case command line fails to parse, it may open
   * again if -log is specified on command line */
  rai::Log::openLog( "-", rai::Log::LVL_MINOR, 4 );

  try {
    /* open the api type from the command line */
    api = RaiApi::RaiOpen( NULL, argc, argv );
    /* get the api's configuration arguments */
    api->GetArgs( args );
    /* get the msg2csv args */
    csvargs.getArgs( args );
    /* get the arguments for the dictionary, useful for parsing dict files
     * locally in the filesystem instead of receiving it on the network */
    RaiApi::GetDictArgs( args );
    /* get the logging, version, help, rc arguments and sets error output */
    args.addDefaults( api->RaiVersion(), "rai_", rai::Sys::err, argv[ 0 ] );

    try {
      if ( args.processArgs( argc, argv ) ) {
        msg2csv = NEW Msg2Csv();
        if ( msg2csv->init( api, args ) )
          msg2csv->dispatchLoop();
        msg2csv->close();
        delete msg2csv;
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
  if ( msg2csv == NULL )
    exit( 1 );
  msg2csv->quit = true;
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
  if ( msg2csv == NULL )
    exit( 1 );
  if ( msg2csv != NULL )
    msg2csv->quit = true;
  return TRUE;
}
#endif
#endif
