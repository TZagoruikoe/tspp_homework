#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 64
#define MAX_ITER 1000
#define MIN(a,b) ((a) < (b) ? (a) : (b))

int offset(int n, int dim, int idx) {
    return idx * (n / dim) + MIN(idx, n % dim);
}

void initialize_grid(double *grid, int nx, int ny, int nz) {
    for (int i = 0; i < nx * ny * nz; i++) {
        grid[i] = rand() % 1000 / 1000.;
    }
}

int idx(int x, int y, int z, int nx, int ny) {
    return z * nx * ny + y * nx + x;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int dims[3] = {0, 0, 0};
    MPI_Dims_create(size, 3, dims);
    int periods[3] = {1, 1, 1};

    MPI_Comm cart_comm;
    MPI_Cart_create(MPI_COMM_WORLD, 3, dims, periods, 0, &cart_comm);

    int coords[3] = {0, 0, 0};
    MPI_Cart_coords(cart_comm, rank, 3, coords);

    int nx = offset(N, dims[2], coords[2] + 1) - offset(N, dims[2], coords[2]);
    int ny = offset(N, dims[1], coords[1] + 1) - offset(N, dims[1], coords[1]);
    int nz = offset(N, dims[0], coords[0] + 1) - offset(N, dims[0], coords[0]);

    double *grid = (double *)malloc((nx + 2) * (ny + 2) * (nz + 2) * sizeof(double));
    double *new_grid = (double *)malloc((nx + 2) * (ny + 2) * (nz + 2) * sizeof(double));

    srand(rank);
    initialize_grid(grid, nx + 2, ny + 2, nz + 2);

    MPI_Datatype xy_side, xz_side, yz_side;
    MPI_Type_vector(ny, nx, nx + 2, MPI_DOUBLE, &xy_side);
    MPI_Type_commit(&xy_side);

    MPI_Type_vector(nz, nx, (nx + 2) * (ny + 2), MPI_DOUBLE, &xz_side);
    MPI_Type_commit(&xz_side);

    MPI_Type_vector(ny * nz, 1, nx + 2, MPI_DOUBLE, &yz_side);
    MPI_Type_commit(&yz_side);

    int nbr_x_low, nbr_x_high, nbr_y_low, nbr_y_high, nbr_z_low, nbr_z_high;
    MPI_Cart_shift(cart_comm, 0, 1, &nbr_z_low, &nbr_z_high);
    MPI_Cart_shift(cart_comm, 1, 1, &nbr_y_low, &nbr_y_high);
    MPI_Cart_shift(cart_comm, 2, 1, &nbr_x_low, &nbr_x_high);

    int iter = 0;
    double start_time, end_time;

    start_time = MPI_Wtime();
    do {
        MPI_Request reqs[12];

        MPI_Isend(&grid[idx(1, 1, 1, nx + 2, ny + 2)], 1, yz_side, nbr_x_low, 0, cart_comm, &reqs[0]);
        MPI_Irecv(&grid[idx(nx + 1, 1, 1, nx + 2, ny + 2)], 1, yz_side, nbr_x_high, 0, cart_comm, &reqs[1]);
        MPI_Irecv(&grid[idx(0, 1, 1, nx + 2, ny + 2)], 1, yz_side, nbr_x_low, 1, cart_comm, &reqs[2]);
        MPI_Isend(&grid[idx(nx, 1, 1, nx + 2, ny + 2)], 1, yz_side, nbr_x_high, 1, cart_comm, &reqs[3]);

        MPI_Isend(&grid[idx(1, 1, 1, nx + 2, ny + 2)], 1, xy_side, nbr_z_low, 2, cart_comm, &reqs[4]);
        MPI_Irecv(&grid[idx(1, 1, nz + 1, nx + 2, ny + 2)], 1, xy_side, nbr_z_high, 2, cart_comm, &reqs[5]);
        MPI_Irecv(&grid[idx(1, 1, 0, nx + 2, ny + 2)], 1, xy_side, nbr_z_low, 3, cart_comm, &reqs[6]);
        MPI_Isend(&grid[idx(1, 1, nz, nx + 2, ny + 2)], 1, xy_side, nbr_z_high, 3, cart_comm, &reqs[7]);

        MPI_Isend(&grid[idx(1, 1, 1, nx + 2, ny + 2)], 1, xz_side, nbr_y_low, 4, cart_comm, &reqs[8]);
        MPI_Irecv(&grid[idx(1, ny + 1, 1, nx + 2, ny + 2)], 1, xz_side, nbr_y_high, 4, cart_comm, &reqs[9]);
        MPI_Irecv(&grid[idx(1, 0, 1, nx + 2, ny + 2)], 1, xz_side, nbr_y_low, 5, cart_comm, &reqs[10]);
        MPI_Isend(&grid[idx(1, ny, 1, nx + 2, ny + 2)], 1, xz_side, nbr_y_high, 5, cart_comm, &reqs[11]);

        MPI_Waitall(12, reqs, MPI_STATUSES_IGNORE);

        for (int z = 1; z <= nz; z++) {
            for (int y = 1; y <= ny; y++) {
                for (int x = 1; x <= nx; x++) {
                    new_grid[idx(x, y, z, nx + 2, ny + 2)] = (
                        grid[idx(x - 1, y, z, nx + 2, ny + 2)] +
                        grid[idx(x + 1, y, z, nx + 2, ny + 2)] +
                        grid[idx(x, y - 1, z, nx + 2, ny + 2)] +
                        grid[idx(x, y + 1, z, nx + 2, ny + 2)] +
                        grid[idx(x, y, z - 1, nx + 2, ny + 2)] +
                        grid[idx(x, y, z + 1, nx + 2, ny + 2)]) / 6.;

                }
            }
        }

        double *temp = grid;
        grid = new_grid;
        new_grid = temp;

        iter++;
    } while (iter < MAX_ITER);


    double sum_diff = 0;
    for (int z = 1; z <= nz; z++) {
            for (int y = 1; y <= ny; y++) {
                for (int x = 1; x <= nx; x++) {
                    double diff = (new_grid[idx(x, y, z, nx + 2, ny + 2)] - grid[idx(x, y, z, nx + 2, ny + 2)]) *
                                  (new_grid[idx(x, y, z, nx + 2, ny + 2)] - grid[idx(x, y, z, nx + 2, ny + 2)]);
                    sum_diff += diff;
            }
        }
    }

    double global_diff;
    MPI_Allreduce(&sum_diff, &global_diff, 1, MPI_DOUBLE, MPI_SUM, cart_comm);
    sum_diff = global_diff;

    end_time = MPI_Wtime();

    if (rank == 0) {
        printf("Difference: %lf\n", sqrt(sum_diff));
        printf("Time: %lf\n", end_time - start_time);
        printf("Dims 0 --> %d, 1 --> %d, 2 --> %d\n", dims[0], dims[1], dims[2]);
    }

    MPI_Type_free(&xy_side);
    MPI_Type_free(&xz_side);
    MPI_Type_free(&yz_side);
    free(grid);
    free(new_grid);

    MPI_Finalize();
    return 0;
}