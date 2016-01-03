#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <sys/time.h>

class CTimer
{
private:
    pthread_t thread_timer;
    long m_second, m_microsecond;

    static void *OnTimer_stub(void *p){
	    (static_cast<CTimer*>(p))->thread_proc();
		return (void *)0;
    }
    void thread_proc();
    virtual void OnTimer();
public:
    CTimer();
    CTimer(long second, long microsecond);
    virtual ~CTimer();
    void SetTimer(long second, long microsecond);
    void StartTimer();
    void StopTimer();
};

#endif
