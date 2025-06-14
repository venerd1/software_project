#!/bin/bash

#Check if all 6 arguments are passed
if [ "$#" -ne 6 ]; then
  echo "Uso: $0 <processors> <rows> <columns> <cx> <cy> <nts>"
  exit 1
fi 

# Variables
PROCESSORS="$1"
ROWS="$2"
COLS="$3"
CX="$4"
CY="$5"
NTS="$6"

SOFTWARE_PROJECT="software_project" #project directory
IMAGE_NAME="heat_diffusion"         #container image
OUTPUT_PATH="$HOME/heat_diffusion"  


#create the output directory in the home
mkdir -p "$OUTPUT_PATH"

# Build the Docker image
echo "Building Docker image '$IMAGE_NAME'..."
sudo docker build -t "$IMAGE_NAME" .

# Run the Docker container with volume mapping
echo "Running Docker container"
sudo docker run -v "$OUTPUT_PATH":/software_project/heat_diffusion "$IMAGE_NAME" \
  "$PROCESSORS" "$ROWS" "$COLS" "$CX" "$CY" "$NTS"
