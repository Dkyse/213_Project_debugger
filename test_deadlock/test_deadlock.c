
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// For passing arguments to thread functions
typedef struct args{
  pthread_mutex_t m1;
  int count;
} arg_t;

/**
 * Increment shared counter and print it's value
 * @param  void_args pointer to an initialized arg_t struct
 */
void * thread_fn(void * void_args) {
  //Note: this function has an intentional deadlock, it forgot to unlock its lock.
  arg_t * args = (arg_t *) void_args;

  pthread_mutex_lock(&args->m1);
  args->count = args->count + 1;

  printf("count: %d\n", args->count);

  return NULL;
}

/**
 * This program has a deadlock bug
 * - Five threads are creted, each of which increments a shred counter.
 * - A lock is supposed to protect each access to the shared counter
 * - However, the thread_fn function has forgotten to unlock its lock!
 * - Therefore, the first thread to acquire the lock will never release it,
 *    and all other threads are stuck waiting for the first thread.
 * - In short, we have deadlocked!
 */
int main(int argc, char** argv)  {
  // Declare and initialize variables
  pthread_t threads[5];
  arg_t deadlock;
  pthread_mutex_init(&deadlock.m1, NULL);
  deadlock.count = 0;

  // Create threads
  for(int i = 0; i < 5; i++){
    pthread_create(&threads[i], NULL, thread_fn, (void*) &deadlock);
  }

  // Join threads
  for(int i = 0; i < 5; i++){
    pthread_join(threads[i], NULL);
  }

  // Print final count
  printf("All threads have joined!\nCount is: %d\n", deadlock.count);
  return 0;
}
