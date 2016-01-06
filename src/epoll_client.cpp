#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <stdio.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <string.h>

#define	TIME_OUT_TIME 10 
#define MAX_EVENTS 10

int main(int argc,char *argv[])  
{  
	int result, optval, byte, len = sizeof(int);  
	struct sockaddr_in address;     
	struct timeval time_limit;
	int sockfd, epoll_fd, maxfd = -1;
	fd_set fds;

	if(argc != 3){  
		printf("Usage: fileclient <address> <port>/n"); 
		return 0;  
	}  
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1){  
		perror("sock");  
		exit(EXIT_FAILURE);  
	}  

	struct epoll_event ev;
	struct epoll_event events[MAX_EVENTS];
	int nfds;
	char arg_str[100] = {0};	

	epoll_fd = epoll_create(MAX_EVENTS);
	if(epoll_fd == -1){
		perror("epoll_create failed");
		exit(EXIT_FAILURE);
	}	
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd=sockfd;
	// 向epoll注册server_sockfd监听事件
	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1){
		perror("epll_ctl:server_sockfd register failed");
		exit(EXIT_FAILURE);
	}

	bzero(&address,sizeof(address));  
	address.sin_family = AF_INET;  
	address.sin_port = htons(atoi(argv[2]));  
	inet_pton(AF_INET,argv[1],&address.sin_addr);  

	ioctl(sockfd, FIONBIO);	
	while(true){ 
		result = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
		if(result < 0){
			time_limit.tv_sec = TIME_OUT_TIME;
			time_limit.tv_usec = 0;
			FD_ZERO(&fds);
			FD_SET(sockfd, &fds);
			maxfd = sockfd + 1;
			result = select(maxfd, NULL, &fds, NULL, &time_limit);
			if(result < 1){
				perror("connect failed, sleep 5 secs");
				sleep(5);
			}else{
				result = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, (socklen_t *)&len);
				if(result == 0){
					break;	
				}
			}
		}else{
			break;
		}
	}


	while(true){
		nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if(nfds == -1){
			perror("start epoll_wait failed");
			exit(EXIT_FAILURE);
		}
		int i;
		for(i=0;i<nfds;i++){
			if(events[i].data.fd == sockfd){
				len = recv(sockfd, arg_str, 100, 0);
				if(len < 0){
					perror("receive from client failed");
					exit(EXIT_FAILURE);
				}
				printf("receive from client:%s\n", arg_str);
				//send(client_sockfd,"I have received your message.",30,0);
			}
		}
	} 
	close(sockfd);  
	exit(0);  
}  
