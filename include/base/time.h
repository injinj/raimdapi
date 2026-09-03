/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_base__time_h__
#define __rai_base__time_h__

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

namespace rai {

typedef ullong       TimeMSecs;
typedef ullong       TimeUSecs;
typedef ullong       TimeNSecs;
typedef ullong       TimeHires;
typedef unsigned int TimeSecs;

struct RAIBASE_DLL_EXP TimeRotate {
  enum DayOrWeek {
    ROTATE_UNSPECIFIED = 0,
    ROTATE_DAILY       = 1,
    ROTATE_WEEKLY      = 2
  };
  TimeMSecs time,
            period,
            lastTime;
  DayOrWeek dayOrWeek;

  static DayOrWeek parseTimeSpec( const char *timeSpec,  TimeMSecs &ms );

  static void adjustWeekday( TimeMSecs &rotTime,  const char *timeSpec );

  void init( void ) {
    this->time      = 0;
    this->period    = 0;
    this->lastTime  = 0;
    this->dayOrWeek = ROTATE_UNSPECIFIED;
  };
  void setLastTime( TimeMSecs lt ) {
    this->lastTime = lt;
  };
  bool setRotateTime( const char *timeSpec,
                      DayOrWeek rotDorW = ROTATE_UNSPECIFIED,
                      TimeMSecs rotTime = 0 );
  bool setRotatePeriod( const char *periodSpec,  TimeMSecs rotatePeriod = 0 );

  bool checkRotate( void );

  TimeMSecs nextRotate( TimeMSecs *diffTime = NULL );

  TimeMSecs getInterval( void );
};

struct RAIBASE_DLL_EXP NanoSleep {
  int       fd;
  TimeHires last;
  double    rateMSec;

  /* there should only be one of these, /dev/rtc is limited to one process */
  NanoSleep() {
    this->fd       = -1;
    this->last     = 0;
    this->rateMSec = 0.0;
  };
    
  bool open( unsigned int hz = 8192,  const char *rtcName = "/dev/rtc" );

  void close( void );
    
  double sleep( double ms ); /* return amount of time in sleep() */
};

class RAIBASE_DLL_EXP LatencyMon {
  public:
    TimeHires    accum,
                 minVal,
                 maxVal,
                 stamp;
    unsigned int count;

    LatencyMon() {
      this->reset();
      this->stamp = 0;
    };
    void reset() {
      this->accum  = 0;
      this->minVal = ~(TimeHires) 0;
      this->maxVal = 0;
      this->count  = 0;
    };

    virtual ~LatencyMon() {};

    virtual void updateLatency( TimeHires val ) = 0;

    void getLat( TimeHires &acc,  TimeHires &min,  TimeHires &max,
                 unsigned int &cnt ) {
      acc = this->accum;
      min = this->minVal;
      max = this->maxVal;
      cnt = this->count;
      this->reset();
    };
    void updateLat( TimeHires val ) {
      if ( val != 0 ) {
        this->accum += val;
        this->count++;
        if ( this->minVal > val )
          this->minVal = val;
        if ( this->maxVal < val )
          this->maxVal = val;
      }
    };
};

namespace Time {
  enum TZone {
    TZ_LOCAL_TIME = 0,
    TZ_GM_TIME    = 1
  };

  static const TimeMSecs MAX_TIME_MSECS = ~(TimeMSecs) 0;
  static const TimeHires MAX_TIME_HIRES = ~(TimeHires) 0;
  static const TimeSecs  MAX_TIME_SECS  = ~(TimeSecs) 0;

  RAIBASE_DLL_EXP
  char *timestamp( char *buf,  unsigned int bufLen );

  RAIBASE_DLL_EXP
  char *timestamp( TimeMSecs when,  char *buf,  unsigned int bufLen );

  RAIBASE_DLL_EXP
  char *timestamp( TimeNSecs when,  unsigned int precision,  char *buf,
                   unsigned int bufLen );
  RAIBASE_DLL_EXP
  char *strftime( int tz,  TimeMSecs when,  const char *fmt,  char *buf,
                  unsigned int bufLen );
  RAIBASE_DLL_EXP
  TimeMSecs strptime( int tz,  const char *fmt,  const char *buf );

  enum ClrFlags {
    CLR_NONE = 0, CLR_SEC = 1, CLR_MIN = 2, CLR_HOUR = 3, CLR_DAY = 4,
    CLR_MONTH = 5
  };
  RAIBASE_DLL_EXP
  TimeMSecs strptime_clr( int tz,  const char *fmt,  const char *buf,
                          int clr );
  RAIBASE_DLL_EXP
  bool strptime_date( const char *fmt,  const char *buf,  int &mday,
                      int &mon,  int &year );
  RAIBASE_DLL_EXP
  bool strptime_time( const char *fmt,  const char *buf,  int &hour,
                      int &min,  int &sec );
  /* if dayName = NULL, return current day, otherwise dayName = Mon, Tue, etc */
  RAIBASE_DLL_EXP
  int getDayOfWeek( const char *dayName = NULL );

  RAIBASE_DLL_EXP
  void getymdhms( int tz,  TimeMSecs when,  unsigned int &yr,
                  unsigned int &mon,  unsigned int &day,  unsigned int &hr,
                  unsigned int &min,  unsigned int &sec );
  RAIBASE_DLL_EXP
  void gethms( int tz,  TimeMSecs when,  unsigned int &hr,  unsigned int &min,
               unsigned int &sec );
  RAIBASE_DLL_EXP
  void getymd( int tz,  TimeMSecs when,  unsigned int &yr,  unsigned int &mon,
               unsigned int &day );
  RAIBASE_DLL_EXP
  TimeMSecs getms( unsigned int yr,  unsigned int mon,  unsigned int day,
                   unsigned int hr,  unsigned int min,  unsigned int sec, 
                   unsigned int msec );
  RAIBASE_DLL_EXP
  char *hmsToString( unsigned int hr,  unsigned int min,  unsigned int sec,
                     char *buf,  unsigned int bufLen,  const char *sep = ":" );
  RAIBASE_DLL_EXP
  char *hmToString( unsigned int hr,  unsigned int min,
                    char *buf,  unsigned int bufLen,  const char *sep = ":" );
  enum YMDFormat {
    YYMMDD, YYYYMMDD, MMDDYY, MMDDYYYY, DDMMYY, DDMMYYYY, DDMMMYYYY, DDMMM
  };
  RAIBASE_DLL_EXP
  char *ymdToString( unsigned int yr,  unsigned int mon,  unsigned int day,
                     char *buf,  unsigned int bufLen,  YMDFormat fmt = YYMMDD,
                     const char *sep = "/" );
  RAIBASE_DLL_EXP
  void initialize( const char *timeSource = NULL ); /* init hires calculations*/
  RAIBASE_DLL_EXP
  TimeNSecs getWallclockNanosecs( void );
  /* wrap a sys call for current time with two hires time stamps */
  RAIBASE_DLL_EXP
  TimeNSecs syncTimeToHires( TimeHires &n1,  TimeHires &n2 );
  /* if monotonic clock drifts from wall clock, these get the adjustments */
  RAIBASE_DLL_EXP
  void getFirstTimeAdjustment( unsigned int &index,  TimeNSecs &when,
                               llong &amountNS );
  RAIBASE_DLL_EXP
  bool getNextTimeAdjustment( unsigned int &index,  TimeNSecs &when,
                              llong &amountNS );
  /* returns null terminated array of available time sources */
  RAIBASE_DLL_EXP
  const char **getAvailableTimeSources( char *buf = NULL, /* Arg description */
                                unsigned int bufLen = 0,
                                char *defaultSrc = NULL,  /* Name of default */
                                unsigned int defaultLen = 0,
                                bool preferTSC = false ); /* If TSC preferred */
  RAIBASE_DLL_EXP
  const char *getCurrentTimeSource( void );

  RAIBASE_DLL_EXP
  bool testHiresClock( void ); /* test x86 tsr on mp machines */

  RAIBASE_DLL_EXP
  bool warnHiresDiff( void ); /* log x86 tsr difference */

  RAIBASE_DLL_EXP
  TimeMSecs uptime( TimeNSecs ns = 0 );

  RAIBASE_DLL_EXP
  TimeHires getHiresTime( double *perMSec = NULL );

  RAIBASE_DLL_EXP
  void initHiresSeed( const byte *entropy,  unsigned int len );

  RAIBASE_DLL_EXP /* event counter incremented in getHiresTime(), useful for */
  TimeHires getHiresSeed( void );  /* rand seeding entropy */

  RAIBASE_DLL_EXP
  double getCyclesPerMSec( void );
  /* hres -> time */
  RAIBASE_DLL_EXP
  TimeMSecs hiresToMillisecs( TimeHires htime );
  /* time in 1,000 */
  inline TimeMSecs currentTimeMillisecs( void ) { return hiresToMillisecs( 0 ); };
  /* hres -> time */
  RAIBASE_DLL_EXP
  TimeUSecs hiresToMicrosecs( TimeHires htime );
  /* time in 1,000,000 */
  inline TimeUSecs currentTimeMicrosecs( void ) { return hiresToMicrosecs( 0 ); };
  /* hres -> time */
  RAIBASE_DLL_EXP
  TimeNSecs hiresToNanosecs( TimeHires htime );
  /* time in 1,000,000,000 */
  inline TimeNSecs currentTimeNanosecs( void ) { return hiresToNanosecs( 0 ); };
  /* returns count of threads, zero if unable to compute */
  RAIBASE_DLL_EXP
  unsigned int getRusage( TimeMSecs &sysTime,  TimeMSecs &userTime,
                          TimeMSecs &totalTime,  TimeMSecs *taskSysTime = NULL,
                          TimeMSecs *taskUserTime = NULL,
                          unsigned int *taskId = NULL,
                          unsigned int maxTaskCnt = 0 );
  RAIBASE_DLL_EXP
  unsigned int getRusage2( TimeMSecs &sysTime,  TimeMSecs &userTime,
                           TimeMSecs &totalTime,  ullong &rss,  ullong &vsz,
                           TimeMSecs *taskSysTime = NULL,
                           TimeMSecs *taskUserTime = NULL,
                           unsigned int *taskId = NULL,
                           unsigned int maxTaskCnt = 0 );
  inline TimeSecs currentTimeSeconds( void ) {
    return (TimeSecs) ( currentTimeMillisecs() / 1000U );
  }
  inline TimeMSecs secsToMillisecs( TimeSecs time ) {
    return (TimeMSecs) time * (TimeMSecs) 1000U;
  }
  inline TimeMSecs nanosecsToMillisecs( TimeNSecs ns ) {
    return (TimeMSecs) ( ns / (TimeNSecs) 1000000 );
  }
  inline TimeUSecs nanosecsToMicrosecs( TimeNSecs ns ) {
    return (TimeUSecs) ( ns / (TimeNSecs) 1000 );
  }
  inline TimeUSecs secsToMicrosecs( TimeSecs time ) {
    return (TimeMSecs) time * (TimeMSecs) ( 1000U * 1000U );
  }
  inline TimeNSecs secsToNanosecs( TimeSecs time ) {
    return (TimeNSecs) time * (TimeNSecs) ( 1000U * 1000U * 1000U );
  }
  inline TimeNSecs millisecsToNanosecs( TimeSecs t,  TimeMSecs ms ) {
    return (TimeNSecs) t * (TimeNSecs) 1000000000 +
           (TimeNSecs) ms * (TimeNSecs) 1000000;
  }
  inline TimeNSecs nanosecsToNanosecs( TimeSecs t,  TimeNSecs ns ) {
    return (TimeNSecs) t * (TimeNSecs) 1000000000 + ns;
  }
  inline TimeNSecs microsecsToNanosecs( TimeSecs t,  TimeUSecs us ) {
    return (TimeNSecs) t * (TimeNSecs) 1000000000 +
           (TimeNSecs) us * (TimeNSecs) 1000;
  }
  inline double millisecsToSecs( TimeMSecs ms ) {
    return (double) ms / 1000.0;
  }
  inline double microsecsToSecs( TimeUSecs us ) {
    return (double) us / 1000000.0;
  }
  inline double nanosecsToSecs( TimeNSecs ns ) {
    return (double) ns / 1000000000.0;
  }
  RAIBASE_DLL_EXP
  void sleepMillisecs( TimeMSecs mSecs );

  RAIBASE_DLL_EXP
  void sleepNanosecs( TimeNSecs nSecs /* must be < 1,000,000,000 */ );

  RAIBASE_DLL_EXP
  void sleepMS( double mSecs );
  /* return the number of seconds offset from GMT */
  RAIBASE_DLL_EXP
  int utcOffsetSecs();
}

} // namespace rai

#endif
