/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if __GNUC__ >= 4
  #define HAS_GCC_ATOMIC_BUILTIN
#endif

/* like val++, returns old val, sets new val to += addVal */
unsigned int
atomic_add_c( volatile unsigned int *val,  unsigned int addVal )
{
#if defined( HAS_GCC_ATOMIC_BUILTIN )
  return __sync_fetch_and_add( val, addVal );
#else
#if ( defined( __i386 ) || defined( __amd64__ ) )
  __asm__ __volatile__ ( "pause\n\t"
                         "lock; xaddl %0, %1"
            : "=r" (addVal), "=m" (*val)
            : "0" (addVal) );
#else
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
#endif
  return addVal;
#endif
}

/* returns old val, sets new val to newVal */
unsigned int
atomic_xchg_c( volatile unsigned int *val, unsigned int newVal )
{
#if defined( HAS_GCC_ATOMIC_BUILTIN )
  unsigned int oldVal = *val;
  for ( ; ; oldVal = *val )
    if ( __sync_bool_compare_and_swap( val, oldVal, newVal ) )
      return oldVal;
#else
#if ( defined( __i386 ) || defined( __amd64__ ) )
  __asm__ __volatile__ ( "pause\n\t"
                         "lock; xchgl %0, %1"
            : "=r" (newVal), "=m" (*val)
            : "0" (newVal) );
#else
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
#endif
  return newVal;
#endif
}

/* like val++, returns old val, sets new val to += addVal */
unsigned long long
atomic_addq_c( volatile unsigned long long *val,  long long addVal )
{
#if defined( HAS_GCC_ATOMIC_BUILTIN )
  return __sync_fetch_and_add( val, addVal );
#else
#if defined( __i386 )
  register unsigned long long oldVal;
  unsigned long long newVal;
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
  } while ( (unsigned long long) addVal != oldVal );
#elif defined( __amd64__ )
  __asm__ __volatile__ ( "pause\n\t"
                         "lock; xaddq %0, %1"
            : "=r" (addVal), "=m" (*val)
            : "0" (addVal) );
#else
#error "atomic_addq_c"
#endif
  return addVal;
#endif
}

/* returns old val, sets new val to newVal */
unsigned long long
atomic_xchgq_c( volatile unsigned long long *val, unsigned long long newVal )
{
#if defined( HAS_GCC_ATOMIC_BUILTIN )
  unsigned long long oldVal = *val;
  for ( ; ; oldVal = *val )
    if ( __sync_bool_compare_and_swap( val, oldVal, newVal ) )
      return oldVal;
#else
#if defined( __i386 )
  register unsigned long long oldVal;
  unsigned long long newVal2 = newVal;
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
#elif defined( __amd64__ )
  __asm__ __volatile__ ( "pause\n\t"
                         "lock; xchgq %0, %1"
            : "=r" (newVal), "=m" (*val)
            : "0" (newVal) );
#else
#error "atomic_xchgq_c"
#endif
  return newVal;
#endif
}

