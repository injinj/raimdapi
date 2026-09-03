/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIMSG_DLL_EXP ) && defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "msg/sass_const.h"

using namespace rai;

static inline char *
shortToString( unsigned short s,  char *buf )
{
  char *ptr = buf;
  if ( s / 10000 != 0 )
    *ptr++ = '0' + (char) ( s / 10000 );
  if ( ptr > buf || s / 1000 != 0 )
    *ptr++ = '0' + (char) ( ( s / 1000 ) % 10 );
  if ( ptr > buf || s / 100 != 0 )
    *ptr++ = '0' + (char) ( ( s / 100 ) % 10 );
  if ( ptr > buf || s / 10 != 0 )
    *ptr++ = '0' + (char) ( ( s / 10 ) % 10 );
  *ptr++ = '0' + ( s % 10 );
  *ptr = '\0';
  return buf;
}

#define DEF_STRING( ENUM ) \
  static const char ENUM ## _STRING[] = #ENUM ;
#define DEF_CASE_VAL( ENUM ) \
  case ENUM : return ENUM ## _STRING ;
#define DEF_MAP( ENUM ) \
{ SassConst:: ENUM , ENUM ## _STRING },

#define DEF_MSG_TYPE( DEF ) \
  DEF( VERIFY ) \
  DEF( UPDATE ) \
  DEF( CORRECT ) \
  DEF( CLOSING ) \
  DEF( DROP ) \
  DEF( AGGREGATE ) \
  DEF( STATUS ) \
  DEF( CANCEL ) \
  DEF( INITIAL ) \
  DEF( TRANSIENT ) \
  DEF( DERIVED ) \
  DEF( DELETE ) \
  DEF( SUBREINIT ) \
  DEF( SNAPSHOT ) \
  DEF( CONFIRM ) \
  DEF( BDS_CONFIRM ) \
  DEF( EDIT ) \
  DEF( EDIT_FORCE ) \
  DEF( RENAME ) \
  DEF( SERVICE_STATUS ) \
  DEF( CONTRIB_REPLY ) \
  DEF( GROUP_STATUS ) \
  DEF( GROUP_MERGE ) \
  DEF( GROUP_CHANGE ) \
  DEF( INITIAL_PASS_THRU ) \
  DEF( UPDATE_PASS_THRU ) \
  DEF( INITIAL_AGGREGATE ) \
  DEF( UPDATE_AGGREGATE ) \
  DEF( FINISH_AGGREGATE )

DEF_MSG_TYPE( DEF_STRING )


const char *
SassConst::msgTypeToString( unsigned short msgType,  char *buf )
{
  switch ( (MsgType) msgType ) {
    DEF_MSG_TYPE( DEF_CASE_VAL )
  }
  return shortToString( msgType, buf );
}


static struct TypeMap {
  SassConst::MsgType type;
  const char       * string;
} typeMap[] = {
/* for generating typeMap:
   DEF_MSG_TYPE( DEF_MAP ) */
{ SassConst::AGGREGATE, AGGREGATE_STRING }, /* alphabetical order */
{ SassConst::BDS_CONFIRM, BDS_CONFIRM_STRING },
{ SassConst::CANCEL, CANCEL_STRING },
{ SassConst::CLOSING, CLOSING_STRING },
{ SassConst::CONFIRM, CONFIRM_STRING },
{ SassConst::CONTRIB_REPLY, CONTRIB_REPLY_STRING },
{ SassConst::CORRECT, CORRECT_STRING },
{ SassConst::DELETE, DELETE_STRING },
{ SassConst::DERIVED, DERIVED_STRING },
{ SassConst::DROP, DROP_STRING },
{ SassConst::EDIT, EDIT_STRING },
{ SassConst::EDIT_FORCE, EDIT_FORCE_STRING },
{ SassConst::FINISH_AGGREGATE, FINISH_AGGREGATE_STRING },
{ SassConst::GROUP_CHANGE, GROUP_CHANGE_STRING },
{ SassConst::GROUP_MERGE, GROUP_MERGE_STRING },
{ SassConst::GROUP_STATUS, GROUP_STATUS_STRING },
{ SassConst::INITIAL, INITIAL_STRING },
{ SassConst::INITIAL_AGGREGATE, INITIAL_AGGREGATE_STRING },
{ SassConst::INITIAL_PASS_THRU, INITIAL_PASS_THRU_STRING },
{ SassConst::RENAME, RENAME_STRING },
{ SassConst::SERVICE_STATUS, SERVICE_STATUS_STRING },
{ SassConst::SNAPSHOT, SNAPSHOT_STRING },
{ SassConst::STATUS, STATUS_STRING },
{ SassConst::SUBREINIT, SUBREINIT_STRING },
{ SassConst::TRANSIENT, TRANSIENT_STRING },
{ SassConst::UPDATE, UPDATE_STRING },
{ SassConst::UPDATE_AGGREGATE, UPDATE_AGGREGATE_STRING },
{ SassConst::UPDATE_PASS_THRU, UPDATE_PASS_THRU_STRING },
{ SassConst::VERIFY, VERIFY_STRING }
};
extern "C" {
static int
cmpTypeMap( const void *m1,  const void *m2 )
{
  return ::strcmp( ((const TypeMap *) m1)->string,
                   ((const TypeMap *) m2)->string );
} }

#ifdef __GEN_TYPE_MAP__
/* delete existing TypeMap records, uncomment DEF_MSG_TYPE( DEF_MAP )
   cd src/msg ; g++ -D__GEN_TYPE_MAP__ -I.. sass_const.cpp ; a.out > out */
#include <stdio.h>
int
main( void )
{
  unsigned int i;

  ::qsort( typeMap, sizeof( typeMap ) / sizeof( typeMap[ 0 ] ),
           sizeof( typeMap[ 0 ] ), cmpTypeMap );
  for ( i = 0; i < sizeof( typeMap ) / sizeof( typeMap[ 0 ] ); i++ ) {
    printf( "{ SassConst::%s, %s_STRING },\n",
            typeMap[ i ].string, typeMap[ i ].string );
  }
  return 0;
}
#endif


unsigned short
SassConst::stringToMsgType( const char *s )
{
  if ( s != NULL ) {
    unsigned short t;
    TypeMap        key,
                 * ptr;

    key.string = s;
    ptr = (TypeMap *)
          ::bsearch( &key, typeMap,
                     sizeof( typeMap ) / sizeof( typeMap[ 0 ] ),
                     sizeof( typeMap[ 0 ] ), cmpTypeMap );
    if ( ptr != NULL )
      return ptr->type;

    if ( s[ 0 ] >= '0' && s[ 0 ] <= '9' ) {
      t = (unsigned short) ( (unsigned char) *s++ - '0' );
      while ( s[ 0 ] >= '0' && s[ 0 ] <= '9' )
        t = t * 10U + (unsigned short) ( (unsigned char) *s++ - '0' );
      return t;
    }
  }
  return MAX_TYPE;
}


#undef DEF_STRING
#undef DEF_CASE_VAL
#undef DEF_MAP

#define DEF_STRING( ENUM ) \
  static const char STATUS_ ## ENUM ## _STRING[] = #ENUM ;
#define DEF_CASE_VAL( ENUM ) \
  case STATUS_ ## ENUM : return STATUS_ ## ENUM ## _STRING ;
#define DEF_MAP( ENUM ) \
{ SassConst::STATUS_ ## ENUM , STATUS_ ## ENUM ## _STRING },

#define DEF_REC_STATUS( DEF ) \
  DEF( OK ) \
  DEF( BAD_NAME ) \
  DEF( BAD_LINE ) \
  DEF( CACHE_FULL ) \
  DEF( PERMISSION_DENIED ) \
  DEF( PREEMPTED ) \
  DEF( BAD_ACCESS ) \
  DEF( TEMP_UNAVAIL ) \
  DEF( REASSIGN ) \
  DEF( NOSUBSCRIBERS ) \
  DEF( EXPIRED ) \
  DEF( TIC_DOWN ) \
  DEF( FEED_DOWN ) \
  DEF( GSM_DOWN ) \
  DEF( SUBSC_DENIED ) \
  DEF( SUBSC_TEMP_DENIED ) \
  DEF( NOT_FOUND ) \
  DEF( STALE_VALUE ) \
  DEF( RELOCATE ) \
  DEF( ENTITLEMENT_DENIED ) \
  DEF( REC_OVERFLOW ) \
  DEF( TIC_TUPLE_FAIL ) \
  DEF( ENTITLEMENT_MIGRATED ) \
  DEF( CI_DISCONNECTED ) \
  DEF( CI_DIAG_START ) \
  DEF( NO_CACHED_DATA ) \
  DEF( NO_REPLY ) \
  DEF( TMF_DOWN ) \
  DEF( TPT_DISCONNECTED ) \
  DEF( TIMEOUT ) \
  DEF( PERIODIC_SNAPSHOT ) \
  DEF( FEED_UP ) \
  DEF( HL_ROUTER_DOWN ) \
  DEF( DQA_SUSPECT ) \
  DEF( DQA_ACTIVE ) \
  DEF( GSM_UP ) \
  DEF( HL_ROUTER_UP ) \
  DEF( TIC_UP ) \
  DEF( FEED_SWITCHOVER ) \
  DEF( DATA_SUSPECT ) \
  DEF( RECAP ) \
  DEF( CI_RECONNECTED ) \
  DEF( CI_DIAG_END ) \
  DEF( RECOVER_SUBSC_DENIED ) \
  DEF( CONTRIB_ACK ) \
  DEF( CONTRIB_NACK ) \
  DEF( TMF_UP ) \
  DEF( TPT_CONNECTED ) \
  DEF( FEED_NOT_ACCEPTING )

DEF_REC_STATUS( DEF_STRING )


const char *
SassConst::recStatusToString( unsigned short status,  char *buf )
{
  switch ( (RecStatus) status ) {
    DEF_REC_STATUS( DEF_CASE_VAL )
  }
  return shortToString( status, buf );
}


static struct StatusMap {
  SassConst::RecStatus status;
  const char         * string;
} statusMap[] = {
/* for generating statusMap:
   DEF_REC_STATUS( DEF_MAP ) */
{ SassConst::STATUS_BAD_ACCESS, STATUS_BAD_ACCESS_STRING }, /* alphabetical */
{ SassConst::STATUS_BAD_LINE, STATUS_BAD_LINE_STRING },
{ SassConst::STATUS_BAD_NAME, STATUS_BAD_NAME_STRING },
{ SassConst::STATUS_CACHE_FULL, STATUS_CACHE_FULL_STRING },
{ SassConst::STATUS_CI_DIAG_END, STATUS_CI_DIAG_END_STRING },
{ SassConst::STATUS_CI_DIAG_START, STATUS_CI_DIAG_START_STRING },
{ SassConst::STATUS_CI_DISCONNECTED, STATUS_CI_DISCONNECTED_STRING },
{ SassConst::STATUS_CI_RECONNECTED, STATUS_CI_RECONNECTED_STRING },
{ SassConst::STATUS_CONTRIB_ACK, STATUS_CONTRIB_ACK_STRING },
{ SassConst::STATUS_CONTRIB_NACK, STATUS_CONTRIB_NACK_STRING },
{ SassConst::STATUS_DATA_SUSPECT, STATUS_DATA_SUSPECT_STRING },
{ SassConst::STATUS_DQA_ACTIVE, STATUS_DQA_ACTIVE_STRING },
{ SassConst::STATUS_DQA_SUSPECT, STATUS_DQA_SUSPECT_STRING },
{ SassConst::STATUS_ENTITLEMENT_DENIED, STATUS_ENTITLEMENT_DENIED_STRING },
{ SassConst::STATUS_ENTITLEMENT_MIGRATED, STATUS_ENTITLEMENT_MIGRATED_STRING },
{ SassConst::STATUS_EXPIRED, STATUS_EXPIRED_STRING },
{ SassConst::STATUS_FEED_DOWN, STATUS_FEED_DOWN_STRING },
{ SassConst::STATUS_FEED_NOT_ACCEPTING, STATUS_FEED_NOT_ACCEPTING_STRING },
{ SassConst::STATUS_FEED_SWITCHOVER, STATUS_FEED_SWITCHOVER_STRING },
{ SassConst::STATUS_FEED_UP, STATUS_FEED_UP_STRING },
{ SassConst::STATUS_GSM_DOWN, STATUS_GSM_DOWN_STRING },
{ SassConst::STATUS_GSM_UP, STATUS_GSM_UP_STRING },
{ SassConst::STATUS_HL_ROUTER_DOWN, STATUS_HL_ROUTER_DOWN_STRING },
{ SassConst::STATUS_HL_ROUTER_UP, STATUS_HL_ROUTER_UP_STRING },
{ SassConst::STATUS_NOSUBSCRIBERS, STATUS_NOSUBSCRIBERS_STRING },
{ SassConst::STATUS_NOT_FOUND, STATUS_NOT_FOUND_STRING },
{ SassConst::STATUS_NO_CACHED_DATA, STATUS_NO_CACHED_DATA_STRING },
{ SassConst::STATUS_NO_REPLY, STATUS_NO_REPLY_STRING },
{ SassConst::STATUS_OK, STATUS_OK_STRING },
{ SassConst::STATUS_PERIODIC_SNAPSHOT, STATUS_PERIODIC_SNAPSHOT_STRING },
{ SassConst::STATUS_PERMISSION_DENIED, STATUS_PERMISSION_DENIED_STRING },
{ SassConst::STATUS_PREEMPTED, STATUS_PREEMPTED_STRING },
{ SassConst::STATUS_REASSIGN, STATUS_REASSIGN_STRING },
{ SassConst::STATUS_RECAP, STATUS_RECAP_STRING },
{ SassConst::STATUS_RECOVER_SUBSC_DENIED, STATUS_RECOVER_SUBSC_DENIED_STRING },
{ SassConst::STATUS_REC_OVERFLOW, STATUS_REC_OVERFLOW_STRING },
{ SassConst::STATUS_RELOCATE, STATUS_RELOCATE_STRING },
{ SassConst::STATUS_STALE_VALUE, STATUS_STALE_VALUE_STRING },
{ SassConst::STATUS_SUBSC_DENIED, STATUS_SUBSC_DENIED_STRING },
{ SassConst::STATUS_SUBSC_TEMP_DENIED, STATUS_SUBSC_TEMP_DENIED_STRING },
{ SassConst::STATUS_TEMP_UNAVAIL, STATUS_TEMP_UNAVAIL_STRING },
{ SassConst::STATUS_TIC_DOWN, STATUS_TIC_DOWN_STRING },
{ SassConst::STATUS_TIC_TUPLE_FAIL, STATUS_TIC_TUPLE_FAIL_STRING },
{ SassConst::STATUS_TIC_UP, STATUS_TIC_UP_STRING },
{ SassConst::STATUS_TIMEOUT, STATUS_TIMEOUT_STRING },
{ SassConst::STATUS_TMF_DOWN, STATUS_TMF_DOWN_STRING },
{ SassConst::STATUS_TMF_UP, STATUS_TMF_UP_STRING },
{ SassConst::STATUS_TPT_CONNECTED, STATUS_TPT_CONNECTED_STRING },
{ SassConst::STATUS_TPT_DISCONNECTED, STATUS_TPT_DISCONNECTED_STRING },
};
extern "C" {
static int
cmpStatusMap( const void *m1,  const void *m2 )
{
  return ::strcmp( ((const StatusMap *) m1)->string,
                   ((const StatusMap *) m2)->string );
} }

#ifdef __GEN_STATUS_MAP__
/* delete existing StatusMap records, uncomment DEF_REC_STATUS( DEF_MAP )
   cd src/msg ; g++ -D__GEN_STATUS_MAP__ -I.. sass_const.cpp ; a.out > out */
#include <stdio.h>
int
main( void )
{
  unsigned int i;

  ::qsort( statusMap, sizeof( statusMap ) / sizeof( statusMap[ 0 ] ),
           sizeof( statusMap[ 0 ] ), cmpStatusMap );
  for ( i = 0; i < sizeof( statusMap ) / sizeof( statusMap[ 0 ] ); i++ ) {
    printf( "{ SassConst::STATUS_%s, STATUS_%s_STRING },\n",
            statusMap[ i ].string, statusMap[ i ].string );
  }
  return 0;
}
#endif


unsigned short
SassConst::stringToRecStatus( const char *s )
{
  if ( s != NULL ) {
    unsigned short t;
    StatusMap      key,
                 * ptr;

    key.string = s;
    ptr = (StatusMap *)
          ::bsearch( &key, statusMap,
                     sizeof( statusMap ) / sizeof( statusMap[ 0 ] ),
                     sizeof( statusMap[ 0 ] ), cmpStatusMap );
    if ( ptr != NULL )
      return ptr->status;

    if ( s[ 0 ] >= '0' && s[ 0 ] <= '9' ) {
      t = (unsigned short) ( (unsigned char) *s++ - '0' );
      while ( s[ 0 ] >= '0' && s[ 0 ] <= '9' )
        t = t * 10U + (unsigned short) ( (unsigned char) *s++ - '0' );
      return t;
    }
  }
  return MAX_STATUS;
}


static bool
feed_cmp_vals( const char **vals,  const char *s )
{
  for ( ; vals[ 0 ] != NULL; vals++ ) {
    const char *v = vals[ 0 ];
    const char *u = s;
    for (;;) {
      if ( *v != *u ) {
        if ( toupper( *v ) != *u ) {
          if ( *v != '_' || *u != '-' )
            break;
        }
      }
      else if ( *v == '\0' )
        return true;
      v++; u++;
    }
  }
  return false;
}


bool
SassConst::stringToFeedState( const char *s,  unsigned short &state )
{
  static const char *nota_vals[] = { "feed_not_accepting", "not_accepting",
                                     "notaccepting", "up_not_accepting",
                                     "upnotaccepting", "2", NULL };
  static const char *down_vals[] = { "feed_down", "down", "false", "no", "0",
                                     "off", NULL };
  static const char *up_vals[]   = { "feed_up", "up", "true", "yes", "0",
                                     "on", NULL };
  state = SassConst::STATUS_FEED_UP;
  if ( s != NULL ) {
    if ( feed_cmp_vals( nota_vals, s ) ) {
      state = SassConst::STATUS_FEED_NOT_ACCEPTING;
      return true;
    }
    if ( feed_cmp_vals( down_vals, s ) ) {
      state = SassConst::STATUS_FEED_DOWN;
      return true;
    }
    if ( feed_cmp_vals( up_vals, s ) ) {
      state = SassConst::STATUS_FEED_UP;
      return true;
    }
  }
  return false;
}

