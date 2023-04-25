/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#ifdef __cplusplus
extern "C" {
#endif


// NetVM Instruction set
// Table of conversion from textual instructions to hexadecimal opcodes

// In the draft version some instructions are precedeed by the flag <m>. This flag is unnecessary as the set of primitive types is reduced to only one element.
// Some intructions keep the type specification, as the LOAD instructions, 

//Da considerare
// load con byte swap. E' + efficiente della IESWAP perche' fa tutto in un'operazione

enum{
	#define nvmOPCODE(id, name, pars, code, consts, desc) id=code,
	#include "opcodes.txt"
	#include "newopcodes.txt"
	#undef nvmOPCODE
};



#ifdef __cplusplus
}
#endif

