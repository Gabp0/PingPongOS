#!/bin/bash

# compile and run all files in tests directory
for file in tests/*.c
do
    echo " "
    echo "---------------------------------------------------"
    echo "Compiling $file"
    cc --std=c99 -Wall -g -o teste lib/ppos_core.c lib/queue.c $file -lm
    echo "Running $file"
    ./teste
done