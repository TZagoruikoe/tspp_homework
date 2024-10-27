#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <time.h>
#include <sys/time.h>
#include <omp.h>

#define NUM 100

static double rtclock() {
    struct timeval Tp;
    int stat;

    stat = gettimeofday(&Tp, NULL);
    if (stat != 0)
        printf ("Error return from gettimeofday: %d", stat);

    return (Tp.tv_sec + Tp.tv_usec * 1.0e-6);
}

inline double frand(double a, double b) {
	return a + (b - a) * (rand() / double(RAND_MAX));
}

int main(int argc, char** argv) {
	int a = atoi(argv[1]);
	int b = atoi(argv[2]);
	double p = atof(argv[3]);
	int x0 = atoi(argv[4]);
	int N = atoi(argv[5]);
    int P = atoi(argv[6]);

    srand(time(NULL));

    double t = 0.0;
    double w = 0.0;
    int i;
    double bench_t_start, bench_t_end;

    bench_t_start = rtclock();

#pragma omp parallel private(i) shared(a, b, p, x0, N) num_threads(P)
{
    int thread_num = omp_get_thread_num();
    unsigned int seed = time(NULL) + thread_num;

    #pragma omp for reduction(+:t, w) schedule(static)
	for (i = 0; i < N; i++)	{
        int x = x0;
	    while (x > a && x < b) {
		    if(rand_r(&seed) % 100 < p * 100)
			    x += 1;
		    else
			    x -= 1;
		    t += 1.0;
	    }
		if (x == b) {
			w += 1;
        }
	}
}

    bench_t_end = rtclock();

    std::cout << "w = " << w / N << std::endl;
    std::cout << "t = " << t / N << std::endl;
    std::cout << "Elapsed time: " << bench_t_end - bench_t_start << std::endl;

	return 0;
}