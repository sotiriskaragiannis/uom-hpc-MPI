#!/bin/bash

# Compile both versions
gcc -O3 count_sort_seq.c -o count_sort_seq.out -fopenmp
mpicc -O3 count_sort_mpi.c -o count_sort_mpi.out

# Define problem sizes and process counts
problem_sizes=(25000 50000 100000 200000)
processes=(2 4 8 12)
output_file="results.txt"
csv_file="results.csv"

# Clear previous results
echo "Count Sort Experiment Results (MPI)" > $output_file
echo "====================================" >> $output_file
echo "Version,Processes,Size,Total_Time(s),Comp_Time(s),Comm_Time(s)" > $csv_file

# Run sequential and save results to CSV
for size in "${problem_sizes[@]}"; do
    echo "Running sequential for size $size..." | tee -a $output_file
    for i in {1..5}; do
        result=$(./count_sort_seq.out $size)
        echo "$result" >> $output_file
        time=$(echo "$result" | grep -oP '\d+\.\d+')
        echo "sequential,1,$size,$time" >> $csv_file
    done

    # Run MPI parallel with different process counts and save to CSV
    for p in "${processes[@]}"; do
        echo "Running MPI with $p processes for size $size..." | tee -a $output_file
        for i in {1..5}; do
            result=$(mpirun -np $p ./count_sort_mpi.out $size)
            echo "$result" >> $output_file

            # Extract timings from the "Total time: ..." line
            timing_line=$(echo "$result" | grep "Total time")
            total_time=$(echo "$timing_line" | awk '{print $3}')
            comp_time=$(echo "$timing_line" | awk '{print $6}')
            comm_time=$(echo "$timing_line" | awk '{print $9}')

            echo "mpi,$p,$size,$total_time,$comp_time,$comm_time" >> $csv_file
        done
    done

    echo "--------------------------------------" >> $output_file
done

echo "Experiment completed. Results saved in $output_file and $csv_file."
