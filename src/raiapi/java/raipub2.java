/* Copyright (c) 2011 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

import com.rai.raimsg.*;
import com.rai.raiapi2.*;
import com.rai.raiexception.RaiException;

/* 
 * This example Publisher creates a Publishing class. The class uses a 
 * main loop to Publish a message up to count times. 
 */

class raipub2Args {
  StringArg subject_arg, prefix_arg, data_arg, form_arg,
            msgtype_arg, recstat_arg;
  IntArg    count_arg;
  BoolArg   sass_arg;

  raipub2Args() {
    subject_arg = new StringArg( "subject", "TEST.REC.AAA.NaE", "<subject>",
                 "Subject name to publsh" );
    prefix_arg = new StringArg( "prefix", null, "<subject>",
                 "Subject to prefix publish subject with, usually set to " +
                 "_TIC. if using SASS/RV" );
    data_arg = new StringArg( "data", "ASK=11.0,BID=10.5", "<field=val,...>",
                 "Field values to publish" );
    form_arg = new StringArg( "form", "EQ", "<name>",
                 "Form class to use, for none use \"\"" );
    msgtype_arg = new StringArg( "msgType", "INITIAL", "<msg-type>",
                 "Type of message to Publish, for example: " +
                 "UPDATE, VERIFY, DROP" );
    recstat_arg = new StringArg( "recStatus", "OK", "<rec-status>",
                 "Status message to Publish, for example: OK, EXPIRED" );
    count_arg = new IntArg( "count", 1, "<num>",
                 "Number of times to publish message");
    sass_arg = new BoolArg( "sass", false, "<bool>",
                 "Use QFORMs instead of RaiMsg message format" );
  }

  void getArgs( Args args ) throws RaiException {
    args.add( subject_arg );
    args.add( prefix_arg );
    args.add( data_arg );
    args.add( form_arg );
    args.add( msgtype_arg );
    args.add( recstat_arg );
    args.add( count_arg );
    args.add( sass_arg );
  }
}

  
public class raipub2 {
  RaiApi       api;
  RaiSession   session;
  RaiPublish   pub;
  RaiDict      dataDict;
  RaiQueue     queue;
  String       subjname,
               formname,
               typenam,
               recstat,
               datavals;
  int          counter;
  short        seqNo;
  int          sigCaught;
  boolean      useSass,
               quit;
  /*
   * Class constructor, ensure all internal values are initialized
   */
  public raipub2() {
    seqNo = 1;
  }

  public void pubMsg() {
    RaiMsg       raiMsg;
    int          ptr,
                 tmp,
                 len;
    String       fname,
                 fval;
    short        msgType   = SassConst.INITIAL,
                 recStatus = SassConst.STATUS_OK;
    try {
      /* 
       * Check the session to see what sort of message we want to use. 
       * If we are packing a SASS compatible QForm use the appropriate
       * message prototype
       */
      if ( this.typenam != null ) {
        msgType = SassConst.stringToMsgType( this.typenam );
        this.api.PrintLog( RaiApi.LVL_DEBUG, "MSG_TYPE " +
                           this.typenam + "=" + msgType );
      }
      if ( this.recstat != null ) {
        recStatus = SassConst.stringToRecStatus( this.recstat );
        this.api.PrintLog( RaiApi.LVL_DEBUG, "REC_STATUS " + this.recstat +
                           "=" + recStatus );
      }
      try {
        if ( this.useSass )
          raiMsg = RaiApi.NewSASSMsg( msgType, this.formname,
                                      this.seqNo, recStatus );
        else
          raiMsg = RaiApi.NewRaiMsg( msgType, this.formname,
                                     this.seqNo, recStatus );
      } catch ( RaiException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e,
                           "Creating message formclass = " + this.formname );
        throw e;
      }
      this.seqNo++;

      /* 
       * Take the data values provided on the command line and add them 
       * to the message
       */
      ptr = 0;
      len = this.datavals.length();
      for (;;) {
        tmp = ptr;
        if ( ptr == -1 || (ptr = this.datavals.indexOf( '=', ptr )) == -1 )
          break;
        fname = this.datavals.substring( tmp, ptr );

        tmp = ++ptr;
        if ( (ptr = this.datavals.indexOf( ',', ptr )) == -1 )
          ptr = len;
  
        fval = this.datavals.substring( tmp, ptr );
        /*
         * Add the field and data to the message. In this case
         * the message is not using a pre-defined record type so
         * any field can be specified.
         */
        this.api.PrintLog( RaiApi.LVL_DEBUG, "Setting field " +
                           fname + "=" + fval );
        raiMsg.Append( fname, fval );
        if ( ptr == len )
          break;
        ptr++;
      }
      this.pub.Publish( this.subjname, raiMsg );

      String pref = this.pub.GetPrefix();
      if ( pref == null )
        pref = "";

      this.api.PrintLog( RaiApi.LVL_MINOR,
                         "Published " + pref + this.subjname );
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Publishing message" );
    }
  }

  public boolean init( RaiApi api,  Args args ) {
    this.api = api;
    try {
      raipub2Args pubargs = new raipub2Args();
      this.subjname = args.getString( pubargs.subject_arg.name );
      this.formname = args.getString( pubargs.form_arg.name );
      this.typenam  = args.getString( pubargs.msgtype_arg.name );
      this.recstat  = args.getString( pubargs.recstat_arg.name );
      this.datavals = args.getString( pubargs.data_arg.name );
      this.counter  = args.getInt( pubargs.count_arg.name );
      this.useSass  = args.getBoolean( pubargs.sass_arg.name );
      boolean noDictionary = ! this.useSass;

      RaiApi.OpenLog( args );
      /* if cfilePaath specified on the command line */
      if ( ! noDictionary )
        noDictionary = RaiApi.OpenDict( args );
      this.api.ParseArgs( args );

      this.session = this.api.CreateSession();
      this.session.Start();
      this.queue   = this.session.CreateQueue();
      this.pub     = this.session.CreatePublish();
      this.pub.SetPrefix( args.getString( pubargs.prefix_arg.name ) );

      /*
       * If we are using the SASS protocol for Publishing we need 
       * to create and Load a dictionary. 
       */
      if ( this.useSass && ! noDictionary ) {
        this.dataDict = this.session.CreateDict();
        this.dataDict.Load( 3, null, false );
        while ( this.dataDict.InProgress() && ! this.quit )
          ;
        if ( this.quit )
          return false;
        if ( ! this.dataDict.HaveDict() ) {
          this.api.PrintLog( RaiApi.LVL_MINOR, "Dictionary load timed out" );
          return false;
        }
        this.api.PrintLog( RaiApi.LVL_MINOR, "Dictionary received" );
      }
      return true;
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Not initialized, stopped" );
    }
    return false;
  }

  public void close() {
    this.quit = true;
    if ( this.pub != null )
      this.pub.Destroy();
    if ( this.queue != null )
      this.queue.Destroy();
    if ( this.session != null )
      this.session.Destroy();
    if ( this.api != null )
      this.api.Close();
  }

  /* thread from service for dispatch loop */
  public void serviceRun() {
    this.dispatchLoop();
  }

  void dispatchLoop() {
    while ( ! this.quit ) {
      try {
        this.queue.TimedDispatch( 100 );
        this.pubMsg();
        if ( --this.counter <= 0 )
          break;
      } catch ( RaiException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "In dispatchLoop" );
      }
    }
  }

  public static raipub2 pubTest;

  public static void sigHandler( int sig ) {
    /* be careful with locks in here, could deadlock if interrupt
     * happened while inside a critical section (for example, logging) */
    if ( pubTest != null ) {
      pubTest.sigCaught = sig;
      pubTest.quit = true;
    }
    else
      System.exit( 1 );
  }

  /*
   * Example Rai Publisher. This example creates a Publishing object,
   * retrieves the data dictionary from the Cache and Publishes into
   * the cache using the specified subject. If no data is provided then
   * some pseudo random data is Published. The Publisher will Publish
   * only onc unless a count is specified. 
   */
  public static void main( String[] argv ) {
    RaiApi      api = null;
    Args        args;
    raipub2Args pubargs;

    try {
      /* traps SIGINT, SIGHUP, SIGTERM and calls sigHandler(),
       * this may require LD_PRELOAD=libjsig.so and/or
       * java option -Xrs (rs = reduce signals) */
      RaiApi.RegisterSigHandler( raipub2.class.getName(), "sigHandler" );
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
      pubargs = new raipub2Args();
      /* Open the api type from the command line, looks for -api <name> in
       * argc/argv[] and loads that middleware.  Program could also pass "tibrv"
       * or some other api name in the first argument.  If neither are specfied
       * then the default api is loaded (capr) */
      api = RaiApi.RaiOpen( null, argv );
      /* get the api's configuration arguments */
      api.GetArgs( args );
      /* get the publish args */
      pubargs.getArgs( args );
     /* get the arguments for the dictionary, useful for parsing dict files
      * locally in the filesystem instead of receiving it on the network */
      RaiApi.GetDictArgs( args );
      /* get the logging, version, help, rc arguemtns and sets error output */
      args.addDefaults( api.RaiVersion(), "rai_", System.err, "raipub2" );

      try {
        if ( args.processArgs( argv ) ) {
          pubTest = new raipub2();
          if ( pubTest.init( api, args ) )
            pubTest.dispatchLoop();
          /* close the api */
          if ( pubTest.sigCaught != 0 )
            api.Log( "Caught signal " + pubTest.sigCaught + ", quitting" );
          pubTest.close();
        }
      } catch ( RaiException e ) {
        RaiApi.Log( RaiApi.LVL_ERROR, e, "Main" );
      }
    } catch ( RaiException e ) {
      RaiApi.Log( RaiApi.LVL_ERROR, e, "Unable to load Rai API" );
    }
    RaiApi.Log( RaiApi.LVL_MINOR, "Finished" );
  }
}
