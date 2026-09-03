/* Copyright (c) 2009 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

#ifndef __rai_base__rai_factory_h__
#define __rai_base__rai_factory_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else                             
#define RAIBASE_DLL_EXP
#endif                            
#endif

#if ! defined( RAINET_DLL_EXP )
#if defined( RAI_DLL )
#define RAINET_DLL_EXP __declspec(dllimport)
#else
#define RAINET_DLL_EXP
#endif
#endif

#if ! defined( RAIMESSAGE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIMESSAGE_DLL_EXP __declspec(dllimport)
#else
#define RAIMESSAGE_DLL_EXP
#endif
#endif

#if ! defined( RAICACHE_DLL_EXP )
#if defined( RAI_DLL )
#define RAICACHE_DLL_EXP __declspec(dllimport)
#else
#define RAICACHE_DLL_EXP
#endif
#endif

extern "C" {
  /* init msg factories */
  RAIMESSAGE_DLL_EXP
  int rai_msg_factory_initialize( int argc,  char *argv[],  char *err = 0,
                                  unsigned int errlen = 0 );
  /* init net factories */
  RAINET_DLL_EXP
  int rai_net_factory_initialize( int argc,  char *argv[],  char *err = 0,
                                  unsigned int errlen = 0 );
  /* init all factories (msg, net, proto, ftproto, svc) */
  RAICACHE_DLL_EXP
  int rai_factory_initialize( int argc,  char *argv[],  char *err = 0,
                              unsigned int errlen = 0 );
  /* init java factories */
  int rai_java_factory_initialize( int argc,  char *argv[],  char *err = 0,
                                   unsigned int errlen = 0 );
  RAIBASE_DLL_EXP
  int rai_factory_initialize2( int argc,  char *argv[],  char *err,
                               unsigned int errlen,
             const char **dll1, const char **func1, const char *dllVe1,
             const char **dll2=0, const char **func2=0, const char *dllVe2=0,
             const char **dll3=0, const char **func3=0, const char *dllVe3=0 );
}

#endif
