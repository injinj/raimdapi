/* Copyright (c) 2004 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_api__raisub_cpp[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <signal.h>
#else
#include <windows.h>
#endif
#include "raiapi.h"
#include "raisampleutil.h"

using namespace rai;
using namespace rai_old; /* v1 api */

class SubTest {
 public:
  RaiDict      * dataDict;
  RaiSession   * session;
  RaiSubscribe * sub; 
  bool           quit;
  
  static void
  subject_onMsg( RaiEvent * event,  RaiMsg * raiMsg,  void * /* closure */ ) {
    
    try {
      Sys::out->printf( "subscribed subject %s\n", event->subscribedSubject );
      Sys::out->printf( "received subject   %s\n", event->receivedSubject );
      raiMsg->Print( Sys::out );
      Sys::out->flush();
    } catch ( RaiException e ) {
      Sys::err->printf( "error %s.%u: %s\n", e->module, e->status, e->reason );
    }
  }; 
  
  SubTest() {
    this->dataDict = NULL;
    this->session  = NULL;
    this->sub      = NULL;
    this->quit     = false;
  };
  
  bool init( const char *svcname, const char *netname,
	     const char *dmnname, unsigned int logLevel, 
             const char *username ){
    char dictSubject[80];

    try {
      RaiApi::RaiOpen( RV7, SASS2 );
      RaiApi::SetLogLevel( logLevel );
      this->session = NEW RaiSession(svcname, netname, dmnname );
      
      this->dataDict = new RaiDict();
      ::strcpy(dictSubject, "_TIC.REPLY.SASS.DATA.DICTIONARY");
      this->dataDict->Load( this->session, dictSubject );
      if( username != NULL )
        RaiApi::RaiLogin( this->session, (char *)username );
      return true;
    } catch ( RaiException e ) {
      Sys::err->printf( "Init: %s.%u: %s\n", e->module, e->status, e->reason );
    };
    return false;
  };

  bool subscribe( const char * subject ) {
   void * closure = ( void * ) this; // pass a handle to the subtest object
   
   
    try {
      this->sub = new RaiSubscribe( this->session, subject,
                                    SubTest::subject_onMsg, closure );
      return true;
    } catch ( RaiException e ){
      Sys::err->printf( "Subscribe: %s.%u: %s\n", e->module, e->status,
                        e->reason );
    };
    return false;
  };
  
  void close( void ) {
    RaiApi::RaiClose();
  };
  
  void mainloop( void ) {
    while ( ! this->quit ) {
      try {
        RaiApi::RaiTimedDispatch( this->session, 1 );
      } catch ( RaiException e ) {
        Sys::err->printf( "Dispatch: %s.%u: %s\n", e->module, e->status,
                          e->reason );
      }
    }
    if ( this->sub != NULL ) {
      try {
        this->sub->Cancel();
        delete this->sub;
      } catch ( RaiException e ) {
        Sys::err->printf( "Cancel: %s.%u: %s\n", e->module, e->status,
                          e->reason );
      }
    }
    this->close();
  };

  virtual ~SubTest(){
  };
};

class SubTest;
SubTest * interruptSubTest = NULL;
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
static void
interruptHandler( int sig ) {
  if ( sig == SIGHUP )
    return;
  Sys::err->printf( "Caught signal %d event, shutting down\n", sig );
  if ( interruptSubTest == NULL )
    exit( 1 );
  interruptSubTest->quit = true;
}
#else
BOOL
CtrlHandler( DWORD fdwCtrlType )
{
  const char *s = NULL;
  switch ( fdwCtrlType ) {
    case CTRL_C_EVENT:
      s = "ctrl-c";
      break;
    case CTRL_CLOSE_EVENT:
      s = "close";
      break;
    case CTRL_BREAK_EVENT:
      s = "ctrl-break";
      break;
    case CTRL_LOGOFF_EVENT:
      s = "logoff";
      break;
    case CTRL_SHUTDOWN_EVENT:
    default:
      s = "shutdown";
      break;
  }
  Sys::err->printf( "Caught %s event, shutting down\n", s );
  if ( interruptSubTest == NULL )
    exit( 1 );
  if ( interruptSubTest != NULL )
    interruptSubTest->quit = true;
  return TRUE;
}
#endif


int main( int argc, char *argv[] ) {
  Argument subject(  "subject", "TEST.REC.AAA.NaE", "-subject TEST.REC.AAA.NaE",
                      "Subject to subscribe to" );
  Argument network(  "network", NULL, "-network 172.16.1.0",
                      "Network of interface to use" );
  Argument service(  "service", NULL, "-service 7600",
                      "rv service to use" );
  Argument daemon(   "daemon", NULL, "-daemon tcp:7600",
                      "Daemon to connect to" );
  Argument userid(   "userid", NULL, "Default",
                      "User ID to login with NULL to disable" );
  Argument loglevel(  "logLevel", "0", "-logLevel 3",
                      "1-Major, 2-Minor, 3-Debug, 4-Trace, 5-Full" );
  ArgList args;

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
   struct sigaction nsa;

  /* do this before threads are created so that they inherit the sigs */
  ::memset( &nsa, 0, sizeof( nsa ) );
  ::sigemptyset( &nsa.sa_mask );
  nsa.sa_handler = ( (void (*)(int) ) ::interruptHandler );
  ::sigaction( SIGHUP, &nsa, NULL );
  ::sigaction( SIGINT, &nsa, NULL );
  ::sigaction( SIGTERM, &nsa, NULL ); 
#else
  ::SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ::CtrlHandler, TRUE );
#endif

  Sys::initialize();

  args.add( &subject );
  args.add( &network );
  args.add( &service );
  args.add( &daemon  );
  args.add( &userid  );
  args.add( &loglevel);

  try {
    if ( args.processArgs( argc, argv ) ) {
      const char * subjname  = args.getString( subject.name ),
                 * netname   = args.getString( network.name ),
                 * svcname   = args.getString( service.name ),
                 * dmnname   = args.getString( daemon.name  ),
                 * username  = args.getString( userid.name  );
      unsigned int logLevel  = args.getUInt( loglevel.name  );
                 
      SubTest *subTest = new SubTest();
      if ( subTest->init( svcname, netname, dmnname, logLevel, username ) ) {
        if ( subTest->subscribe( subjname ) ) {
          interruptSubTest = subTest;
          subTest->mainloop();
          interruptSubTest = NULL;
          delete subTest;
        }
      }
    }
  } catch ( RaiException e ) {
    Sys::err->printf( "Error: %s.%u: %s\n", e->module, e->status, e->reason );
  }
  return 0;
}


