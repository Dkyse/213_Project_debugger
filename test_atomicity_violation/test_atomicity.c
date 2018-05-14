#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


typedef struct args{
  pthread_mutex_t m1;
  int count;
}arg_t;

void * threadA(void * args){
  arg_t * temp = (arg_t *) args;
  
  if(temp->count == 1){
    printf("count is 1!\n");
  }

  return NULL;
}

void * threadB(void * args){
  arg_t * temp = (arg_t *) args;

  
  temp->count = temp->count + 1;
  
  return NULL;
}

//this function has an atomicity violation
//threadA has a conditional dependent on the value of count
//but threadB increments count
//it is possible for threadA to accept the conditional, then for threadB to increment the count
//in other words, sometimes when threadA passes the conditional and prints out count is 1, the count is actually not 1.
int main(int argc, char** argv)  {
  pthread_t testA[2];
  arg_t atomicity;
  
  pthread_mutex_init(&atomicity.m1, NULL);
  atomicity.count = 1;
  
  pthread_create(&testA[0], NULL, threadA, (void*)  &atomicity);
  pthread_create(&testA[1], NULL, threadB, (void*)  &atomicity);

  pthread_join(testA[0], NULL);
  pthread_join(testA[1], NULL);

  printf("countA: %d\n", atomicity.count);
  return 0;
}
