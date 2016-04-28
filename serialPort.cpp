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
	sem_init(&semMsgRecv, 0, 0);	
	sem_init(&semMsgSent, 0, 0);
	sem_init(&readWaitTimeOuted, 0, 0);

	pthread_mutex_init(&lock, NULL);	
	pthread_mutex_init(&globalComLock, NULL);

	semCtrl = new MySemaphore(0);
}

SerialPort::~SerialPort()
{
	sem_destroy(&semMsgRecv);
	sem_destroy(&semMsgSent);
	sem_destroy(&readWaitTimeOuted);

	pthread_mutex_destroy(&lock);
	pthread_mutex_destroy(&globalComLock);

	delete semCtrl;
}

void SerialPort::Start()
{
	unsigned long tid;
	pthread_create(&tid, NULL, startFun, (void*) this);
}

void* SerialPort::startFun(void* pvParam)
{
	CANIntfBrdComm* service = (CANIntfBrdComm*) pvParam;
	printf("CANIntfBrdComm tid  = %ld\n", syscall(SYS_gettid));
	service->execute();
	return NULL;
}

void SerialPort::execute()
{
	char inBuffer[1000];
	int n;
	unsigned int threadCntr = 0;
	char statString[255];

	commRXBufferHead = 0;

    while (1)
    {
    	sprintf(statString, "CAN thread: %d", threadCntr++);
    	TCS_ObjectFactory::Inst().GetThreadStats()->WriteToStats(1, statString);
    	sem_wait(&semMsgSent);
    	int byteCount = read(fd, inBuffer, 3);
		errno = 0;
    	sem_trywait(&readWaitTimeOuted);
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

    		if((inBuffer[1] + 3) != byteCount)
			{
    			sprintf(infoString, "[CAN brd intf] Wrong num of bytes read [%d/%d], receive failed", byteCount, (inBuffer[1] + 3));
    			TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog(infoString);
    		}
    		else if(inBuffer[inBuffer[1] + 2] != generateChecksum((unsigned char *)inBuffer, byteCount - 1))
    		{
    			sprintf(infoString, "[CAN brd intf] Wrong checksum received in %d bytes", byteCount);
    			TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog(infoString);
    		}
    		else
    		{
    			if((inBuffer[0] & 0xC0) == 0x40) //modules odgovor
    			{
    				if(((inBuffer[0] & 0x3F) > 0) && ((inBuffer[0] & 0x3F) < 4))
    				{
						for(n = 0;n < byteCount;n++ )
						{
							commRXBuffer[n] = inBuffer[n];
						}
						respReceived = true;
						bytesReceived = byteCount;
    				}
    				else
    				{
    					if(inBuffer[0] & 0x20)
						{
    		    			sprintf(infoString, "[CAN brd intf] LPC didn't recognize command %02X\n\r", inBuffer[0] & 0x1F);
    		    			TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog(infoString);
						}
    					else
    					{
    		    			TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog("[CAN brd intf] Unknown command received");
    					}
    				}
    			}
    			else if((inBuffer[0] & 0xC0) == 0x00)//motors odgovor
    			{
    				if((inBuffer[0] > 0) && (inBuffer[0] < 0x40))
					{
						for(n = 0;n < byteCount;n++ )
						{
							commRXBuffer[n] = inBuffer[n];
						}
						respReceived = true;
						bytesReceived = byteCount;
					}
					else
					{
	    				TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog("[CAN brd intf] Motor response error, unknown command resp");
					}
    			}
    			else
    			{
    				printf("Wrong command word\r\n");
	    			TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog("[CAN brd intf] Wrong command word");
    			}
    		}
	    	sem_trywait(&semMsgSent);
    		sem_post(&semMsgRecv);
    	}
    }
}

int SerialPort::sendAndDontWaitForRecv(unsigned char bytesToSend, unsigned char muxLineVal)
{
	if(initOK == false) return -1;
	pthread_mutex_lock(&globalComLock);
	int csValue = 2;

    if(muxLineVal != 0 && muxLineVal != 1)
    {
		TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog("[CAN brd intf] Wrong CS val1!");
		printf("Wrong cs val1\n");
		return -1;
    }

	TCS_ObjectFactory::Inst().GetTCSStateControl()->SetOutCtrlBit(CAN_SERIAL_COM_CS, muxLineVal);

	while(csValue != muxLineVal)
	{
		csValue = TCS_ObjectFactory::Inst().GetTCSStateControl()->GetFpgaBit(HW_SEQUENCER_OUT_CTRL_REG_ADDRESS, CAN_SERIAL_COM_CS);
	}

	semCtrl->TimeWaitMs(5);
    tcflush(fd, TCIOFLUSH);
	outBuffer[bytesToSend - 1] = generateChecksum((unsigned char*)outBuffer, bytesToSend - 1);
	write(fd, outBuffer, bytesToSend);
	pthread_mutex_unlock(&globalComLock);
	return 0;
}

int SerialPort::sendAndWaitForRecv(unsigned char bytesToSend, unsigned char muxLineVal)
{	
	if(initOK == false)
	{
		return -1;
	}
	pthread_mutex_lock(&globalComLock);
	volatile int csValue = 2;

	if(muxLineVal != 0 && muxLineVal != 1)
	{
		TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog("[CAN brd intf] Wrong CS val1!");
		return -1;
	}

	TCS_ObjectFactory::Inst().GetTCSStateControl()->SetOutCtrlBit(CAN_SERIAL_COM_CS, muxLineVal);

	while(csValue != muxLineVal)
	{
		csValue = TCS_ObjectFactory::Inst().GetTCSStateControl()->GetFpgaBit(HW_SEQUENCER_OUT_CTRL_REG_ADDRESS, CAN_SERIAL_COM_CS);
	}

    tcflush(fd, TCIOFLUSH);
    memset(inBuffer, 0, 255);
	sem_trywait(&readWaitTimeOuted);
	outBuffer[bytesToSend - 1] = generateChecksum((unsigned char*)outBuffer, bytesToSend - 1);
	write(fd, outBuffer, bytesToSend);
	sem_post(&semMsgSent);
	
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	myINT64 tt = my_timespec_to_ns(&ts);
	tt+=((myINT64)SERIAL_PORT_TIMEOUT_SYNTH * 1000000L);
	ts = my_ns_to_timespec(tt);
	if(sem_timedwait(&semMsgRecv, &ts) != 0)
	{
		if(outBuffer[0] < 0x40)
		{
			sprintf(infoString, "[CAN brd intf] Can motors com timeout");
			TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog(infoString);
		}
		else if(outBuffer[3] != 0)
		{
			if(TCS_ObjectFactory::Inst().GetDPContainer()->GetDataEnum_Int("RF Modules Connected") == 1)
			{
				sprintf(infoString, "[CAN brd intf] Can module %d com timeout", outBuffer[3]);
				TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog(infoString);
			}
		}
		sem_post(&readWaitTimeOuted);
		++numTimeouts;
		sem_trywait(&semMsgSent);
		csValue = 13;
		csValue = TCS_ObjectFactory::Inst().GetTCSStateControl()->GetFpgaBit(HW_SEQUENCER_OUT_CTRL_REG_ADDRESS, CAN_SERIAL_COM_CS);
		if(csValue != muxLineVal)
		{
			sprintf(infoString, "[CAN brd intf] CS is: %d | should be: %d", csValue, muxLineVal);
			TCS_ObjectFactory::Inst().GetInfoLog()->WriteToLog(infoString);
		}
		pthread_mutex_unlock(&globalComLock);
		return -1;
	}

	int ret = 0;
	pthread_mutex_unlock(&globalComLock);

	return ret;			
}

int SerialPort::Init(TCSFidelityRFModuleControl* rfModules_)
{
	rfModules = rfModules_;
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

