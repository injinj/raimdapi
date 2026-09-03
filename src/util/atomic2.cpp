#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include "util/atomic.h"

RAIBASE_DLL_EXP
unsigned int watomic_add_c( volatile unsigned int *val,
                           unsigned int addVal ) {
  return atomic_add_c( val, addVal );
}
RAIBASE_DLL_EXP
unsigned int watomic_xchg_c( volatile unsigned int *val,
                            unsigned int newVal ) {
  return atomic_xchg_c( val, newVal );
}
RAIBASE_DLL_EXP
unsigned long long watomic_addq_c( volatile unsigned long long *val,
                                  long long addVal ) {
  return atomic_addq_c( val, addVal );
}
RAIBASE_DLL_EXP
unsigned long long watomic_xchgq_c( volatile unsigned long long *val,
                                   unsigned long long newVal ) {
  return atomic_xchgq_c( val, newVal );
}
