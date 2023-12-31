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
#include "queue.h"

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"

static int running = 1;
pthread_mutex_t data_mutex;
pthread_t thread;
FILE *data_file;
SLIST_HEAD(client_list, client_node)
client_head;
SLIST_HEAD(thread_list, thread_node)
thread_head;

struct client_node
{
    int client_fd;
    SLIST_ENTRY(client_node)
    entries;
};

struct thread_node
{
    pthread_t thread_id;
    int completed; // Variable to track thread completion
    SLIST_ENTRY(thread_node)
    entries;
};

struct node_info
{
    struct client_node *client;
    struct thread_node *thread;
};

void handle_signal(int signal)
{
    syslog(LOG_INFO, "Caught signal, exiting");

    // Set running to 0 to stop accepting new connections
    running = 0;

    // Cancel the thread and wait for it to complete
    pthread_cancel(thread);
    pthread_join(thread, NULL);

    printf("Thread signaled\n");

    // Clean up and exit
    fclose(data_file);
    remove(DATA_FILE);
    closelog();
    exit(EXIT_SUCCESS);
}

void *client_thread(void *arg)
{
    struct node_info *node = (struct node_info *)arg;
    int client_fd = node->client->client_fd;
    //free(arg);

    // Get client IP address
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    getpeername(client_fd, (struct sockaddr *)&address, &addrlen);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);

    // Receive data from the client and append to the data file
    char buffer[1024];
    ssize_t bytes_received;
    size_t total_bytes_received = 0;

    printf("New client thread!!\n");

    if (fileno(data_file) == -1)
    {
        printf("Invalid file descriptor\n");
        // Handle the error or exit the loop
    }

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0)
    {
        // Append the data to the file
        pthread_mutex_lock(&data_mutex);
        size_t bytes_written = fwrite(buffer, 1, bytes_received, data_file);
        pthread_mutex_unlock(&data_mutex);

        total_bytes_received += bytes_received;
        printf("Total Received %zu bytes from client %s\n", total_bytes_received, client_ip);

        // Check for newline character to indicate end of packet
        if (strchr(buffer, '\n') != NULL)
        {
            // Send the contents of the data file back to the client
            pthread_mutex_lock(&data_mutex);

            int seek_result = fseek(data_file, 0, SEEK_SET);

            while (fgets(buffer, sizeof(buffer), data_file) != NULL)
            {
                int sent_bytes = send(client_fd, buffer, strlen(buffer), 0);
                printf("Sent %d bytes to client %s\n", sent_bytes, client_ip);
            }

            if (feof(data_file))
            {
                printf("End of file reached.\n");
            }
            else if (ferror(data_file))
            {
                printf("Error occurred while reading from the file: %s\n", strerror(errno));
            }

            pthread_mutex_unlock(&data_mutex);
            break;
        }
    }

    // Close the client connection
    close(client_fd);

    // Mark the thread as completed
    struct thread_node* thread_info = node->thread;
    //thread_info->thread_id = pthread_self();
    //thread_info->completed = 1;

    printf("Thread completed\n");

    //pthread_cancel(pthread_self());
    pthread_exit(NULL);
}

void *append_timestamp(void *arg)
{
    while (running)
    {
        // Get the current system time
        time_t current_time = time(NULL);
        struct tm *time_info = localtime(&current_time);

        // Format the timestamp string
        char timestamp[128];
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S %z", time_info);
        strcat(timestamp, "\n");

        // Append the timestamp to the data file
        pthread_mutex_lock(&data_mutex);
        fputs(timestamp, data_file);
        fflush(data_file);
        pthread_mutex_unlock(&data_mutex);

        printf("Timestamp: %s\n", timestamp);

        // Sleep for 10 seconds
        sleep(10);
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int daemonize = 0;

    // Initialize the client and thread lists
    SLIST_INIT(&client_head);
    SLIST_INIT(&thread_head);

    // Set up signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            daemonize = 1;
        }
    }

    // Open the syslog
    openlog("aesdsocket", LOG_PID, LOG_USER);

    printf("aesksocket starts\n");

    // Create the server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
        return -1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        // handle error
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

    // Open the data file
    data_file = fopen(DATA_FILE, "a+");
    if (data_file == NULL)
    {
        syslog(LOG_ERR, "Failed to open data file: %s", strerror(errno));
        exit(EXIT_FAILURE);
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

        printf("aesksocket daemon starts\n");
        // Child process continues
        umask(0);
        setsid();
        chdir("/");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    // Initialize the mutex
    if (pthread_mutex_init(&data_mutex, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed to initialize mutex");
        fclose(data_file);
        remove(DATA_FILE);
        exit(EXIT_FAILURE);
    }

    // Create a thread to append timestamps
    pthread_t timestamp_thread;
    if (pthread_create(&timestamp_thread, NULL, append_timestamp, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed to create timestamp thread");
        pthread_cancel(timestamp_thread);
        pthread_join(timestamp_thread, NULL);
        pthread_mutex_destroy(&data_mutex);
        fclose(data_file);
        remove(DATA_FILE);
        exit(EXIT_FAILURE);
    }

    // Main program loop
    while (running)
    {
        // Accept a new client connection        
        if ((client_fd = accept(server_fd, (struct sockaddr*) &address, (socklen_t*) &addrlen)) < 0) {
            syslog(LOG_ERR, "Failed to accept connection: %s", strerror(errno));
            continue;
        }

        // Create a client information structure
        struct client_node* client_info = malloc(sizeof(struct client_node));
        client_info->client_fd = client_fd;

        // Add the client information to the client list
        SLIST_INSERT_HEAD(&client_head, client_info, entries);

        // Create a thread to handle the client connection
        struct thread_node* thread_info = malloc(sizeof(struct thread_node));
        thread_info->completed = 0;

        struct node_info* node_info = malloc(sizeof(struct node_info));
        node_info->client = client_info;
        node_info->thread = thread_info;

        if (pthread_create(&thread_info->thread_id, NULL, client_thread, node_info) != 0)
        {
            syslog(LOG_ERR, "Failed to create client thread");
            printf("Failed to create client thread");
            pthread_cancel(thread_info->thread_id);
            pthread_join(thread_info->thread_id, NULL);
            free(thread_info);
            free(client_info);
            continue;
        }

        // Add the thread information to the thread list
        SLIST_INSERT_HEAD(&thread_head, thread_info, entries);
    }

    
    printf("All threads completed");

    // Wait for all client threads to complete
    struct thread_node* thread_info;
    SLIST_FOREACH(thread_info, &thread_head, entries)
    {
        pthread_join(thread_info->thread_id, NULL);
        free(thread_info);
    }

    struct client_node* client_node;
    SLIST_FOREACH(client_node, &client_head, entries)
    {
        free(client_node);
    }

    // Clean up and exit
    pthread_cancel(timestamp_thread);
    pthread_join(timestamp_thread, NULL);
    pthread_mutex_destroy(&data_mutex);
    fclose(data_file);
    remove(DATA_FILE);
    exit(EXIT_SUCCESS);
    return 0;
}