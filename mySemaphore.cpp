/*
 * LinuxSemaphore.cpp
 *
 *  Created on: Nov 12, 2010
 *      Author: mbenco
 */


#include "mySemaphore.h"

#include <time.h>
#include <assert.h>
#include <errno.h>


MySemaphore::MySemaphore(int n)
{

	sem_init(&_hndl, 0, n);
}

MySemaphore::~MySemaphore(void)
{
	sem_destroy(&_hndl);
}

void MySemaphore::Wait()
{
	sem_wait(&_hndl);
}

void MySemaphore::Signal()
{
	sem_post(&_hndl);
}

int MySemaphore::TimeWaitMs(unsigned int waitMs)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	unsigned long long tt = ((unsigned long long) ts.tv_sec * 1000000000L) + ts.tv_nsec;
	tt += ((unsigned long long)waitMs * 1000000L);
	if(tt < 0)
	{
		return -1;
	}
    if (!tt)
    {
    	ts = (struct timespec) {0, 0};
    }
    ts.tv_sec = tt / 1000000000L;
    ts.tv_nsec = tt % 1000000000L;
	return sem_timedwait(&_hndl, &ts);
}
