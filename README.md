# Documentation for MPI Heat Diffusion Simulation
**Author:** Camilla Roselli

## Table of Contents
- [Introduction](#introduction)
- [Program Overview](#program-overview)
- [Main](#main)
  - [Domain definition](#domain-definition)
  - [Domain decomposition](#domain-decomposition)
  - [Heat computation](#heat-computation)
  - [Data Collection](#data-collection)
- [Subroutines](#subroutines)
  - [inidat()](#inidat)
  - [prtdat()](#prtdat)
  - [send_forwards() and send_backwards()](#send_forwards-and-send_backwards)
  - [update()](#update)

---

## Introduction

This document provides a detailed explanation of the parallel implementation of a 2D heat diffusion simulation implemented using the MPI (Message Passing Interface) framework. This review contains an explanation of the code written in the file `mpi_heat_diffusion.c` and the obtained results.

---

## Program Overview

The program calculates the heat distribution on a 2D grid over a number of time steps. It distributes the computational workload across multiple processors via domain decomposition.

The **master process** (rank 0) initializes the data and distributes them to the other processes, as well as computing the results for its own chunk of data. The **other processes** receive the data, compute on them and return the results. The master also collects the results from each process and outputs the final matrix.

Note that the master is also a worker.

---

## Main

The main function first checks if the right number of arguments is passed, which are:

- number of rows
- number of columns
- cx
- cy
- number of time steps

If the arguments are not 5, the program exits.



### Domain definition

The master allocates the global matrix `u`, calls `inidat` to initialize it, and `prtdat` to write the values to a file named `initial.dat`.

### Domain decomposition

The master decomposes the grid into smaller chunks and sends each chunk to a process using:

```c
MPI_Scatterv(u, sendcounts, offsets, MPI_FLOAT,
             local_u1, local_rows * cols, MPI_FLOAT,
             0, MPI_COMM_WORLD);
```

Where:

- `u` is the send buffer
- `sendcounts` is an array containing the number of elements to send to each process
- `offsets` is an array containing the displacement (starting index) from where to take the elements for each process
- `MPI_FLOAT` is the data type
- `local_u1` is the receive buffer
- `local_rows * cols` is the number of elements to receive
- `0` is the root
- `MPI_COMM_WORLD` is the communicator

To define `sendcounts` and `offsets`, the code splits the number of rows between processes. When the rows do not divide evenly, the `remainder` is considered and the remaining rows are distributed one by one to the tasks. This procedure allows the workload to remain as balanced as possible.

The code:

```c
int minimum_rows = rows / numtasks;
int remainder = rows % numtasks;

for (int i = 0; i < numtasks; i++) {
    chunks_dimension[i] = minimum_rows + (i < remainder ? 1 : 0);
    sendcounts[i] = chunks_dimension[i] * cols;
    offsets[i] = (i == 0) ? 0 : offsets[i - 1] + sendcounts[i - 1];
}
```

### Heat computation

Each process performs `nts` iterations. At each iteration, a process:

1. Sends its last row to the next process and receives the last row of the previous process using `send_forwards()`.
2. Sends its first row to the previous process and receives the first row of the next process using `send_backwards()`.
3. Calls `update()` to compute the new temperature values.
4. Swaps the matrices `local_u1` and `local_u2`.

### Data Collection

At the end of the computation, the master gathers the results using `MPI_Gatherv`:

```c
MPI_Gatherv(local_u1, local_rows * cols, MPI_FLOAT,
            u, sendcounts, offsets, MPI_FLOAT,
            0, MPI_COMM_WORLD);
```

The final matrix is then written to a file named `final.dat`.

---

## Subroutines

### inidat()

Initializes the matrix:

```c
void inidat(int nx, int ny, float *u) {
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < ny; j++)
            u[i * ny + j] = i * (ny - j - 1);
}
```

### prtdat()

Prints the matrix to a file:

```c
void prtdat(int nx, int ny, float *u, char *fname) {
    FILE *fp = fopen(fname, "w");
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++)
            fprintf(fp, "%8.3f", u[i * ny + j]);
        fprintf(fp, "
");
    }
    fclose(fp);
}
```

### send_forwards() and send_backwards()

These two functions handle the communication of boundary rows.

#### send_forwards()

```c
void send_forwards(int rank, int numtasks, float *local_u,
                   int local_rows, int cols, float *prv_msg) {
    if (rank != numtasks - 1)
        MPI_Send(&local_u[(local_rows - 1) * cols], cols, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD);
    if (rank != 0)
        MPI_Recv(prv_msg, cols, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
```

#### send_backwards()

```c
void send_backwards(int rank, int numtasks, float *local_u,
                    int local_rows, int cols, float *flw_msg) {
    if (rank != 0)
        MPI_Send(local_u, cols, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD);
    if (rank != numtasks - 1)
        MPI_Recv(flw_msg, cols, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
```
The exchanged rows are stored in two arrays: `prv_msg` contains the row from the previous process,  `flw_msg` contains the row from the following one.

### update()

This function updates the matrix values:

```c
void update(float *u1, float *u2, float *prv_msg, float *flw_msg,
            int local_rows, int cols, float cx, float cy) {
    for (int i = 0; i < local_rows; i++) {
        for (int j = 0; j < cols; j++) {
            float up    = (i == 0) ? prv_msg[j] : u1[(i - 1) * cols + j];
            float down  = (i == local_rows - 1) ? flw_msg[j] : u1[(i + 1) * cols + j];
            float left  = (j == 0) ? u1[i * cols + j] : u1[i * cols + j - 1];
            float right = (j == cols - 1) ? u1[i * cols + j] : u1[i * cols + j + 1];

            u2[i * cols + j] = u1[i * cols + j] + 
                               cx * (left + right - 2 * u1[i * cols + j]) +
                               cy * (up + down - 2 * u1[i * cols + j]);
        }
    }
}
```
 
# Parallel Heat Diffusion Simulation with Docker

This project provides a `Dockerfile` that creates a container environment to compile and run the parallel heat diffusion simulation using MPI. It also includes a Python script to visualize the simulation results.  

## Prerequisites

Before creating the container, **create a folder called `output` in your home directory**. This is where the simulation results will be saved:

- `initial.dat`
- `final.dat`
- `heat_diffusion_comparison.png`

## Build and Run

To build and run the Docker container:

```bash
./software_project/run_container.sh <processors_number> <rows> <columns> <cx> <cy> <time_steps>
```
