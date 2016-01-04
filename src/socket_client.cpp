#include <sys/types.h>  
#include <sys/socket.h>  
#include <stdio.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

int main(int argc,char *argv[])  
{  
	int sockfd;  
	int len;  
	struct sockaddr_in address;     
	int result;  
	int byte;  
	if(argc != 3){  
		printf("Usage: fileclient <address> <port>/n"); 
		return 0;  
	}  
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){  
		perror("sock");  
		exit(EXIT_FAILURE);  
	}  
	bzero(&address,sizeof(address));  
	address.sin_family = AF_INET;  
	address.sin_port = htons(atoi(argv[2]));  
	inet_pton(AF_INET,argv[1],&address.sin_addr);  
	len = sizeof(address);  

	if((result = connect(sockfd, (struct sockaddr *)&address, len))==-1){  
		perror("connect");  
		exit(EXIT_FAILURE);  
	}  

	int arg_cnt = 0;
	char arg_str[4] = {0};	
	
	int ret_val;
	fd_set wfds;
	int maxfd = -1;
	struct timeval time_limit;
	time_limit.tv_sec = 1;
	time_limit.tv_usec = 0;
	while(true){
		FD_ZERO(&wfds);
		FD_SET(0, &wfds);
		maxfd = 0;
		FD_SET(sockfd, &wfds);
		if(sockfd > maxfd)	
			maxfd = sockfd;
		ret_val = select(maxfd + 1, NULL, &wfds, NULL, &time_limit);
		if(ret_val < 1){
			close(sockfd);
			//reconnect
			while(connect(sockfd, (struct sockaddr *)&address, len) == -1)	
				perror("reconnect");
		}
		if(FD_ISSET(sockfd, &wfds)){
			if(arg_cnt == 20)
				arg_cnt = 0;
			sprintf(arg_str, "%d", arg_cnt++);
			if((byte=send(sockfd, arg_str, 4, 0))==-1){  
				perror("send");  
				exit(EXIT_FAILURE);  
			}             
		}
		sleep(1);
	}  
	close(sockfd);  
	exit(0);  
}  
