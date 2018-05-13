
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct args{
  pthread_mutex_t m1;
  int count;
}arg_t;

//this function has an intentional deadlock
//it will 
void * threadA(void * args){
  arg_t * temp = (arg_t *) args;

  
  pthread_mutex_lock(&temp->m1);
  temp->count = temp->count + 1;
  printf("count: %d\n", temp->count);
  return NULL;
}

//this program is an incorrect implementation 
//5 threads are created, they each increment a shared counter.
//this program contains a lock to protect the shared count
//But the program has forgotten to unlock its lock!
//there's a concurrency error!

int main(int argc, char** argv)  {
  
   pthread_t testA[5];
   arg_t deadlock;
   pthread_mutex_init(&deadlock.m1, NULL);
   deadlock.count = 0;
     
   for(int i = 0; i < 5; i++){
        
     pthread_create(&testA[i], NULL, threadA, (void*)  &deadlock);
   }

   for(int i = 0; i < 5; i++){
     pthread_join(testA[i], NULL);
   }
   
   printf("all threads have joined! Count is :%d \n", deadlock.count);
   return 0;
}
     
