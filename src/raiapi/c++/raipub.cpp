/* Copyright (c) 2004 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_api__raipub_cpp[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "raiapi.h"
#include "raisampleutil.h"

using namespace rai;

/* 
 * This example Publisher creates a Publishing class. The class uses a 
 * main loop to Publish a message up to count times. 
 *
 * This example is very inefficient in that it creates and Destroys messages
 * each time and is not meant to demonstrate optimization.
 *
 */

class PubTest {
 protected:
  RaiSession  * session;
  RaiPublish  * Publisher;
  const char  * subjname,
              * formname,
              * protoname,
              * typenam,
              * username,
              * datavals;
  unsigned int  formId,
                counter,
                logLevel,
                seqNo;
  Sass::MsgType  msgType;
  int           isComplex;

 public:

  /*
   * Class constructor, ensure all internal values are initialized
   */

  PubTest( const char *subjname, const char *formname, const char *typenam,
           const char *protoname, const char *datavals, unsigned int counter, 
           unsigned int logLevel ){
    this->subjname  = subjname;
    this->formname  = formname;
    this->typenam   = typenam;
    this->protoname = protoname;
    this->datavals  = datavals;
    this->counter   = counter;
    this->logLevel  = logLevel;
    this->msgType   = Sass::INITIAL;
    this->formId    = 0;
    this->isComplex = isComplex;
    this->seqNo     = 1;         /* start from 1 for complex publisher */
                                 /* simple publisher start from 0. */
  };

  void pubMsg( void ){
    RaiMsg        * raiMsg = NULL;
    const char    * ptr,
                  * tmp;
    unsigned int    len;
    char            fname[256],
                    fval[256];
    const RaiMsg_form * form;

    try {

      /* 
       * Check the session to see what sort of message we want to use. 
       * If we are packing a SASS compatible QForm use the appropriate
       * message prototype
       */
      
      if( this->typenam != NULL ) {
        if( isdigit( this->typenam[0] ) )
          this->msgType = ( Sass::MsgType ) ::atoi( this->typenam );
        else if( strcmp( this->typenam, "VERIFY" ) == 0 )
          this->msgType = Sass::VERIFY;
        else if( strcmp( this->typenam, "UPDATE" ) == 0 )
          this->msgType = Sass::UPDATE;
        else // default to INITIAL
          this->msgType = Sass::INITIAL;
      }
      if( DataDictionary != NULL &&
          (form = DataDictionary->getForm( this->formname )) != NULL )
        raiMsg = RaiApi::NewSASSMsg( this->msgType, form->entry->fid );
      else 
        raiMsg = RaiApi::NewRaiMsg( this->msgType, this->formname );

      /* 
       * Take the data values provided on the command line and add them 
       * to the message
       */

      ptr = this->datavals;
      for (;;) {
        tmp = ptr;
        if ( ptr == NULL || (ptr = ::strchr( ptr, '=' )) == NULL )
          break;
        len = ptr - tmp;
        if ( len > sizeof( fname ) - 1 )
          len = sizeof( fname ) - 1;
        ::strncpy( fname, tmp, len );
        fname[ len ] = '\0';

        tmp = ++ptr;
        if ( (ptr = ::strchr( ptr, ',' )) == NULL )
          ptr = &tmp[ ::strlen( tmp ) ];
  
        len = ptr - tmp;
        if ( len > sizeof( fval ) - 1 )
          len = sizeof( fval ) - 1;
        ::strncpy( fval, tmp, len );
        fval[ len ] = '\0';

        /*
         * Add the field and data to the message. In this case
         * the message is not using a pre-defined record type so
         * any field can be specified.
         */

        printf( "Setting field %s=%s\n", fname, fval );
        raiMsg->Append( fname, (const char *) fval );
        if ( *ptr == '\0' )
          break;
        ptr++;
      }
      if ( this->isComplex ) {
        raiMsg->Update( "SEQ_NO", ( Rai_u16 ) this->seqNo++ );
      }

      this->Publisher->Publish( this->subjname, raiMsg );
      raiMsg->Release();
      delete raiMsg;

    } catch ( RaiException e ) {
      printf( "error: %s.%u: %s\n", e->module, e->status, e->reason );
    }
  };


  void init( const char *svcname, const char *netname,
             const char *dmnname, bool isComplex, 
             const char *username ) throw( RaiException ){

    char    dictSubject[80];
    RaiDict * dataDict;

    try {
      RaiApi::RaiOpen( RV7, SASS2 ); 
      RaiApi::SetLogLevel( this->logLevel );
      this->session = NEW RaiSession( svcname, netname, dmnname );
      this->Publisher = new RaiPublish( this->session, isComplex );
      this->isComplex = isComplex;
      this->username = username;

      /*
       * If we are using the SASS protocol for Publishing we need 
       * to create and Load a dictionary. 
       */
        
      if ( strcmp( this->protoname, "SASS" ) == 0 ){
        printf("Uses SASS dict\n");
        dataDict = new RaiDict();
        ::strcpy(dictSubject, "_TIC.REPLY.SASS.DATA.DICTIONARY");
        dataDict->Load( this->session, dictSubject );
      }
      if( username != NULL )
        RaiApi::RaiLogin( this->session, (char *)username );
      printf( "Finished init\n" );
    } catch ( RaiException e ) {
      printf( "init error: %s.%u: %s\n", e->module, e->status, e->reason );
    } catch ( ... ){      
      printf( "Unknown init error\n" );
    }
  };

  void close( void ){
    RaiApi::RaiClose();
  };

  /*
   * We need to dispatch at least once due to the dictionary request.
   * Use a loop to send the message a number of times. 
   */
  
  void mainloop( void ){
    
    for (;;) {
      RaiApi::RaiTimedDispatch( this->session, 1 );
      this->pubMsg();
      if ( --this->counter <= 0 )
        break;
    }
  };
  
  virtual ~PubTest(){
  };

};


/*
 * Example Rai Publisher. This example creates a Publishing object,
 * retrieves the data dictionary from the Cache and Publishes into
 * the cache using the specified subject. If no data is provided then
 * some pseudo random data is Published. The Publisher will Publish
 * only onc unless a count is specified. 
 */

int main( int argc, char *argv[] ) {

  Argument subject( "subject", "TEST.REC.AAA.NaE", "-subject RAITEST.a.b.c",
                    "Publish subject name" );
  Argument form(  "form", "EQ", "-form RAITEST", "Form class to use" );
  Argument proto(  "proto", "RaiMsg", "-proto SASS", "SASS | RaiMsg" );
  Argument data(  "data", "ASK=11.0,BID=10.5", "-data STRING_80=blah",
                    "field values to Publish" );
  Argument msgType( "msgType", "INITIAL", "INITIAL|UPDATE|...",
                    "Type of message to Publish" );
  Argument network( "network", NULL, "-network 172.16.1.0", "Interface to use" );
  Argument service( "service", NULL, "-service 7600", "rv service to use" );
  Argument daemon(  "daemon", NULL, "-daemon tcp:7600", "rv daemon to connect to" );
  Argument userid(   "userid", NULL, "Default",
                      "User ID to login with NULL to disable" );
  Argument count(   "count", "1", "-count 50", "no of count to Publish message" );
  Argument loglevel( "logLevel", "0", "-logLevel 2", "1-Major, 2-Minor, 3-Debug, 4-Trace, 5-Full" );
  Argument pubType( "pubType", "simple", "-pubType complex", 
                    "Publisher can be simple or complex.\n"
                    "\t\t\t Simple automatically increments SEQ_NO field from 0.\n"
                    "\t\t\t Complex starts of 1, to show that is it set." );
  ArgList args;
  
  PubTest * pubTest = NULL ;

  Sys::initialize();
    
  args.add( &subject );
  args.add( &form );
  args.add( &proto );
  args.add( &data );
  args.add( &msgType );
  args.add( &network );
  args.add( &service );
  args.add( &daemon );
  args.add( &count );  
  args.add( &loglevel );  
  args.add( &pubType );  
  args.add( &userid );  

  try {
    if ( args.processArgs( argc, argv ) ) {
      const char  * subjname  = args.getString( subject.name ),
      * formname        = args.getString( form.name ),
      * datavals        = args.getString( data.name ),
      * typenam         = args.getString( msgType.name ),
      * protoname       = args.getString( proto.name ),
      * netname         = args.getString( network.name ),
      * svcname         = args.getString( service.name ),
      * dmnname         = args.getString( daemon.name ),
      * username        = args.getString( userid.name ),
      * publishType     = args.getString( pubType.name );
      unsigned int  counter     = args.getUInt( count.name );
      unsigned int  logLevel    = args.getUInt( loglevel.name );

      bool isComplex = ( strcmp( publishType, "simple" ) == 0 );
      pubTest = new PubTest( subjname, formname, typenam, protoname, datavals, counter, logLevel );
      pubTest->init(svcname, netname, dmnname, isComplex, username );
      pubTest->mainloop();
      pubTest->close();
      delete pubTest;      
    }
  } catch ( RaiException e ) {
    printf( "error: %s.%u: %s\n", e->module, e->status, e->reason );
  }    
  return 0;
}
