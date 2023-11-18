#!/bin/sh

# Path to the aesdsocket executable
AESDSOCKET="aesdsocket"

# Function to start the aesdsocket daemon
start() {
    echo "starting aesdsocket daemon"
    start-stop-daemon --start --background --exec "$AESDSOCKET" -- -d
}

# Function to stop the aesdsocket daemon
stop() {
    echo "stoping aesdsocket daemon"
    start-stop-daemon --stop --signal SIGTERM --exec "$AESDSOCKET"
}

# Check the command-line argument
case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0