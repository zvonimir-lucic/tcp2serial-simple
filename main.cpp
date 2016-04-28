// main.cpp of terminal-app-template application
// author: Zvonimir Lucić
// email: zvone.lucic@gmail.com
// date: 9.4.2016

#include "objectFactory.h"

int main(int argc, char *argv[])
{
	return ObjectFactory::Inst().Init(argc, argv);
}
