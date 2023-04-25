/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include "genericbackend.h"

uint8_t* jit::GenericBackend::emitNativeFunction()
{
	std::cerr << "Native code emission not implemented by this backend" << std::endl;
	return NULL;
}

void jit::GenericBackend::emitNativeAssembly(std::string prefix)
{
	std::cerr << "Native assembly emission not implemented by this backend" << std::endl;
	return;
}


void jit::GenericBackend::emitNativeAssembly(std::ostream &str)
{
	std::cerr << "Native assembly emission not implemented by this backend" << std::endl;
	return;
}

jit::GenericBackend::~GenericBackend()
{}
