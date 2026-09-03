/* Copyright (c) 2016 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include "util/throttle.h"

using namespace rai;

void
Throttle::clear( void )
{
  this->startTime = 0; this->nextTime = 0;
  this->cpms = 0; this->perMSec = 0; this->ival = 0; this->available = 0;
  this->available = 0; this->used = 0;
}

void
Throttle::init( double ratePerSec,  double ivalMSecs )
{
  this->perMSec   = ratePerSec / 1000.0;
  this->startTime = Time::getHiresTime( &this->cpms );
  this->ival      = ivalMSecs;
  this->nextTime  = this->startTime + (TimeHires) ( this->cpms * ivalMSecs );
  this->available = this->perMSec * ivalMSecs;
  this->used      = this->available;
}

double
Throttle::charge( double amt )
{
  if ( this->perMSec == 0 )
    return 0;
  TimeHires curTime = Time::getHiresTime( &this->cpms );
  double ms;
  if ( curTime >= this->nextTime ) {
    for (;;) {
      this->startTime = this->nextTime;
      this->nextTime += (TimeHires) ( this->cpms * this->ival );
      if ( this->used <= this->available ) {
        this->used = 0;
        if ( this->nextTime > curTime )
          break;
        this->nextTime = curTime; /* restart iinterval at curTime */
      }
      else {
        this->used -= this->available;
        if ( this->nextTime > curTime )
          break;
      }
    }
  }
  if ( this->used > this->available )
    ms = ( ( this->used - this->available ) / this->perMSec );
  else
    ms = 0;
  this->used += amt;
  return ms;
}
