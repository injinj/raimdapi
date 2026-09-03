/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <process.h>
#else
#ifndef  _XOPEN_SOURCE
  #define _XOPEN_SOURCE 500 /* for mutexattr & RECURSIVE style mutexes */
#endif
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sched.h> /* sched_yield() */
#endif
#include <string.h>

#include "base/thread.h"
#include "base/log.h"
#include "util/atomic.h"

#if defined( __sun__ )
 #include <sys/lwp.h>
#endif

using namespace rai;

Thread::SchedPolicy rai::Thread::defaultSched     = Thread::THREAD_SCHED_OTHER;
unsigned int        rai::Thread::defaultPrio      = 0,
                    rai::Thread::defaultStackSize = DEFAULT_STACK_SIZE;
static Thread     * rai_thrhd, * rai_thrtl;
static unsigned int threadSelfKey                 = Thread::NIL_KEY,
                    threadAllocKey                = Thread::NIL_KEY,
                    threadExitKey                 = Thread::NIL_KEY;

namespace rai {

static ThreadHandle *createThreadHandle( void );

struct ThrMBufPtr {
  protected:
    static const size_t MBUF_MARKER = 0x79900000; /* must be > MAX_ALLOC_SIZE */
    static const size_t MBUF_OFF_MASK = 0xfffff;  /* must be > MAX_ALLOC_SIZE */
    size_t size, offset;

  public:
    byte *bufPtr( void ) const {
      return &((byte *) this )[ sizeof( size_t ) * 2 ];
    }
    void setOffset( size_t pos ) { this->offset = pos | MBUF_MARKER; }
    bool isValid( size_t &val ) {
      if ( ( this->offset & ~MBUF_OFF_MASK ) == MBUF_MARKER ) {
        val = this->offset & MBUF_OFF_MASK;
        this->offset = 0;
        return true;
      }
      return false;
    }
    size_t sz( void ) const      { return this->size; }
    void setSize( size_t sz )    { this->size = sz; }
};

class ThrMBuf { /* local alloc mem */
  protected:
    /* mblock for pkt mem that usually alloced and freed in order */
    struct MBlock {
      AtomicUInt   off2;
      byte         pad1[ 3 * sizeof( unsigned int ) ];
      byte * bufPtr( size_t i ) {
        return &((byte *) this )[ i ];
      }
    };
    /* a block of mem doled out in pkt size chunks */
    MBlock * mem;
    unsigned int off, pad;
    static const size_t MAX_ALLOC_SIZE = 80 * 1024;

    void allocBlock( void ) {
      size_t allocsz = MAX_ALLOC_SIZE;
      this->mem = NULL;
      MALLOC( allocsz, &this->mem );
      this->mem->off2.init( sizeof( MBlock ) );
      this->mem->off2.add( 0 ); /* causes a pause + lock cache flush */
      this->off = sizeof( MBlock );
    }

    static void releaseBlock( MBlock *m ) {
      FREE( m );
    }

  public:
    SYS_OPS( ThrMBuf );
    ThrMBuf() : mem( 0 ), off( 0 ), pad( 0xf44ff11f ) {};
    ~ThrMBuf() {
      /* eat whats left of memory */
      if ( this->mem != NULL ) {
        MBlock & blk = *this->mem;
        if ( this->off < MAX_ALLOC_SIZE ) {
          unsigned int i = MAX_ALLOC_SIZE - this->off;
          if ( i + blk.off2.add( i ) == MAX_ALLOC_SIZE )
            ThrMBuf::releaseBlock( &blk );
        }
      }
    }
    /* alloc pkt sz */
    void alloc( size_t sz,  void *pkt,  size_t alignment );
    /* release ptr returned by alloc() */
    static void release( void *pkt ) {
      ThrMBufPtr * a = (ThrMBufPtr *) ( (byte *) pkt - sizeof( ThrMBufPtr ) );
      size_t off;
      if ( ! a->isValid( off ) ) {
        logError( LERROR, NULL, "Invalid localfree ptr (%p)", pkt );
        return;
      }
      if ( off != 0 ) {
        unsigned int sz = a->sz();
        MBlock * m = (MBlock *) ( (byte *) pkt - off );
        if ( sz + m->off2.add( sz ) == MAX_ALLOC_SIZE )
          ThrMBuf::releaseBlock( m );
      }
      else {
        FREE( a );
      }
    }
};


inline void
ThrMBuf::alloc( size_t sz,  void *pkt,  size_t alignment )
{
  /* align and add 2 size_t's */
  sz = ( sz + sizeof( size_t ) * 2 + alignment - 1 ) & ~( alignment - 1 );
  if ( sz > MAX_ALLOC_SIZE - sizeof( MBlock ) ) {
    ThrMBufPtr * a;
    MALLOC( sz, &a );
    a->setSize( sz );
    a->setOffset( 0 );
    *(void **) pkt = a->bufPtr();
    return;
  }

  if ( this->mem == NULL ) /* starts null, or after out-of-mem error */
    this->allocBlock();
  for (;;) {
    MBlock     & blk = *this->mem;
    unsigned int j   = this->off,
                 k   = ( ( j + sizeof( size_t ) * 2 ) & ( alignment - 1 ) );
    /* check that after adding 2 size_t's, aligned on alignment */
    if ( k != 0 ) {
      k = alignment - k;
      j = ( this->off += k );
      /* this presumes that MAX_ALLOC_SIZE will be on an alignment boundary */
      if ( k + blk.off2.add( k ) == MAX_ALLOC_SIZE ) {
        this->allocBlock();
        ThrMBuf::releaseBlock( &blk );
        continue;
      }
    }
    /* see if enough is available in current block */
    this->off += sz;
    if ( this->off <= MAX_ALLOC_SIZE ) {
      ThrMBufPtr * a = (ThrMBufPtr *) blk.bufPtr( j );
      a->setSize( sz );
      a->setOffset( a->bufPtr() - (byte *) &blk );
      *(void **) pkt = a->bufPtr();
      if ( this->off == MAX_ALLOC_SIZE )
        this->allocBlock();
      return;
    }
    /* get a new block */
    this->allocBlock();

    /* if some left over in old block, accumilate it in off2 */
    if ( j < MAX_ALLOC_SIZE ) {
      unsigned int i = MAX_ALLOC_SIZE - j;
      if ( i + blk.off2.add( i ) == MAX_ALLOC_SIZE )
        ThrMBuf::releaseBlock( &blk );
    }
  }
}

static inline ThrMBuf *
getThrMBuf( void )
{
  ThrMBuf * mbuf;
  if ( threadAllocKey == Thread::NIL_KEY ) {
    threadAllocKey = Thread::createSpecificKey();
    mbuf = NULL;
  }
  else {
    mbuf = (ThrMBuf *) Thread::getSpecific( threadAllocKey );
  }
  if ( mbuf == NULL ) {
    mbuf = NEW ThrMBuf();
    Thread::putSpecific( threadAllocKey, mbuf );
  }
  return mbuf;
}
} // namespace rai


ThrMBuf *
Thread::localThrMBuf( void )
{
  return getThrMBuf();
}


ThrMBuf *
Thread::createThrMBuf( void )
{
  return NEW ThrMBuf();
}


void
Thread::localAlloc( size_t sz,  void *ptr,  size_t alignment )
{
  getThrMBuf()->alloc( sz, ptr, alignment );
  //logMinor( LMINOR, "localAlloc %u (%p)", (unsigned int) sz, *(void **) ptr );
}


void
Thread::heapAlloc( size_t sz,  void *ptr,  size_t alignment )
{
  sz = ( sz + sizeof( size_t ) * 2 + alignment - 1 ) & ~( alignment - 1 );
  ThrMBufPtr * a;
  MALLOC( sz, &a );
  a->setSize( sz );
  a->setOffset( 0 );
  *(void **) ptr = a->bufPtr();
}


void
Thread::localAlloc( ThrMBuf *mbuf,  size_t sz,  void *ptr,
                    size_t alignment )
{
  mbuf->alloc( sz, ptr, alignment );
}


void
Thread::localFree( void *ptr )
{
  //logMinor( LMINOR, "localFree (%p) %s:%u", ptr, fn, ln );
  ThrMBuf::release( ptr );
}


unsigned int
Thread::getProcessId( void )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  return ::GetCurrentProcessId();
#else
  return (unsigned int) ::getpid();
#endif
}

#if defined( __SUNPRO_CC ) && defined( __linux )
  extern "C" long int syscall( long int __sysno, ...) __THROW;
#endif
#if defined( __APPLE__ )
  #include <sys/syscall.h>
  extern "C" int syscall( long int __sysno, ...);
#endif
unsigned int
Thread::getOSThreadId( void )
{
#if defined( _WIN32 ) || defined( _WIN64 )
  return ::GetCurrentThreadId();
#elif defined( __linux  )
  return (unsigned int) ::syscall( __NR_gettid );
#elif defined( __APPLE__ )
  return (unsigned int) ::syscall( SYS_gettid );
#elif defined( __sun__ )
  return ( unsigned int ) _lwp_self();
#else
  error
#endif
}

Thread *
Thread::self( void )
{
  if ( threadSelfKey == Thread::NIL_KEY )
    return NULL;
  return (Thread *) Thread::getSpecific( threadSelfKey );
}


void
Thread::addThrList( void )
{
  Thread * p = rai_thrhd;

  if ( this->inThrList ) {
    logError( LERROR, NULL, "Thread \"%s\" (%u) already in thrlist",
              this->name, this->tid );
    return;
  }
  for ( ; p != NULL && ::strcmp( p->name, this->name ) < 0; p = p->tnext )
    ;
  if ( p != NULL ) {
    this->tnext = p;
    this->tlast = p->tlast;
    p->tlast    = this;

    if ( this->tlast == NULL )
      rai_thrhd = this;
    else
      this->tlast->tnext = this;
  }
  else {
    if ( rai_thrhd == NULL ) {
      rai_thrhd = this;
      this->tlast = NULL;
    }
    else {
      rai_thrtl->tnext = this;
      this->tlast = rai_thrtl;
    }
    rai_thrtl = this;
    this->tnext = NULL;
  }
  this->inThrList = true;
}

void
Thread::removeThrList( void )
{
  if ( ! this->inThrList ) {
    logError( LERROR, NULL, "Thread \"%s\" (%u) not in thrlist",
              this->name, this->tid );
    return;
  }
  if ( this->tnext == NULL )
    rai_thrtl = this->tlast;
  else
    this->tnext->tlast = this->tlast;

  if ( this->tlast == NULL )
    rai_thrhd = this->tnext;
  else
    this->tlast->tnext = this->tnext;

  this->tnext = NULL;
  this->tlast = NULL;
  this->inThrList = false;
}

void
Thread::recurseThrList( ListRec &r )
{
  for ( Thread *t = rai_thrhd; t != NULL; t = t->tnext )
    if ( ! r.onThread( *t ) )
      return;
}

static void rai_lock_thr_list( void ); /* defined below */
static void rai_unlock_thr_list( void );
static void rai_create_exit_key( void );

static void
rai_create_thread_keys( void )
{
  if ( threadSelfKey == Thread::NIL_KEY )
    threadSelfKey = Thread::createSpecificKey();
  if ( threadAllocKey == Thread::NIL_KEY )
    threadAllocKey = Thread::createSpecificKey();
  if ( threadExitKey == Thread::NIL_KEY )
    ::rai_create_exit_key();
}


struct ExternalThread : public Thread {
  SYS_OPS( ExternalThread );
  ExternalThread() : Thread( NULL ) {}
  virtual ~ExternalThread() {}
  virtual void run( void ) {}
};


bool
Thread::createExternalThread( const char *name )
{
  rai_create_thread_keys();

  if ( Thread::self() == NULL ) {
    ExternalThread * thr;
    unsigned int tid;

    thr = NEW ExternalThread();
    thr->threadPtr = rai::createThreadHandle();
    Thread::putSpecific( threadSelfKey, thr );

    tid = Thread::getOSThreadId();
    thr->tid = tid;

    char b[ 16 ];
    unsigned int i = sizeof( b );
    for ( b[ --i ] = '\0'; tid > 0; tid /= 10 )
       b[ --i ] = tid % 10 + '0';
    thr->setName( name, &b[ i ] );

    ::rai_lock_thr_list();
    thr->addThrList();
    ::rai_unlock_thr_list();
    return true;
  }
  return false;
}


void
Thread::stopExternalThread( void )
{
  Thread  * thr;
  ThrMBuf * mbuf;

  if ( threadSelfKey == NIL_KEY )
    return;

  if ( (thr = Thread::self()) != NULL ) {
    ::rai_lock_thr_list();
    thr->removeThrList();
    ::rai_unlock_thr_list();
    if ( (mbuf = (ThrMBuf *) Thread::getSpecific( threadAllocKey )) != NULL )
      delete mbuf;
    delete thr;
  }
}

namespace rai {
  struct ThreadExitList {
    ThreadOnExit   & ex;
    ThreadExitList * next;
    bool             done;

    SYS_OPS( ThreadExitList );
    ThreadExitList( ThreadOnExit &x ) : ex( x ), next( 0 ), done( false ) {}

    static ThreadExitList *create( ThreadOnExit &x,  ThreadExitList *n ) {
      if ( n == NULL )
        return NEW ThreadExitList( x );
      for ( ThreadExitList *l = n; ; l = l->next ) {
        if ( &l->ex == &x )
          break;
        if ( l->next == NULL ) {
          l->next = NEW ThreadExitList( x );
          break;
        }
      }
      return n;
    }

    static void doExit( ThreadExitList *l ) {
      for ( ; l != NULL; l = l->next ) {
        if ( ! l->done ) {
          l->done = true;
          try {
            l->ex.onExit();
          } catch ( ... ) {
          }
        }
      }
    }

    static void release( ThreadExitList *l ) {
      while ( l != NULL ) {
        ThreadExitList *n = l->next;
        delete l;
        l = n;
      }
    }
  };
}


static void
rai_thread_on_exit_release( void )
{
  if ( threadExitKey != Thread::NIL_KEY ) {
    ThreadExitList *l = (ThreadExitList *) Thread::getSpecific( threadExitKey );
    if ( l != NULL ) {
      Thread::putSpecific( threadExitKey, NULL );
      ThreadExitList::doExit( l );
      ThreadExitList::release( l );
    }
  }
}


void
Thread::onExit( ThreadOnExit *onEx )
{
  if ( threadExitKey == Thread::NIL_KEY ) {
    ::rai_create_exit_key();
    Thread::putSpecific( threadExitKey, ThreadExitList::create( *onEx, NULL ) );
  }
  else {
    Thread::putSpecific( threadExitKey, ThreadExitList::create( *onEx, 
                   (ThreadExitList *) Thread::getSpecific( threadExitKey ) ) );
  }
}


void
rai_thread_cleanup( void *arg )
{
  ((Thread *) arg)->cleanup();
}


#if defined( _WIN32 ) || defined( _WIN64 )
/*
 * WIN32 thread stuff
 *
 * Tries to emulate posix mutex and condition variables
 */

static rai::Mutex *thrListLock;
static rai::Condition *thrListCond;

static void rai_lock_thr_list( void ) {
  if ( ! thrListLock )
    thrListLock = Mutex::create();
  thrListLock->lock();
}
static void rai_unlock_thr_list( void ) {
  thrListLock->unlock();
}
static void rai_create_exit_key( void ) {
  threadExitKey = Thread::createSpecificKey();
}

namespace rai {

typedef VOID ( WINAPI * InitCondFunc )( PCONDITION_VARIABLE );
typedef BOOL ( WINAPI * SleepCondFunc )( PCONDITION_VARIABLE, PCRITICAL_SECTION,
                                       DWORD );
typedef VOID ( WINAPI * WakeAllFunc )( PCONDITION_VARIABLE );
typedef VOID ( WINAPI * WakeFunc )( PCONDITION_VARIABLE );

static struct {
  InitCondFunc  init;
  SleepCondFunc sleep;
  WakeAllFunc   wakeAll;
  WakeFunc      wake;
} rai_wcond;

static VOID WINAPI RAI_NO_CONDITION_VARS( PCONDITION_VARIABLE x ) {}

static bool
rai_check_condition_variables( void )
{
  HMODULE h = GetModuleHandle( "kernel32" );

  rai_wcond.init    = (InitCondFunc) GetProcAddress( h,
                                               "InitializeConditionVariable" );
  rai_wcond.sleep   = (SleepCondFunc) GetProcAddress( h,
                                               "SleepConditionVariableCS" );
  rai_wcond.wakeAll = (WakeAllFunc) GetProcAddress( h,
                                               "WakeAllConditionVariable" );
  rai_wcond.wake    = (WakeFunc) GetProcAddress( h, "WakeConditionVariable" );

  if ( rai_wcond.init && rai_wcond.sleep && rai_wcond.wakeAll &&
       rai_wcond.wake )
    return true;
  rai_wcond.init = RAI_NO_CONDITION_VARS;
  return false;
}

static inline bool
rai_has_condition_vars_check( void ) {
  if ( rai_wcond.init == NULL )
    return rai_check_condition_variables();
  if ( rai_wcond.init != RAI_NO_CONDITION_VARS )
    return true;
  return false;
}

static inline bool
rai_has_condition_vars_use( void ) {
  return rai_wcond.init != RAI_NO_CONDITION_VARS;
}

struct MutexHandle {
  union {
    HANDLE mutex; /* for XP */
    CRITICAL_SECTION crit; /* for Vista+ */
  };
};

struct ConditionHandle {
  union {
    /* XP doesn't have condition vars */
    struct {
      unsigned int waitCount;
      bool         wasBroadcast;
      HANDLE       signalWaiters,
                   signalBroadcaster,
                   countMutex;
    };
    /* Vista+ have condition vars */
    CONDITION_VARIABLE cond;
  };
};

struct ThreadHandle {
  unsigned int threadID;
  HANDLE       threadHandle;
  void       * exitCode;
  char       * stk_thr_name;
};

static ThreadHandle *
createThreadHandle( void )
{
  ThreadHandle * threadPtr;
  MALLOC( sizeof( ThreadHandle ), &threadPtr );
  ::memset( threadPtr, 0, sizeof( ThreadHandle ) );
  return threadPtr;
}

struct RwLockHandle {
  Mutex      * lock;
  Condition  * cond;
  unsigned int rd,
               wr,
               waiters;
};
} // namespace rai


Mutex *
Mutex::create( MutexType isRecursiveLock )
{
  Mutex * m;

  MALLOC( sizeof( Mutex ) + sizeof( MutexHandle ), &m );
  m->mutexPtr = (MutexHandle *) &m[ 1 ];
  ::memset( m->mutexPtr, 0, sizeof( MutexHandle ) );
#ifdef MUTEX_OWNER
  m->owner = NULL;
#endif
  if ( rai_has_condition_vars_check() )
    ::InitializeCriticalSection( &m->mutexPtr->crit );
  else {
    try {
      if ( (m->mutexPtr->mutex = ::CreateMutex( NULL, FALSE, NULL )) == NULL )
        throw ThreadErr::getErr( ThreadErr::MUTEX_CREATE );
    } catch ( ... ) {
      FREE( m );
      throw;
    }
  }
  return m;
}


Mutex::~Mutex()
{
  if ( rai_has_condition_vars_use() )
    ::DeleteCriticalSection( &this->mutexPtr->crit );
  else
    ::CloseHandle( this->mutexPtr->mutex );
}


void
Mutex::operator delete( void *p )
{
  FREE( p );
}


void
Mutex::lock( void )
{
  if ( rai_has_condition_vars_use() )
    ::EnterCriticalSection( &this->mutexPtr->crit );
  else if ( ::WaitForSingleObject( this->mutexPtr->mutex,
                                   INFINITE ) != WAIT_OBJECT_0 )
    throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_LOCK );
#ifdef MUTEX_OWNER
  this->owner = Thread::self();
#endif
}


bool
Mutex::tryLock( void )
{
  bool success = false;
  if ( rai_has_condition_vars_use() ) {
    if ( ::TryEnterCriticalSection( &this->mutexPtr->crit ) )
      success = true;
  }
  else {
    switch ( ::WaitForSingleObject( this->mutexPtr->mutex, 0 ) ) {
      case WAIT_OBJECT_0:
        success = true;
        break;
      case WAIT_TIMEOUT:
        break;
      default:
        throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_TRYLOCK );
    }
  }
#ifdef MUTEX_OWNER
  if ( success )
    this->owner = Thread::self();
#endif
  return success;
}


void
Mutex::unlock( void )
{
  if ( rai_has_condition_vars_use() )
    ::LeaveCriticalSection( &this->mutexPtr->crit );
  else if ( ! ::ReleaseMutex( this->mutexPtr->mutex ) )
    throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_UNLOCK );
}


RwLock *
RwLock::create( void )
{
  RwLock     * m;
  unsigned int size;

  size = sizeof( RwLockHandle );
  MALLOC( sizeof( RwLock ) + size, &m );
  m->mutexPtr = (RwLockHandle *) &m[ 1 ];
  ::memset( m->mutexPtr, 0, size );
#ifdef MUTEX_OWNER
  m->owner = NULL;
#endif
  try {
    m->mutexPtr->lock = Mutex::create();
    m->mutexPtr->cond = Condition::create();
  } catch ( ... ) {
    delete m;
    throw;
  }

  return m;
}


RwLock::~RwLock()
{
  if ( this->mutexPtr->lock != NULL )
    delete this->mutexPtr->lock;
  if ( this->mutexPtr->cond != NULL )
    delete this->mutexPtr->cond;
}


void
RwLock::operator delete( void *p )
{
  FREE( p );
}


void
RwLock::rdLock( void )
{
  RwLockHandle &h = *this->mutexPtr;
  h.lock->lock();
  if ( h.wr != 0 ) {
    h.waiters++;
    while ( h.wr != 0 )
      h.cond->wait( h.lock );
    h.waiters--;
  }
  h.rd++;
#ifdef MUTEX_OWNER
  this->owner = Thread::self();
#endif
  h.lock->unlock();
}


bool
RwLock::tryRdLock( void )
{
  RwLockHandle &h = *this->mutexPtr;
  bool acquired = false;
  h.lock->lock();
  if ( h.wr == 0 ) {
    h.rd++;
    acquired = true;
#ifdef MUTEX_OWNER
    this->owner = Thread::self();
#endif
  }
  h.lock->unlock();
  return acquired;
}


void
RwLock::wrLock( void )
{
  RwLockHandle &h = *this->mutexPtr;
  h.lock->lock();
  if ( ( h.wr | h.rd ) != 0 ) {
    h.waiters++;
    while ( ( h.wr | h.rd ) != 0 )
      h.cond->wait( h.lock );
    h.waiters--;
  }
  h.wr = 1;
#ifdef MUTEX_OWNER
  this->owner = Thread::self();
#endif
  h.lock->unlock();
}


bool
RwLock::tryWrLock( void )
{
  RwLockHandle &h = *this->mutexPtr;
  bool acquired = false;
  h.lock->lock();
  if ( ( h.wr | h.rd ) == 0 ) {
    h.wr = 1;
    acquired = true;
#ifdef MUTEX_OWNER
    this->owner = Thread::self();
#endif
  }
  h.lock->unlock();
  return acquired;
}


void
RwLock::unlock( void )
{
  RwLockHandle &h = *this->mutexPtr;
  h.lock->lock();
  if ( h.rd > 0 )
    h.rd--;
  else
    h.wr = 0;
  if ( h.waiters != 0 )
    h.cond->signal();
  h.lock->unlock();
}


Condition *
Condition::create( void )
{
  Condition * c;

  MALLOC( sizeof( Condition ) + sizeof( ConditionHandle ), &c );
  c->conditionPtr = (ConditionHandle *) &c[ 1 ];
  ::memset( c->conditionPtr, 0, sizeof( ConditionHandle ) );

  if ( rai_has_condition_vars_check() ) {
    rai_wcond.init( &c->conditionPtr->cond );
  }
  else {
    if ( (c->conditionPtr->signalWaiters = ::CreateSemaphore( NULL, 0,
                                                 0x7fffffff, NULL )) == NULL ||
         (c->conditionPtr->signalBroadcaster = ::CreateEvent( NULL, FALSE,
                                                 FALSE, NULL )) == NULL ||
         (c->conditionPtr->countMutex = ::CreateMutex( NULL, FALSE,
                                                 NULL )) == NULL ) {
      if ( c->conditionPtr->signalWaiters != NULL )
        ::CloseHandle( c->conditionPtr->signalWaiters );
      if ( c->conditionPtr->signalBroadcaster != NULL )
        ::CloseHandle( c->conditionPtr->signalBroadcaster );
      FREE( c );
      throw ThreadErr::getErr( ThreadErr::COND_CREATE );
    }
  }
  return c;
}


Condition::~Condition()
{
  if ( ! rai_has_condition_vars_use() ) {
    ::CloseHandle( this->conditionPtr->signalWaiters );
    ::CloseHandle( this->conditionPtr->signalBroadcaster );
    ::CloseHandle( this->conditionPtr->countMutex );
  }
}


void
Condition::operator delete( void *p )
{
  FREE( p );
}


void
Condition::wait( Mutex *mutex )
{
  if ( rai_has_condition_vars_use() ) {
    rai_wcond.sleep( &this->conditionPtr->cond, &mutex->mutexPtr->crit,
                     INFINITE );
  }
  else {
    bool isLastWaiter;

    /* serialize access of waitCount */
    if ( ::WaitForSingleObject( this->conditionPtr->countMutex,
                                INFINITE ) != WAIT_OBJECT_0 )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );

    this->conditionPtr->waitCount++;

    if ( ! ::ReleaseMutex( this->conditionPtr->countMutex ) )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );

    /* release mutex and wait on sem, signal() or broadcast() will wakeup */
    if ( ::SignalObjectAndWait( mutex->mutexPtr->mutex,
                                this->conditionPtr->signalWaiters,
                                INFINITE, FALSE ) != WAIT_OBJECT_0 )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );

    /* serialize access of waitCount */
    if ( ::WaitForSingleObject( this->conditionPtr->countMutex,
                                INFINITE ) != WAIT_OBJECT_0 )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );

    this->conditionPtr->waitCount--;
    if ( this->conditionPtr->wasBroadcast &&
         this->conditionPtr->waitCount == 0 )
      isLastWaiter = true;
    else
      isLastWaiter = false;

    if ( ! ::ReleaseMutex( this->conditionPtr->countMutex ) )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );

    /* reaquire mutex */
    if ( isLastWaiter ) {
      if ( ::SignalObjectAndWait( this->conditionPtr->signalBroadcaster,
                                  mutex->mutexPtr->mutex, INFINITE,
                                  FALSE ) != WAIT_OBJECT_0 )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );
    }
    else {
      if ( ::WaitForSingleObject( mutex->mutexPtr->mutex,
                                  INFINITE ) != WAIT_OBJECT_0 )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );
    }
  }
}


bool
Condition::timedWait( Mutex *mutex,  TimeMSecs howLong )
{
  bool wasSignaled = false;
  if ( howLong == 0 )
    howLong = 1;
  if ( rai_has_condition_vars_use() ) {
    if ( rai_wcond.sleep( &this->conditionPtr->cond, &mutex->mutexPtr->crit,
                          (DWORD) howLong ) )
      wasSignaled = true;
  }
  else {
    bool isLastWaiter;
    /* serialize access of waitCount */
    if ( ::WaitForSingleObject( this->conditionPtr->countMutex,
                                INFINITE ) != WAIT_OBJECT_0 )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );

    this->conditionPtr->waitCount++;

    if ( ! ::ReleaseMutex( this->conditionPtr->countMutex ) )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );

    /* release mutex and wait on sem, signal() or broadcast() will wakeup */
    switch ( ::SignalObjectAndWait( mutex->mutexPtr->mutex,
                                    this->conditionPtr->signalWaiters,
                                    (DWORD) howLong, FALSE ) ) {
      case WAIT_OBJECT_0:
        wasSignaled = true;
        break;
      case WAIT_TIMEOUT:
        break;
      default:
        throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );
    }

    /* serialize access of waitCount */
    if ( ::WaitForSingleObject( this->conditionPtr->countMutex,
                                INFINITE ) != WAIT_OBJECT_0 )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );

    this->conditionPtr->waitCount--;
    if ( /*wasSignaled &&*/ this->conditionPtr->wasBroadcast &&
                        this->conditionPtr->waitCount == 0 )
      isLastWaiter = true;
    else
      isLastWaiter = false;

    if ( ! ::ReleaseMutex( this->conditionPtr->countMutex ) )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );

    /* reaquire mutex */
    if ( isLastWaiter ) {
      if ( ::SignalObjectAndWait( this->conditionPtr->signalBroadcaster,
                                  mutex->mutexPtr->mutex, INFINITE,
                                  FALSE ) != WAIT_OBJECT_0 )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );
    }
    else {
      if ( ::WaitForSingleObject( mutex->mutexPtr->mutex,
                                  INFINITE ) != WAIT_OBJECT_0 )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );
    }
  }
  return wasSignaled;
}


bool
Condition::timedWait( Mutex *mutex,  double howLong )
{
  return this->timedWait( mutex, (TimeMSecs) howLong );
}


void
Condition::signal( void )
{
  if ( rai_has_condition_vars_use() ) {
    rai_wcond.wake( &this->conditionPtr->cond );
  }
  else {
    bool haveWaiters;

    /* check if there are any waiters */
    if ( ::WaitForSingleObject( this->conditionPtr->countMutex,
                                INFINITE ) != WAIT_OBJECT_0 )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_SIGNAL );

    if ( this->conditionPtr->waitCount > 0 )
      haveWaiters = true;
    else
      haveWaiters = false;

    if ( ! ::ReleaseMutex( this->conditionPtr->countMutex ) )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_SIGNAL );

    /* signal a waiter */
    if ( haveWaiters ) {
      if ( ! ::ReleaseSemaphore( this->conditionPtr->signalWaiters, 1, NULL ) )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_SIGNAL );
    }
  }
}


void
Condition::broadcast( void )
{
  if ( rai_has_condition_vars_use() ) {
    rai_wcond.wakeAll( &this->conditionPtr->cond );
  }
  else {
    bool haveWaiters;

    /* check if there are any waiters */
    if ( ::WaitForSingleObject( this->conditionPtr->countMutex,
                                INFINITE ) != WAIT_OBJECT_0 )
      throw ThreadErr::getErr( ThreadErr::BAD_COND_BROADCAST );

    if ( this->conditionPtr->waitCount > 0 ) {
      this->conditionPtr->wasBroadcast = true;
      haveWaiters = true;
    }
    else {
      haveWaiters = false;
    }

    if ( haveWaiters ) {
      /* wake everyone up */
      if ( ! ::ReleaseSemaphore( this->conditionPtr->signalWaiters,
                                 this->conditionPtr->waitCount, NULL ) )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_BROADCAST );

      if ( ! ::ReleaseMutex( this->conditionPtr->countMutex ) )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_BROADCAST );

      /* wait for last waiter to wakeup */
      if ( ::WaitForSingleObject( this->conditionPtr->signalBroadcaster,
                                  INFINITE ) != WAIT_OBJECT_0 )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_BROADCAST );

      this->conditionPtr->wasBroadcast = false;
    }
    else {
      if ( ! ::ReleaseMutex( this->conditionPtr->countMutex ) )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_BROADCAST );
    }
  }
}


namespace rai {
class StartThread {
  public:
    static unsigned __stdcall startRoutine( void *thread );
};
}

unsigned __stdcall
StartThread::startRoutine( void *thread )
{
  Thread * thr = (Thread *) thread;
  /* name copy followed by a "THRBASE" marker, both live on this stack
   * frame so the thread base can be located by scanning for the marker */
  static const size_t marker_len = sizeof( "THRBASE" );  /* 8 incl NUL */
  char thr_name[ sizeof( thr->name ) + marker_len ];

  ::memcpy( &thr_name[ sizeof( thr->name ) ], "THRBASE", marker_len );
  ::memcpy( thr_name, thr->name, sizeof( thr->name ) );
  thr->threadPtr->stk_thr_name = thr_name;
  logDebug( LDEBUG, "Thread %s 0x%lx this 0x%lx", thr->name,
            (unsigned long) (ulongptr) (void *) thr_name,
            (unsigned long) (ulongptr) (void *) thr );
  thr->isRunning = true;
  thr->tid = Thread::getOSThreadId(); 
  if ( thr->sched_prio != 0 )
    thr->setPriority( thr->sched_prio, thr->sched_policy );

  Thread::putSpecific( threadSelfKey, thr );

  thrListLock->lock();
  thr->isJoined  = false;
  thr->addThrList();
  thrListCond->broadcast();
  thrListLock->unlock();

  logDebug( LDEBUG, "Thread(%u) %s start", thr->tid, thr->name );
  thr->run();
  logDebug( LDEBUG, "Thread(%u) %s stop", thr->tid, thr->name );

  if ( thr->isRunning )
    thr->exit( NULL );
  return 0;
}


void
Thread::disableCancelState( void )
{
}

void
Thread::enableCancelState( void )
{
}

void
Thread::recurseAll( ListRec &r )
{
  if ( thrListLock == NULL )
    return;
  thrListLock->lock();
  try {
    Thread::recurseThrList( r );
  } catch ( ... ) {
  }
  thrListLock->unlock();
}


Thread::Thread( const char *nm,  unsigned int stack_size,  int sched_priority,
                SchedPolicy sched_policy )
{
  this->stack_size   = stack_size;
  this->sched_prio   = sched_priority;
  this->sched_policy = sched_policy;
  this->threadPtr    = NULL;
  this->isRunning    = false;
  this->isJoined     = true;
  this->inThrList    = false;
  this->tid          = 0;
  this->tnext        = NULL;
  this->tlast        = NULL;
  this->setName( nm );
}


void
Thread::cleanup( void )
{
  if ( this->inThrList && thrListLock != NULL ) {
    thrListLock->lock();
    this->removeThrList();
    thrListLock->unlock();
  }

  if ( threadAllocKey != Thread::NIL_KEY ) {
    ThrMBuf *mbuf = (ThrMBuf *) Thread::getSpecific( threadAllocKey );
    if ( mbuf != NULL ) {
      Thread::putSpecific( threadAllocKey, NULL );
      delete mbuf;
    }
  }

  rai_thread_on_exit_release();
}


Thread::~Thread()
{
  if ( this->inThrList && thrListLock != NULL ) {
    thrListLock->lock();
    this->removeThrList();
    thrListLock->unlock();
  }
  if ( this->threadPtr != NULL )
    FREE( this->threadPtr );
}


bool
Thread::setPriority( int sched_priority,  SchedPolicy sched_policy )
{
  int priority;

  switch( sched_priority ) {
    case 1:
    case 2: priority = THREAD_PRIORITY_TIME_CRITICAL; break;
    case 3:
    case 4: priority = THREAD_PRIORITY_HIGHEST; break;
    case 5:
    case 6: priority = THREAD_PRIORITY_ABOVE_NORMAL; break;
    case 7:
    case 8: priority = THREAD_PRIORITY_NORMAL; break;
    case 9:
    default:
    case 10: priority = THREAD_PRIORITY_BELOW_NORMAL; break;
  }

  if ( ::SetThreadPriority( this->threadPtr->threadHandle, priority ) ) {
    this->sched_prio   = sched_priority;
    this->sched_policy = sched_policy;
    return true;
  }
  logMinor( LMINOR, "SetThreadPriority ( %d ) failed: errno=%d",
            priority, ::GetLastError() );
  return false;
}


void
Thread::getThreadHandle( void *hndl )
{
  *((HANDLE *) hndl) = (HANDLE) this->threadPtr->threadHandle;
}


void
Thread::start( void )
{
  rai_create_thread_keys();

  if ( thrListLock == NULL )
    thrListLock = Mutex::create();
  if ( thrListCond == NULL )
    thrListCond = Condition::create();

  if ( this->isRunning )
    throw ThreadErr::getErr( ThreadErr::THREAD_RUNNING );

  if ( this->threadPtr == NULL ) {
    this->threadPtr = rai::createThreadHandle();
  }
  this->threadPtr->threadHandle = 0;
  this->threadPtr->threadID     = -1;
  this->threadPtr->exitCode     = NULL;
  this->isRunning = true;
  if ( (this->threadPtr->threadHandle = (HANDLE) _beginthreadex(
                                          NULL, this->stack_size,
                                          StartThread::startRoutine,
                                          this, 0,
                                          &this->threadPtr->threadID )) == 0 ) {
    this->isRunning = false;
    throw ThreadErr::getErr( ThreadErr::THREAD_START );
  }
  else {
    /* wait for thread to run() before returning */
    thrListLock->lock();
    while ( this->isJoined )
      thrListCond->wait( thrListLock );
    thrListLock->unlock();
  }
}


void *
Thread::join( void )
{
  logDebug( LDEBUG, "Thread(%u) %s join", this->tid, this->name );
  if ( ! this->isJoined ) {
    if ( WaitForSingleObject( this->threadPtr->threadHandle,
                              INFINITE ) != WAIT_OBJECT_0 )
      throw ThreadErr::getErr( ThreadErr::THREAD_JOIN );
    ::CloseHandle( this->threadPtr->threadHandle );
    this->isJoined = true;
  }
  logDebug( LDEBUG, "Thread(%u) %s join done", this->tid, this->name );
  if ( this->threadPtr == NULL )
    return NULL;
  return this->threadPtr->exitCode;
}


void
Thread::kill( void )
{
  logDebug( LDEBUG, "Thread(%u) %s kill (nop)", this->tid, this->name );
  /* no way to do this like pthread_cancel + deferred */
}

void
Thread::interrupt( void )
{
  /* no way to do this like pthread_kill */
}

void
Thread::sleep( TimeMSecs ms )
{
  Time::sleepMillisecs( ms );
}


void
Thread::yield( void )
{
  ::SwitchToThread();
}


void
Thread::exit( void *exitCode )
{
  logDebug( LDEBUG, "Thread(%u) %s exiting", this->tid, this->name );
  this->threadPtr->exitCode = exitCode;
  this->threadPtr->stk_thr_name = NULL;
  this->isRunning = false;
  this->cleanup();
  logDebug( LDEBUG, "Thread(%u) %s exit", this->tid, this->name );
  _endthreadex( 0 );
}


unsigned int
Thread::createSpecificKey( void )
{
  DWORD dwIdx = ::TlsAlloc();

  if ( dwIdx == TLS_OUT_OF_INDEXES )
    throw ThreadErr::getErr( ThreadErr::THREAD_CREATE_KEY );

  return (unsigned int) dwIdx;
}


void
Thread::putSpecific( unsigned int key,  void *ptr )
{
  ::TlsSetValue( (DWORD) key, ptr );
}


void *
Thread::getSpecific( unsigned int key )
{
  return ::TlsGetValue( (DWORD) key );
}


#else
/*
 * POSIX thread stuff
 *
 * Mostly a wrapper around pthread calls
 */

#include <sys/time.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>

static pthread_mutex_t thrListLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t thrListCond = PTHREAD_COND_INITIALIZER;

static void rai_lock_thr_list( void ) {
  ::pthread_mutex_lock( &thrListLock );
}
static void rai_unlock_thr_list( void ) {
  ::pthread_mutex_unlock( &thrListLock );
}
static void rai_thread_on_exit( void *p ) {
  ThreadExitList::doExit( (ThreadExitList *) p );
}
static void rai_create_exit_key( void ) {
  pthread_key_t k;
  ::pthread_key_create( &k, rai_thread_on_exit );
  threadExitKey = k;
}


namespace rai {
/* #define EMULATE_RECURSIVE_LOCK for solaris 2.7 */
#if defined( EMULATE_RECURSIVE_LOCK )

  struct MutexHandle {
    pthread_mutex_t  mutex;
    Mutex::MutexType isRecursive;
  };


  struct RecursiveMutexHandle : public MutexHandle {
    pthread_cond_t condition;
    pthread_t      threadId;
    unsigned int   recursionCount;
  };

#else /* use native recursive attribute */

  struct MutexHandle {
    pthread_mutex_t mutex;
  };

#endif

struct RwLockHandle {
  pthread_rwlock_t mutex;
};

struct ConditionHandle {
  pthread_cond_t condition;
};


struct ThreadHandle {
  pthread_t        threadId;
  void           * exitCode;
#if defined( __linux ) && defined( SET_PROFILER_ITIMER )
  /* linux pthreads doesn't automatically set the profile timer */
  struct itimerval itimer;
#endif
  char           * stk_thr_name;
  int              cancel_stk[ 32 ];
  unsigned int     cancel_top;

  void pushCancelState( int state ) {
    if ( this->cancel_top < 32 )
      this->cancel_stk[ this->cancel_top++ ] = state;
  }
  int popCancelState( void ) {
    if ( this->cancel_top == 0 )
      return PTHREAD_CANCEL_ENABLE;
    return this->cancel_stk[ --this->cancel_top ];
  }
};

static ThreadHandle *
createThreadHandle( void )
{
  ThreadHandle * threadPtr;
  MALLOC( sizeof( ThreadHandle ), &threadPtr );
  ::memset( threadPtr, 0, sizeof( ThreadHandle ) );
  return threadPtr;
}

struct SemaphoreHandle {
  sem_t		   semaphore;
};
} // namespace rai

Mutex *
Mutex::create( MutexType isRecursiveLock )
{
  Mutex      * m;
  unsigned int size;

  size = sizeof( rai::MutexHandle );
#if defined( EMULATE_RECURSIVE_LOCK )
  if ( isRecursiveLock == RECURSIVE_LOCK )
    size = sizeof( RecursiveMutexHandle );
#endif
  MALLOC( sizeof( Mutex ) + size, &m );
  m->mutexPtr = (MutexHandle *) &m[ 1 ];
#ifdef MUTEX_OWNER
  m->owner = NULL;
#endif
  pthread_mutex_init( &m->mutexPtr->mutex, NULL );

#if defined( EMULATE_RECURSIVE_LOCK )
  if ( (m->mutexPtr->isRecursive = isRecursiveLock) == RECURSIVE_LOCK ) {
    RecursiveMutexHandle * recursivePtr =
      (RecursiveMutexHandle *) m->mutexPtr;
    pthread_cond_init( &recursivePtr->condition, NULL );
    recursivePtr->threadId       = 0;
    recursivePtr->recursionCount = 0;
  }
#else
  {
    pthread_mutexattr_t attr;

    if ( isRecursiveLock == RECURSIVE_LOCK ) {
      pthread_mutexattr_init( &attr );
#if defined( PTHREAD_MUTEX_RECURSIVE )
      pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
#else
      pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE_NP );
#endif
      pthread_mutex_init( &m->mutexPtr->mutex, &attr );
      pthread_mutexattr_destroy( &attr );
    }
    else {
      pthread_mutex_init( &m->mutexPtr->mutex, NULL );
    }
  }
#endif

  return m;
}


Mutex::~Mutex()
{
  pthread_mutex_destroy( &this->mutexPtr->mutex );
#if defined( EMULATE_RECURSIVE_LOCK )
  if ( this->mutexPtr->isRecursive == RECURSIVE_LOCK ) {
    pthread_cond_destroy( &((RecursiveMutexHandle *)
                            this->mutexPtr)->condition );
  }
#endif
}


void
Mutex::operator delete( void *p )
{
  FREE( p );
}


void
Mutex::lock( void )
{
  if ( pthread_mutex_lock( &this->mutexPtr->mutex ) != 0 )
    throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_LOCK );

#if ! defined( EMULATE_RECURSIVE_LOCK )
  #ifdef MUTEX_OWNER
    this->owner = Thread::self();
  #endif
#else
  if ( this->mutexPtr->isRecursive == RECURSIVE_LOCK ) {
    RecursiveMutexHandle * recursivePtr =
      (RecursiveMutexHandle *) this->mutexPtr;
    pthread_t self = pthread_self();
    while ( recursivePtr->recursionCount != 0 &&
            recursivePtr->threadId != self ) {
      if ( pthread_cond_wait( &recursivePtr->condition,
                              &recursivePtr->mutex ) != 0 )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );
    }
    recursivePtr->recursionCount++;
    recursivePtr->threadId = self;
  #ifdef MUTEX_OWNER
    this->owner = Thread::self();
  #endif
    if ( pthread_mutex_unlock( &recursivePtr->mutex ) != 0 )
      throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_UNLOCK );
  }
#endif
}


bool
Mutex::tryLock( void )
{
  switch ( pthread_mutex_trylock( &this->mutexPtr->mutex ) ) {
    case 0:
#if ! defined( EMULATE_RECURSIVE_LOCK )
    #ifdef MUTEX_OWNER
      this->owner = Thread::self();
    #endif
#else
      if ( this->mutexPtr->isRecursive == RECURSIVE_LOCK ) {
        RecursiveMutexHandle * recursivePtr =
          (RecursiveMutexHandle *) this->mutexPtr;
        pthread_t self = pthread_self();
        if ( recursivePtr->recursionCount != 0 &&
             recursivePtr->threadId != self ) {
          if ( pthread_mutex_unlock( &recursivePtr->mutex ) != 0 )
            throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_UNLOCK );
          break; /* not aquired */
        }
        recursivePtr->recursionCount++;
        recursivePtr->threadId = self;
      #ifdef MUTEX_OWNER
        this->owner = Thread::self();
      #endif
        if ( pthread_mutex_unlock( &recursivePtr->mutex ) != 0 )
          throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_UNLOCK );
      }
#endif
      return true;
    case EBUSY:
      break;
    default:
      throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_TRYLOCK );
  }
  return false;
}


void
Mutex::unlock( void )
{
#if defined( EMULATE_RECURSIVE_LOCK )
  if ( this->mutexPtr->isRecursive == RECURSIVE_LOCK ) {
    RecursiveMutexHandle * recursivePtr =
      (RecursiveMutexHandle *) this->mutexPtr;

    if ( pthread_mutex_lock( &recursivePtr->mutex ) != 0 )
      throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_LOCK );
    if ( --recursivePtr->recursionCount == 0 ) {
      if ( pthread_cond_signal( &recursivePtr->condition ) != 0 )
        throw ThreadErr::getErr( ThreadErr::BAD_COND_SIGNAL );
    }
  }
#endif
  if ( pthread_mutex_unlock( &this->mutexPtr->mutex ) != 0 )
    throw ThreadErr::getErr( ThreadErr::BAD_MUTEX_UNLOCK );
}


RwLock *
RwLock::create()
{
  RwLock      * m;
  unsigned int size;

  size = sizeof( RwLockHandle );
  MALLOC( sizeof( RwLock ) + size, &m );
  m->mutexPtr = (RwLockHandle *) &m[ 1 ];
#ifdef MUTEX_OWNER
  m->owner = NULL;
#endif
  pthread_rwlock_init( &m->mutexPtr->mutex, NULL );

  return m;
}


RwLock::~RwLock()
{
  pthread_rwlock_destroy( &this->mutexPtr->mutex );
}

void
RwLock::operator delete( void *p )
{
  FREE( p );
}

void
RwLock::rdLock( void )
{
  int err;

  while( ( err = pthread_rwlock_rdlock( &this->mutexPtr->mutex ) ) != 0 ) {
    switch(err) {
    case EINVAL:
      throw ThreadErr::getErr( ThreadErr::RWLOCK_RDLOCK_EINVAL );
    case EDEADLK:
      throw ThreadErr::getErr( ThreadErr::RWLOCK_RDLOCK_EDEADLK );
    case EAGAIN:
      break;
    default:
      logError( LERROR, getErr(ThreadErr::RWLOCK_RDLOCK), "RwLock rdLock unknown error %d", err );
      throw ThreadErr::getErr( ThreadErr::RWLOCK_RDLOCK );
    }
  }

#ifdef MUTEX_OWNER
  this->owner = Thread::self();
#endif
}

bool
RwLock::tryRdLock( void )
{
  int err;
  switch ( ( err = pthread_rwlock_tryrdlock( &this->mutexPtr->mutex ) ) ) {
  case 0:
#ifdef MUTEX_OWNER
    this->owner = Thread::self();
#endif
    return true;
  case EBUSY:
  case EAGAIN:
    break;
  case EINVAL:
    throw ThreadErr::getErr( ThreadErr::RWLOCK_TRYRDLOCK_EINVAL );
  case EDEADLK:
    throw ThreadErr::getErr( ThreadErr::RWLOCK_TRYRDLOCK_EDEADLK );
  default:
    logError( LERROR, getErr(ThreadErr::RWLOCK_TRYRDLOCK), "RwLock tryRdLock unknown error %d", err );
    throw ThreadErr::getErr( ThreadErr::RWLOCK_TRYRDLOCK );
  }
  return false;
}

void
RwLock::wrLock( void )
{
  int err;
  switch( (err = pthread_rwlock_wrlock( &this->mutexPtr->mutex ) ) ) {
  case 0:
    break;
  case EINVAL:
    throw ThreadErr::getErr( ThreadErr::RWLOCK_WRLOCK_EINVAL );
  case EDEADLK:
    throw ThreadErr::getErr( ThreadErr::RWLOCK_WRLOCK_EDEADLK );
  default:
    logError( LERROR, getErr(ThreadErr::RWLOCK_WRLOCK), "RwLock wrLock unknown error %d", err );
    throw ThreadErr::getErr( ThreadErr::RWLOCK_WRLOCK );
  }
#ifdef MUTEX_OWNER
  this->owner = Thread::self();
#endif
}

bool
RwLock::tryWrLock( void )
{
  int err;
  switch ( (err = pthread_rwlock_trywrlock( &this->mutexPtr->mutex ) ) ) {
    case 0:
#ifdef MUTEX_OWNER
      this->owner = Thread::self();
#endif
      return true;
    case EBUSY:
      break;
  case EINVAL:
    throw ThreadErr::getErr( ThreadErr::RWLOCK_TRYWRLOCK_EINVAL );
  case EDEADLK:
      throw ThreadErr::getErr( ThreadErr::RWLOCK_TRYWRLOCK_EDEADLK );
    default:
      logError( LERROR, getErr(ThreadErr::RWLOCK_TRYWRLOCK), "RwLock wrLock unknown error %d", err );
      throw ThreadErr::getErr( ThreadErr::RWLOCK_TRYWRLOCK );
  }
  return false;
}

void
RwLock::unlock( void )
{
  int err;
  switch( (err = pthread_rwlock_unlock( &this->mutexPtr->mutex ) ) ) {
  case 0:
    break;
  case EINVAL:
    throw ThreadErr::getErr( ThreadErr::RWLOCK_UNLOCK_EINVAL );
  case EPERM:
    throw ThreadErr::getErr( ThreadErr::RWLOCK_UNLOCK_EPERM );
  default:
    logError( LERROR, getErr(ThreadErr::RWLOCK_UNLOCK), "RwLock unknown unlock error %d", err );
    throw ThreadErr::getErr( ThreadErr::RWLOCK_UNLOCK );
  }
}

Condition *
Condition::create( void )
{
  Condition * c;

  MALLOC( sizeof( Condition ) + sizeof( ConditionHandle ), &c );
  c->conditionPtr = (ConditionHandle *) &c[ 1 ];

  pthread_cond_init( &c->conditionPtr->condition, NULL );

  return c;
}


Condition::~Condition()
{
  pthread_cond_destroy( &this->conditionPtr->condition );
}


void
Condition::operator delete( void *p )
{
  FREE( p );
}


void
Condition::wait( Mutex *mutex )
{
  if ( pthread_cond_wait( &this->conditionPtr->condition,
                          &mutex->mutexPtr->mutex ) != 0 )
    throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );
}


bool
Condition::timedWait( Mutex *mutex,  TimeMSecs howLong )
{
  struct timespec timeout;
  struct timeval  tv;
  bool            wasSignaled;

  ::gettimeofday( &tv, NULL );
  howLong += (TimeMSecs) tv.tv_sec * 1000U + (TimeMSecs) tv.tv_usec / 1000U;
  timeout.tv_sec  = (time_t) ( howLong / (TimeMSecs) 1000U );
  timeout.tv_nsec = (long) ( howLong % (TimeMSecs) 1000U ) * 1000U * 1000U;

  switch ( ::pthread_cond_timedwait( &this->conditionPtr->condition,
                                     &mutex->mutexPtr->mutex, &timeout ) ) {
    case 0:
      wasSignaled = true;
      break;
    case ETIMEDOUT:
    case EINTR: /* signal interrupted */
      wasSignaled = false;
      break;
    default:
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );
  }

  return wasSignaled;
}


bool
Condition::timedWait( Mutex *mutex,  double howLong )
{
  struct timespec timeout;
  struct timeval  tv;
  double          secs;
  bool            wasSignaled;

  ::gettimeofday( &tv, NULL );
  howLong /= 1000.0;
  secs     = ::floor( howLong );
  howLong  = ( howLong - secs ) + (double) tv.tv_usec / 1000.0 / 1000.0;
  timeout.tv_sec = (time_t) secs + tv.tv_sec;
  while ( howLong >= 1.0 ) {
    timeout.tv_sec += 1;
    howLong -= 1.0;
  }
  timeout.tv_nsec = (long) ( howLong * 1000.0 * 1000.0 * 1000.0 );

  switch ( ::pthread_cond_timedwait( &this->conditionPtr->condition,
                                     &mutex->mutexPtr->mutex, &timeout ) ) {
    case 0:
      wasSignaled = true;
      break;
    case ETIMEDOUT:
    case EINTR: /* signal interrupted */
      wasSignaled = false;
      break;
    default:
      throw ThreadErr::getErr( ThreadErr::BAD_COND_WAIT );
  }

  return wasSignaled;
}


void
Condition::signal( void )
{
  if ( ::pthread_cond_signal( &this->conditionPtr->condition ) != 0 )
    throw ThreadErr::getErr( ThreadErr::BAD_COND_SIGNAL );
}


void
Condition::broadcast( void )
{
  if ( ::pthread_cond_broadcast( &this->conditionPtr->condition ) != 0 )
    throw ThreadErr::getErr( ThreadErr::BAD_COND_BROADCAST );
}


namespace rai {
class StartThread {
  public:
    static void *startRoutine( void *thread );
};
}

void *
StartThread::startRoutine( void *thread )
{
  Thread * thr = (Thread *) thread;
  /* name copy followed by a "THRBASE" marker, both live on this stack
   * frame so the thread base can be located by scanning for the marker */
  static const size_t marker_len = sizeof( "THRBASE" );  /* 8 incl NUL */
  char thr_name[ sizeof( thr->name ) + marker_len ];

  ::memcpy( &thr_name[ sizeof( thr->name ) ], "THRBASE", marker_len );
  ::memcpy( thr_name, thr->name, sizeof( thr->name ) );
  thr->threadPtr->stk_thr_name = thr_name;
  logDebug( LDEBUG, "Thread %s 0x%lx this 0x%lx", thr->name,
            (unsigned long) (ulongptr) (void *) thr_name,
            (unsigned long) (ulongptr) (void *) thr );
  sigset_t  set;
  ::sigemptyset( &set );
#ifndef PTHREAD_CANCEL_DEFERRED
  ::sigaddset( &set, SIGUSR1 ); /* use sigusr1 to simulate pthread cancel */
#endif
  ::sigaddset( &set, SIGUSR2 ); /* use sigusr2 for interrupt() */
  ::pthread_sigmask( SIG_UNBLOCK, &set, NULL );
#ifdef PTHREAD_CANCEL_DEFERRED
  logTrace( LTRACE, "%s pthread_setcancelstate( PTHREAD_CANCEL_ENABLE )",
            thr->name );
  if ( ::pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL ) != 0 ) {
    logError( LERROR, NULL,
     "Thread %s, unable to set pthread_setcancelstate( PTHREAD_CANCEL_ENABLE )",
              thr->name );
  }
  else {
    const char *s = "PTHREAD_CANCEL_DEFERRED";
    /* deferred type causes pthread_cancel() to defer until the next
     * function that is a cancellation point, like read() and recv() */
    int ctype = PTHREAD_CANCEL_DEFERRED;
    if ( ::getenv( "RAI_PTHREAD_CANCEL_ASYNCHRONOUS" ) != NULL ) {
     /* asynchronous type causes pthread_cancel() to stop the thread at any
      * time */
      ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
      s = "PTHREAD_CANCEL_ASYNCHRONOUS";
    }
    logTrace( LTRACE, "%s pthread_setcanceltype( %s )", thr->name, s );
    if ( ::pthread_setcanceltype( ctype, NULL ) != 0 ) {
      logError( LERROR, NULL,
        "Thread %s, unable to set pthread_setcanceltype( %s )", thr->name, s );
    }
  }
#endif
#if defined( __linux ) && defined( SET_PROFILER_ITIMER )
  ::setitimer( ITIMER_PROF, &thr->threadPtr->itimer, NULL );
#endif
  thr->isRunning = true;
  thr->tid = Thread::getOSThreadId(); 
  if ( thr->sched_prio != 0 )
    thr->setPriority( thr->sched_prio, thr->sched_policy );

  Thread::putSpecific( threadSelfKey, thr );

  pthread_cleanup_push( rai_thread_cleanup, thr );

  ::pthread_mutex_lock( &thrListLock );
  thr->isJoined  = false;
  thr->addThrList();
  ::pthread_cond_broadcast( &thrListCond );
  ::pthread_mutex_unlock( &thrListLock );

  logDebug( LDEBUG, "Thread(%u) %s start", thr->tid, thr->name );
  thr->run();
  logDebug( LDEBUG, "Thread(%u) %s stop", thr->tid, thr->name );

  pthread_cleanup_pop( 0 );

  if ( thr->isRunning )
    thr->exit( NULL );
  return thr->threadPtr->exitCode;
}

void
Thread::disableCancelState( void )
{
  Thread *thr = Thread::self();
  int state;
  if ( thr != NULL ) {
    /* can't log error here, since log calls this function */
    if ( ::pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &state ) == 0 )
      if ( thr->threadPtr != NULL )
        thr->threadPtr->pushCancelState( state );
  }
}

void
Thread::enableCancelState( void )
{
  Thread *thr = Thread::self();
  if ( thr != NULL ) {
    int state = ( thr->threadPtr != NULL ? 
                  thr->threadPtr->popCancelState() : PTHREAD_CANCEL_ENABLE );
    ::pthread_setcancelstate( state, NULL );
  }
}

extern "C" void *
rai_start_thread_startRoutine( void *thread ) {
  return StartThread::startRoutine( thread );
}

void
Thread::recurseAll( ListRec &r )
{
  ::pthread_mutex_lock( &thrListLock );
  try {
    Thread::recurseThrList( r );
  } catch ( ... ) {
  }
  ::pthread_mutex_unlock( &thrListLock );
}


Thread::Thread( const char *nm,  unsigned int stack_sz,  int sched_priority,
                SchedPolicy sched_pol )
{
  this->stack_size   = stack_sz;
  this->sched_prio   = sched_priority;
  this->sched_policy = sched_pol;
  this->threadPtr    = NULL;
  this->isRunning    = false;
  this->isJoined     = true;
  this->inThrList    = false;
  this->tid          = 0;
  this->tnext        = NULL;
  this->tlast        = NULL;
  this->setName( nm );
}


void
Thread::cleanup( void )
{
  logDebug( LDEBUG, "Cleanup thread %s (%u)", this->name, 
            Thread::getOSThreadId() );

  if ( this->inThrList ) {
    ::pthread_mutex_lock( &thrListLock );
    this->removeThrList();
    ::pthread_mutex_unlock( &thrListLock );
  }

  if ( threadAllocKey != Thread::NIL_KEY ) {
    ThrMBuf *mbuf = (ThrMBuf *) Thread::getSpecific( threadAllocKey );
    if ( mbuf != NULL ) {
      Thread::putSpecific( threadAllocKey, NULL );
      delete mbuf;
    }
  }

  rai_thread_on_exit_release();
}


Thread::~Thread()
{
  if ( this->inThrList ) {
    ::pthread_mutex_lock( &thrListLock );
    this->removeThrList();
    ::pthread_mutex_unlock( &thrListLock );
  }
  if ( this->threadPtr != NULL )
    FREE( this->threadPtr );
}

#ifndef PTHREAD_CANCEL_DEFERRED
extern "C" { static void rai_thread_sigusr1_handler( int ) {} }
#endif
extern "C" { static void rai_thread_sigusr2_handler( int ) {} }

bool
Thread::setPriority( int sched_priority,  SchedPolicy sched_pol )
{
#ifdef _POSIX_PRIORITY_SCHEDULING
  int                minPrio,
                     maxPrio,
                     pthreadSchedPolicy,
                     status;
  struct sched_param param;

  switch( sched_pol ) {
  default:
    sched_pol = THREAD_SCHED_OTHER;
    /* fall through */
  case THREAD_SCHED_OTHER :
    pthreadSchedPolicy = SCHED_OTHER;
    break;
  case THREAD_SCHED_RR :
    pthreadSchedPolicy = SCHED_RR;
    break;
  case THREAD_SCHED_FIFO :
    pthreadSchedPolicy = SCHED_FIFO;
    break;
  }

  minPrio = ::sched_get_priority_min( sched_pol );
  maxPrio = ::sched_get_priority_max( sched_pol );

  if ( sched_priority <= 0 || sched_priority > 10 )
    sched_priority = 10;
  ::memset( &param, 0, sizeof( param ) );
  param.sched_priority = minPrio +
                         ( maxPrio - minPrio ) * ( sched_priority - 1 ) / 10;

  status = ::pthread_setschedparam( this->threadPtr->threadId,
                                    pthreadSchedPolicy, &param );
  if ( status == 0 ) {
    this->sched_prio   = sched_priority;
    this->sched_policy = sched_pol;
    return true;
  }
  logMinor( LMINOR, "Set schedpolicy( %s, %d ) failed: %d, errno=%d",
            pthreadSchedPolicy == SCHED_OTHER ? "SCHED_OTHER" :
            pthreadSchedPolicy == SCHED_RR ? "SCHED_RR" :
            "SCHED_FIFO", param.sched_priority, status, errno );
#endif
  return false;
}


void
Thread::start( void )
{
  static bool isSet;
  if ( ! isSet ) {
#ifndef PTHREAD_CANCEL_DEFERRED
    static struct sigaction act;
    act.sa_handler = rai_thread_sigusr1_handler;
    ::sigaction( SIGUSR1, &act, NULL );
#endif
    static struct sigaction act2;
    act2.sa_handler = rai_thread_sigusr2_handler;
    ::sigaction( SIGUSR2, &act2, NULL );
    isSet = true;
  }
  rai_create_thread_keys();

  if ( this->isRunning )
    throw ThreadErr::getErr( ThreadErr::THREAD_RUNNING );

  if ( this->threadPtr == NULL ) {
    this->threadPtr = createThreadHandle();
  }
  this->threadPtr->threadId = 0;
  this->threadPtr->exitCode = NULL;
#if defined( __linux ) && defined( SET_PROFILER_ITIMER )
  /* linux pthreads doesn't automatically set the profile timer */
  ::getitimer( ITIMER_PROF, &this->threadPtr->itimer );
#endif

  pthread_attr_t attr;
  int status;
  ::pthread_attr_init( &attr );
#ifdef PTHREAD_SCOPE_SYSTEM
  status = ::pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );
  if ( status != 0 )
    throw ThreadErr::getErr( ThreadErr::THREAD_SCOPE );
#endif
#ifdef PTHREAD_STACK_MIN
  status = ::pthread_attr_setstacksize( &attr, (size_t) this->stack_size );
  if ( status != 0 )
    throw ThreadErr::getErr( ThreadErr::THREAD_STACK );
#endif
  this->isRunning = true;
  status = ::pthread_create( &this->threadPtr->threadId, &attr,
                             rai_start_thread_startRoutine, this );
  if ( status != 0 ) {
    int err = errno;
    this->isRunning = false;
    Error e = ThreadErr::getErr( ThreadErr::THREAD_START );
    logError( LERROR, e, "errno=%d/%s", err, strerror( err ) );
    throw e;
  }
  else {
    /* wait for thread to run() before returning */
    ::pthread_mutex_lock( &thrListLock );
    while ( this->isJoined )
      ::pthread_cond_wait( &thrListCond, &thrListLock );
    ::pthread_mutex_unlock( &thrListLock );
  }
}


void *
Thread::join( void )
{
  logDebug( LDEBUG, "Thread(%u) %s join", this->tid, this->name );
  if ( ! this->isJoined ) {
    if ( ::pthread_join( this->threadPtr->threadId,
                         &this->threadPtr->exitCode ) != 0 )
      throw ThreadErr::getErr( ThreadErr::THREAD_JOIN );
    this->isJoined = true;
  }
  logDebug( LDEBUG, "Thread(%u) %s join done", this->tid, this->name );
  if ( this->threadPtr == NULL )
    return NULL;
  return this->threadPtr->exitCode;
}


void
Thread::kill( void )
{
  logDebug( LDEBUG, "Thread(%u) %s kill", this->tid, this->name );
  if ( this->threadPtr != NULL ) {
  #ifndef PTHREAD_CANCEL_DEFERRED
    if ( ::pthread_kill( this->threadPtr->threadId, SIGUSR1 ) != 0 )
      throw ThreadErr::getErr( ThreadErr::THREAD_KILL );
  #else
    if ( ::pthread_cancel( this->threadPtr->threadId ) != 0 )
      throw ThreadErr::getErr( ThreadErr::THREAD_KILL );
  #endif
  }
  this->isRunning = false;
}


void
Thread::interrupt( void )
{
  logDebug( LDEBUG, "Thread(%u) %s interrupt", this->tid, this->name );
  if ( this->threadPtr != NULL ) {
    if ( ::pthread_kill( this->threadPtr->threadId, SIGUSR2 ) != 0 )
      throw ThreadErr::getErr( ThreadErr::THREAD_KILL );
  }
}


void
Thread::sleep( TimeMSecs ms )
{
  Time::sleepMillisecs( ms );
}


void
Thread::yield( void )
{
  ::sched_yield();
}


void
Thread::exit( void *exitCode )
{
  logDebug( LDEBUG, "Thread(%u) %s exiting", this->tid, this->name );
  this->threadPtr->exitCode = exitCode;
  this->threadPtr->stk_thr_name = NULL;
  this->isRunning = false;
  this->cleanup();
  logDebug( LDEBUG, "Thread(%u) %s done exit", this->tid, this->name );
  ::pthread_exit( exitCode );
}


unsigned int
Thread::createSpecificKey( void )
{
  pthread_key_t k;
  if ( ::pthread_key_create( &k, NULL ) != 0 )
    throw ThreadErr::getErr( ThreadErr::THREAD_CREATE_KEY );
  return k;
}


void
Thread::putSpecific( unsigned int key,  void *ptr )
{
  ::pthread_setspecific( key, ptr );
}


void *
Thread::getSpecific( unsigned int key )
{
  return ::pthread_getspecific( key );
}

#if 0
static void
rai_thread_on_exit( void *p )
{
  while ( p != NULL ) {
    ThreadOnExit *next = ((ThreadOnExit *) p)->next;
    try {
      ((ThreadOnExit *) p)->onExit();
    } catch ( ... ) {
    }
    p = next;
  }
}


void
Thread::onExit( ThreadOnExit *onEx )
{
  if ( threadExitKey == Thread::NIL_KEY ) {
    pthread_key_t k;
    ::pthread_key_create( &k, rai_thread_on_exit );
    threadExitKey = k;
  }
  ThreadOnExit *next = (ThreadOnExit *) ::pthread_getspecific( threadExitKey );
  for ( ThreadOnExit *p = next; p != NULL; p = p->next )
    if ( p == onEx )
      return;
  onEx->next = next;
  ::pthread_setspecific( threadExitKey, onEx ); 
}
#endif

Semaphore::Semaphore()
{
  this->semaphorePtr = NULL;
}


Semaphore::~Semaphore()
{
  if ( this->semaphorePtr != NULL ) {
    Error e = NULL;
    if ( sem_destroy( &this->semaphorePtr->semaphore ) != 0 ) {
      switch( errno ) {
        case EINVAL:
          e = ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE_VAL ); break;
        case ENOSYS:
          e = ThreadErr::getErr( ThreadErr::NO_SEMAPHORES ); break;
        case EBUSY:
          e = ThreadErr::getErr( ThreadErr::SEMAPHORE_BLOCKED ); break;
        default:
          e = ThreadErr::getErr( ThreadErr::UNK_SEMAPHORE_ERR ); break;
      }
      if ( e != NULL ) {
        logError( LERROR, e, "Unable to destroy semaphore" );
      }
    }
  }
}

void
Semaphore::operator delete( void *p )
{
  FREE( p );
}

Semaphore *
Semaphore::create( int value, SemaphoreType shared )
{
  Semaphore	* s;

  MALLOC( sizeof( Semaphore ) + sizeof( SemaphoreHandle ), &s );
  s->semaphorePtr = ( SemaphoreHandle *) &s[ 1 ];

  if( sem_init( &s->semaphorePtr->semaphore, shared, value ) != 0 ) {
    switch( errno ) {
    case EINVAL:
      throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE_VAL );
    case ENOSYS:
      throw ThreadErr::getErr( ThreadErr::NO_SEMAPHORES );
    case ENOSPC:
      throw ThreadErr::getErr( ThreadErr::NO_MORE_SEMAPHORES );
    case EPERM:
      throw ThreadErr::getErr( ThreadErr::NO_SEMAPHORE_PERM );
    default:
      throw ThreadErr::getErr( ThreadErr::UNK_SEMAPHORE_ERR );
    }
  }
  return s;
}

/** Return current value of semaphore */
int 
Semaphore::getValue( void )
{
  int val;

  if( ! this->semaphorePtr ) {
    throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE );
  }
  
  if( sem_getvalue( &this->semaphorePtr->semaphore, &val ) != 0 ) {
    switch( errno ) {
    case EINVAL:
      throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE );
    case ENOSYS:
      throw ThreadErr::getErr( ThreadErr::NO_SEMAPHORES );
    default:
      throw ThreadErr::getErr( ThreadErr::UNK_SEMAPHORE_ERR );
    }
  }
  return val;
}

/** Post semaphore */
void 
Semaphore::post( void )
{
  if( ! this->semaphorePtr ) {
    throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE );
  }
  if( sem_post( &this->semaphorePtr->semaphore ) != 0 ) {
    switch( errno ) {
    case ERANGE:
      /* FIXME - need define for solaris... */
#ifdef Solaris
    case: EOVERFLOW:
#endif
      throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE_VAL );
    case EINVAL:
      throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE );
    case ENOSYS:
      throw ThreadErr::getErr( ThreadErr::NO_SEMAPHORES );
    default:
      throw ThreadErr::getErr( ThreadErr::UNK_SEMAPHORE_ERR );
    }
  }
}

/** Try waiting for semaphore */
bool 
Semaphore::tryWait( void )
{
  if( ! this->semaphorePtr ) {
    throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE );
  }
  if( sem_trywait( &this->semaphorePtr->semaphore ) != 0 ) {
    switch( errno ) {
    case EINVAL:
      throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE );
    case ENOSYS:
      throw ThreadErr::getErr( ThreadErr::NO_SEMAPHORES );
    case EAGAIN:
    case EINTR:
      return false;	// could not lock semaphore
    case EDEADLK:
      throw ThreadErr::getErr( ThreadErr::SEMAPHORE_DEADLK );
    default:
      throw ThreadErr::getErr( ThreadErr::UNK_SEMAPHORE_ERR );      
    }
  }
  return true;
}

/** Block until semaphore is posted */
bool
Semaphore::wait( void )
{
  if( ! this->semaphorePtr ) {
    throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE );
  }
  if( sem_wait( &this->semaphorePtr->semaphore ) != 0 ) {
    switch( errno ) {
    case EINVAL:
      throw ThreadErr::getErr( ThreadErr::BAD_SEMAPHORE );
    case ENOSYS:
      throw ThreadErr::getErr( ThreadErr::NO_SEMAPHORES );
    case EINTR:
      return false;	// could not lock semaphore
    case EDEADLK:
      throw ThreadErr::getErr( ThreadErr::SEMAPHORE_DEADLK );
    default:
      throw ThreadErr::getErr( ThreadErr::UNK_SEMAPHORE_ERR );      
    }
  }
  return true;
}


#ifdef __linux
#include <sys/syscall.h>
#include <linux/unistd.h>

#ifndef __NR_sched_setaffinity
  #if defined( __i386 ) && ! defined( __amd64__ )
    #define __NR_sched_setaffinity  241
    #define __NR_sched_getaffinity  242
  #else
    #define __NR_sched_setaffinity  203
    #define __NR_sched_getaffinity  204
  #endif
#endif

#define __NR_rai_sch_setaffinity __NR_sched_setaffinity
#define __NR_rai_sch_getaffinity __NR_sched_getaffinity

#ifndef _syscall3
  #define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
  type name(type1 arg1,type2 arg2,type3 arg3) \
  {\
    return syscall(__NR_##name, arg1, arg2, arg3);\
  }
#endif

#ifndef MAX_ERRNO
 #define MAX_ERRNO 4095
#endif

_syscall3 (int, rai_sch_setaffinity, pid_t, pid, unsigned int, len, unsigned long *, user_mask_ptr)
_syscall3 (int, rai_sch_getaffinity, pid_t, pid, unsigned int, len, unsigned long *, user_mask_ptr)

static void
set_affinity( pid_t i,  unsigned int l,  unsigned long *p )
{
  rai_sch_setaffinity( i, l, p );
}

static void
get_affinity( pid_t i,  unsigned int l,  unsigned long *p )
{
  memset( p, 0, l );
  rai_sch_getaffinity( i, l, p );
}
#endif


bool
CpuAffinity::getAffinity( void )
{
  this->zero();
#ifdef __linux
  get_affinity( 0, sizeof( this->mask ), this->mask );
  return true;
#else
  return false;
#endif
}


bool
CpuAffinity::setAffinity( void )
{
#ifdef __linux
  set_affinity( 0, sizeof( this->mask ), this->mask );
  return true;
#else
  return false;
#endif
}


#endif /* posix, win32, no_sys_thread */


void
Thread::setName( const char *nm,  const char *nm2 )
{
  char         tmp[ sizeof( this->name ) ];
  unsigned int len,
               len2;

  if ( nm == NULL )
    len = 0;
  else {
    len = ::strlen( nm );
    if ( len > sizeof( tmp ) - 3 )
      len = sizeof( tmp ) - 3;
  }
  if ( nm2 == NULL )
    len2 = 0;
  else {
    len2 = ::strlen( nm2 );
    if ( len2 > sizeof( tmp ) - ( len + 2 ) )
      len2 = sizeof( tmp ) - ( len + 2 );
  }
  if ( len > 0 ) {
    ::memcpy( tmp, nm, len );
  }
  if ( len2 > 0 ) {
    if ( len > 0 )
      tmp[ len++ ] = '.';
    ::memcpy( &tmp[ len ], nm2, len2 );
  }
  len += len2;
  while ( len < sizeof( tmp ) )
    tmp[ len++ ] = '\0';
  ::memcpy( this->name, tmp, sizeof( tmp ) );

  if ( this->threadPtr != NULL && this->threadPtr->stk_thr_name != NULL )
    ::memcpy( this->threadPtr->stk_thr_name, this->name, sizeof( this->name ) );
}


Error
ThreadErr::getErr( unsigned int status )
{
  static const char     mod[] = "Thread";
  static const ErrorRec err[] = {
  /*  0 */ { NO_THREADS,          "Thread library not compiled into program",
                                  mod },
  /*  1 */ { MUTEX_CREATE,        "Unable to create mutex", mod },
  /*  2 */ { BAD_MUTEX_LOCK,      "Unable to lock mutex", mod },
  /*  3 */ { BAD_MUTEX_TRYLOCK,   "Unable to trylock mutex", mod },
  /*  4 */ { BAD_MUTEX_UNLOCK,    "Unable to unlock mutex", mod },
  /*  5 */ { COND_CREATE,         "Unable to create condition", mod },
  /*  6 */ { BAD_COND_WAIT,       "Unable to wait on condition", mod },
  /*  7 */ { BAD_COND_SIGNAL,     "Unable to signal condition", mod },
  /*  8 */ { BAD_COND_BROADCAST,  "Unable to broadcast to condition", mod },
  /*  9 */ { THREAD_START,        "Unable to start thread", mod },
  /* 10 */ { THREAD_JOIN,         "Unable to join thread", mod },
  /* 11 */ { THREAD_KILL,         "Unable to kill thread", mod },
  /* 12 */ { BAD_SEMAPHORE_VAL,   "Bad semaphore value", mod },
  /* 13 */ { NO_SEMAPHORES,       "Semaphores not support", mod },
  /* 14 */ { NO_MORE_SEMAPHORES,  "No more semaphores", mod },
  /* 15 */ { NO_SEMAPHORE_PERM,   "No permission for semaphore", mod },
  /* 16 */ { UNK_SEMAPHORE_ERR,   "Unknown semaphore error", mod },
  /* 17 */ { BAD_SEMAPHORE,       "Invalid semaphore", mod },
  /* 18 */ { SEMAPHORE_DEADLK,    "Semaphore deadlock", mod },
  /* 19 */ { SEMAPHORE_BLOCKED,   "Detroying semaphore while processs still waiting on it", mod },
  /* 20 */ { THREAD_SCOPE,        "Unable to set thread scope", mod },
  /* 21 */ { THREAD_STACK,        "Unable to set thread stack", mod },
  /* 22 */ { THREAD_SCHED_PRIO,   "Unable to set thread scheduling priority",
             mod },
  /* 23 */ { THREAD_SCHED_POLICY, "Unable to set thread scheduling policy",
             mod },
  /* 24 */ { THREAD_CREATE_KEY,   "Unable to create thread specific key", mod },
  /* 25 */ { THREAD_NO_LOCAL,     "No local alloc available", mod },
  /* 26 */ { THREAD_RUNNING,      "Thread already running and using class", mod},
  /* 27 */ { RWLOCK_RDLOCK,		"RWLock read lock failed", mod },
  /* 28 */ { RWLOCK_RDLOCK_EINVAL,	"RWLock read lock not initiallized", mod },
  /* 29 */ { RWLOCK_RDLOCK_EDEADLK,	"RWLock read lock already locked for writing", mod },
  /* 30 */ { RWLOCK_TRYRDLOCK,		"RWLock Try Read lock failed", mod },
  /* 31 */ { RWLOCK_TRYRDLOCK_EINVAL,	"RWLock Try Read lock not initiallized", mod },
  /* 32 */ { RWLOCK_TRYRDLOCK_EDEADLK,  "RWLock Try Read lock already locked for writing", mod },
  /* 33 */ { RWLOCK_WRLOCK,		"RWLock Write lock failed", mod },
  /* 34 */ { RWLOCK_WRLOCK_EINVAL,	"RWLock Write lock not initiallized", mod },
  /* 35 */ { RWLOCK_WRLOCK_EDEADLK,	"RWLock Write lock alread locked for reading or writing", mod },
  /* 36 */ { RWLOCK_TRYWRLOCK,		"RWLock Try Write lock failed", mod },
  /* 37 */ { RWLOCK_TRYWRLOCK_EINVAL,	"RWLock Try Write lock not initiallized", mod },
  /* 38 */ { RWLOCK_TRYWRLOCK_EDEADLK,	"RWLock Try Write lock alread locked for reading or writing", mod },
  /* 39 */ { RWLOCK_UNLOCK,		"RWLock unlock failed", mod },
  /* 40 */ { RWLOCK_UNLOCK_EINVAL,	"RWLock unlock not initiallized", mod },
  /* 41 */ { RWLOCK_UNLOCK_EPERM,	"RWLock unlock not owned by current thread", mod },
  /* 42 */ { 42,			"Unknown error", mod },
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}
