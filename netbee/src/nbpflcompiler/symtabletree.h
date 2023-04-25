/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#include "errors.h"
#include "strsymboltable.h"
#include <list>

using namespace std;

struct SymbolField;
typedef list<SymbolField*> FieldsList_t;	//different fields can share the same name in a protocol description

//StrSymbolTable specializations
typedef StrSymbolTable<FieldsList_t*> FieldSymbolTable_t;

class SymTblTree
{
private:
	struct TreeNode
	{
		TreeNode			*Parent;
		list<TreeNode*>		Children;
		FieldSymbolTable_t	SymbolTable;

		TreeNode(TreeNode *parent = 0)
			:Parent(parent){}
	};
	
	TreeNode	*m_Root;
	TreeNode	*m_CurrNode;

public:
	SymTblTree()
		:m_Root(0), m_CurrNode(0)
	{
		RootNode();
	}

	~SymTblTree()
	{
		delete m_Root;
	}

	void RootNode(void);
	void NewChild();
	void UpToParent();
	void StoreFieldSym(const string name, SymbolField *constSymbol);
	FieldsList_t *LookUpField(const string name);

};
