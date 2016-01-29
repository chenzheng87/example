#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "threadpool.h"
#include "epoll_client.h"

//for multi thread errno
extern int *__errno_location(void);
#define errno (*__errno_location())

static int epfd;
static pthread_t event_tids[NETWORK_READ_THREAD_COUNT];
static pthread_t state_tids[NETWORK_STATE_THREAD_COUNT];
struct net_pool *netpool_p = NULL;

void net_context_init(net_context *context) {
	pthread_mutex_init(&context->mutex_buf, NULL);
	pthread_mutex_init(&context->mutex_state, NULL);
	gettimeofday(&context->tv, NULL);
	context->state = DISCED;
}

void net_context_destroy(net_context *context) {
	pthread_mutex_destroy(&context->mutex_buf);
	pthread_mutex_destroy(&context->mutex_state);
}


static void setnonblocking(int sock) {
	int opts, ret;
	opts = fcntl(sock, F_GETFL, 0);
   /* if(opts < 0) {*/
		//fprintf(stdout, "fcntl(sock,GETFL)\n");
		//exit(1);
	/*}*/

	opts = opts | O_NONBLOCK;
	ret = fcntl(sock, F_SETFL, opts);
   /* if(ret < 0) {*/
		//fprintf(stdout, "fcntl(sock,SETFL,opts)\n");
		//exit(1);
	/*}*/
}

int add_to_network(net_context *context) {
	context->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (context->sockfd == -1) {
		fprintf(stdout, "can't create socket.\n");
		return (-1);
	}
	setnonblocking(context->sockfd);
	int optionVal = 0;
	setsockopt(context->sockfd, SOL_SOCKET, SO_REUSEADDR,  &optionVal, sizeof(optionVal));
	
	bzero(&context->address, sizeof(context->address));
	context->address.sin_family = AF_INET;
	context->address.sin_port = htons(context->port);
	inet_pton(AF_INET, context->ip_addr, &context->address.sin_addr);
	//fprintf(stdout, "ip: %s, port: %d\n", context->ip_addr, context->port);

	int ret = connect(context->sockfd, (struct sockaddr *)&context->address, sizeof(context->address)); 
	if(ret != 0){
		perror("connect failed");

		struct epoll_event ev;
		ev.data.ptr = context;
		ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
		epoll_ctl(epfd, EPOLL_CTL_ADD, context->sockfd, &ev);

		pthread_mutex_lock(&context->mutex_state);	
		context->state = DISCED;
		pthread_mutex_unlock(&context->mutex_state);
	} else {
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.ptr = context;
		epoll_ctl(epfd, EPOLL_CTL_ADD, context->sockfd, &ev); 
		
		pthread_mutex_lock(&context->mutex_state);
		context->state = CONNED;
		pthread_mutex_unlock(&context->mutex_state);
	}
	if(netpool_p->queue->count == 0) {
		netpool_p->queue->context_head = context;
		netpool_p->queue->context_tail = context;
		context->next = NULL;
	} else {
		netpool_p->queue->context_tail->next = context;
		context->next = NULL;
	}

	netpool_p->queue->count ++;
	return 0;
}

int del_from_network(net_context *context) {
	if(context == NULL)
		return -1;
	if(netpool_p->queue->count == 0)
		return -1;

	int fd = context->sockfd;
	struct epoll_event ev;
	ev.data.ptr = context;
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
	close(fd);
	context->sockfd = -1;

	net_context	*ptr = netpool_p->queue->context_head;
	net_context *previous = NULL;
	//pthread_mutex_lock(&netpool_p->queue->mutex_queue);
	ptr = netpool_p->queue->context_head;
	while(ptr) {
		if(ptr == context) {
			if(previous != NULL)
				previous->next = ptr->next;
			else
				netpool_p->queue->context_head = ptr->next;
			net_context_destroy(ptr);
			free(ptr);
			netpool_p->queue->count --;
			return 0;
		}
		previous = ptr;
		ptr = ptr->next;
	}	
	//pthread_mutex_unlock(&netpool_p->queue->mutex_queue);
	
	return -1;
}

static void reconnect(net_context *context) {
	//del previous connect epoll event and file description
	struct epoll_event ev;
	ev.data.ptr = context;
	epoll_ctl(epfd, EPOLL_CTL_DEL, context->sockfd, &ev);
	close(context->sockfd);

	context->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (context->sockfd == -1) {
		fprintf(stdout, "reconnect can't create socket.\n");
		return;
	}
	int ret = connect(context->sockfd, (struct sockaddr *)&context->address, sizeof(context->address)); 
	if(ret != 0){
		perror("reconnect failed");
		ev.data.ptr = context;
		ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
		epoll_ctl(epfd, EPOLL_CTL_ADD, context->sockfd, &ev);
		
		pthread_mutex_lock(&context->mutex_state);
		context->state = DISCED;
		pthread_mutex_unlock(&context->mutex_state);
	} else {
		fprintf(stdout, "reconnect successed.\n");
		ev.data.ptr = context;
		ev.events = EPOLLIN | EPOLLET;
		epoll_ctl(epfd, EPOLL_CTL_ADD, context->sockfd, &ev);

		pthread_mutex_lock(&context->mutex_state);
		context->state = CONNED;
		pthread_mutex_unlock(&context->mutex_state);
	}
}

static void* get_network_event(void *param) {
	long network_event_id;
	int i, read_num;

	network_event_id = (long) param;
	fprintf(stdout, "begin thread get_network_event: %ld\n", network_event_id);

	net_context *context = NULL;
	int nfds;
	struct epoll_event events[MAX_CLIENT_COUNT];

	while(1) {
		nfds = epoll_wait(epfd, events, MAX_CLIENT_COUNT, 1000000);
		for(i = 0; i < nfds; i++) {
			context = (net_context *)events[i].data.ptr;
			if(events[i].events & EPOLLIN) {
				pthread_mutex_lock(&context->mutex_buf);
				read_num = read(context->sockfd, context->buf, MAX_READ_BUF_SIZE);
				if(read_num == 0) {
					pthread_mutex_unlock(&context->mutex_buf);
					fprintf(stdout, "read_num == 0, connect closed!\n");
					pthread_mutex_lock(&context->mutex_state);
					context->state = RECONN;
					pthread_mutex_unlock(&context->mutex_state);
					break;
				}
				gettimeofday(&context->tv, NULL);			
				fprintf(stdout, "%s, pid %u\n", context->buf, (int)pthread_self());
				pthread_mutex_unlock(&context->mutex_buf);
   /*             if(thpool_p)*/
					/*thpool_add_work(thpool_p, taskProcessor, (void *)job);*/
			
			}
			// connect failed				
			else if (events[i].events & (EPOLLHUP | EPOLLERR)) {
				pthread_mutex_lock(&context->mutex_state);
				context->state = RECONN;
				pthread_mutex_unlock(&context->mutex_state);
				fprintf(stdout, "EPOLLHUP | EPOLLERR, pid %u\n", (int)pthread_self());
			}
			// connect successed
			else if(events[i].events & EPOLLOUT) {
				struct epoll_event ev;
				ev.events = EPOLLIN | EPOLLET;
				ev.data.ptr = context;
				epoll_ctl(epfd, EPOLL_CTL_MOD, context->sockfd, &ev); 
				
				pthread_mutex_lock(&context->mutex_state);
				context->state = CONNED;
				pthread_mutex_unlock(&context->mutex_state);
				fprintf(stdout, "EPOLLOUT, pid %u\n", (int)pthread_self());
			}
   /*         else if(events[i].events & EPOLLHUP) {*/
				//fprintf(stdout, "EPOLLHUP, pid %u\n", (int)pthread_self());
				////del_from_network(context);
			//}
			//else if(events[i].events & EPOLLERR) {
				//fprintf(stdout, "EPOLLHUP, pid %u\n", (int)pthread_self());
				////del_from_network(context);
			/*}*/
			else {
				fprintf(stdout, "[warming]other epoll event: %d\n", events[i].events);
			}
		}
	}
	return NULL;
}

static void get_network_state(int signo) {
	//while(1) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		gettimeofday(&tv, NULL);
		if(netpool_p != NULL) {
			net_context	*ptr = netpool_p->queue->context_head;
			//pthread_mutex_lock(&netpool_p->queue->mutex_queue);
			ptr = netpool_p->queue->context_head;
			while(ptr) {
				pthread_mutex_lock(&ptr->mutex_state);
				switch (ptr->state) {
					case CONNED:
						if(tv.tv_sec - ptr->tv.tv_sec > RECONNECT_SECS)
							ptr->state = NODATA;
						pthread_mutex_unlock(&ptr->mutex_state);
						break;	
					case RECONN:
						pthread_mutex_unlock(&ptr->mutex_state);
						reconnect(ptr);
						break;
					case NODATA:
					case DISCED:
					default:
						pthread_mutex_unlock(&ptr->mutex_state);
						break;
				}
				ptr = ptr->next;
			}	
			//pthread_mutex_unlock(&netpool_p->queue->mutex_queue);	
		}

	   /* tv.tv_sec = RECONNECT_SECS - 1;*/
		//tv.tv_usec = 0;
		/*select(0, NULL, NULL, NULL, &tv); */
		//fprintf(stdout, "%s, state check\n", __FUNCTION__);
	//}
	return;
}

int network_init(thpool_* thpool_p) {
	int ret, i;
	pthread_attr_t tattr;
	/* Initialize with default */
	ret = pthread_attr_init(&tattr);
	if(ret != 0) {
		perror("Error initializing thread attribute [pthread_attr_init(3C)] ");
		return (-1);
	}
	/* Make it a bound thread */
	ret = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);
	if(ret != 0) {
		perror("Error making bound thread [pthread_attr_setscope(3C)] ");
		return (-1);
	}

	signal(SIGALRM, get_network_state);
	struct itimerval tv;
	memset(&tv, 0, sizeof(tv));
	tv.it_value.tv_sec = RECONNECT_SECS;
	tv.it_value.tv_usec = 0;
	tv.it_interval.tv_sec = RECONNECT_SECS;
	tv.it_interval.tv_usec = 0;
	ret = setitimer(ITIMER_REAL, &tv, NULL);
	if(ret < 0) {
		fprintf(stdout, "Set timer failed!\n");
		return -1;
	}

	epfd = epoll_create(MAX_CLIENT_COUNT);
	netpool_p = (struct net_pool*)malloc(sizeof(struct net_pool));
	netpool_p->queue = (struct net_queue*)malloc(sizeof(struct net_queue));
	//pthread_mutex_init(&netpool_p->queue->mutex_queue, NULL);
	netpool_p->queue->context_head = NULL;
	netpool_p->queue->context_tail = NULL;
	netpool_p->queue->count = 0;

	for (i = 0; i < NETWORK_READ_THREAD_COUNT; i++) {
		ret = pthread_create(&event_tids[i], &tattr, get_network_event, (void*)i);
		if(ret != 0)
			return (-1);
		pthread_detach(event_tids[i]);
	}

	return 0;	
}

void network_destroy(void) {
	fprintf(stdout, "begin shutdown network ...");
	
	int i, ret;
	for (i = 0; i < NETWORK_READ_THREAD_COUNT; i++) {
		ret = pthread_cancel(event_tids[i]);
		if(ret != 0)
			return;
	}

	for (i = 0; i < NETWORK_STATE_THREAD_COUNT; i++) {
		ret = pthread_cancel(state_tids[i]);
		if(ret != 0)
			return;
	}		

	struct net_context *p;	
	//pthread_mutex_lock(&netpool_p->queue->mutex_queue);
	while(netpool_p->queue->count != 0) {
		p = netpool_p->queue->context_head;
		pthread_mutex_destroy(&p->mutex_buf);
		pthread_mutex_destroy(&p->mutex_state);
		netpool_p->queue->context_head = p->next;
		free(p);
		netpool_p->queue->count --;
	}	
	//pthread_mutex_unlock(&netpool_p->queue->mutex_queue);	
	//pthread_mutex_destroy(&netpool_p->queue->mutex_queue);

	return;
}
