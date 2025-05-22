#!/bin/bash

# Compile both versions
gcc -O3 histogram_seq.c -o histogram_seq.out -fopenmp
mpicc -O3 histogram_mpi.c -o histogram_mpi.out

# Define MPI process counts
procs=(2 4 8 12)
output_file="results.txt"
csv_file="results.csv"

# Input image file
input_image="original.bmp"

# Ensure image exists
if [ ! -f "$input_image" ]; then
    echo "Error: Input image '$input_image' not found!"
    exit 1
fi

# Clear previous results
echo "Histogram MPI Experiment Results" > $output_file
echo "================================" >> $output_file
echo "Version,Processes,Total_Time(s),Computation_Time(s),Communication_Time(s)" > $csv_file

# Run sequential version (record only total time for simplicity)
echo "Running Histogram Sequential..." | tee -a $output_file
for i in {1..5}; do
    result=$(./histogram_seq.out)
    echo "$result" >> $output_file
    total_time=$(echo "$result" | grep -oP '\d+\.\d+')
    echo "sequential,1,$total_time,NA,NA" >> $csv_file
done

echo "--------------------------------" >> $output_file

# Run MPI version with various process counts
for p in "${procs[@]}"; do
    echo "Running Histogram MPI with $p processes..." | tee -a $output_file
    for i in {1..5}; do
        result=$(mpirun -np $p ./histogram_mpi.out)
        echo "$result" >> $output_file

        total=$(echo "$result" | grep "Total time" | grep -oP '\d+\.\d+')
        comp=$(echo "$result" | grep "Computation time" | grep -oP '\d+\.\d+')
        comm=$(echo "$result" | grep "Communication time" | grep -oP '\d+\.\d+')

        echo "mpi,$p,$total,$comp,$comm" >> $csv_file
    done
    echo "--------------------------------" >> $output_file
done

echo "Experiment completed. Results saved in $output_file and $csv_file."
