#include "useful.h"
#include "serial_port.h"
#include "myTimerCounter.h"
#include "mySemaphore.h"
#include "myMutex.h"
#include "IOSObjectsFactory.h"
#include "object_factory.h"
#include "bitops.h"
#include "myLogger.h"
#include "time_diff.h"

#include <cstdlib>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syscall.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <cctype>
#include <exception>

serial_com::serial_com()
:
num_of_timeouts_(0),
num_of_bytes_received_(0),
num_of_bytes_to_read_(0),
is_timeout_(false),
reply_ok_(false),
transmit_failed_(false),
init_ok_(false)
{
	sem_init(&sem_msg_sent, 0, 0);

	memset(in_buffer, 0, sizeof(in_buffer));
	memset(out_buffer, 0, sizeof(out_buffer));

	sem_receive_finished_ = IOSObjectsFactory::GetInstance().CreateSemaphore(0);
	sem_ctrl = IOSObjectsFactory::GetInstance().CreateSemaphore(0);
	lock = IOSObjectsFactory::GetInstance().CreateMutex();
}

serial_com::~serial_com()
{
	close(fd);

	sem_destroy(&sem_msg_sent);

	delete lock;
	delete sem_ctrl;
	delete sem_receive_finished_;
}

void serial_com::Start(master_slave_mode_t mode)
{
	mode_ = mode;
	unsigned long tid;
	pthread_create(&tid, NULL, start_fun, (void*) this);
}

void* serial_com::start_fun(void* pvParam)
{
	serial_com* service = (serial_com*) pvParam;
	Logger::Inst().Log("serial_com PID  = %ld\n", syscall(SYS_gettid));
	service->execute();
	return NULL;
}

void serial_com::reset_receive_control()
{
	num_of_bytes_received_ = 0;
}

void serial_com::execute()
{
	bool transmission_in_progress = false;
	while (1)
	{
		if(mode_ == master)
		{
			sem_wait(&sem_msg_sent);
			transmission_in_progress = true;
			while(transmission_in_progress)
			{
				int32_t num_of_bytes_read = read(fd, in_buffer, UART_BUFF_SIZE);
				/*if(num_of_bytes_read)
				{
					printf("num_of_bytes_read: %d\n", num_of_bytes_read);
					for(int i = 0; i < num_of_bytes_read; i++)
					{
						printf("0x%02X ", in_buffer[i]);
					}
					printf("\n");
				}*/
				clock_gettime(CLOCK_REALTIME, &end_time);
				uint32_t time_elapsed_in_ms = timespec_diff(&start_time, &end_time);
				if(time_elapsed_in_ms > timeout_in_ms_)
				{
					is_timeout_ = true;
					transmission_in_progress = false;
					lock->UnLock();
					sem_receive_finished_->Signal();
				}
				num_of_bytes_received_ += num_of_bytes_read;
				if(num_of_bytes_received_ >= 3 && num_of_bytes_to_read_ == 3)
				{
					num_of_bytes_to_read_ = 3 + in_buffer[2] + 2;
				}
				if(num_of_bytes_received_ == num_of_bytes_to_read_)
				{
					reply_ok_ = true;
					transmission_in_progress = false;
					lock->UnLock();
					sem_receive_finished_->Signal();
				}
				else
				{
					sem_ctrl->TimeWaitMs(5);
				}
			}
		}
		else if (mode_ == slave)
		{
			int32_t num_of_bytes_read = read(fd, in_buffer + num_of_bytes_received_, UART_BUFF_SIZE - num_of_bytes_received_);
			if(num_of_bytes_read > 0)
			{
				num_of_bytes_received_ += num_of_bytes_read;
				sem_receive_finished_->Signal();
			}
			else
			{
				Logger::Inst().Log("Error reading slave request: %d\n", num_of_bytes_read);
			}
		}
		else
		{

		}
	}
}

int32_t serial_com::Send_with_timeout(uint8_t *data_to_send, uint32_t bytes_to_send, uint32_t timeout_in_ms)
{
	if(mode_ == slave)
	{
		return -102;
	}
	int32_t ret = -2;

	if(init_ok_ == false) 
	{
		return ret;
	}

	/*printf("Sending: ");
	for(int i = 0; i < bytes_to_send; i++)
	{
		printf("0x%02X ", data_to_send[i]);
	}
	printf("\n");*/

	lock->Lock();
    tcflush(fd, TCIOFLUSH);
	num_of_bytes_received_ = 0;
	num_of_bytes_to_read_ = 3;
	timeout_in_ms_ = timeout_in_ms;
	reply_ok_ = false;
	is_timeout_ = false;
	ret = write(fd, data_to_send, bytes_to_send);
	if(ret != (int32_t)bytes_to_send)
	{
		transmit_failed_ = true;
	}
	else
	{
		transmit_failed_ = false;
		clock_gettime(CLOCK_REALTIME, &start_time);
		sem_post(&sem_msg_sent);
	}

	return ret;
}

int32_t serial_com::Send(uint8_t *data_to_send, uint32_t bytes_to_send)
{
	if(mode_ == master)
	{
		return -101;
	}
	int32_t ret = -2;

	if(init_ok_ == false) 
	{
		return ret;
	}

	/*printf("slave Sending: ");
	for(int i = 0; i < bytes_to_send; i++)
	{
		printf("0x%02X ", data_to_send[i]);
	}
	printf("\n");*/

	lock->Lock();
    tcflush(fd, TCIOFLUSH);
	num_of_bytes_received_ = 0;
	ret = write(fd, data_to_send, bytes_to_send);
	if(ret != (int32_t)bytes_to_send)
	{
		transmit_failed_ = true;
	}
	else
	{
		transmit_failed_ = false;
	}
	lock->UnLock();

	return ret;
}

int serial_com::Init(const char* baud_rate, const char* serial_device)
{
    // Open modem device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(serial_device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0)
    {
    	//Logger::Inst().Log("Error opening serial device\n");
    	//perror(serial_device);
    	//return -1;
		std::string msg = "Error opening serial device " + (std::string)serial_device;
		throw std::runtime_error(msg);
    }
	
    tcgetattr(fd,&old_tio); 				// save current serial port settings
    memset(&new_tio, 0, sizeof(new_tio)); 	// clear struct for new port settings

    if(strcmp(baud_rate, "2400") == 0)
    {
    	new_tio.c_cflag = B2400 | CS8 | CLOCAL | CREAD;
    }
    else if(strcmp(baud_rate, "4800") == 0)
	{
		new_tio.c_cflag = B4800 | CS8 | CLOCAL | CREAD;
	}
    else if(strcmp(baud_rate, "9600") == 0)
	{
		new_tio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	}
    else if(strcmp(baud_rate, "19200") == 0)
	{
		new_tio.c_cflag = B19200 | CS8 | CLOCAL | CREAD;
	}
    else if(strcmp(baud_rate, "38400") == 0)
	{
		new_tio.c_cflag = B38400 | CS8 | CLOCAL | CREAD;
	}
    else if(strcmp(baud_rate, "57600") == 0)
	{
		new_tio.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
	}
    else if(strcmp(baud_rate, "115200") == 0)
	{
		new_tio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	}
    else
    {
    	Logger::Inst().Log("Setting default baud_rate (9600)\n");
		new_tio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
    }

    // IGNPAR  : ignore bytes with parity errors
    // ICRNL   : map CR to NL (otherwise a CR input on the other computer will not terminate input)
    new_tio.c_iflag = IGNPAR;

    // Raw output.
    new_tio.c_oflag = 0;

    //   ICANON  : enable canonical input
    //   disable all echo functionality, and don't send signals to calling program
	// Canonical Mode: In canonical mode, read on the serial port will not return until 
	// a new line, EOF or EOL character is received  by the Linux Kernel. In this more, 
	// read will always return an entire line, no matter how many bytes are requested.

	// Non-Canonical Mode: In non-canonical mode, read on the serial port will not wait 
	// until the new line. Number of bytes read depends on the MIN and TIME settings.
	// MIN: Minimum number of bytes that must be available in the input queue in order for read to return
	// TIME: How long to wait for input before returning, in units of 0.1 seconds.
    new_tio.c_lflag = 0;//ICANON;

    //   initialize all control characters
    //   default values can be found in /usr/include/termios.h, and are given
    //   in the comments, but we don't need them here

    new_tio.c_cc[VINTR]    = 0;     
    new_tio.c_cc[VQUIT]    = 0;     
    new_tio.c_cc[VERASE]   = 0;     
    new_tio.c_cc[VKILL]    = 0;     
    new_tio.c_cc[VEOF]     = 0;     
    new_tio.c_cc[VTIME]    = 0;
	if(mode_ == master)
	{
    	new_tio.c_cc[VMIN]     = 0;
	}
	else if(mode_ == slave)
	{
    	new_tio.c_cc[VMIN]     = 1;
	}
    new_tio.c_cc[VSWTC]    = 0;     
    new_tio.c_cc[VSTART]   = 0;    
    new_tio.c_cc[VSTOP]    = 0;     
    new_tio.c_cc[VSUSP]    = 0;     
    new_tio.c_cc[VEOL]     = 0;     
    new_tio.c_cc[VREPRINT] = 0;     
    new_tio.c_cc[VDISCARD] = 0;     
    new_tio.c_cc[VWERASE]  = 0;     
    new_tio.c_cc[VLNEXT]   = 0;     
    new_tio.c_cc[VEOL2]    = 0;     

    //now clean the modem line and activate the settings for the port
    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd,TCSANOW,&new_tio);

    //terminal settings done
    fcntl(fd, F_SETFL, 0);

	Logger::Inst().Log("SERIAL device %s opened with baud_rate %s\n", serial_device, baud_rate);

    init_ok_= true;

    return 0;
}
