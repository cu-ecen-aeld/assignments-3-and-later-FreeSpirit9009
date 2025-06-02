#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    // struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* pthread_param = (struct thread_data*)thread_param;
    
    // wait
    usleep (pthread_param->wait_to_obtain_ms * 1000);
    
    // obtain mutex (assume no timeout; any error in obtaining the mutex will cause a failure code). Note, it is assumed that mutex was previously initialized.
    int res = pthread_mutex_lock(pthread_param->mutex);
    if (res != 0)
    {
        printf("Error: could not obtain mutex, error code %i, in %s, will return failure code\n", res, __func__);
        return thread_param;
    }
    
    // wait
    usleep (pthread_param->wait_to_release_ms * 1000);
    
    // release mutex
    res = pthread_mutex_unlock(pthread_param->mutex);
    if (res != 0)
    {
        printf("Error: could not release mutex, error code %i, in %s, will return failure code\n", res, __func__);
        return thread_param;
    }
   
    // return pointer to the struct we initially got, makes releasing of memory simpler; also set success to true
    pthread_param->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t* thread, pthread_mutex_t* mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
     
    // check preconditions
    assert(wait_to_obtain_ms >= 0);
    assert(wait_to_release_ms >= 0);
    assert(thread);
    assert(mutex);    
     
    // allocate memory for thread_data, fail if not successful
    struct thread_data* pthread_param = malloc(sizeof(struct thread_data));
    if(pthread_param == NULL)
    {
        printf("Error: could not allocate %lu bytes of memory in %s, will return false\n", sizeof(struct thread_data), __func__);
        return false;
    }
    
    // pthread_param allocated, now add the parameters
    pthread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    pthread_param->wait_to_release_ms = wait_to_release_ms;
    pthread_param->thread_complete_success = false;
    pthread_param->mutex = mutex;
    
    // start the thread, fail if not successful
    pthread_attr_t* attr = NULL;
    const int res = pthread_create(thread, /* given to us */
                                   attr,   /* what's expected here? */
                                   threadfunc,
                                   (void*)pthread_param);
    if(res != 0)
    {
        printf("Error: could not start thread, error code %i, in %s, will return false\n", res, __func__);
        return false;
    }
    
    // copy the thread ID, it is guaranteed to be valid at this point, even in the case of reentrancy
    pthread_param->thread = thread;

    // all went fine. Printing "thread" might not be portable, it is supposed to be opaque (but it might help debugging)
    printf("Info: thread started successfully with thread=%u\n", (unsigned int) (*pthread_param->thread));
    return true;
}

