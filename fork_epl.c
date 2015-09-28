#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	int ret;
	pid_t child_pid;
	child_pid = fork();
	if(child_pid < 0)
		perror("fork process failed.");
	else if(child_pid == 0){
		//in child process
		child_pid = fork();
		if(child_pid < 0)
			perror("fork process failed in child process.");
		else if(child_pid == 0){
			//in grand child process
			int ret_value;
			ret_value = execl(argv[1], 0);
			if(ret_value == -1)
				perror("grand child process exec error.");
			exit(0);
		}else{
			//in parent process
			sleep(1);
			int child_status;
			child_status = waitpid(child_pid, 0, WNOHANG);
			if(child_status == 0){
				printf("grand child process is running.\n");
				exit(0);
			}else{
				printf("grand child process not run.\n");
				exit(-1);
			}
		}
	}else{
		//in parent process
		int status;
		int statloc;
		status = waitpid(child_pid, &statloc, 0);
		if(statloc == 0)
			printf("child process is running.\n");
		else
			printf("child process not run.\n");
		while(1){
			printf("parent pid is: %d.\n", getpid());
			sleep(2);
		}
	}
	return 0;
}
