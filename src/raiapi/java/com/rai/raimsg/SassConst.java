package com.rai.raimsg;

/** This is a collection of constants that are used by applications which deal
 * with SASS message formats.  The most important are the MSG_TYPE and
 * REC_STATUS constants, which provide control and status information for
 * applications that use SASS protocols.  There are usually 4 fields in the
 * header of SASS messages.
 * <p><table border="1">
 * <caption>SASS/SASS2 Control Fields</caption>
 * <tr><th>Field</th><th>Description</th></tr>
 * <tr><td>MSG_TYPE</td><td>Determine the type of the message. Provides
 * information about how the receiving application should interpret the
 * message.  These values are defined below.
 * </td></tr>
 * <tr><td>REC_TYPE</td><td>The class ID (FormClass) of the message. If the
 * MSG_TYPE is an INITIAL then it is a complete image of the record, otherwise
 * it will be a sub-set of the fields.
 * </td></tr>
 * <tr><td>SEQ_NO</td><td>Increases with every message. The granularity of the
 * increment depends on the sending application. In some cases it increases on
 * a subject by subject basis while in others it increases across all messages
 * from the source. It is primarily used as a high level gap detection
 * mechanism between the data source and the Rai Cache.
 * </td></tr>
 * <tr><td>REC_STATUS</td><td>Contains an enumerated value indicating the
 * status of the image. It is usually 0, STATUS_OK. It should be checked to
 * detect errors in the data stream or a failed subscription.  These values
 * are defined below.
 * </td></tr>
 * </table>
 * <p><table border="1">
 * <caption>The MSG_TYPE constants</caption>
 * <tr><th>Name</th><th>Value</th><th>Description</th></tr>
 * <tr><td>VERIFY</td><td>0</td><td>Verify message is processed the same as an
 * update but may contain the current state of fields that have not changed
 * </td></tr>
 * <tr><td>UPDATE</td><td>1</td><td>Update message will contain only those
 * fields that have changed since the last message </td></tr>
 * <tr><td>CORRECT</td><td>2</td><td>Identical to UPDATE</td></tr>
 * <tr><td>CLOSING</td><td>3</td><td>Identical to UPDATE</td></tr>
 * <tr><td>DROP</td><td>4</td><td>Message indicating that the subscription is
 * no longer valid and should be closed</td></tr>
 * <tr><td>AGGREGATE</td><td>5</td><td>Identical to UPDATE</td></tr>
 * <tr><td>STATUS</td><td>6</td><td>Identical to VERIFY</td></tr>
 * <tr><td>CANCEL</td><td>7</td><td>Identical to UPDATE</td></tr>
 * <tr><td>INITIAL</td><td>8</td><td>Initial image for subject. Will contain
 * current values for all fields</td></tr>
 * <tr><td>TRANSIENT</td><td>9</td><td>Information message that may change the
 * status of a subscription</td></tr>
 * <tr><td>DERIVED</td><td>10</td><td>Identical to VERIFY</td></tr>
 * <tr><td>DELETE</td><td>11</td><td>GSM-&gt;TIC (similar to DROP)</td></tr>
 * <tr><td>SUBREINIT</td><td>12</td><td>GSM-&gt;TIC + TIC converts to INITIAL so API resets subscription</td></tr>
 * <tr><td>SNAPSHOT</td><td>13</td><td>Indicates snapshot data msg type</td></tr>
 * <tr><td>CONFIRM</td><td>14</td><td>FH-&gt;GSM confirm subscription request</td></tr>
 * <tr><td>BDS_CONFIRM</td><td>15</td><td>FH-&gt;GSM confirm broadscast subscription request</td></tr>
 * <tr><td>EDIT</td><td>16</td><td>TibMsg editing cmds of how to update data</td></tr>
 * <tr><td>EDIT_FORCE</td><td>17</td><td>Like EDIT but some cmds allowed to fail</td></tr>
 * <tr><td>RENAME</td><td>18</td><td>Indicates subject name is renamed</td></tr>
 * <tr><td>SERVICE_STATUS</td><td>19</td><td>Triarch source server-&gt;TIC, service up or down</td></tr>
 * <tr><td>CONTRIB_REPLY</td><td>20</td><td>Triarch source server-&gt;TIC, if dist bit on TIC-&gt;APP A contribution event</td></tr>
 * <tr><td>GROUP_STATUS</td><td>21</td><td>Triarch source server-&gt;TIC, if dist bit on TIC-&gt;APP Group status for grouped subs</td></tr>
 * <tr><td>GROUP_MERGE</td><td>22</td><td>Triarch source server-&gt;TIC, if dist bit on TIC-&gt;APP Two groups merge</td></tr>
 * <tr><td>GROUP_CHANGE</td><td>23</td><td>Triarch source server-&gt;TIC, if dist bit on TIC-&gt;APP An individual sub group number is changed</td></tr>
 * <tr><td>INITIAL_PASS_THRU</td><td>24</td><td>FH -&gt; TIC, if subscriber(s) then passed through the TIC (without caching) as a VERIFY, else TIC silently ignores</td></tr>
 * <tr><td>UPDATE_PASS_THRU</td><td>25</td><td>FH -&gt; TIC, if subscriber(s) then passed through the TIC (without caching) as a UPDATE, else TIC silently ignores</td></tr>
 * <tr><td>INITIAL_AGGREGATE</td><td>26</td><td>Init cached message, don't forward (INITIAL)</td></tr>
 * <tr><td>UPDATE_AGGREGATE</td><td>27</td><td>Update cached message, don't forward (UPDATE)</td></tr>
 * <tr><td>FINISH_AGGREGATE</td><td>28</td><td>Update cached message, forward cached (VERIFY</td></tr>
 * </table>
 * <p><table border="1">
 * <caption>The REC_STATUS constants</caption>
 * <tr><th>Name</th><th>Value</th><th>Description</th></tr>
 * <tr><td>STATUS_OK</td><td>0</td><td>Normal status, ok status</td></tr>
 * <tr><td>STATUS_BAD_NAME</td><td>1</td><td>No such subject available from the publisher</td></tr>
 * <tr><td>STATUS_BAD_LINE</td><td>2</td><td>Communication line to the publisher went down</td></tr>
 * <tr><td>STATUS_CACHE_FULL</td><td>3</td><td>Publisher's limited cache capacity is now full</td></tr>
 * <tr><td>STATUS_PERMISSION_DENIED</td><td>4</td><td>feed denied permission (not site's sysadm denied)</td></tr>
 * <tr><td>STATUS_PREEMPTED</td><td>5</td><td>Bumped out by cache preemption algorithms</td></tr>
 * <tr><td>STATUS_BAD_ACCESS</td><td>6</td><td>Feed specific failures</td></tr>
 * <tr><td>STATUS_TEMP_UNAVAIL</td><td>7</td><td>When publisher will take a while to service the subscription</td></tr>
 * <tr><td>STATUS_REASSIGN</td><td>8</td><td>When GSM is reassigning a subject to another feed</td></tr>
 * <tr><td>STATUS_NOSUBSCRIBERS</td><td>9</td><td>when all subscribers have gone away</td></tr>
 * <tr><td>STATUS_EXPIRED</td><td>10</td><td>When a previously existing subject has disappeared (options expired etc)</td></tr>
 * <tr><td>STATUS_TIC_DOWN</td><td>11</td><td>The TIC where the current subject is in is down</td></tr>
 * <tr><td>STATUS_FEED_DOWN</td><td>12</td><td>The feed where the current subject is in is down</td></tr>
 * <tr><td>STATUS_GSM_DOWN</td><td>14</td><td>The gsm service where the current subject is in is down</td></tr>
 * <tr><td>STATUS_SUBSC_DENIED</td><td>15</td><td>The subscription was denied by the feedhandler because it was not configured to service it</td></tr>
 * <tr><td>STATUS_SUBSC_TEMP_DENIED</td><td>16</td><td>The subscription was dropped by feedhandler because it the value in it was stale/non-deterministic, user should retry</td></tr>
 * <tr><td>STATUS_NOT_FOUND</td><td>17</td><td>Sent from TIC to client if instrument not in TIC for broadcast feeds</td></tr>
 * <tr><td>STATUS_STALE_VALUE</td><td>18</td><td>Value is stale, consider stale until updated</td></tr>
 * <tr><td>STATUS_RELOCATE</td><td>19</td><td>Feed tells GSM to relocate this subscription to a better source, include itself, if possible</td></tr>
 * <tr><td>STATUS_ENTITLEMENT_DENIED</td><td>20</td><td>This is used to indicate a permission denied status code for an open subscription</td></tr>
 * <tr><td>STATUS_REC_OVERFLOW</td><td>21</td><td>This is used by the server based entitlements infrastructure, if the group or user entitlement record overflows the client will receive this status code</td></tr>
 * <tr><td>STATUS_TIC_TUPLE_FAIL</td><td>22</td><td>This is used by the server based entitlements infrastructure, if the creation of group or user entitlement in the TIC fails, the client will receive this status code</td></tr>
 * <tr><td>STATUS_ENTITLEMENT_MIGRATED</td><td>23</td><td>This is used to indicate a permission denied event due to entitlements migration (to other host)</td></tr>
 * <tr><td>STATUS_CI_DISCONNECTED</td><td>24</td><td>ciServer disconnect</td></tr>
 * <tr><td>STATUS_CI_DIAG_START</td><td>25</td><td>ciServer diagnosis start</td></tr>
 * <tr><td>STATUS_NO_CACHED_DATA</td><td>26</td><td>No cached data for subject</td></tr>
 * <tr><td>STATUS_NO_REPLY</td><td>27</td><td>SASS3 only, used to show the subscription has not yet received any reply from the TIC or feed. This is needed because the SASS3 API allows the user to query the subscription handle for the current message type and status, and they may do so before the subscription has received any reply.</td></tr>
 * <tr><td>STATUS_TMF_DOWN</td><td>28</td><td>TMF Down</td></tr>
 * <tr><td>STATUS_TPT_DISCONNECTED</td><td>29</td><td>The connection oriented transport disconnected</td></tr>
 * <tr><td>STATUS_TIMEOUT</td><td>30</td><td>The subscription timed out</td></tr>
 * <tr><td>STATUS_PERIODIC_SNAPSHOT</td><td>64</td><td>This subject will be updated periodically</td></tr>
 * <tr><td>STATUS_FEED_UP</td><td>65</td><td>The feed is up</td></tr>
 * <tr><td>STATUS_HL_ROUTER_DOWN</td><td>66</td><td>The news headline router is down</td></tr>
 * <tr><td>STATUS_DQA_SUSPECT</td><td>67</td><td>The DQA monitor process is not heartbeating</td></tr>
 * <tr><td>STATUS_DQA_ACTIVE</td><td>68</td><td>The DQA monitor process is back after being down</td></tr>
 * <tr><td>STATUS_GSM_UP</td><td>69</td><td>The GSM is back up after begin down</td></tr>
 * <tr><td>STATUS_HL_ROUTER_UP</td><td>71</td><td>The news headline router is up after being down</td></tr>
 * <tr><td>STATUS_TIC_UP</td><td>72</td><td>The TIC is up</td></tr>
 * <tr><td>STATUS_FEED_SWITCHOVER</td><td>73</td><td>If FT backup feeds switch non-transparently</td></tr>
 * <tr><td>STATUS_DATA_SUSPECT</td><td>74</td><td>This record from tic is unreliable, the source may be in rebuilding cycl</td></tr>
 * <tr><td>STATUS_RECAP</td><td>75</td><td>This status code is used to indicate the receiving record is a recap/refresh record, not a real update/transaction, if application is to accumulate updates for historical purpose, the record should be ignore</td></tr>
 * <tr><td>STATUS_CI_RECONNECTED</td><td>76</td><td>ciServer connected</td></tr>
 * <tr><td>STATUS_CI_DIAG_END</td><td>77</td><td>ciServer diagnosis ended</td></tr>
 * <tr><td>STATUS_RECOVER_SUBSC_DENIED</td><td>80</td><td>The subscription recovered is denied</td></tr>
 * <tr><td>STATUS_CONTRIB_ACK</td><td>81</td><td>Indicates an acknowledge status for contribution event</td></tr>
 * <tr><td>STATUS_CONTRIB_NACK</td><td>82</td><td>Indicates an not acknowledge status for contribution event</td></tr>
 * <tr><td>STATUS_TMF_UP</td><td>83</td><td>TMF Up</td></tr>
 * <tr><td>STATUS_TPT_CONNECTED</td><td>84</td><td>Transport now connected</td></tr>
 * <tr><td>STATUS_FEED_NOT_ACCEPTING</td><td>85</td><td>Feed state up, but not accepting new subscriptions</td></tr>
 * </table>
 */
public class SassConst {
  final static public int MDSS_CHANNEL  = 0x12; /* 18 */ /* channel protocols */
  final static public int MSA_CHANNEL   = 0x13; /* 19 */
  final static public int SASS_CHANNEL  = 0x14; /* 20 */
  final static public int TIC_CHANNEL   = 0x15; /* 21 */

  final static public int SASS_TOKEN    = 0x11; /* 17 */ /* service tokens */
  final static public int TIC_TOKEN     = 0x15; /* 21 */
  final static public int MSA_TOKEN     = 0x27; /* 39 */
  final static public int SASS_WILDCARD = 0x3fffff;

  final static public int MAX_SUBJECT_LEN = ( ( 255 * 4 + 9 ) + 7 ) & ~7;
  final static public int MAX_RV_SEGMENTS = 32;

  final static public short SASS3_SUB_MAGIC = 23176;
  final static public short SASS3_PUB_MAGIC = 23177;

  /*enum SubType */
  /** One-time image request */
  final static public int SNAPSHOT_FLAG       = 0x01;
  /** Subscribe, send updates */
  final static public int SUBSCRIBE_FLAG      = 0x02;
  /** Send initial image */
  final static public int INITIAL_VALUES_FLAG = 0x04;
  /** Stop sending updates */
  final static public int UNSUBSCRIBE_FLAG    = 0x08;
  /** Refresh image */
  final static public int REFRESH_FLAG        = 0x10;
  /** Reassert subscription */
  final static public int RESUBSCRIBE_FLAG    = 0x80;
  /** Is entitled to subscription */
  final static public int ENTITLED_FLAG       = 0x4000;

  /*enum Sass3Indicator */
  final static public int IND_NONE     = 0;
  final static public int IND_UPDATE   = 0x01;
  final static public int IND_INITIAL  = 0x02;
  final static public int IND_OOB      = 0x04;
  final static public int IND_RESET    = 0x08;
  final static public int IND_SNAPSHOT = 0x10;
  final static public int IND_ACK      = 0x20;
 
 /*enum MsgType */
 /** Verify message is processed the same as an update but may contain the current
  * state of fields that have not changed */
  final static public short VERIFY         = 0; 
  /** Update message will contain only those fields that have changed since the
   * last message */
  final static public short UPDATE         = 1; 
  /** Identical to UPDATE */
  final static public short CORRECT        = 2; 
  /** Identical to UPDATE */
  final static public short CLOSING        = 3; 
  /** Message indicating that the subscription is no longer valid and should be
   * closed */
  final static public short DROP           = 4; 
  /** Identical to UPDATE */
  final static public short AGGREGATE      = 5; 
  /** Identical to VERIFY */
  final static public short STATUS         = 6; 
  /** Identical to UPDATE */
  final static public short CANCEL         = 7; 
  /** Initial image for subject. Will contain current values for all fields. */
  final static public short INITIAL        = 8; 
  /** Information message that may change the status of a subscription. */
  final static public short TRANSIENT      = 9; 
  /** Identical to VERIFY */
  final static public short DERIVED        = 10;
  /** GSM-&gt;TIC (similar to DROP) */
  final static public short DELETE         = 11;
  /** GSM-&gt;TIC + TIC converts to INITIAL so API resets subscription */
  final static public short SUBREINIT      = 12;
  /** indicates snapshot data msg type */
  final static public short SNAPSHOT       = 13;
  /** FH-&gt;GSM confirm subscription request */
  final static public short CONFIRM        = 14;
  /** FH-&gt;GSM confirm broadscast subscription request */
  final static public short BDS_CONFIRM    = 15;
  /** TibMsg editing cmds of how to update data */
  final static public short EDIT           = 16;
  /** like EDIT but some cmds allowed to fail */
  final static public short EDIT_FORCE     = 17;
  /** indicates subject name is renamed */
  final static public short RENAME         = 18;
  /** Triarch source server-&gt;TIC, service up or down */
  final static public short SERVICE_STATUS = 19;
  /** Triarch source server-&gt;TIC, if dist bit on TIC-&gt;APP A contribution
   * event */
  final static public short CONTRIB_REPLY  = 20;
  /** Triarch source server-&gt;TIC, if dist bit on TIC-&gt;APP Group status
   * for grouped subs */
  final static public short GROUP_STATUS   = 21;
  /** Triarch source server-&gt;TIC, if dist bit on TIC-&gt;APP Two groups
   * merge */
  final static public short GROUP_MERGE    = 22;
  /** Triarch source server-&gt;TIC, if dist bit on TIC-&gt;APP An individual
   * sub group number is changed */
  final static public short GROUP_CHANGE   = 23;
  /** FH -&gt; TIC, if subscriber(s) then passed through the TIC (without
   * caching) as a VERIFY, else TIC silently ignores */
  final static public short INITIAL_PASS_THRU = 24;
  /** FH -&gt; TIC, if subscriber(s) then passed through the TIC (without
   * caching) as a UPDATE, else TIC silently ignores */
  final static public short UPDATE_PASS_THRU  = 25;
  /** Init cached message, don't forward (INITIAL) */
  final static public short INITIAL_AGGREGATE = 26;
  /** Update cached message, don't forward (UPDATE) */
  final static public short UPDATE_AGGREGATE  = 27;
  /** Update cached message, forward cached (VERIFY)*/
  final static public short FINISH_AGGREGATE  = 28;

  /** String identifiers for MSG_TYPE enumeration */
  final static public String [] MsgTypeStrings = {
    "VERIFY" /* 0 */ ,
    "UPDATE" /* 1 */ ,
    "CORRECT" /* 2 */ ,
    "CLOSING" /* 3 */ ,
    "DROP" /* 4 */ ,
    "AGGREGATE" /* 5 */ ,
    "STATUS" /* 6 */ ,
    "CANCEL" /* 7 */ ,
    "INITIAL" /* 8 */ ,
    "TRANSIENT" /* 9 */ ,
    "DERIVED" /* 10 */ ,
    "DELETE" /* 11 */ ,
    "SUBREINIT" /* 12 */ ,
    "SNAPSHOT" /* 13 */ ,
    "CONFIRM" /* 14 */ ,
    "BDS_CONFIRM" /* 15 */ ,
    "EDIT" /* 16 */ ,
    "EDIT_FORCE" /* 17 */ ,
    "RENAME" /* 18 */ ,
    "SERVICE_STATUS" /* 19 */ ,
    "CONTRIB_REPLY" /* 20 */ ,
    "GROUP_STATUS" /* 21 */ ,
    "GROUP_MERGE" /* 22 */ ,
    "GROUP_CHANGE" /* 23 */ ,
    "INITIAL_PASS_THRU" /* 24 */ ,
    "UPDATE_PASS_THRU" /* 25 */ ,
    "INITIAL_AGGREGATE" /* 26 */ ,
    "UPDATE_AGGREGATE" /* 27 */ ,
    "FINISH_AGGREGATE" /* 28 */
  };

  /** Invalid message type */
  final static public short NO_TYPE  = 0x7fff;
  /** Msg has no message type */
  final static public short MAX_TYPE = -1;

  /** Convert a message type to a string (ex: 8 = "INITIAL")
   * @return Message type string or number string when not found. */
  final public static String msgTypeToString( short msgType ) {
    if ( msgType < 0 || msgType >= MsgTypeStrings.length )
      return Short.toString( msgType );
    return MsgTypeStrings[ msgType ];
  }
  /** Convert a string to a message type (ex: "INITIAL" = 8)
   * @return Message type or MAX_TYPE if not found. */
  final public static short stringToMsgType( String s ) {
    if ( s != null ) {
      /* first values (VERIFY,UPDATE) are most common usage,
       * if a program access is not common, use a non-linear lookup */
      for ( short msgType = 0; msgType < MsgTypeStrings.length; msgType++ )
        if ( s.equals( MsgTypeStrings[ msgType ] ) )
          return msgType;
    }
    return MAX_TYPE;
  }

  /*enum RecStatus */
  /** Normal status, ok status */
  final static public short STATUS_OK                   = 0;
  /** No such subject available from the publisher */
  final static public short STATUS_BAD_NAME             = 1; 
  /** Communication line to the publisher went down */
  final static public short STATUS_BAD_LINE             = 2; 
  /** Publisher's limited cache capacity is now full */
  final static public short STATUS_CACHE_FULL           = 3; 
  /** feed denied permission (not site's sysadm denied) */
  final static public short STATUS_PERMISSION_DENIED    = 4; 
  /** Bumped out by cache preemption algorithms */
  final static public short STATUS_PREEMPTED            = 5; 
  /** Feed specific failures */
  final static public short STATUS_BAD_ACCESS           = 6; 
  /** When publisher will take a while to service the subscription */
  final static public short STATUS_TEMP_UNAVAIL         = 7; 
  /** When GSM is reassigning a subject to another feed */
  final static public short STATUS_REASSIGN             = 8; 
  /** when all subscribers have gone away */
  final static public short STATUS_NOSUBSCRIBERS        = 9; 
  /** When a previously existing subject has disappeared (options expired etc)
   */
  final static public short STATUS_EXPIRED              = 10;
  /** The TIC where the current subject is in is down */
  final static public short STATUS_TIC_DOWN             = 11;
  /** The feed where the current subject is in is down */
  final static public short STATUS_FEED_DOWN            = 12;
  /** The gsm service where the current subject is in is down */
  final static public short STATUS_GSM_DOWN             = 14;
  /** The subscription was denied by the feedhandler because it was not
   * configured to service it */
  final static public short STATUS_SUBSC_DENIED         = 15;
  /** The subscription was dropped by feedhandler because it the value in it
   * was stale/non-deterministic, user should retry */
  final static public short STATUS_SUBSC_TEMP_DENIED    = 16;
  /** Sent from TIC to client if instrument not in TIC for broadcast feeds */
  final static public short STATUS_NOT_FOUND            = 17;
  /** Value is stale, consider stale until updated */
  final static public short STATUS_STALE_VALUE          = 18;
  /** Feed tells GSM to relocate this subscription to a better source, include
   * itself, if possible */
  final static public short STATUS_RELOCATE             = 19;
  /** This is used to indicate a permission denied status code for an open
   * subscription */
  final static public short STATUS_ENTITLEMENT_DENIED   = 20;
  /** This is used by the server based entitlements infrastructure, if the
   * group or user entitlement record overflows the client will receive this
   * status code */
  final static public short STATUS_REC_OVERFLOW         = 21;
  /** This is used by the server based entitlements infrastructure, if the
   * creation of group or user entitlement in the TIC fails, the client will
   * receive this status code */
  final static public short STATUS_TIC_TUPLE_FAIL       = 22;
  /** This is used to indicate a permission denied event due to entitlements
   * migration (to other host) */
  final static public short STATUS_ENTITLEMENT_MIGRATED = 23;
  /** ciServer disconnect */
  final static public short STATUS_CI_DISCONNECTED      = 24;
  /** ciServer diagnosis start */
  final static public short STATUS_CI_DIAG_START        = 25;
  /** No cached data for subject */
  final static public short STATUS_NO_CACHED_DATA       = 26;
  /** SASS3 only, used to show the subscription has not yet received any reply
   * from the TIC or feed. This is needed because the SASS3 API allows the user
   * to query the subscription handle for the current message type and status,
   * and they may do so before the subscription has received any reply. */
  final static public short STATUS_NO_REPLY             = 27;
  final static public short STATUS_TMF_DOWN             = 28;
  final static public short STATUS_TPT_DISCONNECTED     = 29;
  final static public short STATUS_TIMEOUT              = 30;
  /** This subject will be updated periodically */
  final static public short STATUS_PERIODIC_SNAPSHOT    = 64;
  final static public short STATUS_FEED_UP              = 65;
  /** The news headline router is down */
  final static public short STATUS_HL_ROUTER_DOWN       = 66;
  /** The DQA monitor process is not heartbeating */
  final static public short STATUS_DQA_SUSPECT          = 67;
  /** The DQA monitor process is back after being down */
  final static public short STATUS_DQA_ACTIVE           = 68;
  /** The GSM is back up after begin down */
  final static public short STATUS_GSM_UP               = 69;
  /** The news headline router is up after being down */
  final static public short STATUS_HL_ROUTER_UP         = 71;
  final static public short STATUS_TIC_UP               = 72;
  /** If FT backup feeds switch non-transparently */
  final static public short STATUS_FEED_SWITCHOVER      = 73;
  /** This record from tic is unreliable, the source may be in rebuilding
   * cycle*/
  final static public short STATUS_DATA_SUSPECT         = 74;
  /** This status code is used to indicate the receiving record is a
   * recap/refresh record, not a real update/transaction, if application is to
   * accumulate updates for historical purpose, the record should be ignore */
  final static public short STATUS_RECAP                = 75;
  /** ciServer connected */
  final static public short STATUS_CI_RECONNECTED       = 76;
  /** ciServer diagnosis ended */
  final static public short STATUS_CI_DIAG_END          = 77;
  final static public short STATUS_RECOVER_SUBSC_DENIED = 80;
  /** Indicates an acknowledge status for contribution event */
  final static public short STATUS_CONTRIB_ACK          = 81;
  /** Indicates an not acknowledge status for contribution event */
  final static public short STATUS_CONTRIB_NACK         = 82;
  final static public short STATUS_TMF_UP               = 83;
  /** Transport now connected */
  final static public short STATUS_TPT_CONNECTED        = 84;
  /** Feed state up, but not accepting new subscriptions */
  final static public short STATUS_FEED_NOT_ACCEPTING   = 85;

  /** String identifiers for REC_STATUS enumeration */
  final static public String [] RecStatusStrings = {
    "OK" /* 0 */ ,
    "BAD_NAME" /* 1 */ ,
    "BAD_LINE" /* 2 */ ,
    "CACHE_FULL" /* 3 */ ,
    "PERMISSION_DENIED" /* 4 */ ,
    "PREEMPTED" /* 5 */ ,
    "BAD_ACCESS" /* 6 */ ,
    "TEMP_UNAVAIL" /* 7 */ ,
    "REASSIGN" /* 8 */ ,
    "NOSUBSCRIBERS" /* 9 */ ,
    "EXPIRED" /* 10 */ ,
    "TIC_DOWN" /* 11 */ ,
    "FEED_DOWN" /* 12 */ ,
    "13",
    "GSM_DOWN" /* 14 */ ,
    "SUBSC_DENIED" /* 15 */ ,
    "SUBSC_TEMP_DENIED" /* 16 */ ,
    "NOT_FOUND" /* 17 */ ,
    "STALE_VALUE" /* 18 */ ,
    "RELOCATE" /* 19 */ ,
    "ENTITLEMENT_DENIED" /* 20 */ ,
    "REC_OVERFLOW" /* 21 */ ,
    "TIC_TUPLE_FAIL" /* 22 */ ,
    "ENTITLEMENT_MIGRATED" /* 23 */ ,
    "CI_DISCONNECTED" /* 24 */ ,
    "CI_DIAG_START" /* 25 */ ,
    "NO_CACHED_DATA" /* 26 */ ,
    "NO_REPLY" /* 27 */ ,
    "TMF_DOWN" /* 28 */ ,
    "TPT_DISCONNECTED" /* 29 */ ,
    "TIMEOUT" /* 30 */ ,
    "31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
    "41", "42", "43", "44", "45", "46", "47", "48", "49", "50",
    "51", "52", "53", "54", "55", "56", "57", "58", "59", "60",
    "61", "62", "63",
    "PERIODIC_SNAPSHOT" /* 64 */ ,
    "FEED_UP" /* 65 */ ,
    "HL_ROUTER_DOWN" /* 66 */ ,
    "DQA_SUSPECT" /* 67 */ ,
    "DQA_ACTIVE" /* 68 */ ,
    "GSM_UP" /* 69 */ ,
    "70",
    "HL_ROUTER_UP" /* 71 */ ,
    "TIC_UP" /* 72 */ ,
    "FEED_SWITCHOVER" /* 73 */ ,
    "DATA_SUSPECT" /* 74 */ ,
    "RECAP" /* 75 */ ,
    "CI_RECONNECTED" /* 76 */ ,
    "CI_DIAG_END" /* 77 */ ,
    "78", "79",
    "RECOVER_SUBSC_DENIED" /* 80 */ ,
    "CONTRIB_ACK" /* 81 */ ,
    "CONTRIB_NACK" /* 82 */ ,
    "TMF_UP" /* 83 */ ,
    "TPT_CONNECTED" /* 84 */ ,
    "FEED_NOT_ACCEPTING" /* 85 */
  };

  /** Message has no status */
  final static public short MAX_STATUS = -1;

  /** Convert a rec status to a string (ex: 0 = "OK")
   * @return Rec status string or number string when not found. */
  final public static String recStatusToString( short recStatus ) {
    if ( recStatus < 0 || recStatus >= RecStatusStrings.length )
      return Short.toString( recStatus );
    return RecStatusStrings[ recStatus ];
  }
  /** Convert a string to a rec status (ex: "OK" = 0)
   * @return Rec status or MAX_STATUS if not found. */
  final public static short stringToRecStatus( String s ) {
    if ( s != null ) {
      for ( short recStatus = 0; recStatus < RecStatusStrings.length;
            recStatus++ )
        if ( s.equals( RecStatusStrings[ recStatus ] ) )
          return recStatus;
    }
    return MAX_STATUS;
  }
};

