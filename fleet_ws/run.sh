#!/bin/bash

# Stop on errors
set -e

# Auto-escalation/fallback for docker group permissions in new shell sessions
if ! docker ps >/dev/null 2>&1; then
  if sg docker -c "docker ps" >/dev/null 2>&1; then
    echo "Current shell session doesn't have active docker group privileges yet. Re-running via 'sg docker'..."
    exec sg docker -c "$0 $*"
  else
    echo "Error: Cannot connect to docker socket. Please ensure the Docker daemon is running and you have permissions."
    exit 1
  fi
fi

# Build the docker image if it doesn't exist or if requested
if [[ "$(docker images -q drone_fleet:latest 2> /dev/null)" == "" ]]; then
  echo "Docker image 'drone_fleet:latest' not found. Building..."
  docker build -t drone_fleet:latest .
else
  echo "Docker image 'drone_fleet:latest' exists. Starting..."
fi

# Run the container
# Mounts the local src folder as a volume
# Uses host networking and sets ROS_DOMAIN_ID to 42
docker run -it --rm \
  --net=host \
  -e ROS_DOMAIN_ID=42 \
  -v "$(pwd)/src:/workspace/src" \
  drone_fleet:latest
