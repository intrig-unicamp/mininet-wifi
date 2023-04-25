#include "reassociation_fixer.h"
#include "registers.h"
#include "basicblock.h"
#include "mironode.h"
#include "tree.h"


ReassociationFixer::ReassociationFixer(PFLCFG &cfg): _cfg(cfg) {};

void ReassociationFixer::VisitBasicBlock(PFLBasicBlock *bb, PFLBasicBlock *from)
{
	typedef MIRONode IRType;
	typedef IRType::IRNodeIterator it_t;
	typedef list<IRType*>::iterator l_it_t;
	typedef PFLBasicBlock BBType;
	typedef MIRONode::RegType RegType;
	typedef map<RegType, IRType*> map_t;
	typedef set<RegType>::iterator s_it;

	set<RegType> locUses;

	BBType::IRStmtNodeIterator i;
	for(i = bb->codeBegin(); i != bb->codeEnd(); i++)
	{
		//cout << "Reassociation FIxer" << endl;
		//cout << " PRima passata Analizzo il nodo" << endl;
		//(*i)->printNode(std::cout, true);
		//cout << endl;

		IRType *root = (*i)->getTree();
		if(root)
		{
			for(it_t j = root->nodeBegin(); j != root->nodeEnd(); j++)
			{
				if((*j)->isLoad())
				{
					if( (*j)->getDefReg().get_model()->get_space() == 2)
					{
						//cout << "Inserisco l'uso di un nodo intermedio: " << (*j)->getDefReg() << endl;
						locUses.insert((*j)->getDefReg());
					}
					continue;
				}
			}
		}
	}
	list<IRType*> &codelist(bb->getCode());  
	for(l_it_t k = codelist.begin(); k != codelist.end(); k++)
	{
		//cout << "Reassociation FIxer" << endl;
		//cout << "Seconda passata Analizzo il nodo" << endl;
		//(*k)->printNode(std::cout, true);
		//cout << endl;

		IRType *root = (*k)->getTree();
		if(root)
		{
			for(it_t j = root->nodeBegin(); j != root->nodeEnd(); j++)
			{
				for(int n=0; n < 2; n++)
				{
					IRType *kid;
					if((kid = (*j)->getKid(n)))
					{
						s_it s;

						if(!kid->isStore() && !kid->isLoad() && ( (s = locUses.find(kid->getDefReg())) != locUses.end() ))
						{
							//cout << "il nodo definisce n registro usato!!!" << endl;
							RegType new_reg = RegType::get_new(2);
							RegType def_reg = kid->getDefReg();

							IRType *str = new MIRONode(LOCST, kid->copy(), new SymbolTemp(kid->getDefReg()));
							GenMIRONode *stmt = new GenMIRONode(str);
							codelist.insert(k, stmt);
							str->getKid(0)->setDefReg(new_reg);

							//cout << "Ho inserito un nuovo nodo nella lista: " << endl;
							//stmt->printNode(cout, true);
							//cout << endl;

							IRType *lclod = new MIRONode(LOCLD, new SymbolTemp(def_reg));
							locUses.erase(s);
							(*j)->setKid(lclod, n);
						}
					}
				}
			}
		}
	}

	return;
}

void ReassociationFixer::VisitBBEdge(PFLBasicBlock *from, PFLBasicBlock *to)
{
	// NOP
	return;
}

bool ReassociationFixer::run()
{
	CFGVisitor visitor(*this);
	visitor.VisitPreOrder(_cfg);
	return true;
}
