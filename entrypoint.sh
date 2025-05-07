#!/bin/bash

# Check if all 6 arguments are passed
if [ "$#" -ne 6 ]; then
  echo "Usage: $0 <processors> <rows> <columns> <cx> <cy> <nts>"
  exit 1
fi

# Run the MPI program with passed arguments
mpirun --allow-run-as-root --oversubscribe -np "$1" ./mpi_heat_diffusion "$2" "$3" "$4" "$5" "$6"

# Run the Python visualization
python3 diffusion_plot.py
