/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#if defined( _WIN32 ) || defined( _WIN64 )
  #include <windows.h>
#endif

#include "base/types.h"

#if ( defined( __linux ) || defined( __APPLE__ ) ) && ! defined( _XOPEN_SOURCE )
/* for strptime() */
#define _XOPEN_SOURCE
#endif
#if defined( __linux )
#include <errno.h>
#endif

#include <time.h>

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  #include <sys/time.h>
  #include <sys/types.h>
  #include <sys/resource.h>
  #include <unistd.h>
  #if defined( __linux )
    #include <sys/ioctl.h>
    #include <fcntl.h>
    #include <linux/rtc.h>
  #elif defined( __sparcv9 ) || \
        ( defined( __sun__ ) && defined( __amd64__ ) ) || \
        ( defined( __sparc ) && ! defined( _FILE_OFFSET_BITS ) )
    /* can't use procfs when 32 bits and _FILE_OFFSET_BITS=64 */
    #include <sys/procfs.h>
  #endif
#endif

#include "base/time.h"
#include "base/sys.h"
#include "base/thread.h"
#include "stream/io_stream.h"
#include "stream/stdio_stream.h"
#include "base/file.h"
#include "base/dir.h"
#include "base/log.h"
#include "util/str_util.h"
#include "util/atomic.h"

#if defined( _WIN32 ) || defined( _WIN64 ) || ( defined( __sun__ ) && ! defined( __GLIBC__ ) )
#include "util/strptime.h"
#else
extern "C" char *strptime(const char *s, const char  *format,  struct tm *tm);
#endif

using namespace rai;


char *
Time::timestamp( char *buf,  unsigned int bufLen )
{
  return Time::timestamp( Time::currentTimeMillisecs(), buf, bufLen );
}


char *
Time::timestamp( TimeMSecs when,  char *buf,  unsigned int bufLen )
{
  return Time::strftime( Time::TZ_LOCAL_TIME, when, "%Y-%m-%d %H:%M:%S",
                         buf, bufLen );
}


char *
Time::timestamp( TimeNSecs when,  unsigned int precision,  char *buf,
                 unsigned int bufLen )
{
  Time::strftime( Time::TZ_LOCAL_TIME, when / 1000000, "%Y-%m-%d %H:%M:%S",
                  buf, bufLen );
  if ( precision > 0 ) {
    unsigned int i = ::strlen( buf ),
                 j = 1000000000,
                 k = 100000000;
    if ( i < bufLen - 1 )
      buf[ i++ ] = '.';
    for ( ; i < bufLen - 1 && precision > 0; precision-- ) {
      buf[ i++ ] = '0' + ( ( when % j ) / k );
      j = k; k /= 10;
    }
    buf[ i ] = '\0';
  }
  return buf;
}

#if defined( _WIN32 ) || defined( _WIN64 )
static AtomicUInt rai_tmspin;

static struct tm *
rai_gmtime_r( time_t *t, struct tm *tm )
{
  while ( rai_tmspin.xchg( 1 ) != 0 )
    ;
  struct tm * tmp = gmtime( t );
  *tm = *tmp;
  rai_tmspin.xchg( 0 );
  return tm;
}

#define gmtime_r rai_gmtime_r

static struct tm *
rai_localtime_r( time_t *t, struct tm *tm )
{
  while ( rai_tmspin.xchg( 1 ) != 0 )
    ;
  struct tm * tmp = localtime( t );
  *tm = *tmp;
  rai_tmspin.xchg( 0 );
  return tm;
}

#define localtime_r rai_localtime_r

#endif

#if defined( __linux ) || defined( __APPLE__ )
#define rai_strptime strptime
#endif

char *
Time::strftime( int tz,  TimeMSecs when,  const char *fmt,  char *buf,
                unsigned int bufLen )
{
  struct tm tm;

  time_t t = (time_t) (unsigned long) ( when / (TimeMSecs) 1000U );

  if ( tz == Time::TZ_GM_TIME )
    ::gmtime_r( &t, &tm );
  else
    ::localtime_r( &t, &tm );

  buf[ 0 ] = '\0';
  ::strftime( buf, bufLen, fmt, &tm );

  buf[ bufLen - 1 ] = '\0';

  return buf;
}


TimeMSecs
Time::strptime( int tz,  const char *fmt,  const char *buf )
{
  return Time::strptime_clr( tz, fmt, buf, Time::CLR_SEC );
}



TimeMSecs
Time::strptime_clr( int tz,  const char *fmt,  const char *buf,  int clr )
{
  struct tm tmbuf;
  time_t t, t2 = -1;

  /* initialize tm with current time so that %M:%S works */
  t = (time_t) ( Time::currentTimeMillisecs() / (TimeMSecs) 1000 );
  if ( tz == Time::TZ_GM_TIME )
    ::gmtime_r( &t, &tmbuf );
  else
    ::localtime_r( &t, &tmbuf );
  switch ( clr ) {
    case CLR_MONTH:
      tmbuf.tm_mon = 0;
    case CLR_DAY:
      tmbuf.tm_mday = 1;
    case CLR_HOUR:
      tmbuf.tm_hour = 0;
    case CLR_MIN:
      tmbuf.tm_min = 0;
    case CLR_SEC:
      tmbuf.tm_sec = 0;
    case CLR_NONE:
      break;
  }

  if ( ::rai_strptime( buf, fmt, &tmbuf ) == NULL )
    return 0;

  if ( tz == Time::TZ_GM_TIME ) {
#if defined( _WIN32 ) || defined( _WIN64 ) || ! defined( __GLIBC__ )
    static time_t skewGM = 99;
    if ( skewGM == 99 ) {
      time_t xt, xt2;
      struct tm tm2;

      ::time( &xt );
      ::memcpy( &tm2, gmtime( &xt ), sizeof( tm2 ) );
      xt2 = ::mktime( &tm2 );
      skewGM = (time_t) difftime( xt2, xt );
    }
    t2 = ::mktime( &tmbuf );
    if ( t2 != -1 )
      t2 += skewGM;
#else
    t2 = ::timegm( &tmbuf );
#endif
  }
  else {
    struct tm tmbuf2;
    ::memcpy( &tmbuf2, &tmbuf, sizeof( tmbuf2 ) );
    t2 = ::mktime( &tmbuf );
    /* if daylight saving changed, put back the hour and recompute */
    if ( tmbuf.tm_isdst != tmbuf2.tm_isdst ) {
      tmbuf.tm_hour = tmbuf2.tm_hour;
      tmbuf.tm_mday = tmbuf2.tm_mday;
      tmbuf.tm_mon  = tmbuf2.tm_mon;
      tmbuf.tm_year = tmbuf2.tm_year;
      t2 = ::mktime( &tmbuf );
    }
  }

  if ( t2 == -1 )
    return 0;
  return (TimeMSecs) 1000 * (TimeMSecs) (unsigned long) t2;
}


bool
Time::strptime_date( const char *fmt,  const char *buf,  int &mday,
                     int &mon,  int &year )
{
  struct tm tm;
  tm.tm_mday = -1;
  tm.tm_mon  = -1;
  tm.tm_year = -1;
  if ( ::rai_strptime( buf, fmt, &tm ) == NULL )
    return false;
  mday = tm.tm_mday;
  mon  = tm.tm_mon;
  year = tm.tm_year;
  return true;
}


bool
Time::strptime_time( const char *fmt,  const char *buf,  int &hour,
                     int &min,  int &sec )
{
  struct tm tm;
  tm.tm_hour = -1;
  tm.tm_min  = -1;
  tm.tm_sec  = -1;
  if ( ::rai_strptime( buf, fmt, &tm ) == NULL )
    return false;
  hour = tm.tm_hour;
  min  = tm.tm_min;
  sec  = tm.tm_sec;
  return true;
}


int
Time::getDayOfWeek( const char *dayName )
{
  struct tm tm;

  ::memset( &tm, 0, sizeof( tm ) );
  if ( dayName != NULL ) {
    if ( ::rai_strptime( dayName, "%a", &tm ) == NULL )
      return -1;
  }
  else {
    time_t t = time( NULL );
    ::localtime_r( &t, &tm );
  }
  return tm.tm_wday;
}


void
Time::getymdhms( int tz,  TimeMSecs when,  unsigned int &yr,  unsigned int &mon,
                 unsigned int &day,  unsigned int &hr,  unsigned int &min,
                 unsigned int &sec )
{
  struct tm tmbuf;

  time_t t = (time_t) ( when / (TimeMSecs) 1000 );
  if ( tz == Time::TZ_GM_TIME )
    ::gmtime_r( &t, &tmbuf );
  else
    ::localtime_r( &t, &tmbuf );

  yr  = tmbuf.tm_year + 1900;
  mon = tmbuf.tm_mon + 1; /* range 1 -> 12 */
  day = tmbuf.tm_mday;
  hr  = tmbuf.tm_hour;
  min = tmbuf.tm_min;
  sec = tmbuf.tm_sec;
}


TimeMSecs
Time::getms( unsigned int yr,  unsigned int mon,  unsigned int day,
             unsigned int hr,  unsigned int min,  unsigned int sec,
             unsigned int msec )
{
  struct tm tmbuf;
  time_t t;

  ::memset( &tmbuf, 0, sizeof( tmbuf ) );
  tmbuf.tm_year  = yr - 1900;
  tmbuf.tm_mon   = mon - 1; /* range 1 -> 12 */
  tmbuf.tm_mday  = day;
  tmbuf.tm_hour  = hr;
  tmbuf.tm_min   = min;
  tmbuf.tm_sec   = sec;
  tmbuf.tm_isdst = -1;

  t = ::mktime( &tmbuf );
  if ( t == -1 )
    return 0;
  return (TimeMSecs) 1000 * (TimeMSecs) (unsigned long) t + (TimeMSecs) msec;
}


void
Time::gethms( int tz,  TimeMSecs when,  unsigned int &hr,  unsigned int &min,
              unsigned int &sec )
{
  unsigned int y, m, d;
  Time::getymdhms( tz, when, y, m, d, hr, min, sec );
}


void
Time::getymd( int tz,  TimeMSecs when,  unsigned int &yr,  unsigned int &mon,
              unsigned int &day )
{
  unsigned int h, m, s;
  Time::getymdhms( tz, when, yr, mon, day, h, m, s );
}


static inline char *
num3( const unsigned int *n,  const byte *f,  char *buf,  unsigned int bufLen,
      const char *sep )
{
  unsigned int off;
  const char * ptr;

  for ( off = 0; off < bufLen; n++ ) {
    if ( off + *f >= bufLen )
      break;
    switch ( *f++ ) {
      case 3: {
        static const char *mon[ 13 ] = { "JAN", "FEB", "MAR", "APR", "MAY",
                                         "JUN", "JUL", "AUG", "SEP", "OCT",
                                         "NOV", "DEC", "---" };
        ptr = mon[ *n >= 1 && *n <= 12 ? *n - 1 : 12 ];
        buf[ off++ ] = *ptr++;
        buf[ off++ ] = *ptr++;
        buf[ off++ ] = *ptr++;
        break;
      }
      case 4:
        buf[ off++ ] = '0' + (byte) ( ( *n / 1000 ) % 10 );
        buf[ off++ ] = '0' + (byte) ( ( *n / 100 ) % 10 );
      case 2:
        buf[ off++ ] = '0' + (byte) ( ( *n / 10 ) % 10 );
        buf[ off++ ] = '0' + (byte) ( *n % 10 );
        break;
    }
    if ( *f == 0 )
      break;
    if ( off < bufLen )
      buf[ off++ ] = sep[ 0 ];
  }
  if ( off < bufLen )
    buf[ off ] = '\0';
  return buf;
}


static const byte f22[]  = { 2, 2, 0 },
                  f23[]  = { 2, 3, 0 },
                  f222[] = { 2, 2, 2, 0 },
                  f422[] = { 4, 2, 2, 0 },
                  f224[] = { 2, 2, 4, 0 },
                  f234[] = { 2, 3, 4, 0 };
char *
Time::hmsToString( unsigned int hr,  unsigned int min,  unsigned int sec,
                   char *buf,  unsigned int bufLen,  const char *sep )
{
  const unsigned int hms[] = { hr, min, sec };
  return num3( hms, f222, buf, bufLen, sep );
}


char *
Time::hmToString( unsigned int hr,  unsigned int min,
                  char *buf,  unsigned int bufLen,  const char *sep )
{
  const unsigned int hm[] = { hr, min };
  return num3( hm, f22, buf, bufLen, sep );
}


char *
Time::ymdToString( unsigned int yr,  unsigned int mon,  unsigned int day,
                   char *buf,  unsigned int bufLen,  YMDFormat fmt,
                   const char *sep )
{
  switch ( fmt ) {
    case YYMMDD: case YYYYMMDD: {
      const unsigned int ymd[] = { yr, mon, day };
      if ( fmt == YYMMDD )
        return num3( ymd, f222, buf, bufLen, sep );
      return num3( ymd, f422, buf, bufLen, sep );
    }
    case MMDDYY: case MMDDYYYY: {
      const unsigned int mdy[] = { mon, day, yr };
      if ( fmt == MMDDYY )
        return num3( mdy, f222, buf, bufLen, sep );
      return num3( mdy, f224, buf, bufLen, sep );
    }
    case DDMMM: {
      const unsigned int dm[] = { day, mon };
      return num3( dm, f23, buf, bufLen, sep );
    }
    default:
    case DDMMYY: case DDMMYYYY: case DDMMMYYYY: {
      const unsigned int dmy[] = { day, mon, yr };
      if ( fmt == DDMMYY )
        return num3( dmy, f222, buf, bufLen, sep );
      if ( fmt == DDMMMYYYY )
        return num3( dmy, f234, buf, bufLen, sep );
      return num3( dmy, f224, buf, bufLen, sep );
    }
  }
}


/* TIMEOFDAY is a very very bad timesource */
#define HAS_TIMEOFDAY
/* clock_gettime() always uses system call */
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  #if defined( CLOCK_MONOTONIC )
    #define HAS_CLOCK_MONOTONIC
  #endif
  #if defined( CLOCK_REALTIME )
    #define HAS_CLOCK_REALTIME
  #endif
#endif
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  #define HAS_X86_TSC
  #define HAS_TSC_HIRES
#endif
#if defined( _WIN32 ) || defined( _WIN64 )
  #define HAS_QUERYPERF_TSC
  #define HAS_TSC_HIRES
  #define HAS_GETTICKCOUNT
  #define HAS_TIMEGETTIME
  #undef HAS_TIMEOFDAY
#endif
#if defined( __sun__ )
  #define HAS_HRTIME_TSC
  #define HAS_TSC_HIRES
#endif

static bool hasBuiltinFreq;
#if defined( HAS_CLOCK_MONOTONIC )
static TimeHires clockHiresTime( void );
static bool clockCyclesPerMillisec( double &cm,  double &cn );
#ifndef DEFAULT_HIRES_TIME   
#define DEFAULT_HIRES_TIME   clockHiresTime
#define DEFAULT_HIRES_CYCLES clockCyclesPerMillisec
#endif
#endif
#if defined( HAS_TSC_HIRES ) || defined( HAS_QUERYPERF_TSC )
/* traditional TSC polling */
static TimeHires tscHiresTime( void );
static bool tscCyclesPerMillisec( double &cm,  double &cn );
#ifndef DEFAULT_HIRES_TIME   
#define DEFAULT_HIRES_TIME   tscHiresTime
#define DEFAULT_HIRES_CYCLES tscCyclesPerMillisec
#endif
#endif
#ifdef HAS_TIMEGETTIME
static TimeHires timeHiresTime( void );
static bool timeCyclesPerMillisec( double &cm,  double &cn );
static UINT timeGetTimeResolution;
#ifndef DEFAULT_HIRES_TIME
#define DEFAULT_HIRES_TIME   timeHiresTime
#define DEFAULT_HIRES_CYCLES timeCyclesPerMillisec
#endif
#endif
#if defined( HAS_GETTICKCOUNT )
static TimeHires tickHiresTime( void );
static bool tickCyclesPerMillisec( double &cm,  double &cn );
#ifndef DEFAULT_HIRES_TIME
#define DEFAULT_HIRES_TIME   tickHiresTime
#define DEFAULT_HIRES_CYCLES tickCyclesPerMillisec
#endif
#endif
#if defined( HAS_TIMEOFDAY )
/* gettimeofday(), not monotonic, probably shouldn't use */
static TimeHires tofdHiresTime( void );
static bool tofdCyclesPerMillisec( double &cm,  double &cn );
#ifndef DEFAULT_HIRES_TIME   
#define DEFAULT_HIRES_TIME   tofdHiresTime
#define DEFAULT_HIRES_CYCLES tofdCyclesPerMillisec
#endif
#endif
static TimeHires (*hiresTime)( void ) = DEFAULT_HIRES_TIME;
static bool (*getCyclesPerMillisec)( double &cm,  double &cn )
                                      = DEFAULT_HIRES_CYCLES;

#if defined( HAS_X86_TSC ) && ! defined( HAS_HRTIME_TSC )

static byte
getCurrentApicId( void )
{
  unsigned int x = 0;
  /* the cpu id, which is usually physical cpu or core unless hyperthreading */
#if defined( __amd64__ )
  ullong y;
  __asm__ __volatile__ (   "movq %%rbx, %1\n"
                         "\tmovl $1, %%eax\n"
                         "\tcpuid\n"
                         "\tmovl %%ebx, %0\n"
                         "\tmovq %1, %%rbx\n"
                           :
                           : "m" (x), "m" (y)
                           : "%rax", "%rcx", "%rdx" );
#elif defined( __i386 ) /* > 486 */
  unsigned int y;
  __asm__ __volatile__ (   "movl %%ebx, %1\n"
                         "\tmovl $1, %%eax\n"
                         "\tcpuid\n"
                         "\tmovl %%ebx, %0\n"
                         "\tmovl %1, %%ebx\n"
                           :
                           : "m" (x), "m" (y)
                           : "%eax", "%ecx", "%edx" );
#endif
  return x >> 24;
}

#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
static TimeHires
calculateHiresDiff( unsigned int c0,  unsigned int c1,  unsigned short &apicId )
{
  CpuAffinity cpu0, cpu1, cur;
  TimeHires diff, x, y, z, h0, h1;
  unsigned int i;

  apicId = 0xffffU;
  if ( ! cur.getAffinity() )
    return 0;

  h0 = 0;
  h1 = 0;
  diff = 0;
  cpu0.zero();
  cpu1.zero();
  cpu0.set( c0 );
  cpu1.set( c1 );
  
  if ( ! cur.getAffinity() )
    return 0;

  apicId = getCurrentApicId();
  
  /* diff hires time between cpu0 and cpu1 */
  for ( i = 0; i < 20; i++ ) {
    if ( ! cpu0.setAffinity() )
      break;
    x = Time::getHiresTime();

    if ( ! cpu1.setAffinity() )
      break;
    y = Time::getHiresTime();

    if ( ! cpu0.setAffinity() )
      break;
    z = Time::getHiresTime();

    if ( i == 0 || z - x < diff ) {
      diff = z - x;
      h0   = x + diff / 2;
      h1   = y;
    }
  }
  
  cur.setAffinity();
  
  return h0 - h1;
}
#endif

#endif

static struct TimeData {
  unsigned int sampleShift,
               currentSample;
  TimeHires    baseCyclestamp;
  TimeNSecs    baseTimestamp,
               lastTimestamp,
               zeroTimestamp;
  TimeMSecs    baseTimestampMSecs;
  TimeUSecs    baseTimestampUSecs;
  double       cyclesPerNanosec,
               cyclesPerMillisec;
  llong        adjustmentNS;
} td = { 3 }, td2 = { 3 }, tdsave[ 128 ];

static unsigned int tdsaveCount; /* index into tdsave[] */

static TimeNSecs timeAdjustments; /* for uptime */

void
Time::getFirstTimeAdjustment( unsigned int &index,  TimeNSecs &when,
                              llong &amountNS )
{
  if ( tdsaveCount == 0 )
    index = sizeof( tdsave ) / sizeof( tdsave[ 0 ] ) - 1;
  else
    index = tdsaveCount - 1;
  when     = td.baseTimestamp;
  amountNS = td.adjustmentNS;
}

bool
Time::getNextTimeAdjustment( unsigned int &index,  TimeNSecs &when,
                             llong &amountNS )
{
  if ( index == tdsaveCount )
    return false;
  if ( index == 0 )
    index = sizeof( tdsave ) / sizeof( tdsave[ 0 ] ) - 1;
  else
    index = index - 1;
  when     = tdsave[ index ].baseTimestamp;
  amountNS = tdsave[ index ].adjustmentNS;
  if ( when == 0 )
    return false;
  return true;
}


static const char *timeSources[] = {
/* first timeSource is default */
#if defined( HAS_HRTIME_TSC )
  "gethrtime",
#endif
#if defined( HAS_CLOCK_MONOTONIC )
  "clock_gettime",
#endif
#if defined( HAS_QUERYPERF_TSC )
  "QueryPerformanceCounter",
#endif
#if defined( HAS_TIMEGETTIME )
  "timeGetTime",
#endif
#if defined( HAS_GETTICKCOUNT )
  "GetTickCount",
#endif
#if defined( HAS_TSC_HIRES ) && ! defined( HAS_HRTIME_TSC ) && ! defined( HAS_QUERYPERF_TSC )
  "TSC",
#endif
#if defined( HAS_TIMEOFDAY )
  "gettimeofday",
#endif
  NULL
};

const char **
Time::getAvailableTimeSources( char *buf,  unsigned int bufLen,
                               char *defaultSrc,  unsigned int defaultLen,
                               bool preferTSC )
{
  if ( buf != NULL && bufLen > 0 ) {
    char availDescr[ 80 * 3 ], *a = availDescr;
    ::strcpy( a, "Select a high resolution time source; valid: " );
    a = &a[ ::strlen( a ) ];
    unsigned int tscnt = 0;
    for ( const char **ts = Time::getAvailableTimeSources(); *ts; ts++ ) {
      for ( const char *s = *ts; *s; *a++ = *s++ )
        ;
      if ( ts[ 1 ] ) {
        *a++ = ',';
        *a++ = ' ';
      }
      if ( tscnt++ == 0 || ( ::strcmp( ts[ 0 ], "TSC" ) == 0 && preferTSC ) ) {
        if ( defaultSrc != NULL && defaultLen > 0 ) {
          ::strncpy( defaultSrc, ts[ 0 ], defaultLen - 1 );
          defaultSrc[ defaultLen - 1 ] = '\0';
        }
      }
    }
    a[ 0 ] = '\0';
    if ( buf != NULL && bufLen > 0 ) {
      ::strncpy( buf, availDescr, bufLen - 1 );
      buf[ bufLen - 1 ] = '\0';
    }
  }
  return timeSources;
}


const char *
Time::getCurrentTimeSource( void )
{
#if defined( HAS_HRTIME_TSC )
  if ( hiresTime == tscHiresTime )
    return "gethrtime";
#elif defined( HAS_QUERYPERF_TSC )
  if ( hiresTime == tscHiresTime )
    return "QueryPerformanceCounter";
#elif defined( HAS_TSC_HIRES )
  if ( hiresTime == tscHiresTime )
    return "TSC";
#endif
#if defined( HAS_CLOCK_MONOTONIC )
  if ( hiresTime == clockHiresTime )
    return "clock_gettime";
#endif
#if defined( HAS_TIMEOFDAY )
  if ( hiresTime == tofdHiresTime )
    return "gettimeofday";
#endif
#if defined( HAS_TIMEGETTIME )
  if ( hiresTime == timeHiresTime )
    return "timeGetTime";
#endif
#if defined( HAS_GETTICKCOUNT )
  if ( hiresTime == tickHiresTime )
    return "GetTickCount";
#endif
  return NULL;
}


void
Time::initialize( const char *timeSource )
{
  const char *s;

  hiresTime            = DEFAULT_HIRES_TIME;
  getCyclesPerMillisec = DEFAULT_HIRES_CYCLES;

  ::memset( &td, 0, sizeof( td ) );
  ::memset( &td2, 0, sizeof( td2 ) );
  timeAdjustments = 0;
  td.sampleShift  = 3;
  td2.sampleShift = 3;

  if ( (s = timeSource) == NULL )
    if ( (s = ::getenv( "RAI_TIME" )) == NULL )
      s = timeSources[ 0 ];
  if ( s != NULL ) {
    if ( StrUtil::strncasecmp( s, "USE_", 4 ) == 0 )
      s = &s[ 4 ];
#if defined( HAS_TSC_HIRES ) || defined( HAS_QUERYPERF_TSC )
    if ( ::strcmp( s, "0" ) == 0 ||
         StrUtil::strcasecmp( s, "TSC" ) == 0 ||
         StrUtil::strcasecmp( s, "GETHRTIME" ) == 0 ||
         StrUtil::strcasecmp( s, "QueryPerformanceCounter" ) == 0 ) {
      hiresTime            = tscHiresTime;
      getCyclesPerMillisec = tscCyclesPerMillisec;
    }
#endif
#if defined( HAS_CLOCK_MONOTONIC )
    if ( ::strcmp( s, "1" ) == 0 ||
         StrUtil::strcasecmp( s, "CLOCK_GETTIME" ) == 0 ) {
      hiresTime            = clockHiresTime;
      getCyclesPerMillisec = clockCyclesPerMillisec;
    }
#endif
#if defined( HAS_TIMEOFDAY )
    if ( ::strcmp( s, "2" ) == 0 ||
         StrUtil::strcasecmp( s, "GETTIMEOFDAY" ) == 0 ) {
      hiresTime            = tofdHiresTime;
      getCyclesPerMillisec = tofdCyclesPerMillisec;
    }
#endif
#if defined( HAS_TIMEGETTIME )
    if ( ::strcmp( s, "3" ) == 0 ||
         StrUtil::strcasecmp( s, "TIMEGETTIME" ) == 0 ) {
      hiresTime            = timeHiresTime;
      getCyclesPerMillisec = timeCyclesPerMillisec;
    }
#endif
#if defined( HAS_GETTICKCOUNT )
    if ( ::strcmp( s, "4" ) == 0 ||
         StrUtil::strcasecmp( s, "GETTICKCOUNT" ) == 0 ) {
      hiresTime            = tickHiresTime;
      getCyclesPerMillisec = tickCyclesPerMillisec;
    }
#endif
  }
#if defined( HAS_TIMEGETTIME )
  if ( hiresTime == timeHiresTime && timeGetTimeResolution == 0 ) {
    do {
      timeGetTimeResolution++;
    } while ( ::timeBeginPeriod( timeGetTimeResolution ) == TIMERR_NOCANDO &&
              timeGetTimeResolution <= 100 );
  }
#if defined( HAS_GETTICKCOUNT )
  if ( hiresTime == timeHiresTime && timeGetTimeResolution > 100 ) {
    hiresTime = tickHiresTime;
    getCyclesPerMillisec = tickCyclesPerMillisec;
  }
#endif
#endif
  Time::currentTimeMillisecs();

#if defined( __linux )
  try {
    byte   entropy[ 4 * 1024 ];
    File * in = File::openFile( "/dev/urandom", File::FILE_RDONLY );
    in->read( entropy, sizeof( entropy ) );
    in->close();
    delete in;
    Time::initHiresSeed( entropy, sizeof( entropy ) );
  } catch ( ... ) {
  }
#endif
}


#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
/* if x86 smp, difference of cpu timestamp */
static TimeHires diffHires[ CpuAffinity::MAX_CPU_AFFINITY ];
#endif

bool
Time::testHiresClock( void )
{
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  CpuAffinity    cur;
  TimeHires      diff[ CpuAffinity::MAX_CPU_AFFINITY ]; /* max 256 cpus */
  unsigned int   i, j;
  unsigned short apicId; /* index into diff[] */
  bool           isWhacked;

  if ( cur.getAffinity() && hiresTime == tscHiresTime ) {
    isWhacked = false;
    ::memset( diff, 0, sizeof( diff ) );

    for ( i = 0; i < CpuAffinity::MAX_CPU_AFFINITY; i++ )
      if ( cur.isSet( i ) )
        break;

    /* diff hires time on each cpu from cpu0 */
    for ( j = i + 1; j < CpuAffinity::MAX_CPU_AFFINITY; j++ ) {
      if ( cur.isSet( j ) ) {
        TimeHires h = calculateHiresDiff( i, j, apicId );
        if ( apicId < sizeof( diff ) / sizeof( diff[ 0 ] ) ) {
          diff[ apicId ] = h;
          if ( h > 100000 )
            isWhacked = true;
        }
      }
    }
    ::memcpy( diffHires, diff, sizeof( diffHires ) );
    return ! isWhacked; /* return true if ok, false if whacked */
  }
#endif
  return true;
}


bool
Time::warnHiresDiff( void )
{
  bool hasSkew = false;
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  CpuAffinity  cur;
  unsigned int i,
               cpumask,
               cpucnt;
  double       cpms;

  if ( cur.getAffinity() && hiresTime == tscHiresTime ) {
    Time::getHiresTime( &cpms );
    cpucnt = 0;
    for ( i = 0; i < CpuAffinity::MAX_CPU_AFFINITY; i++ )
      if ( cur.isSet( i ) )
        cpucnt = i;
    for ( cpumask = 2; cpumask < cpucnt; cpumask *= 2 )
      ;
    cpumask--;
    for ( i = 0; i < CpuAffinity::MAX_CPU_AFFINITY; i++ ) {
      if ( diffHires[ i ] > 0 ) {
        double ms = (double) (long long) diffHires[ i ] / (double) cpms;
        logNormal( LNORMAL, NULL, "Cpu #%u TSC is skewed by %.6fms",
                  ( i & cpumask ), ms );
        hasSkew = true;
      }
    }
  }
#endif
  return hasSkew;
}

#ifdef HAS_GETTICKCOUNT
static TimeHires
tickHiresTime( void )
{
  return ::GetTickCount();
}

static bool
tickCyclesPerMillisec( double &cm,  double &cn )
{
#if 0
  hasBuiltinFreq = true;
  cm = 1.0;
  cn = cm * 1000000.0;
  return true;
#endif
  return false;
}
#endif

#ifdef HAS_TIMEGETTIME
static TimeHires
timeHiresTime( void )
{
  if ( timeGetTimeResolution == 0 ) {
    do {
      timeGetTimeResolution++;
    } while ( ::timeBeginPeriod( timeGetTimeResolution ) == TIMERR_NOCANDO &&
              timeGetTimeResolution <= 100 );
  }
  return ::timeGetTime();
}

static bool
timeCyclesPerMillisec( double &cm,  double &cn )
{
#if 0
  hasBuiltinFreq = true;
  cm = 1.0;
  cn = cm * 1000000.0;
  return true;
#endif
  return false;
}
#endif

#ifdef HAS_TSC_HIRES
static TimeHires
tscHiresTime( void )
{
  register TimeHires x;
#if defined( HAS_HRTIME_TSC )
  x = (TimeHires) gethrtime();
#elif defined( HAS_QUERYPERF_TSC )
  ::QueryPerformanceCounter( (LARGE_INTEGER *) &x );
#elif defined( __i386 )
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
#elif defined( __amd64__ )
  __asm__ volatile ("rdtsc\n"
                    "\tshlq $32, %%rdx\n"
                    "\torq  %%rdx, %%rax"
                        : "=a" (x)
                        : 
                        : "%rdx" );
#elif defined( __ppc )
  register unsigned int hi1, hi2, lo;
  __asm__ volatile ("1:\tmftbu %0\n"
                    "\tmftb %1\n"
                    "\tmftbu %2\n"
                    "\tcmpw %3,%4\n"
                    "\tbne  1b\n"
                       : =r (hi1), =r (lo), =r (hi2)
                       : 0 (hi1), 2 (hi2) );

  x = (TimeHires) ( (ullong) hi1 << 32 ) | (ullong) lo;
#else
  hiresTime            = clockHiresTime;
  getCyclesPerMillisec = clockCyclesPerMillisec;
  x = clockHiresTime();
#endif
  return x;
}

#if defined( __linux )
static bool
tscCyclesPerMillisec( double &cm,  double &cn )
{
  char         buf[ 1024 ],
             * p;
  double       mhz;
               /*mhz2;
  unsigned int numCpus;*/

  mhz     = 0.0;
  /*mhz2    = 0.0;
  numCpus = 0;*/
  try {
    File           * f = File::openFile( "/proc/cpuinfo", File::FILE_RDONLY );
    StdioInputStream in( f, 1024 ); /* can't seek */

    while ( in.gets( buf, sizeof( buf ) ) > 0 ) {
      if ( ::strncmp( buf, "cpu MHz", 7 ) == 0 &&
           (p = ::strchr( buf, ':' )) != NULL ) {
        for ( p++; isspace( *p ); p++ )
          ;
        StrUtil::parseFloat( p, &mhz );
        break;
        /*if ( numCpus++ == 0 )
          mhz2 = mhz;
        else if ( mhz2 != mhz ) {
          logMinor( LMINOR, "Cpu%u mhz=%.3f different from cpu0 mhz=%.3f!",
                    numCpus-1, mhz, mhz2 );
        }*/
      }
    }

    in.close();
  } catch ( ... ) {
    return 0;
  }
  hasBuiltinFreq = ( ( mhz != 0.0 ) ? true : false );
  /*cpuCount = numCpus;*/
  
  cm = mhz * 1000.0;
  cn = mhz / 1000.0;
  return true;
}

#elif defined( HAS_QUERYPERF_TSC )
static bool
tscCyclesPerMillisec( double &cm,  double &cn )
{
  TimeHires freq;
  if ( ::QueryPerformanceFrequency( (LARGE_INTEGER *) &freq ) ) {
    hasBuiltinFreq = true;
    cm = (double) freq / 1000.0;
    cn = (double) freq / 1000000000.0;
    return true;
  }
  return false;
}

#elif defined( HAS_HRTIME_TSC )
static bool
tscCyclesPerMillisec( double &cm,  double &cn )
{
  hasBuiltinFreq = true;
  cm = 1000000.0; /* nsecs */
  cn = 1.0;
  return true;
}

#else
static bool
tscCyclesPerMillisec( double &cm,  double &cn )
{
  return false;
}
#endif
#endif

#ifdef HAS_CLOCK_MONOTONIC
static TimeHires
clockHiresTime( void )
{
  struct timespec ts;
  static int tp = CLOCK_MONOTONIC;

  if ( ::clock_gettime( tp, &ts ) != 0 ) {
    tp = CLOCK_REALTIME;
    ::clock_gettime( tp, &ts );
  }
  return ( (TimeHires) ts.tv_sec * (TimeHires) 1000000000 ) +
         ( (TimeHires) ts.tv_nsec );
}


static bool
clockCyclesPerMillisec( double &cm,  double &cn )
{
  hasBuiltinFreq = true;
  cm = 1000000.0;
  cn = 1.0;
  return true;
}
#endif

#ifdef HAS_TIMEOFDAY
static TimeHires
tofdHiresTime( void )
{
  struct timeval tv;
  ::gettimeofday( &tv, NULL );
  return ( (TimeHires) tv.tv_sec * (TimeHires) 1000000 ) +
         ( (TimeHires) tv.tv_usec );
}


static bool
tofdCyclesPerMillisec( double &cm,  double &cn )
{
  hasBuiltinFreq = true;
  cm = 1000.0;
  cn = 1.0 / 1000.0;
  return true;
}
#endif


TimeMSecs
Time::uptime( TimeNSecs ns )
{
  /* how many msecs up */
  if ( ns == 0 )
    ns = Time::currentTimeNanosecs();
  ns -= td.baseTimestamp;
  ns += timeAdjustments;
  return ns / 1000000;
}


static const unsigned int ESZ = 256; /* 16384 bits of entropy */
static TimeHires eventCount[ ESZ ];
static unsigned int eventConsume, eventProduce;

void
Time::initHiresSeed( const byte *entropy,  unsigned int len )
{
  unsigned int i, j, k;
  TimeHires e;
  bool onePass, allTouched;

  i = 0;
  j = 0;
  e = 0;
  onePass = false;
  allTouched = false;
  eventConsume = entropy[ j ] % ESZ; 
  j = ( j + 1 ) % len;
  eventProduce = entropy[ j ] % ESZ;
  j = ( j + 1 ) % len;
  while ( ! onePass || ! allTouched ) {
    for ( k = 0; k < sizeof( e ); k++ ) {
      e = ( e << 8 ) | (TimeHires) entropy[ j++ ];
      if ( j == len ) {
        j = 0;
        onePass = true;
      }
    }
    eventCount[ i ] ^= e;
    if ( ++i == ESZ ) {
      i = 0;
      allTouched = true;
    }
  }
}

/* random seed entropy */
TimeHires
Time::getHiresSeed( void )
{
  TimeHires t = Time::getHiresTime(), h = 0;
  unsigned int j = eventConsume;
  eventConsume = ( eventConsume + 63 ) % ESZ;
  for ( unsigned int i = 0; i < 63; i++ ) {
    if ( ( t & ( (TimeHires) 1U << i ) ) == 0 )
      h += eventCount[ j ];
    else
      h ^= eventCount[ j ];
    if ( ++j == ESZ )
      j = 0;
  }
  return h;
}


TimeHires
Time::getHiresTime( double *perMSec )
{
  if ( perMSec != NULL ) {
    *perMSec = td.cyclesPerMillisec;
    if ( *perMSec == 0 ) {
      for (;;) {
        Time::currentTimeMillisecs();
        *perMSec = td.cyclesPerMillisec;
        if ( *perMSec != 0 ) /* wait for cycles to be computed */
          break;
        Time::sleepMillisecs( (TimeMSecs) 1 );
      }
    }
  }
  TimeHires t = (*hiresTime)();
  eventCount[ eventProduce ] += t;
  eventProduce = ( eventProduce + 1 ) % ESZ;
  return t;
}


double
Time::getCyclesPerMSec( void )
{
  if ( td.cyclesPerMillisec != 0 )
    return td.cyclesPerMillisec;
  for (;;) {
    Time::currentTimeMillisecs();
    if ( td.cyclesPerMillisec != 0 )
      return td.cyclesPerMillisec;
    Time::sleepMillisecs( (TimeMSecs) 1 );
  }
}


TimeNSecs
Time::getWallclockNanosecs( void )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  static const TimeMSecs DELTA_EPOCH_IN_MICROSECS = (TimeMSecs) 116444736U *
                                                    (TimeMSecs) 1000000000U;
  FILETIME filet;
  TimeMSecs tmpres;
  struct timeval tv;

  GetSystemTimeAsFileTime( &filet );
  tmpres = (TimeMSecs) filet.dwHighDateTime << 32;
  tmpres |= filet.dwLowDateTime;
  tmpres -= DELTA_EPOCH_IN_MICROSECS;
  tmpres /= 10;
  tv.tv_sec  = (long) ( tmpres / 1000000ULL );
  tv.tv_usec = (long) ( tmpres % 1000000ULL );

  return Time::microsecsToNanosecs( tv.tv_sec, tv.tv_usec );
#elif defined( HAS_CLOCK_REALTIME )
  struct timespec ts;

  ::clock_gettime( CLOCK_REALTIME, &ts );
  return Time::nanosecsToNanosecs( ts.tv_sec, ts.tv_nsec );
#else
  struct timeval tv;

  ::gettimeofday( &tv, NULL );
  return Time::microsecsToNanosecs( tv.tv_sec, tv.tv_usec );
#endif
}


TimeNSecs
Time::syncTimeToHires( TimeHires &n1,  TimeHires &n2 )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  static const TimeMSecs DELTA_EPOCH_IN_MICROSECS = (TimeMSecs) 116444736U *
                                                    (TimeMSecs) 1000000000U;
  FILETIME filet;
  TimeMSecs tmpres;
  struct timeval tv;

  do {
    n1 = (*hiresTime)();
    GetSystemTimeAsFileTime( &filet );
    n2 = (*hiresTime)();
  } while ( n2 < n1 );
  tmpres = (TimeMSecs) filet.dwHighDateTime << 32;
  tmpres |= filet.dwLowDateTime;
  tmpres -= DELTA_EPOCH_IN_MICROSECS;
  tmpres /= 10;
  tv.tv_sec  = (long) ( tmpres / 1000000ULL );
  tv.tv_usec = (long) ( tmpres % 1000000ULL );

  return Time::microsecsToNanosecs( tv.tv_sec, tv.tv_usec );
#elif defined( HAS_CLOCK_REALTIME )
  struct timespec ts;

  n1 = (*hiresTime)();
  ::clock_gettime( CLOCK_REALTIME, &ts );
  n2 = (*hiresTime)();
  return Time::nanosecsToNanosecs( ts.tv_sec, ts.tv_nsec );
#else
  struct timeval tv;

  n1 = (*hiresTime)();
  ::gettimeofday( &tv, NULL );
  n2 = (*hiresTime)();
  return Time::microsecsToNanosecs( tv.tv_sec, tv.tv_usec );
#endif
}


static TimeNSecs
updateTimeNanosecs( void )
{
  static AtomicUInt timeLock;
  TimeHires n1, n2;
  TimeNSecs ns, ns2;
  double    cs;
  llong     nsdiff;

  ns = Time::syncTimeToHires( n1, n2 );

  if ( timeLock.xchg( 1 ) == 0 ) {
    /* only one thread allowed in here */
    if ( td.baseTimestamp == 0 ) {
      td.zeroTimestamp      = ns;
      td.lastTimestamp      = ns;
      td.baseTimestamp      = ns;
      td.baseTimestampUSecs = ( ns + 500 ) / 1000;
      td.baseTimestampMSecs = ( ns + 500000 ) / 1000000;
      td.baseCyclestamp     = n1 + ( n2 - n1 ) / 2;
      /* if first time through, need a longer time sample */
      if ( ! (*getCyclesPerMillisec)( td.cyclesPerMillisec,
                                      td.cyclesPerNanosec ) ) {
        /* 1st sample isn't off by more than 1%, avg .5%, granularity is 1ms*/
        Time::sleepMillisecs( (TimeMSecs) 100 );

        ns = Time::syncTimeToHires( n1, n2 );

        /* compute the resolution of the hires timer */
        cs = (double) ( ( n1 + ( n2 - n1 ) / 2 ) - td.baseCyclestamp ) /
             (double) ( ns - td.baseTimestamp );
        td.cyclesPerNanosec  = cs;
        td.cyclesPerMillisec = cs * 1000000.0;
      }
    }
    else {
      /* if there is enough samples */
      if ( td.sampleShift >= 7 ) {
        /* check if time is off by more than .01 second */
        ns2 = td.baseTimestamp + (TimeNSecs)
              ( (double) ( n1 - td.baseCyclestamp ) / td.cyclesPerNanosec );
        nsdiff = (llong) ( ns - ns2 );
        /* if off by 10ms */
        if ( nsdiff >= 10000000 || nsdiff <= -10000000 ) {
          /* recompute base timestamp and cyclestamp */
          if ( td2.baseTimestamp == 0 ) {
            td2.lastTimestamp      = ns;
            td2.baseTimestamp      = ns;
            td2.baseTimestampUSecs = ( ns + 500 ) / 1000;
            td2.baseTimestampMSecs = ( ns + 500000 ) / 1000000;
            td2.zeroTimestamp      = ns2;
            td2.baseCyclestamp     = n1 + ( n2 - n1 ) / 2;
            if ( hasBuiltinFreq )
              (*getCyclesPerMillisec)( td2.cyclesPerMillisec,
                                       td2.cyclesPerNanosec );
          }
          /* wait for 1ms before computing cycles */
          else if ( ns >= td2.lastTimestamp + (TimeNSecs) 1000000 ) {
            /* remember how much time since the old baseTimestamp for rusage */
            td2.adjustmentNS  = (llong) td2.zeroTimestamp -
                                (llong) td.baseTimestamp;
            timeAdjustments  += td2.adjustmentNS;
            td2.lastTimestamp = ns;
            if ( td2.cyclesPerMillisec == 0 ) {
              cs = (double) ( ( n1 + ( n2 - n1 ) / 2 ) - td2.baseCyclestamp ) /
                   (double) ( ns - td2.baseTimestamp );
              td2.cyclesPerNanosec  = cs;
              td2.cyclesPerMillisec = cs * 1000000.0;
            }
            ::memcpy( &tdsave[ tdsaveCount ], &td, sizeof( td ) );
            tdsaveCount = ( tdsaveCount + 1 ) % ( sizeof( tdsave ) / sizeof( tdsave[ 0 ] ) );
            ::memcpy( &td, &td2, sizeof( td ) );
            td2.baseTimestamp     = 0;
            td2.lastTimestamp     = 0;
            td2.zeroTimestamp     = 0;
            td2.cyclesPerMillisec = 0;

            /* if off by more than a second, log it */
            if ( nsdiff >= 1000000000 || nsdiff <= -1000000000 ) {
              char buf[ 80 ], buf2[ 80 ];
              logNormal( LNORMAL, NULL, "Detected shift in system clock: "
                                        "%s -> %s (adjust=%.3fms)",
                Time::timestamp( ns2, 3, buf, sizeof( buf ) ),
                Time::timestamp( ns, 3, buf2, sizeof( buf2 ) ),
                (double) nsdiff / 1000000.0 );
            }
            goto do_unlock; /* return new time */
          }
          ns = ns2; /* return old time */
          goto do_unlock;
        }
      }
      /* need at least 2ms to compute a sample */
      if ( Time::nanosecsToMillisecs( ns ) > 
           Time::nanosecsToMillisecs( td.lastTimestamp ) ) {
        td.lastTimestamp = ns;
        if ( ! hasBuiltinFreq ) {
          /* compute the resolution of the hires timer */
          cs = (double) ( ( n1 + ( n2 - n1 ) / 2 ) - td.baseCyclestamp ) /
               (double) ( ns - td.baseTimestamp );
          td.cyclesPerNanosec  = cs;
          td.cyclesPerMillisec = cs * 1000000.0;
        }
        if ( td.sampleShift < 8 )
          td.sampleShift++;
      }
    }
  do_unlock:;
    timeLock.xchg( 0 );
  }

  return ns;
}


TimeMSecs
Time::hiresToMillisecs( TimeHires htime )
{
  for (;;) {
    if ( td.cyclesPerMillisec != 0 &&
         ( td.currentSample++ & ( ( 1U << td.sampleShift ) - 1 ) ) != 0 ) {
      if ( htime == 0 )
        htime = (*hiresTime)();
      return td.baseTimestampMSecs + (TimeMSecs)
           ( (double) ( htime - td.baseCyclestamp ) / td.cyclesPerMillisec );
    }
    updateTimeNanosecs();
  }
}


TimeUSecs
Time::hiresToMicrosecs( TimeHires htime )
{
  for (;;) {
    if ( td.cyclesPerMillisec != 0 &&
         ( td.currentSample++ & ( ( 1U << td.sampleShift ) - 1 ) ) != 0 ) {
      if ( htime == 0 )
        htime = (*hiresTime)();
      return td.baseTimestampUSecs +
             (TimeUSecs) ( (double) ( htime - td.baseCyclestamp ) /
                         td.cyclesPerNanosec ) / 1000;
    }
    updateTimeNanosecs();
  }
}


TimeNSecs
Time::hiresToNanosecs( TimeHires htime )
{
  for (;;) {
    if ( td.cyclesPerMillisec != 0 &&
         ( td.currentSample++ & ( ( 1U << td.sampleShift ) - 1 ) ) != 0 ) {
      if ( htime == 0 )
        htime = (*hiresTime)();
      return td.baseTimestamp +
             (TimeNSecs) ( (double) ( htime - td.baseCyclestamp ) /
                         td.cyclesPerNanosec );
    }
    if ( htime == 0 )
      return updateTimeNanosecs();
    updateTimeNanosecs();
  }
}


#ifdef __linux
static bool
readRusageFromProc( const char *path,  unsigned int pgrp,  TimeMSecs &stime,
                    TimeMSecs &utime )
{
#if 0
  /* the proc stat entry after pid (%d), command (%s), state (%c) */
  static const char *proc[] = {
    /* 0 (%d) */ "ppid",         /* 1 (%d) */ "pgrp",   /* 2 (%d) */ "session",
    /* 3 (%d) */ "tty_nr",       /* 4 (%d) */ "tpgid",

    /* 5 (%lu) */ "flags",       /* 6 (%lu) */ "minflt",
    /* 7 (%lu) */ "cminflt",     /* 8 (%lu) */ "majflt",
    /* 9 (%lu) */ "cmajflt",     /* 10 (%lu) */ "utime",
    /* 11 (%lu) */ "stime",

    /* 12 (%ld) */ "cutime",     /* 13 (%ld) */ "cstime",
    /* 14 (%ld) */ "priority",   /* 15 (%ld) */ "nice",
    /* 16 (%ld) */ "0",          /* 17 (%ld) */ "itrealvalue",

    /* 18 (%lu) */ "starttime",  /* 19 (%lu) */ "vsize",
    /* 20 (%lu) */ "rss",        /* 21 (%lu) */ "rlim",

    /* 22 (%lu) */ "startcode",  /* 23 (%lu) */ "endcode",
    /* 24 (%lu) */ "startstack", /* 25 (%lu) */ "kstkesp",
    /* 26 (%lu) */ "kstkeip",    /* 27 (%lu) */ "signal",
    /* 28 (%lu) */ "blocked",    /* 29 (%lu) */ "sigignore",
    /* 30 (%lu) */ "sigcatch",   /* 31 (%lu) */ "wchan",
    /* 32 (%lu) */ "nswap",      /* 33 (%lu) */ "cnswap",

    /* 34 (%d) */ "exit_signal", /* 35 (%d) */ "processor"
  };
#endif
  int           d1[ 4 + 1 - 0 ];   /* ppid, pgrp, session, tty_nr, tpgid */
  unsigned long lu1[ 11 + 1 - 5 ]; /* flags, minflt, cminflt, majflt, ... */
#if 0
  long          ld[ 17 + 1 - 12 ]; /* the rest of the entries */
  unsigned long lu2[ 33 + 1 - 18 ];
  int           d2[ 35 + 1 - 34 ];
#endif
  static const unsigned int PGRP  = 1;      /* into d1[] */
  static const unsigned int STIME = 11 - 5; /* into lu1[] */
  static const unsigned int UTIME = 10 - 5; /* into lu1[] */

  File       * in;
  char         buf[ 2 * 1024 ];
  const char * end;
  unsigned int i,
               j,
               n;
  in = NULL;
  try {
    in = File::openFile( path, File::FILE_RDONLY );
    n  = in->read( buf, sizeof( buf ) - 1 );
    in->close();
    delete in;
    in = NULL;

    buf[ n ] = '\0';
    i = 0;
    for ( ; isdigit( buf[ i ] ); i++ )  /* pid (%d) */
      ;
    i++;

    for ( ; buf[ i ] != ')'; i++ )
      ;
    i++; i++;                          /* (command) (%s) */

    i++; i++;                          /* state (%c) */

    j = 0;
    /* test process group id */
    if ( pgrp != 0 ) {
      StrUtil::parseInt( &buf[ i ], &d1[ j ], &end ); /* ppid */
      i += end - &buf[ i ] + 1;
      j++;
      StrUtil::parseInt( &buf[ i ], &d1[ j ], &end ); /* pgrp */
      i += end - &buf[ i ] + 1;
      j++;
      if ( (unsigned int) d1[ PGRP ] != pgrp )
        return false;
    }

    for ( ; j < sizeof( d1 ) / sizeof( d1[ 0 ] ); j++ ) {
      StrUtil::parseInt( &buf[ i ], &d1[ j ], &end );
      i += end - &buf[ i ] + 1;
    }
    for ( j = 0; j < sizeof( lu1 ) /
                     sizeof( lu1[ 0 ] ); j++ ) {
      StrUtil::parseInt( &buf[ i ], &lu1[ j ], &end );
      i += end - &buf[ i ] + 1;
    }
    /* linux uses 'jiffies', 1/100th of a second */
    if ( lu1[ UTIME ] < 0x7fffffffU && lu1[ STIME ] < 0x7fffffffU ) {
      stime = (TimeMSecs) lu1[ STIME ] * (TimeMSecs) 10;
      utime = (TimeMSecs) lu1[ UTIME ] * (TimeMSecs) 10;
      return true;
    }
  } catch ( ... ) {
    /* probably some kind of secure linux */
    if ( in != NULL )
      delete in;
  }
  return false;
}

static bool
readProcessSizeFromProc( const char *path,  ullong &vsz,  ullong &rss )
{
/* Name:   cat
 * State:  R (running)
 * Tgid:   5588
 * Pid:    5588
 * PPid:   3104
 * TracerPid:      0
 * Uid:    109     109     109     109
 * Gid:    109     109     109     109
 * Utrace: 0
 * FDSize: 256
 * Groups: 109 501 
 * VmPeak:   100892 kB
 * VmSize:   100892 kB
 * VmLck:         0 kB
 * VmHWM:       484 kB
 * VmRSS:       484 kB
 * VmData:      180 kB
 * VmStk:       140 kB
 * VmExe:        44 kB
 * VmLib:      1612 kB
 * VmPTE:        44 kB
 * VmSwap:        0 kB
 * Threads:        1
 * SigQ:   1/193103
 * SigPnd: 0000000000000000
 * ShdPnd: 0000000000000000
 * SigBlk: 0000000000000000
 * SigIgn: 0000000000000000
 * SigCgt: 0000000000000000
 * CapInh: 0000000000000000
 * CapPrm: 0000000000000000
 * CapEff: 0000000000000000
 * CapBnd: ffffffffffffffff
 * Cpus_allowed:   ffff
 * Cpus_allowed_list:      0-15
 * Mems_allowed:   00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000001
 * Mems_allowed_list:      0
 * voluntary_ctxt_switches:        1
 * nonvoluntary_ctxt_switches:     2
 */
  File       * in = NULL;
  char         b[ 4 * 1024 ];
  unsigned int i, n;

  try {
    in = File::openFile( path, File::FILE_RDONLY );
    n  = in->read( b, sizeof( b ) - 1 );
    in->close();
    delete in;
    in = NULL;

    b[ n ] = '\0';
    
    for ( i = 0; i < n; ) {
      static const char VmRSS[]  = "VmRSS:",
                        VmSize[] = "VmSize:";
      const char *p = ::strchr( &b[ i ], '\n' );
      switch ( b[ i + 2 ] ) {
        case 'R':
          if ( rss == 0 ) {
            if ( ::strncmp( &b[ i ], VmRSS, sizeof( VmRSS ) - 1 ) == 0 ) {
              rss = ::strtoll( &b[ i + sizeof( VmRSS ) ], NULL, 10 ) * 1024ULL;
              //logMinor( LMINOR, "Parse %.*s = %qu", (int) ( p - &b[ i ] ), &b[ i ], rss );
            }
          }
          break;
        case 'S':
          if ( vsz == 0 ) {
            if ( ::strncmp( &b[ i ], VmSize, sizeof( VmSize ) - 1 ) == 0 ) {
              vsz = ::strtoll( &b[ i + sizeof( VmSize ) ], NULL, 10 ) * 1024ULL;
              //logMinor( LMINOR, "Parse %.*s = %qu", (int) ( p - &b[ i ] ), &b[ i ], vsz );
            }
          }
          break;
      }
      if ( p == NULL )
        break;
      i += (unsigned int) ( &p[ 1 ] - &b[ i ] );
    }

    return true;
  } catch ( ... ) {
    /* probably some kind of secure linux */
    if ( in != NULL )
      delete in;
  }
  return false;
}
#endif

#if defined( __sparcv9 ) || \
    ( defined( __sun__ ) && defined( __amd64__ ) ) || \
    ( defined( __sparc ) && ! defined( _FILE_OFFSET_BITS ) )
/* can't use procfs when 32 bits and _FILE_OFFSET_BITS=64 */
static bool
readRusageFromProc2( const char *path,  TimeMSecs &stime,  TimeMSecs &utime )
{
  File        * in;
  prusage_t     u;
  unsigned int  n;

  in = NULL;
  try {
    in = File::openFile( path, File::FILE_RDONLY );
    n  = in->read( (byte *) &u, sizeof( u ) );
    in->close();
    delete in;
    in = NULL;

    if ( n == sizeof( u ) ) {
      stime = (TimeMSecs) u.pr_stime.tv_sec * 1000 +
              ( (TimeMSecs) u.pr_stime.tv_nsec + 500000 ) / ( 1000 * 1000 );
      utime = (TimeMSecs) u.pr_utime.tv_sec * 1000 +
              ( (TimeMSecs) u.pr_utime.tv_nsec + 500000 ) / ( 1000 * 1000 );
      return true;
    }
  } catch ( ... ) {
    if ( in != NULL )
      delete in;
  }
  return false;
}
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
struct RusageThrListRec : public Thread::ListRec {
  TimeMSecs    * taskSysTime,
               * taskUserTime;
  unsigned int * taskId,
                 maxTid,
                 cnt;

  RusageThrListRec( TimeMSecs *tst,  TimeMSecs *tut,  unsigned int *tid,
                    unsigned int maxid )
      : taskSysTime( tst ), taskUserTime( tut ), taskId( tid ),
        maxTid( maxid ), cnt( 0 ) {};

  virtual bool onThread( Thread &thr ) {
    HANDLE   h;
    FILETIME ctime, etime, utime, ktime;
    unsigned int thrId = thr.getThreadId();

    if ( this->taskId != NULL &&
         ( this->cnt < this->maxTid || this->maxTid == 0 ) )
      this->taskId[ this->cnt ] = thrId;
    if ( this->taskSysTime != NULL || this->taskUserTime != NULL ) {
      thr.getThreadHandle( &h );

      if ( ::GetThreadTimes( h, &ctime, &etime, &ktime, &utime ) ) {
        if ( this->taskSysTime != NULL &&
             ( this->cnt < this->maxTid || this->maxTid == 0 ) ) {
          ::memcpy( &this->taskSysTime[ this->cnt ], &ktime,
                    sizeof( this->taskSysTime[ this->cnt ] ) );
          this->taskSysTime[ this->cnt ] += 5000;
          this->taskSysTime[ this->cnt ] /= 10000;
        }
        if ( this->taskUserTime != NULL &&
             ( this->cnt < this->maxTid || this->maxTid == 0 ) ) {
          ::memcpy( &this->taskUserTime[ this->cnt ], &utime,
                    sizeof( this->taskUserTime[ this->cnt ] ) );
          this->taskUserTime[ this->cnt ] += 5000;
          this->taskUserTime[ this->cnt ] /= 10000;
        }
      }
    }
    this->cnt++;
    return true;
  };
};
#endif


unsigned int
Time::getRusage( TimeMSecs &sysTime,  TimeMSecs &userTime,
                 TimeMSecs &totalTime,  TimeMSecs *taskSysTime,
                 TimeMSecs *taskUserTime,  unsigned int *taskId,
                 unsigned int maxTaskCnt )
{
  ullong rss, vsz;
  return Time::getRusage2( sysTime, userTime, totalTime, rss, vsz,
                           taskSysTime, taskUserTime, taskId, maxTaskCnt );
}


unsigned int
Time::getRusage2( TimeMSecs &sysTime,  TimeMSecs &userTime,
                  TimeMSecs &totalTime,  ullong &rss,  ullong &vsz,
                  TimeMSecs *taskSysTime,  TimeMSecs *taskUserTime,
                  unsigned int *taskId,  unsigned int maxTaskCnt )
{
  unsigned int cnt;

  /* if no usage defined */
  sysTime   = 0;
  userTime  = 0;
  rss       = 0;
  vsz       = 0;
  totalTime = Time::uptime();
  cnt       = 0;

#if ! defined( __linux ) && ! defined( _WIN32 ) && ! defined( _WIN64 )
  struct rusage usage;
  unsigned int  tries = 0;
  ::memset( &usage, 0, sizeof( usage ) );
bad_rusage:; /* solaris has bad rusage times every once in a while */
  if ( ::getrusage( RUSAGE_SELF, &usage ) == 0 ) {
    sysTime  =
      (TimeMSecs) (unsigned long) usage.ru_stime.tv_sec * (TimeMSecs) 1000 +
      ( ( (TimeMSecs) (unsigned long) usage.ru_stime.tv_usec + 500 ) /
        (TimeMSecs) 1000 );
    userTime =
      (TimeMSecs) (unsigned long) usage.ru_utime.tv_sec * (TimeMSecs) 1000 +
      ( ( (TimeMSecs) (unsigned long) usage.ru_utime.tv_usec + 500 ) /
        (TimeMSecs) 1000 );
    if ( ( sysTime + userTime ) / 32 > totalTime ) { /* sanity check */
      if ( tries++ > 0 )
        return 0;
      goto bad_rusage;
    }
    if ( taskSysTime != NULL || taskUserTime != NULL || taskId != NULL ) {
#if defined( __sparcv9 ) || \
    ( defined( __sun__ ) && defined( __amd64__ ) ) || \
    ( defined( __sparc ) && ! defined( _FILE_OFFSET_BITS ) )
      static const char PROC[]     = "/proc";
      static const char LWP[]      = "/lwp";
      static const char LWPUSAGE[] = "/lwpusage";
      TimeMSecs    stime,
                   utime;
      Dir        * dir;
      char       * ptr,
                   path[ 128 ],
                   path2[ 128 ],
                   path3[ 128 ];
      unsigned int pid,
                   len;
      
      ::strcpy( path, PROC );
      path[ sizeof( PROC ) - 1 ] = '/';
      pid = (unsigned int) ::getpid();
      StrUtil::intToString( pid, &path[ sizeof( PROC ) ],
                            sizeof( path ) - sizeof( PROC ) - 1, U_DECIMAL,
                            false, &ptr );
      ::strcpy( ptr, LWP );

      if ( Dir::dirExists( path ) ) {
        dir = NULL;
        try {
          dir = Dir::openDir( path );
          ::strcpy( path3, path );
          ::strcat( path3, "/" );
          len = ::strlen( path3 );
          while ( dir->read( path2, sizeof( path2 ) ) ) {
            if ( isdigit( path2[ 0 ] ) ) {
              ::strcpy( &path3[ len ], path2 );
              /* /proc/123/lwp/1/lwpusage */
              ::strcat( &path3[ len ], LWPUSAGE );

              if ( ::readRusageFromProc2( path3, stime, utime ) ) {
                if ( taskSysTime != NULL &&
                     ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
                  taskSysTime[ cnt ] = stime;
                if ( taskUserTime != NULL &&
                     ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
                  taskUserTime[ cnt ] = utime;
                if ( taskId != NULL &&
                     ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
                  taskId[ cnt ] = atoi( path2 );
                cnt++;
              }
            }
          }
          dir->close();
          delete dir;
        } catch ( ... ) {
          if ( dir != NULL )
            delete dir;
          return 0;
        }
      }
#else
      if ( taskSysTime != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
        taskSysTime[ 0 ] = sysTime;
      if ( taskUserTime != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
        taskUserTime[ 0 ] = userTime;
      if ( taskId != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
        taskId [ 0 ] = (unsigned int) ::getpid();
      cnt = 1;
#endif
    }
    else {
      cnt = 1;
    }
    return cnt;
  }
#elif defined( _WIN32 ) || defined( _WIN64 )
  FILETIME ctime, etime, utime, ktime;
  HANDLE   h;

  h = ::GetCurrentProcess();
  if ( ::GetProcessTimes( h, &ctime, &etime, &ktime, &utime ) ) {
    ::memcpy( &sysTime, &ktime, sizeof( sysTime ) );
    ::memcpy( &userTime, &utime, sizeof( userTime ) );
    sysTime += 5000;
    sysTime /= 10000;
    userTime += 5000;
    userTime /= 10000;
  }
  if ( taskSysTime != NULL || taskUserTime != NULL || taskId != NULL ) {
    RusageThrListRec listRec( taskSysTime, taskUserTime, taskId, maxTaskCnt );
    Thread::recurseAll( listRec );
    cnt = listRec.cnt;
  }
  else {
    cnt = 1;
  }
  return cnt;

#elif defined( __linux ) /* linux, read /proc to get thread cpu usage */
  static const char PROC[] = "/proc";
  static const char STAT[] = "/stat";
  static const char TASK[] = "/task";
  static const char STATUS[] = "/status";
  TimeMSecs      stime,
                 utime;
  Dir          * dir;
  char         * ptr,
                 path[ 128 ],
                 path2[ 128 ],
                 path3[ 128 ];
  unsigned int   pid,
                 len;

  ::strcpy( path, PROC );
  path[ sizeof( PROC ) - 1 ] = '/';
  pid = (unsigned int) ::getpid();
  StrUtil::intToString( pid, &path[ sizeof( PROC ) ],
                        sizeof( path ) - sizeof( PROC ) - 1, U_DECIMAL, false,
                        &ptr );

  /* linux 2.6 puts every thread under /proc/123/task/thread-pid/stat */
  ::strcpy( ptr, STATUS );
  ::readProcessSizeFromProc( path, vsz, rss );

  ::strcpy( ptr, TASK );
  if ( Dir::dirExists( path ) ) {
    dir = NULL;
    try {
      dir = Dir::openDir( path );
      ::strcpy( path3, path );
      ::strcat( path3, "/" );
      len = ::strlen( path3 );
      while ( dir->read( path2, sizeof( path2 ) ) ) {
        if ( isdigit( path2[ 0 ] ) ) {
          ::strcpy( &path3[ len ], path2 );
          ::strcat( &path3[ len ], STAT ); /* /proc/123/task/124/stat */

          if ( ::readRusageFromProc( path3, 0, stime, utime ) ) {
            sysTime  += stime;
            userTime += utime;
            if ( taskSysTime != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ))
              taskSysTime[ cnt ] = stime;
            if ( taskUserTime != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0))
              taskUserTime[ cnt ] = utime;
            if ( taskId != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
              taskId[ cnt ] = atoi( path2 );
            cnt++;
          }
        }
      }
      dir->close();
      delete dir;
    } catch ( ... ) {
      if ( dir != NULL )
        delete dir;
      return 0;
    }
  }
  /* linux 2.4 puts threads in /proc/.thread-pid, in the same dir as parent */
  else {
    ::strcpy( ptr, STAT );
    if ( ::readRusageFromProc( path, 0, sysTime, userTime ) ) {
      if ( taskSysTime != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
        taskSysTime[ cnt ] = sysTime;
      if ( taskUserTime != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
        taskUserTime[ cnt ] = userTime;
      if ( taskId != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
        taskId[ cnt ] = pid;
      cnt++;
    }
    dir = NULL;
    try {
      dir = Dir::openDir( PROC );
      ::strcpy( path3, PROC );
      path3[ sizeof( PROC ) - 1 ] = '/';
      while ( dir->read( path2, sizeof( path2 ) ) ) {
        if ( path2[ 0 ] == '.' && isdigit( path2[ 1 ] ) ) {
          ::strcpy( &path3[ sizeof( PROC ) ], path2 );
          ::strcat( &path3[ sizeof( PROC ) + 2 ], STAT ); /* /proc/.124/stat */
          if ( ::readRusageFromProc( path3, pid, stime, utime ) ) {
            sysTime  += stime;
            userTime += utime;
            if ( taskSysTime != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ))
              taskSysTime[ cnt ] = stime;
            if ( taskUserTime != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0))
              taskUserTime[ cnt ] = utime;
            if ( taskId != NULL && ( cnt < maxTaskCnt || maxTaskCnt == 0 ) )
              taskId[ cnt ] = atoi( &path2[ 1 ] );
            cnt++;
          }
        }
      }
      dir->close();
      delete dir;
    } catch ( ... ) {
      if ( dir != NULL )
        delete dir;
    }
  }
#endif
  return cnt;
}


void
Time::sleepMillisecs( TimeMSecs mSecs )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  ::Sleep( (DWORD) mSecs );
#else
  struct timeval time;

  time.tv_sec  = (time_t) (unsigned long) ( mSecs / (TimeMSecs) 1000U );
  time.tv_usec = (time_t) (unsigned long) ( mSecs % (TimeMSecs) 1000U ) * 1000U;
  ::select( 0, NULL, NULL, NULL, &time );
#endif
}


void
Time::sleepNanosecs( TimeNSecs nanoSecs )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  ::Sleep( (DWORD) Time::nanosecsToMillisecs( nanoSecs +
                   Time::millisecsToNanosecs( 0, 1 ) - 1 ) );
#else
  struct timespec in;
  in.tv_sec  = 0;
  in.tv_nsec = nanoSecs;
  ::nanosleep( &in, NULL );
#endif
}


void
Time::sleepMS( double ms )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  ::Sleep( (DWORD) ms );
#else
  ullong ns = (ullong) ( ms * 1000000.0 );
  struct timespec in;
  in.tv_sec  = ns / 1000000000;
  in.tv_nsec = ns % 1000000000;
  ::nanosleep( &in, NULL );
#endif
}


int
Time::utcOffsetSecs()
{
  struct tm     gmt;
  struct tm     local;
  time_t        now;
  time_t        gmtTime;
  time_t        localTime;

  const int     SECS_PER_YEAR   = 365 * 24 * 60 * 60;
  const int     SECS_PER_DAY    = 24 * 60 * 60;
  const int     SECS_PER_HOUR   = 60 * 60;
  const int     SECS_PER_MIN    = 60;

  now = ::time( NULL );
  ::localtime_r( &now, &local );
  ::gmtime_r( &now, &gmt );
  
  gmtTime = ( ( gmt.tm_year - 1970 ) * SECS_PER_YEAR ) + 
    ( gmt.tm_yday * SECS_PER_DAY ) +
    ( gmt.tm_hour * SECS_PER_HOUR ) + 
    ( gmt.tm_min * SECS_PER_MIN ) + gmt.tm_sec;

  localTime = ( ( local.tm_year - 1970 ) * SECS_PER_YEAR ) + 
    ( local.tm_yday * SECS_PER_DAY ) +
    ( local.tm_hour * SECS_PER_HOUR ) + 
    ( local.tm_min * SECS_PER_MIN ) + local.tm_sec;

  return localTime - gmtTime;
}


void
TimeRotate::adjustWeekday( TimeMSecs &rotTime,  const char *timeSpec )
{
  const char * ptr;
  char         buf[ 80 ];
  unsigned int i;
  int          day,
               cur;
  /* presumes timespec starts with Mon, Tue, etc */
  i = 0;
  for ( ptr = timeSpec; *ptr != '\0' && *ptr != ' ' && *ptr != ','; ptr++ ) {
    if ( i < sizeof( buf ) - 1 )
      buf[ i++ ] = *ptr;
  }
  buf[ i ] = '\0';

  day = Time::getDayOfWeek( buf );
  if ( day == -1 )
    return;
  cur = Time::getDayOfWeek( NULL );
  if ( cur == -1 )
    return;

  if ( cur > day )
    rotTime -= (TimeMSecs) MSECS_IN_DAY * (TimeMSecs) ( cur - day );
  else if ( day > cur )
    rotTime += (TimeMSecs) MSECS_IN_DAY * (TimeMSecs) ( day - cur );
}


static bool
spaceMatches( const char *s1,  const char *s2 )
{
  while ( *s1 == ' ' )
    s1++;
  while ( *s2 == ' ' )
    s2++;
  for (;;) {
    if ( *s1 == '\0' && *s2 == '\0' )
      return true;
    if ( *s1 == '\0' || *s2 == '\0' )
      return false;
    while ( *s1 != '\0' && *s1 != ' ' )
      s1++;
    while ( *s1 == ' ' )
      s1++;
    while ( *s2 != '\0' && *s2 != ' ' )
      s2++;
    while ( *s2 == ' ' )
      s2++;
  }
}


TimeRotate::DayOrWeek
TimeRotate::parseTimeSpec( const char *timeSpec,  TimeMSecs &ms )
{
  /* old gcc warns about char used as index in freq[] */
  static const byte x_m='m',x_M='M',x_a='a',x_p='p',x_A='A',x_P='P',x_spc=' ',
                    x_slash='/',x_dash='-',x_period='.',x_comma=',',x_colon=':';
  struct FmtClr { const char *fmt;  int clr; };
  byte freq[ 256 ];
  unsigned int ampm, hasalpha, hasdigit, hasyear;

  ::memset( freq, 0, sizeof( freq ) );
  hasyear  = 0;
  hasdigit = 0;
  hasalpha = 0;
  /* accum frequency, check for alphas, digits, year */
  for ( const char *p = timeSpec; *p != '\0'; p++ ) {
    freq[ (unsigned char) *p ]++;
    if ( p[ 0 ] >= '0' && p[ 0 ] <= '9' ) {
      hasdigit = 1;
      if ( p[ 1 ] >= '0' && p[ 1 ] <= '9' &&
           p[ 2 ] >= '0' && p[ 2 ] <= '9' &&
           p[ 3 ] >= '0' && p[ 3 ] <= '9' &&
           ( p[ 4 ] < '0' || p[ 4 ] > '9' ) )
        hasyear = 1;
    }
    else if ( ( p[ 0 ] >= 'a' && p[ 0 ] <= 'z' ) ||
              ( p[ 0 ] >= 'A' && p[ 0 ] <= 'Z' ) ) {
      hasalpha = 1;
    }
  }
  /* check for am/pm */
  ampm = 0;
  if ( hasalpha &&
       ( ( freq[ x_m ] | freq[ x_M ] ) != 0 ) && 
       ( ( freq[ x_a ] | freq[ x_p ] | freq[ x_A ] | freq[ x_P ] ) != 0 ) ) {
    char last = 0;
    for ( const char *p = timeSpec; *p != '\0'; p++ ) {
      if ( ( p[ 0 ] == 'a' && ::strncmp( p, "am", 2 ) == 0 ) ||
           ( p[ 0 ] == 'A' && ::strncmp( p, "AM", 2 ) == 0 ) ||
           ( p[ 0 ] == 'p' && ::strncmp( p, "pm", 2 ) == 0 ) ||
           ( p[ 0 ] == 'P' && ::strncmp( p, "PM", 2 ) == 0 ) ) {
        if ( last == ' ' || ( last >= '0' && last <= '9' ) ) {
          ampm = 1;
          break;
        }
      }
      last = *p;
    }
  }

  /* 2013/10/10 10:10:10 */
  byte cnt = freq[ x_slash ] + freq[ x_dash ] + freq[ x_period ];
  if ( cnt > 0 ) {
    char tmpFmt[ 24 ];
    int clr;
    if ( freq[ x_spc ] > 0 ) {
      static const FmtClr fmts[ 2 ][ 3 ][ 2 ] = {
    { { { "%Y/%m/%d %H", Time::CLR_MIN }, { "%m/%d %H", Time::CLR_MIN } },
      { { "%Y/%m/%d %H:%M", Time::CLR_SEC }, { "%m/%d %H:%M", Time::CLR_SEC } },
      { { "%Y/%m/%d %H:%M:%S", Time::CLR_NONE },
          { "%m/%d %H:%M:%S", Time::CLR_NONE } } },
    { { { "%Y/%m/%d %I %p", Time::CLR_MIN }, { "%m/%d %I %p", Time::CLR_MIN } },
      { { "%Y/%m/%d %I:%M %p", Time::CLR_SEC },
        { "%m/%d %I:%M %p", Time::CLR_SEC } },
      { { "%Y/%m/%d %I:%M:%S %p", Time::CLR_NONE },
        { "%m/%d %I:%M:%S %p", Time::CLR_NONE } } } };
      const FmtClr &fc = fmts [ ampm ]
                              [ freq[ x_colon ] <= 2 ? freq[ x_colon ] : 0 ]
                              [ cnt == 2 ? 0 : 1 ];
      ::strcpy( tmpFmt, fc.fmt );
      clr = fc.clr;
    }
    else {
      static const FmtClr fmts[ 2 ] = {
        { "%Y/%m/%d", Time::CLR_HOUR }, { "%m/%d", Time::CLR_HOUR } };
      const FmtClr &fc = fmts [ cnt == 2 ? 0 : 1 ];
      ::strcpy( tmpFmt, fc.fmt );
      clr = fc.clr;
    }
    char *p = tmpFmt;
    if ( freq[ x_dash ] == cnt ) {
      while ( (p = ::strchr( p, '/' )) != NULL )
        *p = '-';
    }
    else if ( freq[ x_period ] == cnt ) {
      while ( (p = ::strchr( p, '/' )) != NULL )
        *p = '.';
    }
    ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, tmpFmt, timeSpec, clr );
    if ( ms != 0 )
      return ROTATE_DAILY;
  }

  /* am, pm */
  if ( ampm ) {
    /* Mon, 9:00 pm or Mon 9:00 pm */
    static const FmtClr fmts[ 3 ][ 2 ] = {
      { { "%a %I %p", Time::CLR_MIN }, { "%a, %I %p", Time::CLR_MIN } },
      { { "%a %I:%M %p", Time::CLR_SEC }, { "%a, %I:%M %p", Time::CLR_SEC } },
      { { "%a %I:%M:%S %p", Time::CLR_NONE },
        { "%a, %I:%M:%S %p", Time::CLR_NONE } } };
    const FmtClr &fc = fmts [ freq[ x_colon ] <= 2 ? freq[ x_colon ] : 0 ]
                            [ freq[ x_comma ] ? 1 : 0 ];
    ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, fc.fmt, timeSpec, fc.clr );
    if ( ms != 0 ) {
      /* strptime() doesn't adjust weekday time */
      TimeRotate::adjustWeekday( ms, timeSpec );
      return ROTATE_WEEKLY;
    }
    static const FmtClr fmts2[ 3 ] = {
      { "%I %p", Time::CLR_MIN },
      { "%I:%M %p", Time::CLR_SEC },
      { "%I:%M:%S %p", Time::CLR_NONE } };
    const FmtClr &fc2 = fmts2 [ freq[ x_colon ] <= 2 ? freq[ x_colon ] : 0 ];
    ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, fc2.fmt, timeSpec, fc2.clr );
    if ( ms != 0 )
      return ROTATE_DAILY;

    /* Nov 10 2010 10:10:10 pm */
    static const FmtClr fmts3[ 3 ][ 2 ] = {
      { { "%b %d %Y %I %p", Time::CLR_HOUR },
        { "%b %d, %Y %I %p", Time::CLR_HOUR } },
      { { "%b %d %Y %I:%M %p", Time::CLR_SEC },
        { "%b %d, %Y %I:%M %p", Time::CLR_SEC } },
      { { "%b %d %Y %I:%M:%S %p", Time::CLR_NONE },
        { "%b %d, %Y %I:%M:%S %p", Time::CLR_NONE } } };
    const FmtClr &fc3 = fmts3 [ freq[ x_colon ] <= 2 ? freq[ x_colon ] : 0 ]
                            [ freq[ x_comma ] ? 1 : 0 ];
    ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, fc3.fmt, timeSpec, fc3.clr );
    if ( ms != 0 )
      return ROTATE_DAILY;

    /* 10 Nov 2010 10:10:10 pm */
    static const FmtClr fmts4[ 3 ] = {
      { "%d %b %Y %I %p", Time::CLR_MIN },
      { "%d %b %Y %I:%M %p", Time::CLR_SEC },
      { "%d %b %Y %I:%M:%S %p", Time::CLR_NONE } };
    const FmtClr &fc4 = fmts4 [ freq[ x_colon ] <= 2 ? freq[ x_colon ] : 0 ];
    ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, fc4.fmt, timeSpec, fc4.clr );
    if ( ms != 0 )
      return ROTATE_DAILY;
  }

  /* Mon, 21:00 or Mon 21:00 */
  static const FmtClr fmts[ 3 ][ 2 ] = {
    { { "%a %H", Time::CLR_MIN }, { "%a, %H", Time::CLR_MIN } },
    { { "%a %H:%M", Time::CLR_SEC }, { "%a, %H:%M", Time::CLR_SEC } },
    { { "%a %H:%M:%S", Time::CLR_NONE }, { "%a, %H:%M:%S", Time::CLR_NONE } } };
  const FmtClr &fc = fmts [ freq[ x_colon ] <= 2 ? freq[ x_colon ] : 0 ]
                          [ freq[ x_comma ] ? 1 : 0 ];
  ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, fc.fmt, timeSpec, fc.clr );
  if ( ms != 0 ) {
    /* strptime() doesn't adjust weekday time */
    TimeRotate::adjustWeekday( ms, timeSpec );
    return ROTATE_WEEKLY;
  }

  /* Nov 10 or 10 Nov */
  if ( hasalpha ) {
    const char *p = timeSpec;
    while ( *p == ' ' )
      p++;
    /* check if number first, skip over spaces */
    if ( *p >= '0' && *p <= '9' ) {
      /* 10 Nov 2010 10:10:10 */
      static const FmtClr fmts[ 2 ][ 3 ] = {
      { { "%d %b", Time::CLR_HOUR },
        { "%d %b %H:%M", Time::CLR_SEC },
        { "%d %b %H:%M:%S", Time::CLR_NONE } },
      { { "%d %b %Y", Time::CLR_HOUR },
        { "%d %b %Y %H:%M", Time::CLR_SEC },
        { "%d %b %Y %H:%M:%S", Time::CLR_NONE } } };
      const FmtClr &fc = fmts [ hasyear ]
                              [ freq[ x_colon ] <= 2 ? freq[ x_colon ] : 0 ];
      if ( spaceMatches( fc.fmt, timeSpec ) ) {
        ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, fc.fmt, timeSpec, fc.clr);
        if ( ms != 0 )
          return ROTATE_DAILY;
      }
    }
    else {
      /* Nov 10 2010 10:10:10 */
      static const FmtClr fmts[ 2 ][ 3 ][ 2 ] = {
      { { { "%b %d", Time::CLR_HOUR }, { "%b %d,", Time::CLR_HOUR } },
        { { "%b %d %H:%M", Time::CLR_SEC },
          { "%b %d, %H:%M", Time::CLR_SEC } },
        { { "%b %d %H:%M:%S", Time::CLR_NONE },
          { "%b %d, %H:%M:%S", Time::CLR_NONE } } },
      { { { "%b %d %Y", Time::CLR_HOUR }, { "%b %d, %Y", Time::CLR_HOUR } },
        { { "%b %d %Y %H:%M", Time::CLR_SEC },
          { "%b %d, %Y %H:%M", Time::CLR_SEC } },
        { { "%b %d %Y %H:%M:%S", Time::CLR_NONE },
          { "%b %d, %Y %H:%M:%S", Time::CLR_NONE } } } };
      const FmtClr &fc = fmts [ hasyear ]
                              [ freq[ x_colon ] <= 2 ? freq[ x_colon ] : 0 ]
                              [ freq[ x_comma ] ? 1 : 0 ];
      if ( spaceMatches( fc.fmt, timeSpec ) ) {
        ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, fc.fmt, timeSpec, fc.clr);
        if ( ms != 0 )
          return ROTATE_DAILY;
      }
    }
  }

  /* Nov 2010 */
  if ( hasyear ) {
    static const FmtClr fmts[ 2 ] = {
      { "%Y", Time::CLR_MONTH }, { "%b %Y", Time::CLR_DAY } };
    const FmtClr &fc = fmts [ hasalpha ];
    ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, fc.fmt, timeSpec, fc.clr );
    if ( ms != 0 )
      return ROTATE_DAILY;
  }

  /* 22:00 */
  if ( hasdigit ) {
    const char *fmt = NULL;
    int clr = Time::CLR_NONE;
    switch ( freq[ x_colon ] ) {
      case 1:
        fmt = "%H:%M";
        clr = Time::CLR_SEC;
        break;
      case 2:
        fmt = "%H:%M:%S";
        break;
      default:
        fmt = "%H";
        clr = Time::CLR_MIN;
        break;
    }
    ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, fmt, timeSpec, clr );
    if ( ms != 0 )
      return ROTATE_DAILY;
  }

  /* Monday or March */
  if ( hasalpha ) {
    ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, "%a", timeSpec,
                             Time::CLR_HOUR );
    if ( ms != 0 ) {
      TimeRotate::adjustWeekday( ms, timeSpec );
      return ROTATE_WEEKLY;
    }
    ms = Time::strptime_clr( Time::TZ_LOCAL_TIME, "%b", timeSpec,
                             Time::CLR_DAY );
    if ( ms != 0 )
      return ROTATE_DAILY;
  }

  return ROTATE_UNSPECIFIED;
}


bool
TimeRotate::setRotateTime( const char *timeSpec,  DayOrWeek rotDorW,
                           TimeMSecs rotTime )
{
  TimeMSecs t;

  if ( timeSpec != NULL ) {
    TimeRotate::DayOrWeek rot = TimeRotate::parseTimeSpec( timeSpec, rotTime );
    if ( rotDorW == ROTATE_UNSPECIFIED )
      rotDorW = rot;

    if ( rotTime != 0 ) {
      t = Time::currentTimeMillisecs();
      /* adjust so time the last time log was rotated */
      if ( rotTime > t ) {
        if ( rotDorW == ROTATE_DAILY )
          rotTime -= (TimeMSecs) MSECS_IN_DAY;
        else if ( rotDorW == ROTATE_WEEKLY )
          rotTime -= (TimeMSecs) MSECS_IN_WEEK;
      }
    }
  }
  this->time      = rotTime;
  this->dayOrWeek = rotDorW;

  if ( rotTime == 0 && timeSpec != NULL )
    return false;
  return true;
}


bool
TimeRotate::setRotatePeriod( const char *periodSpec,  TimeMSecs rotatePeriod )
{
  double    d;
  TimeMSecs currTime;

  if ( periodSpec != NULL ) {
    try {
      StrUtil::parseFloat( periodSpec, &d, NULL, U_SECONDS );
    } catch ( ... ) {
      d = 0.0;
    }
    rotatePeriod = (TimeMSecs) ( d * 1000.0 );
  }
  this->period = rotatePeriod;

  if ( rotatePeriod == 0 && periodSpec != NULL )
    return false;

  currTime = Time::currentTimeMillisecs();
  if ( this->time == 0 )
    this->time = currTime;
  else if ( this->period != 0 ) {
    while ( this->time + this->period <= currTime )
      this->time += this->period;
  }
  return true;
}


bool
TimeRotate::checkRotate( void )
{
  TimeMSecs currTime,
            per;
  bool      doRotate = false;

  if ( this->time != 0 ) {
    if ( this->period != 0 ) {
      per = this->period;
    }
    else {
      if ( this->dayOrWeek == ROTATE_WEEKLY )
        per = MSECS_IN_WEEK;
      else if ( this->dayOrWeek == ROTATE_DAILY )
        per = MSECS_IN_DAY;
      else
        per = 0;
    }

    if ( per != 0 ) {
      currTime = Time::currentTimeMillisecs();
      /* lastTime check is to rotate from a previous run */
      if ( this->time > this->lastTime ||
           this->time + per <= currTime ) {
        doRotate = true;
        while ( this->time > currTime )
          this->time -= per;
        while ( this->time + per <= currTime )
          this->time += per;
      }
      this->lastTime = currTime;
    }
  }
  return doRotate;
}


TimeMSecs
TimeRotate::nextRotate( TimeMSecs *diffTime )
{
  TimeMSecs currTime,
            nextTime;

  if ( this->time == 0 ) {
    if ( diffTime != NULL )
      *diffTime = 0;
    nextTime = 0;
  }
  else {
    if ( this->period != 0 ) {
      nextTime = this->period;
    }
    else {
      if ( this->dayOrWeek == ROTATE_WEEKLY )
        nextTime = MSECS_IN_WEEK;
      else if ( this->dayOrWeek == ROTATE_DAILY )
        nextTime = MSECS_IN_DAY;
      else
        nextTime = 0;
    }

    if ( nextTime != 0 ) {
      currTime = Time::currentTimeMillisecs();
      /* lastTime check is to rotate from a previous run */
      if ( this->time > this->lastTime )
        nextTime = currTime;
      else {
        nextTime += this->time;
        if ( nextTime < currTime )
          nextTime = currTime;
      }
      if ( diffTime != NULL )
        *diffTime = nextTime - currTime;
    }
    else {
      if ( diffTime != NULL )
        *diffTime = 0;
    }
  }
  return nextTime;
}


TimeMSecs
TimeRotate::getInterval( void )
{
  if ( this->time == 0 )
    return 0;
  if ( this->period != 0 )
    return this->period;
  if ( this->dayOrWeek == ROTATE_WEEKLY )
    return MSECS_IN_WEEK;
  if ( this->dayOrWeek == ROTATE_DAILY )
    return MSECS_IN_DAY;
  return 0;
}


#ifdef __linux

static const unsigned long RTC_RATE_SET_MAX = 8192; /* highest available */
static const unsigned long RTC_RATE_SET_MIN = 2; /* highest available */

bool
NanoSleep::open( unsigned int hz,  const char *rtcName )
{
  unsigned long rate = RTC_RATE_SET_MIN;
  while ( rate < RTC_RATE_SET_MAX && rate < hz )
    rate *= 2;
  this->rateMSec = 1000.0 / (double) rate;
  if ( (this->fd = ::open( rtcName, O_RDONLY )) == -1 ) {
    int e = errno;
    logMinor( LMINOR, "open( %s, O_RDONLY ) failed, errno=%d/%s",
              rtcName, e, strerror( e ) );
    return false;
  }
  if ( ::ioctl( this->fd, RTC_IRQP_SET, rate ) == -1 ) {
    do {
      rate /= 2;
      if ( rate < RTC_RATE_SET_MIN ) {
        int e = errno;
        logMinor( LMINOR, "ioctl( %s, RTC_IRQP_SET, %ld ) failed, errno=%d/%s",
                  rtcName, rate, e, strerror( e ) );
        this->close();
        return false;
      }
    } while ( ::ioctl( this->fd, RTC_IRQP_SET, rate ) == -1 );
    logMinor( LMINOR, "rate=%ld, ioctl( %s, RTC_IRQP_SET, %ld ) succeeded",
              rate, rtcName, rate );
    this->rateMSec = 1000.0 / (double) rate;
  }
  if ( ::ioctl( this->fd, RTC_PIE_ON, 0 ) == -1 ) {
    int e = errno;
    logMinor( LMINOR, "ioctl( %s, RTC_PIE_ON, 0 ) failed, errno=%d/%s",
              rtcName, e, strerror( e ) );
    this->close();
    return false;
  }
  this->last = Time::getHiresTime();
  return true;
}
  
void
NanoSleep::close( void )
{
  if ( this->fd != -1 ) {
    ::close( this->fd );
    this->fd = -1;
  }
}

double
NanoSleep::sleep( double ms )
{
  if ( this->fd != -1 ) {
    if ( ms >= 0.0 ) {
      double        cpms;
      TimeHires     start = Time::getHiresTime( &cpms ),
                    delta = start;
      unsigned long data;

      delta += (TimeHires) ( ms * cpms );
      while ( this->last < delta ) {
        ::read( fd, &data, sizeof( data ) );
        this->last += (TimeHires) ( this->rateMSec * cpms );
      }
      this->last = Time::getHiresTime();
      return (double) ( this->last - start ) / cpms;
    }
  }
  return 0;
}

#else

bool
NanoSleep::open( unsigned int hz,  const char *rtcName )
{
  logMinor( LMINOR, "No hires time source available" );
  return false;
}

void
NanoSleep::close( void )
{
}

double
NanoSleep::sleep( double ms )
{
  return 0;
}

#endif
