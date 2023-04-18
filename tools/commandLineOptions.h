#ifndef COMMANDLINEOPTIONS_H
#define COMMANDLINEOPTIONS_H

#include <string>
#include <stdexcept>
#include <vector>

class CommandLineOption
{
public:
	CommandLineOption(const char parametarName[], const char commandShort[], const char commandLong[], const char description[], bool isRequired = false);
	CommandLineOption(const char commandShort[], const char commandLong[], const char description[], bool isRequired = false);
	virtual ~CommandLineOption() {}

	void setValue(std::string valToSet) { value_ = valToSet;}
	std::string value()
	{
		if(!isDataSet_)
		{
			throw std::domain_error("Value not set");
		}
		return value_;
	}

	std::string commandShort() { return commandShort_;}
	std::string commandLong() { return commandLong_;}
	std::string parametarName() { return parametarName_;}
	std::string description() { return description_;}
	bool isRequired(){ return isRequired_;}
	bool hasParamValue() { return hasParamValue_;}
	
private:
	std::string parametarName_;
	std::string commandShort_;
	std::string commandLong_;	
	std::string description_;
	std::string value_;
	bool isRequired_;
	bool isDataSet_;
	bool hasParamValue_;
};

class CommandLineParser
{
public:
	CommandLineParser();
	~CommandLineParser() {}

	enum errorType
	{
		OK = 0,
		HELP_WANTED,
		NO_REQUIRED_OPTIONS,
		UNKNOWN_OPTION,
		NO_PARAM_VALUE,
		UNKNOW_ERROR
	};

	errorType parse(int argc, char *argv[]);

	void addOption(CommandLineOption* option)
	{
		options_.push_back(option);
	}

	void setVersion(std::string valToSet)
	{
		version_ = valToSet;
	}

	void setDescription(std::string valToSet)
	{
		description_ = valToSet;
	}

	void addHelpOption()
	{
		hasHelpOption_ = true;
	}

	void printHelp();
	void printUsage();
	void printUnknownOption();
	void printNoParamValue();

private:
	std::vector<CommandLineOption*> options_;
	std::string version_;
	std::string description_;
	std::string appName_;
	bool hasHelpOption_;
};

#endif
