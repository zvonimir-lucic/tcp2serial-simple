#ifndef OBJECTFACTORY_H_
#define OBJECTFACTORY_H_

class CommandLineParser;

class ObjectFactory
{
public:
	~ObjectFactory() {}
	int Init(int argc, char *argv[]);
	static ObjectFactory& Inst();
	
protected:
	ObjectFactory() {}
	static ObjectFactory inst;

	CommandLineParser* commandLineParser_;
};

#endif
