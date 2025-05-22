// Compile: mpicc -O3 string_matching_mpi.c -o string_matching_mpi.out
// Execute: mpirun -n <number_of_mpi_processes> ./string_matching_mpi.out <file_name> <string>
// Create file: base64 /dev/urandom | head -c 1000000000 > file.txt
#include "mpi.h"
#include "mpi_proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

    long file_size, block_size, match_size, pattern_size, partial_total_matches,
        total_matches, my_block_size, overlap;
    char *buffer, *partial_buffer;
    char *filename, *pattern;
    size_t result;
    int i, j, *match;
    int *sendcounts = NULL;
    int *displs = NULL;
    double t_total, t_comp;
    int proc_rank, proc_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);

    if (proc_rank == 0) { // master only
        FILE *pFile;
        if (argc != 3) {
            printf("Usage : %s <file_name> <string>\n", argv[0]);
            return 1;
        }
        filename = argv[1];

        pFile = fopen(filename, "rb");
        if (pFile == NULL) {
            printf("File error\n");
            return 2;
        }

        // obtain file size:
        fseek(pFile, 0, SEEK_END);
        file_size = ftell(pFile);
        rewind(pFile);
        printf("\nfile size is %ld\n", file_size);
        // allocate memory to contain the file:
        buffer = (char *)malloc(sizeof(char) * file_size);
        if (buffer == NULL) {
            printf("Memory error\n");
            return 3;
        }
        // copy the file into the buffer:
        result = fread(buffer, 1, file_size, pFile);
        if (result != file_size) {
            printf("Reading error\n");
            return 4;
        }
        fclose(pFile);
    }

    pattern = argv[2]; // Every process can read the arguments.
    pattern_size = strlen(pattern);
    overlap = pattern_size - 1;

    if (proc_rank == 0) {
        t_total = MPI_Wtime();
    }

    MPI_Bcast(&file_size, 1, MPI_LONG, 0, MPI_COMM_WORLD); // Share the file_size
    block_size = (long)file_size / proc_size;

    // Prepare arguments for MPI_Scatterv
    if (proc_rank == 0) {
        sendcounts = (int *)malloc(sizeof(int) * proc_size);
        displs = (int *)malloc(sizeof(int) * proc_size);
        for (int i = 0; i < proc_size; i++) {
            displs[i] = i * block_size;
            sendcounts[i] = block_size;
            if (i != proc_size - 1) {
                sendcounts[i] += overlap;
            }
        }
    }

    my_block_size = block_size;
    if (proc_rank != proc_size - 1) {
        my_block_size += overlap;
    }

    partial_buffer = (char *)malloc(sizeof(char) * my_block_size);

    MPI_Scatterv(buffer, sendcounts, displs, MPI_CHAR, partial_buffer, my_block_size, MPI_CHAR, 0, MPI_COMM_WORLD);

    printf("block size for task %d is %ld\n", proc_rank, my_block_size);
    if (proc_rank != proc_size - 1) {
        match_size = block_size; // Only up to the non-overlapping part
    } else {
        match_size = my_block_size - pattern_size + 1;
    }

    match = (int *)malloc(sizeof(int) * match_size);
    if (match == NULL) {
        printf("Malloc error\n");
        return 5;
    }

    partial_total_matches = 0;
    for (j = 0; j < match_size; j++) {
        match[j] = 0;
    }

    if (proc_rank == 0) {
        t_comp = MPI_Wtime();
    }

    /* Brute Force string matching */
    for (j = 0; j < match_size; ++j) {
        for (i = 0; i < pattern_size && pattern[i] == partial_buffer[i + j]; ++i);
        if (i >= pattern_size) {
            match[j] = 1;
            partial_total_matches++;
        }
    }
    if (proc_rank == 0) {
        t_comp = MPI_Wtime() - t_comp;
    }

    printf("Total matches for %d = %ld\n", proc_rank, partial_total_matches);

    // reduce
    MPI_Reduce(&partial_total_matches, &total_matches, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (proc_rank == 0) {
        t_total = MPI_Wtime() - t_total;
        printf("Total matches = %ld\n", total_matches);
        printf("Total time: %f Comp time %f Comm time %f \n", t_total, t_comp, t_total - t_comp);
        free(buffer);
    }
    free(partial_buffer);
    free(match);

    MPI_Finalize();

    return 0;
}
