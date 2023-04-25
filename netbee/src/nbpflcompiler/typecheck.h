/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#include "symbols.h"
#include "tree.h"


bool CheckIntOperand(Node *node);
bool CheckRefBounds(Node *node);
bool CheckVarBounds(SymbolVariable *varSym);
bool CheckFieldBounds(SymbolField *fldSym);
bool CheckSymbolBounds(Symbol *symbol);

