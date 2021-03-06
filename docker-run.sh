#!/bin/bash
SERVICE_NAME=amiga-gcc-build-process
IMAGE=amiga-gcc-builder
PROCESS_RUNNING=`docker ps -a --format '{{.Names}}' | grep $SERVICE_NAME`

if [ ! -f demo-1-gcc ]
then
    echo creating binary file to avoid wrong permissions
    touch demo-1-gcc
    chmod +x demo-1-gcc
fi
  
if [ -n "$PROCESS_RUNNING" ]
then
    echo terminating old build process
    docker rm $SERVICE_NAME > /dev/null
fi

echo "starting $IMAGE"
echo
docker run --name=$SERVICE_NAME \
    -v  $PWD:/build \
    --env BUILD_RESULT=demo-1-gcc \
    $IMAGE:latest
