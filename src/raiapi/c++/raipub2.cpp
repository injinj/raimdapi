/* Copyright (c) 2009 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_api__raipub2_cpp[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <signal.h>
#else
#include <windows.h>
#endif
#include "raiapi2.h"
#include "msg/string_to_msg.h"
#include "stream/io_stream.h"
#include "stream/file_stream.h"
#include "stream/byte_array_stream.h"
#include "util/str_util.h"

/* 
 * This example Publisher creates a Publishing class. The class uses a 
 * main loop to Publish a message up to count times. 
 *
 * This example is very inefficient in that it creates and Destroys messages
 * each time and is not meant to demonstrate optimization.
 *
 */
namespace {

struct RaiPub2Args {
  const rai::StringArg subject_arg, prefix_arg, data_arg, form_arg,
                       msgtype_arg, recstat_arg, save_arg;
  const rai::UIntArg   count_arg, delay_arg;
  const rai::BoolArg   inf_arg, quiet_arg, sass_arg, rvmsg_arg, nohdr_arg;

  RaiPub2Args() :
    subject_arg( "subject", "TEST.REC.AAA.NaE", "<subject>",
                 "Subject name to publsh, use '-' to read from stdin" ),
    prefix_arg(  "prefix", NULL, "<subject>",
                 "Subject to prefix publish subject with, usually set to "
                 "_TIC. if using SASS/RV" ),
    data_arg(    "data", "ASK=11.0,BID=10.5", "<field=val,...>",
                 "Field values to publish" ),
    form_arg(    "form", "EQ", "<name>",
                 "Form class to use, for none use \"\"" ),
    msgtype_arg( "msgType", "INITIAL", "<msg-type>",
                 "Type of message to Publish, for example: "
                 "UPDATE, VERIFY, DROP" ),
    recstat_arg( "recStatus", "OK", "<rec-status>",
                 "Status message to Publish, for example: OK, EXPIRED" ),
    save_arg(    "save", NULL, "<file>",
                 "Save messages to file in replay format instead of "
                 "publishing them" ),
    count_arg(   "count", 1, "<num>", "Number of times to publish message"),
    delay_arg(    "delay", 1, "<num>", "Number of milliseconds between each published message"),
    inf_arg(     "inf", false, "<bool>", "Publish forever" ),
    quiet_arg(   "q", false, "<bool>", "Be quiet, otherwise log each publish" ),
    sass_arg(    "sass", false, "<bool>",
                 "Use QFORMs instead of RaiMsg message format" ),
    rvmsg_arg(   "rv", false, "<bool>",
                 "Use RvMsg instead of RaiMsg message format" ),
    nohdr_arg(   "nohdr", false, "<bool>",
                 "Publish a message without adding the SASS header, "
                 "without MSG_TYPE, REC_TYPE, SEQ_NO, REC_STATUS fields" ) {}

  void getArgs( rai::Args &args ) const {
    args.add( &subject_arg, rai::COMMAND_ARG | rai::RESOURCE_ARG |
                            rai::LIST_ARG );
    args.add( &prefix_arg );
    args.add( &data_arg );
    args.add( &form_arg );
    args.add( &msgtype_arg );
    args.add( &recstat_arg );
    args.add( &count_arg );
    args.add( &delay_arg );
    args.add( &inf_arg );
    args.add( &quiet_arg );
    args.add( &sass_arg );
    args.add( &rvmsg_arg );
    args.add( &nohdr_arg );
    args.add( &save_arg );
  }
};

  
struct RaiPub2 {
  RaiApi            * api;
  RaiSession        * session;
  RaiPublish        * pub;
  RaiDict           * dataDict;
  RaiQueue          * queue;
  char              * subjname,
                    * formname,
                    * typenam,
                    * recstat,
                    * datavals,
                    * buf;
  rai::StringToMsg    strToMsg;
  rai::InputStream  * subjIn;
  rai::OutputStream * saveOut;
  unsigned int        counter,
		      delay,
                      msgCount,
                      bufSize;
  Rai_u16             seqNo;
  bool                infinite,
                      quiet,
                      useSass,
                      useRvmsg,
                      noHdr,
                      quit;
  /*
   * Class constructor, ensure all internal values are initialized
   */
  RaiPub2()
      : api( 0 ), session( 0 ), pub( 0 ), dataDict( 0 ), queue( 0 ),
        subjname( 0 ), formname( 0 ), typenam( 0 ), recstat( 0 ), datavals( 0 ),
        buf( 0 ), subjIn( 0 ), saveOut( 0 ), counter( 0 ), delay(1), msgCount( 0 ),
        bufSize( 0 ), seqNo( 1 ), infinite( false ), quiet( false ),
        useSass( false ), useRvmsg( false ), noHdr( false ), quit( false ) {}
  ~RaiPub2() {
    if ( this->buf != NULL )
      FREE( this->buf );
    if ( this->pub != NULL )
      delete this->pub;
    if ( this->dataDict != NULL )
      delete this->dataDict;
    if ( this->queue != NULL )
      delete this->queue;
    if ( this->session != NULL )
      delete this->session;
    if ( this->subjname != NULL )
      FREE( this->subjname );
    if ( this->formname != NULL )
      FREE( this->formname );
    if ( this->typenam != NULL )
      FREE( this->typenam );
    if ( this->recstat != NULL )
      FREE( this->recstat );
    if ( this->datavals != NULL )
      FREE( this->datavals );
    if ( this->saveOut != NULL )
      delete this->saveOut;
    if ( this->api != NULL )
      delete this->api;
  }

  void pubMsg( void ) {
    RaiMsg     * raiMsg = NULL;
    Rai_u16      msgType   = rai::SassConst::INITIAL,
                 recStatus = rai::SassConst::STATUS_OK;
    const char * what = this->saveOut ? "Save" : "Publish",
               * pref = NULL;
    try {
      /* 
       * Check the session to see what sort of message we want to use. 
       * If we are packing a SASS compatible QForm use the appropriate
       * message prototype
       */
      if ( this->noHdr ) {
        if ( this->useSass ) {
          raiMsg = NEW RaiMsg( TIB_SASS_PROTO );
          this->api->PrintLog( LDEBUG, "Creating SASS Msg" );
        }
        else if ( this->useRvmsg ) {
          raiMsg = NEW RaiMsg( RV_PROTO );
          this->api->PrintLog( LDEBUG, "Creating RvMsg Msg" );
        }
        else {
          raiMsg = NEW RaiMsg( RAIMSG_PROTO );
          this->api->PrintLog( LDEBUG, "Creating RaiMsg" );
        }
      }
      else {
        if ( this->typenam != NULL ) {
          msgType = rai::SassConst::stringToMsgType( this->typenam );
          this->api->PrintLog( LDEBUG, "MSG_TYPE %s=%u", this->typenam,
                               msgType );
        }
        if ( this->recstat != NULL ) {
          recStatus = rai::SassConst::stringToRecStatus( this->recstat );
          this->api->PrintLog( LDEBUG, "REC_STATUS %s=%u", this->recstat,
                               recStatus );
        }
        try {
          if ( this->useSass ) {
            raiMsg = RaiApi::NewSASSMsg( msgType, this->formname,
                                         this->seqNo, recStatus );
            this->api->PrintLog( LDEBUG, "Creating SASS Msg" );
          }
          else if ( this->useRvmsg ) {
            raiMsg = RaiApi::NewRaiMsg( RV_PROTO, msgType, this->formname,
                                        this->seqNo, recStatus );
            this->api->PrintLog( LDEBUG, "Creating RvMsg Msg" );
          }
          else {
            raiMsg = RaiApi::NewRaiMsg( msgType, this->formname,
                                        this->seqNo, recStatus );
            this->api->PrintLog( LDEBUG, "Creating RaiMsg" );
          }
        } catch ( RaiException e ) {
          this->api->PrintLog( LERROR, e, "Creating message formclass = %s",
                               this->formname );
          throw e;
        }
        this->seqNo++;
      }
      /* convert the command line string to a fielded message */
      this->strToMsg.addFields( *raiMsg, this->datavals );

      /* if %u or %d appears in subject, replace it with a sequence number */
      char subjBuf[ rai::SassConst::MAX_SUBJECT_LEN ];
      unsigned int off = 0;
      for ( const char *ptr = this->subjname; *ptr != '\0'; ) {
        if ( ptr[ 0 ] == '%' && ( ptr[ 1 ] == 'd' || ptr[ 1 ] == 'u' ) ) {
          char *end;
          rai::StrUtil::intToString( this->msgCount, &subjBuf[ off ],
                                sizeof( subjBuf ) - ( off + 1 ),
                                rai::U_DECIMAL, false, &end );
          off += ( end - &subjBuf[ off ] );
          ptr += 2;
        }
        else {
          if ( off < sizeof( subjBuf ) - 1 )
            subjBuf[ off++ ] = *ptr;
          ptr++;
        }
      }
      subjBuf[ off ] = '\0';

      /* if message should be saved for later replay */
      if ( this->saveOut != NULL ) {
        RaiMsg_size size = raiMsg->PackSize();
        this->saveOut->printf( "%s\n%u\n", subjBuf,
                               (unsigned int) size );
        this->saveOut->writeBytes( (const byte *) raiMsg->Packed(),
                                   (unsigned int) size );
        this->saveOut->flush();
      }
      else {
        this->pub->Publish( subjBuf, *raiMsg );

        pref = this->pub->GetPrefix();
      }
      /* print the publish subject */
      if ( ! this->quiet )
        this->api->PrintLog( LMINOR, "%s %s%s", what, pref ? pref : "",
                             subjBuf );
      /* print the publish message */
      if ( rai::Log::minLevel <= rai::Log::LVL_DEBUG ) {
        rai::OutputStream *out = rai::Log::lock();
        raiMsg->Print( out );
        out->flush();
        rai::Log::unlock();
      }
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "%s message", what );
    }
    if ( raiMsg != NULL )
      delete raiMsg;
  }

  bool init( RaiApi *apip,  rai::Args &args ) {
    const char * save;
    this->api = apip;
    try {
      RaiPub2Args pubargs;
      STRDUP( this->subjname, args.getString( pubargs.subject_arg.name ) );
      STRDUP( this->formname, args.getString( pubargs.form_arg.name ) );
      STRDUP( this->typenam, args.getString( pubargs.msgtype_arg.name ) );
      STRDUP( this->recstat, args.getString( pubargs.recstat_arg.name ) );
      STRDUP( this->datavals, args.getString( pubargs.data_arg.name ) );
      this->counter     = args.getUInt( pubargs.count_arg.name );
      this->infinite    = args.getBoolean( pubargs.inf_arg.name );
      this->quiet       = args.getBoolean( pubargs.quiet_arg.name );
      this->useSass     = args.getBoolean( pubargs.sass_arg.name );
      this->useRvmsg    = args.getBoolean( pubargs.rvmsg_arg.name );
      this->noHdr       = args.getBoolean( pubargs.nohdr_arg.name );
      this->delay       = args.getUInt( pubargs.delay_arg.name );

      RaiApi::OpenLog( args );
      /* if cfilePaath specified on the command line */
      bool noDictionary = RaiApi::OpenDict( args );

      if ( (save = args.getString( pubargs.save_arg.name )) != NULL ) {
        try {
          this->saveOut = rai::FileOutputStream::append( save );
        } catch ( RaiException e ) {
          this->api->PrintLog( LERROR, e, "Save file %s", save );
          throw e;
        }
      }
      this->api->ParseArgs( args );

      this->session = this->api->CreateSession();
      this->session->Start();
      this->queue   = this->session->CreateQueue();

      if ( save == NULL ) {
        this->pub     = this->session->CreatePublish();
        this->pub->SetPrefix( args.getString( pubargs.prefix_arg.name ) );
      }
      /*
       * If we are using the SASS protocol for Publishing we need 
       * to create and Load a dictionary. 
       */
      if ( this->useSass && ! noDictionary ) {
        this->dataDict = this->session->CreateDict();
        this->dataDict->Load( 5, NULL, false );
        while ( this->dataDict->InProgress() && ! this->quit )
          ;
        if ( ! this->dataDict->HaveDict() ) {
          this->api->PrintLog( LMINOR, "Dictionary load timed out" );
          return false;
        }
        this->api->PrintLog( LMINOR, "Dictionary received" );
      }
      if ( this->subjname[ 0 ] != '-' ) {
        unsigned int n;
        if ( (n = args.getNumValues( pubargs.subject_arg.name )) > 1 ) {
          unsigned int i, dataLen;
          byte *data;
          rai::ByteArrayOutputStream bout( NULL, 1024, true );
          bout.printf( "%s\n", this->subjname );
          for ( i = 1; i < n; i++ )
            bout.printf( "%s\n", args.getString( pubargs.subject_arg.name, i ));
          bout.flush();
          dataLen = bout.getData( &data );
          bout.reset();
          this->subjIn = NEW rai::ByteArrayInputStream( data, dataLen, false,
                                                        true );
          STRDUP( this->subjname, "-" );
        }
      }
      else {
        this->subjIn = rai::Sys::in;
      }
      return true;
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "Not initialized, stopped" );
    }
    return false;
  }

  void close( void ) {
    this->quit = true;
    if ( this->pub != NULL )
      this->pub->Destroy();
    if ( this->queue != NULL )
      this->queue->Destroy();
    if ( this->session != NULL )
      this->session->Destroy();
    if ( this->saveOut != NULL )
      this->saveOut->close();
    if ( this->api != NULL )
      this->api->Close();
  }

  /* thread from service for dispatch loop */
  void serviceRun( void ) {
    this->dispatchLoop();
  }

  void dispatchLoop( void ) {
    for (;;) {
      if ( this->quit )
        break;
      if ( this->msgCount >= this->counter ) {
        if ( ! this->infinite )
          if ( this->subjIn == NULL )
            break;
      }
      try {
        if ( this->subjIn != NULL ) {
          char buf[ rai::SassConst::MAX_SUBJECT_LEN ];
        get_next_line:;
          unsigned int n = this->subjIn->gets( buf, sizeof( buf ) );
          if ( n == 0 ) /* end of input */
            break;
          /* strip end of line whitespace */
          while ( n > 0 && ( buf[ n-1 ] == ' ' || buf[ n-1 ] == '\n' ||
                             buf[ n-1 ] == '\r' ) )
            buf[ --n ] = '\0';
          if ( n == 0 )
            goto get_next_line;
          STRDUP( this->subjname, buf );
        }
        this->pubMsg();
        this->msgCount++;
        this->queue->TimedDispatch( delay );
      } catch ( RaiException e ) {
        this->api->PrintLog( LERROR, e, "In dispatchLoop" );
        return;
      }
    }
    try {
      this->queue->TimedDispatch( 250 );
    } catch ( ... ) {
    }
  }
};
} // namespace

#ifdef RAI_DLL_EMBEDDED

#include "raiapi2_service.h"

/* dll entry point, determined by service file name: rai_service_raipub2.so */
extern "C" RAI_DLL_EXPORT void
RAIPUB2_ServiceInitialize( void )
{
  rai::ServiceFactory * fact = NEW
    T_RaiApiServiceFactory< RaiPub2Args, 
      T_RaiApiServiceProto< RaiPub2 > >( "raipub2" );
  rai::ServiceFactory::installService( fact );
}

#else
RaiPub2 * raipub2 = NULL;

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
extern "C" void myInterruptHandler( int );
#else
extern "C" BOOL myCtrlHandler( DWORD );
#endif
/*
 * Example Rai Publisher. This example creates a Publishing object,
 * retrieves the data dictionary from the Cache and Publishes into
 * the cache using the specified subject. If no data is provided then
 * some pseudo random data is Published. The Publisher will Publish
 * only onc unless a count is specified. 
 */

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
  RaiPub2Args pubargs;

  /* initialize rai::Time, rai::Sys::in, rai::Sys::out, rai::Sys::err */
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
    /* get the publish args */
    pubargs.getArgs( args );
   /* get the arguments for the dictionary, useful for parsing dict files
    * locally in the filesystem instead of receiving it on the network */
    RaiApi::GetDictArgs( args );
    /* get the logging, version, help, rc arguemtns and sets error output */
    args.addDefaults( api->RaiVersion(), "rai_", rai::Sys::err, argv[ 0 ] );

    try {
      if ( args.processArgs( argc, argv ) ) {
        raipub2 = new RaiPub2();
        if ( raipub2->init( api, args ) )
          raipub2->dispatchLoop();
        raipub2->close();
        delete raipub2;
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
  if ( raipub2 == NULL )
    exit( 1 );
  raipub2->quit = true;
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
  if ( raipub2 == NULL )
    exit( 1 );
  if ( raipub2 != NULL )
    raipub2->quit = true;
  return TRUE;
}   
#endif 
#endif
