/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "mem_translator.h"
#include <algorithm>
#include <iostream>

/*!
 * \file mem_translator.cpp
 * \brief this file contains the definition of the class which linearize the memory accesses
 */

using namespace jit;

MemTranslator::MemTranslator(CFG<MIRNode>& _cfg)
	:
		nvmJitFunctionI("Mem translator"),
		cfg(_cfg),
		mem_register(RegisterModel::get_new(0))
{
}

bool MemTranslator::run()
{
	cfg.PreOrder(*cfg.getEntryNode(), *this);
	return true;
}
/*!
 * \param cfg The CFG to translate
 * 
 * Since the MemTranslator class is a IGraphVisitor this static
 * member function initializes and starts the visit.
 */
void MemTranslator::translate(CFG<jit::MIRNode>& cfg)
{
	//CFG<MIRNode>::manager_t& manager = cfg.manager;
	//uint32_t regcount = manager.getCount();
	//manager.addNewRegister(regcount);

	MemTranslator translator(cfg);

	cfg.PreOrder(*cfg.getEntryNode(), translator);
}

/*!
 * Does nothing
 */
int32_t MemTranslator::ExecuteOnEdge(const GraphNode &visitedNode, const GraphEdge &edge, bool lastEdge)
{
	return nvmJitSUCCESS;
}

/*!
 * For every instruction in the basic block we transform it in case it is a node operation.
 */
int32_t MemTranslator::ExecuteOnNodeFrom(const GraphNode &visitedNode, const GraphNode *comingFrom)
{
	BasicBlock<MIRNode> *BB = visitedNode.NodeInfo;

	transform(BB->codeBegin(), BB->codeEnd(), BB->codeBegin(), *this);

	return nvmJitSUCCESS;
}

/*!
 * \param code The original memory operation code.
 * \return the new code assocciated to the lineatized instruction
 *
 * In the netil there a lot of memory operation since in the opcode
 * are hardcode both the type of memory accessed and the size of the operand. <br>
 * In the intermediate representation we need only to keep the size
 * information in the opcode.
 */
uint8_t getMemCode(uint8_t code)
{
	uint8_t opcode = code;

	switch ( nvmOpCodeTable[opcode].Consts & OPCODE_MASK )
	{
		case OP_LOAD:
			switch ( nvmOpCodeTable[opcode].Consts & DIM_MASK )
			{
				case DIM_8:  opcode = LLOAD8;  break;
				case DIM_16: opcode = LLOAD16; break;
				case DIM_32: opcode = LLOAD32; break;
			}
			break;
		case OP_STORE:
			switch ( nvmOpCodeTable[opcode].Consts & DIM_MASK )
			{
				case DIM_8:  opcode = LSTORE8;  break;
				case DIM_16: opcode = LSTORE16; break;
				case DIM_32: opcode = LSTORE32; break;
			}
			break;
	}
	return opcode;

}

/*!
 * \param memtype a flag indicating the memory accessed
 * \return the code of the load base operation needed
 */
uint8_t getBaseCode(uint32_t memtype)
{
	uint8_t res = 0;

	switch (memtype)
	{
		case MEM_PACKET:
			res = LDPKTBASE;
			break;
		case MEM_INFO:
			res = LDINFOBASE;
			break;
		case MEM_DATA:
			res = LDDATABASE;
			break;
		case MEM_SHARED:
			res = LDSHAREDBASE;
			break;
	}

	return res;
}

/*!
 * \param node the node to transform
 *
 * If the node is a memory operation we transform it in a load/store 
 * instruction and the address is calculated added the offset to the
 * memory base associated to the original instruction.
 */
MIRNode *MemTranslator::operator()(MIRNode *node)
{
	if(node == NULL)
		return node;

	for(int i = 0; i < 2; i++)
	{
		node->setKid( (*this)(node->getKid(i)), i);
	}

	if(!node->isMemoryOperation())
		return node;

	//kid with the offset to add to the base address
	int offset_kid(0);
#if 0
	// Apparently, this was somehow fixed and now the offset is always the argument 0
	int offset_kid = (nvmOpCodeTable[node->getOpcode()].Consts & OP_STORE) != 0;
	std::cout << "Il nodo " << nvmOpCodeTable[node->getOpcode()].CodeName << " ha come figlio offset "
		<< offset_kid << '\n';
#endif
	MIRNode *offset = node->getKid(offset_kid);

	//add node
	RegisterInstance r(mem_register);
	uint8_t base_code = getBaseCode(nvmOpCodeTable[node->getOpcode()].Consts & MEM_MASK);
	MIRNode *base = MIRNode::leafNode(base_code, node->belongsTo(), r);
	MIRNode *add = MIRNode::binOp(ADD, node->belongsTo(), base, offset, r);

	//load or store node
	uint8_t mem_code = getMemCode(node->getOpcode());
	node->setKid(add, offset_kid);
	node->setOpcode(mem_code);

	return node;
}
