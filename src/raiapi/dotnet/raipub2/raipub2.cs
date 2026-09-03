/* Copyright (c) 2026 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com
 * .NET port of src/raiapi/java/raipub2.java
 *
 * This example publisher creates a publishing class.  The class uses a main
 * loop to publish a message up to count times. */
using System;
using System.Threading;
using Com.Rai.Raimsg;
using Com.Rai.Raiapi2;
using Com.Rai.Raiexception;

class raipub2Args {
  public StringArg subject_arg, prefix_arg, data_arg, form_arg, msgtype_arg, recstat_arg;
  public IntArg    count_arg;
  public BoolArg   sass_arg;

  public raipub2Args() {
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
                 "Type of message to Publish, for example: UPDATE, VERIFY, DROP" );
    recstat_arg = new StringArg( "recStatus", "OK", "<rec-status>",
                 "Status message to Publish, for example: OK, EXPIRED" );
    count_arg = new IntArg( "count", 1, "<num>",
                 "Number of times to publish message" );
    sass_arg = new BoolArg( "sass", false, "<bool>",
                 "Use QFORMs instead of RaiMsg message format" );
  }

  public void getArgs( Args args ) {
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
  string       subjname, formname, typenam, recstat, datavals;
  int          counter;
  short        seqNo;
  int          sigCaught;
  bool         useSass;
  volatile bool quit;

  public raipub2() { seqNo = 1; }

  public void pubMsg() {
    RaiMsg raiMsg;
    short  msgType   = SassConst.INITIAL,
           recStatus = SassConst.STATUS_OK;
    try {
      /* Check what sort of message we want to use.  If we are packing a SASS
       * compatible QForm use the appropriate message prototype */
      if ( this.typenam != null ) {
        msgType = SassConst.stringToMsgType( this.typenam );
        this.api.PrintLog( RaiApi.LVL_DEBUG, "MSG_TYPE " + this.typenam + "=" + msgType );
      }
      if ( this.recstat != null ) {
        recStatus = SassConst.stringToRecStatus( this.recstat );
        this.api.PrintLog( RaiApi.LVL_DEBUG, "REC_STATUS " + this.recstat + "=" + recStatus );
      }
      try {
        if ( this.useSass )
          raiMsg = RaiApi.NewSASSMsg( msgType, this.formname, this.seqNo, recStatus );
        else
          raiMsg = RaiApi.NewRaiMsg( msgType, this.formname, this.seqNo, recStatus );
      } catch ( RaiException e ) {
        this.api.PrintLog( RaiApi.LVL_ERROR, e, "Creating message formclass = " + this.formname );
        throw;
      }
      this.seqNo++;

      /* Take the data values provided on the command line and add them to the
       * message: field=value,field=value */
      using ( raiMsg ) {
        int ptr = 0, len = this.datavals.Length;
        for (;;) {
          int tmp = ptr;
          if ( ptr == -1 || ( ptr = this.datavals.IndexOf( '=', ptr ) ) == -1 )
            break;
          string fname = this.datavals.Substring( tmp, ptr - tmp );
          tmp = ++ptr;
          if ( ( ptr = this.datavals.IndexOf( ',', ptr ) ) == -1 )
            ptr = len;
          string fval = this.datavals.Substring( tmp, ptr - tmp );
          /* the message is not using a pre-defined record type so any field
           * can be specified */
          this.api.PrintLog( RaiApi.LVL_DEBUG, "Setting field " + fname + "=" + fval );
          raiMsg.Append( fname, fval );
          if ( ptr == len )
            break;
          ptr++;
        }
        this.pub.Publish( this.subjname, raiMsg );
      }
      string pref = this.pub.GetPrefix();
      if ( pref == null ) pref = "";
      this.api.PrintLog( RaiApi.LVL_MINOR, "Published " + pref + this.subjname );
    } catch ( RaiException e ) {
      this.api.PrintLog( RaiApi.LVL_ERROR, e, "Publishing message" );
    }
  }

  public bool init( RaiApi api,  Args args ) {
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
      bool noDictionary = ! this.useSass;

      RaiApi.OpenLog( args );
      /* if cfilePath specified on the command line */
      if ( ! noDictionary )
        noDictionary = RaiApi.OpenDict( args );
      this.api.ParseArgs( args );

      this.session = this.api.CreateSession();
      this.session.Start();
      this.queue   = this.session.CreateQueue();
      this.pub     = this.session.CreatePublish();
      this.pub.SetPrefix( args.getString( pubargs.prefix_arg.name ) );

      /* If we are using the SASS protocol for publishing we need to create and
       * load a dictionary. */
      if ( this.useSass && ! noDictionary ) {
        this.dataDict = this.session.CreateDict();
        this.dataDict.Load( 3, null, false );
        while ( this.dataDict.InProgress() && ! this.quit )
          Thread.Yield();
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
    if ( this.pub != null ) this.pub.Destroy();
    if ( this.queue != null ) this.queue.Destroy();
    if ( this.session != null ) this.session.Destroy();
    if ( this.api != null ) this.api.Close();
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
    if ( pubTest != null ) {
      pubTest.sigCaught = sig;
      pubTest.quit = true;
    }
    else
      Environment.Exit( 1 );
  }

  public static int Main( string[] argv ) {
    RaiApi      api = null;
    Args        args;
    raipub2Args pubargs;

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
      pubargs = new raipub2Args();
      /* get the api's configuration arguments */
      api.GetArgs( args );
      /* get the publish args */
      pubargs.getArgs( args );
      /* get the arguments for the dictionary */
      RaiApi.GetDictArgs( args );
      /* get the logging, version, help, rc arguments and sets error output */
      args.addDefaults( RaiApi.RaiVersion(), "rai_", Console.Error, "raipub2" );

      try {
        if ( args.processArgs( argv ) ) {
          pubTest = new raipub2();
          if ( pubTest.init( api, args ) )
            pubTest.dispatchLoop();
          if ( pubTest.sigCaught != 0 )
            RaiApi.Log( "Caught signal " + pubTest.sigCaught + ", quitting" );
          pubTest.close();
        }
      } catch ( RaiException e ) {
        RaiApi.Log( RaiApi.LVL_ERROR, e, "Main" );
      }
    } catch ( RaiException e ) {
      RaiApi.Log( RaiApi.LVL_ERROR, e, "Unable to load Rai API" );
    }
    RaiApi.Log( RaiApi.LVL_MINOR, "Finished" );
    return 0;
  }
}
