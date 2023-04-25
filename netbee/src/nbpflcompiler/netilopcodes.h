/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


enum NetILOpcodes
{
#define nvmOPCODE(id, name, pars, code, desc) id=code,
#include "opcodes.txt"
#undef nvmOPCODE
};

