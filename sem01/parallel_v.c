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

typedef struct {
    int start_i;
    int end_i;
    double step;
} pthrData;

void* threadFunc(void* thread_data) {
    pthrData* data = (pthrData*) thread_data;
    double step = data->step;
    int start_i = data->start_i;
    int end_i = data->end_i;
    double* sum = (double*)malloc(sizeof(double));
    *sum = 0;

    for (int i = start_i; i <= end_i; i++) {
        *sum += step * 4 / (1 + (i * step + step / 2) * (i * step + step / 2));
    }

    return (void*)sum;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("ERROR: Incorrect arguments number.\n");
        exit(1);
    }

    double bench_t_start, bench_t_end;
    int count = atoi(argv[1]);
    int threads_count = atoi(argv[2]);
    double step_tmp = 1./count;
    double result = 0;
    int tmp1 = 0;
    int tmp2 = -1;

    pthread_t* threads = (pthread_t*)malloc(threads_count * sizeof(pthread_t));
    pthrData* threadData = (pthrData*)malloc(threads_count * sizeof(pthrData));

    bench_t_start = rtclock();
    
    for (int i = 0; i < threads_count; i++) {
        /*Start_i*/
        if (count % threads_count == 0) {
            threadData[i].start_i = tmp1;
            tmp1 += count / threads_count;
        }
        else {
            threadData[i].start_i = tmp1;
            tmp1 += count / threads_count + 1;
        }

        /*End_i*/
        if (count % threads_count == 0) {
            tmp2 += count / threads_count;
            threadData[i].end_i = tmp2;
        }
        else {
            if (i != threads_count - 1) {
                tmp2 += count / threads_count + 1;
                threadData[i].end_i = tmp2;
            }
            else {
                tmp2 += count % threads_count;
                threadData[i].end_i = tmp2;
            }
        }

        /*Step*/
        threadData[i].step = step_tmp;

        pthread_create(&(threads[i]), NULL, threadFunc, &threadData[i]);
    }

    double* tmp = NULL;
    for (int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], (void**)&tmp);
        result += *tmp;
        free(tmp);
    }

    bench_t_end = rtclock();

    printf("%lf\n", result);
    printf("Elapsed time: %lf s\n", bench_t_end - bench_t_start);

    free(threads);
    free(threadData);

    return 0;
}