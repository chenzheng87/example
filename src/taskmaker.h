#include <stdio.h>
#include "timer.h"
#include "threadpool.h"
#include "taskprocessor.h"

class TaskMaker : public CTimer
{
   private:
	  threadpool thpool_p;
	  int flag_cnt; 
	  void OnTimer(){
		 if(flag_cnt == 1000) 
			flag_cnt = 0;
		 char str_p[4];
		 sprintf(str_p, "%d", flag_cnt++);
		 if(thpool_p)
	  		 thpool_add_work(thpool_p, taskProcessor, (void *)str_p);
	  }
   public:
	  TaskMaker(){
		 thpool_p = NULL;
		 flag_cnt = 0;
	  }
	  TaskMaker(long second, long microsecond, threadpool p)
		 : thpool_p(p), flag_cnt(0){
		 CTimer(second, microsecond);
	  }
	  void setThreadPool(threadpool p){
		 thpool_p = p;
	  }
};
