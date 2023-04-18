/*
 * myLogger.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: zvone
 */

#include "myLogger.h"

Logger Logger::inst;

Logger::Logger(void)
{
}

Logger::~Logger(void)
{
}

Logger& Logger::Inst()
{
	return inst;
}

int32_t Logger::Init(std::string app_name)
{
	std::size_t pos = app_name.find_last_of("/");
	app_name = app_name.substr(pos+1);

	file_name = "/tmp/" + app_name + ".log";
	tmp_file_name = file_name + ".write";
	command = "cp " + tmp_file_name + " " + file_name;

	this->Log("%s log start\n", app_name.c_str());

	this->locked = 0;

	return 0;
}

void Logger::Log(const char *format, ...)
{
	char log_msg[2048];
	va_list ap;
	va_start(ap, format);
	vsnprintf(log_msg, sizeof(log_msg), format, ap);

	char date_and_time[50];
	t = time(NULL);
	tm = *localtime(&t);
	snprintf(date_and_time, sizeof(date_and_time), "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	while(this->locked);
	this->locked = 1;
	log_file.open(tmp_file_name, std::ofstream::out | std::ofstream::app);
	log_file << "[" << date_and_time << "] " << log_msg;
	log_file.close();
	this->locked = 0;

	system(command.c_str());
}
