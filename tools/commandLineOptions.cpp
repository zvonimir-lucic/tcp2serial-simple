/*
 * commandLineOptions.cpp
 *
 *  Created on: 11 Apr 2016
 *      Author: zvone
 */
#include "commandLineOptions.h"
#include <iostream>
#include <string.h>


CommandLineOption::CommandLineOption(const char parametarName[], const char commandShort[], const char commandLong[], const char description[], bool isRequired)
:parametarName_(parametarName),
 description_(description),
 value_("N/A"),
 isRequired_(isRequired),
 isDataSet_(false),
 hasParamValue_(true)
{
	commandShort_ = "-" + std::string(commandShort);
	commandLong_ = "--" + std::string(commandLong);
}

CommandLineOption::CommandLineOption(const char commandShort[], const char commandLong[], const char description[], bool isRequired)
:description_(description),
 value_("false"),
 isRequired_(isRequired),
 isDataSet_(false),
 hasParamValue_(false)
{
	commandShort_ = "-" + std::string(commandShort);
	commandLong_ = "--" + std::string(commandLong);
}

CommandLineParser::CommandLineParser()
{
	version_ = "N/A";
	description_ = "N/A";
	appName_ = "N/A";
	hasHelpOption_ = false;
}

CommandLineParser::errorType CommandLineParser::parse(int argc, char *argv[])
{
	appName_ = std::string(argv[0]);

	if(hasHelpOption_)
	{
		for(int i = 1; i < argc; i++)
		{
			if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
			{
				return HELP_WANTED;
			}
		}
	}

	for(int i = 1; i < argc; i++)
	{
		bool found = false;
		for(std::vector<CommandLineOption*>::iterator optsIter = options_.begin(); optsIter != options_.end(); optsIter++)
		{
			if(std::string(argv[i]) == (*optsIter)->commandShort() ||
			   std::string(argv[i]) == (*optsIter)->commandLong()  )
			{
				found = true;
				if((*optsIter)->hasParamValue())
				{
					if(i < argc - 1)
					{
						i++;
						(*optsIter)->setValue(std::string(argv[i]));
					}
					else
					{
						return NO_PARAM_VALUE;
					}
				}
				else
				{
					(*optsIter)->setValue("true");
				}
				break;
			}
		}
		if(!found)
		{
			return UNKNOWN_OPTION;
		}
	}

	for(std::vector<CommandLineOption*>::iterator optsIter = options_.begin(); optsIter != options_.end(); optsIter++)
	{
		bool found = false;
		int i = 1;
		for(; i < argc; i++)
		{
			if(std::string(argv[i]) == (*optsIter)->commandShort() ||
			   std::string(argv[i]) == (*optsIter)->commandLong()  )
		   {
			   found = true;
			   break;
		   }
		}
		if((*optsIter)->isRequired() && (!found || i == argc - 1))
		{
			return NO_REQUIRED_OPTIONS;
		}
	}
	return OK;
}

void CommandLineParser::printHelp()
{
	std::cout << "\n" << description_ << std::endl;
	std::cout << "Version: " << version_ << std::endl;

	std::cout << "Usage:\n   " << appName_;
	for(std::vector<CommandLineOption*>::iterator optsIter = options_.begin(); optsIter != options_.end(); optsIter++)
	{
		if((*optsIter)->isRequired())
		{
			std::cout << " " << (*optsIter)->commandShort() << " <" << (*optsIter)->parametarName() << ">";
		}
	}
	std::cout << " [options]" << std::endl;

	std::cout << "\nRequired parameters:" << std::endl;
	for(std::vector<CommandLineOption*>::iterator optsIter = options_.begin(); optsIter != options_.end(); optsIter++)
	{
		if((*optsIter)->isRequired() && !(*optsIter)->hasParamValue())
		{
			std::cout << "   " << (*optsIter)->commandShort();
			std::cout << ", " << (*optsIter)->commandLong();
			std::cout << "\t" << (*optsIter)->description() << std::endl;
		}
	}
	for(std::vector<CommandLineOption*>::iterator optsIter = options_.begin(); optsIter != options_.end(); optsIter++)
	{
		if((*optsIter)->isRequired() && (*optsIter)->hasParamValue())
		{
			std::cout << "   " << (*optsIter)->commandShort() << " <" << (*optsIter)->parametarName() << ">";
			std::cout << ", " << (*optsIter)->commandLong() << " <" << (*optsIter)->parametarName() << ">";
			std::cout << "\t" << (*optsIter)->description() << std::endl;
		}
	}
	std::cout << std::endl;

	std::cout << "Options:" << std::endl;
	if(hasHelpOption_)
	{
		std::cout << "   -h, --help\tPrints help" << std::endl;
	}
	for(std::vector<CommandLineOption*>::iterator optsIter = options_.begin(); optsIter != options_.end(); optsIter++)
	{
		if(!(*optsIter)->isRequired() && !(*optsIter)->hasParamValue())
		{
			std::cout << "   " << (*optsIter)->commandShort();
			std::cout << ", " << (*optsIter)->commandLong();
			std::cout << "\t" << (*optsIter)->description() << std::endl;
		}
	}
	for(std::vector<CommandLineOption*>::iterator optsIter = options_.begin(); optsIter != options_.end(); optsIter++)
	{
		if(!(*optsIter)->isRequired() && (*optsIter)->hasParamValue())
		{
			std::cout << "   " << (*optsIter)->commandShort() << " <" << (*optsIter)->parametarName() << ">";
			std::cout << ", " << (*optsIter)->commandLong() << " <" << (*optsIter)->parametarName() << ">";
			std::cout << "\t" << (*optsIter)->description() << std::endl;
		}
	}
	std::cout << std::endl;
}

void CommandLineParser::printUsage()
{
	std::cout << "\n" << description_ << std::endl;
	std::cout << "Version: " << version_ << std::endl;
	std::cout << "Error calling application: not enough parameters" << std::endl;

	std::cout << "Usage:\n   " << appName_;
	for(std::vector<CommandLineOption*>::iterator optsIter = options_.begin(); optsIter != options_.end(); optsIter++)
	{
		if((*optsIter)->isRequired())
		{
			std::cout << " " << (*optsIter)->commandShort() << " <" << (*optsIter)->parametarName() << ">";
		}
	}
	std::cout << " [options]" << std::endl;

	if(hasHelpOption_)
	{
		std::cout << "\nFor help use option -h or --help" << std::endl;
	}
	std::cout << std::endl;
}

void CommandLineParser::printUnknownOption()
{
	std::cout << "\n" << description_ << std::endl;
	std::cout << "Version: " << version_ << std::endl;
	std::cout << "Error calling application: unknown parameter(s)" << std::endl;
	if(hasHelpOption_)
	{
		std::cout << "\nFor help use option -h or --help" << std::endl;
	}
	std::cout << std::endl;
}

void CommandLineParser::printNoParamValue()
{
	std::cout << "\n" << description_ << std::endl;
	std::cout << "Version: " << version_ << std::endl;
	std::cout << "Error calling application: missing parameter(s) value(s)" << std::endl;
	if(hasHelpOption_)
	{
		std::cout << "\nFor help use option -h or --help" << std::endl;
	}
	std::cout << std::endl;
}
