#include "serialPort.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <syscall.h>

SerialPort::SerialPort()
{
	sem_init(&semMsgRecv_, 0, 0);
	sem_init(&semMsgSent_, 0, 0);
	sem_init(&readWaitTimeOuted_, 0, 0);

	pthread_mutex_init(&lock_, NULL);

	semCtrl_ = new MySemaphore(0);
}

SerialPort::~SerialPort()
{
	sem_destroy(&semMsgRecv_);
	sem_destroy(&semMsgSent_);
	sem_destroy(&readWaitTimeOuted_);

	pthread_mutex_destroy(&lock_);

	delete semCtrl_;
}

void SerialPort::start()
{
	unsigned long tid;
	pthread_create(&tid, NULL, startFun, (void*) this);
}

void* SerialPort::startFun(void* pvParam)
{
	SerialPort* service = (SerialPort*) pvParam;
	printf("SerialPort tid  = %ld\n", syscall(SYS_gettid));
	service->execute();
	return NULL;
}

void SerialPort::execute()
{
	commRXBufferHead_ = 0;

	while(!initOK_)
	{
		semCtrl_->TimeWaitMs(100);
	}

    while (1)
    {
    	sem_wait(&semMsgSent_);
    	int byteCount = read(fd_, inBuffer, 3);
		errno = 0;
    	sem_trywait(&readWaitTimeOuted_);
    	if((byteCount > 1) && (errno == EAGAIN))
    	{
    		while((inBuffer[1] + 3) > byteCount)
			{
				byteCount += read(fd, inBuffer + byteCount, inBuffer[1] + 3 - byteCount);
				errno = 0;
		    	sem_trywait(&readWaitTimeOuted);
		    	if(errno != EAGAIN)
		    	{
		    		break;
		    	}
			}

    		sem_trywait(&semMsgSent_);
    		sem_post(&semMsgRecv_);
    	}
    }
}

int SerialPort::sendAndDontWaitForRecv()
{
	if(initOK == false) return -1;
	pthread_mutex_lock(&globalComLock);
    tcflush(fd, TCIOFLUSH);
	write(fd, outBuffer, bytesToSend);
	pthread_mutex_unlock(&globalComLock);
	return 0;
}

int SerialPort::sendAndWaitForRecv(unsigned int timeout)
{	
	if(initOK == false)
	{
		return -1;
	}
	pthread_mutex_lock(&globalComLock);

    tcflush(fd, TCIOFLUSH);
    memset(inBuffer, 0, 255);
	sem_trywait(&readWaitTimeOuted);
	write(fd, outBuffer, bytesToSend);
	sem_post(&semMsgSent);
	
	if(sem_timedwait(&semMsgRecv, &ts) != 0)
	{

		sem_post(&readWaitTimeOuted);
		++numTimeouts;
		sem_trywait(&semMsgSent);

		pthread_mutex_unlock(&globalComLock);
		return -1;
	}

	int ret = 0;
	pthread_mutex_unlock(&globalComLock);

	return ret;			
}

int SerialPort::Open()
{
    // Open modem device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.

    fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0)
    {
    	perror(MODEMDEVICE);
    	return -1;
    }
    tcgetattr(fd,&oldtio); 			// save current serial port settings
    memset(&newtio, 0, sizeof(newtio)); // clear struct for new port settings

    newtio.c_cflag = B921600 | CS8 | CLOCAL | CREAD;

    // IGNPAR  : ignore bytes with parity errors
    // ICRNL   : map CR to NL (otherwise a CR input on the other computer will not terminate input)
    newtio.c_iflag = IGNPAR;

    // Raw output.
    newtio.c_oflag = 0;

    //   ICANON  : enable canonical input
    //   disable all echo functionality, and don't send signals to calling program
    newtio.c_lflag = 0;//ICANON;


    //   initialize all control characters
    //   default values can be found in /usr/include/termios.h, and are given
    //   in the comments, but we don't need them here

    newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */
    newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
    newtio.c_cc[VERASE]   = 0;     /* del */
    newtio.c_cc[VKILL]    = 0;     /* @ */
    newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
    newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 3;     /* blocking read until 3 character arrives */
    newtio.c_cc[VSWTC]    = 0;     /* '\0' */
    newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */
    newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
    newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
    newtio.c_cc[VEOL]     = 0;     /* '\0' */
    newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
    newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
    newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
    newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
    newtio.c_cc[VEOL2]    = 0;     /* '\0' */

    //now clean the modem line and activate the settings for the port
    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);

    //terminal settings done, now handle input
    //loop until we have a terminating condition TODO HIGHEST ovo nije dobro ako imamo blocking read !!!!!
    // ali za sada iaonako zanemarujemo stop !!!!!!!!!
    fcntl(fd, F_SETFL, 0);
    initOK = true;
    return 0;
}

