#include "objectFactory.h"
#include "commandLineOptions.h"
#include <iostream>

ObjectFactory ObjectFactory::inst;

ObjectFactory::ObjectFactory()
{
	commandLineParser_ = new CommandLineParser();
}

int ObjectFactory::Init(int argc, char *argv[])
{
	commandLineParser_->setVersion("0.1");
	commandLineParser_->setDescription("TCP to serial port simple application");
	commandLineParser_->addHelpOption();

	//if error in parsing command line arguments print message and end program
	CommandLineParser::errorType ret = commandLineParser_->parse(argc, argv);
	if(ret == CommandLineParser::HELP_WANTED)
	{
		commandLineParser_->printHelp();
		return 0;
	}
	else if(ret == CommandLineParser::NO_REQUIRED_OPTIONS)
	{
		commandLineParser_->printUsage();
		return 1;
	}
	else if(ret == CommandLineParser::UNKNOWN_OPTION)
	{
		commandLineParser_->printUnknownOption();
		return 2;
	}
	else if(ret == CommandLineParser::NO_PARAM_VALUE)
	{
		commandLineParser_->printNoParamValue();
		return 2;
	}
	return 0;
}

ObjectFactory& ObjectFactory::Inst()
{
	return inst;
}
