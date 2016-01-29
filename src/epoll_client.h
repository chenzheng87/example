#ifndef EPOLL_CLIENT
#define EPOLL_CLIENT

#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>

#define MAX_CLIENT_COUNT		1024
#define MAX_READ_BUF_SIZE		1024 * 8
#define NETWORK_READ_THREAD_COUNT	2
#define NETWORK_STATE_THREAD_COUNT	1
#define SOCKET_ERROR			-1
#define IPV4_ADDR_LEN			16
#define RECONNECT_SECS			30
typedef enum {
	CONNED,
	DISCED,
	RECONN,
	NODATA
} net_state;

typedef struct job_context {
	pthread_t pid;
	char buf[MAX_READ_BUF_SIZE];
	int buf_len;
	pthread_mutex_t mutex; 
} job_context;

/* net buffer semaphore */ 
typedef struct net_context {
	//net information
	char ip_addr[IPV4_ADDR_LEN];
	int port;
	int sockfd;
	//net buffer and buffer mutex
	char buf[MAX_READ_BUF_SIZE];
	int buf_len;
	pthread_mutex_t mutex_buf; 
	//net state and state mutex
	net_state state;
	struct timeval tv;
	pthread_mutex_t mutex_state;	
	//net reconnect info
	struct sockaddr_in address;
	//fine next net_context in management
	struct net_context *next;
} net_context;

typedef struct net_queue {
	net_context *context_head;
	net_context *context_tail;
	int count;
	//pthread_mutex_t mutex_queue;
} net_queue;

typedef struct thpool_ thpool_;

struct net_pool {
	net_queue *queue;
	thpool_ *thpool_p;	
};

void net_context_init(net_context *context);
void net_context_destroy(net_context *connect);
int add_to_network(net_context *context);
int del_from_network(net_context *context);
int network_init(thpool_ *);
void network_destroy();

#endif
