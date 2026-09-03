#ifndef __rai_util__test_cpu_feature_h__
#define __rai_util__test_cpu_feature_h__

 namespace rai {
 enum CpuFeature {
 /* features_edx */
 CPU_FEATURE_FPU = 0,  CPU_FEATURE_VME = 1,  CPU_FEATURE_DE = 2,
 CPU_FEATURE_PSE = 3,  CPU_FEATURE_TSC = 4,  CPU_FEATURE_MSR = 5,
 CPU_FEATURE_PAE = 6,  CPU_FEATURE_MCE = 7,  CPU_FEATURE_CX8 = 8,
 CPU_FEATURE_APIC = 9,           /* = 10,*/  CPU_FEATURE_SEP = 11,
 CPU_FEATURE_MTRR = 12, CPU_FEATURE_PGE = 13,  CPU_FEATURE_MCA = 14,
 CPU_FEATURE_CMOV = 15, CPU_FEATURE_PAT = 16,  CPU_FEATURE_PSE36 = 17,
 CPU_FEATURE_PSN = 18,  CPU_FEATURE_CLF = 19,            /* = 20, */
 CPU_FEATURE_DTES = 21, CPU_FEATURE_ACPI = 22, CPU_FEATURE_MMX = 23,
 CPU_FEATURE_FXSR = 24, CPU_FEATURE_SSE = 25,  CPU_FEATURE_SSE2 = 26,
 CPU_FEATURE_SS = 27,   CPU_FEATURE_HTT = 28,  CPU_FEATURE_TM1 = 29,
 CPU_FEATURE_IA64 = 30, CPU_FEATURE_PBE = 31,
 /* features_ecx */
 CPU_FEATURE_SSE3 = 32,    CPU_FEATURE_PCLMUL = 33, CPU_FEATURE_DTES64 = 34,
 CPU_FEATURE_MONITOR = 35, CPU_FEATURE_DS_CPL = 36, CPU_FEATURE_VMX = 37,
 CPU_FEATURE_SMX = 38,     CPU_FEATURE_EST = 39,    CPU_FEATURE_TM2 = 40,
 CPU_FEATURE_SSSE3 = 41,   CPU_FEATURE_CID = 42, /* = 43, */
 CPU_FEATURE_FMA = 44,     CPU_FEATURE_CX16 = 45,   CPU_FEATURE_ETPRD = 46,
 CPU_FEATURE_PDCM = 47,    /* = 48 */            /* = 49 */
 CPU_FEATURE_DCA = 50,     CPU_FEATURE_SSE4_1 = 51, CPU_FEATURE_SSE4_2 = 52,
 CPU_FEATURE_x2APIC = 53,  CPU_FEATURE_MOVBE = 54,  CPU_FEATURE_POPCNT = 55,
               /* = 56, */ CPU_FEATURE_AES = 57,    CPU_FEATURE_XSAVE = 58,
 CPU_FEATURE_OSXSAVE = 59, CPU_FEATURE_AVX = 60
};

static inline bool
test_cpu_feature( int name )
{
#if defined( __linux ) && ( defined( __amd64__ ) || defined( __i386 ) )
  int eax, ecx, edx;
  /* the cpu id, which is usually physical cpu or core unless hyperthreading */
#if defined( __amd64__ )
  __asm__ __volatile__ ( "cpuid"
                         : "=a" (eax), "=c" (ecx), "=d" (edx)
                         : "0" (1) : "ebx" );
#else
  __asm__ __volatile__ ( "pushl %%ebx\n\t"
                         "cpuid\n\t"
                         "popl %%ebx"
                         : "=a" (eax), "=c" (ecx), "=d" (edx)
                         : "0" (1) : );
#endif
  if ( name > 31 )
    return ( ecx & ( 1U << ( name - 32 ) ) ) != 0;
  return ( edx & ( 1U << name ) ) != 0;
#else
  return false;
#endif
}

} // namespace rai

#if 0
int
main( int argc, char *argv[] )
{
  if ( test_cpu_feature( CPU_FEATURE_CX8 ) )
    printf( "has cx8\n" );
  if ( test_cpu_feature( CPU_FEATURE_CX16 ) )
    printf( "has cx16\n" );
  return 0;
}
#endif
#endif
