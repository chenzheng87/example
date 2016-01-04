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
	char arg_str[100] = {0};	

	int ret_val;
	fd_set fds;
	int maxfd = -1;
	struct timeval time_limit;
	time_limit.tv_sec = 1;
	time_limit.tv_usec = 0;
	//send data example
   /* while(true){*/
		//FD_ZERO(&fds);
		//FD_SET(0, &fds);
		//maxfd = 0;
		//FD_SET(sockfd, &fds);
		//if(sockfd > maxfd)	
			//maxfd = sockfd;
		//ret_val = select(maxfd + 1, NULL, &fds, NULL, &time_limit);
		//switch(ret_val){
			//case -1:
				////select error
			//case 0:
				////time out
				//close(sockfd);	
				//if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){  
					//perror("reconnect sock");  
					//exit(EXIT_FAILURE);  
				//}
				//while(connect(sockfd, (struct sockaddr *)&address, len) == -1){
					//perror("reconnect");
					//sleep(1);
				//}
				//break;
			//default:
				////data ready to send
				//if(FD_ISSET(sockfd, &fds)){
					//if(arg_cnt == 20)
						//arg_cnt = 0;
					//sprintf(arg_str, "%d", arg_cnt++);
					//if((byte=send(sockfd, arg_str, 4, MSG_NOSIGNAL))==-1){  
						//perror("send");  
						//close(sockfd);
						//if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){  
							//perror("reconnect sock");  
							//exit(EXIT_FAILURE);  
						//}
						//while(connect(sockfd, (struct sockaddr *)&address, len) == -1){
							//perror("reconnect connect");
							//sleep(1);
						//}
					//}             
				//}
				//break;
		//}
		//sleep(1);
	/*}*/ 
	//recieve data example
	while(true){
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		maxfd = 0;
		FD_SET(sockfd, &fds);
		if(sockfd > maxfd)	
			maxfd = sockfd;
		ret_val = select(maxfd + 1, &fds, NULL, NULL, &time_limit);
		switch(ret_val){
			case -1:
				//select error
				close(sockfd);	
				if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){  
					perror("reconnect sock");  
					exit(EXIT_FAILURE);  
				}
				while(connect(sockfd, (struct sockaddr *)&address, len) == -1){
					perror("reconnect");
					sleep(1);
				}
				break;
			case 0:
				//time out, try again
				break;
			default:
				//data ready to send
				if(FD_ISSET(sockfd, &fds)){
					if(arg_cnt == 20)
						arg_cnt = 0;
					sprintf(arg_str, "%d", arg_cnt++);
					if((byte=recv(sockfd, arg_str, 100, MSG_NOSIGNAL))==0){  
						perror("recieve");  
						close(sockfd);
						if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){  
							perror("reconnect sock");  
							exit(EXIT_FAILURE);  
						}
						while(connect(sockfd, (struct sockaddr *)&address, len) == -1){
							perror("reconnect connect");
							sleep(1);
						}
					}             
				}
				break;
		}
		sleep(1);
	}  
	close(sockfd);  
	exit(0);  
}  
