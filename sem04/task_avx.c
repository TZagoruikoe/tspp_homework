#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#ifndef DIM
#define DIM 4
#endif

static double rtclock() {
    struct timeval Tp;
    int stat;

    stat = gettimeofday(&Tp, NULL);
    if (stat != 0)
        printf ("Error return from gettimeofday: %d", stat);

    return (Tp.tv_sec + Tp.tv_usec * 1.0e-6);
}

void fill_matrx(int N, double* matrx) {
    srand(time(NULL));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            matrx[i * N + j] = rand() % 1000 * 0.001;
        }
    }

}

void matmul_avx(double* A, double* B, double* C, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C[i * N + j] = 0.0;
        }
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j += 4) {
            __m256d c = _mm256_loadu_pd(&C[i * N + j]);

            for (int k = 0; k < N; k++) {
                __m256d a = _mm256_set1_pd(A[i * N + k]);
                __m256d b = _mm256_loadu_pd(&B[k * N + j]);
                c = _mm256_fmadd_pd(a, b, c);
            }

            _mm256_storeu_pd(&C[i * N + j], c);
        }
    }
}

int main() {
    double A[DIM * DIM], B[DIM * DIM], C[DIM * DIM];
    double bench_t_start, bench_t_end;

    fill_matrx(DIM, A);
    fill_matrx(DIM, B);

    bench_t_start = rtclock();
    matmul_avx(A, B, C, DIM);
    bench_t_end = rtclock();

    printf("Result matrix C:\n");
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            printf("%f ", C[i * DIM + j]);
        }
        printf("\n");
    }

    printf("Elapsed time: %lf\n", bench_t_end - bench_t_start);

    return 0;
}