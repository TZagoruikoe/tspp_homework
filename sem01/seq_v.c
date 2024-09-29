#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

static double rtclock() {
    struct timeval Tp;
    int stat;
    
    stat = gettimeofday(&Tp, NULL);
    if (stat != 0)
      printf ("Error return from gettimeofday: %d", stat);
    
    return (Tp.tv_sec + Tp.tv_usec * 1.0e-6);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("ERROR: Incorrect arguments number.\n");
        exit(1);
    }

    double bench_t_start, bench_t_end;
    int count = atoi(argv[1]);
    //int threads_count = atoi(argv[2]);
    double step = 1./count;
    double sum = 0;

    bench_t_start = rtclock();

    for (int i = 0; i < count; i++) {
        sum += step * 4 / (1 + (i * step + step / 2) * (i * step + step / 2));
    }

    bench_t_end = rtclock();

    printf("%lf\n", sum);
    printf("Elapsed time: %lf s\n", bench_t_end - bench_t_start);

    return 0;
}