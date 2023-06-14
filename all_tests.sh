#!/bin/bash

# compile and run all files in tests directory
for file in tests/*.c
do
    echo " "
    echo "---------------------------------------------------"
    echo "Compiling $file"
    cc -Wall -g -o teste lib/ppos_core.c lib/queue.c $file
    echo "Running $file"
    ./teste
done