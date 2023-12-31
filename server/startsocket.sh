#!/bin/sh
### BEGIN INIT INFO
# Provides:          aesdsocket
# Required-Start:    $local_fs $network
# Required-Stop:     $local_fs $network
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start aesdsocket program
### END INIT INFO

# Change the following line to the path where aesdsocket is installed
#AESDSOCKET_PATH="/home/jqiao/Development/course1/assignments-3-and-later-jqiaobln/server/aesdsocket"
AESDSOCKET_PATH="/usr/bin/aesdsocket"

case "$1" in
    start)
        echo "Starting aesdsocket..."
        # Add any additional commands or options needed to start aesdsocket
        $AESDSOCKET_PATH &
        ;;
    stop)
        echo "Stopping aesdsocket..."
        # Add any additional commands or options needed to stop aesdsocket
        pkill -f $AESDSOCKET_PATH
        ;;
    restart)
        $0 stop
        sleep 1
        $0 start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac

echo "exit"

exit 0