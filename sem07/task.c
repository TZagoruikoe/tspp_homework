#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define N 128
#define MAX_ITER 1000

void initialize_grid(double *grid, int rows, int cols, int rank) {
    for (int i = 1; i < rows - 1; i++) {
        srand(rank * rows + i);
        for (int j = 1; j < cols - 1; j++) {
            grid[i * cols + j] = rand() / (double)RAND_MAX; //sin(j / (double)cols);
        }
    }
}

double compute_norm(double *old_grid, double *new_grid, int rows, int cols) {
    double norm = 0.0;
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < cols - 1; j++) {
            norm += (new_grid[i * cols + j] - old_grid[i * cols + j]) * 
                    (new_grid[i * cols + j] - old_grid[i * cols + j]);
        }
    }
    return norm;
}

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rows_per_process = N / size;
    int total_rows = rows_per_process + 2;
    double *grid = (double*)calloc(total_rows * N, sizeof(double));
    double *new_grid = (double*)calloc(total_rows * N, sizeof(double));

    initialize_grid(grid, total_rows, N, rank);

    for (int iter = 0; iter < MAX_ITER; iter++) {
        if (size > 1) {
            if (rank > 0 && rank < size - 1) {
                MPI_Sendrecv(grid + N, N, MPI_DOUBLE, rank - 1, 0, grid + (total_rows - 1) * N, N, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Sendrecv(grid + (total_rows - 2) * N, N, MPI_DOUBLE, rank + 1, 0, grid, N, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            if (rank == 0) {
                MPI_Sendrecv(grid + (total_rows - 2) * N, N, MPI_DOUBLE, rank + 1, 0, grid + (total_rows - 1) * N, N, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
    
            if (rank == size - 1) {
                MPI_Sendrecv(grid + N, N, MPI_DOUBLE, rank - 1, 0, grid, N, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        for (int i = 1; i < total_rows - 1; i++) {
            for (int j = 1; j < N - 1; j++) {
                new_grid[i * N + j] = 0.25 * (grid[(i - 1) * N + j] + grid[(i + 1) * N + j] +
                                              grid[i * N + (j - 1)] + grid[i * N + (j + 1)]);
            }
        }

        double *temp = grid;
        grid = new_grid;
        new_grid = temp;
    }
    
    double norm = compute_norm(new_grid, grid, total_rows, N);
        
    double global_norm;
    MPI_Allreduce(&norm, &global_norm, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        
    if (rank == 0) {
        printf("Norm: %lf\n", sqrt(global_norm));
    }

    free(grid);
    free(new_grid);
    MPI_Finalize();

    return 0;
}
