#ifndef MYSEMAPHORE_H_
#define MYSEMAPHORE_H_

#include <semaphore.h>

class MySemaphore
{
public:
	MySemaphore(int n);
	~MySemaphore(void);
	int TimeWaitMs(unsigned int waitMs);
	void Wait();
	void Signal();
private:
	sem_t _hndl;
};

#endif /* MYSEMAPHORE_H_ */
