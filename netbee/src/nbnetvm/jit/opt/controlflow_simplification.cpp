/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*
 * \file controlflow_simplification.cpp This file contains method to peform controlflow simplification
 */
#include "controlflow_simplification.h"
#include "nvm_optimizer.h"
#include <algorithm>

namespace jit{
namespace opt{


		/*!
		 * \brief This method analyze a node with two kids that are constant
		 *
		 * JCMPEQ, JCMPEQ, JCMPG, JCMPGE, JCMPL, JCMLLE plus signed version
		 * are analyzed in this function and a new unconditonal JUMP node is created 
		 * pointing to the evaluated target
		 */

		
} /* OPT */
} /* JIT */
