#!/bin/bash

# Compile both versions
gcc -O3 string_matching_seq.c -o string_matching_seq.out -fopenmp
mpicc -O3 string_matching_mpi.c -o string_matching_mpi.out

# Define number of MPI processes
process_counts=(2 4 8 12)
output_file="results.txt"
csv_file="results.csv"

# Input file and pattern to search
input_file="file.txt"
search_pattern="test"

# Clear previous results
echo "String Matching Experiment Results" > $output_file
echo "================================" >> $output_file
echo "Version,Processes,Total_Time(s),Comp_Time(s),Comm_Time(s)" > $csv_file  # CSV header

# Run sequential version and write results to CSV
echo "Running String Matching sequential..." | tee -a $output_file
for i in {1..5}; do
    result=$(./string_matching_seq.out "$input_file" "$search_pattern")
    echo "$result" >> $output_file
    time=$(echo "$result" | grep -oP '\d+\.\d+')  # Extract time
    echo "sequential,1,$time,," >> $csv_file  # Empty comp/comm columns for sequential
done

echo "--------------------------------" >> $output_file

# Run MPI version with different process counts
for p in "${process_counts[@]}"; do
    echo "Running String Matching MPI with $p processes..." | tee -a $output_file
    for i in {1..5}; do
        result=$(mpirun -np $p ./string_matching_mpi.out "$input_file" "$search_pattern")
        echo "$result" >> $output_file

        # Extract timings from the "Total time: ..." line
        timing_line=$(echo "$result" | grep "Total time")
        total_time=$(echo "$timing_line" | awk '{print $3}')
        comp_time=$(echo "$timing_line" | awk '{print $6}')
        comm_time=$(echo "$timing_line" | awk '{print $9}')

        echo "mpi,$p,$total_time,$comp_time,$comm_time" >> $csv_file
    done
    echo "--------------------------------" >> $output_file
done

echo "Experiment completed. Results saved in $output_file and $csv_file."
