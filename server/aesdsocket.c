#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"

static int running = 1;

void handle_signal(int signal)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    running = 0;
}

int main(int argc, char* argv[])
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    FILE* data_file;
    int bytes_received;
    int total_bytes_received = 0;
    int daemonize = 0;
    struct sigaction sa;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            daemonize = 1;
        }
    }

    // Set up signal handling
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Open the syslog
    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Create the server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
        return -1;
    }

    // Bind the socket to port 9000
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        syslog(LOG_ERR, "Failed to bind socket: %s", strerror(errno));
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 1) < 0) {
        syslog(LOG_ERR, "Failed to listen for connections: %s", strerror(errno));
        return -1;
    }

    // Daemonize if requested
    if (daemonize) {
        pid_t pid = fork();

        if (pid < 0) {
            syslog(LOG_ERR, "Failed to fork daemon process: %s", strerror(errno));
            return -1;
        }

        if (pid > 0) {
            // Parent process exits
            return 0;
        }

        // Child process continues
        umask(0);
        setsid();
        chdir("/");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    // Accept incoming connections and handle data
    while (running) {
        syslog(LOG_INFO, "Waiting for connection...");

        if ((client_fd = accept(server_fd, (struct sockaddr*) &address, (socklen_t*) &addrlen)) < 0) {
            syslog(LOG_ERR, "Failed to accept connection: %s", strerror(errno));
            continue;
        }

        // Log the client IP address
        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(address.sin_addr));

        // Open the data file for appending
        data_file = fopen(DATA_FILE, "a+");

        if (data_file == NULL) {
            syslog(LOG_ERR, "Failed to open data file: %s", strerror(errno));
            close(client_fd);
            continue;
        }

        // Receive data from the client and append to the data file
        while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
            // Append the data to the file
            int bytes_written = fwrite(buffer, 1, bytes_received, data_file);

            total_bytes_received += bytes_received;
            printf("Total Received %d bytes\n", total_bytes_received);

            // Check for newline character to indicate end of packet
            if (strchr(buffer, '\n') != NULL) {
                // Send the contents of the data file back to the client
                int seek_result = fseek(data_file, 0, SEEK_SET);

                while (fgets(buffer, sizeof(buffer), data_file) != NULL) {
                    int sent_bytes = send(client_fd, buffer, strlen(buffer), 0);
                    printf("Send buffer %d bytes\n", sent_bytes);
                }

                fclose(data_file);
            }
        }
                
        // Close the client socket and log the client IP address
        close(client_fd);
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(address.sin_addr));
    }
      
    // Clear the data file          
    data_file = fopen(DATA_FILE, "w");
    if (data_file == NULL) {
        syslog(LOG_ERR, "Failed to clear data file: %s", strerror(errno));
    }

    // Clean up and exit
    close(server_fd);
    remove(DATA_FILE);
    closelog();

    return 0;
}