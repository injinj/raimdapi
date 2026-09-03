/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__sass_const_h__
#define __rai_msg__sass_const_h__

#if ! defined( RAIMSG_DLL_EXP )
#if defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllimport)
#else
#define RAIMSG_DLL_EXP
#endif
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
#undef DELETE
#undef ERROR
#undef STATUS_TIMEOUT
#endif

namespace rai {
namespace SassConst {
  const unsigned int MDSS_CHANNEL  = 0x12; /* 18 */ /* channel protocols */
  const unsigned int MSA_CHANNEL   = 0x13; /* 19 */
  const unsigned int SASS_CHANNEL  = 0x14; /* 20 */
  const unsigned int TIC_CHANNEL   = 0x15; /* 21 */

  const unsigned int SASS_TOKEN    = 0x11; /* 17 */ /* service tokens */
  const unsigned int TIC_TOKEN     = 0x15; /* 21 */
  const unsigned int MSA_TOKEN     = 0x27; /* 39 */
  const unsigned int SASS_WILDCARD = 0x3fffff;

  const unsigned int MAX_SUBJECT_LEN = ( ( 255 * 4 + 9 ) + 7 ) & ~7U;
  const unsigned int MAX_RV_SEGMENTS = 32;

  const unsigned short SASS3_SUB_MAGIC = 23176;
  const unsigned short SASS3_PUB_MAGIC = 23177;

  enum SubType {
    SNAPSHOT_FLAG       = 0x01, /* One-time image request */
    SUBSCRIBE_FLAG      = 0x02, /* Subscribe, send updates */
    INITIAL_VALUES_FLAG = 0x04, /* Send initial image */
    UNSUBSCRIBE_FLAG    = 0x08, /* Stop sending updates */
    REFRESH_FLAG        = 0x10, /* Refresh image */
    RESUBSCRIBE_FLAG    = 0x80, /* Reassert subscription */
    ENTITLED_FLAG       = 0x4000 /* Is entitled to subscription */
  };

  enum Sass3Indicator {
    IND_NONE     = 0,
    IND_UPDATE   = 0x01,
    IND_INITIAL  = 0x02,
    IND_OOB      = 0x04,
    IND_RESET    = 0x08,
    IND_SNAPSHOT = 0x10,
    IND_ACK      = 0x20
  };

  enum MsgType {
    VERIFY         = 0,  /* Feed->FH, FH->TIC, if dist bit on TIC->APP */
    UPDATE         = 1,  /* Feed->FH, FH->TIC, if dist bit on TIC->APP */
    CORRECT        = 2,  /* identical to UPDATE */
    CLOSING        = 3,  /* identical to UPDATE */
    DROP           = 4,  /* Feed->FH, FH->GSM, GSM->TIC, TIC->APP */
    AGGREGATE      = 5,  /* identical to UPDATE */
    STATUS         = 6,  /* identical to VERIFY */
    CANCEL         = 7,  /* identical to UPDATE */
    INITIAL        = 8,  /* identical to VERIFY (except unconditional dist)
                            API treats as subscription reset, recover counts */
    TRANSIENT      = 9,  /* identical to UPDATE (except unconditional dist) */
    DERIVED        = 10, /* identical to VERIFY */
    DELETE         = 11, /* GSM->TIC (similar to DROP) */
    SUBREINIT      = 12, /* GSM->TIC + TIC converts to INITIAL so API resets
                            subscription */
    SNAPSHOT       = 13, /* indicates snapshot data msg type */
    CONFIRM        = 14, /* FH->GSM confirm subscription request */
    BDS_CONFIRM    = 15, /* FH->GSM confirm broadscast subscription request */
    EDIT           = 16, /* TibMsg editing cmds of how to update data */
    EDIT_FORCE     = 17, /* like EDIT but some cmds allowed to fail */
    RENAME         = 18, /* indicates subject name is renamed */
    SERVICE_STATUS = 19, /* Triarch source server->TIC, service up or down */
    CONTRIB_REPLY  = 20, /* Triarch source server->TIC, if dist bit on TIC->APP
                            A contribution event */
    GROUP_STATUS   = 21, /* Triarch source server->TIC, if dist bit on TIC->APP
                            Group status for grouped subs */
    GROUP_MERGE    = 22, /* Triarch source server->TIC, if dist bit on TIC->APP
                            Two groups merge */
    GROUP_CHANGE   = 23, /* Triarch source server->TIC, if dist bit on TIC->APP
                            An individual sub group number is changed */
    INITIAL_PASS_THRU = 24, /* FH -> TIC, if subscriber(s) then passed through
                               the TIC (without caching) as a VERIFY, else TIC
                               silently ignores */
    UPDATE_PASS_THRU  = 25, /* FH -> TIC, if subscriber(s) then passed through
                               the TIC (without caching) as a UPDATE, else TIC
                               silently ignores */
    INITIAL_AGGREGATE = 26, /* Init cached message, don't forward (INITIAL) */
    UPDATE_AGGREGATE  = 27, /* Update cached message, don't forward (UPDATE) */
    FINISH_AGGREGATE  = 28  /* Update cached message, forward cached (VERIFY)*/
  };

  const unsigned short NO_TYPE  = 0x7fffU; /* invalid message type */
  const unsigned short MAX_TYPE = 0xffffU; /* msg has no message type */

  RAIMSG_DLL_EXP
  extern const char *msgTypeToString( unsigned short msgType,  char *buf );
  RAIMSG_DLL_EXP
  extern unsigned short stringToMsgType( const char *s );

  enum RecStatus {
    STATUS_OK                   = 0,
    STATUS_BAD_NAME             = 1,  /* no such subject available from the
                                         publisher */
    STATUS_BAD_LINE             = 2,  /* comm. line to the publisher went down*/
    STATUS_CACHE_FULL           = 3,  /* publisher's limited cache capacity is
                                         now full */
    STATUS_PERMISSION_DENIED    = 4,  /* feed denied permission (not site's
                                         sysadm denied) */
    STATUS_PREEMPTED            = 5,  /* bumped out by cache preemption
                                         algorithms */
    STATUS_BAD_ACCESS           = 6,  /* feed specific failures */
    STATUS_TEMP_UNAVAIL         = 7,  /* when publisher will take a while to
                                         service the subscription */
    STATUS_REASSIGN             = 8,  /* when GSM is reassigning a subject to
                                         another feed */
    STATUS_NOSUBSCRIBERS        = 9,  /* when all subscribers have gone away */
    STATUS_EXPIRED              = 10, /* when a previously existing subject has
                                         disappeared (options expired etc) */
    STATUS_TIC_DOWN             = 11, /* the TIC where the current subject is
                                         in is down */
    STATUS_FEED_DOWN            = 12,
    STATUS_GSM_DOWN             = 14,
    STATUS_SUBSC_DENIED         = 15, /* the subscription was denied by the
                                         feedhandler because it was not
                                         configured to service it */
    STATUS_SUBSC_TEMP_DENIED    = 16, /* the subscription was dropped by
                                         feedhandler because it the value in it
                                         was stale/non-deterministic, user
                                         should retry */
    STATUS_NOT_FOUND            = 17, /* sent from TIC to client if instrument
                                         not in TIC for broadcast feeds */
    STATUS_STALE_VALUE          = 18,
    STATUS_RELOCATE             = 19, /* Feed tells GSM to relocate this
                                         subscription to a better source,
                                         include itself, if possible */
    STATUS_ENTITLEMENT_DENIED   = 20, /* this is used to indicate a permission
                                         denied status code for an open
                                         subscription */
    STATUS_REC_OVERFLOW         = 21, /* this is used by the server based
                                         entitlements infrastructure, if the
                                         group or user entitlement record
                                         overflows the client will receive this
                                         status code */
    STATUS_TIC_TUPLE_FAIL       = 22, /* this is used by the server based
                                         entitlements infrastructure, if the
                                         creation of group or user entitlement
                                         in the TIC fails, the client will
                                         receive this status code */
    STATUS_ENTITLEMENT_MIGRATED = 23, /* this is used to indicate a permission
                                         denied event due to entitlements
                                         migration (to other host) */
    STATUS_CI_DISCONNECTED      = 24,
    STATUS_CI_DIAG_START        = 25,
    STATUS_NO_CACHED_DATA       = 26,
    STATUS_NO_REPLY             = 27, /* SASS3 only, used to show the
                                         subscription has not yet received any
                                         reply from the TIC or feed. This is
                                         needed because the SASS3 API allows
                                         the user to query the subscription
                                        handle for the current message type
                                         and status, and they may do so before
                                         the subscription has received any
                                         reply. */
    STATUS_TMF_DOWN             = 28,
    STATUS_TPT_DISCONNECTED     = 29,
    STATUS_TIMEOUT              = 30,
    STATUS_PERIODIC_SNAPSHOT    = 64, /* this subject will be updated
                                         periodically */
    STATUS_FEED_UP              = 65,
    STATUS_HL_ROUTER_DOWN       = 66, /* the news headline router is down */
    STATUS_DQA_SUSPECT          = 67, /* the DQA monitor process is not
                                         heartbeating */
    STATUS_DQA_ACTIVE           = 68, /* the DQA monitor process is back after
                                         being down */
    STATUS_GSM_UP               = 69, /* the GSM is back up after begin down */
    STATUS_HL_ROUTER_UP         = 71, /* the news headline router is up after
                                         being down */
    STATUS_TIC_UP               = 72,
    STATUS_FEED_SWITCHOVER      = 73, /* if FT backup feeds switch
                                         non-transparently */
    STATUS_DATA_SUSPECT         = 74, /* this record from tic is unreliable,
                                         the source may be in rebuilding cycle*/
    STATUS_RECAP                = 75, /* this status code is used to indicate
                                         the receiving record is a
                                         recap/refresh record, not a real
                                         update/transaction, if application is
                                         to accumulate updates for historical
                                         purpose, the record should be ignore */
    STATUS_CI_RECONNECTED       = 76,
    STATUS_CI_DIAG_END          = 77,
    STATUS_RECOVER_SUBSC_DENIED = 80,
    STATUS_CONTRIB_ACK          = 81, /* indicates an acknowledge status for
                                         contribution event */
    STATUS_CONTRIB_NACK         = 82, /* indicates an not acknowledge status
                                         for contribution event */
    STATUS_TMF_UP               = 83,
    STATUS_TPT_CONNECTED        = 84,
    STATUS_FEED_NOT_ACCEPTING   = 85
  };

  const unsigned short MAX_STATUS = 0xffffU;

  RAIMSG_DLL_EXP
  extern const char *recStatusToString( unsigned short status,  char *buf );
  RAIMSG_DLL_EXP
  extern unsigned short stringToRecStatus( const char *s );
  RAIMSG_DLL_EXP
  extern bool stringToFeedState( const char *s,  unsigned short &state );
};
}

#endif
