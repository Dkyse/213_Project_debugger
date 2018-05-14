#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// For passing arguments to thread functions
typedef struct args{
  pthread_mutex_t m1;
  int count;
} arg_t;


/**
 * Initialize shared counter to 1
 * @param  void_args pointer to an initialized arg_t struct
 */
void * threadA(void * void_args){
  arg_t * args = (arg_t *) void_args;

  pthread_mutex_lock(&args->m1);
  args->count = 1;
  pthread_mutex_unlock(&args->m1);

  return NULL;
}

/**
* Increment shared counter and print it's value
* @param  void_args pointer to an initialized arg_t struct
*/
void * threadB(void * void_args){
  arg_t * args = (arg_t *) void_args;

  pthread_mutex_lock(&args->m1);
  args->count = args->count + 1;
  printf("threadB count: %d\n", args->count);
  pthread_mutex_unlock(&args->m1);

  return NULL;
}

/**
 * This program has an order violation bug
 * - The shared counter is not initialized in main, but rather in threadA
 * - threadB then increments the shared counter
 * - But if threadB runs before threadA, threadB is accessing an uninitialized
 *   struct, which could result in execution errors
 * - In particular, since the compiler automatically initializes ints to 0,
 *   count will have a value of 1, instead of the expected value of 2
 */
int main(int argc, char** argv)  {
  // Declare and initialize variables
  pthread_t threads[2];
  arg_t order;
  pthread_mutex_init(&order.m1, NULL);

  // Create threads
  pthread_create(&threads[0], NULL, threadB, (void*)  &order);
  pthread_create(&threads[1], NULL, threadA, (void*)  &order);

  // Join threads
  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);

  // Print final count
  printf("count: %d\n", order.count);
  return 0;
}
