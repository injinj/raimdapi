/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_base__rai_banner_h__
#define __rai_base__rai_banner_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

#include "base/rai_version.h"

RAIBASE_DLL_EXP
RaiVersion *BannerVersion( void );

RAIBASE_DLL_EXP
void RaiBanner( RaiVersion * raiVersion );

RAIBASE_DLL_EXP
void RaiBannerFull( RaiVersion * raiVersion );

RAIBASE_DLL_EXP
void RaiBannerShort( RaiVersion * raiVersion );

// Return version string with new-line
RAIBASE_DLL_EXP
const char * RaiVerString( RaiVersion * raiVersion );

// Return version string without new-line
RAIBASE_DLL_EXP
const char * RaiVerString2( RaiVersion * raiVersion );

#endif
