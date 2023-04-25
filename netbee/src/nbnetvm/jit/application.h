/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "rt_environment.h"
#include "bytecode_segments.h"
#include "basicblock.h"

namespace jit {

//!class that holds information from the runtime environment useful for the jit
class Application {
  private:
	nvmNetVM *netvm; //!< pointer to the current instance of netvm
	nvmRuntimeEnvironment *runtime; //!< pointer to current runtime
	nvmByteCodeSegment *segment; //!<pointer to the current bytecode segment
	nvmPEHandler *pe_handler; //!<pointer to the current pe_handler
	//nvmCounter	*profile_counters;		//!< Profiling counters
	
	static Application app; //!<static member which really holds the information (singleton)
	
	//!constructor
	Application();

  public:	
	//!get a reference to the application instance
	template<typename IR>
	static Application &getApp(BasicBlock<IR> &);
	static Application &getApp(uint16_t BBId);
  
	// Hooks to update the Application status
	static void setCurrentPEHandler(nvmPEHandler *pe);
	static nvmPEHandler* getCurrentPEHandler() {return app.pe_handler;};
	static SegmentTypeEnum getCurrentSegmentType() {return app.getSegmentType();};
	static void setCurrentSegment(nvmByteCodeSegment *segment);
	static void setCurrentRuntime(nvmRuntimeEnvironment *runtime);
	static void setCurrentNetVM(nvmNetVM *netvm);
	//void setCurrentProfCounters(nvmCounter *profcounters);
	//! different type of memory
	typedef enum {
		info, data, shared, inited
	} mem_type_t;

	// Data retrieval - to be expanded!
	//!get the information of a coprocessor
	nvmCoprocessorState *getCoprocessor(uint32_t which_one);
	uint32_t getCoprocessorBaseRegister(uint32_t which_one) const;
	static uint32_t getCoprocessorRegSpace();
	
	//!get information about a memory
	nvmMemDescriptor getMemDescriptor(mem_type_t); // Please do not make it return a pointer, we might need to
												   // generate an object on the fly.
	//!get the current segment
	nvmByteCodeSegment *getCurrentSegment();
//	nvmPEHandler *getCurrentPEHandler() const;
	SegmentTypeEnum getSegmentType() const;
	//nvmCounter *getCurrentProfCounters() const;
}; /* class Application */

inline SegmentTypeEnum Application::getSegmentType() const
{
	return segment->SegmentType;
}

template<typename IR>
Application& Application::getApp(BasicBlock<IR> &bb)
{
	setCurrentPEHandler(bb.template getProperty<nvmPEHandler *>("handler"));
	return app;
}


} /* namespace jit */

#endif /* __APPLICATION_H__ */
