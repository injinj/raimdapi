/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__types_h__
#define __rai_msg__types_h__

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

typedef const struct rai::ErrorRec *RaiMsgException;

enum RaiMsg_type {
  RAIMSG_NODATA   = 0,
  RAIMSG_MESSAGE  = 1,
  RAIMSG_STRING   = 2,
  RAIMSG_OPAQUE   = 3,
  RAIMSG_BOOLEAN  = 4,
  RAIMSG_INT      = 5,
  RAIMSG_UINT     = 6,
  RAIMSG_REAL     = 7,
  RAIMSG_ARRAY    = 8,
  RAIMSG_PARTIAL  = 9,
  RAIMSG_IPDATA   = 10,
  RAIMSG_MAXVALID = 11
};

enum RaiRvMsg_type {
  RAI_RV_BADDATA   = 0,
  RAI_RV_RVMSG     = 1,
  RAI_RV_SUBJECT   = 2,
  RAI_RV_DATETIME  = 3,
  RAI_RV_OPAQUE    = 7,
  RAI_RV_STRING    = 8,
  RAI_RV_BOOLEAN   = 9,
  RAI_RV_IPDATA    = 10,  /* 0a */
  RAI_RV_INT       = 11,  /* 0b */
  RAI_RV_UINT      = 12,  /* 0c */
  RAI_RV_REAL      = 13,  /* 0d */
  RAI_RV_ENCRYPTED = 32,  /* 20 */
  RAI_RV_ARRAY_I8  = 34,  /* 22 */
  RAI_RV_ARRAY_U8  = 35,  /* 23 */
  RAI_RV_ARRAY_I16 = 36,  /* 24 */
  RAI_RV_ARRAY_U16 = 37,  /* 25 */
  RAI_RV_ARRAY_I32 = 38,  /* 26 */
  RAI_RV_ARRAY_U32 = 39,  /* 27 */
  RAI_RV_ARRAY_I64 = 40,  /* 28 */
  RAI_RV_ARRAY_U64 = 41,  /* 29 */
  RAI_RV_ARRAY_F32 = 44,  /* 2c */
  RAI_RV_ARRAY_F64 = 45   /* 2d */
};

enum RaiRvMsg_typesize {
  RAI_RV_TINY_SIZE  = 120, /* 78 */
  RAI_RV_SHORT_SIZE = 121, /* 79 */
  RAI_RV_LONG_SIZE  = 122  /* 7a */
};

const unsigned int MAX_RV_SHORT_SIZE = 0x7530; /* 30000 */

enum RaiTSS_type {
  RAI_TSS_NODATA     =  0,
  RAI_TSS_INTEGER    =  1,
  RAI_TSS_STRING     =  2,
  RAI_TSS_BOOLEAN    =  3,
  RAI_TSS_DATE       =  4,
  RAI_TSS_TIME       =  5,
  RAI_TSS_PRICE      =  6,
  RAI_TSS_BYTE       =  7,
  RAI_TSS_FLOAT      =  8,
  RAI_TSS_SHORT_INT  =  9,
  RAI_TSS_DOUBLE     = 10,
  RAI_TSS_OPAQUE     = 11,
  RAI_TSS_NULL       = 12,
  RAI_TSS_RESERVED   = 13,
  RAI_TSS_DOUBLE_INT = 14,
  RAI_TSS_GROCERY    = 15,
  RAI_TSS_SDATE      = 16,
  RAI_TSS_STIME      = 17,
  RAI_TSS_LONG       = 18,
  RAI_TSS_U_SHORT    = 19,
  RAI_TSS_U_INT      = 20,
  RAI_TSS_U_LONG     = 21
};

enum RaiTSS_hint {
  RAI_TSS_HINT_NONE            = 0,   /* no hint */
  RAI_TSS_HINT_DENOM_2         = 1,   /* 1/2 */
  RAI_TSS_HINT_DENOM_4         = 2,
  RAI_TSS_HINT_DENOM_8         = 3,
  RAI_TSS_HINT_DENOM_16        = 4,
  RAI_TSS_HINT_DENOM_32        = 5,
  RAI_TSS_HINT_DENOM_64        = 6,
  RAI_TSS_HINT_DENOM_128       = 7,
  RAI_TSS_HINT_DENOM_256       = 8,   /* 1/256 */
  RAI_TSS_HINT_PRECISION_1     = 17,  /* 10^-1 */
  RAI_TSS_HINT_PRECISION_2     = 18,
  RAI_TSS_HINT_PRECISION_3     = 19,
  RAI_TSS_HINT_PRECISION_4     = 20,
  RAI_TSS_HINT_PRECISION_5     = 21,
  RAI_TSS_HINT_PRECISION_6     = 22,
  RAI_TSS_HINT_PRECISION_7     = 23,
  RAI_TSS_HINT_PRECISION_8     = 24,
  RAI_TSS_HINT_PRECISION_9     = 25,   /* 10^-9 */
  RAI_TSS_HINT_BLANK_VALUE     = 127,  /* no value */
  RAI_TSS_HINT_DATE_TYPE       = 256,  /* SASS TSS_STIME */
  RAI_TSS_HINT_TIME_TYPE       = 257,  /* SASS TSS_SDATE */
  RAI_TSS_HINT_MF_DATE_TYPE    = 258,  /* marketfeed date */
  RAI_TSS_HINT_MF_TIME_TYPE    = 259,  /* marketfeed time */
  RAI_TSS_HINT_MF_TIME_SECONDS = 260,  /* marketfeed time_seconds */
  RAI_TSS_HINT_MF_ENUM         = 261   /* marketfeed enum */
};

typedef signed char    Rai_i8;
typedef unsigned char  Rai_u8;
typedef signed short   Rai_i16;
typedef unsigned short Rai_u16;
typedef signed int     Rai_i32;
typedef unsigned int   Rai_u32;
typedef llong          Rai_i64;
typedef ullong         Rai_u64;
typedef float          Rai_f32;
typedef double         Rai_f64;
typedef Rai_u16        Rai_ipport;
typedef Rai_u32        Rai_ipaddr;
typedef const char   * RaiMsg_name;
typedef void         * RaiMsg_data;
typedef Rai_u32        RaiMsg_size;

#define RAIMSG_MAGIC_MESSAGE   0xce13aa1fU
#define RAIMSG_MAGIC_RV_SASS   0x11111111U
#define RAIMSG_MAGIC_TIB_SASS  0x11111112U
#define RAIMSG_MAGIC_RV_RAIMSG 0x11111114U
#define RAIMSG_MAGIC_XREP      12345U
#define RAIMSG_MAGIC_RV        0x9955eeaaU

#endif
