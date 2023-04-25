/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __X11_OPTIMIZATION__
#define __X11_OPTIMIZATION__

#include "cfg.h"
#include "ssa_graph.h"
#include "basicblock.h"
#include "digraph.h"
#include "registers.h"
#include "netvmjitglobals.h"
#include "opt/nvm_optimizer.h"

#include "x11-ir.h"

#include <map>

namespace jit {
namespace X11 {

class Merge {
  public:
	typedef jit::CFG<X11IRNode> CFG;
	typedef jit::BasicBlock<X11IRNode> BasicBlock;
	typedef CFG::IRType::RegType RegType;

  private:
	CFG &cfg;
	
  public:
	Merge(CFG &cfg);
	~Merge();
	
	void run();
};

class Zero_Versions {
  public:
	typedef jit::CFG<X11IRNode> CFG;
	typedef jit::BasicBlock<X11IRNode> BasicBlock;
	typedef CFG::IRType::RegType RegType;

  private:
	CFG &cfg;
	
  public:
	Zero_Versions(CFG &cfg);
	~Zero_Versions();
	
	void run();
};

class Simple_Redundancy_Elimination {
  public:
	typedef jit::CFG<X11IRNode> CFG;
	typedef jit::BasicBlock<X11IRNode> BasicBlock;
	typedef CFG::IRType::RegType RegType;
	
  private:
	CFG &cfg;
	
	void walk_definitions(SSA_Graph<CFG> &, std::set<RegType> &defs);

  public:
	Simple_Redundancy_Elimination(CFG &cfg);
	~Simple_Redundancy_Elimination();
	
	void run();
}; /* class Simple_Redundancy_Elimination */

/* This simply uses the generic optimization algorithm on X11 code, wrapping it to
 * put the CFG in SSA form and then undoing it
 */
class Dead_Code_Elimination {
  public:
	typedef jit::CFG<X11IRNode> CFG;
	typedef jit::BasicBlock<X11IRNode> BasicBlock;
	typedef CFG::IRType::RegType RegType;

  private:
	jit::opt::DeadcodeElimination<CFG> dead_code;
	CFG &cfg;

  public:
	Dead_Code_Elimination(CFG &cfg);
	~Dead_Code_Elimination();
	
	void run(bool go_to_ssa = true);
};

} /* namespace X11 */
} /* namespace jit */

#endif /* __X11_OPTIMIZATION__ */
