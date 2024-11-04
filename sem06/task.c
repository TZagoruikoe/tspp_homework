#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

static double rtclock() {
    struct timeval Tp;
    int stat;

    stat = gettimeofday(&Tp, NULL);
    if (stat != 0)
        printf ("Error return from gettimeofday: %d", stat);

    return (Tp.tv_sec + Tp.tv_usec * 1.0e-6);
}

void get_array(int n, int* arr) {
    srand(2);

    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 100;
    }
    
}

void merge_sort(int *array, int left, int right) {
    if (left < right) {
        int middle = left + (right - left) / 2;

        int i, j, k;
        int n1 = middle - left + 1;
        int n2 = right - middle;
    
        int *left_arr = (int*)malloc(n1 * sizeof(int));
        int *right_arr = (int*)malloc(n2 * sizeof(int));
    
#pragma omp task shared(array) if(right - left > 1000)
        merge_sort(array, left, middle);
    
#pragma omp task shared(array) if(right - left > 1000)
        merge_sort(array, middle + 1, right);
    
#pragma omp taskwait
      
        for (i = 0; i < n1; i++)
            left_arr[i] = array[left + i];
        for (j = 0; j < n2; j++)
            right_arr[j] = array[middle + 1 + j];
    
        i = 0;
        j = 0;
        k = left;
        while (i < n1 && j < n2) {
            if (left_arr[i] <= right_arr[j]) {
                array[k] = left_arr[i];
                i++;
            } else {
                array[k] = right_arr[j];
                j++;
            }
            
            k++;
        }
    
        while (i < n1) {
            array[k] = left_arr[i];
            i++;
            k++;
        }
        
        while (j < n2) {
            array[k] = right_arr[j];
            j++;
            k++;
        }
        
        free(left_arr);
        free(right_arr);
    }
}

int sort_fuction(const void* a, const void* b) {
    return *(int*)(a) - *(int*)(b);
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int p = atoi(argv[2]);
    int* array = (int*)malloc(n * sizeof(int));
    int* copy_arr = (int*)malloc(n * sizeof(int));

    double q_t_start, q_t_end;
    double merge_t_start, merge_t_end;
    
    get_array(n, array);
    copy_arr = memcpy(copy_arr, array, n * sizeof(int));
    
    q_t_start = rtclock();
    qsort(array, n, sizeof(int), sort_fuction);
    q_t_end = rtclock();
    
    merge_t_start = rtclock();
#pragma omp parallel num_threads(p)
{
#pragma omp single
    merge_sort(copy_arr, 0, n - 1);
}
    merge_t_end = rtclock();

    printf("Elapsed q_time: %lf\n", q_t_end - q_t_start);
    printf("Elapsed merge_time: %lf\n", merge_t_end - merge_t_start);    
    
    free(array);
    return 0;
}