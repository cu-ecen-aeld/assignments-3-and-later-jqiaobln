#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // // hint: use a cast like the one below to obtain thread arguments from your parameter
    // //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    // return thread_param;

    struct thread_data* thread_func_args = (struct thread_data*) thread_param;

    // Wait for the specified amount of time
    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    // Obtain the mutex
    pthread_mutex_lock(thread_func_args->mutex);

    // Wait for the specified amount of time again
    usleep(thread_func_args->wait_to_release_ms * 1000);

    // Release the mutex
    pthread_mutex_unlock(thread_func_args->mutex);

    // Set the thread complete success flag to true
    thread_func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    // /**
    //  * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
    //  * using threadfunc() as entry point.
    //  *
    //  * return true if successful.
    //  *
    //  * See implementation details in threading.h file comment block
    //  */
    // return false;
        // Allocate memory for the thread data structure
    struct thread_data* thread_data_ptr = (struct thread_data*) malloc(sizeof(struct thread_data));

    if (thread_data_ptr == NULL) {
        return false;
    }

    // Set up the thread data structure
    thread_data_ptr->mutex = mutex;
    thread_data_ptr->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data_ptr->wait_to_release_ms = wait_to_release_ms;
    thread_data_ptr->thread_complete_success = false;

    // Create the new thread
    int result = pthread_create(thread, NULL, threadfunc, thread_data_ptr);

    if (result != 0) {
        free(thread_data_ptr);
        return false;
    }

    return true;
}

