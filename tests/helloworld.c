#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void * thread_fn(void * void_args)  {
  printf("hello this is one of the threads\n");
  return NULL;
}


int
main(int argc, char *argv[])
{
  pthread_t thread1;
  pthread_t thread2;

  /* Create threads */
  if (pthread_create(&thread1, NULL, thread_fn, NULL) != 0) {
    perror("Error creating thread 1");
    exit(EXIT_FAILURE);
  }

  if(pthread_create(&thread2, NULL, thread_fn, NULL) != 0) {
    perror("Error creating thread 2");
    exit(EXIT_FAILURE);
  }

  /* Join threads */
  if(pthread_join(thread1, NULL) != 0) {
    perror("Error joining with thread 1");
    exit(EXIT_FAILURE);
  }

  if(pthread_join(thread2, NULL) != 0) {
    perror("Error joining with thread 2");
    exit(EXIT_FAILURE);
  }

  return 0;
}
