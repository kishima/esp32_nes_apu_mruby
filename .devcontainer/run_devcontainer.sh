#!/bin/bash

# Check for available serial devices and add them to docker run command
DEVICE_ARGS=""
for device in /dev/ttyUSB* /dev/ttyACM*; do
    if [ -e "$device" ]; then
        DEVICE_ARGS="$DEVICE_ARGS --device=$device"
        # Make sure the device is accessible
        sudo chmod 666 "$device" 2>/dev/null || true
    fi
done

# If arguments provided, use them as command, otherwise use bash
if [ $# -gt 0 ]; then
    CMD="$@"
else
    CMD="bash"
fi

# Set docker flags based on whether we have a TTY
if tty -s; then
    DOCKER_FLAGS="-it"
else
    DOCKER_FLAGS="-i"
fi

docker run $DOCKER_FLAGS --rm --group-add=dialout --group-add=plugdev --privileged $DEVICE_ARGS --user $(id -u):$(id -g) -v $PWD/..:/project -v /dev/bus/usb:/dev/bus/usb esp32_build_container:v5.4.1 $CMD
