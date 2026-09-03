
/******************************************************************************
 *
 * Session Interface
 *
 *****************************************************************************/
package com.rai.raiapi;

import java.util.*;
import com.rai.raimsg.*;
import java.lang.*;
import java.io.*;

public class RaiSession implements com.rai.raiapi2.RaiMsgCallback,
                                   com.rai.raiapi2.RaiTimerCallback,
                                   com.rai.raiapi2.RaiDataLossCallback,
                                   Runnable {

  com.rai.raiapi2.RaiSession session2;
  com.rai.raiapi2.RaiQueue   defaultQueue2;

  public void run() {
    try {
      RaiApi.RaiMainloop( this );
    } catch ( RaiException e ) {
      System.out.println( "Run caught exception: " + e );
    }
  }

  /* RaiDataLossCallback */
  public void onConnection( com.rai.raiapi2.RaiConnectionEvent event,
                            Object cl ) {
    RaiApi.api2.PrintLog( com.rai.raiapi2.RaiApi.LVL_MINOR, "onConnection: " +
                          event.description );
  }

  public void onDataLoss( com.rai.raiapi2.RaiDataLossEvent event,  Object cl ) {
    RaiApi.api2.PrintLog( com.rai.raiapi2.RaiApi.LVL_ERROR, "onDataLoss: " +
                          event.description );
    if ( event.connectionLoss && event.connectionCount == 0 ) {
      try {
        this.session2.NotifyStatus( SassConst.TRANSIENT,
                                    SassConst.STATUS_TPT_DISCONNECTED );
      } catch ( com.rai.raiexception.RaiException e ) {
        RaiApi.api2.PrintLog( com.rai.raiapi2.RaiApi.LVL_ERROR, e,
                              "NotifyStatus" );
      }
    }
  }

  public
  RaiSession() throws RaiException
  {
    DoRaiSession( (String) null, (String) null, (String) null );
  }

  public
  RaiSession( String host, int port ) throws RaiException
  {
    DoRaiSession( ((Integer) port).toString(), host, null );
  }

  public
  RaiSession( String svcname, String netname, String dmnname,
              boolean createRVDispatcher ) throws RaiException
  {
    DoRaiSession( svcname, netname, dmnname );
    if ( createRVDispatcher )
      (new Thread( this )).start();
  }

  public
  RaiSession( String svcname, String netname,
              String dmnname ) throws RaiException
  {
    DoRaiSession( svcname, netname, dmnname );
  }

  public void
  DoRaiSession( String svcname, String netname,
                String dmnname ) throws RaiException
  {
    try {
      RaiApi.RaiOpen2( dmnname );
      com.rai.raiapi2.Args args = new com.rai.raiapi2.Args();
      RaiApi.api2.GetArgs( args );

      if ( RaiApi.apiType == RaiApi.caprApi ) {
        String addr = null;
        if ( netname != null ) {
          if ( svcname != null )
            addr = netname + ":" + svcname;
          else
            addr = netname;
        }
        else
          addr = ":" + svcname;
        args.setString( "address", addr );
      }
      else if ( RaiApi.apiType == RaiApi.tibrvApi ) {
        if ( dmnname != null && dmnname.equals( "tibrv" ) )
          dmnname = null;
        args.setString( "daemon", dmnname );
      }
      if ( RaiApi.apiType != RaiApi.caprApi ) {
        args.setString( "network", netname );
        args.setString( "service", svcname );
      }
      if ( RaiApi.apiType == RaiApi.sass2Api )
        args.setBoolean( "sass2", true );
      else if ( RaiApi.apiType == RaiApi.sass3Api )
        args.setBoolean( "sass2", false );

      RaiApi.api2.ParseArgs( args );

      this.session2 = RaiApi.api2.CreateSession();
      this.session2.SetDataLossCB( this );
      this.session2.Start();
      this.defaultQueue2 = this.session2.CreateQueue();
      RaiApi.defaultSession = this;
    } catch ( com.rai.raiexception.RaiException e ) {
      throw RaiApi.getException( e, "CreateSession" );
    }
  }

  public void
  onMsg( com.rai.raiapi2.RaiMsgEvent event2,  RaiMsg msg,  Object closure )
  {
    if ( this.defaultQueue2 != null ) {
      SubHandle handle = (SubHandle) closure;
      RaiEvent  event  = new RaiEvent( this );
      event.receivedSubject   = event2.subject;
      event.subscribedSubject = handle.theSubject;
      handle.cb.onMsg( event, msg, handle.arg );
    }
  }

  public void
  onTimer( com.rai.raiapi2.RaiTimer timer2,  Object closure )
  {
    if ( this.defaultQueue2 != null ) {
      TimerHandle handle = (TimerHandle) closure;
      handle.cb.onTimer( this, handle.arg );
    }
  }

  public void
  Destroy()
  {
    com.rai.raiapi2.RaiSession s = this.session2;
    com.rai.raiapi2.RaiQueue   q = this.defaultQueue2;

    this.defaultQueue2 = null;
    this.session2      = null;
    if ( q != null ) {
      q.Destroy();
      s.Destroy();
    }
  }
}

