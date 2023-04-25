/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include "cfg.h"
#include "cfg_liveness.h"
#include "irnode.h"
#include "iltranslator.h"
#include "mirnode.h"
#include "basicblock.h"
#include "jit_internals.h"
#include "registers.h"
#include "cfgdom.h"
#include "../opcodes.h"
#include <stack>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>
#include <typeinfo>
#include <iostream>
#include <sstream>


typedef struct {

	bool isValid ;

	int32_t value;

	//int32_t max_value_dominators;

} Value;

template <typename _CFG>
class Boundscheck_remover: public nvmJitFunctionI {
  public:
	typedef _CFG CFG;

  private:

	CFG &cfg;
	jit::TargetOptions* options;
	jit::Liveness<CFG> liveness;
	std::map<uint16_t, std::map<string,   Value > > pkt_map, info_map, data_map;
	std::map<string, std::map<string, Value > >  sndpkt_tree;
	uint32_t pkt_ncheck_before,  info_ncheck_before, data_ncheck_before;
	uint32_t data_ncheck_removed, info_ncheck_removed, pkt_ncheck_removed;
	uint32_t data_ncheck_not_optimizable, info_ncheck_not_optimizable, pkt_ncheck_not_optimizable;
	uint32_t pkt_ncheck , info_ncheck, data_ncheck ;

	std::map<uint16_t, std::map<string, std::map<string, Value > > > idom_value;
	std::map<uint16_t, std::map<string, std::map<string, Value > > > idom_tree_value;

	std::set<uint16_t> sendpkt_tree_bb;
	std::map<string, std::map<string, Value > > branch_value;

	bool check_not_optimizable;
	bool isLoop;
	
  public:
	/*!
	 * \param cfg The cfg to rename
	 */
	  Boundscheck_remover(CFG &cfg, jit::TargetOptions* opt);

	// return true if the node or one of his kids is a load from memory
	bool isAccMem(typename _CFG::IRType *n);
	
	// return true if the node or one of his kids is a regload
	bool isLocLoad(typename _CFG::IRType *n );

	// Return the string that describes RegType
	string printRegType(typename _CFG::IRType *n);

	// Optimization at level of BasiBlock if kids of the check node are constant node
	void constKids(typename _CFG::IRType *n, typename CFG::BBType* bb, std::map<string,  Value > *regload);

	// Optimization at level of BasiBlock if one of kids of the check node is a regload node
	void locKids(typename _CFG::IRType *n, typename CFG::BBType* bb, std::map<string, Value > *regload);

	void check(typename _CFG::IRType *n, typename CFG::BBType* bb, std::map<string, Value > *regload);


	// Return true if the check node can be removed
	bool checkModifier(typename _CFG::IRType *n, typename CFG::BBType* bb, std::map<uint16_t, std::map<string,  Value > > * map);

	// Optimization of the check analysing the IDOM tree
	int32_t checkValueDom(typename _CFG::IRType *n, typename CFG::BBType* bb, std::map<uint16_t, std::map<string, Value > > * map,  string code);
	
	// Remove check node if the linked load/store was removed
	void removeMIRnode(typename CFG::BBType* bb);

	void check_branch(typename CFG::BBType* bb);

	void SetInvalidCheckMIRnodeTree();

	// Print Summary of the checks
	void ScanMap();

	std::map<string, std::map<string, Value > > check_SNDPKT_tree(typename CFG::BBType* bb , typename CFG::BBType* idom, std::list<uint16_t> visitedBB  );

	std::map<string, std::map<string, Value > > apply_tree_modification(typename CFG::BBType* bb , typename CFG::BBType* idom, std::list<uint16_t> visitedBB  );

	void union_tree( std::map<string, std::map<string, Value > >  & m ,std::map<string, std::map<string, Value > >  t, bool addflag);

	void update_tree( string typemem, typename CFG::BBType* bb, std::map<string, std::map<string, Value > >& temp);

	void union_predecessors_tree( std::map<string, std::map<string, Value > >  & m ,std::map<string, std::map<string, Value > >  t, string typemem);

	// Return true if the check node can be removed
	bool checkModifierTree(typename _CFG::IRType *n, typename CFG::BBType* bb, string typemem, std::map<string, Value > bb_map, std::map<string, std::map<string, Value > >* map);

	void tree_modification(typename CFG::BBType* bb, std::map<string, std::map<string, Value > >*  temp_tree);

	void removeMIRnodeTree();

	void setflag( string typemem , std::map<string, std::map<string, Value > >& temp );

	void update_branch_value( std::map<string, Value > map , string typemem);

	void apply_branch_value( typename CFG::BBType* bb);

	// Return true if the check node can be removed
	bool branchModifier(typename _CFG::IRType *n, typename CFG::BBType* bb, string typemem,  std::map<string, Value > * map);

	/*!
	 * Run the BOUNDSCHECK_REMOVER algorithm.
	 */
	bool run();

}; /* class SSA */


template<typename _CFG>
Boundscheck_remover<_CFG>::Boundscheck_remover(_CFG &_cfg, jit::TargetOptions * opt) : nvmJitFunctionI("Boundscheck_remover"), cfg(_cfg), liveness(cfg)
{
	options = opt;	
}

template<typename _CFG>
void Boundscheck_remover<_CFG>::removeMIRnode(typename CFG::BBType* bb)
{
	bool clr = false;
	typename _CFG::IRType *del;

	typename _CFG::BBType::IRStmtNodeIterator j, l;

	for(j = bb->codeBegin(); j != bb->codeEnd(); )
	{
		typename _CFG::IRType *ir(*j);
		typename _CFG::IRType::IRNodeIterator k(ir->nodeBegin());
			
		for(; k != ir->nodeEnd(); k++)
		{
			typename _CFG::IRType *n(*k);
			del=n;
			if(n->getIsNotValid() == true )
			{
				#ifdef _DEBUG_BCHECK
				std::cout << "Found invalid node " << (*n) << std::endl;
				#endif

				clr = true;
				break;

			} // if getIsNotValid()

		} // end for node
		
		l = j;
		++j;

		if( clr )
		{
			#ifdef _DEBUG_BCHECK
				std::cout << "BB: "<< bb->getId()  << " remove instruction: " << (*del)<< std::endl;
			#endif
			bb->eraseCode(l);
			delete del;
			clr = false;
		}

	} // end code


}


template<typename _CFG>
bool Boundscheck_remover<_CFG>::run()
{
	//uint32_t temp, temp2;
	/*
	* Ivano: this return disables the bcheckremover algorithm
	*/
	return true;
	
	pkt_ncheck_before = 0;
	info_ncheck_before = 0;
	data_ncheck_before = 0;
	data_ncheck_removed = 0;
	info_ncheck_removed = 0;
	pkt_ncheck_removed = 0;
	pkt_ncheck_not_optimizable = 0;
	info_ncheck_not_optimizable = 0;
	data_ncheck_not_optimizable = 0;
	pkt_ncheck = 0;
	info_ncheck = 0;
	data_ncheck = 0;
	isLoop = false;

	std::map<string, Value > pkt_regload;
	std::map<string, Value > info_regload;
	std::map<string, Value > data_regload;
		
	std::list<typename CFG::BBType*> sendpkts;

	#ifdef _DEBUG_BCHECK
		std::cout << std::endl << "Boundcheck Analysis " << cfg.getName() << std::endl;
	#endif

	cfg.SortPreorder(*cfg.getEntryNode());
	
	typename DiGraph< typename _CFG::BBType* >::SortedIterator i(cfg.FirstNodeSorted());

	//for each node of the cfg
	for(  ; i != cfg.LastNodeSorted(); ++i)
	{
		typename _CFG::BBType *bb( (*i)->NodeInfo );
	
	//std::list<typename _CFG::BBType*> *list(cfg.getBBList());

	//typename std::list<typename _CFG::BBType*>::iterator i;
	//
	//for(i = list->begin(); i != list->end(); ++i)
	//{
	//	typename _CFG::BBType *bb(*i);

		removeMIRnode(bb);
		
		//std::list<typename CFG::IRType *> &code_list(bb->getCode());

		//typename std::list<typename CFG::IRType *>::iterator j;
		typename _CFG::BBType::IRStmtNodeIterator j, l;
		
		for(j = bb->codeBegin(); j != bb->codeEnd(); ++j) 
		{
			typename _CFG::IRType *ir(*j);
			typename _CFG::IRType::IRNodeIterator k(ir->nodeBegin());
			
			for(; k != ir->nodeEnd(); k++)
			{
				typename _CFG::IRType *n(*k);

				if( (n)->getOpcode() == PCHECK  )
				{
					#ifdef _DEBUG_BCHECK
						std::cout << "PKT CHECK: ";
						n->printNode(std::cout, true);
						std::cout << std::endl;
					#endif
					
					pkt_ncheck_before++;
					//std::cout << "trovato PKT CHECK: " << pkt_ncheck_before << std::endl;
					check(n, bb, &pkt_regload);

				} // end if PCHECK

				if( (n)->getOpcode() == ICHECK  )
				{
					#ifdef _DEBUG_BCHECK
						std::cout << "INFO CHECK: ";
					#endif

					info_ncheck_before++;
					check(n, bb, &info_regload);

				} // end if ICHECK

				if( (n)->getOpcode() == DCHECK  )
				{
					#ifdef _DEBUG_BCHECK
					 std::cout << "DATA CHECK: ";
					#endif
					data_ncheck_before++;
					check(n, bb, &data_regload);

				} // end if ICHECK

				if( (n)->getOpcode() == SNDPKT  )
				{
					#ifdef _DEBUG_BCHECK
					std::cout << "SNDPKT found" << std::endl;
					#endif
					
					sendpkts.insert(sendpkts.begin(), bb);
					
				} // end if SNDPKT


			} //node 

		} // code_list  

		if(!pkt_regload.empty() )
		{
			
			pkt_map.insert(make_pair(bb->getId(), pkt_regload));
	
		}
		if(!info_regload.empty())
		{
			info_map.insert(make_pair(bb->getId(), info_regload));
		}
		if(!data_regload.empty())
		{
			data_map.insert(make_pair(bb->getId(), data_regload));
		}

		pkt_regload.clear();
		info_regload.clear();
		data_regload.clear();


// cfg modification		
		
		bool clr = false;

		check_not_optimizable = true;

		typename _CFG::IRType *istr = NULL;
		
		for(j = bb->codeBegin(); j != bb->codeEnd(); )
		{
			typename _CFG::IRType *ir(*j);
			typename _CFG::IRType::IRNodeIterator k(ir->nodeBegin());
			
			for(; k != ir->nodeEnd(); k++)
			{
				typename _CFG::IRType *n(*k);
				istr = n;

				check_not_optimizable = true;

				if( (n)->getOpcode() == PCHECK  )
				{
					clr = checkModifier(n, bb,  &pkt_map);
					if(clr)
					{
						pkt_ncheck_removed++;
					}

					if(check_not_optimizable)
					{
						pkt_ncheck_not_optimizable++;
						
					}

					break;
					
				} // end if PCHECK

				if( (n)->getOpcode() == ICHECK  )
				{
					clr = checkModifier(n, bb,  &info_map);
					if(clr)
					{
						info_ncheck_removed++;
					}

					if(check_not_optimizable)
					{
						info_ncheck_not_optimizable++;
					}
					
					break;

				} // end if ICHECK

				if( (n)->getOpcode() == DCHECK  )
				{

					clr = checkModifier(n, bb,  &data_map);
					if(clr)
					{
						data_ncheck_removed++;
					}

					if(check_not_optimizable)
					{
						data_ncheck_not_optimizable++;
					}
					break;
					
	
				} // end if ICHECK


			} //node 

			l = j;
			++j;

			if( clr )
			{
				#ifdef _DEBUG_BCHECK
				std::cout << "BB: "<< bb->getId()  << " remove instruction: " << (*istr)<< std::endl;
				#endif
				bb->eraseCode(l);
				delete istr;
				clr = false;
			}

		} // code_list  

// end cfg modification

	}  // list 

		if( options->OptLevel > 2 )
		{

			pkt_ncheck = pkt_ncheck_before - pkt_ncheck_removed - pkt_ncheck_not_optimizable;
			info_ncheck = info_ncheck_before - info_ncheck_removed - info_ncheck_not_optimizable;
			data_ncheck = data_ncheck_before - data_ncheck_removed - data_ncheck_not_optimizable;


			if(  (pkt_ncheck < 1 ) && (info_ncheck < 1 ) && (data_ncheck < 1 ) )
			{
					#ifdef _DEBUG_BCHECK
						std::cout << std::endl <<"No packet check and info and data check only on constant"  <<  std::endl <<  std::endl;
					#endif
			} else {

				typename std::list<typename CFG::BBType*>::iterator s_iter(sendpkts.begin());

				std::list<uint16_t> visitedBB;
				std::map<string, std::map<string, Value > > tree;

				for( ; s_iter != sendpkts.end(); s_iter++ )
				{
					#ifdef _DEBUG_BCHECK
						std::cout <<  "SNDPKT at BB: " << (*s_iter)->getId() << std::endl; 
					#endif

					tree = check_SNDPKT_tree(*s_iter, NULL, visitedBB);

					// set flag isValid to true in tree map
					setflag("pkt",  tree);
					setflag("info", tree);
					setflag("data", tree);


					// tree union
					union_tree( sndpkt_tree, tree, false);

				}

				#ifdef _DEBUG_BCHECK
				std::map<string, Value >::iterator it2( sndpkt_tree["pkt"].begin());
				for( ; it2 != sndpkt_tree["pkt"].end() ; it2++)
				{
					std::cout <<  "\tsndpkt_tree variable " << it2->first << " has value " << (it2->second).value << std::endl;
				}
				#endif

			//std::cout << "SET = " << sendpkt_tree_bb.size()<< std::endl;

			//for( std::set<uint16_t>::iterator set_it = sendpkt_tree_bb.begin() ; set_it != sendpkt_tree_bb.end() ; set_it++)
			//{
			//	std::cout << "\tBB  = " << *set_it<< std::endl;
			//}

			
				SetInvalidCheckMIRnodeTree();
		
				for( s_iter = sendpkts.begin() ; s_iter != sendpkts.end(); s_iter++ )
				{
					apply_tree_modification((*s_iter), NULL, visitedBB);
				}

				removeMIRnodeTree();


			}

		}



	//		cfg.SortPreorder(*cfg.getEntryNode());

	////		cfg.SortRevPostorder(*((*(sendpkts.begin()))->getNode()));

	//
	//typename DiGraph< typename _CFG::BBType* >::SortedIterator i2(cfg.FirstNodeSorted());
	//
	//for(  ; i2 != cfg.LastNodeSorted(); ++i2)
	//{
	//	typename _CFG::BBType *bbs((*i2)->NodeInfo);

	//	std::cout << "BB: " << bbs->getId() << std::endl;

	//}


	#ifdef _DEBUG_BCHECK
		std::cout << std::endl <<"Summary"  <<  std::endl <<  std::endl;
		ScanMap();
		std::cout<< std::endl;
	#endif

	return true;
}  // fine Boundscheck_remover<_CFG>::run()



/*
* n: instruction under analysis
* bb: basic block having the instruction
* regload: memory
*/
template<typename _CFG>
void Boundscheck_remover<_CFG>::check(typename _CFG::IRType *n, typename CFG::BBType* bb, std::map<string, Value > *regload)
{
	jit::MIRNode * kid1((n)->getKid(0));		
	jit::MIRNode * kid2((n)->getKid(1));	

	#ifdef _DEBUG_BCHECK
  std::cout << std::endl;
	kid1->printNode(std::cout, true);
  std::cout << std::endl;
	kid2->printNode(std::cout, true);
  std::cout << std::endl;
  #endif
	
	if( !isLocLoad(kid2))
	{

		if( !isAccMem(n) )
		{
			if(kid1->isConst() && kid2->isConst())
			{
				std::cout << "due constanti\n";
				constKids(n, bb, regload);
			} 
			
			if(isLocLoad(kid1) || isLocLoad(kid2))
			{
				std::cout << "una constante\n";
				locKids(n, bb, regload);
			}

		} else { 
			#ifdef _DEBUG_BCHECK
				std::cout << "Found boundcheck with offset from memory" <<  std::endl;
			#endif
		}
	} else { 
		#ifdef _DEBUG_BCHECK
			std::cout << "Found boundcheck with dimension in a local variable" <<  std::endl;
		#endif

	}	
} // end check




template<typename _CFG>
bool Boundscheck_remover<_CFG>::checkModifier(typename _CFG::IRType *n, typename CFG::BBType* bb,  std::map<uint16_t, std::map<string, Value > > * map)
{
	jit::MIRNode * kid1((n)->getKid(0));		
	jit::MIRNode * kid2((n)->getKid(1));

//	jit::MIRNode * temp;

	jit::ConstNode *leftkidconst, *rightkidconst;

	std::stringstream ss;
	
	if( !isLocLoad(kid2))
	{

		if( !isAccMem(n) )
		{
			if(kid1->isConst() && kid2->isConst())
			{
				check_not_optimizable = false;

				if( (n)->getOpcode() == ICHECK )
				{
					if( kid1->getValue() + kid2->getValue() > (int32_t)jit::Application::getApp(bb->getId()).getMemDescriptor(jit::Application::info).Size )
					{ 
						ss << "Error offset " << kid1->getValue() + kid2->getValue();
						ss << "greater than dim INFO mem " <<  (int32_t)jit::Application::getApp(bb->getId()).getMemDescriptor(jit::Application::info).Size << std::endl;
						throw std::string(ss.str());
					} else {

						return true; 
					}

				} // end if ICHECK

				if(  (n)->getOpcode() == DCHECK  )
				{
					if( kid1->getValue() + kid2->getValue() > (int32_t)jit::Application::getApp(bb->getId()).getMemDescriptor(jit::Application::data).Size )
					{
						ss << "Error offset " << kid1->getValue() + kid2->getValue();
						ss << "greater than dim DATA mem " <<  (int32_t)jit::Application::getApp(bb->getId()).getMemDescriptor(jit::Application::data).Size << std::endl;
						throw std::string(ss.str());

					} else {

						return true; 
					}

				} // end if DCHECK

				string code = "constant";

				std::map<string, Value > tmp = (*map)[bb->getId()];
				//std::cout << "BB: "<< bb->getId() << " prima " << tmp.size()<< std::endl;

				std::map<string, Value >::iterator iter =  tmp.find(code);
						
				if( iter != tmp.end() && tmp[code].isValid)
				{
					int32_t value_idoms = checkValueDom(n, bb, map, code);

					if( value_idoms < tmp[code].value)
					{
						leftkidconst = (jit::ConstNode*) kid1;
						rightkidconst = (jit::ConstNode*) kid2;
						leftkidconst->setValue(tmp[code].value);
						rightkidconst->setValue(0);

						// Setto flag a false così che non venga utilizzato più in questo BB
						((*map)[bb->getId()])[code].isValid = false;

						return false;
					} else 
					{
						#ifdef _DEBUG_BCHECK
						std::cout << "value of constant taken from IDOM" << std::endl;
						#endif
						
						// Setto flag a false così che non venga utilizzato più in questo BB
						((*map)[bb->getId()])[code].isValid = false;

						//((*map)[bb->getId()])["constant"].value = -1;
						//((*map)[bb->getId()])["constant"].max_value_dominators = value_idoms;

						((*map)[bb->getId()]).erase(code);

						return true;
					}


					
				} else {

					return true;
			
				}
			
			} 
			
			if(isLocLoad(kid1))
			{

				if( (kid1)->getOpcode() == LDREG)
				{
					check_not_optimizable = false;
					string code = printRegType(kid1);

					std::map<string, Value > tmp =  (*map)[bb->getId()];
					//std::cout << "BB: "<< bb->getId() << " prima " << tmp.size()<< std::endl;

					std::map<string, Value >::iterator iter =  tmp.find(code);

					if( iter != tmp.end() && tmp[code].isValid )
					{
						int32_t value_idoms = checkValueDom(n, bb, map, code);

						if( value_idoms < tmp[code].value)
						{
						
						rightkidconst = (jit::ConstNode*) kid2;
						rightkidconst->setValue(tmp[code].value);
					
						// (*map)[bb->getId()].erase(code);

						// Setto flag a false così che non venga utilizzato più in questo BB
						((*map)[bb->getId()])[code].isValid = false;

						
						return false;
					
						} else 
						{
							#ifdef _DEBUG_BCHECK
								std::cout << "Value of " << code << " taken from IDOM" << std::endl;
							#endif

							((*map)[bb->getId()])[code].isValid = false;
							
							//((*map)[bb->getId()])[code].value = -1;	
							//((*map)[bb->getId()])[code].max_value_dominators = value_idoms;	

							((*map)[bb->getId()]).erase(code);
							return true;
						}
					
					} else {  // end if( iter != tmp.end() && tmp[code].isValid )

						return true;
					}



				} else {   // end kid1==LDREG

					if((kid1)->getOpcode() == ADD || (kid1)->getOpcode() == ADDSOV || (kid1)->getOpcode() == ADDSOV)
					{

						if( ((kid1)->getKid(0))->getOpcode() == LDREG  &&  ((kid1)->getKid(1))->getOpcode() != LDREG )
						{
							check_not_optimizable = false;
							string code = printRegType(kid1->getKid(0));
							
							std::map<string, Value > tmp =  (*map)[bb->getId()];
							//std::cout << "ADD-LOCL BB: "<< bb->getId() << " prima " << tmp.size()<< std::endl;

							std::map<string, Value >::iterator iter =  tmp.find(code);

							if( iter != tmp.end()  && tmp[code].isValid )
							{
								int32_t value_idoms = checkValueDom(n, bb, map, code);

								if( value_idoms < tmp[code].value)
								{
									rightkidconst = (jit::ConstNode*) kid2;
									rightkidconst->setValue(tmp[code].value);

									//temp = kid1;
		
									//std::cout << "kid1: " << (*kid1) << std::endl;
									//std::cout << "temp: " << (*temp) << std::endl;
									//std::cout << "kid1->getKid(0): " << (*(kid1->getKid(0))) << std::endl;
								
									n->setKid(kid1->getKid(0), 0);

									//std::cout << "Dopo setkid " << (*(n->getKid(0))) << std::endl;
								
									// Setto flag a false così che non venga utilizzato più in questo BB
									((*map)[bb->getId()])[code].isValid = false;

									//std::cout << "ADD-LOCL BB: "<< bb->getId() << " dopo " <<  (*map)[bb->getId()].size()<< std::endl;

									return false;

								} else 
								{
									#ifdef _DEBUG_BCHECK
										std::cout << "Value of " << code << " taken from IDOM" << std::endl;
									#endif
								
									((*map)[bb->getId()])[code].isValid = false;
							
									//((*map)[bb->getId()])[code].value = -1;	
									//((*map)[bb->getId()])[code].max_value_dominators = value_idoms;	

									((*map)[bb->getId()]).erase(code);
									return true;
								}

					
							} else { // end if( iter != tmp.end()  && tmp[code].isValid )

								return true;
							}


						} else if( ((kid1)->getKid(1))->getOpcode() == LDREG  && ((kid1)->getKid(0))->getOpcode() != LDREG)
						{
							check_not_optimizable = false;
							string code = printRegType(kid1->getKid(1));
							
							std::map<string, Value > tmp = (*map)[bb->getId()];
							//std::cout << "ADD-LOCL BB: "<< bb->getId() << " prima " << tmp.size()<< std::endl;

							std::map<string, Value >::iterator iter =  tmp.find(code);

							if( iter != tmp.end()  && tmp[code].isValid )
							{
								int32_t value_idoms = checkValueDom(n, bb, map, code);

								if( value_idoms < tmp[code].value)
								{
									rightkidconst = (jit::ConstNode*) kid2;
									rightkidconst->setValue(tmp[code].value);

									//temp = kid1;

									//std::cout << "kid1: " << (*kid1) << std::endl;
									//std::cout << "temp: " << (*temp) << std::endl;
									//std::cout << "kid1->getKid(0): " << (*(kid1->getKid(0))) << std::endl;
								
									n->setKid(kid1->getKid(1), 0);

									//std::cout << "Dopo setkid " << (*(n->getKid(0))) << std::endl;
								
									// Setto flag a false così che non venga utilizzato più in questo BB
									((*map)[bb->getId()])[code].isValid = false;

									//std::cout << "ADD-LOCL BB: "<< bb->getId() << " dopo " <<  (*map)[bb->getId()].size()<< std::endl;

									return false;
								} else 
								{
									#ifdef _DEBUG_BCHECK
										std::cout << "Value of " << code << " taken from IDOM" << std::endl;
									#endif
							
									((*map)[bb->getId()])[code].isValid = false;
							
									//((*map)[bb->getId()])[code].value = -1;	
									//((*map)[bb->getId()])[code].max_value_dominators = value_idoms;	
									((*map)[bb->getId()]).erase(code);
									return true;
								}

					
							} else { // end if( iter != tmp.end()  && tmp[code].isValid )

								return true;
							}

						}



					} //END KID1->getOpcode() == ADD
				} // end else
				
			} // end isLocLoad(kid1)

		} else { 
			#ifdef _DEBUG_BCHECK
			std::cout << "BB: " << bb->getId()  << " - found boundcheck with offset from memoria" <<  std::endl;
			#endif

			
		}
	} else { 
		#ifdef _DEBUG_BCHECK
			std::cout << "BB: " << bb->getId()  << " - Found boundcheck with dimension in a local variable" <<  std::endl;
		#endif


	}	

	return false;
} // end check

template<typename _CFG>
int32_t Boundscheck_remover<_CFG>::checkValueDom(typename _CFG::IRType *n,  typename CFG::BBType* bb, std::map<uint16_t, std::map<string, Value > > * map, string code)
{
	typename CFG::BBType* idom;

	idom = bb->template getProperty< typename CFG::BBType* >("IDOM");
	if(idom == NULL)
	{
		return 0;
	}

	#ifdef _DEBUG_BCHECK
	std::cout << "IDOM of BB " << bb->getId() << " is the BB "  << idom->getId() << std::endl;
	#endif

	if( map->find(idom->getId()) != map->end())
	{

		std::map<string, Value > tmp =  (*map)[idom->getId()];

		std::map<string, Value >::iterator iter =  tmp.find(code);

		if( iter != tmp.end())
		{
			//return tmp[code].max_value_dominators;
			return tmp[code].value;

		}
	}

	
 return checkValueDom(n, idom , map, code);
}

template<typename _CFG>
std::map<string, std::map<string, Value > >  Boundscheck_remover<_CFG>::check_SNDPKT_tree(typename CFG::BBType* bb, typename CFG::BBType* idom, std::list<uint16_t> visitedBB  )
{
typedef typename DiGraph<typename CFG::BBType*>::GraphNode GraphNode;
typedef std::list<GraphNode*> p_list_t;
typedef typename p_list_t::iterator l_it;

bool isNull = false;

std::map<uint16_t, std::map<string, Value > >::iterator iter;

std::map<string, std::map<string, Value > > temp;

// Insert  id of the BB that belong to sendpkt tree in the set 
sendpkt_tree_bb.insert(bb->getId());


// Check for loop
std::list<uint16_t>::iterator it(visitedBB.begin() );

for( ; it != visitedBB.end() ; it++)
{
	if( (*it) == bb->getId() )
	{
		#ifdef _DEBUG_BCHECK
			std::cout << "BB: " << bb->getId() << " already visited" << std::endl;
		#endif

		isLoop = true;
		return temp;

	}

}

visitedBB.insert( visitedBB.begin() , bb->getId() );


if( idom_value.find(bb->getId()) != idom_value.end() )
{
	#ifdef _DEBUG_BCHECK
		std::cout << "Value of BB: " << bb->getId() << " found in idom_tree_value " << visitedBB.size() << std::endl;
	#endif

	return idom_value[bb->getId()];
}



if( bb->getPredecessors().size() > 0)
{
	//std::cout << *bb << " ";

	if( bb->getPredecessors().size() > 1)
	{
		std::map<string, std::map<string, Value > > temp2, temp_idom;

		p_list_t preds( bb->getPredecessors() );

		typename CFG::BBType* idom;

		idom = bb->template getProperty< typename CFG::BBType* >("IDOM");

		#ifdef _DEBUG_BCHECK
			std::cout << " has " << bb->getPredecessors().size() << " predecessors: "  << std::endl;
		#endif

		if( idom_value.find(idom->getId()) == idom_value.end() )
		{
			temp_idom = check_SNDPKT_tree( idom, NULL, visitedBB);
			idom_value.insert(make_pair( idom->getId() , temp_idom ));
		
			#ifdef _DEBUG_BCHECK
				std::cout << "Inserted in idom_value temp_idom for BB: " <<  idom->getId() << std::endl;
			#endif

		} else {
			#ifdef _DEBUG_BCHECK
				std::cout << "Used temp_idom in idom_value for BB: " <<  idom->getId() << std::endl;
			#endif

			temp_idom = idom_value[ idom->getId() ];
		}

		//std::map<string, Value >::iterator it2( temp_idom["pkt"].begin());
		//for( ; it2 != temp_idom["pkt"].end() ; it2++)
		//{
		//	std::cout <<  "\t " << bb->getId() << " temp_idom variable " << it2->first << " has value " << (it2->second).value << std::endl;
		//}

		#ifdef _DEBUG_BCHECK
		std::cout << "temp_Idom size: " << temp_idom["pkt"].size() << std::endl;
		#endif

		//union_tree( temp, temp_idom, true );

		for(l_it i = preds.begin(); i != preds.end(); i++)
		{
			typename _CFG::BBType *bb2((*i)->NodeInfo);

			

		#ifdef _DEBUG_BCHECK
			std::cout << *bb << " - Predecessor: " << *bb2 << std::endl;
		#endif

		#ifdef _DEBUG_BCHECK
			std::cout << "Idom: " << *idom << std::endl;
		#endif

		temp2 = check_SNDPKT_tree( bb2, idom, visitedBB);

		//		std::map<string, Value >::iterator it4( temp2["pkt"].begin());
		//for( ; it4 != temp2["pkt"].end() ; it4++)
		//{
		//	std::cout <<  "\t " << bb->getId() << " temp2 variable " << it4->first << " has value " << (it4->second).value << std::endl;
		//}

		if( !isLoop ) 
		{

			if( !temp2.empty() )
			{
				if( temp.empty() )
				{
					temp = temp2;

				} else {


					union_predecessors_tree( temp, temp2, "pkt");

					union_predecessors_tree( temp, temp2, "info");

					union_predecessors_tree( temp, temp2, "data");
			
				} // end else if( !temp.empty() )

			} else { // if( !temp2.empty() )

				isNull = true;
			
				//return temp_idom;
			}

		

	
		//std::map<string, Value >::iterator it3( temp2["pkt"].begin());
		//for( ; it3 != temp2["pkt"].end() ; it3++)
		//{
		//	std::cout <<  "\t " << bb->getId() << " temp2 variable " << it3->first << " has value " << (it3->second).value << std::endl;
		//}

		} else {
	
			isLoop = false;

		}

		} //end for

		if( isNull == true)
		{
			isNull = false;
			temp =  temp_idom;

		}else {
		
			union_tree( temp, temp_idom, true );
		}

		

		if( pkt_map.find(bb->getId()) != pkt_map.end())
		{
			update_tree("pkt", bb, temp);
		}

		if( info_map.find(bb->getId()) != info_map.end())
		{
			update_tree("info", bb, temp);
		} // end if( info_map.find(bb->getId()) != info_map.end()

		if( data_map.find(bb->getId()) != data_map.end())
		{
			update_tree("data", bb, temp);

		} // end if( data_map.find(bb->getId()) != data_map.end())

		//	std::map<string, Value >::iterator it4( temp_idom["pkt"].begin());
		//for( ; it4 != temp_idom["pkt"].end() ; it4++)
		//{
		//	std::cout <<  "\t " << bb->getId() << " temp_idom dopo union variable " << it4->first << " has value " << (it4->second).value << std::endl;
		//}

		idom_value[bb->getId()] = temp;

		return temp;


	} else if( bb->getPredecessors().size() == 1) // end list->size() > 1
	{
		typename _CFG::BBType *bb2(*(bb->precBegin()));
				
		#ifdef _DEBUG_BCHECK
			std::cout << "has only one predecessor: " << *bb2 <<std::endl;
		#endif

		if(idom != bb2)
		{
			temp = check_SNDPKT_tree( bb2, idom, visitedBB);
		}

		if( !temp.empty() )
		{
			if( pkt_map.find(bb->getId()) != pkt_map.end())
			{
				
				update_tree("pkt", bb, temp);

			} // if( pkt_map.find(bb->getId()) != pkt_map.end())

			if( info_map.find(bb->getId()) != info_map.end())
			{

				update_tree("info", bb, temp);

			} // end if( info_map.find(bb->getId()) != info_map.end()

			if( data_map.find(bb->getId()) != data_map.end())
			{
				update_tree("data", bb, temp);		
			} // end if( data_map.find(bb->getId()) != data_map.end())
			
			
		} else  // end !temp.empty()
		{


			if( pkt_map.find(bb->getId()) != pkt_map.end())
			{
			
				temp.insert(make_pair( "pkt",  pkt_map[bb->getId()] ));

			}

			if( info_map.find(bb->getId()) != info_map.end())
			{

				temp.insert(make_pair( "info",  info_map[bb->getId()] ));

			}

			if( data_map.find(bb->getId()) != data_map.end())
			{

				temp.insert(make_pair( "data",  data_map[bb->getId()] ));

			}
		} // end temp.empty()


		idom_value[bb->getId()] = temp;

		return temp;

	} // end else

} else {

	#ifdef _DEBUG_BCHECK
		std::cout << "No predecessor for: " << *bb  << " " << bb->getId() <<std::endl;
	#endif

	if( pkt_map.find(bb->getId()) != pkt_map.end())
	{

		temp.insert(make_pair( "pkt",  pkt_map[bb->getId()] ));

	}

	if( info_map.find(bb->getId()) != info_map.end())
	{

		temp.insert(make_pair( "info",  info_map[bb->getId()] ));

	}

	if( data_map.find(bb->getId()) != data_map.end())
	{

		temp.insert(make_pair( "data",  data_map[bb->getId()] ));

	}

	idom_value[bb->getId()] = temp;

	return temp;

} // end No predecessor

return temp;
}


template<typename _CFG>
std::map<string, std::map<string, Value > >  Boundscheck_remover<_CFG>::apply_tree_modification(typename CFG::BBType* bb, typename CFG::BBType* idom , std::list<uint16_t> visitedBB  )
{
typedef typename DiGraph<typename CFG::BBType*>::GraphNode GraphNode;
typedef std::list<GraphNode*> p_list_t;
typedef typename p_list_t::iterator l_it;

//std::map<uint16_t, std::map<string, Value > >::iterator iter;

std::map<string, Value >::iterator it2;

std::map<string, std::map<string, Value > > temp;


std::list<uint16_t>::iterator it(visitedBB.begin() );

for( ; it != visitedBB.end() ; it++)
{
	if( (*it) == bb->getId() )
	{
		#ifdef _DEBUG_BCHECK
			std::cout << "BB: " << bb->getId() << " already visited" << std::endl;
		#endif
		return temp;

	}

}

visitedBB.insert( visitedBB.begin() , bb->getId() );


if( idom_tree_value.find(bb->getId()) != idom_tree_value.end() )
{
	#ifdef _DEBUG_BCHECK
		std::cout << "Value of BB: " << bb->getId() << " found in idom_tree_value " << visitedBB.size() << std::endl;
	#endif

	return idom_tree_value[bb->getId()];
}


if( bb->getPredecessors().size() > 0)
{

	if( bb->getPredecessors().size() > 1)
	{
		std::map<string, std::map<string, Value > > temp2, temp_idom;

		p_list_t preds( bb->getPredecessors() );

		typename CFG::BBType* idom;

		idom = bb->template getProperty< typename CFG::BBType* >("IDOM");

		#ifdef _DEBUG_BCHECK
			std::cout << " has " << bb->getPredecessors().size() << " predecessors: "  << std::endl;
		#endif

		if( idom_tree_value.find(idom->getId()) == idom_tree_value.end() )
		{
			temp_idom = apply_tree_modification( idom, NULL, visitedBB);
			idom_tree_value.insert(make_pair( idom->getId() , temp_idom ));
		
			#ifdef _DEBUG_BCHECK
				std::cout << "Inserted in idom_tree_value temp_idom for BB: " <<  idom->getId() << std::endl;
			#endif

		} 

		//std::map<string, Value >::iterator it2( temp_idom["pkt"].begin());
		//for( ; it2 != temp_idom["pkt"].end() ; it2++)
		//{
		//	std::cout <<  "\t " << bb->getId() << " temp_idom variable " << it2->first << " has value " << (it2->second).value << std::endl;
		//}

		#ifdef _DEBUG_BCHECK
		std::cout << "temp_Idom size: " << temp_idom["pkt"].size() << std::endl;
		#endif

		//union_tree( temp, temp_idom, true );

		for(l_it i = preds.begin(); i != preds.end(); i++)
		{
			typename _CFG::BBType *bb2((*i)->NodeInfo);

		#ifdef _DEBUG_BCHECK
			std::cout << *bb << " - Predecessor: " << *bb2 << std::endl;
		#endif

		#ifdef _DEBUG_BCHECK
			std::cout << "Idom: " << *idom << std::endl;
		#endif

		#ifdef _DEBUG_BCHECK
			std::cout << "apply check_branch for BB: " << *bb2 << std::endl;
		#endif

		check_branch(bb2);

		temp2 = apply_tree_modification( bb2, idom, visitedBB);

		
		if( temp.empty() )
		{
					temp = temp2;

		} else {
			
			std::map<string, Value >::iterator iter;
					
					if(temp.find("pkt") != temp.end() ) 
					{
						for( iter = temp["pkt"].begin() ; iter != temp["pkt"].end() ;iter++ )
						{
							if( (iter->second).isValid || temp2["pkt"][iter->first].isValid )
							{
								temp2["pkt"][iter->first].isValid = true;
							}
						}

					}

			
					if(temp.find("info") != temp.end() ) 
					{
						for( iter = temp["info"].begin() ; iter != temp["info"].end() ;iter++ )
						{
							if( (iter->second).isValid || temp2["info"][iter->first].isValid )
							{
								temp2["info"][iter->first].isValid = true;
							}
						}
					}

					if(temp.find("data") != temp.end() ) 
					{
						for( iter = temp["data"].begin() ; iter != temp["data"].end() ;iter++ )
						{
							if( (iter->second).isValid || temp2["data"][iter->first].isValid )
							{
								temp2["data"][iter->first].isValid = true;
							}
						}
					}


 
		} // end else if( temp.empty() )



	
		} //end for

	if( pkt_map.find(bb->getId()) != pkt_map.end() || info_map.find(bb->getId()) != info_map.end() || data_map.find(bb->getId()) != data_map.end())
			{
				tree_modification(bb, &temp2);
			}

	
		//it2 = temp["pkt"].begin();
		//for( ; it2 != temp["pkt"].end() ; it2++)
		//{
		//	std::cout <<  "\t " << bb->getId() << " temp_idom variable " << it2->first << " has value " << (it2->second).value << std::endl;
		//}

		idom_tree_value.insert(make_pair( bb->getId(), temp2));

		return temp2;


	} else if( bb->getPredecessors().size() == 1) // end list->size() > 1
	{
		typename _CFG::BBType *bb2(*(bb->precBegin()));
				
		#ifdef _DEBUG_BCHECK
			std::cout << "has only one predecessor: " << *bb2 <<std::endl;
		#endif

		if(idom == bb2)
		{
			#ifdef _DEBUG_BCHECK
				std::cout << "Found IDOM " << *bb2 <<std::endl;
			#endif
			temp = idom_tree_value[ idom->getId() ];
			

		} else{

			temp = apply_tree_modification( bb2, idom, visitedBB);

		}

		if( pkt_map.find(bb->getId()) != pkt_map.end() || info_map.find(bb->getId()) != info_map.end() || data_map.find(bb->getId()) != data_map.end())
			{
			tree_modification(bb, &temp);
			
			}
		//it2 = temp["pkt"].begin();
		//for( ; it2 != temp["pkt"].end() ; it2++)
		//{
		//	std::cout <<  "\t " << bb->getId() << " temp_idom variable " << it2->first << " has value " << (it2->second).value << std::endl;
		//}

		idom_tree_value.insert(make_pair( bb->getId(), temp));
		
		return temp;

	} // end else

} else {

	#ifdef _DEBUG_BCHECK
		std::cout << "No predecessor for: " << *bb  << " " << bb->getId() <<std::endl;
	#endif

	temp = sndpkt_tree;

	if( pkt_map.find(bb->getId()) != pkt_map.end() || info_map.find(bb->getId()) != info_map.end() || data_map.find(bb->getId()) != data_map.end())
	{
		tree_modification(bb, &temp);
	}

		//it2 = temp["pkt"].begin();
		//for( ; it2 != temp["pkt"].end() ; it2++)
		//{
		//	std::cout <<  "\t " << bb->getId() << " temp_idom variable " << it2->first << " has value " << (it2->second).value << std::endl;
		//}

	idom_tree_value.insert(make_pair( bb->getId(), temp));

	return temp;

} // end No predecessor

return temp;
}



template<typename _CFG>
void Boundscheck_remover<_CFG>::tree_modification(typename CFG::BBType* bb, std::map<string, std::map<string, Value > >*  temp_tree)
{
	bool clr = false;

	typename _CFG::BBType::IRStmtNodeIterator j;

	for(j = bb->codeBegin(); j != bb->codeEnd(); j++)
	{
		typename _CFG::IRType *ir(*j);
		typename _CFG::IRType::IRNodeIterator k(ir->nodeBegin());
			
		for(; k != ir->nodeEnd(); k++)
		{
			typename _CFG::IRType *n(*k);
			

			if( (n)->getOpcode() == PCHECK  )
			{
				//std::cout << "BB : " << bb->getId() << " found pcheck " << std::endl;
				clr = checkModifierTree(n, bb, "pkt", pkt_map[bb->getId()] , temp_tree);
				
				if(clr)
				{
					pkt_ncheck_removed++;
				}
				
				break;

			}

			if( (n)->getOpcode() == ICHECK  )
			{
				clr = checkModifierTree(n, bb, "info", info_map[bb->getId()], temp_tree);
			
				if(clr)
				{
					info_ncheck_removed++;
				}				
				
				break;

			}

			if( (n)->getOpcode() == DCHECK  )
			{
				clr = checkModifierTree(n, bb, "data", data_map[bb->getId()], temp_tree);
				
				if(clr)
				{
					data_ncheck_removed++;
				}
				break;

			}

		}

	}
		
}


template<typename _CFG>
bool Boundscheck_remover<_CFG>::checkModifierTree(typename _CFG::IRType *n, typename CFG::BBType* bb, string typemem,  std::map<string, Value > bb_map, std::map<string, std::map<string, Value > >* map)
{
	jit::MIRNode * kid1((n)->getKid(0));		
	jit::MIRNode * kid2((n)->getKid(1));

	//jit::MIRNode * temp;

	jit::ConstNode *leftkidconst, *rightkidconst;

	std::stringstream ss;
	
	if( !isLocLoad(kid2))
	{

		if( !isAccMem(n) )
		{
			if(kid1->isConst() && kid2->isConst())
			{
				string code = "constant";
						
				if( (*map)[typemem].find(code) != (*map)[typemem].end()  )
				{
					
					if(bb_map[code].value > (*map)[typemem][code].value)
					{
						#ifdef _DEBUG_BCHECK
							std::cout << "Check on constant in BB " <<  bb->getId() << " left" << std::endl;
						#endif
						
						n->setIsNotValid(false);
						return false;

					}

						if( (*map)[typemem][code].isValid)
						{
							leftkidconst = (jit::ConstNode*) kid1;
							rightkidconst = (jit::ConstNode*) kid2;
							leftkidconst->setValue((*map)[typemem][code].value);
							rightkidconst->setValue(0);

							// Setto flag a false così che non venga utilizzato più in questo BB
							(*map)[typemem][code].isValid = false;

							n->setIsNotValid(false);

							#ifdef _DEBUG_BCHECK
								std::cout << "Check on constant in BB " <<  bb->getId() << " modified" << std::endl;
							#endif

							
						} else
						{
							#ifdef _DEBUG_BCHECK
								std::cout << *bb << " Check on Constant is already inserted "  << std::endl;
							#endif

							return true ;
						}

						return false;
					
										
				} else {

					#ifdef _DEBUG_BCHECK
						std::cout << *bb << "Error variable not found in tree map " << std::endl;
					#endif

					n->setIsNotValid(false);
					return false;
				}
			
			} 
			
			if(isLocLoad(kid1))
			{

				if( (kid1)->getOpcode() == LDREG)
				{
					#ifdef _DEBUG_BCHECK
						std::cout << "case (kid1) == LDREG" << std::endl;
					#endif

					string code = printRegType(kid1);
											
					if( (*map)[typemem].find(code) != (*map)[typemem].end() )
					{
						if(bb_map[code].value > (*map)[typemem][code].value)
						{
							#ifdef _DEBUG_BCHECK
								std::cout << "Check on " << code << " in BB " <<  bb->getId() << " left" << std::endl;
							#endif
							n->setIsNotValid(false);
							return false;

						}
							if( (*map)[typemem][code].isValid)
							{
								
								rightkidconst = (jit::ConstNode*) kid2;
								rightkidconst->setValue((*map)[typemem][code].value);

								// Setto flag a false così che non venga utilizzato più in questo BB
								(*map)[typemem][code].isValid = false;

								n->setIsNotValid(false);

								#ifdef _DEBUG_BCHECK
									std::cout << "Check on " << code << " in BB " <<  bb->getId() << " modified" << std::endl;
								#endif

							
							} else
							{
								#ifdef _DEBUG_BCHECK
									std::cout << *bb << " Check on " << code << " is already inserted "  << std::endl;
								#endif
								return true ;
							}

					

						return false;
					
		
					} else {

						#ifdef _DEBUG_BCHECK
							std::cout << *bb << " Error variable not found in tree map " << std::endl;
						#endif

						n->setIsNotValid(false);
						return false;
			
					}
			
				} else {   // end kid1==LDREG

					if((kid1)->getOpcode() == ADD || (kid1)->getOpcode() == ADDSOV || (kid1)->getOpcode() == ADDSOV)
					{
						#ifdef _DEBUG_BCHECK
							std::cout << "case ADD" << std::endl;
						#endif
	
						if( ((kid1)->getKid(0))->getOpcode() == LDREG  &&  ((kid1)->getKid(1))->getOpcode() != LDREG )
						{
							#ifdef _DEBUG_BCHECK
								std::cout << "case (kid1)->getKid(0) == LDREG" << std::endl;
							#endif

							string code = printRegType(kid1->getKid(0));
			
							
							if( (*map)[typemem].find(code) != (*map)[typemem].end() )
							{
									if(bb_map[code].value > (*map)[typemem][code].value)
									{
										#ifdef _DEBUG_BCHECK
											std::cout << "Check on " << code << " in BB " <<  bb->getId() << " left" << std::endl;
										#endif
										n->setIsNotValid(false);
										return false;

									}
									if( (*map)[typemem][code].isValid)
									{
										
										rightkidconst = (jit::ConstNode*) kid2;
										rightkidconst->setValue((*map)[typemem][code].value);
										n->setKid(kid1->getKid(0), 0);

										// Setto flag a false così che non venga utilizzato più in questo BB
										(*map)[typemem][code].isValid = false;

										n->setIsNotValid(false);

										#ifdef _DEBUG_BCHECK
											std::cout << "Check on " << code << " in BB " <<  bb->getId() << " modified" << std::endl;
										#endif

							
									} else
									{
										#ifdef _DEBUG_BCHECK
											std::cout << *bb << " Check on " << code << " is already inserted "  << std::endl;
										#endif
										return true ;
									}

									return false;
							
							} else {

								#ifdef _DEBUG_BCHECK
								std::cout << "Error variable not found in tree map " << std::endl;
								#endif

								n->setIsNotValid(false);
								return false;
			
							}
				
			
						} else if( ((kid1)->getKid(1))->getOpcode() == LDREG  && ((kid1)->getKid(0))->getOpcode() != LDREG)
						{
							#ifdef _DEBUG_BCHECK
								std::cout << "case (kid1)->getKid(1) == LDREG" << std::endl;
							#endif

							string code = printRegType(kid1->getKid(1));
			
							if(  (*map)[typemem].find(code) != (*map)[typemem].end() )
							{
									if(bb_map[code].value > (*map)[typemem][code].value)
									{
										n->setIsNotValid(false);
										return false;
									}

									if( (*map)[typemem][code].isValid)
									{
										rightkidconst = (jit::ConstNode*) kid2;
										rightkidconst->setValue((*map)[typemem][code].value);
										n->setKid(kid1->getKid(1), 0);

										// Setto flag a false così che non venga utilizzato più in questo BB
										(*map)[typemem][code].isValid = false;

										n->setIsNotValid(false);

										#ifdef _DEBUG_BCHECK
											std::cout << "Check on " << code << " in BB " <<  bb->getId() << " modified" << std::endl;
										#endif

							
									} else
									{
										#ifdef _DEBUG_BCHECK
											std::cout << *bb << " Check on " << code << " is already inserted "  << std::endl;
										#endif
										return true ;
									}

									return false;
							
							} else {

								#ifdef _DEBUG_BCHECK
									std::cout << "Error variable not found in tree map " << std::endl;
								#endif

								n->setIsNotValid(false);
								return false;
			
							}
						}

					} //END KID1->getOpcode() == ADD
				} // end else
				
			} // end isLocLoad(kid1)

		} else { 
			#ifdef _DEBUG_BCHECK
			std::cout << "BB: " << bb->getId()  << " - found boundcheck with offset from memoria" <<  std::endl;
			#endif
			
			n->setIsNotValid(false);
		}
	} else { 
		#ifdef _DEBUG_BCHECK
			std::cout << "BB: " << bb->getId()  << " - Found boundcheck with dimension in a local variable" <<  std::endl;
		#endif
		
		n->setIsNotValid(false);
	}	

	n->setIsNotValid(false);

	return false;
} // end check


template<typename _CFG>
void Boundscheck_remover<_CFG>::removeMIRnodeTree()
{
	std::list<typename _CFG::BBType*> *list(cfg.getBBList());

	typename std::list<typename _CFG::BBType*>::iterator i;
	
	for(i = list->begin(); i != list->end(); ++i)
	{
		typename _CFG::BBType *bb(*i);

		removeMIRnode(bb);

	}



 
}

template<typename _CFG>
void Boundscheck_remover<_CFG>::SetInvalidCheckMIRnodeTree()
{
		
	std::list<typename _CFG::BBType*> *list(cfg.getBBList());

	typename std::list<typename _CFG::BBType*>::iterator i;
	
	for(i = list->begin(); i != list->end(); ++i)
	{
		typename _CFG::BBType *bb(*i);

		typename _CFG::BBType::IRStmtNodeIterator j;

		for(j = bb->codeBegin(); j != bb->codeEnd(); j++)
		{
			typename _CFG::IRType *ir(*j);
			typename _CFG::IRType::IRNodeIterator k(ir->nodeBegin());
			
			for(; k != ir->nodeEnd(); k++)
			{
				typename _CFG::IRType *n(*k);
			

				if( (n)->getOpcode() == PCHECK  )
				{
					n->setIsNotValid(true);
					
					break;

				}

				if( (n)->getOpcode() == ICHECK  )
				{
					n->setIsNotValid(true);
					
					break;
				}

				if( (n)->getOpcode() == DCHECK  )
				{
					n->setIsNotValid(true);
					
					break;
	
				}

		}

	}
		

	}

 
}



template<typename _CFG>
void Boundscheck_remover<_CFG>::check_branch(typename CFG::BBType* bb)
{
	int bb_in_sendpkt_tree = 0;

	if(  bb->getPredecessors().size() <= 1 )
	{
		typedef typename DiGraph<typename CFG::BBType*>::GraphNode GraphNode;
		typedef std::list<GraphNode*> s_list_t;
		typedef typename s_list_t::iterator l_it;

		s_list_t succs( bb-> getSuccessors() );

		l_it i = succs.begin();

		for( ; i != succs.end() ;i++)
		{
			if( sendpkt_tree_bb.find(((*i)->NodeInfo)->getId() ) != sendpkt_tree_bb.end() )
			{
				#ifdef _DEBUG_BCHECK
				std::cout << "BB: "<< bb->getId()  << " has successor in send_pkt_tree BB: " << ((*i)->NodeInfo)->getId() << std::endl;
				#endif

				bb_in_sendpkt_tree++;
			
			}

		}

		if( bb_in_sendpkt_tree == 1)
		{
			#ifdef _DEBUG_BCHECK
				std::cout << "BB: "<< bb->getId()  << " update branch_value" << std::endl;
			#endif

			if( pkt_map.find(bb->getId()) != pkt_map.end() )
			{
				update_branch_value( pkt_map[bb->getId()], "pkt" );
			}

			if( info_map.find(bb->getId()) != info_map.end() )
			{
				update_branch_value( info_map[bb->getId()], "info" );
			}

			if( data_map.find(bb->getId()) != data_map.end() )
			{
				update_branch_value( data_map[bb->getId()], "data" );
			}

			if( !(bb->getPredecessors().size() == 0) )
			{
				check_branch( *(bb->precBegin()) );
			}

			apply_branch_value(bb);



		}
		


	}

}




template<typename _CFG>
void Boundscheck_remover<_CFG>::update_branch_value( std::map<string, Value > map , string typemem)
{
	std::map<string, Value >::iterator it = map.begin();

	if( branch_value.find(typemem) != branch_value.end() )
	{
		for( ; it != map.end() ; it++)
		{
			if( branch_value[typemem].find(it->first) != branch_value[typemem].end() )
			{
				if( (it->second).value > branch_value[typemem][it->first].value)
				{
					branch_value[typemem][it->first].value = (it->second).value;

				}

			} else {
		
				branch_value[typemem].insert(make_pair(it->first, it->second));
		
			}

		} 

	} else {

		for( ; it != map.end() ; it++)
		{
			branch_value[typemem].insert(make_pair(it->first, it->second));
		}



	}





}


template<typename _CFG>
void Boundscheck_remover<_CFG>::apply_branch_value( typename CFG::BBType* bb)
{
	bool clr = false;

	check_not_optimizable = true;

	typename _CFG::IRType *istr = NULL;

		typename _CFG::BBType::IRStmtNodeIterator j, l;
		
		for(j = bb->codeBegin(); j != bb->codeEnd(); )
		{
			typename _CFG::IRType *ir(*j);
			typename _CFG::IRType::IRNodeIterator k(ir->nodeBegin());
			
			for(; k != ir->nodeEnd(); k++)
			{
				typename _CFG::IRType *n(*k);
				istr = n;

				

				if( (n)->getOpcode() == PCHECK  )
				{
					clr = branchModifier(n, bb, "pkt", &pkt_map[bb->getId()]);
					if(clr)
					{
						pkt_ncheck_removed++;
					}


					break;
					
				} // end if PCHECK

				if( (n)->getOpcode() == ICHECK  )
				{
					clr = branchModifier(n, bb, "info", &info_map[bb->getId()]);
					if(clr)
					{
						info_ncheck_removed++;
					}

			
					break;

				} // end if ICHECK

				if( (n)->getOpcode() == DCHECK  )
				{

					clr = branchModifier(n, bb, "data", &(data_map[bb->getId()]));
					if(clr)
					{
						data_ncheck_removed++;
					}

					break;
					
	
				} // end if ICHECK


			} //node 

			l = j;
			++j;

			if( clr )
			{
				#ifdef _DEBUG_BCHECK
				std::cout << "BB: "<< bb->getId()  << " remove instruction: " << (*istr)<< std::endl;
				#endif
				bb->eraseCode(l);
				delete istr;
				clr = false;
			}

		} // code_list  


}


template<typename _CFG>
bool Boundscheck_remover<_CFG>::branchModifier(typename _CFG::IRType *n, typename CFG::BBType* bb, string typemem,  std::map<string, Value > * map)
{
		jit::MIRNode * kid1((n)->getKid(0));		
	jit::MIRNode * kid2((n)->getKid(1));

	//jit::MIRNode * temp;

	jit::ConstNode *leftkidconst, *rightkidconst;

	std::stringstream ss;
	
	if( !isLocLoad(kid2))
	{

		if( !isAccMem(n) )
		{
			if(kid1->isConst() && kid2->isConst())
			{
				string code = "constant";
						
				if( branch_value[typemem].find(code) !=  branch_value[typemem].end()  )
				{
					if( (*map).find(code) == (*map).end() )
					{
						#ifdef _DEBUG_BCHECK
							std::cout << "Error: not found " << code << " in BB " <<  bb->getId() <<  std::endl;
						#endif

						return false;

					}

					leftkidconst = (jit::ConstNode*) kid1;
					rightkidconst = (jit::ConstNode*) kid2;
					leftkidconst->setValue( branch_value[typemem][code].value);
					rightkidconst->setValue(0);


					#ifdef _DEBUG_BCHECK
						std::cout << "Check on constant in BB " <<  bb->getId() << " modified" << std::endl;
					#endif

					branch_value[typemem].erase(code);
							
					return false;
					
										
				} else {

					#ifdef _DEBUG_BCHECK
						std::cout << *bb << "Check is already added " << std::endl;
					#endif

					
					return true;
				}
			
			} 
			
			if(isLocLoad(kid1))
			{

				if( (kid1)->getOpcode() == LDREG)
				{
					#ifdef _DEBUG_BCHECK
						std::cout << "case (kid1) == LDREG" << std::endl;
					#endif

					string code = printRegType(kid1);
											
				if( branch_value[typemem].find(code) !=  branch_value[typemem].end()  )
				{
					if( (*map).find(code) == (*map).end() )
					{
						#ifdef _DEBUG_BCHECK
							std::cout << "Error: not found " << code << " in BB " <<  bb->getId() << std::endl;
						#endif

						return false;

					}

					
					rightkidconst = (jit::ConstNode*) kid2;
					rightkidconst->setValue(branch_value[typemem][code].value);


					#ifdef _DEBUG_BCHECK
						std::cout << "Check on constant in BB " <<  bb->getId() << " modified" << std::endl;
					#endif

					branch_value[typemem].erase(code);
							
					return false;
					
										
				} else {

					#ifdef _DEBUG_BCHECK
						std::cout << *bb << "Check is already added " << std::endl;
					#endif

					
					return true;
				}
			
				} else {   // end kid1==LDREG

					if((kid1)->getOpcode() == ADD || (kid1)->getOpcode() == ADDSOV || (kid1)->getOpcode() == ADDSOV)
					{
						#ifdef _DEBUG_BCHECK
							std::cout << "case ADD" << std::endl;
						#endif
	
						if( ((kid1)->getKid(0))->getOpcode() == LDREG  &&  ((kid1)->getKid(1))->getOpcode() != LDREG )
						{
							#ifdef _DEBUG_BCHECK
								std::cout << "case (kid1)->getKid(0) == LDREG" << std::endl;
							#endif

							string code = printRegType(kid1->getKid(0));
			
							
							if( branch_value[typemem].find(code) !=  branch_value[typemem].end()  )
							{
								if( (*map).find(code) == (*map).end() )
								{
									#ifdef _DEBUG_BCHECK
										std::cout << "Error: not found " << code << " in BB " <<  bb->getId() << std::endl;
									#endif

									return false;

								}
										
								rightkidconst = (jit::ConstNode*) kid2;
								rightkidconst->setValue(branch_value[typemem][code].value);
								n->setKid(kid1->getKid(0), 0);

								#ifdef _DEBUG_BCHECK
									std::cout << "Check on constant in BB " <<  bb->getId() << " modified" << std::endl;
								#endif
		
								branch_value[typemem].erase(code);
							
								return false;
					
										
							} else {

								#ifdef _DEBUG_BCHECK
									std::cout << *bb << "Check is already added " << std::endl;
								#endif

								return true;
							}
			
			
						} else if( ((kid1)->getKid(1))->getOpcode() == LDREG  && ((kid1)->getKid(0))->getOpcode() != LDREG)
						{
							#ifdef _DEBUG_BCHECK
								std::cout << "case (kid1)->getKid(1) == LDREG" << std::endl;
							#endif

							string code = printRegType(kid1->getKid(1));
			
							if( branch_value[typemem].find(code) !=  branch_value[typemem].end()  )
							{
								if( (*map).find(code) == (*map).end() )
								{
									#ifdef _DEBUG_BCHECK
										std::cout << "Error: not found " << code << " in BB " <<  bb->getId() << std::endl;
									#endif

									return false;

								}
								
								rightkidconst = (jit::ConstNode*) kid2;
								rightkidconst->setValue(branch_value[typemem][code].value);
								n->setKid(kid1->getKid(1), 0);

								#ifdef _DEBUG_BCHECK
									std::cout << "Check on constant in BB " <<  bb->getId() << " modified" << std::endl;
								#endif

								branch_value[typemem].erase(code);
							
								return false;
					
										
							} else {

								#ifdef _DEBUG_BCHECK
									std::cout << *bb << "Check is already added " << std::endl;
								#endif

								return true;
							}
			
						}

					} //END KID1->getOpcode() == ADD
				} // end else
				
			} // end isLocLoad(kid1)

		} else { 
			#ifdef _DEBUG_BCHECK
			std::cout << "BB: " << bb->getId()  << " - found boundcheck with offset from memoria" <<  std::endl;
			#endif
			
			n->setIsNotValid(false);
		}
	} else { 
		#ifdef _DEBUG_BCHECK
			std::cout << "BB: " << bb->getId()  << " - Found boundcheck with dimension in a local variable" <<  std::endl;
		#endif
		
		n->setIsNotValid(false);
	}	

	n->setIsNotValid(false);

	return false;

	return false;
}


template<typename _CFG>
bool Boundscheck_remover<_CFG>::isAccMem(typename _CFG::IRType *n)
{
	bool tmp1 = false, tmp2 = false;
	jit::MIRNode * kid1((n)->getKid(0));		
	jit::MIRNode * kid2((n)->getKid(1));	

	if(n->isMemLoad())
	{
		return true;						
			
	} else {

		if( kid1 != NULL ) tmp1 = isAccMem(kid1);

		if(tmp1){
			
			return true;
		}

		if( kid2 != NULL) tmp2 = isAccMem(kid2);

		if(tmp2){
	
			return true;
		}
	}

return false;
}

template<typename _CFG>
bool Boundscheck_remover<_CFG>::isLocLoad(typename _CFG::IRType *n)
{
	bool tmp1 = false, tmp2 = false;
	jit::MIRNode * kid1((n)->getKid(0));		
	jit::MIRNode * kid2((n)->getKid(1));	

	string codice = nvmOpCodeTable[(n)->getOpcode()].CodeName ;

	if( (n)->getOpcode() == LDREG)
	{

		return true;						
			
	} else {

		if( kid1 != NULL ) tmp1 = isLocLoad(kid1);

		if(tmp1){
	
			return true;
		}

		if( kid2 != NULL) tmp2 = isLocLoad(kid2);

		if(tmp2){
			
			return true;
		}
	}


return false;
}

template<typename _CFG>
string Boundscheck_remover<_CFG>::printRegType(typename _CFG::IRType *n)
{
	 std::ostringstream stm ,stm2, stm3;
	
	stm << (((n)->getDefReg()).get_model())->get_space();
	stm2 << (((n)->getDefReg()).get_model())->get_name();
	stm3 << ((n)->getDefReg()).version();

	return "r" + stm.str() + "." + stm2.str() + "." + stm3.str()  ;
}



template<typename _CFG>
void Boundscheck_remover<_CFG>::constKids(typename CFG::IRType *n, typename _CFG::BBType* bb, std::map<string, Value > *regload)
{
	int32_t temp;
	jit::MIRNode * kid1((n)->getKid(0));		
	jit::MIRNode * kid2((n)->getKid(1));	
	temp = kid1->getValue() + kid2->getValue();


	#ifdef _DEBUG_BCHECK
		std::cout << "constant" << ": new value of offset: " << temp << " for BB: " << bb->getId() << std::endl;
	#endif

	string code = "constant";

	std::map<string, Value >::iterator iter =  (*regload).find(code);
						
	if( iter == (*regload).end() )
	{
		//Value v = {true, kid1->getValue() + kid2->getValue() , kid1->getValue() + kid2->getValue() };
		Value v = {true, kid1->getValue() + kid2->getValue() };
		(*regload).insert(make_pair(code, v ));
					
	} else {

					
		if(temp >  ((*regload)[code]).value)
		{
			#ifdef _DEBUG_BCHECK
				std::cout << "New value of " << code << std::endl;
				std::cout << "BEFORE" << std::endl;
				std::cout << "Offset max: " << (*regload)[code].value << std::endl;
				std::cout << "BB: " << bb->getId() << std::endl;
			#endif

			(*regload)[code].value = temp;	
			//(*regload)[code].max_value_dominators = temp;
							
			#ifdef _DEBUG_BCHECK
				std::cout << "AFTER" << std::endl;
				std::cout << "Offset max: " << (*regload)[code].value << std::endl;
				std::cout << "BB: " << bb->getId() << std::endl;
			#endif

		} // fine if offset_max

	} //fine else

}


template<typename _CFG>
void Boundscheck_remover<_CFG>::locKids(typename CFG::IRType *n, typename _CFG::BBType* bb, std::map<string, Value > *regload)
{
	jit::MIRNode * kid1((n)->getKid(0));		
	jit::MIRNode * kid2((n)->getKid(1));	

	if( (kid1)->getOpcode() == LDREG)
	{
		string code = printRegType(kid1);

		#ifdef _DEBUG_BCHECK
			std::cout << code << ": new value of offset: " << (kid2)->getValue() << " for BB: " << bb->getId() << std::endl;
		#endif

		std::map<string, Value >::iterator iter =  (*regload).find( code );

		if(iter == (*regload).end())
		{
			//Value v = {true,  (kid2)->getValue() , (kid2)->getValue() };
			Value v = {true,  (kid2)->getValue() };
			(*regload).insert( make_pair( code , v ));
			
		} else {

			if( (*regload)[code].value < (kid2)->getValue() ) 
			{
				#ifdef _DEBUG_BCHECK
					std::cout << "New value of " << code << std::endl;
					std::cout << "BEFORE" << std::endl;
					std::cout << "Offset max: " << (*regload)[code].value << std::endl;
					std::cout << "BB: " << bb->getId() << std::endl;
				#endif
				
				(*regload)[code].value = (kid2)->getValue();
				//(*regload)[code].max_value_dominators = (kid2)->getValue();

				#ifdef _DEBUG_BCHECK
					std::cout << "AFTER" << std::endl;
					std::cout << "Offset max: " << (*regload)[code].value << std::endl;
					std::cout << "BB: " << bb->getId() << std::endl;
				#endif
			}
		} // end else

	} else {// end kid1==LDREG

		if((kid1)->getOpcode() == ADD || (kid1)->getOpcode() == ADDSOV || (kid1)->getOpcode() == ADDSOV)
		{

			if( ((kid1)->getKid(0))->getOpcode() == LDREG  &&  ((kid1)->getKid(1))->getOpcode() != LDREG )
			{
				string code = printRegType((kid1)->getKid(0));

				#ifdef _DEBUG_BCHECK
					std::cout << code << ": new value of offset: " << ((kid1)->getKid(1))->getValue() + (kid2)->getValue() << " for BB: " << bb->getId() << std::endl;
				#endif

				map<string, Value >::iterator iter =  (*regload).find( code );

				if(iter == (*regload).end())
				{
					//Value v = {true, ((kid1)->getKid(1))->getValue() + (kid2)->getValue(), ((kid1)->getKid(1))->getValue() + (kid2)->getValue() };
					Value v = {true, ((kid1)->getKid(1))->getValue() + (kid2)->getValue() };
					(*regload).insert( make_pair( code , v ));

				} else {
				
					if( (*regload)[code].value <  ((kid1)->getKid(1))->getValue() + (kid2)->getValue() ) 
					{
						#ifdef _DEBUG_BCHECK
							std::cout << "New value of " << code << std::endl;
							std::cout << "BEFORE" << std::endl;
							std::cout << "Offset max: " << (*regload)[code].value << std::endl;
							std::cout << "BB: " << bb->getId() << std::endl;
						#endif
	
						(*regload)[code].value = ((kid1)->getKid(1))->getValue() + (kid2)->getValue();
						//(*regload)[code].max_value_dominators = ((kid1)->getKid(1))->getValue() + (kid2)->getValue();

						#ifdef _DEBUG_BCHECK
							std::cout << "AFTER" << std::endl;
							std::cout << "Offset max: " << (*regload)[code].value << std::endl;
							std::cout << "BB: " << bb->getId() << std::endl;
						#endif

					}
				}

				// end (kid1)->getKid(0) == LDREG
			} else if( ((kid1)->getKid(1))->getOpcode() == LDREG  && ((kid1)->getKid(0))->getOpcode() != LDREG)
			{
				string code = printRegType((kid1)->getKid(1));

				#ifdef _DEBUG_BCHECK
					std::cout << code << ": new value of offset: " << ((kid1)->getKid(0))->getValue() + (kid2)->getValue() << " for BB: " << bb->getId() << std::endl;
				#endif

				map<string, Value >::iterator iter =  (*regload).find( code );

				if(iter == (*regload).end())
				{
					//Value v = {true, ((kid1)->getKid(0))->getValue() + (kid2)->getValue(), ((kid1)->getKid(0))->getValue() + (kid2)->getValue() };
					Value v = {true, ((kid1)->getKid(0))->getValue() + (kid2)->getValue() };
					(*regload).insert( make_pair( code , v ));

				} else {
				
					if( (*regload)[code].value <  ((kid1)->getKid(0))->getValue() + (kid2)->getValue() ) 
					{
						#ifdef _DEBUG_BCHECK
							std::cout << "New value of " << code << std::endl;
							std::cout << "BEFORE" << std::endl;
							std::cout << "Offset max: " << (*regload)[code].value << std::endl;
							std::cout << "BB: " << bb->getId() << std::endl;
						#endif

							(*regload)[code].value = ((kid1)->getKid(0))->getValue() + (kid2)->getValue();
							//(*regload)[code].max_value_dominators = ((kid1)->getKid(0))->getValue() + (kid2)->getValue();

						#ifdef _DEBUG_BCHECK
							std::cout << "AFTER" << std::endl;
							std::cout << "Offset max: " << (*regload)[code].value << std::endl;
							std::cout << "BB: " << bb->getId() << std::endl;
						#endif

					}
				}

				// end (kid1)->getKid(1) == LDREG
			}

		} //END KID1->getOpcode() == ADD
	} // end else

} //end locKids



template<typename _CFG>
void Boundscheck_remover<_CFG>::setflag( string typemem , std::map<string, std::map<string, Value > >& temp )
{
	std::map<string, Value >::iterator it;

	if( temp.find(typemem) != temp.end() )
	{
		for( it = temp[typemem].begin() ; it != temp[typemem].end(); it++ )
		{
			it->second.isValid = true;

		}

	}

}




template<typename _CFG>
void Boundscheck_remover<_CFG>::union_tree( std::map<string, std::map<string, Value > > & m , std::map<string, std::map<string, Value > >  t, bool addflag)
{
	if( !t.empty() )
	{
		if( m.empty() )
		{
		m = t;

		} else {

			if(t.find("pkt") != t.end())
			{
				if(m.find("pkt") == m.end())
				{
					m.insert(make_pair("pkt", t["pkt"]));

				} else {

					std::map<string, Value >::iterator it( t["pkt"].begin() );
			
					for( ; it != t["pkt"].end() ; it++)
					{
						if( m["pkt"].find(it->first) != m["pkt"].end() )
						{
							if(addflag)
							{
								if( m["pkt"][it->first].value < (it->second).value)
								{
									m["pkt"][it->first].value = (it->second).value;
								}

							} else {
								if( m["pkt"][it->first].value > (it->second).value)
								{
									m["pkt"][it->first].value = (it->second).value;
								}
							}


						} else {
							m["pkt"].insert(make_pair(it->first, it->second));

						}

					}

				}
			}

			if(t.find("info") != t.end())
			{
				if(m.find("info") == m.end())
				{
					m.insert(make_pair("info", t["info"]));

				} else {

					std::map<string, Value >::iterator it( t["info"].begin() );
			
					for( ; it != t["info"].end() ; it++)
					{
						if( m["info"].find(it->first) != m["info"].end() )
						{
							if(addflag)
							{
								if( m["info"][it->first].value < (it->second).value)
								{
									m["info"][it->first].value = (it->second).value;
								}

							} else {
								if( m["info"][it->first].value > (it->second).value)
								{
									m["info"][it->first].value = (it->second).value;
								}
							}

						} else {
							m["info"].insert(make_pair(it->first, it->second));

						}

					}

				}
			}

			if(t.find("data") != t.end())
			{
				if(m.find("data") == m.end())
				{
					m.insert(make_pair("data", t["data"]));

				} else {

					std::map<string, Value >::iterator it( t["data"].begin() );
			
					for( ; it != t["data"].end() ; it++)
					{
						if( m["data"].find(it->first) != m["data"].end() )
						{
							if(addflag)
							{
								if( m["data"][it->first].value < (it->second).value)
								{
									m["data"][it->first].value = (it->second).value;
								}

							} else {
								if( m["data"][it->first].value > (it->second).value)
								{
									m["data"][it->first].value = (it->second).value;
								}
							}


						} else {
							m["data"].insert(make_pair(it->first, it->second));

						} //end else if( m["data"].find(it->first) != m["data"].end() )

					} // end for ( ; it != t["data"].end() ; it++)

				} // end  else if(m.find("data") == m.end())

			} // end if(t.find("data") != t.end())


		}  //  end else m.empty()

	} //  end !t.empty()

 
} // end union_tree



template<typename _CFG>
void Boundscheck_remover<_CFG>::union_predecessors_tree( std::map<string, std::map<string, Value > >  & m ,std::map<string, std::map<string, Value > >  t, string typemem)
{
	std::map<string, Value >::iterator it;

		
		if( t.find(typemem) != t.end() )
		{
			it = (t[typemem].begin());

			for( ; it != t[typemem].end() ; it++ )
			{
				if(m[typemem].find(it->first) != m[typemem].end() )
				{
					if( (m[typemem])[it->first].value > (it->second).value)
					{
						#ifdef _DEBUG_BCHECK
							std::cout << "value of " << it->first << " change from " << m[typemem][it->first].value << " to " << (it->second).value << std::endl;
						#endif
					
						(m[typemem])[it->first].value = (it->second).value;
					}

				} else {
					#ifdef _DEBUG_BCHECK
						std::cout << "Add value of " << it->first << " = " << (it->second).value << std::endl;
					#endif
	
					//m[typemem].insert(make_pair(it->first, it->second));
									
				}

			}

		} // end if( t.find(typemem) != t.end() )


		if( m.find(typemem) != m.end() )
		{
			it = (m[typemem].begin());

			for( ; it != m[typemem].end() ; it++ )
			{
				if(t[typemem].find(it->first) == t[typemem].end() )
				{
					#ifdef _DEBUG_BCHECK
						std::cout << "Delete " << it->first << " from temp" << std::endl;
					#endif

					m[typemem].erase(it);
				}

			}

		} // end if( t.find(typemem) != t.end() )

}

template<typename _CFG>
void Boundscheck_remover<_CFG>::update_tree( string typemem, typename CFG::BBType* bb, std::map<string, std::map<string, Value > >& temp)
{
	if( typemem == "pkt" )
	{
		std::map<string, Value >::iterator it((pkt_map[bb->getId()]).begin());

		for(; it != (pkt_map[bb->getId()]).end(); it++)
		{	
			if( temp[typemem].find(it->first) != temp[typemem].end())
			{
				if( (temp[typemem])[it->first].value < (it->second).value )
				{
					(temp[typemem])[it->first].value = (it->second).value;
				}

			} else {
				temp[typemem].insert( make_pair(it->first, it->second));
			}
		}
	} else if( typemem == "info" )
	{
		std::map<string, Value >::iterator it((info_map[bb->getId()]).begin());

		for(; it != (info_map[bb->getId()]).end(); it++)
		{	
			if( temp[typemem].find(it->first) != temp[typemem].end())
			{
				if( (temp[typemem])[it->first].value < (it->second).value )
				{
					(temp[typemem])[it->first].value = (it->second).value;
				}

			} else {
				temp[typemem].insert( make_pair(it->first, it->second));
			}
		}
	} else if( typemem == "data" )
	{
		std::map<string, Value >::iterator it((data_map[bb->getId()]).begin());

		for(; it != (data_map[bb->getId()]).end(); it++)
		{	
			if( temp[typemem].find(it->first) != temp[typemem].end())
			{
				if( (temp[typemem])[it->first].value < (it->second).value )
				{
					(temp[typemem])[it->first].value = (it->second).value;
				}

			} else {
				temp[typemem].insert( make_pair(it->first, it->second));
			}
		}
	}
}

template<typename _CFG>
void Boundscheck_remover<_CFG>::ScanMap()
{
	std::map<string,  Value >::iterator it;

	//if(!pkt_map.empty() || pkt_ncheck > 0)
	{
		
		std::cout << "PKT CHECK "  << std::endl;
	
	
		std::map<uint16_t, std::map<string, Value > >::iterator iter(pkt_map.begin());

		for(; iter != pkt_map.end(); iter++) {
	
			for ( it=(iter->second).begin() ; it !=(iter->second).end(); it++ )
			{
				// for not printing offset of the nodes removed from the IDOM
				//if( (it->second).value != -1 ){
				
					std::cout << "BB: " << (iter->first);
					std::cout << " " << (it->first) << " with max offset: " << (it->second).value << std::endl;

				//}
			
			}
			
		

		} // fine for iter pkt check


		std::cout << "PKT CHECK : Before = " << pkt_ncheck_before << " after = " << pkt_ncheck_before  - pkt_ncheck_removed  << std::endl;

	}
	//if(!info_map.empty() ||info_ncheck > 0)
	{

		std::cout << "INFO CHECK" << std::endl;
		
	
		std::map<uint16_t, std::map<string, Value > >::iterator iter2(info_map.begin());

		for(; iter2 != info_map.end(); iter2++) {

			for ( it=(iter2->second).begin() ; it !=(iter2->second).end(); it++ )
			{
				
					std::cout << "BB: " << (iter2->first);
					std::cout << " " << (it->first) << " with max offset: " << (it->second).value << std::endl;
			
			}

			

		} // fine for iter info check

		
		std::cout << "INFO CHECK : Before = " << info_ncheck_before << " after = " << info_ncheck_before  - info_ncheck_removed  << std::endl;
		
	} // end info_map.size()

	//if(!data_map.empty() || data_ncheck > 0 )
	{
		
		std::cout << "DATA CHECK" << std::endl;
	
	
		std::map<uint16_t, std::map<string, Value > >::iterator iter3(data_map.begin());

		for(; iter3 != data_map.end(); iter3++) {

			for ( it=(iter3->second).begin() ; it !=(iter3->second).end(); it++ )
			{			
					std::cout << "BB: " << (iter3->first);
					std::cout << " " << (it->first) << " with max offset: " << (it->second).value << std::endl;
			}

			

		} // fine for iter data check

		
		std::cout << "DATA CHECK : Before = " << data_ncheck_before << " after = " << data_ncheck_before  - data_ncheck_removed << std::endl;
		

	} // end data_map.size()


	// TREE

	std::cout << std::endl <<"TREE" << std::endl;

	if( sndpkt_tree.find("pkt") != sndpkt_tree.end() )
	{
		std::cout << std::endl <<"PKT" <<  std::endl;

		for( it = sndpkt_tree["pkt"].begin() ; it != sndpkt_tree["pkt"].end() ; it++ )
		{
			std::cout << "Variable " << it->first << " has offset " << it->second.value <<  std::endl;
		}

	}

	if( sndpkt_tree.find("info") != sndpkt_tree.end() )
	{
		std::cout << std::endl <<"INFO" <<  std::endl;

		for( it = sndpkt_tree["info"].begin() ; it != sndpkt_tree["info"].end() ; it++ )
		{
			std::cout << "Variable " << it->first << " has offset " << it->second.value <<  std::endl;
		}

	}


	if( sndpkt_tree.find("data") != sndpkt_tree.end() )
	{
		std::cout << std::endl <<"DATA" <<  std::endl;

		for( it = sndpkt_tree["data"].begin() ; it != sndpkt_tree["data"].end() ; it++ )
		{
			std::cout << "Variable " << it->first << " has offset " << it->second.value <<  std::endl;
		}

	}


} // fine ScanMap

