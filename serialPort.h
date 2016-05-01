#ifndef SERIALPORT_H_
#define SERIALPORT_H_

#include <semaphore.h>
#include <pthread.h>
#include <termios.h>
#include <map>
#include "mySemaphore.h"

#define BUFF_SIZE 1024

class SerialPort
{
public:
	enum baudRateType
	{
		BR1200 = 0000011,
		BR1800 = 0000012,
		BR2400 = 0000013,
		BR4800 = 0000014,
		BR9600 = 0000015,
		BR19200 = 0000016,
		BR38400 = 0000017,
		BR57600 = 0010001,
		BR115200 = 0010002,
		BR230400 = 0010003,
		BR460800 = 0010004,
		BR500000 = 0010005,
		BR576000 = 0010006,
		BR921600 = 0010007
	};

	enum stopBitsType
	{
		One,
		Two
	};

	enum bitsInByteType
	{
		Bits5,
		Bits6,
		Bits7,
		Bits8
	};

	enum parityType
	{
		None,
		Odd,
		Even
	};

	SerialPort();
	virtual ~SerialPort();
	int Open();

private:
	void start();
	static void* startFun (void*);
	void execute();
	int sendAndDontWaitForRecv();
	int sendAndWaitForRecv(unsigned int timeout = 100);

    int fd_;
    struct termios oldtio_;
    struct termios newtio_;
    char* modemDev_;
		
	char outBuffer_[BUFF_SIZE];
	char inBuffer_[BUFF_SIZE];

	sem_t semMsgRecv_;
	sem_t semMsgSent_;
	sem_t readWaitTimeOuted_;
	pthread_mutex_t lock_;
	unsigned int numTimeouts_;
	volatile bool initOK_;
	volatile unsigned int commRXBufferHead_;

	MySemaphore* semCtrl_;

	baudRateType baudRate_;
	bitsInByteType bitsInByte_;
	parityType parity_;
	stopBitsType stopBits_;
};

#endif /*SERIALPORT_H_*/
