/* Copyright (c) 2004 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include "util/enum_map.h"

const char * rai::valToStr( ValToStr * map, int val, const char * notFound )
{
  while( map->str != 0 ) {
    if( val == map->val ) {
      return map->str;
    }
    map++;
  }
  return notFound;
}
