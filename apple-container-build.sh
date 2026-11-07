#!/bin/bash
SERVICE_NAME=amiga-gcc-build-process
IMAGE=amiga-gcc-builder

# Check if container engine is running, start it if not
if ! container system status > /dev/null 2>&1; then
    echo "Container engine not running. Starting container system..."
    container system start
    sleep 2
fi

PROCESS_RUNNING=`container ls -a | grep $SERVICE_NAME`

if [ ! -f demo-1-gcc ]
then
    echo creating binary file to avoid wrong permissions
    touch demo-1-gcc
    chmod +x demo-1-gcc
fi

if [ -n "$PROCESS_RUNNING" ]
then
    echo terminating old build process
    container rm $SERVICE_NAME > /dev/null
fi

echo "starting $IMAGE"
echo
container run --name=$SERVICE_NAME \
    -v  $PWD:/build \
    --env BUILD_RESULT=demo-1-gcc \
    $IMAGE:latest
