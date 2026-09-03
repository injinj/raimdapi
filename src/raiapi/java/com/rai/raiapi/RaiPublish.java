
/******************************************************************************
 *
 * Publisher Interface
 *
 *****************************************************************************/
package com.rai.raiapi;

import java.util.*;
import com.rai.raimsg.*;

public class RaiPublish {

  com.rai.raiapi2.RaiPublish pub;
  String  theSubject;
  int     seqNo;
  boolean isComplex;

  public
  RaiPublish( RaiSession session )
  {
    this( session, null );
  };

  public
  RaiPublish( RaiSession session, String subject )
  {
    this( session, subject, false );
  };

  public
  RaiPublish( RaiSession session, boolean isComplex )
  {
    this( session, null, isComplex );
  };

  public
  RaiPublish( RaiSession session, String subject, boolean isComplex )
  {
    try {
      this.pub = session.session2.CreatePublish();
      if ( ( RaiApi.apiType & 0xff ) == RaiApi.SASS2 )
        this.pub.SetPrefix( "_TIC." );
      this.theSubject = subject;
      this.isComplex  = isComplex;
      this.seqNo      = 0;
    } catch ( com.rai.raiexception.RaiException e ) {
    }
  };

  public void
  Publish( RaiMsg raiMsg ) throws RaiException
  {
    this.Publish( this.theSubject, raiMsg );
  };

  public void
  Publish( String subject, RaiMsg raiMsg ) throws RaiException
  {
    try {
      if ( ! this.isComplex ) {
        raiMsg.UpdateUShort( "SEQ_NO", (short) this.seqNo );
        this.seqNo++;
      }
      this.pub.Publish( subject, raiMsg, 0 );
    } catch ( com.rai.raiexception.RaiException e ) {
      throw RaiApi.getException( e, "Publish" );
    }
  };

  public void
  Publish( byte buffer[], int size) throws RaiException
  {
    this.Publish( this.theSubject, buffer, size );
  }

  public void
  Publish( String subject, byte buffer[], int size ) throws RaiException
  {
    try {
      this.pub.Publish( subject, buffer, 0, size, 0 );
    } catch ( com.rai.raiexception.RaiException e ) {
      throw RaiApi.getException( e, "Publish" );
    }
  };

  public void
  Ioctl( int parameter, String value)
  {
    switch ( parameter ) {
      case RaiApi.SetSubject:
        this.theSubject = value;
        break;
    }
  };
}
