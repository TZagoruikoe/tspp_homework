#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define K 100
#define GLIDER 1200

int f(int* data, int i, int j, int n) {
    int state = data[i * (n + 2) + j];
    int s = -state;

    for (int ii = i - 1; ii <= i + 1; ii++) {
        for (int jj = j - 1; jj <= j + 1; jj++) {
            s += data[ii * (n + 2) + jj];
        }
    }

    if (state == 0 && s == 3) 
        return 1;
    if (state == 1 && (s < 2 || s > 3)) 
        return 0;
    return state;
}

void update_data(int n, int* data, int* temp, int rows) {
    for (int i = 1; i <= rows; i++) {
        for (int j = 1; j <= n; j++) {
            temp[i * (n + 2) + j] = f(data, i, j, n);
        }
    }

    for (int i = 1; i <= rows; i++) {
        temp[i * (n + 2)] = temp[i * (n + 2) + n];
        temp[i * (n + 2) + (n + 1)] = temp[i * (n + 2) + 1];
    }
}

void init(int n, int* local_data, int rows, int rank, int size) {
    int x[GLIDER], y[GLIDER], direction[GLIDER];

    if (rank == 0) {
        srand(time(NULL) + rank);
        for (int i = 0; i < GLIDER; i++) {
            direction[i] = rand() % 4;
            x[i] = 2 + rand() % (n - 2);
            y[i] = 2 + rand() % (rows - 2);
        }
    }

    int my_x[GLIDER / size], my_y[GLIDER / size], my_direction[GLIDER / size];
    MPI_Scatter(x, GLIDER / size, MPI_INT, my_x, GLIDER / size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(y, GLIDER / size, MPI_INT, my_y, GLIDER / size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(direction, GLIDER / size, MPI_INT, my_direction, GLIDER / size, MPI_INT, 0, MPI_COMM_WORLD);
    for (int i = 0; i < GLIDER / size; i++) {
        local_data[my_y[i] * (n + 2) + my_x[i]] = 1;
        
        if (direction[i] == 0) {
            local_data[my_y[i] * (n + 2) + my_x[i] + 1] = 1;
            local_data[(my_y[i] - 1) * (n + 2) + my_x[i] - 1] = 1;
            local_data[(my_y[i] + 1) * (n + 2) + my_x[i]] = 1;
            local_data[(my_y[i] + 1) * (n + 2) + my_x[i] - 1] = 1;
        }
        if (direction[i] == 1) {
            local_data[my_y[i] * (n + 2) + my_x[i] - 1] = 1;
            local_data[(my_y[i] - 1) * (n + 2) + my_x[i] + 1] = 1;
            local_data[(my_y[i] + 1) * (n + 2) + my_x[i]] = 1;
            local_data[(my_y[i] + 1) * (n + 2) + my_x[i] + 1] = 1;
        }
        if (direction[i] == 2) {
            local_data[my_y[i] * (n + 2) + my_x[i] - 1] = 1;
            local_data[(my_y[i] - 1) * (n + 2) + my_x[i] + 1] = 1;
            local_data[(my_y[i] - 1) * (n + 2) + my_x[i]] = 1;
            local_data[(my_y[i] + 1) * (n + 2) + my_x[i] + 1] = 1;
        }
        if (direction[i] == 3) {
            local_data[my_y[i] * (n + 2) + my_x[i] + 1] = 1;
            local_data[(my_y[i] - 1) * (n + 2) + my_x[i] - 1] = 1;
            local_data[(my_y[i] - 1) * (n + 2) + my_x[i]] = 1;
            local_data[(my_y[i] + 1) * (n + 2) + my_x[i] - 1] = 1;
        }
    }

}

void setup_boundaries(int n, int* local_data, int rows, int rank, int size) {
    MPI_Request requests[4];
    MPI_Status statuses[4];

    int* top_row = &local_data[0 * (n + 2)];
    int* bottom_row = &local_data[(rows + 1) * (n + 2)];
    int* send_top = &local_data[1 * (n + 2)];
    int* send_bottom = &local_data[rows * (n + 2)];

    if (rank > 0) {
        MPI_Isend(send_top, n + 2, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &requests[0]);
        MPI_Irecv(top_row, n + 2, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &requests[1]);
    } else {
        MPI_Isend(send_top, n + 2, MPI_INT, size - 1, 0, MPI_COMM_WORLD, &requests[0]);
        MPI_Irecv(top_row, n + 2, MPI_INT, size - 1, 1, MPI_COMM_WORLD, &requests[1]);
    }

    if (rank < size - 1) {
        MPI_Isend(send_bottom, n + 2, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, &requests[2]);
        MPI_Irecv(bottom_row, n + 2, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &requests[3]);
    } else {
        MPI_Isend(send_bottom, n + 2, MPI_INT, 0, 1, MPI_COMM_WORLD, &requests[2]);
        MPI_Irecv(bottom_row, n + 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &requests[3]);
    }

    MPI_Waitall(4, requests, statuses);
}

int count_alive(int* local_data, int rows, int n) {
    int count = 0;
    for (int i = 1; i <= rows; i++) {
        for (int j = 1; j <= n; j++) {
            count += local_data[i * (n + 2) + j];
        }
    }
    return count;
}

void run_life(int n, int T, int rank, int size) {
    int rows = n / size;
    int* local_data = (int*)calloc((rows + 2) * (n + 2), sizeof(int));
    int* temp = (int*)calloc((rows + 2) * (n + 2), sizeof(int));

    init(n, local_data, rows, rank, size);

    int stable = 0, global_stable = 0;
    int previous_alive = -1;
    int current_alive = -1;
    int stop_iteration = -1;

    for (int t = 0; t < T; t++) {
        setup_boundaries(n, local_data, rows, rank, size);
        update_data(n, local_data, temp, rows);

        int* swap = local_data;
        local_data = temp;
        temp = swap;

        if (t >= K) {
            current_alive = count_alive(local_data, rows, n);

            if (current_alive == previous_alive) {
                stable = 1;
            }
            previous_alive = current_alive;

            MPI_Allreduce(&stable, &global_stable, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
            if (global_stable) {
                stop_iteration = t;
                break;
            }
        }
    }

    int local_count = count_alive(local_data, rows, n);
    int global_count = 0;

    MPI_Reduce(&local_count, &global_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Total number of living cells: %d\n", global_count);
        if (stop_iteration >= 0) {
            printf("The game stopped at the iteration: %d\n", stop_iteration);
        } else {
            printf("The game didn't stop for a given number of iterations.\n");
        }
    }

    free(local_data);
    free(temp);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int n = atoi(argv[1]);
    int T = atoi(argv[2]);

    int rank, size;
    double start_t, stop_t;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (n % size != 0) {
        if (rank == 0) {
            printf("ERROR: The grid size must be divided by the number of processes.\n");
        }
        MPI_Finalize();
        return 1;
    }
    
    start_t = MPI_Wtime();
    run_life(n, T, rank, size);
    stop_t = MPI_Wtime();

    if (rank == 0) {
        printf("Elapsed time: %lf\n", stop_t - start_t);
    }

    MPI_Finalize();
    return 0;
}