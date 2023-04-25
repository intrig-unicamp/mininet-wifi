/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include "application.h"
#include "int_structs.h"
#include "../coprocessor.h"
#include <cassert>

namespace jit {

Application Application::app;

Application::Application() : netvm(0), runtime(0), segment(0), pe_handler(0)
{
	// Nop
	return;
}

void Application::setCurrentPEHandler(nvmPEHandler *p)
{
	assert(p != 0);
	app.pe_handler = p;
	return;
}

void Application::setCurrentSegment(nvmByteCodeSegment *s)
{
	assert(s != 0);
	app.segment = s;
	return;
}

void Application::setCurrentRuntime(nvmRuntimeEnvironment *r)
{
	assert(r != 0);
	app.runtime = r;
	return;
}

void Application::setCurrentNetVM(nvmNetVM *n)
{
	assert(n != 0);
	app.netvm = n;
	return;
}

//void Application::setCurrentProfCounters(nvmCounter	*profcounters)
//{
//	assert(profcounters != 0);
//	profile_counters = profcounters;
//}


nvmCoprocessorState *Application::getCoprocessor(uint32_t id)
{
	assert(id < pe_handler->HandlerState->PEState->NCoprocRefs);
	nvmCoprocessorState *res = &pe_handler->HandlerState->PEState->CoprocTable[id];
	return res;
}

uint32_t Application::getCoprocessorBaseRegister(uint32_t id) const
{
	assert(id < pe_handler->HandlerState->PEState->NCoprocRefs);
	return id*MAX_COPRO_REGISTERS;
}

uint32_t Application::getCoprocessorRegSpace() 
{
	return 32;
}

Application &Application::getApp(uint16_t BBId)
{
	return app;
}

nvmMemDescriptor Application::getMemDescriptor(mem_type_t mt)
{
	nvmMemDescriptor r;
	r.Base = 0;
	r.Size = 0;
	r.Flags = 0;

	switch(mt) {
		case data:
			r = *pe_handler->HandlerState->PEState->DataMem;
			break;

		case shared:
			r = *pe_handler->HandlerState->PEState->ShdMem;
			break;

		case inited:
			r = *pe_handler->HandlerState->PEState->InitedMem;
			break;

		case info:
			// Stub implementation that does exactly what I need - Pierluigi
			//r.Size = 16*2; // 16 variables, 2 bytes each.
			r.Size = runtime->ExbufPool->InfoLen;
			break;

		default:
			;
	}
	
	return r;
}

nvmByteCodeSegment *Application::getCurrentSegment()
{
	return segment;
}


#if 0
nvmPEHandler* Application::getCurrentPEHandler() const
{
	return pe_handler;
}
#endif

//nvmCounter *Application::getCurrentProfCounters() const
//{
//	return profile_counters;
//}


} /* namespace jit */
