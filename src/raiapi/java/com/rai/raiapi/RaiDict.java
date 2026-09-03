
/******************************************************************************
 *
 * Dictionary Interface Class
 *
 *****************************************************************************/
package com.rai.raiapi;

public class RaiDict {
  com.rai.raiapi2.RaiDict dict;

  public RaiDict() throws RaiException
  {
  }

  public synchronized void
  Load( RaiSession session, String dictSubject ) throws RaiException
  {
    if (session == null)
      throw new RaiException( RaiException.BAD_SESSION );

    RaiApi.haveDictionary = false;
    RaiApi.dictionaryLoadInProgress=true;
    try {
      this.dict = session.session2.CreateDict();
      this.dict.Load( RaiApi.DictTimeoutSeconds, null, true );
    } catch ( com.rai.raiexception.RaiException e ) {
      RaiApi.dictionaryLoadInProgress = false;
      throw RaiApi.getException( e, "CreateDict" );
    }
    RaiApi.haveDictionary = this.dict.HaveDict();
    RaiApi.dictionaryLoadInProgress = false;

    if ( ! RaiApi.haveDictionary )
      throw new RaiException( RaiException.BAD_DICT,
        RaiException.errString[ RaiException.BAD_DICT ], "RaiSession" );
  }
};

