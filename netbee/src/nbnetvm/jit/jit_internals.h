/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef _JIT_INTERNALS
#define _JIT_INTERNALS

#include "nbnetvm.h"
#include "mirnode.h"
#include "genericbackend.h"
#include "cfg.h"
#include "function_interface.h"
#include <iostream>


namespace jit
{

	//!options of the jit
	struct _TargetOptions
	{
		uint8_t OptLevel; 	//!<optimization level
		const char *OutputFilePrefix;			//!<prefix for files emitted
		uint32_t Flags;  		//!<Jit Flags
		std::ostream &assembly_stream;   //!the stream where the target code is emitted

		_TargetOptions(uint8_t optLevel, const char *outputFilePrefix, uint32_t fl, std::ostream &targetCode)
			:OptLevel(optLevel), OutputFilePrefix(outputFilePrefix), Flags(fl), assembly_stream(targetCode){}
	};

	typedef struct _TargetOptions TargetOptions;

	//!class that every target should implement
	class TargetDriver{
		public:

		typedef std::list<nvmJitFunctionI *> func_list_t;

		//!constructor
		TargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options);
		//!destructor
		virtual ~TargetDriver();

		//!function which initialize the algorithms for this target
		virtual void init(CFG<MIRNode>& cfg) = 0;

		/*!
		 * \brief compile the application
		 */
		virtual void compile();

		/*!
		 * \brief check if the option passed are supported
		 *
		 * Backend can override this function to force some options even if
		 * they were not passed by the user
		 */
		virtual int32_t check_options(uint32_t index);

		protected:

		/*!
		 * \brief pre-compile the application
		 */
		virtual DiGraph<nvmNetPE*>* precompile(DiGraph<nvmNetPE*>* pe_graph);

		//!compile all the netvm instance making inline
		virtual void compileInline(DiGraph<nvmNetPE*>* pe_graph);

		//!compile every PE separately
		virtual void compilePEs(DiGraph<nvmNetPE*>* pe_graph);

		//!compile a cfg calling the algorithms set up by init
		virtual void compileCFG(CFG<MIRNode>& cfg);

		//!return a backend class for this driver
		virtual GenericBackend* get_genericBackend(CFG<MIRNode>& cfg) = 0;

		//!when doing jit compiling connect the emitted function to the PE handler
		void connectFunctionToHandler(uint8_t* functPushBuffer, nvmHandlerState* HandlerState);


		func_list_t algorithms; 		//!<list of the algorithm to apply to a cfg
		nvmNetVM* netvm; 				//!<netvm object to compile
		TargetOptions *options;			//!<options for this compilation
		nvmRuntimeEnvironment* RTObj;	//!the runtime environment of this application

				private:
		//!empty the algorithm list when done with a cfg
		void clear_algorithm_list();
	};

	//!inteface function type to be called from by nvmNetCompileApplication
	typedef TargetDriver* (TargetInterfaceFunc)(nvmNetVM*, nvmRuntimeEnvironment*, TargetOptions*);
}

#endif
