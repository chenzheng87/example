#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	pid_t pid = getpid();
	while(1){
		printf("process pid is: %d.\n", pid);
		sleep(2);
	}
	return 0;
}
