/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "symtabletree.h"
#include "symbols.h"

void SymTblTree::RootNode(void)
{
	m_Root = new TreeNode();
	if (m_Root == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	m_CurrNode = m_Root;
}


void SymTblTree::NewChild()
{
	TreeNode *child(0);

	child = new TreeNode();
	if (child == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	child->Parent = m_CurrNode;
	m_CurrNode->Children.push_back(child);

	m_CurrNode = child;

}

void SymTblTree::UpToParent()
{
	nbASSERT(m_CurrNode->Parent == NULL, "The current node is the ROOT node of the tree");
	m_CurrNode = m_CurrNode->Parent;
}

void SymTblTree::StoreFieldSym(const string name, SymbolField *fieldSymbol)
{
	FieldsList_t *fieldsList(0);

	if (!m_CurrNode->SymbolTable.LookUp(name, fieldsList))
	{
		fieldsList = new FieldsList_t();
		if (fieldsList == NULL)
			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
		m_CurrNode->SymbolTable.LookUp(name, fieldsList, ENTRY_ADD);
	}

	fieldsList->push_back(fieldSymbol);
}


FieldsList_t *SymTblTree::LookUpField(const string name)
{
	TreeNode *node = m_CurrNode;
	FieldsList_t *fieldsList;

	while (node)
	{
		if (node->SymbolTable.LookUp(name, fieldsList))
			return fieldsList;
		//iterate through the path to the root
		node = node->Parent;
	}

	//if the symbol is not found into the root symbol table, give up
	return NULL;
}


