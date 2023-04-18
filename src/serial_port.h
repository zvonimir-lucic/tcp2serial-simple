// serial_port.h of tcp2serial-simple application
// author: Zvonimir Lucić
// email: zvone.lucic@gmail.com
// date: 18.4.2023.

#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#include <semaphore.h>
#include <pthread.h>
#include <termios.h>
#include <cstdint>
#include <ctime>
#include "useful.h"
#include <list>
#include <map>

class mySemaphore;
class myMutex;

#define UART_BUFF_SIZE 256

class serial_com
{
public:
	serial_com();
	~serial_com();

	int32_t Init(const char* baud_rate, const char* serial_device);
	void Start(master_slave_mode_t mode);
	int32_t Send(uint8_t* data_to_send, uint32_t num_of_bytes);

	bool init_ok() {return init_ok_;}
	bool transmit_failed() {return transmit_failed_;}
	mySemaphore* sem_receive_finished() {return sem_receive_finished_;}
	uint8_t* reply(){return in_buffer;}
	void reset_receive_control();

private:
	static void* start_fun (void*);
	void execute();

    int32_t fd;
    struct termios old_tio;
    struct termios new_tio;

	master_slave_mode_t mode_;

	uint8_t out_buffer[UART_BUFF_SIZE]; 
	uint8_t in_buffer[UART_BUFF_SIZE]; 

	sem_t sem_msg_sent;
	
	bool transmit_failed_;

	bool init_ok_;

    struct timespec start_time, end_time;

	mySemaphore* sem_ctrl;
	mySemaphore* sem_receive_finished_;
	myMutex* lock;
};

#endif /*SERIAL_PORT_H_*/
