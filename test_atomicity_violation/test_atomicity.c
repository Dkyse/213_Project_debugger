#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct args{
  pthread_mutex_t m1;
  int count;
}arg_t;

void * threadA(void * args){
  arg_t * temp = (arg_t *) args;
  temp->count = temp->count + 1;
  printf("threadA: %d\n", temp->count);

  return NULL;
}

void * threadB(void * args){
  arg_t * temp = (arg_t *) args;
  //printf("countA: %d\n", temp->count);

  temp->count = 1;
  //printf("countA: %d\n", temp->count);
  
  return NULL;
}
 
//this program has an order violation
//count is not initialized in main
//instead, count is initialized in threadB, then incremented in threadA
//the intended final value of count is 2
//but if threadA runs before threadB, threadA does nothing
//and the final count is 1
int main(int argc, char** argv)  {
  pthread_t testA[2];
  arg_t atomicity;
  
  pthread_mutex_init(&atomicity.m1, NULL);

  pthread_create(&testA[0], NULL, threadA, (void*)  &atomicity);
  pthread_create(&testA[1], NULL, threadB, (void*)  &atomicity);

  pthread_join(testA[0], NULL);
  pthread_join(testA[1], NULL);

  printf("countA: %d\n", atomicity.count);
  return 0;
}
