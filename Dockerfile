FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Install system packages
RUN apt-get update -y && \
    apt-get install -y openmpi-bin libopenmpi-dev build-essential python3 python3-pip python3-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install Python packages
RUN pip3 install --no-cache-dir numpy matplotlib

# Set working directory
WORKDIR /software_project

# Create output directory
RUN mkdir -p /software_project/output2


# Copy source files
COPY mpi_heat_diffusion.c .
COPY diffusion_plot.py .

# Compile the MPI program
RUN mpicc -o mpi_heat_diffusion mpi_heat_diffusion.c

# Create a script to run both MPI program and Python script
RUN echo '#!/bin/bash\n\
mpirun --allow-run-as-root --oversubscribe -np 2 ./mpi_heat_diffusion 100 100 0.2 0.01 500\n\
python3 diffusion_plot.py' > start.sh && chmod +x start.sh


# Run both MPI and Python script
CMD ["./start.sh"]