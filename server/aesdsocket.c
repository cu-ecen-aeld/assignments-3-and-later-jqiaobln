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
#include <pthread.h>
#include <time.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"

static int running = 1;
pthread_mutex_t data_mutex;
pthread_t thread;
FILE* data_file;

void handle_signal(int signal)
{
    syslog(LOG_INFO, "Caught signal, exiting");

    // Set running to 0 to stop accepting new connections
    running = 0;

    // Cancel the thread and wait for it to complete
    pthread_cancel(thread);
    pthread_join(thread, NULL);

    // Clean up and exit
    pthread_mutex_destroy(&data_mutex);
    fclose(data_file);
    closelog();
    exit(0);
}

void* handle_connection(void* arg)
{
    int client_fd = *(int*)arg;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};
    int bytes_received;
    int total_bytes_received = 0;

    // Get client IP address
    getpeername(client_fd, (struct sockaddr*)&address, &addrlen);

    // Receive data from the client and append to the data file
    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        // Append the data to the file
        pthread_mutex_lock(&data_mutex);
        int bytes_written = fwrite(buffer, 1, bytes_received, data_file);
        pthread_mutex_unlock(&data_mutex);

        total_bytes_received += bytes_received;
        printf("Total Received %d bytes\n", total_bytes_received);

        // Check for newline character to indicate end of packet
        if (strchr(buffer, '\n') != NULL) {
            // Send the contents of the data file back to the client
            pthread_mutex_lock(&data_mutex);
            int seek_result = fseek(data_file, 0, SEEK_SET);

            while (fgets(buffer, sizeof(buffer), data_file) != NULL) {
                int sent_bytes = send(client_fd, buffer, strlen(buffer), 0);
                printf("Send buffer %d bytes\n", sent_bytes);
            }

            pthread_mutex_unlock(&data_mutex);
            break;
        }
    }

    // Close the client socket and log the client IP address
    close(client_fd);
    syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(address.sin_addr));

    pthread_exit(NULL);
}

void* append_timestamp(void* arg)
{
    while (running) {
        // Get the current system time
        time_t current_time = time(NULL);
        struct tm* time_info = localtime(&current_time);

        // Format the timestamp string
        char timestamp[128];
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S %z", time_info);
        strcat(timestamp, "\n");

        // Append the timestamp to the data file
        pthread_mutex_lock(&data_mutex);
        fputs(timestamp, data_file);
        fflush(data_file);
        pthread_mutex_unlock(&data_mutex);

        // Sleep for 10 seconds
        sleep(10);
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
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

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        syslog(LOG_ERR, "Failed to bind socket: %s", strerror(errno));
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 1) < 0) {
        syslog(LOG_ERR, "Failed to listen for connections: %s", strerror(errno));
        return -1;
    }

    // Initialize the mutex
    if (pthread_mutex_init(&data_mutex, NULL) != 0) {
        syslog(LOG_ERR, "Failed to initialize mutex");
        return -1;
    }

    // Open the data file for appending
    data_file = fopen(DATA_FILE, "a+");
    if (data_file == NULL) {
        syslog(LOG_ERR, "Failed to open data file: %s", strerror(errno));
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

    // Create a thread to append timestamp every 10 seconds
    pthread_t timestamp_thread;
    if (pthread_create(&timestamp_thread, NULL, append_timestamp, NULL) != 0) {
        syslog(LOG_ERR, "Failed to create timestamp thread: %s", strerror(errno));
        fclose(data_file);
        pthread_mutex_destroy(&data_mutex);
        closelog();
        return -1;
    }

    // Accept incoming connections and handle data
    while (running) {
        syslog(LOG_INFO, "Waiting for connection...");

        int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_fd < 0) {
            syslog(LOG_ERR, "Failed to accept connection: %s", strerror(errno));
            continue;
        }

        // Log the client IP address
        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(address.sin_addr));

        // Create a new thread to handle the connection
        if (pthread_create(&thread, NULL, handle_connection, &client_fd) != 0) {
            syslog(LOG_ERR, "Failed to create thread for connection: %s", strerror(errno));
            close(client_fd);
        }

        // Detach the thread to allow it to run independently
        pthread_detach(thread);
    }

    // Clean up and exit
    close(server_fd);
    pthread_cancel(timestamp_thread);
    pthread_join(timestamp_thread, NULL);
    pthread_mutex_destroy(&data_mutex);
    fclose(data_file);
    closelog();

    return 0;
}