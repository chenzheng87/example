#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "taskprocessor.h"

#define MAX_EPOLL_EVENT_COUNT	500			// 同时监听的 epoll 数
#define MAX_READ_BUF_SIZE		1024 * 8 	// 最大读取数据 buf
#define NETWORK_READ_THREAD_COUNT	2
#define SOCKET_ERROR	-1
#define IPV4ADDRLEN		16

static int epfd;
static int listenfd;
static pthread_t tids[NETWORK_READ_THREAD_COUNT];

typedef struct job_context {
	pthread_t pid;
	char buf[MAX_READ_BUF_SIZE];
	int buf_len;
	pthread_mutex_t mutex; 
} job_context;

/* net buffer semaphore */ 
typedef struct net_context {
	char ip_addr[IPV4ADDRLEN];
	int port;
	int sockfd;
	char buf[MAX_READ_BUF_SIZE];
	int buf_len;
	pthread_mutex_t mutex; 
} net_context;

int context_init(net_context *context) {
	pthread_mutex_init(&context->net_buf->mutex, NULL);
}

void context_destroy(net_context *context) {
	pthread_mutex_destroy(&context->net_buf->mutex, NULL);
}

void setnonblocking(int sock) {
	int opts;
	opts = fcntl(sock, F_GETFL, 0);
	if(opts < 0) {
		printf("fcntl(sock,GETFL)");
		exit(1);
	}

	opts = opts | O_NONBLOCK;
	if(fcntl(sock, F_SETFL, opts)<0) {
		printf("fcntl(sock,SETFL,opts)");
		exit(1);
	}
}

void* get_network_event(void *param) {
	long network_event_id;
	int i, sockfd, read_num;

	network_event_id = (long) param;
	printf("begin thread get_network_event: %ld", network_event_id);

	net_context *context = NULL;
	unsigned nfds;
	struct epoll_event now_ev, ev, events[MAX_EPOLL_EVENT_COUNT];

	while(1) {
		nfds = epoll_wait(epfd, events, MAX_EPOLL_EVENT_COUNT, 100000);
		for(i = 0; i < nfds; i++) {
			now_ev = events[i];
			context = (net_context *)now_ev.data.ptr;
			if(now_ev.events & EPOLLIN) {
				pthread_mutex_lock(&context->net_buf->mutex);
				read_num = read(context->fd, context->buf, MAX_READ_BUF_SIZE);
				if(read_num == 0){
					pthread_mutex_unlock(&context->net_buf->mutex);
					printf("read_num == 0, connect may closed\n");
					del_from_network(context);
					break;
				}
				
				pthread_mutex_lock(&
				memcpy();
				pthread_mutex_unlock(&context->net_buf->mutex);
				if(thpool_p)
					thpool_add_work(thpool_p, taskProcessor, (void *)job);
				
			} else if(now_ev.events & EPOLLOUT) {
/* 				sockfd = context->fd;
				struct epoll_event ev;
				ev.events = EPOLLIN | EPOLLET;
				ev.data.ptr = context;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev); */
				printf("else if(now_ev.events & EPOLLOUT)\n");
			} else if(now_ev.events & EPOLLHUP) {
				printf("else if(now_ev.events & EPOLLHUP)\n");
				del_from_network(context);
			} else if(now_ev.events & EPOLLERR) {
				printf("else if(now_ev.events & EPOLLHUP)\n");
				del_from_network(context);
			} else {
				printf("[warming]other epoll event: %d\n", now_ev.events);
			}
		}
	}

	return NULL;
}

int start_network(void) {
	int ret, i, sockfd;
	struct epoll_event ev;

	pthread_attr_t tattr;
	/* Initialize with default */
	if(ret = pthread_attr_init(&tattr)) {
		perror("Error initializing thread attribute [pthread_attr_init(3C)] ");
		return (-1);
	}
	/* Make it a bound thread */
	if(ret = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM)) {
		perror("Error making bound thread [pthread_attr_setscope(3C)] ");
		return (-1);
	}

	epfd = epoll_create(MAX_EPOLL_EVENT_COUNT);

	pthread_t tids_get_network_event[NETWORK_READ_THREAD_COUNT];
	for (int i = 0; i < NETWORK_READ_THREAD_COUNT; i++) {
		ret = pthread_create(&tids_get_network_event[i], &tattr, get_network_event, (void*)i);
		if(ret != 0)
			return (-1);
		pthread_detach(tids_get_network_event[i]);
	}

	get_network_event(NULL);
	return 0;
}

void shutdown_network(void) {
	printf("begin shutdown network ...");
}

int add_to_network(context *context) {
	int ret;
	struct sockaddr_in address;
	context->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (context->sockfd == -1) {
		printf("can't create socket.\n");
		return (-1);
	}
	
	//把socket设置为非阻塞方式
	_setnonblocking(context->sockfd);
	int optionVal = 0;
	setsockopt(context->sockfd, SOL_SOCKET, SO_REUSEADDR,  &optionVal, sizeof(optionVal));
	//设置与要处理的事件相关的文件描述符
	ev.data.ptr = context;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
	epoll_ctl(epfd, EPOLL_CTL_ADD, context->sockfd, &ev);

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(SERV_PORT);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = connect(context->sockfd, (struct sockaddr *)&address, sizeof(address)); 
	if(ret < 0){
		return (-1);
	}
}

int del_from_network(context *context) {
	int fd = context->fd;
	if (fd != -1) {
		struct epoll_event ev;
		ev.data.fd = fd;
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
		close_socket(fd);
		context->fd = -1;
	}
}