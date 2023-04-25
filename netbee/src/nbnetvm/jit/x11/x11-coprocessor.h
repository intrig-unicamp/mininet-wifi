/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __X11_COPROCESSOR_H__
#define __X11_COPROCESSOR_H__

#include "mirnode.h"
#include "basicblock.h"
#include "registers.h"
#include "x11-ir.h"
#include "x11-emit.h"

namespace jit {
namespace X11 {

class Coprocessor {
  public:
	typedef jit::BasicBlock<X11IRNode> BasicBlock;

  private:
	typedef RegisterInstance RegType;

	static unsigned table;

	static void do_lookup(CopMIRNode *, BasicBlock &bb, X11EmissionContext *);
	static void do_update(CopMIRNode *, BasicBlock &bb, X11EmissionContext *);

  public:  
	static void emit(CopMIRNode *, BasicBlock &bb, X11EmissionContext *);

};

class DataMemory {
  public:
	typedef jit::BasicBlock<X11IRNode> BasicBlock;

  private:
	typedef RegisterInstance RegType;

  public:  
	static void add(uint16_t con, uint16_t address, RegType low, RegType high, BasicBlock &bb,
		X11EmissionContext *ctx);

};


} /* namespace X11 */
} /* namespace jit */

#endif /* __X11_COPROCESSOR_H__ */
