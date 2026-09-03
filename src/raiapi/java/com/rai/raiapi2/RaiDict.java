package com.rai.raiapi2;
import com.rai.raiexception.RaiException;

/** The RaiDict class provides the methods necessary for requesting a
 * dictionary from the network and unpacking it. The result creates a global
 * dictionary structure that can be used across multiple sessions and APIs.
 * Since programs which use a dictionary cannot parse messages without first
 * installing it, this is the first network operation that occurs.
 *
 * @see RaiSession#CreateDict
 */
public class RaiDict {
  long       dict;
  RaiSession session;

  /** Constructor used internally.
   * @see RaiSession#CreateDict */
  protected RaiDict( long d,  RaiSession s ) {
    this.dict    = d;
    this.session = s;
  }
  /** Destructor used internally. */
  protected void finalize() {
    Delete( this.dict );
  } 
  private static native void Delete( long dict );

  /** Load data dictionary from a dictionary provider (such as the Rai Cache)
   * on the configured transport. If dictSubject is NULL, the protocol will use
   * the default name for the dictionary.  If loadWait is true, then return
   * will block until the dictionary is loaded (up to timeoutSecs). If the
   * dictionary is not loaded and the timeout expires while loadWait is true,
   * an exception is thrown. If loadWait is false, then the application must
   * use InProgress() and HaveDict() to determine whether the dictionary call
   * to Load() was successful. When the dictionary is loaded, the global
   * DataDictionary is replaced with the new dictionary, invalidating the old
   * dictionary, if it existed.
   *
   * @param timeoutSecs The timeout period in seconds for the dictionary load
   * procedure to time out.
   * @param dictSubject The subject of the dictionary, should be null unless
   * the dictionary service is configured to listen on another subject.
   * @param loadWait Whether or not to return immediately (false) or to wait
   * for dictionary load to complete or timeoutSecs to expire (true).
   * @see RaiSession#CreateDict
   */
  public native void Load( int timeoutSecs,  String dictSubject,
                           boolean loadWait )   throws RaiException;
  /** Determine if the dictionary has been loaded.
   * @return True if dictionary has been loaded or false if has not been
   * loaded. */
  public native boolean HaveDict();
  /** Determine if the dictionary load is in progress.  Use this when passing
   * loadWait = false in the Load() call, otherwise Load() will wait while
   * InProgress() == true.
   * @return True if the dictionary has not been loaded and has not timed out. */
  public native boolean InProgress();
  /** Get the session which created this */
  public RaiSession GetSession() {
    return this.session;
  }
};
