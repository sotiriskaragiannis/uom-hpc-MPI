// Compile: mpicc -O3 count_sort_mpi.c -o count_sort_mpi.out
// Execute: mpirun -n <number_of_mpi_processes> ./count_sort_mpi.out <array_size>
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define UPPER 1000
#define LOWER 0

int main(int argc, char *argv[]) {
    long block_size, n;
    int *x, *y, *my_y;
    int i, j, my_num, my_place;
    double t_total, t_comp;
    int start, stop;
    int proc_rank, proc_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);

    if (argc != 2) {
        printf("Usage : %s <array_size>\n", argv[0]);
        return 1;
    }

    n = strtol(argv[1], NULL, 10); // Every process gets the n from the arguments
    x = (int *)malloc(n * sizeof(int));
    y = (int *)malloc(n * sizeof(int));
    my_y = (int *)malloc(n * sizeof(int));

    block_size = (long) n / proc_size;

    for (int i=0; i<n; i++){
        my_y[i] = 0; // Initialise my_y with zeros to sum them at them with reduce
        y[i] = 0;
    }

    if (proc_rank == 0){
        for (i = 0; i < n; i++)
        x[i] = n - i;
        // x[i] = (rand() % (UPPER - LOWER + 1)) + LOWER;
    }

    if (proc_rank == 0) {
        t_total = MPI_Wtime();
    }

    MPI_Bcast(x, n, MPI_INT, 0, MPI_COMM_WORLD);

    if (proc_rank == 0) {
        t_comp = MPI_Wtime();
    }

    start = proc_rank * block_size;
    if (proc_rank == proc_size){
        stop = n;
    } else {
        stop = start + block_size;
    }

    for (j = start; j < stop; j++) {
        my_num = x[j];
        my_place = 0;
        for (i = 0; i < n; i++){
            if ((my_num > x[i]) || ((my_num == x[i]) && (j < i))){
                my_place++;
            }
        }
        my_y[my_place] = my_num;
    }

    if (proc_rank == 0) {
        t_comp = MPI_Wtime() - t_comp;
    }

    MPI_Reduce(my_y, y, n, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (proc_rank == 0){
        t_total = MPI_Wtime() - t_total;

        // for (i = 0; i < n; i++){
        //     printf("%d\n", y[i]);
        // }

        printf("Total time: %f Comp time %f Comm time %f \n", t_total, t_comp, t_total - t_comp);
    }

    free(x);
    free(y);
    free(my_y);

    MPI_Finalize();

    return 0;
}
