#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "epoll_client.h"
#include "threadpool.h"

extern struct net_pool *netpool_p;
/*
   Function Declarations for builtin shell commands:
   */
static int add_net(char **args);
static int del_net(char **args);
static int print_net(char **args);
static int lsh_help(char **args);
static int lsh_exit(char **args);

/*
   List of builtin commands, followed by their corresponding functions.
   */
char *builtin_str[] = {
	"addnet",
	"delnet",
	"printnet",
	"help",
	"exit"
};

int (*builtin_func[]) (char **) = {
	&add_net,
	&del_net,
	&print_net,
	&lsh_help,
	&lsh_exit
};

int lsh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}

/*
   Builtin function implementations.
   */

static int add_net(char **args)
{
	if (args[1] == NULL || args[2] == NULL) {
		fprintf(stderr, "expected argument to \"add_net\"\n");
		return -1;
	} else {
//		fprintf(stdout, "ip: %s, port: %s\n", args[1], args[2]);
		net_context *context = (net_context*)malloc(sizeof(net_context));
		net_context_init(context);
		strncpy(context->ip_addr, args[1], IPV4_ADDR_LEN);
		context->port = atoi(args[2]);
		if (add_to_network(context) != 0) {
			perror("addnet");
			return 0;
		}
	}
	return 0;
}

static int del_net(char **args)
{
	if (args[1] == NULL || args[2] == NULL) {
		fprintf(stderr, "expected argument to \"del_net\"\n");
		return -1;
	} else {
		net_context *ptr = netpool_p->queue->context_head;
		while(ptr != NULL){
			if(strcmp(ptr->ip_addr, args[1]) == 0 && ptr->port == atoi(args[2]))
				if(del_from_network(ptr) == 0){
					fprintf(stdout, "delnet: successed.\n");				
					return 0;
				}
			ptr = ptr->next;
		}
		fprintf(stdout, "delnet: net not exist\n");
	}
	return 0;
}

static int print_net(char **args)
{
	net_context *ptr = netpool_p->queue->context_head;
	while(ptr != NULL){
		fprintf(stdout, "ip: %s, port: %d\n", ptr->ip_addr, ptr->port);	
		ptr = ptr->next;
	}	
	return 0;
}
/**
  @brief Builtin command: print help.
  @param args List of args.  Not examined.
  @return Always returns 1, to continue executing.
  */
static int lsh_help(char **args)
{
	int i;
	printf("Stephen Brennan's LSH\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for (i = 0; i < lsh_num_builtins(); i++) {
		printf("  %s\n", builtin_str[i]);
	}

	printf("Use the man command for information on other programs.\n");
	return 1;
}

/**
  @brief Builtin command: exit.
  @param args List of args.  Not examined.
  @return Always returns 0, to terminate execution.
  */
static int lsh_exit(char **args)
{
	return -1;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
  */
int lsh_launch(char **args)
{
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(args[0], args) == -1) {
			perror("lsh");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// Error forking
		perror("lsh");
	} else {
		// Parent process
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 0;
}

/**
  @brief Execute shell built-in or launch program.
  @param args Null terminated list of arguments.
  @return 1 if the shell should continue running, 0 if it should terminate
  */
int lsh_execute(char **args)
{
	int i;

	if (args[0] == NULL) {
		// An empty command was entered.
		return 0;
	}

	for (i = 0; i < lsh_num_builtins(); i++) {
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}

	return lsh_launch(args);
}

#define LSH_RL_BUFSIZE 1024
/**
  @brief Read a line of input from stdin.
  @return The line from stdin.
  */
char *lsh_read_line(void)
{
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = (char *)malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		// Read a character
		c = getchar();

		// If we hit EOF, replace it with a null character and return.
		if (c == EOF || c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		// If we have exceeded the buffer, reallocate.
		if (position >= bufsize) {
			bufsize += LSH_RL_BUFSIZE;
			buffer = (char *)realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
  @brief Split a line into tokens (very naively).
  @param line The line.
  @return Null-terminated array of tokens.
  */
char **lsh_split_line(char *line)
{
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char **tokens = (char **)malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens) {
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, LSH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += LSH_TOK_BUFSIZE;
			tokens = (char **)realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, LSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

/**
  @brief Loop getting input and executing it.
  */
void lsh_loop(void)
{
	char *line;
	char **args;
	int status;

	threadpool thp = thpool_init(3);
	network_init(thp);

	do {
		printf("> ");
		line = lsh_read_line();
		args = lsh_split_line(line);
		status = lsh_execute(args);

		free(line);
		free(args);
	} while (!status);
}
