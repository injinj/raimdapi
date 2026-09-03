/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__atomic_h__
#define __rai_util__atomic_h__

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
#undef byte
#include <windows.h>
#endif

#define A_UINT  unsigned int
#define A_INT   int
#define A_ULONG ullong
#define A_LONG  llong

#if defined( __GNUC__ )
  #if __GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ > 3 )
    #define HAS_GCC_ATOMIC_BUILTIN
  #endif
#endif

#if defined( __GNUC__ ) || defined( __ICC )
  #define HAS_GCC_ASM
#else
  /* vc8 win32 doesn't have x64 atomics */
  #if defined( __SUNPRO_CC ) || ( defined( _WIN32 ) && ! defined( _WIN64 ) )
    extern "C" {
      unsigned int atomic_add_c( volatile unsigned int *val,
                                 unsigned int addVal );
      unsigned int atomic_xchg_c( volatile unsigned int *val,
                                  unsigned int newVal );
      unsigned long long atomic_addq_c( volatile unsigned long long *val,
                                        long long addVal );
      unsigned long long atomic_xchgq_c( volatile unsigned long long *val,
                                         unsigned long long newVal );
    #if defined( _WIN32 )
      RAIBASE_DLL_EXP
      unsigned int watomic_add_c( volatile unsigned int *val,
                                 unsigned int addVal );
      RAIBASE_DLL_EXP
      unsigned int watomic_xchg_c( volatile unsigned int *val,
                                  unsigned int newVal );
      RAIBASE_DLL_EXP
      unsigned long long watomic_addq_c( volatile unsigned long long *val,
                                        long long addVal );
      RAIBASE_DLL_EXP
      unsigned long long watomic_xchgq_c( volatile unsigned long long *val,
                                         unsigned long long newVal );
    #endif
    }
    #define HAS_ATOMIC_C
  #endif
#endif

namespace rai {
struct AtomicUInt {
  volatile A_UINT val;

  void init( A_UINT v = 0 ) {
    this->val = v;
  }
  inline A_UINT add( A_INT addVal ) {
    return AtomicUInt::addv( &this->val, addVal );
  }
  inline A_UINT xchg( A_UINT newVal ) {
    return AtomicUInt::xchgv( &this->val, newVal );
  }
  /* like val++, returns old val, sets new val to += addVal */
  static inline A_UINT addv( volatile A_UINT *val,  A_INT addVal ) {
#if defined( __NEVER__ )
    A_UINT oldVal = *val; *val += addVal; return oldVal;

#elif defined( HAS_GCC_ATOMIC_BUILTIN )
    return __sync_fetch_and_add( val, addVal );

#elif defined( _WIN32 ) || defined( _WIN64 )
    return InterlockedExchangeAdd( (volatile LONG *) val, addVal );

#elif ( defined( __i386 ) || defined( __amd64__ ) ) && defined( HAS_GCC_ASM )
    __asm__ __volatile__ ( "lock; xaddl %0, %1"
              : "=r" (addVal), "=m" (*val)
              : "0" (addVal) );
    return addVal;
                              /* actually v8plus */
#elif ( defined( __sparcv9 ) || defined( __sparcv8 ) ) && defined( HAS_GCC_ASM )
  /* l0 = val; 
     do { val = *ptr; l2 = l0 + val; } while ( cas( ptr, val, l2 ) != val ); */
    __asm__ __volatile__(
                   "membar #Lookaside | #LoadLoad | #LoadStore | #StoreLoad\n\t"
                             "mov %0, %%l0\n\t"
                          "1: ld [%2], %0\n\t"
                             "add %%l0, %0, %%l2\n\t"
                             "cas [%2], %0, %%l2\n\t"
                             "cmp %0, %%l2\n\t"
                             "bne 1b\n\t"
                             "nop\n\t"
                   "membar #Lookaside | #LoadLoad | #LoadStore | #StoreLoad"
              : "=&r" (addVal)
              : "0" (addVal), "r" (val)
              : "memory", "%l0", "%l2" );
    return addVal;
#elif defined( HAS_ATOMIC_C )
    return atomic_add_c( val, addVal );

#else
#error "AtomicUInt.add"
#endif
  }

  /* returns old val, sets new val to newVal */
  static inline A_UINT xchgv( volatile A_UINT *val,  A_UINT newVal ) {
#if defined( __NEVER__ )
    A_UINT oldVal = *val; *val = newVal; return oldVal;

#elif defined( HAS_GCC_ATOMIC_BUILTIN )
    for ( A_UINT oldVal = *val; ; oldVal = *val )
      if ( __sync_bool_compare_and_swap( val, oldVal, newVal ) )
        return oldVal;

#elif defined( _WIN32 ) || defined( _WIN64 )
    return InterlockedExchange( (volatile LONG *) val, newVal );

#elif ( defined( __i386 ) || defined( __amd64__ ) ) && defined( HAS_GCC_ASM )
    __asm__ __volatile__ ( "pause\n\t"
                           "lock; xchgl %0, %1"
              : "=r" (newVal), "=m" (*val)
              : "0" (newVal) );
    return newVal;

#elif ( defined( __sparcv9 ) || defined( __sparcv8 ) ) && defined( HAS_GCC_ASM )
    /* do { l0 = *ptr; } while ( cas( ptr, l0, val ) != val ); */
    __asm__ __volatile__(
                   "membar #Lookaside | #LoadLoad | #LoadStore | #StoreLoad\n\t"
                             "mov %0, %%l2\n\t"
                          "1: ld [%2], %%l0\n\t"
                             "mov %%l2, %0\n\t"
                             "cas [%2], %%l0, %0\n\t"
                             "cmp %%l0, %0\n\t"
                             "bne 1b\n\t"
                             "nop\n\t"
                   "membar #Lookaside | #LoadLoad | #LoadStore | #StoreLoad"
              : "=&r" (newVal)
              : "0" (newVal), "r" (val)
              : "memory", "%l0", "%l2" );
    return newVal;
#elif defined( HAS_ATOMIC_C )
    return atomic_xchg_c( val, newVal );

#else
#error "AtomicUInt.xchg"
#endif
  }
};

struct AtomicULong {
  volatile A_ULONG val;

  void init( A_ULONG v = 0 ) {
    this->val = v;
  }
  inline A_ULONG add( A_LONG addVal ) {
    return AtomicULong::addv( &this->val, addVal );
  }
  inline A_ULONG xchg( A_ULONG newVal ) {
    return AtomicULong::xchgv( &this->val, newVal );
  }

  /* like val++, returns old val, sets new val to += addVal */
  static inline A_ULONG addv( volatile A_ULONG *val,  A_LONG addVal ) {
#if defined( __NEVER__ )
    A_ULONG oldVal = *val; *val += addVal; return oldVal;

#elif defined( HAS_GCC_ATOMIC_BUILTIN )
    return __sync_fetch_and_add( val, addVal );

#elif defined( _WIN64 )
    return InterlockedExchangeAdd64( (volatile LONGLONG *) val, addVal );

#elif defined( _WIN32 )
    return watomic_addq_c( val, addVal );

#elif defined( __amd64__ ) && defined( HAS_GCC_ASM )
    __asm__ __volatile__ ( "lock; xaddq %0, %1"
              : "=r" (addVal), "=m" (*val)
              : "0" (addVal) );
    return addVal;

#elif defined( __i386 ) && defined( HAS_GCC_ASM )
  register A_ULONG oldVal;
  A_LONG newVal;
  do {
    oldVal = *val; newVal = oldVal + addVal;
    __asm__ __volatile__("push %%ebx\n\t"
                         "movl (%3), %%ebx\n\t"
                         "movl 4(%3), %%ecx\n\t"
                         "lock; cmpxchg8b (%1)\n\t"
                         "pop %%ebx"
                         : "=A" (addVal)
                         : "D" (val),
                           "0" (oldVal),
                           "S" (&newVal)
                         : "memory", "%ecx");
  } while ( (A_ULONG) addVal != oldVal );
  return addVal;
                              /* actually v8plus */
#elif ( defined( __sparcv9 ) || defined( __sparcv8 ) ) && defined( HAS_GCC_ASM )
#warning "Dummy AtomicULong.addv"
  A_ULONG oldVal = *val; *val += addVal; return oldVal;

#elif defined( HAS_ATOMIC_C )
    return atomic_addq_c( val, addVal );

#else
#error "AtomicULong.add"
#endif
  }

  /* returns old val, sets new val to newVal */
  static inline A_ULONG xchgv( volatile A_ULONG *val,  A_ULONG newVal ) {
#if defined( __NEVER__ )
    A_ULONG oldVal = *val; *val = newVal; return oldVal;

#elif defined( HAS_GCC_ATOMIC_BUILTIN )
    for ( A_ULONG oldVal = *val; ; oldVal = *val )
      if ( __sync_bool_compare_and_swap( val, oldVal, newVal ) )
        return oldVal;

#elif  defined( _WIN64 )
    return InterlockedExchange64( (volatile LONGLONG *) val, newVal );

#elif defined( _WIN32 )
    return watomic_xchgq_c( val, newVal );

#elif defined( __amd64__ ) && defined( HAS_GCC_ASM )
    __asm__ __volatile__ ( "pause\n\t"
                           "lock; xchgq %0, %1"
              : "=r" (newVal), "=m" (*val)
              : "0" (newVal) );
    return newVal;

#elif defined( __i386 ) && defined( HAS_GCC_ASM )
  register A_ULONG oldVal;
  A_LONG newVal2 = newVal;
  do {
    oldVal = *val;
    __asm__ __volatile__("push %%ebx\n\t"
                         "movl (%3), %%ebx\n\t"
                         "movl 4(%3), %%ecx\n\t"
                         "lock; cmpxchg8b (%1)\n\t"
                         "pop %%ebx"
                         : "=A" (newVal)
                         : "D" (val),
                           "0" (oldVal),
                           "S" (&newVal2)
                         : "memory", "%ecx");
  } while ( newVal != oldVal );
  return newVal;

#elif ( defined( __sparcv9 ) || defined( __sparcv8 ) ) && defined( HAS_GCC_ASM )
#warning "Dummy AtomicULong.xchg"
  A_ULONG oldVal = *val; *val = newVal; return oldVal;

#elif defined( HAS_ATOMIC_C )
    return atomic_xchgq_c( val, newVal );

#else
#error "AtomicULong.xchg"
#endif
  }
};
#if 0
#endif
} // namespace rai

#undef A_UINT
#undef A_INT
#undef A_ULONG
#undef A_LONG

#endif
