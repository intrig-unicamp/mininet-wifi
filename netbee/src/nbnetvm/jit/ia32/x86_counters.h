/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef _ACCESS_COUNTER_H_
#define _ACCESS_COUNTER_H_

#include "inssel-ia32.h"


void counter_access_mem_profiling(jit::BasicBlock<jit::ia32::x86Instruction>& BB , jit::ia32::x86_dim_man::dim_mem_type type_mem);

void counter_check_profiling(jit::BasicBlock<jit::ia32::x86Instruction>& BB , jit::ia32::x86_dim_man::dim_mem_type type_mem);



#endif /* _ACCESS_COUNTER_H_ */

