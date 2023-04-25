/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef X64_OPT_H
#define X64_OPT_H

#include "cfg.h"
#include "x64-asm.h"

namespace jit{
namespace x64{
class x64Optimizations
{
  public:
    x64Optimizations(CFG<x64Instruction> &cfg): _cfg(cfg) { };	
    void run();
  
  protected:
    CFG<x64Instruction> &_cfg;
};
}
}
#endif // X64_OPT_H
