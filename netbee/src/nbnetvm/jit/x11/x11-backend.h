/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __X11_BACKEND_H__
#define __X11_BACKEND_H__

#include "mirnode.h"
#include "cfg_builder.h"
#include "basicblock.h"
#include "cfg.h"
#include "genericbackend.h"
#include "jit_internals.h"
#include "x11-ir.h"
#include <ostream>

jit::TargetDriver* x11_getTargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, jit::TargetOptions* options);

namespace jit {
namespace X11 {

class X11TargetDriver : public TargetDriver
{
	public:
		X11TargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options);
		void init(CFG<MIRNode>& cfg);
		int32_t check_options(uint32_t index);

	protected:
		GenericBackend* get_genericBackend(CFG<MIRNode>& cfg);
};

//! this class is the interface exported by the X11 backend
class X11Backend : public GenericBackend {
  private:
	CFG<MIRNode>&	MLcfg; //!<source CFG
	CFG<X11IRNode> LLcfg; //!<the new CFG with x86 instruction

  public:
	X11Backend(CFG<MIRNode>& cfg);
	virtual ~X11Backend();

	virtual void emitNativeAssembly(std::ostream &str);


	virtual void emitNativeAssembly(std::string file_name);
};

/* The X11 can execute coprocessor accesess only on a basic block boundary, so whe split
 * the BBs which have coprocessors accesses in them so that they are the last instruction
 * of the basicblock they belong to.
 */
class X11SplitCoprocessors : private CFGBuilder<X11IRNode> {
  public:
	X11SplitCoprocessors(CFG<X11IRNode> &cfg);
	virtual ~X11SplitCoprocessors();

	virtual void buildCFG() {run(); return;};

	void run();
};

} /* namespace X11 */
} /* namespace jit */

#endif /* __X11_BACKEND_H__ */
