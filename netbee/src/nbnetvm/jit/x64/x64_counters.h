/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef _ACCESS_COUNTER_H_
#define _ACCESS_COUNTER_H_

#include "inssel-x64.h"


void counter_access_mem_profiling(jit::BasicBlock<jit::x64::x64Instruction>& BB , jit::x64::x64_dim_man::dim_mem_type type_mem);
void counter_check_profiling(jit::BasicBlock<jit::x64::x64Instruction>& BB , jit::x64::x64_dim_man::dim_mem_type type_mem);


#endif /* _ACCESS_COUNTER_H_ */

