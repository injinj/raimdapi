
/******************************************************************************
 *
 * Subscriber Interface
 *
 *****************************************************************************/
package com.rai.raiapi;

import java.util.*;
import com.rai.raimsg.*;


public class RaiSubscribe {
  com.rai.raiapi2.RaiSubscribe sub;
  SubHandle cl;

  public
  RaiSubscribe( RaiSession session,  String subject,
                RaiCallback callback,  Object closure ) throws RaiException
  {
    this( session, subject, callback, closure, RaiApi.BOTH );
  }

  public
  RaiSubscribe( RaiSession session,  String subject,
                RaiCallback callback,  Object closure,
                int parm ) throws RaiException
  {
    try {
      this.cl = new SubHandle( callback, closure, subject );
      this.sub = session.defaultQueue2.CreateSubscribe( session, this.cl );
      if ( parm == RaiApi.BOTH )
        parm = com.rai.raiapi2.RaiSubscribe.BOTH;
      else if ( parm == RaiApi.UPDATE )
        parm = com.rai.raiapi2.RaiSubscribe.UPDATE;
      else if ( parm == RaiApi.SNAP )
        parm = com.rai.raiapi2.RaiSubscribe.SNAP;
      this.sub.Start( subject, parm );
    } catch ( com.rai.raiexception.RaiException e ) {
      throw RaiApi.getException( e, "CreateSubscribe" );
    }
  }

  public void
  Ioctl( int parameter, String value )
  {
  }

  public void
  Destroy() throws RaiException
  {
    try {
      com.rai.raiapi2.RaiSubscribe s = this.sub;
      if ( s != null ) {
        this.sub = null;
        this.cl = null;
        s.Cancel();
      }
    } catch ( com.rai.raiexception.RaiException e ) {
      throw RaiApi.getException( e, "Cancel" );
    }
  }
}
