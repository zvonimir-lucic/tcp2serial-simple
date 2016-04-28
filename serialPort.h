#ifndef SERIALPORT_H_
#define SERIALPORT_H_

#include <semaphore.h>
#include <pthread.h>
#include <termios.h>
#include <map>
#include "mySemaphore.h"

#define UART_SYNTH_BUFF_SIZE 256

#define SERIAL_PORT_TIMEOUT_SYNTH 50	//in ms

class SerialPort {
public:
	SerialPort();
	virtual ~SerialPort();
	int Init();
	void Start();

	int SendStandardFrame(unsigned int moduleID, unsigned char* dataToSend, unsigned int dataToSendLenght);
	int SendRemoteFrame(unsigned int msgType);
	int SendDataToMotorsControl(unsigned char* dataToSend, unsigned int dataToSendLength);

	unsigned char commRXBuffer[UART_SYNTH_BUFF_SIZE];
	volatile bool respReceived;
	volatile char bytesReceived;

private:
	static void* startFun (void*);

    int fd;
    struct termios oldtio;
    struct termios newtio;
    static const char* MODEMDEVICE;

	void execute();

	enum baudRateType
	{
		BR1200 = 0000011,
	    BR1800 = 0000012,
		BR2400 = 0000013,
		BR4800 = 0000014,
		BR9600 =	0000015,
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



	int sendAndDontWaitForRecv(unsigned char bytesToSend, unsigned char muxLineVal);
	int sendAndWaitForRecv(unsigned char bytesToSend, unsigned char muxLineVal);
		
	char	outBuffer[UART_SYNTH_BUFF_SIZE]; 
	char	inBuffer[UART_SYNTH_BUFF_SIZE]; 

	int rfLevel;
	sem_t semMsgRecv;
	sem_t semMsgSent;
	sem_t readWaitTimeOuted;
	pthread_mutex_t lock;
	pthread_mutex_t globalComLock;
	unsigned int numTimeouts;
	volatile bool initOK;
	volatile unsigned int commRXBufferHead;
	unsigned int tuningInProgress;

	MySemaphore* semCtrl;

	char infoString[255];
};

#endif /*SERIALPORT_H_*/
