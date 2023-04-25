/** @file
 * \brief This file contains definitions of class NodeTranslator
 */
#include "node_translation.h"
#include "mironode.h"
#include "statements.h"
#include <iostream>
#include <iomanip>

std::list<MIRONode*> *NodeTranslator::translate()
{
	assert(_target_list != NULL && "Target list cannot be void");
	if(_code_list->Empty())
		return _target_list;
	StmtBase *iter;
	iter = _code_list->Front(); //_code_list contains LIR code
	while(iter)
	{
		MIRONode *node = iter->translateToMIRONode(); //traslate the node
		assert(node != NULL);
		//std::cout << "Translator BB ID: " << _bb_id << std::endl;
		//node->printNode(std::cout) << std::endl;
		//std::cout << std::endl;
		_target_list->push_back(node);
	
		MIRONode::IRNodeIterator it = node->nodeBegin();
		for(; it != node->nodeEnd(); it++)
			(*it)->setBBId(_bb_id);
		iter = iter->Next;
	}
//	_code_list->SetHead(NULL);

	return _target_list;
}


void NodeTranslator::translate(std::list<MIRONode*> * code_list)
{
	_target_list = code_list;
	translate();
}
