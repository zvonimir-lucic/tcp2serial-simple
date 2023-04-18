/*
 * myLogger.h
 *
 *  Created on: Feb 10, 2022
 *      Author: zvone
 */

#ifndef SRC_MYLOGGER_H_
#define SRC_MYLOGGER_H_

#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdarg>
#include <ctime>

class Logger
{
public:
	~Logger(void);
	static Logger& Inst();
	int32_t Init(std::string app_name);
	void Log(const char *format, ...);

private:
	Logger(void);

	std::ofstream log_file;
	static Logger inst;
	std::string file_name;
	std::string tmp_file_name;
	std::string command;

	time_t t;
	struct tm tm;
	volatile bool locked;
};

#endif /* SRC_MYLOGGER_H_ */
