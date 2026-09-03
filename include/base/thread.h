/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_base__thread_h__
#define __rai_base__thread_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

#ifndef __rai_base__time_h__
#include "base/time.h"
#endif

namespace rai {

struct MutexHandle;
struct RwLockHandle;
struct ConditionHandle;
struct ThreadHandle;
struct SemaphoreHandle;
class Condition;
class StartThread;
class Thread;

class RAIBASE_DLL_EXP Mutex {
  private:
    MutexHandle *mutexPtr;

    friend class Condition;
    Mutex() {};
  public:
/*#define MUTEX_OWNER  causes mutex->lock() to put itself at owner */
  #ifdef MUTEX_OWNER
    Thread * owner;
  #endif
    ~Mutex();
    void operator delete( void *p );

    enum MutexType {
      FAST_LOCK      = 0,
      RECURSIVE_LOCK = 1
    };
    static Mutex *create( MutexType isRecursiveLock = FAST_LOCK )
                                                               throw( Error );
    void lock( void )                                          throw( Error );

    bool tryLock( void )                                       throw( Error );

    void unlock( void )                                        throw( Error );
};

class RAIBASE_DLL_EXP RwLock {
 private:
  RwLockHandle *mutexPtr;
  
  RwLock() {};
 public:
  /*#define MUTEX_OWNER  causes mutex->lock() to put itself at owner */
#ifdef MUTEX_OWNER
  Thread * owner;
#endif
  ~RwLock();
  void operator delete( void *p );
  
  static RwLock *create( )                                     throw( Error );
  
  void rdLock( void )                                          throw( Error );
  
  bool tryRdLock( void )                                       throw( Error );
  
  void wrLock( void )                                          throw( Error );
  
  bool tryWrLock( void )                                       throw( Error );
  
  void unlock( void )                                          throw( Error );
};

class RAIBASE_DLL_EXP Condition {
  private:
    ConditionHandle *conditionPtr;

    Condition() {};
  public:
    ~Condition();
    void operator delete( void *p );

    static Condition *create( void )                           throw( Error );

    void wait( Mutex *mutex )                                  throw( Error );
    /* wait for milliseconds */
    bool timedWait( Mutex *mutex,  TimeMSecs howLong )         throw( Error );
    /* wait for milliseconds with fractions */
    bool timedWait( Mutex *mutex,  double howLong )            throw( Error );
    /* wait for uint milliseconds */
    bool timedWait( Mutex *mutex,  unsigned int howLong ) throw( Error ) {
      return this->timedWait( mutex, (TimeMSecs) howLong );
    };
    /* wait for int milliseconds */
    bool timedWait( Mutex *mutex,  int howLong ) throw( Error ) {
      return this->timedWait( mutex, (TimeMSecs) howLong );
    };
    void signal( void )                                        throw( Error );

    void broadcast( void )                                     throw( Error );
};

class ThrMBuf;

struct RAIBASE_DLL_EXP ThreadOnExit {
  ThreadOnExit * next;

  ThreadOnExit() : next( 0 ) {}

  virtual ~ThreadOnExit() {}

  virtual void onExit( void ) = 0;
};

class RAIBASE_DLL_EXP Thread {
 public:
    static const unsigned int NIL_KEY = 0xffffffffU;
    enum SchedPolicy {
      THREAD_SCHED_OTHER       = 0,     /* OS Defined */
      THREAD_SCHED_RR          = 1,     /* Round Robin */
      THREAD_SCHED_FIFO        = 2      /* First-in First-out */
    };

    struct ListRec {
      virtual ~ListRec() {};

      virtual bool onThread( Thread &thr ) = 0; /* return true to continue */
    };

    template <unsigned int NTASKS>
    struct CpuListT : public ListRec {
      TimeMSecs    tuser[ NTASKS ], tsys[ NTASKS ];
      unsigned int task[ NTASKS ], n, count;
      char         name[ NTASKS ][ 80 ];
      TimeMSecs    sysTime, userTime, totalTime;

      CpuListT() {
        this->init();
      }
      void init( void ) {
        this->n              = 0;
        this->count          = 0;
        this->tuser[ 0 ]     = 0;
        this->tsys[ 0 ]      = 0;
        this->task[ 0 ]      = 0;
        this->name[ 0 ][ 0 ] = '\0';
        this->sysTime        = 0;
        this->userTime       = 0;
        this->totalTime      = 0;
      }
      virtual ~CpuListT() {}
      virtual bool onThread( Thread &thr ) {
        unsigned int i, j, id = thr.getThreadId();
        for ( i = 0; i < this->n && i < NTASKS; i++ )
          if ( this->task[ i ] == id ) {
            for ( j = 0; j < sizeof( this->name[ i ] ) - 1 &&
                         thr.name[ j ] != '\0'; j++ )
              this->name[ i ][ j ] = thr.name[ j ];
            this->name[ i ][ j ] = '\0';
            this->count++;
            break;
          }
        return true;
      }
      void getRusage( void ) {
        unsigned int i, j, pid = 0;
        this->init();
        this->n = Time::getRusage( this->sysTime, this->userTime,
                                   this->totalTime, this->tsys, this->tuser,
                                   this->task, NTASKS );
        for ( i = 0; i < this->n && i < NTASKS; i++ )
          this->name[ i ][ 0 ] = 0;
        Thread::recurseAll( *this );
        for ( i = 0; i < this->n && i < NTASKS; i++ )
          if ( this->name[ i ][ 0 ] == 0 ) {
            if ( pid == 0 )
              pid = Thread::getProcessId();
            const char *s = ( this->task[ i ] == pid ? "main" : "unnamed" );
            for ( j = 0; s[ j ] != '\0'; j++ )
              this->name[ i ][ j ] = s[ j ];
            this->name[ i ][ j ] = 0;
          }
      }
    };

  private:
    ThreadHandle        * threadPtr;
    bool                  isRunning,
                          isJoined,
                          inThrList;
    unsigned int          stack_size;
    int                   sched_prio;
    unsigned int          tid;         /* OS thread ID. */
    SchedPolicy           sched_policy; 
    friend class StartThread;

    Thread              * tnext,
                        * tlast;
    /*static Thread       * hd,
                        * tl;*/

    void addThrList( void );

    void removeThrList( void );

    static void recurseThrList( ListRec &r );

  protected:
    void exit( void *exitCode = NULL );

    virtual void run( void ) = 0;
  public:
    char name[ 80 ];

    static const unsigned int DEFAULT_STACK_SIZE = 2 * 512 * 1024;

    static SchedPolicy  defaultSched;     /* THREAD_SCHED_OTHER */
    static unsigned int defaultPrio;      /* 0 */
    static unsigned int defaultStackSize; /* DEFAULT_STACK_SIZE */

    Thread( const char *nm,
            unsigned int stack_size = rai::Thread::defaultStackSize,
            int sched_priority = rai::Thread::defaultPrio, /* 1=high -> 10=low*/
            SchedPolicy sched_policy = rai::Thread::defaultSched );
    virtual ~Thread();

    void start( void )                                         throw( Error );

    void *join( void )                                         throw( Error );

    void kill( void )                                          throw( Error );

    void interrupt( void )                                     throw( Error );

    void setName( const char *nm1,  const char *nm2 = NULL );

    static bool createExternalThread( const char *name )       throw( Error );

    static void stopExternalThread( void )                     throw( Error );

    bool setPriority( int sched_priority,  SchedPolicy sched_policy );

    static Thread *self( void );

    ThrMBuf *localThrMBuf( void )                              throw( Error );

    ThrMBuf *createThrMBuf( void )                             throw( Error );

    static void localAlloc( size_t sz,  void *ptr,
                         size_t alignment = sizeof( void * ) ) throw( Error );
    static void heapAlloc( size_t sz,  void *ptr,
                         size_t alignment = sizeof( void * ) ) throw( Error );
    static void localAlloc( ThrMBuf *mbuf,  size_t sz,  void *ptr,
                         size_t alignment = sizeof( void * ) ) throw( Error );
    //#define localFree( p ) localFree2( p, __FILE__, __LINE__ )
    static void localFree( void *ptr );

    static unsigned int createSpecificKey( void )              throw( Error );

    static void putSpecific( unsigned int key,  void *ptr )    throw( Error );

    static void *getSpecific( unsigned int key )               throw( Error );

    static void sleep( TimeMSecs ms );

    static void yield( void );

    bool isThreadRunning( void ) {
      return this->isRunning;
    }
    bool isThreadJoined( void ) {
      return this->isJoined;
    }
    static unsigned int getProcessId( void );

    /* get OS thread ID - static for use outside Thread */
    static unsigned int getOSThreadId();

    /* get OS thread ID. linux thread: TID, Solaris LWP */
    inline unsigned int getThreadId( void ) { return tid; };
#if defined( _WIN32 ) || defined( _WIN64 )
    /* get Windows thread HANDLE */
    void getThreadHandle( void *hndl );
#endif
    static void recurseAll( ListRec &r );

    static void onExit( ThreadOnExit *onEx );

    /* Thread::exit() calls this.
     * Should call the parent cleanup() if overridden, this frees thread local
     * storage and removes from thread list */
    virtual void cleanup( void );

    static void disableCancelState( void );

    static void enableCancelState( void );
};

class RAIBASE_DLL_EXP Semaphore {
  private:
    SemaphoreHandle *semaphorePtr;

    Semaphore();
  public:
    ~Semaphore();
    void operator delete( void *p );

    enum SemaphoreType {
      PROCESS_LOCAL	= 0,	/** Local to current process */
      PROCESS_SHARED	= 1	/** Shared between processes */
    };
    static Semaphore *create( int value = 0, SemaphoreType shared = PROCESS_LOCAL )
                                                               throw( Error );

    /** Return current value of semaphore */
    int getValue( void )	                               throw( Error );
    /** Post semaphore */
    void post( void )                                          throw( Error );
    /** Try waiting for semaphore - return true is sucessful */
    bool tryWait( void )                                       throw( Error );
    /** Block until semaphore is posted - return true is sucessful */
    bool wait( void )                                          throw( Error );
};

class RAIBASE_DLL_EXP CpuAffinity {
  public: 
    static const unsigned int MAX_CPU_AFFINITY = 256; /* multiple of long bits*/
    static const unsigned int AFF_LONG_BITS    = sizeof( long ) * 8;
    static const unsigned int AFF_LONG_SIZE    = MAX_CPU_AFFINITY/AFF_LONG_BITS;
    static const unsigned int AFF_LONG_SHIFT   = sizeof( long ) == 4 ? 5 : 6;

    unsigned long mask[ AFF_LONG_SIZE ];

    bool getAffinity( void );

    bool setAffinity( void );

    void zero( void ) {
      for ( unsigned int i = 0; i < AFF_LONG_SIZE; i++ )
        this->mask[ i ] = 0;
    }
    void set( unsigned int i ) {
      this->mask[ i >> AFF_LONG_SHIFT ] |=
         ( 1UL << ( i & ( AFF_LONG_BITS - 1 ) ) );
    }
    bool isSet( unsigned int i ) {
      return ( this->mask[ i >> AFF_LONG_SHIFT ] &
               ( 1UL << ( i & ( AFF_LONG_BITS - 1 ) ) ) ) != 0;
    }
    bool first( unsigned int &i ) {
      i = 0;
      do {
        if ( this->isSet( i ) )
          return true;
      } while ( ++i < MAX_CPU_AFFINITY );
      return false;
    }
    bool next( unsigned int &i ) {
      while ( ++i < MAX_CPU_AFFINITY ) {
        if ( this->isSet( i ) )
          return true;
      }
      return false;
    }
};

// Based on Sun Multithreaded Programming Guide
struct RAIBASE_DLL_EXP Barrier {
  int             maxCnt;       // max number of runners
  struct _sb {
    Condition   * cond;         // condition var for waiters at barrier
    Mutex       * lock;         // mutex for waiters at barrier
    int           runners;      // number of running threads
  } sb[2];
  struct _sb    * sbp;          // Current sub-barrier
  
  SYS_OPS( Barrier );
  Barrier( int count ) {        // Barrier with 'count' threads
    maxCnt      = count;
    sbp         = &sb[0];
    for( int i = 0; i < 2; i++ ) {
      sb[ i ].runners   = count;
      sb[ i ].lock      = Mutex::create();
      sb[ i ].cond      = Condition::create();
    }
  }

  // re-init existing barrier. Assumes there are no runners waiting
  inline void init( int count ) { maxCnt = count; sb[ 0 ].runners = count; sb[ 1 ].runners = count; };

  // wait for all runners to reach barrier
  inline void wait() {
    sbp->lock->lock();
    struct _sb *curSbp = sbp;
    if( curSbp->runners == 1 ) {   // last thread to reach barrier
      //       Sys::out->printf( "Barrier::wait last runner. Sub-barrier %d\n",
      //                         curSbp == &sb[0] ? 0 : 1 );
      //       Sys::out->flush();
      if( maxCnt != 1 ) {       // reset runner count and switch sub-bariers
        curSbp->runners = maxCnt;
        curSbp->cond->broadcast();
        sbp = sbp == &sb[0] ? &sb[1] : &sb[0];
      }
    } else {
      //       Sys::out->printf( "Barrier::wait runners: %d Sub-barrier %d\n",
      //                         sbp->runners, sbp == &sb[0] ? 0 : 1 );
      curSbp->runners--;
      while( curSbp->runners != maxCnt ) {
        //         Sys::out->printf( "Barrier::wait waiting runners: %d Sub-barrier %d\n",
        //                         curSbp->runners, curSbp == &sb[0] ? 0 : 1 );
        //         Sys::out->flush();
        curSbp->cond->wait( curSbp->lock );
        //         Sys::out->printf( "Barrier::wait woke up. runners: %d Sub-barrier %d\n",
        //                         curSbp->runners, curSbp == &sb[0] ? 0 : 1 );
        //         Sys::out->flush();
      }
    }
    curSbp->lock->unlock();
  }

  ~Barrier() {
    for( int i = 0; i < 2; i++ ) {
      if( sb[ i ].lock ) {
        delete sb[ i ].lock;
      }
      if( sb[ i ].cond ) {
        delete sb[ i ].cond;
      }
    }
  }  
};

namespace ThreadErr {
  enum {
    NO_THREADS          = 0,
    MUTEX_CREATE        = 1,
    BAD_MUTEX_LOCK      = 2,
    BAD_MUTEX_TRYLOCK   = 3,
    BAD_MUTEX_UNLOCK    = 4,
    COND_CREATE         = 5,
    BAD_COND_WAIT       = 6,
    BAD_COND_SIGNAL     = 7,
    BAD_COND_BROADCAST  = 8,
    THREAD_START        = 9,
    THREAD_JOIN         = 10,
    THREAD_KILL         = 11,
    BAD_SEMAPHORE_VAL   = 12,
    NO_SEMAPHORES       = 13,
    NO_MORE_SEMAPHORES  = 14,
    NO_SEMAPHORE_PERM   = 15,
    UNK_SEMAPHORE_ERR   = 16,
    BAD_SEMAPHORE       = 17,
    SEMAPHORE_DEADLK    = 18,
    SEMAPHORE_BLOCKED   = 19,
    THREAD_SCOPE        = 20,
    THREAD_STACK        = 21,
    THREAD_SCHED_PRIO   = 22,
    THREAD_SCHED_POLICY = 23,
    THREAD_CREATE_KEY   = 24,
    THREAD_NO_LOCAL     = 25,
    THREAD_RUNNING      = 26,
    RWLOCK_RDLOCK		= 27,
    RWLOCK_RDLOCK_EINVAL	= 28,
    RWLOCK_RDLOCK_EDEADLK	= 29,
    RWLOCK_TRYRDLOCK            = 30,
    RWLOCK_TRYRDLOCK_EINVAL	= 31,
    RWLOCK_TRYRDLOCK_EDEADLK	= 32,
    RWLOCK_WRLOCK		= 33,
    RWLOCK_WRLOCK_EINVAL	= 34,
    RWLOCK_WRLOCK_EDEADLK	= 35,
    RWLOCK_TRYWRLOCK		= 36,
    RWLOCK_TRYWRLOCK_EINVAL	= 37,
    RWLOCK_TRYWRLOCK_EDEADLK	= 38,
    RWLOCK_UNLOCK		= 39,
    RWLOCK_UNLOCK_EINVAL	= 40,
    RWLOCK_UNLOCK_EPERM		= 41,
  };
  RAIBASE_DLL_EXP
  Error getErr( unsigned int status );
}

} // namespace rai

#endif
