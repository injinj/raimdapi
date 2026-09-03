/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__defs_h__
#define __rai_msg__defs_h__

#ifndef __rai_msg__types_h__
#include "msg/types.h"
#endif

namespace RaiMsgConst {
  static const unsigned int MAX_PROTO = 9;

  /* how many bytes added to message for size & magic */
  static const RaiMsg_size HDR_BYTES[ MAX_PROTO ] = {
    /* 0/RAIMSG */        9, /* 1/RV_SASS */       0,
    /* 2/TIB_SASS */      8, /* 3/TIB_SASS_FORM */ 8,
    /* 4/RV_RAIMSG */     0, /* 5/XREP */          0,
    /* 6/RV */            8, /* 7/CI_SASS */       2,
    /* 8/CI_SASS_FORM */  2,
  };
  /* where the message data begins from msgStart */
  static const RaiMsg_size START_OFF[ MAX_PROTO ] = {
    /* 0/RAIMSG */        0, /* 1/RV_SASS */       0,
    /* 2/TIB_SASS */      0, /* 3/TIB_SASS_FORM */ 0,
    /* 4/RV_RAIMSG */     0, /* 5/XREP */          0,
    /* 6/RV */            8, /* 7/CI_SASS */       2,
    /* 8/CI_SASS_FORM */  2,
  };
  /* how much message data before msgStart */
  static const RaiMsg_size HDR_SIZE[ MAX_PROTO ] = {
    /* 0/RAIMSG */        9, /* 1/RV_SASS */       0,
    /* 2/TIB_SASS */      8, /* 3/TIB_SASS_FORM */ 8,
    /* 4/RV_RAIMSG */     0, /* 5/XREP */          0,
    /* 6/RV */            0, /* 7/CI_SASS */       0,
    /* 8/CI_SASS_FORM */  0,
  };
  /* offset from msgStart for size of message */
  static const Rai_i32 SIZE_OFF[ MAX_PROTO ] = {
    /* 0/RAIMSG */       -4, /* 1/RV_SASS */        0,
    /* 2/TIB_SASS */     -4, /* 3/TIB_SASS_FORM */ -4,
    /* 4/RV_RAIMSG */     0, /* 5/XREP */           0,
    /* 6/RV */            0, /* 7/CI_SASS */        0,
    /* 8/CI_SASS_FORM */  0,
  };
  /* offset from msgStart for magic number */
  static const Rai_i32 MAGIC_OFF[ MAX_PROTO ] = {
    /* 0/RAIMSG */       -9, /* 1/RV_SASS */        0,
    /* 2/TIB_SASS */     -8, /* 3/TIB_SASS_FORM */ -8,
    /* 4/RV_RAIMSG */     0, /* 5/XREP */           0,
    /* 6/RV */            4, /* 7/CI_SASS */        0,
    /* 8/CI_SASS_FORM */  0,
  };
  /* uint sizeof message size */
  static const RaiMsg_size SIZE_LEN[ MAX_PROTO ] = {
    /* 0/RAIMSG */        4, /* 1/RV_SASS */        0,
    /* 2/TIB_SASS */      4, /* 3/TIB_SASS_FORM */  4,
    /* 4/RV_RAIMSG */     0, /* 5/XREP */           0,
    /* 6/RV */            4, /* 7/CI_SASS */        2,
    /* 8/CI_SASS_FORM */  2,
  };

  /* magic number in header */
  static const Rai_u32 MAGIC_NUM[ MAX_PROTO ] = {
    /* 0/RAIMSG */             RAIMSG_MAGIC_MESSAGE,
    /* 1/RV_SASS */            /*RAIMSG_MAGIC_RV_SASS*/ 0,
    /* 2/TIB_SASS */           RAIMSG_MAGIC_TIB_SASS,
    /* 3/TIB_SASS_FORM */      RAIMSG_MAGIC_TIB_SASS,
    /* 4/RV_RAIMSG */          /*RAIMSG_MAGIC_RV_RAIMSG*/ 0,
    /* 5/XREP */               /*RAIMSG_MAGIC_XREP*/ 0,
    /* 6/RV */                 RAIMSG_MAGIC_RV,
    /* 7/CI_SASS */            0,
    /* 8/CI_SASS_FORM */       0,
  };
}

static const RaiRvMsg_type raiMsgTypeToRvType[] = {
  /*  0 = RAIMSG_NODATA  */ RAI_RV_BADDATA,
  /*  1 = RAIMSG_MESSAGE */ RAI_RV_RVMSG,
  /*  2 = RAIMSG_STRING  */ RAI_RV_STRING,
  /*  3 = RAIMSG_OPAQUE  */ RAI_RV_OPAQUE,
  /*  4 = RAIMSG_BOOLEAN */ RAI_RV_BOOLEAN,
  /*  5 = RAIMSG_INT     */ RAI_RV_INT,
  /*  6 = RAIMSG_UINT    */ RAI_RV_UINT,
  /*  7 = RAIMSG_REAL    */ RAI_RV_REAL,
  /*  8 = RAIMSG_ARRAY   */ RAI_RV_OPAQUE,
  /*  9 = RAIMSG_PARTIAL */ RAI_RV_OPAQUE,
  /* 10 = RAIMSG_IPDATA  */ RAI_RV_IPDATA
};

static const RaiMsg_type rvTypeToRaiMsgType[] = {
  /*  0 = RAI_RV_BADDATA  */ RAIMSG_NODATA,
  /*  1 = RAI_RV_RVMSG    */ RAIMSG_MESSAGE,
  /*  2 = RAI_RV_SUBJECT  */ RAIMSG_OPAQUE,
  /*  3 = RAI_RV_DATETIME */ RAIMSG_UINT,
  /*  4                   */ RAIMSG_NODATA,
  /*  5                   */ RAIMSG_NODATA,
  /*  6                   */ RAIMSG_NODATA,
  /*  7 = RAI_RV_OPAQUE   */ RAIMSG_OPAQUE,
  /*  8 = RAI_RV_STRING   */ RAIMSG_STRING,
  /*  9 = RAI_RV_BOOLEAN  */ RAIMSG_BOOLEAN,
  /* 10 = RAI_RV_IPDATA   */ RAIMSG_IPDATA,
  /* 11 = RAI_RV_INT      */ RAIMSG_INT,
  /* 12 = RAI_RV_UINT     */ RAIMSG_UINT,
  /* 13 = RAI_RV_REAL     */ RAIMSG_REAL
};

static const RaiMsg_type tssTypeToRaiMsgType[] = {
  /*  0 = RAI_TSS_NODATA     */ RAIMSG_NODATA,
  /*  1 = RAI_TSS_INTEGER    */ RAIMSG_INT,
  /*  2 = RAI_TSS_STRING     */ RAIMSG_STRING,
  /*  3 = RAI_TSS_BOOLEAN    */ RAIMSG_BOOLEAN,
  /*  4 = RAI_TSS_DATE       */ RAIMSG_UINT,
  /*  5 = RAI_TSS_TIME       */ RAIMSG_UINT,
  /*  6 = RAI_TSS_PRICE      */ RAIMSG_REAL,
  /*  7 = RAI_TSS_BYTE       */ RAIMSG_UINT,
  /*  8 = RAI_TSS_FLOAT      */ RAIMSG_REAL,
  /*  9 = RAI_TSS_SHORT_INT  */ RAIMSG_INT,
  /* 10 = RAI_TSS_DOUBLE     */ RAIMSG_REAL,
  /* 11 = RAI_TSS_OPAQUE     */ RAIMSG_OPAQUE,
  /* 12 = RAI_TSS_NULL       */ RAIMSG_NODATA,
  /* 13 = RAI_TSS_RESERVED   */ RAIMSG_NODATA,
  /* 14 = RAI_TSS_DOUBLE_INT */ RAIMSG_REAL,
  /* 15 = RAI_TSS_GROCERY    */ RAIMSG_REAL,
  /* 16 = RAI_TSS_SDATE      */ RAIMSG_STRING,
  /* 17 = RAI_TSS_STIME      */ RAIMSG_STRING,
  /* 18 = RAI_TSS_LONG       */ RAIMSG_UINT,
  /* 19 = RAI_TSS_U_SHORT    */ RAIMSG_UINT,
  /* 20 = RAI_TSS_U_INT      */ RAIMSG_UINT,
  /* 21 = RAI_TSS_U_LONG     */ RAIMSG_UINT
};

static const Rai_u32 MAX_MSG_DEPTH = 8;

enum RaiMsgExtra_type {
  ARRAY      = 0,
  ACTIVATE   = 1,
  DICTIONARY = 2
};

struct RaiMsg_extra {
  RaiMsg_extra   * next;
  RaiMsgExtra_type type;

  union {
    struct {
      RaiMsg_size arrayOffset;
      RaiMsg_data decodedArray;
    } ar;
    struct {
      Rai_u32     top;
      RaiMsg_size offset[ MAX_MSG_DEPTH ];
    } act;
    struct {
      RaiMsg_dict ** dict;
      Rai_u32        numEntries;
    } dict;
  } u;

  SYS_OPS( RaiMsg_extra );
  RaiMsg_extra( RaiMsg_extra *enext,  RaiMsg_dict **dict,  Rai_u32 numEntries );

  RaiMsg_extra( RaiMsg_extra *enext,  Rai_u32 top,  RaiMsg_size *offset );

  RaiMsg_extra( RaiMsg_extra *enext,  RaiMsg_size arrayOffset,
                RaiMsg_data decodedArray );
  ~RaiMsg_extra();

  static void Release( RaiMsg_extra *&msgEx );
};

#endif
