#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#define N 5760
#define BLOCK 40

void matrix_multiply_block(float* A, float* B, float* C, int block_size) {
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            for (int k = 0; k < block_size; k++) {
                C[i * block_size + j] += A[i * block_size + k] * B[k * block_size + j];
            }
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int dims[2] = {0, 0};
    dims[0] = (int)(sqrt(size));
    dims[1] = (int)(sqrt(size));
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (dims[0] * dims[1] != size) {
        if (rank == 0) {
            printf("ERROR: The topology should be square.\n");
            printf("Topology: %d, %d\n", dims[0], dims[1]);
        }
        MPI_Finalize();
        return 1;
    }
    int sqrt_P = dims[0];
    int block_size = N / sqrt_P;

    MPI_Comm cart_comm;
    int periods[2] = {0, 0};
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm);

    int coords[2];
    MPI_Comm_rank(cart_comm, &rank);
    MPI_Cart_coords(cart_comm, rank, 2, coords);

    MPI_Comm row_comm, col_comm;
    MPI_Comm_split(cart_comm, coords[0], coords[1], &row_comm);
    MPI_Comm_split(cart_comm, coords[1], coords[0], &col_comm);

    float* A_block = (float*)malloc(block_size * block_size * sizeof(float));
    float* B_block = (float*)malloc(block_size * block_size * sizeof(float));
    float* C_block = (float*)malloc(block_size * block_size * sizeof(float));

    srand(rank);
    for (int i = 0; i < block_size * block_size; i++) {
        A_block[i] = rand() % 1000 / 1000.;
        B_block[i] = rand() % 1000 / 1000.;
        C_block[i] = 0.0;
    }

    float* A_temp = (float*)malloc(block_size * block_size * sizeof(float));
    float* B_temp = (float*)malloc(block_size * block_size * sizeof(float));

    double start_time, end_time;
    
    MPI_Barrier(cart_comm);
    start_time = MPI_Wtime();
    for (int step = 0; step < sqrt_P; step++) {
        if (coords[1] == step) {
            for (int i = 0; i < block_size * block_size; i++) {
                A_temp[i] = A_block[i];
            }
        }
        MPI_Bcast(A_temp, block_size * block_size, MPI_FLOAT, step, row_comm);

        if (coords[0] == step) {
            for (int i = 0; i < block_size * block_size; i++) {
                B_temp[i] = B_block[i];
            }
        }
        MPI_Bcast(B_temp, block_size * block_size, MPI_FLOAT, step, col_comm);

        for (int i = 0; i < block_size / BLOCK; i++) {
            for (int j = 0; j < block_size / BLOCK; j++) {
                for (int k = 0; k < block_size / BLOCK; k++) {
                    matrix_multiply_block(&A_temp[(i * block_size / BLOCK + k) * BLOCK * BLOCK], &B_temp[(k * block_size / BLOCK + j) * BLOCK * BLOCK], &C_block[(i * block_size / BLOCK + j) * BLOCK * BLOCK], BLOCK);
                }
            }
        }
    }
    MPI_Barrier(cart_comm);
    end_time = MPI_Wtime();

    if (rank == 0) {
        printf("Time: %lf\n", end_time - start_time);
    }

    free(A_block);
    free(B_block);
    free(C_block);
    free(A_temp);
    free(B_temp);

    MPI_Comm_free(&row_comm);
    MPI_Comm_free(&col_comm);
    MPI_Comm_free(&cart_comm);

    MPI_Finalize();
    return 0;
}