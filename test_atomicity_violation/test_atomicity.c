#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// For passing arguments to thread functions
typedef struct args {
  int count;
} arg_t;

/**
 * Check whether global counter has changed since initialization
 * @param  void_args pointer to an initialized arg_t struct
 */
void * threadA(void *void_args){
  arg_t *arg = (arg_t*) void_args;

  if(arg->count == 1){
    printf("count is 1!\n");
  }

  return NULL;
}

/**
 * Increment global counter
 * @param  void_args pointer to an initialized arg_t struct
 */
void * threadB(void *void_args){
  arg_t *args = (arg_t*) void_args;

  args->count = args->count + 1;

  return NULL;
}

/**
 * This program has an atomocity violation bug
 * - threadA has a conditional dependency on the value of count, and threadB
 *   increments count
 * - But both threadA and threadB are given the same struct as an argument
 * - Therefore, there is a data race between threadA and threadB:
 *   threadA might read count == 1 and enter the conditional, at the same time
 *   that threadB incremented count.
 * - In other words, sometimes when threadA passes the conditional and prints
 *   out count is 1, although the count is actually not 1.
 */
int main(int argc, char** argv)  {
  pthread_t threads[2];
  arg_t atomicity;
  atomicity.count = 1;

  // Run threads
  pthread_create(&threads[0], NULL, threadA, (void*)  &atomicity);
  pthread_create(&threads[1], NULL, threadB, (void*)  &atomicity);

  // Join threads
  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);

  // Print final count
  printf("count: %d\n", atomicity.count);
  return 0;
}
