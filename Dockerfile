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
RUN mkdir -p /software_project/heat_diffusion


# Copy source files
COPY mpi_heat_diffusion.c .
COPY diffusion_plot.py .

# Compile the MPI program
RUN mpicc -o mpi_heat_diffusion mpi_heat_diffusion.c

# Copy the script that will be used to run the program
COPY entrypoint.sh .
RUN chmod +x entrypoint.sh

# Set the entrypoint to the script that accepts arguments
ENTRYPOINT ["./entrypoint.sh"]

