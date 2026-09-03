/* Copyright (c) 2004 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

/* version.h will be different for each binary built with the 
 * exception of the test utilities which will all share the same version
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "base/log.h"

#include "rai_version.h"
#include "rai_banner.h"

/* this file contains the banner function that is called on 
 * application start up. The banner extracts information from 
 * the string constants defined in a generated header file.
 *
 * version.h is generated from the script bunp-version
 * that is run each time a build is performed.
 * The Major, Minor and patch numbers will increment if bump 
 * version is called with the appropriate argument otherwise
 * only the build number will increment
 */

using namespace rai;

const char * line = "************************************************************";

static RaiVersion bannerVersion;

RaiVersion *
BannerVersion( void )
{
  return &bannerVersion;
}

void 
RaiBanner( RaiVersion * raiVersion ) 
{
  ::memcpy( &bannerVersion, raiVersion, sizeof( bannerVersion ) );

  unsigned int verbosity;
  bool         syslogOn = Log::flipSyslog( false );
  int          prerel   = atoi( raiVersion->preVer );
  verbosity = Log::getVerbosity();
  Log::setVerbosity( Log::VERB_1 );       /* no need for severity, timestamp */
  //Log::setSyslogVerbosity( Log::VERB_0 ); /* don't syslog banner */
   
  logNormal( LNORMAL, NULL, "%s\n", line );
  logNormal( LNORMAL, NULL, "          %s ", raiVersion->description );
  if( prerel == 0 ) {
    logNormal( LNORMAL, NULL, "          %s Version %s.%s Patch %s Build %s", 
               raiVersion->buildType,
               raiVersion->majorVer,
               raiVersion->minorVer,
               raiVersion->patchVer,
               raiVersion->buildVer
               );
  } else {
    logNormal( LNORMAL, NULL, "          %s Version %s.%s Patch %s Pre-Release %s Build %s", 
               raiVersion->buildType,
               raiVersion->majorVer,
               raiVersion->minorVer,
               raiVersion->patchVer,
               raiVersion->preVer,
               raiVersion->buildVer
               );
  }
  logNormal( LNORMAL, NULL, "          Build Date: %s", raiVersion->buildDate );
  logNormal( LNORMAL, NULL, "          Build Host: %s", raiVersion->buildSystem );
  
  if ( strcmp( raiVersion->buildType, "Engineering Beta" ) == 0 ) {
    logNormal( LNORMAL, NULL, "\n                      !! Warning !! " );
    logNormal( LNORMAL, NULL, "           This is not a production build." );
    logNormal( LNORMAL, NULL, "           Use for development and testing only." );
    logNormal( LNORMAL, NULL, "           Unless it is an emergency fix for severity 1" );
    logNormal( LNORMAL, NULL, "           problem, it should be replaced with a" );
    logNormal( LNORMAL, NULL, "           production version as soon as possible.\n" );
  }
  
  logNormal( LNORMAL, NULL, "          Rai Technology Inc. " );
  unsigned int yr, mo, da;
  Time::getymd( Time::TZ_LOCAL_TIME, Time::currentTimeMillisecs(), yr, mo, da );
  logNormal( LNORMAL, NULL, "          Copyright 2003-%u \n", yr );
  logNormal( LNORMAL, NULL, "%s", line );
  
  Log::setVerbosity( verbosity );
  Log::flipSyslog( syslogOn );
}

void 
RaiBannerFull( RaiVersion * raiVersion ) 
{
  ::memcpy( &bannerVersion, raiVersion, sizeof( bannerVersion ) );

  unsigned int verbosity;
  bool         syslogOn = Log::flipSyslog( false );
  int          prerel   = atoi( raiVersion->preVer );
  verbosity = Log::getVerbosity();
  Log::setVerbosity( Log::VERB_1 );       /* no need for severity, timestamp */
  //Log::setSyslogVerbosity( Log::VERB_0 ); /* don't syslog banner */
   
  logNormal( LNORMAL, NULL, "%s\n", line );
  logNormal( LNORMAL, NULL, "          %s ", raiVersion->description );
  if( prerel == 0 ) {
    logNormal( LNORMAL, NULL, "          %s Version %s.%s Patch %s Build %s", 
               raiVersion->buildType,
               raiVersion->majorVer,
               raiVersion->minorVer,
               raiVersion->patchVer,
               raiVersion->buildVer
               );
  } else {
    logNormal( LNORMAL, NULL, "          %s Version %s.%s Patch %s Pre-Release %s Build %s", 
               raiVersion->buildType,
               raiVersion->majorVer,
               raiVersion->minorVer,
               raiVersion->patchVer,
               raiVersion->preVer,
               raiVersion->buildVer
               );
  }
  logNormal( LNORMAL, NULL, "          Build Date: %s", raiVersion->buildDate );
  logNormal( LNORMAL, NULL, "          Build Host: %s", raiVersion->buildSystem );
  logNormal( LNORMAL, NULL, "          Build Compiler: %s", raiVersion->buildCompiler );
  logNormal( LNORMAL, NULL, "          Build Commit: %s", raiVersion->commit ? raiVersion->commit : "Unknown");
  
  if ( strcmp( raiVersion->buildType, "Engineering Beta" ) == 0 ) {
    logNormal( LNORMAL, NULL, "\n                      !! Warning !! " );
    logNormal( LNORMAL, NULL, "           This is not a production build." );
    logNormal( LNORMAL, NULL, "           Use for development and testing only." );
    logNormal( LNORMAL, NULL, "           Unless it is an emergency fix for severity 1" );
    logNormal( LNORMAL, NULL, "           problem, it should be replaced with a" );
    logNormal( LNORMAL, NULL, "           production version as soon as possible.\n" );
  }
  
  logNormal( LNORMAL, NULL, "          Rai Technology Inc. " );
  unsigned int yr, mo, da;
  Time::getymd( Time::TZ_LOCAL_TIME, Time::currentTimeMillisecs(), yr, mo, da );
  logNormal( LNORMAL, NULL, "          Copyright 2003-%u \n", yr );
  logNormal( LNORMAL, NULL, "%s", line );
  
  Log::setVerbosity( verbosity );
  Log::flipSyslog( syslogOn );
}

void 
RaiBannerShort( RaiVersion * raiVersion ) 
{
  ::memcpy( &bannerVersion, raiVersion, sizeof( bannerVersion ) );

  unsigned int verbosity;
  bool         syslogOn = Log::flipSyslog( false );

  verbosity = Log::getVerbosity();
  Log::setVerbosity( Log::VERB_1 );       /* no need for severity, timestamp */

  logNormal( LNORMAL, NULL, "%s", line );
  logNormal( LNORMAL, NULL, "          %s ", raiVersion->description );
  if( raiVersion->preVer && raiVersion->preVer[0] == '0' ) {
    logNormal( LNORMAL, NULL, "          %s Version %s.%s Patch %s Build %s", 
               raiVersion->buildType,
               raiVersion->majorVer,
               raiVersion->minorVer,
               raiVersion->patchVer,
               raiVersion->buildVer
               );
  } else {
    logNormal( LNORMAL, NULL, "          %s Version %s.%s Patch %s Pre-Release %s Build %s", 
               raiVersion->buildType,
               raiVersion->majorVer,
               raiVersion->minorVer,
               raiVersion->patchVer,
               raiVersion->preVer,
               raiVersion->buildVer
               );
  }

  unsigned int yr, mo, da;
  Time::getymd( Time::TZ_LOCAL_TIME, Time::currentTimeMillisecs(), yr, mo, da );
  logNormal( LNORMAL, NULL, "          Rai Technology Inc.  Copyright 2003-%u", yr );
  logNormal( LNORMAL, NULL, "%s", line );

  Log::setVerbosity( verbosity );
  Log::flipSyslog( syslogOn );
}

char  versionString[256];

// Return version string without new-line
const char *
RaiVerString2( RaiVersion * raiVersion ) {
  if( raiVersion->preVer && raiVersion->preVer[0] == '0' ) {
    sprintf( versionString,"%s Version %s.%s Patch %s Build %s", 
             raiVersion->buildType,
             raiVersion->majorVer,
             raiVersion->minorVer,
             raiVersion->patchVer,
             raiVersion->buildVer
             );
  } else {
    sprintf( versionString,"%s Version %s.%s Patch %s Pre-Release %s Build %s", 
             raiVersion->buildType,
             raiVersion->majorVer,
             raiVersion->minorVer,
             raiVersion->patchVer,
             raiVersion->preVer,
             raiVersion->buildVer
             );
  }
  return versionString;
}

// Return version string with new-line
const char *
RaiVerString( RaiVersion * raiVersion ) {
  // Loose const here so that we can add new-line
  char *ver = ( char * )RaiVerString2( raiVersion );
  int   len = ::strlen(ver);

  ver[len]	='\n';
  ver[len + 1]  =0;

  return ver;
}
