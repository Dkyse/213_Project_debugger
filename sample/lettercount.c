#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <openssl/md5.h>
#include <stdbool.h>

#define PAGE_SIZE 0x1000
#define ROUND_UP(x,y) ((x) % (y) == 0 ? (x) : (x) + ((y) - (x) % (y)))

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

typedef struct thread_args {
  char * input;
} thread_args_t;

/// The number of times we've seen each letter in the input, initially zero
size_t letter_counts[26] = {0};

void* thread_fn(void* void_args);
/**
 * This function should divide up the file_data between the specified number of
 * threads, then have each thread count the number of occurrences of each letter
 * in the input data. Counts should be written to the letter_counts array. Make
 * sure you count only the 26 different letters, treating upper- and lower-case
 * letters as the same. Skip all other characters.
 *
 * \param num_threads   The number of threads your program must use to count
 *                      letters. Divide work evenly between threads
 * \param file_data     A pointer to the beginning of the file data array
 * \param file_size     The number of bytes in the file_data array
 */
void count_letters(int num_threads, char* file_data, off_t file_size) {
  pthread_t threads[num_threads];
  thread_args_t args[num_threads];

  ///splits up file_data into the input parameter of the arg structs
  for(int i = 0; i < num_threads; i++){
    //this is for the last thread, in case the file_size and the num_threads don't divide evenly
    if(i == num_threads - 1){
      args[i].input = malloc(file_size/num_threads + file_size % num_threads);
      strncpy(args[i].input, file_data + i *(file_size/num_threads), file_size/num_threads + file_size % num_threads);
    }
    else{
      args[i].input = malloc(file_size/num_threads);
      strncpy(args[i].input, file_data + i *(file_size/num_threads), file_size/num_threads);
    }
  }
  
  for(int i = 0; i < num_threads; i++){
    if(pthread_create(&threads[i], NULL, thread_fn, &args[i]) != 0){
      fprintf(stderr, "Error creating thread %d", i);
      exit(2);
    }
  }

  for(int i = 0; i < num_threads; i++){
    if(pthread_join(threads[i], NULL) != 0){
      fprintf(stderr, "Error joining thread %d", i);
      exit(2);
    }
  }
}

void* thread_fn(void* void_args){
  thread_args_t* real_args = (thread_args_t*)void_args;
  char * temp = real_args->input;
  int letter;

  //standardizes temp[i] into a number from 0-26 if its upper or lower case
  //protects counts with locks 
  for(int i = 0; i < strlen(temp); i++){
    pthread_mutex_lock(&m);
    if(temp[i] >= 'A' && temp[i] <= 'Z'){
      letter = temp[i] - 'A';
      letter_counts[letter]++;
    }
    else if(temp[i] >= 'a' && temp[i] <= 'z'){
      letter = temp[i] - 'a';
      letter_counts[letter]++;
    }
    pthread_mutex_unlock(&m);
  }
  return NULL;
}
/**
 * Show instructions on how to run the program.
 * \param program_name  The name of the command to print in the usage info
 */
void show_usage(char* program_name) {
  fprintf(stderr, "Usage: %s <N> <input file>\n", program_name);
  fprintf(stderr, "    where <N> is the number of threads (1, 2, 4, or 8)\n");
  fprintf(stderr, "    and <input file> is a path to an input text file.\n");
}

int main(int argc, char** argv) {
  // Check parameter count
  if(argc != 3) {
    show_usage(argv[0]);
    exit(1);
  }
  
  // Read thread count
  int num_threads = atoi(argv[1]);
  if(num_threads != 1 && num_threads != 2 && num_threads != 4 && num_threads != 8) {
    fprintf(stderr, "Invalid number of threads: %s\n", argv[1]);
    show_usage(argv[0]);
    exit(1);
  }
  
  // Open the input file
  int fd = open(argv[2], O_RDONLY);
  if(fd == -1) {
    fprintf(stderr, "Unable to open input file: %s\n", argv[2]);
    show_usage(argv[0]);
    exit(1);
  }
  
  // Get the file size
  off_t file_size = lseek(fd, 0, SEEK_END);
  if(file_size == -1) {
    fprintf(stderr, "Unable to seek to end of file\n");
    exit(2);
  }

  // Seek back to the start of the file
  if(lseek(fd, 0, SEEK_SET)) {
    fprintf(stderr, "Unable to seek to the beginning of the file\n");
    exit(2);
  }
  
  // Load the file with mmap
  char* file_data = mmap(NULL, ROUND_UP(file_size, PAGE_SIZE), PROT_READ, MAP_PRIVATE, fd, 0);
  if(file_data == MAP_FAILED) {
    fprintf(stderr, "Failed to map file\n");
    exit(2);
  }
  
  // Call the function to count letter frequencies
  count_letters(num_threads, file_data, file_size);
  
  // Print the letter counts
  for(int i=0; i<26; i++) {
    printf("%c: %lu\n", 'a' + i, letter_counts[i]);
  }
  
  return 0;
}
