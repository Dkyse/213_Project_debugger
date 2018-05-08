#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>

int
main(int argc, char *argv[])
{
  int x = 0;
  for (size_t i = 0; i < 10; i++) {
    x++;
  }
  printf("hello world!\n");

  return 0;
}
