#include <stdio.h>
#include <pthread.h>

void* taskProcessor(void *arg){
   char *str = (char *)arg;
   fprintf(stdout, "Thread #%u working, parameter %s\n", (int)pthread_self(), str); 
   return (void *)0;
}
