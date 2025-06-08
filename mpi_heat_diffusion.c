#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

float *allocate_matrix(int rows, int cols);
void inidat(int rows, int cols, float *u);
void prtdat(int rows, int cols, float *u, char *fnam);
void send_forwards(int rank, int numtasks, int local_rows, int cols, float *local_u, float *prv_msg, float *flw_msg);
void send_backwards(int rank, int numtasks, int local_rows, int cols, float *local_u, float *prv_msg, float *flw_msg);
void update(int local_rows, int cols, float *u1, float *u2, int rank, int numtasks, float *prv_msg, float *flw_msg, float cx, float cy);

int main(int argc, char **argv) {

  //checking the correct number of arguments
  if (argc != 6){
    printf("Error: the number of passed arguments must be 5: \n number of rows, number of colums, x heat capacity, y heat capacity, time steps\n  ");
    return 0;
  }

  /* Input arguments */
  int rows = atoi(argv[1]); //number of rows of the global grid
  int cols = atoi(argv[2]); //number of columns of the grid
  float cx = atof(argv[3]); //specific heat in x direction
  float cy = atof(argv[4]); //specific heat in y direction
  int nts = atoi(argv[5]); //number of timesteps for the iteration

  /*MPI initialization*/
    int rank, numtasks, rc;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  /* Master prints the parameters*/
  if (rank == 0){
    printf("Starting parallel version of 2D heat exercise...\n");
    printf("number of rows: %i \n", rows);
    printf("number of columns: %i \n", cols);
    printf("specific heat in x: %f \n", cx);
    printf("specific heat in y: %f \n", cy);
    printf("number of timesteps: %i  \n\n\n", nts);
  }

  /*pointers to the used grids*/
    float *local_u1 = NULL; //first local grid for each processor
    float *local_u2 = NULL; //second local grid for each processor
    float *u = NULL; //global grid

   /*variables for domain composition*/
    int *sendcounts; //array containing the number of sended elements from the master to the tasks
    int *offsets; //array containing the offsets for each task
    int remainder; // module between rows and number of tasks
    int minimum_rows; //minimum number of rows for each task
    int chunks_dimension[numtasks]; //array containing chunk size of each task
    int recvcounts; //number of receved elemts of each task
    int local_rows; //number of rows for each processor

    /*variables for tasks communication*/
    float *prv_msg = NULL; //message from the previous task
    float *flw_msg = NULL; //message from the following task

    /*Calculate sendcounts*/
    sendcounts = malloc(numtasks * sizeof(int));
    remainder = rows % numtasks;
    minimum_rows = ((rows - remainder)/numtasks);

    for(int i = 0; i< numtasks; i++){
      chunks_dimension[i] = minimum_rows;
      }
    for(int i = 0; i< remainder; i++){
      chunks_dimension[i] = chunks_dimension[i]+1;
      }
    for(int i = 0; i< numtasks; i++){
      sendcounts[i]= chunks_dimension[i] * cols;
      }

    /*Calculate offsets*/
    offsets = malloc(numtasks * sizeof(int));
    offsets[0] = 0;
    for(int i = 1; i<numtasks;i++){
    offsets[i] = offsets[i-1] + sendcounts[i-1];
    }

    /*Calculate local_rows and recvcounts*/
    local_rows = chunks_dimension[rank];
    recvcounts = local_rows * cols;

    /*Allocate local grid and messages*/
    local_u1 = allocate_matrix(local_rows, cols);
    local_u2 = allocate_matrix(local_rows, cols);
    prv_msg = malloc(cols * sizeof(float));
    flw_msg = malloc(cols * sizeof(float));

    /*DOMAIN DEFINITION*/
    /*Master initializes the global grid and creates input file */
    if (rank == 0) {
      printf("Task %i: Initializing grid and creating input file:\n", rank);
        u = allocate_matrix(rows, cols);
        inidat(rows, cols, u);
        prtdat(rows, cols, u, "heat_diffusion/initial.dat");
    }


    /*DOMAIN DECOPOSITION*/
    /*Master distributes grid data to all the tasks via collective comunication - master keeps one part */
    /*Tasks receve data from the master*/
    MPI_Scatterv(u, sendcounts, offsets, MPI_FLOAT, local_u1, local_rows * cols, MPI_FLOAT, 0, MPI_COMM_WORLD);

    printf("task %i receved data from the master, now starts the computation...\n",rank);

    /*ITERATION FOR HEAT COMPUTATION- all tasks involved*/
    for (int t = 0; t < nts; t++) {
      send_forwards(rank, numtasks, local_rows, cols, local_u1, prv_msg, flw_msg);
      send_backwards(rank, numtasks, local_rows, cols, local_u1, prv_msg, flw_msg);
      update(local_rows, cols, local_u1, local_u2, rank, numtasks, prv_msg, flw_msg, cx, cy);

      /*promoting local_u1 to local_u2*/
        float *temp = local_u1;
        local_u1 = local_u2;
        local_u2 = temp;
    }

    /*MASTER COLLECTS DATA FROM THE TASKS - TASKS SEND DATA TO THE MASTER*/
    if(rank == 0){ printf("Task %i is collecting data from the tasks...\n",rank);}
    else{ printf("Task %i is sending data to the master...\n",rank);}
    MPI_Gatherv(local_u1, local_rows * cols, MPI_FLOAT, u, sendcounts, offsets, MPI_FLOAT, 0, MPI_COMM_WORLD);
    printf("task %i done. \n",rank);


    
    /*MASTER CREATES OUTPUT FILE*/
    if (rank == 0) {
        prtdat(rows, cols, u, "heat_diffusion/final.dat");
        free(u);
    }

    /*Clean up the memory*/
    free(local_u1);
    free(local_u2);
    free(prv_msg);
    free(flw_msg);
    free(sendcounts);
    free(offsets);

    MPI_Finalize();
    return 0;
}

/*subroutines  allocate_matrix to create the grids*/
float *allocate_matrix(int rows, int cols) {
    return malloc(rows * cols * sizeof(float));
}

/*subroutine inidat to initialise the temperature on the grid */
void inidat(int nx, int ny, float *u) {
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            u[i * ny + j] = (float)(i * (nx - i - 1) * j * (ny - j - 1));
        }
    }
}

/*subroutine prtdat to print the data on files*/
void prtdat(int nx, int ny, float *u, char *fnam) {
    FILE *fp = fopen(fnam, "w");
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            fprintf(fp, "%8.3f", u[i * ny + j]);
            if (j != ny - 1) fprintf(fp, " ");
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

/*subroutine send_forwards to send last local row to the following task*/
void send_forwards(int rank, int numtasks, int local_rows, int cols, float *local_u, float *prv_msg, float *flw_msg) {
    if (rank != numtasks - 1) {
        MPI_Send(&local_u[(local_rows - 1) * cols], cols, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD);
    }
    if (rank != 0) {
        MPI_Recv(prv_msg, cols, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

/*subroutine send_backwards to send the first local row to the previous tasks*/
void send_backwards(int rank, int numtasks, int local_rows, int cols, float *local_u, float *prv_msg, float *flw_msg) {
    if (rank != 0) {
        MPI_Send(&local_u[0], cols, MPI_FLOAT, rank - 1, 1, MPI_COMM_WORLD);
    }
    if (rank != numtasks - 1) {
        MPI_Recv(flw_msg, cols, MPI_FLOAT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

/*subroutine update to update the temperature at every timestep*/
void update(int local_rows, int cols, float *u1, float *u2, int rank, int numtasks, float *prv_msg, float *flw_msg, float cx, float cy) {
    for (int i = 0; i < local_rows; i++) {
        for (int j = 0; j < cols; j++) {
            float up = (i == 0 && rank != 0) ? prv_msg[j] : (i > 0 ? u1[(i - 1) * cols + j] : u1[i * cols + j]);
            float down = (i == local_rows - 1 && rank != numtasks - 1) ? flw_msg[j] : (i < local_rows - 1 ? u1[(i + 1) * cols + j] : u1[i * cols + j]);
            float left = (j == 0) ? u1[i * cols + j] : u1[i * cols + j - 1];
            float right = (j == cols - 1) ? u1[i * cols + j] : u1[i * cols + j + 1];
            u2[i * cols + j] = u1[i * cols + j] + cx * (left + right - 2.0 * u1[i * cols + j]) + cy * (up + down - 2.0 * u1[i * cols + j]);
        }
    }
}