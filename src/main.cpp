// main.cpp of tcp2serial application
// author: Zvonimir Lucić
// email: zvone.lucic@gmail.com
// date: 29.4.2016

#include "objectFactory.h"

int main(int argc, char *argv[])
{
	return ObjectFactory::Inst().Init(argc, argv);
}
