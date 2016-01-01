/* 
 * WHAT THIS EXAMPLE DOES
 * 
 * We create a pool of 4 threads and then add 40 tasks to the pool(20 task1 
 * functions and 20 task2 functions). task1 and task2 simply print which thread is running them.
 * 
 * As soon as we add the tasks to the pool, the threads will run them. It can happen that 
 * you see a single thread running all the tasks (highly unlikely). It is up the OS to
 * decide which thread will run what. So it is not an error of the thread pool but rather
 * a decision of the OS.
 * 
 * */

#include <stdio.h>
#include <pthread.h>
#include "threadpool.h"
#include <iostream>
#include <unistd.h>
#include "timer.h"

using namespace std;

void* task1(void *){
	printf("Thread #%u working on task1\n", (int)pthread_self());
}


void* task2(void *){
	printf("Thread #%u working on task2\n", (int)pthread_self());
}


int main()
{
    CTimer t1(1,0),t2(1,0);    //构造函数，设两个定时器，以1秒为触发时间。参数1是秒，参数2是微秒。
    t1.StartTimer();
    t2.StartTimer();
    sleep(10);

	//thread pool start
	puts("Making threadpool with 4 threads");
	threadpool thpool = thpool_init(4);

	puts("Adding 40 tasks to threadpool");
	int i;
	for (i=0; i<20; i++){
		thpool_add_work(thpool, task1, NULL);
		thpool_add_work(thpool, task2, NULL);
	};

	puts("Killing threadpool");
	thpool_destroy(thpool);
	
	return 0;
}