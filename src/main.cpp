#include <stdio.h>
#include <pthread.h>
#include "threadpool.h"
#include <iostream>
#include <unistd.h>
#include <vector>
#include "taskmaker.h"
#include "epoll_client.h"

using namespace std;

void lsh_loop(void);

int main()
{
	//thread pool start
	/*puts("Making threadpool with 4 threads");*/
	//threadpool thpool = thpool_init(4);

	//TaskMaker t1(1, 0, thpool);
	/*t1.StartTimer();*/

	lsh_loop();
	return 0;
}
