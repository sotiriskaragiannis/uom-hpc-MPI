// Compile: mpicc -O3 histogram_mpi.c -o histogram_mpi.out
// Execute: mpirun -n <number_of_mpi_processes> ./histogram_mpi.out
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MASTER 0

int main(int argc, char *argv[]) {
    int rank, size;
    double t_total, t_comp;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    FILE *fIn, *fOut;
    unsigned char header[54];
    int width, height, stride, padding;
    int histogram[256][4] = {0};

    if (rank == MASTER) {
        fIn = fopen("original.bmp", "rb");
        fOut = fopen("new.bmp", "wb");
        if (!fIn || !fOut) {
            printf("File error.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fread(header, sizeof(unsigned char), 54, fIn);
        fwrite(header, sizeof(unsigned char), 54, fOut);

        width = *(int*)&header[18];
        height = abs(*(int*)&header[22]);
        stride = (width * 3 + 3) & ~3;
        padding = stride - width * 3;

        printf("width: %d, height: %d, stride: %d, padding: %d\n", width, height, stride, padding);
    }

    // Broadcast image dimensions to all processes
    MPI_Bcast(&width, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&stride, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&padding, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

    int rows_per_proc = height / size;
    int extra = height % size;
    int start_row = rank * rows_per_proc + (rank < extra ? rank : extra);
    int local_rows = rows_per_proc + (rank < extra ? 1 : 0);
    int local_size = local_rows * stride;

    unsigned char *local_data = (unsigned char *)malloc(local_size);

    if (rank == MASTER) {
        unsigned char *full_data = (unsigned char *)malloc(height * stride);
        fread(full_data, sizeof(unsigned char), height * stride, fIn);
        fclose(fIn);

        int offset = 0;
        for (int i = 0; i < size; i++) {
            int r = rows_per_proc + (i < extra ? 1 : 0);
            int sz = r * stride;
            if (i == MASTER)
                memcpy(local_data, full_data + offset, sz);
            else
                MPI_Send(full_data + offset, sz, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
            offset += sz;
        }

        free(full_data);
    } else {
        MPI_Recv(local_data, local_size, MPI_UNSIGNED_CHAR, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (rank == MASTER)
        t_total = MPI_Wtime();  // Start total timing

    int local_hist[256][4] = {0};

    if (rank == MASTER)
        t_comp = MPI_Wtime();  // Start compute timing

    // Histogram + grayscale conversion
    for (int i = 0; i < local_rows; i++) {
        for (int j = 0; j < width; j++) {
            int idx = i * stride + j * 3;
            unsigned char b = local_data[idx];
            unsigned char g = local_data[idx + 1];
            unsigned char r = local_data[idx + 2];
            local_hist[b][0]++;
            local_hist[g][1]++;
            local_hist[r][2]++;
            unsigned char gray = r * 0.299 + g * 0.587 + b * 0.114;
            local_hist[gray][3]++;
            local_data[idx] = local_data[idx + 1] = local_data[idx + 2] = gray;
        }
    }

    if (rank == MASTER)
        t_comp = MPI_Wtime() - t_comp;

    MPI_Reduce(local_hist, histogram, 256 * 4, MPI_INT, MPI_SUM, MASTER, MPI_COMM_WORLD);

    if (rank == MASTER) {
        unsigned char *full_gray = (unsigned char *)malloc(height * stride);
        memcpy(full_gray + start_row * stride, local_data, local_size);

        for (int i = 1; i < size; i++) {
            int r = rows_per_proc + (i < extra ? 1 : 0);
            int sz = r * stride;
            int offset = (i * rows_per_proc + (i < extra ? i : extra)) * stride;
            MPI_Recv(full_gray + offset, sz, MPI_UNSIGNED_CHAR, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        fOut = fopen("new.bmp", "ab");
        fwrite(full_gray, sizeof(unsigned char), height * stride, fOut);
        fclose(fOut);
        free(full_gray);

        t_total = MPI_Wtime() - t_total;
        printf("Total time: %f sec\n", t_total);
        printf("Computation time: %f sec\n", t_comp);
        printf("Communication time: %f sec\n", t_total - t_comp);
    } else {
        MPI_Send(local_data, local_size, MPI_UNSIGNED_CHAR, MASTER, 1, MPI_COMM_WORLD);
    }

    free(local_data);
    MPI_Finalize();
    return 0;
}
