/* 
 * mmc.cc
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "clock.h"
#include MMC_H

#include <x86intrin.h>

template<idx_t M,idx_t N,idx_t K,idx_t lda,idx_t ldb>
static real comp_ij(matrix_c<M,K,lda>& A, matrix_c<K,N,ldb>& B,
                    idx_t i, idx_t j, long times) {
  real s = 0.0;
  //long K = A.nC;
  for (long t = 0; t < times; t++) {
    asm volatile("# comp_ij K loop begins");
    for (idx_t k = 0; k < K; k++) {
      s += A(i,k) * B(k,j);
    }
    asm volatile("# comp_ij K loop ends");
  }
  return s;
}

static char * wipe_cache(int x) {
  static char * a = 0;
  long n = 1000 * 1000 * 1000;
  if (!a) a = (char *)malloc(n);
  memset(a, x, n);
  return a;
}

int main(int argc, char ** argv) {
  long times = (argc > 1 ? atol(argv[1]) : 100000);
  long chk   = (argc > 2 ? atol(argv[2]) : 1);
  long seed  = (argc > 3 ? atol(argv[3]) : 76843802738543);

  const idx_t dM = 6;
  const idx_t dN = 2;
  const idx_t nV = dM * dN;     // 12
  const idx_t M = nV;           // 12
  const idx_t N = dN * L;       // 32 (AVX-512)
  const idx_t K = 192;
  const idx_t lda = K;
  const idx_t ldb = N;
  const idx_t ldc = N;
  
  assert(K <= lda);
  assert(N <= ldb);
  assert(N <= ldc);
  //assert(M % dM == 0);
  //assert(N % (dN * L) == 0);

  matrix_c<M,K,lda> A;
  matrix_c<K,N,ldb> B;
  matrix_c<M,N,ldc> C;

  unsigned short rg[3] = { (unsigned short)((seed >> 16) & 65535),
			   (unsigned short)((seed >> 8)  & 65535),
			   (unsigned short)((seed >> 0)  & 65535) };
  long fmas      = (long)M * (long)N * (long)K;
  long flops     = 2 * fmas;
  long flops_all = flops * times;
  A.rand_init(rg);
  B.rand_init(rg);
  C.zero();
  printf("M = %ld, N = %ld, K = %ld\n", (long)M, (long)N, (long)K);
  printf("A : %ld x %ld (ld=%ld) %ld bytes\n",
         (long)M, (long)K, (long)lda, (long)M * (long)K * sizeof(real));
  printf("B : %ld x %ld (ld=%ld) %ld bytes\n",
         (long)K, (long)N, (long)ldb, (long)K * (long)N * sizeof(real));
  printf("C : %ld x %ld (ld=%ld) %ld bytes\n",
         (long)M, (long)N, (long)ldc, (long)M * (long)N * sizeof(real));
  printf("total = %ld bytes\n",
	 ((long)M * (long)K + (long)K * (long)N + (long)M * (long)N) * sizeof(real));
  char * wipe = wipe_cache(0);
  printf("repeat : %ld times\n", times);
  printf("perform %ld flops ... ", flops_all); fflush(stdout);
  cpu_clock_counter_t cc = mk_cpu_clock_counter();

  long n_iters = 0;
  long long t0 = cur_time_ns();
  long long c0 = cpu_clock_counter_get(cc);
  long long r0 = rdtsc();
  for (long i = 0; i < times; i++) {
    n_iters += gemm<M,N,K,lda,ldb,ldc,nV,dN>(A, B, C);
  }
  long long r1 = rdtsc();
  long long c1 = cpu_clock_counter_get(cc);
  long long t1 = cur_time_ns();
  long long dr = r1 - r0;
  long long dc = c1 - c0;
  long long dt = t1 - t0;

  printf("done \n");
  printf("%lld CPU clocks\n", dc);
  printf("%lld REF clocks\n", dr);
  printf("%lld nano sec\n", dt);
  printf("%.3f CPU clocks/iter\n",  dc / (double)n_iters);
  printf("%.3f REF clocks/iter\n",  dr / (double)n_iters);
  printf("%.3f flops/CPU clock\n", flops_all / (double)dc);
  printf("%.3f flops/REF clock\n", flops_all / (double)dr);
  printf("%.3f GFLOPS\n",          flops_all / (double)dt);

  if (chk) {
    idx_t i = nrand48(rg) % M;
    idx_t j = nrand48(rg) % N;
    real s = comp_ij(A, B, i, j, times);
    printf("C(%ld,%ld) = %f, ans = %f, |C(%ld,%ld) - s| = %.9f\n",
	   (long)i, (long)j, C(i,j), s,
           (long)i, (long)j, fabs(C(i,j) - s));
  }
  if (wipe) free(wipe);
  cpu_clock_counter_destroy(cc);
  return 0;
}