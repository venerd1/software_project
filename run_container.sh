#!/bin/bash

#Check if all 6 arguments are passed
if [ "$#" -ne 6 ]; then
  echo "Uso: $0 <processors> <rows> <columns> <cx> <cy> <nts>"
  exit 1
fi 

# Variables
PROCESSORS="$1"
ROWS="$2"
COLUMNS="$3"
CX="$4"
CY="$5"
NTS="$6"

SOFTWARE_PROJECT="software_project" #project directory
IMAGE_NAME="heat_diffusion"         #container image
OUTPUT_PATH="/home/croselli/output"  

# Navigate to the project directory
cd "$SOFTWARE_PROJECT" || { echo "Directory not found: $SOFTWARE_PROJECT"; exit 1; }

# Build the Docker image
sudo docker build -t "$IMAGE_NAME" .

# Run the Docker container with volume mapping
sudo docker run -v "$OUTPUT_PATH":/software_project/output "$IMAGE_NAME"

sudo docker run -v "$OUTPUT_PATH":/software_project/output "$IMAGE_NAME" \
  "$PROCESSORS" "$ROWS" "$COLUMNS" "$CX" "$CY" "$NTS"