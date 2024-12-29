#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define N 20480

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double *local_A, *b, *local_c;

    int dims[2] = {0, 0};
    MPI_Dims_create(size, 2, dims);

    int block_rows = N / dims[0];
    int block_cols = N / dims[1];

    MPI_Comm cart_comm;
    int periods[2] = {0, 0};
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

    int coords[2];
    MPI_Cart_coords(cart_comm, rank, 2, coords);

    MPI_Comm row_comm, col_comm;
    MPI_Comm_split(cart_comm, coords[0], coords[1], &row_comm);
    MPI_Comm_split(cart_comm, coords[1], coords[0], &col_comm);

    local_A = (double *)malloc(block_rows * block_cols * sizeof(double));
    local_c = (double *)malloc(block_rows * sizeof(double));
    b = (double *)malloc(N * sizeof(double));

    srand(rank);
    for (int i = 0; i < block_rows; i++) {
        for (int j = 0; j < block_cols; j++) {
            local_A[i * block_cols + j] = rand() % 1000 / 1000.;
        }
    }

    if (rank == 0) {
        for (int i = 0; i < N; i++) {
            b[i] = rand() % 1000 / 1000.;
        }
    }
    
    double start_time, end_time;
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    MPI_Win win_b;
    if (rank == 0) {
        MPI_Win_create(b, N * sizeof(double), sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &win_b);
    }
    else {
        MPI_Win_create(NULL, 0, sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &win_b);
    }
    MPI_Win_fence(0, win_b);

    if (rank != 0) {
        MPI_Get(b, N, MPI_DOUBLE, 0, 0, N, MPI_DOUBLE, win_b);
    }

    MPI_Win_fence(0, win_b);

    for (int i = 0; i < block_rows; i++) {
        local_c[i] = 0.0;
        for (int j = 0; j < block_cols; j++) {
            local_c[i] += local_A[i * block_cols + j] * b[coords[1] * block_cols + j];
        }
    }

    double *final_c = NULL;
    double *tmp_c = NULL;
    if (coords[0] == 0) {
        tmp_c = (double *)malloc(N * sizeof(double));

        if (coords[1] == 0) {
            final_c = (double *)malloc(N * sizeof(double));
        }
    }

    MPI_Gather(local_c, block_rows, MPI_DOUBLE, tmp_c, block_rows, MPI_DOUBLE, 0, col_comm);
    
    if (coords[0] == 0) {
        MPI_Reduce(tmp_c, final_c, N, MPI_DOUBLE, MPI_SUM, 0, row_comm);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();

    if (rank == 0) {
        printf("Time: %lf\n", end_time - start_time);
    }

    if (coords[0] == 0) {
        free(tmp_c);
        if (coords[1] == 0) {
            free(final_c);
        }
    }
    free(local_A);
    free(local_c);
    free(b);

    MPI_Win_free(&win_b);
    MPI_Finalize();
    return 0;
}