/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "encapfsa.h"
#include "DFAset.h"
#include "assignprotocollayer.h"
#include <iomanip> // for EncapFSA::prettyDotPrint()
#include <fstream> // for EncapFSA::prettyDotPrint()


#define TRANSFER_STATE_PROPERTIES(newfsa_ptr,newst_iter,oldst){         \
    if((oldst).isAccepting())                                           \
      newfsa_ptr->setAccepting(newst_iter);                             \
    else                                                                \
      newfsa_ptr->resetAccepting(newst_iter);                           \
    if((oldst).isFinal())                                               \
      newfsa_ptr->setFinal(newst_iter);                                 \
    else                                                                \
      newfsa_ptr->resetFinal(newst_iter);                               \
    if((oldst).isAction())       										\
 	{																	\
      newfsa_ptr->setAction(newst_iter);								\
      list<SymbolField*> fields = (&oldst)->GetExtraInfo();				\
      newfsa_ptr->SetExtraInfoToState(&(*newst_iter),fields);			\
      list<uint32> positions = (&oldst)->GetExtraInfoPlus();			\
      newfsa_ptr->SetExtraInfoPlusToState(&(*newst_iter),positions);	\
      list<bool> multifield = (&oldst)->GetOtherInfo();					\
      newfsa_ptr->SetOtherInfoToState(&(*newst_iter),multifield);		\
	}																	\
    else                                                                \
      newfsa_ptr->resetAction(newst_iter);                       	    \
  }

EncapGraph *EncapFSA::m_ProtocolGraph = NULL;

/***************************** Boolean operations *****************************/

void EncapFSA::migrateTransitionsAndStates(EncapFSA *to, EncapFSA *from, StateIterator init)
{
  //state mapping: first term is inputFSA stateiterator, second term is otputFSA stateiterator
  std::map<EncapFSA::State*, EncapFSA::State*> stateMap;
  // ExtendedTransition mapping
  std::map<EncapFSA::ExtendedTransition*, EncapFSA::ExtendedTransition*> et_map;

  EncapFSA::StateIterator init1 = from->GetInitialState();
  EncapFSA::StateIterator s = to->AddState((*init1).GetInfo());
  
  if((*init1).isAccepting())
    to->setAccepting(s);
  else 
    to->resetAccepting(s);
    
  if((*init1).isAction())
  {
  	to->setAction(s);
  	to->SetExtraInfoToState(&(*s),(&(*s))->GetExtraInfo());
  	to->SetExtraInfoPlusToState(&(*s),(&(*s))->GetExtraInfoPlus());
  	to->SetOtherInfoToState(&(*s),(&(*s))->GetOtherInfo());
  }
  else
  	to->resetAction(s);
  
  stateMap.insert( std::pair<EncapFSA::State*, EncapFSA::State*>(&(*init1),&(*s)) );
  to->AddEpsilon(init, s, NULL);
  
  for(EncapFSA::TransIterator t = from->FirstTrans(); t != from->LastTrans(); t++) //iterates on each transition of the from fsa
  {
      EncapFSA::State *fromState = &(*(*t).FromState());
      EncapFSA::State *toState = &(*(*t).ToState());

      std::map<EncapFSA::State*, EncapFSA::State*>::iterator i = stateMap.find(fromState);
      if(i == stateMap.end())
        {
          EncapFSA::StateIterator st = to->AddState(fromState->GetInfo());
          TRANSFER_STATE_PROPERTIES(to,st,*fromState);
          stateMap.insert(std::pair<EncapFSA::State*,EncapFSA::State*>(fromState,&(*st)) );
          fromState = &(*st);
        }
      else
        {
          fromState = i->second;
        }
      std::map<EncapFSA::State *, EncapFSA::State *>::iterator j = stateMap.find(toState);
      if(j == stateMap.end())
        {
          EncapFSA::StateIterator st2 = to->AddState(toState->GetInfo());
          TRANSFER_STATE_PROPERTIES(to,st2,*toState);
          stateMap.insert( std::pair<EncapFSA::State*,EncapFSA::State*>(toState,&(*st2)) );
          toState = &(*st2);
        }
      else
        {
          toState = j->second;
        }
      EncapFSA::StateIterator iterFrom = to->GetStateIterator(fromState);
      EncapFSA::StateIterator iterTo = to->GetStateIterator(toState);
      TransIterator new_t = to->AddTrans(iterFrom, iterTo, *t);

      // fix the references to the ExtendedTransitions
      ExtendedTransition *et_ref = (*t).getIncludingET();
      if(et_ref){
        // PRINT_DOT(to, "DBG nel loop di migrate", "migrate_loop");
        std::map<EncapFSA::ExtendedTransition*, EncapFSA::ExtendedTransition*>::iterator e = et_map.find(et_ref);
        if (e == et_map.end()){
              
          // duplicate the extended transition
          ExtendedTransition *new_et = new ExtendedTransition(et_ref);
          
          /*if(!new_et->startingNode->advancedPredicates.empty())
         	printf("The new ExtendedTransition has an AdvancedPredicate\n");*/

          // handle ET gates
          std::list<StateIterator> old_outgates = new_et->ToStates();
          State *old_ingate = new_et->FromState();
          std::map<State*, State*> new_outgates;
          State* new_ingate;

          // input gate
          std::map<EncapFSA::State*, EncapFSA::State*>::iterator in_gate_ref = stateMap.find(old_ingate);
          if(in_gate_ref == stateMap.end()) {
            // was not seen yet, handle it
            EncapFSA::StateIterator st = to->AddState((*old_ingate).GetInfo());
            TRANSFER_STATE_PROPERTIES(to, st, *old_ingate);
            stateMap.insert(std::pair<EncapFSA::State*,EncapFSA::State*>(old_ingate, &(*st)));
            new_ingate = &(*st);
          }
          else {
            new_ingate = in_gate_ref->second;
          }

          // output gates
          for (std::list<StateIterator>::iterator old_outgate_ref = old_outgates.begin();
               old_outgate_ref != old_outgates.end();
               old_outgate_ref++){
            std::map<EncapFSA::State*, EncapFSA::State*>::iterator out_gate_ref = stateMap.find(&(**old_outgate_ref));
            if(out_gate_ref == stateMap.end()) {
              // was not seen yet, handle it
              EncapFSA::StateIterator st = to->AddState((**old_outgate_ref).GetInfo());
              TRANSFER_STATE_PROPERTIES(to, st, **old_outgate_ref);
              stateMap.insert(std::pair<EncapFSA::State*,EncapFSA::State*>(&(**old_outgate_ref), &(*st)));
              new_outgates.insert(std::pair<State*, State*>(&(**old_outgate_ref), &(*st)));
            }
            else {
              new_outgates.insert(std::pair<State*, State*>(&(**old_outgate_ref), out_gate_ref->second));
            }
          }

          // notify the ET about the changes
          new_et->changeGateRefs(to, new_ingate, new_outgates);

          // save the new ET
          et_map.insert(std::pair<EncapFSA::ExtendedTransition*, EncapFSA::ExtendedTransition*>(et_ref, new_et));
          (*new_t).setIncludingET(new_et);
          new_et->changeEdgeRef(t, new_t);
        }
        else{
          (*new_t).setIncludingET(e->second);
          e->second->changeEdgeRef(t, new_t);
        }
      }
    }
}

EncapFSA* EncapFSA::BooleanAND(EncapFSA *fsa1, EncapFSA *fsa2)
{
	fsa1->BooleanNot(); 
	fsa2->BooleanNot();
	EncapFSA* fsa = EncapFSA::BooleanOR(fsa1,fsa2);
    EncapFSA* result = EncapFSA::NFAtoDFA(fsa);
	result->BooleanNot();
    result->setFinalStates();
	
	return result;
}

EncapFSA* EncapFSA::BooleanOR(EncapFSA *fsa1, EncapFSA *fsa2, bool setFinals)
{

  nbASSERT(fsa1!=NULL, "fsa1 cannot be NULL");
  nbASSERT(fsa2!=NULL, "fsa2 cannot be NULL");  
  
  EncapFSA *fsa = new EncapFSA(fsa1->m_Alphabet);
  fsa->MergeCode(fsa1,fsa2);
  EncapFSA::StateIterator init = fsa->AddState(NULL);
  fsa->SetInitialState(init);

  PRINT_DOT(fsa, "OR-pre-first-loop", "or_pre_first_loop");
  migrateTransitionsAndStates(fsa, fsa1, init);
  PRINT_DOT(fsa, "OR-pre-second-loop", "or_pre_2nd_loop");
    
  migrateTransitionsAndStates(fsa, fsa2, init);
  PRINT_DOT(fsa, "OR-post-second-loop", "or_post_2nd_loop");

  if(setFinals)
	  fsa->setFinalStates();

  PRINT_DOT(fsa, "OR-after-state-fix", "or_after_statefix");
  
  fsa = EncapFSA::NFAtoDFA(fsa);
  
  PRINT_DOT(fsa,"After determinization","after_or_determ");


  return fsa;
}

EncapFSA::StateIterator EncapFSA::GetStateIterator(EncapFSA::State *state)
{
	EncapFSA::StateIterator s = this->FirstState();
	while(s != this->LastState())
	{
		if((*s).GetID() == state->GetID())
			break;
		s++;
	}
	return s;
}

EncapFSA::TransIterator EncapFSA::GetTransIterator(Transition *transition)
{
	EncapFSA::TransIterator t = this->FirstTrans();
	while(t != this->LastTrans())
	{
		EncapFSA::StateIterator frIt = (*t).FromState();
		EncapFSA::StateIterator frIt2 = transition->FromState();
		if(frIt == frIt2)
		{
			EncapFSA::StateIterator toIt = (*t).ToState();
			EncapFSA::StateIterator toIt2 = transition->ToState();
			if(toIt == toIt2)
			{
				Alphabet_t alph = (*t).GetLabels();
				Alphabet_t alph2 = transition->GetLabels();
				if(alph == alph2)
					break;
			}
		}
		t++;
	}

	return t;
}

/*
* Calculates the epsilon closure of a set of states
*/
void E_Closure(DFAset* ds, EncapFSA *fsa)
{
	bool mod = false;
	std::map<uint32,EncapFSA::State*> states = ds->GetStates();

	for(EncapFSA::TransIterator t = fsa->FirstTrans(); t != fsa->LastTrans(); t++) //for each transition of the FSA
	{
        EncapFSA::State *fromState = &(*(*t).FromState());          
		if((states.find(fromState->GetID()) != states.end()) && (*t).IsEpsilon() && (*t).GetPredicate() == NULL && !(*t).IsVisited())
		{
            EncapFSA::State *toState = &(*(*t).ToState());
			ds->AddState(toState);
			mod = true;
			(*t).SetVisited();
		}
	}

	if(mod)
		E_Closure(ds,fsa);
}

void UnsetVisited(EncapFSA *fsa)
{
	EncapFSA::TransIterator iter = fsa->FirstTrans();
	while(iter != fsa->LastTrans())
	{
		(*iter).ResetVisited();
		iter++;
	}
	return;
}

void Move(DFAset* ds, EncapFSA *fsa, EncapLabel_t sym, bool complement, DFAset* newds)
{
	std::map<uint32,EncapFSA::State*> states = ds->GetStates();

	for(EncapFSA::TransIterator t = fsa->FirstTrans(); t != fsa->LastTrans(); t++)
	{
        EncapFSA::State *fromState = &(*(*t).FromState());
		if(states.find(fromState->GetID()) != states.end())
		{
			if(!complement)
			{
				EncapFSA::Alphabet_t a = (*t).GetInfo().first;
				if((a.contains(sym) && !(*t).IsComplementSet()) || (!a.contains(sym) && (*t).IsComplementSet()))
				{
                	EncapFSA::State *toState = &( *(*t).ToState() );
                    newds->AddState(toState);
				}
			}
			else
			{
				if((*t).IsComplementSet())
				{
                	EncapFSA::State *toState = &( *(*t).ToState() );
                    newds->AddState(toState);
				}
			}
		}
	}
}

/* Return true if an ExtendedTransition was encountered.
 * et_out_gates (if this method returned true) maps the set of gates 
 * that the ExtendedTransition reaches after determinization to a token value
 * (of type State*) that can be reused to set the gate pointers to a sane value,
 * after that those sets have been successfully recreated in the determinized DFA,
 * using ExtendedTransition::changeGateRefs()
 *
 * ds is the considered set of states, i.e. the considered state of the DFA
 *
 * newds takes now the semantic "set of states reachable
 * without transition with predicates". That is, if this method returns true,
 * depending on the value of the predicates, the following sets are reached:
 * [pseudocode]
 * foreach (stateset in et_out_gates.first) Union(newds, stateset)
 *
 * at_least_one_complementset_trans_was_taken (*sigh*) says if newds was reached by
 * at least one transition in the form "* - {symbol}" (useful to invalidate
 * the state information in the state equivalent, after the determinization, to newds)
 */
bool MoveExt(DFAset* ds, EncapFSA *fsa, EncapLabel_t sym, DFAset* newds,
             EncapFSA::ExtendedTransition **ptr_to_et, 
             std::map<DFAset*, EncapFSA::State*> *et_out_gates,
             bool *at_least_one_complementset_trans_was_taken)
{
  //newset, fsa, current symbol, extTra, outGates, bool

  std::map<uint32,EncapFSA::State*> states = ds->GetStates();
  EncapFSA::ExtendedTransition *local_et = NULL;
  std::set<EncapFSA::ExtendedTransition *> known_ETs; /* Stores pointers to all the ETs encountered.
                                                       * Used to avoid handling twice or more the same ET
                                                       */
  for(EncapFSA::TransIterator t = fsa->FirstTrans(); t != fsa->LastTrans(); t++)
  {    
      EncapFSA::State *fromState = &(*(*t).FromState());
      if(states.find(fromState->GetID()) != states.end())
      {	  //if the state belongs to the states within ds...      
      	  EncapFSA::Alphabet_t a = (*t).GetInfo().first;
          if((a.contains(sym) && !(*t).IsComplementSet()) || (!a.contains(sym) && (*t).IsComplementSet()))
          {	  //if the current symbol is placed on the current transition
              if ((*t).IsComplementSet())
                *at_least_one_complementset_trans_was_taken = true;
              EncapFSA::State *toState = &(*(*t).ToState());
              
              if ( (*t).getIncludingET() ) 
              {
                // this transition is handled by an extended transition
                EncapFSA::ExtendedTransition *et = (*t).getIncludingET();
                if(known_ETs.count(et)>0)
                	continue; //done already
                else
                {
                  known_ETs.insert(et); //handling this ET now
                }

                if (local_et)
                  // add to the already existing ET
                  (*local_et) += (*et);
                else
                  // ET must be created and initialized
                  local_et = new EncapFSA::ExtendedTransition(et);
                 
              } 
              else 
              {
              	//the current transition is not part of an extended transition, and
              	//we add its destination set to the new set 
                newds->AddState(toState); // see newds semantic above
                if (local_et)
                  // add to the already existing ET
                  local_et->unionWithoutPredicate(toState, (*t).GetEdge());
              }
           }
       }
   }

   if(local_et==NULL)
   	//we didn't find any extended transition
   	return false;
  			
  local_et->wipeEdgeRefs(); 
  
  // why this conversion? ask to who designed those headers
  std::map<std::set<EncapFSA::State*>, EncapFSA::State*> det_tokens;
  local_et->determinize(&det_tokens);  
  
  for( std::map<std::set<EncapFSA::State*>, EncapFSA::State*>::iterator ii = det_tokens.begin();
       ii != det_tokens.end();
       ++ii) 
    et_out_gates->insert(make_pair<DFAset*, EncapFSA::State*>(new DFAset(ii->first),ii->second));
    

  *ptr_to_et = local_et;

  PRINT_DOT(fsa, "MoveExt end", "moveext_end");
  return true;
}

/*
 * This function populates the DFAset by adding into it all the
 * _symbols_ that appear on non-epsilon, non-complemented transitions
 * that exits from _states_ currently in the DFAset.
 */
void AddSymToDFAset(DFAset* dset, EncapFSA *fsa)
{
	std::map<uint32,EncapFSA::State*> states = dset->GetStates();
	
	for(EncapFSA::TransIterator t = fsa->FirstTrans(); t != fsa->LastTrans(); t++)
	{
      	EncapFSA::State *fromState = &(*((*t).FromState()));
      	if(!(*t).IsEpsilon() && !(*t).IsComplementSet() && (states.find(fromState->GetID())) != states.end())
		{
			EncapFSA::Alphabet_t a = (*t).GetInfo().first;
			if(a.size()>0)
			{
				EncapFSA::Alphabet_t::iterator labelIter = a.begin();
				while(labelIter != a.end())
				{
					EncapLabel_t label = *labelIter;
					dset->AddSymbol(label);
					labelIter++;
				}
			}
		}
	}
}

/*
* Compares the sets stored into dlist with dset.
* If dset is equal to an element of the list, the method 
* returns the id of that element
*/
int ExistsDFAset(std::list<DFAset> dlist, DFAset* dset)
{
	std::list<DFAset>::iterator iter = dlist.begin();
	while(iter != dlist.end())
	{
		if((*iter).CompareTo(*dset)==0)
			return (*iter).GetId();
		iter++;
	}
	return -1;
}

/*
 * Handle a single movement from a state to another set.
 * Returns <0 if the target DFAset was unseen, >0 otherwise.
 * The absolute value of the returned value is the ID of dfaset_to in dfa_list
 * (that might be different from the ID in dfaset_to, as it might be duplicated).
 *
 * The last parameter, when false, requests that no state information are added
 * to the newly generated state (if applicable, that is if effectively the target
 * stateset was still unseen). This is useful when no consistent state information
 * could be added to the newly created state.
 * Note that, even if the last parameter is true, this method might
 * not set the stateInfo of the target state if it is an accepting final
 * state: we will fix that later (fixStateProtocols).
 */
int HandleSingleMovement(EncapFSA *fsa_from, EncapFSA *fsa_to, EncapFSA::State *state_from, DFAset *dfaset_to,
                         EncapFSA::Alphabet_t::iterator symbol_iter, std::list<DFAset> *dfa_list,
                         std::map<int, EncapFSA::State*> *stateMap, EncapFSA::ExtendedTransition *et,
                         bool set_internal_state_info = true)
{
  E_Closure(dfaset_to, fsa_from); // FIXME implementare le ottimizzazioni?
  UnsetVisited(fsa_from);
  std::map<uint32,EncapFSA::State*> newdsStates = dfaset_to->GetStates();
  if(newdsStates.size() < 1)
    return false;

  // add the transition
  int id;
  if((id = ExistsDFAset(*dfa_list,dfaset_to)) > 0)
  {
  	  //dfaset_to already exists
      EncapFSA::State *st = (stateMap->find(id))->second;
      EncapFSA::StateIterator st_1 = fsa_to->GetStateIterator(state_from);
      EncapFSA::StateIterator st_2 = fsa_to->GetStateIterator(st);
      if(et)
      {
        EncapFSA::TransIterator tr_iter = fsa_to->AddTrans(st_1, st_2, *symbol_iter, false, PTraits<PFLTermExpression*>::One());  // FIXME last param (only a printing issue)
        (*tr_iter).setIncludingET(et);
        et->addEdgeRef(tr_iter);
      }
      else
        fsa_to->AddTrans(st_1, st_2, *symbol_iter, false, NULL);
  }
  else
  {
  	  //The current set wasn't found into the list of the already created sets
      AddSymToDFAset(dfaset_to, fsa_from);
      if(set_internal_state_info && !dfaset_to->isAcceptingFinal())
        dfaset_to->AutoSetInfo();
      SymbolProto* info = dfaset_to->GetInfo();
      EncapFSA::StateIterator st = fsa_to->AddState(info);
      if(dfaset_to->isAccepting())
        fsa_to->setAccepting(st);
      else
        fsa_to->resetAccepting(st);
      if(dfaset_to->isAction())
      {
        fsa_to->setAction(st);
        fsa_to->SetExtraInfoToState(&(*st),dfaset_to->GetExtraInfo());
        fsa_to->SetExtraInfoPlusToState(&(*st),dfaset_to->GetExtraInfoPlus());
         fsa_to->SetOtherInfoToState(&(*st),dfaset_to->GetOtherInfo());
      }
      else
        fsa_to->resetAction(st);
      dfa_list->push_front(*dfaset_to);
      stateMap->insert( std::pair<int, EncapFSA::State*>(dfaset_to->GetId(), &(*st)) );
      EncapFSA::StateIterator siter1 = fsa_to->GetStateIterator(state_from);
      if(et)
      {
        EncapFSA::TransIterator tr_iter = fsa_to->AddTrans(siter1, st, *symbol_iter, false, PTraits<PFLTermExpression*>::One());  // FIXME last param (only a printing issue)
        (*tr_iter).setIncludingET(et);
        et->addEdgeRef(tr_iter);
      }
      else
        fsa_to->AddTrans(siter1, st, *symbol_iter, false, NULL);

      id = 0-dfaset_to->GetId();
   }
  return id;
}

//setInfo is false when we don't want that information are assigned to the states. An example is when we 
//are managing an automaton representing the Header Chain
EncapFSA* EncapFSA::NFAtoDFA(EncapFSA *orig, bool setInfo)
{ 
	std::list<DFAset> dlist;
	std::map<int, EncapFSA::State*> stateMap;

	//stack of states of the dfa
	std::list<DFAset*> dfaStack;

	//fase preliminare
	EncapFSA *fsa = new EncapFSA(orig->m_Alphabet); //the two automata have the same alphabet
	fsa->MergeCode(orig); //merge the code associated with the symbols of the two automata we are joining
	EncapFSA::State *init = &(*(orig->GetInitialState()));
	DFAset* dset = new DFAset();
	dset->AddState(init);
	E_Closure(dset, orig);
	UnsetVisited(orig); 
	AddSymToDFAset(dset, orig);
	if(setInfo)
		dset->AutoSetInfo();
	//dset->parseAcceptingm_m_Status();
	dfaStack.push_front(dset);
		
	EncapFSA::StateIterator stateIt = fsa->AddState(dset->GetInfo()); //GetInfo() returns a SymbolProto. In this case returns NULL
	if(dset->isAccepting())
        fsa->setAccepting(stateIt);
    if(dset->isAction())
    {
    	fsa->setAction(stateIt);
        fsa->SetExtraInfoToState(&(*stateIt),dset->GetExtraInfo());
        fsa->SetExtraInfoPlusToState(&(*stateIt),dset->GetExtraInfoPlus());
        fsa->SetOtherInfoToState(&(*stateIt),dset->GetOtherInfo());
    }
    fsa->SetInitialState(stateIt);

	dlist.push_front(*dset);
	std::pair<int, EncapFSA::State*> p = make_pair<int, EncapFSA::State*>(dset->GetId(), &(*stateIt));
	stateMap.insert(p);

	while(dfaStack.size()>0)
	{
        PRINT_DOT(fsa, "nfa2dfa - before considering a set in the stack", "nfa2dfa_before_considering_set_in_stack");
		std::list<DFAset*>::iterator it = dfaStack.begin();
		DFAset *ds = *it;
		dfaStack.pop_front();
        EncapFSA::State *fromState = (stateMap.find(ds->GetId()))->second;
        EncapFSA::Alphabet_t symSet = ds->GetSymbols();
        EncapFSA::Alphabet_t::iterator i = symSet.begin();
		
		//iterates over the symbols of the current DFAset, 
		//i.e. over the symbols exiting from the states that form the DFAset
		while(i != symSet.end()) 
		{			
			DFAset* newds = new DFAset();
			ExtendedTransition *et = NULL;
			std::map<DFAset*, State*> et_out_gates;
			bool complset_seen;
			/*
			*	MoveExt, in addidion to manage the extended transitions exiting from the current set and labeled with the 
			*	current symbols, put into newds the states reachable with this symbol from this state
			*/
			bool et_encountered = MoveExt(ds, orig, *i, newds, &et, &et_out_gates, &complset_seen); 

			if (et_encountered)
			{
				/* maps each of the token values received by the MoveExt
				 * to the actual states in the new fsa
				 */                     
				std::map<State*, State*> updated_gates;

				for(std::map<DFAset*, State*>::iterator gate_iter =
					  et_out_gates.begin();
					gate_iter != et_out_gates.end();
					gate_iter++)
				{
				  DFAset *unionset=new DFAset(gate_iter->first, newds); // read MoveExt() comment
				  int id;
				  if ( (id=HandleSingleMovement(orig, fsa, fromState, unionset, i, &dlist, &stateMap, et,(!setInfo)? !complset_seen : false)) <0 )
					dfaStack.push_front(unionset);

				  std::map<int, EncapFSA::State*>::iterator newstate = stateMap.find(abs(id));
				  nbASSERT(newstate != stateMap.end(),"target new state should have been added into the state map by HandleSingleMovement()");
				  updated_gates.insert(std::pair<State*, State*>(gate_iter->second, newstate->second));
				}
				PRINT_ET_DOT(et,"after handling single movement", "after_move_handling");
				et->changeGateRefs(fsa, fromState, updated_gates);
			}
			/*
			* oring->NFA
			* fsa->DFA
			* fromState->considered state of the DFA
			* newds->new state of the DFA (contains a set of states of the NFA)
			* i->we are considering the i-th symbol exiting from the current state of the DFA
			* dlist->list of the created states in the DFA?
			* stateMap->states of the DFA
			*/
			else if(HandleSingleMovement(orig, fsa, fromState, newds, i, &dlist, &stateMap, NULL,setInfo) <0)
				dfaStack.push_front(newds);
			i++;
		} // end: while(i != symSet.end())

    	PRINT_DOT(fsa, "nfa2dfa - after handling all the symbols exiting from a set", "nfa2dfa_after_handling_all_symbols_from_set");

		//handle the complemented transition
		EncapLabel_t label = std::make_pair<SymbolProto*, SymbolProto *>(NULL, NULL);
		DFAset* newds = new DFAset();
		Move(ds, orig, label, true, newds);
		E_Closure(newds, orig);
		UnsetVisited(orig);
		std::map<uint32,EncapFSA::State*> newdsStates = newds->GetStates();
		if(newdsStates.size() < 1)
			continue;
			
		int id;
		if((id = ExistsDFAset(dlist,newds)) > 0)
		{
			EncapFSA::State *st = (stateMap.find(id))->second;
			EncapFSA::StateIterator iterFrom = fsa->GetStateIterator(fromState);
			EncapFSA::StateIterator iterTo = fsa->GetStateIterator(st);
			EncapFSA::Alphabet_t symSet = ds->GetSymbols();
			fsa->AddTrans(iterFrom, iterTo, symSet, true, NULL);
		}
		else
		{
			AddSymToDFAset(newds, orig);
			newds->AutoSetInfo();
			SymbolProto* info = newds->GetInfo();
			EncapFSA::StateIterator st = fsa->AddState(info);
			if(newds->isAccepting())
	        	fsa->setAccepting(st);
	        else
	        	fsa->resetAccepting(st);
	        if(newds->isAction())
	        {
				fsa->setAction(st);
        		fsa->SetExtraInfoToState(&(*st),newds->GetExtraInfo());
        		fsa->SetExtraInfoPlusToState(&(*st),newds->GetExtraInfoPlus());
        		fsa->SetOtherInfoToState(&(*st),newds->GetOtherInfo());
			}
			else
				fsa->resetAction(st);
			dlist.push_front(*newds);
			std::pair<int, EncapFSA::State*> p = make_pair<int, EncapFSA::State*>(newds->GetId(), &(*st));
			stateMap.insert(p);
			EncapFSA::Alphabet_t symSet = ds->GetSymbols();
			EncapFSA::StateIterator iterFrom = fsa->GetStateIterator(fromState);
			fsa->AddTrans(iterFrom, st, symSet, true, NULL);
			dfaStack.push_front(newds);
		}
	    PRINT_DOT(fsa, "nfa2dfa - after handling an element of the dfa stack", "nfa2dfa_after_handling_element_of_set");
	}
	
	fsa->setFinalStates();
	
	PRINT_DOT(fsa, "nfa2dfa end before setfinalstates", "nfa2dfa_end_almost");
	
	if(setInfo)
        fsa->fixStateProtocols();
    
    PRINT_DOT(fsa, "nfa2dfa end", "nfa2dfa_end");
        
	return fsa;
}


//Complements the automaton. The action status of a state does not change during the compelementation
void EncapFSA::BooleanNot()
{
	PRINT_DOT(this, "fsa before NOT", "fsa_before_not");
	EncapFSA::StateIterator s = this->FirstState();
	while(s != this->LastState())
	{
		if((*s).isAccepting())
        	this->resetAccepting(s);
        else
        	this->setAccepting(s);
        s++;
	}
	
	PRINT_DOT(this, "fsa after NOT", "fsa_after_not");
}


/* This method is the last shore to try and fix the states in the FSA
 * that we failed to associate with a protocol. This happens often
 * when the determinization is rather complex. This operation will
 * not be performed on final, non-accepting states.
 *
 * Needed by the code generation routines.
 */
void EncapFSA::fixStateProtocols(){
  EncapFSA::StateIterator s = this->FirstState();
  for (; s != this->LastState(); ++s)
  {
    if ( ((*s).isFinal() && !(*s).isAccepting()) || (*s).GetInfo() != NULL)
      continue;

    SymbolProto *proto_from_tr = NULL;
    for (EncapFSA::TransIterator t = this->FirstTrans();
         t != this->LastTrans() ;
         ++t){
      if ( (*(*t).ToState()) != *s)
        continue;

      // grab all labels from incoming transitions
      Alphabet_t labels = (*t).GetInfo().first;

      // try and extract the SymbolProto associated with the current state
      for (Alphabet_t::iterator l = labels.begin();
           l != labels.end();
           ++l)
      {
        if(proto_from_tr) {
          if (proto_from_tr != (*l).second) {
            // nothing can be done: incoming transitions with different destination protocols
            proto_from_tr = NULL;
            break;
          }
        }
        else
          proto_from_tr = (*l).second;
      }

      if (proto_from_tr == NULL && labels.size() > 0)
        // the current transition encountered some inconsistencies, this state cannot be uniquely identified
        break;
    }

    if(proto_from_tr)
      (*s).SetInfo(proto_from_tr);
  }
}

/*
 * Sets the 'final' flag on states that have no transitions leading to other states.
 * Compacts final states: accepting with accepting, action with action, normal with normal, accepting action with accepting action
 */
void EncapFSA::setFinalStates()
{
  set<State*> final_accepting_states;
  set<State*> final_normal_states;//final_notaccepting_states;
  set<State*> final_action_states;
  set<State*> final_action_accepting_states;

  EncapFSA::StateIterator s = this->FirstState();
  while(s != this->LastState())
  {
    EncapFSA::TransIterator t = this->FirstTrans();
    bool loop = false;
    int transCount = 0;
    while(t != this->LastTrans())
    {
      if((*(*t).FromState()) == *s)
      {
        if((*(*t).ToState()) == *s)
          loop = true;
        transCount++;
      }
      t++;
    }
    
    if(loop && transCount == 1)
    {
      this->setFinal(s);
      if((*s).isAccepting() && !(*s).isAction()) 
      {
        final_accepting_states.insert(&(*s));
        this->setAccepting(s);
      }
      else if((*s).isAction() && !(*s).isAccepting()) 
      {
      	final_action_states.insert(&(*s));
        this->setAction(s);
      }
      else if((*s).isAction() && (*s).isAccepting()) 
      {
      	final_action_accepting_states.insert(&(*s));
        this->setAction(s);
        this->setAccepting(s);
      }
      else
        final_normal_states.insert(&(*s));
    }
    s++;
  }

  map<State*, State*> trasformation_map;

 if(final_accepting_states.size() > 1) 
 {
    State *tmp_s = NULL;
    for(set<State*>::iterator i = final_accepting_states.begin();
        i != final_accepting_states.end();
        ++i) {
      if (tmp_s)
      {
      	trasformation_map[*i] = tmp_s;
      }
      else
        tmp_s = *i;
    }
  }

  if(final_normal_states.size() > 1) 
  {
    State *tmp_s = NULL;
    for(set<State*>::iterator i = final_normal_states.begin();
        i != final_normal_states.end();
        ++i) {
      if (tmp_s)
        trasformation_map[*i] = tmp_s;
      else
        tmp_s = *i;
    }
  }
  
 if(final_action_states.size() > 1) 
 {
    State *tmp_s = NULL;
    for(set<State*>::iterator i = final_action_states.begin();
        i != final_action_states.end();
        ++i) {
      if (tmp_s)
        trasformation_map[*i] = tmp_s;
      else
        tmp_s = *i;
    }
  }
  
 if(final_action_accepting_states.size() > 1) 
 {
    State *tmp_s = NULL;
    for(set<State*>::iterator i = final_action_accepting_states.begin();
        i != final_action_accepting_states.end();
        ++i) {
      if (tmp_s)
        trasformation_map[*i] = tmp_s;
      else
        tmp_s = *i;
    }
  }

  PRINT_DOT(this, "almost at the end of statefix", "statefix_almost_end");

  if( trasformation_map.size() > 0 )
    compactStates(trasformation_map, true);
}

/*
*	When the NetPFL statement requires to extract a single field from the first instance 
*	of a protocol, the automaton has the action state with all its exiting transitions 
*	toward a final accepting state. This automaton is formally right, but generates more
*	code than the one that is really needed. Then this method, if an accepting and action
*	state has all the transitions toward a final accepting state, removes the transitions,
*	creates a self loop over the state, and set the state itself as final.
*/
void EncapFSA::fixActionStates()
{
	std::list<State*> actions = GetActionStates();
	for(list<State*>::iterator action = actions.begin(); action != actions.end(); action++)
	{
		State *current = *action;
		if(!current->isAccepting())
			continue; //the current state is not accepting, hence no change is required
		
		//since we are considering an accepting state, we are sure that all the reachable states 
		//are accepting too 
		
		StateIterator current_it = GetStateIterator(current);
		list<Transition*> transitions = getTransExitingFrom(&(*current_it));	
		unsigned int counter = 0;
		list<Transition*>::iterator tr;
		for(tr = transitions.begin(); tr != transitions.end(); tr++)
		{
			EncapStIterator_t to_it = (*tr)->ToState();
			State *to = &(*to_it);
			if(to->isAction() || !to->isFinal())
				break;
				
			//to is a final accepting state
			counter++;
		}
		if(tr != transitions.end())
			continue;
		
		//to is a final accepting state
		
		if(counter == transitions.size())
		{	
			//Change!			
			for(list<Transition*>::iterator tr = transitions.begin(); tr != transitions.end();)
			{
				EncapStIterator_t to_it = (*tr)->ToState();				
				list<Transition*>::iterator aux = tr;
				tr++;
				(*aux)->RemoveEdge();
				delete(*aux);	
			}
			Alphabet_t empty;
			AddTrans(current_it, current_it, empty, true, NULL);	
			setFinal(current_it);
		}		
	}
	
	//iterate over the states of the automaton and remove those without incoming transitions
	StateIterator s = FirstState();
  	for (; s != LastState();)
  	{
  		EncapFSA::StateIterator aux = s; //useful in case of the current state must be removed
  		s++;
		if(!(*aux).IsInitial())
		{
			list<EncapFSA::Transition*> transition = getTransEnteringTo(&(*aux));
			if(transition.size()==0 || (transition.size()==1 && ThereIsSelfLoop(transition)))
			{
				//this state must be removed (and also its self loop)
				
				list<EncapFSA::Transition*> exit = getTransExitingFrom(&(*aux));
				nbASSERT(exit.size()==1,"This situation should not happen. If you want it, you should change this piece of code");
				
				Transition *tr = *(exit.begin());
				nbASSERT(tr->ToState() == tr->FromState(),"This situation should not happen. If you want it, you should change this piece of code");
				tr->RemoveEdge();
				delete(tr);	
									
				removeState(&(*aux));
			}
		}
	}
}

/***************************** Print automaton *****************************/

/*
*	Print the fsa in a .dot file
*/
void EncapFSA::prettyDotPrint(EncapFSA *fsa, char *description, char *dotfilename, char *src_file, int src_line) {
  static int dbg_incremental_id = 0;

  ++dbg_incremental_id;

  if (src_file != NULL && src_line != -1){
    cerr << setfill('0');
    cerr << description << ": " << setw(3) << dbg_incremental_id << " (@ "
	 << src_file << " : " << src_line << ")" << endl;
  }

  std::stringstream myconv;
  myconv << setfill('0');
  myconv << setw(3) << dbg_incremental_id;
  std::string fname = string("dbg_") + myconv.str() +
    string("_") + string(dotfilename) + string(".dot");
  ofstream outfile(fname.c_str());

  sftDotWriter<EncapFSA> fsaWriter(outfile);
  fsaWriter.DumpFSA(fsa);
}

/*
*	Print the legend in a .dot file
*	Ivano Cerrato 31/07/2012
*/
void EncapFSA::printDotLegend(char *src_file, int src_line)
{
	cerr << setfill('0');
    cerr << "Legend of the shapes used as states" << ": " << setw(3) << "0" << " (@ "
	 << src_file << " : " << src_line << ")" << endl;

	std::string fname = string("dbg_000_legend.dot");
  	ofstream outfile(fname.c_str());
  	sftDotWriter<EncapFSA> fsaWriter(outfile);
  	fsaWriter.DumpLegend();
}

/***************************** Utilities *****************************/

//The following methods has been written by Ivano Cerrato and Marco Leogrande - 2011/2012

/* 
*	Returns an iterator of a state
*/
EncapFSA::StateIterator EncapFSA::GetStateIterator(EncapFSA::State &state)
{
	EncapFSA::StateIterator s = this->FirstState();
	while(s != this->LastState())
	{
		if((*s).GetID() == state.GetID())
			break;
		s++;
	}
	return s;
}

/*
*	Remove the final status from each state of the FSA
*/
void EncapFSA::ResetFinals()
{
	StateList_t states = this->GetFinalStates();
	for(StateList_t::iterator it = states.begin(); it != states.end(); it++)
	{	
		EncapStIterator_t stateIt = GetStateIterator(*it);
		this->resetFinal(stateIt);
	}
}

/*
*	Remove the transition labeled with "*-{}" which are not self loops, and starting
*	from a state having only this outgloing transition.
*	Moreover, a self loop is added on the state, which is also set as final
*/
void EncapFSA::FixTransitions()
{
	Alphabet_t empty;
	EncapFSA::StateIterator s = this->FirstState();
	while(s != this->LastState())
	{
		list<Transition*> outgoing =  this->getTransExitingFrom(&(*s));	
		Transition *tr = *(outgoing.begin());
		if(outgoing.size()==1 && tr->FromState() != tr->ToState() && tr->IsComplementSet())
		{
			EncapStIterator_t aux = tr->ToState();
			Alphabet_t alph = tr->GetLabels();
			if(alph.size() == 0)
			{
				//we must perform the transformation
				tr->RemoveEdge();
				delete(tr);	
				this->AddTrans(s, s, empty, true, NULL);	
				setFinal(s);
			}
			
			//if the destination state of the removed transitin remains without any incoming edge
			//except the eventual self loop, it must be removed.
			list<Transition*> incoming = this->getTransEnteringTo(&(*aux));
			if(incoming.size()==1 && ThereIsSelfLoop(incoming))
				this->removeState(&(*aux));
		}
		
		s++;		
	}
}

EncapFSA::StateIterator EncapFSABuilder::AddState(EncapFSA &fsa, EncapGraph::GraphNode &origNode, bool forceInsertion)
{
	NodeMap_t::iterator i = m_NodeMap.find(&origNode);
	if (i == m_NodeMap.end() || forceInsertion)
	{
          EncapStIterator_t newState = fsa.AddState(origNode.NodeInfo);
          if ( i == m_NodeMap.end() ) 
          { // add the state to the caching map only if it is the first one associated with this origNode
            pair<EncapGraph::GraphNode*, EncapStIterator_t> p = make_pair<EncapGraph::GraphNode*, EncapStIterator_t>(&origNode, newState);
            m_NodeMap.insert(p);
          }
          return newState;
	}

	return i->second;
}

/***************************** SYMBOL CODE *****************************/

//The following methods have been written by Ivano Cerrato - 07/05/2012 - 07/06/2012

void EncapFSA::AddCode1(EncapLabel_t label,string code, bool check, SymbolTemp *variable)
{	
	AddCode(label.second,code,variable,check,FIRST);	
}

void EncapFSA::AddCode2(EncapLabel_t label,string code, bool check, SymbolTemp *variable)
{
	AddCode(label.second,code,variable,check,SECOND);	
}

void EncapFSA::AddCode(SymbolProto *protocol,string code, SymbolTemp *variable, bool check, instructionNumber number)
{
	Code *c = new Code(code, variable, check);
	
	list<Code*> lCode;
	Instruction_t::iterator ofProto;
	
	switch(number)
	{
		case FIRST:
			ofProto = m_Instruction1.find(protocol);
			if(ofProto != m_Instruction1.end())
			{
				//further code for protocol
				lCode = (*ofProto).second;
				if(AlreadyInserted(lCode,code,check))
					return; //this code has been already inserted
			}
			break;
		case SECOND:
			ofProto = m_Instruction2.find(protocol);
			if(ofProto != m_Instruction2.end())
			{
				//further code for protocol
				lCode = (*ofProto).second;
				if(AlreadyInserted(lCode,code,check))
					return; //this code has been already inserted
			}
			break;
	}	
				
	lCode.push_back(c);
	
	switch(number)
	{
		case FIRST:
			m_Instruction1[protocol]=lCode;
			break;
		case SECOND:
			m_Instruction2[protocol]=lCode; 
	}
}

/*
*	This method returns true if the code already exists in the list
*	In case of check is true, set it to the code
*/
bool EncapFSA::AlreadyInserted(list<Code*> lCode, string code, bool check)
{
	list<Code*>::iterator c = lCode.begin();
	for(; c != lCode.end(); c++)
	{
		if((*c)->GetCode() == code)
		{
			if(check)
				(*c)->SetCheck();
			return true;
		}
	}
	
	return false;
}

string EncapFSA::GetCodeToCheck1(EncapLabel_t label)
{
	return GetCodeToCheck(label.second,FIRST);
}

string EncapFSA::GetCodeToCheck2(EncapLabel_t label)
{
	return GetCodeToCheck(label.second,SECOND);		
}

/*
*	Returns the code for which "check == true". It assumes that, for each protocols, there is only one code satisfying this condition.
*	To extend the code, be careful about our assumption.
*/
string EncapFSA::GetCodeToCheck(SymbolProto *protocol, instructionNumber number)
{
	Instruction_t::iterator ofProto;
	
	switch(number)
	{
		case FIRST:
			ofProto = m_Instruction1.find(protocol);
			nbASSERT(ofProto != m_Instruction1.end(),"There is a BUG!");
			break;
		case SECOND:
			ofProto = m_Instruction2.find(protocol);
			nbASSERT(ofProto != m_Instruction2.end(),"There is a BUG!");
			break;
	}
	
	list<Code*> lCode = (*ofProto).second;
	list<Code*>::iterator code = lCode.begin();
	for(; code != lCode.end(); code++)
	{		
		if((*code)->Check())
			break;
	}
	
	nbASSERT(code != lCode.end(),"There is a bug!");
	return (*code)->GetCode();
}

/*
*	This method merges two codes in a third one, supposing that the third code is empty before the invocation of the method.
*	If you want extend this code, be careful about our assumption.
*/
void EncapFSA::MergeCode(EncapFSA *firstFSA, EncapFSA *secondFSA)
{
	{
		//Copy m_Instruction1 of firstFSA
		Instruction_t::iterator it = firstFSA->m_Instruction1.begin();
		for(; it != firstFSA->m_Instruction1.end(); it++)
		{	
			SymbolProto *proto = (*it).first;
			list<Code*> code = (*it).second;
			m_Instruction1[proto] = code;
		}
	}

	{
		//Copy m_Instruction1 of secondFSA
		Instruction_t::iterator it = secondFSA->m_Instruction1.begin();
		for(; it != secondFSA->m_Instruction1.end(); it++)
		{	
			//new parameters
			SymbolProto *proto = (*it).first;
			list<Code*> secondFSACode = (*it).second;
		
			//get the code already defined for the third automaton
			Instruction_t::iterator ofProto = m_Instruction1.find(proto);
			list<Code*> currentListCode;
			if(ofProto != m_Instruction1.end())
				currentListCode = (*ofProto).second;
			
			for(list<Code*>::iterator c = secondFSACode.begin(); c != secondFSACode.end(); c++)
			{
				if(AlreadyInserted(currentListCode,(*c)->GetCode(),(*c)->Check()))
					continue;
				currentListCode.push_back(*c);
			}
			m_Instruction1[proto] = currentListCode;
		}
	}

	{
		//Copy m_Instruction2 of firstFSA
		Instruction_t::iterator it = firstFSA->m_Instruction2.begin();
		for(; it != firstFSA->m_Instruction2.end(); it++)
		{	
			SymbolProto *proto = (*it).first;
			list<Code*> code = (*it).second;
			nbASSERT(code.size() == 1, "This situation has not been trated");
			m_Instruction2[proto] = code;
		}
	}
	
	{
		//Copy m_Instruction2 of secondFSA
		Instruction_t::iterator it = secondFSA->m_Instruction2.begin();
		for(; it != secondFSA->m_Instruction2.end(); it++)
		{	
			//new parameters
			SymbolProto *proto = (*it).first;
			list<Code*> secondFSACode = (*it).second;
		
			//get the code already defined for the third automaton
			Instruction_t::iterator ofProto = m_Instruction2.find(proto);
			list<Code*> currentListCode;
			if(ofProto != m_Instruction2.end())
				currentListCode = (*ofProto).second;
			
			for(list<Code*>::iterator c = secondFSACode.begin(); c != secondFSACode.end(); c++)
			{
				if(AlreadyInserted(currentListCode,(*c)->GetCode(),(*c)->Check()))
					continue;
				currentListCode.push_back(*c);
			}
			m_Instruction2[proto] = currentListCode;
		}
	}
}

/*
*	This method copies a code into another, supposing that the second is empty before the invocation of the method.
*	If you want extend this code, be careful about our assumption.
*/
void EncapFSA::MergeCode(EncapFSA *originFSA)
{
	{
		//Copy m_Instruction1
		Instruction_t::iterator it = originFSA->m_Instruction1.begin();
		for(; it != originFSA->m_Instruction1.end(); it++)
		{	
			SymbolProto *proto = (*it).first;
			list<Code*> code = (*it).second;
			nbASSERT(code.size() == 1, "This situation has not been trated");
			m_Instruction1[proto] = code;
		}
	}

	{
		//Copy m_Instruction2
		Instruction_t::iterator it = originFSA->m_Instruction2.begin();
		for(; it != originFSA->m_Instruction2.end(); it++)
		{	
			//new parameters
			SymbolProto *proto = (*it).first;
			list<Code*> originFSACode = (*it).second;
		
			//get the code already defined for the third automaton
			Instruction_t::iterator ofProto = m_Instruction2.find(proto);
			list<Code*> currentListCode;
			if(ofProto != m_Instruction2.end())
				currentListCode = (*ofProto).second;
			
			for(list<Code*>::iterator c = originFSACode.begin(); c != originFSACode.end(); c++)
			{
				if(AlreadyInserted(currentListCode,(*c)->GetCode(),(*c)->Check()))
					
					continue;
				currentListCode.push_back(*c);
			}
			m_Instruction2[proto] = currentListCode;
		}
	}
}

bool EncapFSA::HasCode(EncapLabel_t l)
{
	SymbolProto *p = l.second;
	
	int counter = m_Instruction1.count(p);
	if(counter!=0)
		return true;
	
	counter = m_Instruction2.count(p);
	if(counter!=0)
		return true;

	return false;
}

list<SymbolTemp*> EncapFSA::GetVariables(EncapLabel_t symbol)
{
	list<SymbolTemp*> variables;
	
	{	
		Instruction_t::iterator ofProto = m_Instruction1.find(symbol.second);
		if(ofProto!=m_Instruction1.end())
		{
			list<Code*> codeList = (*ofProto).second;
	
			for(list<Code*>::iterator c = codeList.begin(); c != codeList.end(); c++)
				variables.push_back((*c)->GetVariable());	
		}
	}

	{	
		Instruction_t::iterator ofProto = m_Instruction2.find(symbol.second);
		if(ofProto !=m_Instruction2.end())
		{
			list<Code*> codeList = (*ofProto).second;
	
			for(list<Code*>::iterator c = codeList.begin(); c != codeList.end(); c++)
				variables.push_back((*c)->GetVariable());
		}	
	}
	
	return variables;
}

SymbolTemp* EncapFSA::GetVariableToCheck1(EncapLabel_t symbol)
{
	return GetVariableToCheck(symbol.second,FIRST);
}

SymbolTemp* EncapFSA::GetVariableToCheck2(EncapLabel_t symbol)
{
	return GetVariableToCheck(symbol.second,SECOND);
}


/*
*	Returns the variable for which "check == true". It assumes that, for each protocols, there is only one variable satisfying this condition.
*	To extend the code, be careful about our assumption.
*/
SymbolTemp *EncapFSA::GetVariableToCheck(SymbolProto *protocol, instructionNumber number)
{
	Instruction_t::iterator ofProto;
	
	switch(number)
	{
		case FIRST:
			ofProto = m_Instruction1.find(protocol);
			nbASSERT(ofProto != m_Instruction1.end(),"There is a BUG!");
			break;
		case SECOND:
			ofProto = m_Instruction2.find(protocol);
			nbASSERT(ofProto != m_Instruction2.end(),"There is a BUG!");
			break;
	}
	
	list<Code*> lCode = (*ofProto).second;
	list<Code*>::iterator code = lCode.begin();
	for(; code != lCode.end(); code++)
	{		
		if((*code)->Check())
			break;
	}
	
	nbASSERT(code != lCode.end(),"There is a bug!");
	return (*code)->GetVariable();
}


SymbolTemp* EncapFSA::GetVariableFromCode1(string code)
{
	return GetVariableFromCode(code,FIRST);
}

SymbolTemp* EncapFSA::GetVariableFromCode2(string code)
{
	return GetVariableFromCode(code,SECOND);
}

/*
*	Returns the variable associated with code, regardless any protocol
*/
SymbolTemp* EncapFSA::GetVariableFromCode(string code, instructionNumber number)
{
	Instruction_t instruction;
	
	switch(number)
	{
		case FIRST:
			instruction = m_Instruction1;
			break;
		case SECOND:
			instruction = m_Instruction2;
			break;
	}
	
	Instruction_t::iterator ofProto = instruction.begin();
	for(; ofProto != instruction.end(); ofProto++)
	{
		list<Code*> lCode = (*ofProto).second;
		list<Code*>::iterator c = lCode.begin();
		for(; c != lCode.end(); c++)
		{		
			if((*c)->GetCode()==code)
				return (*c)->GetVariable();
		}
	}
	return NULL;
}

/***************************** Automaton builder *****************************/

//The following methods was written by Ivano Cerrato in 2011/12

EncapFSABuilder::EncapFSABuilder(EncapGraph &protograph, EncapFSA::Alphabet_t &alphabet)
	:m_Graph(protograph), m_Alphabet(alphabet), m_Status(nbSUCCESS)
{
}

/*
*   Create the FSA related to a regular expression
*   innerList represents the header chain
*   status is ACCEPTING_STATE or ACTION_STATE, and indicates the status that must be associated with the rightmost state of the automaton
*/

/*
*	Note that the target cannot be a set or the any placeholder, and cannot have a repeat operator
*	Header indexing with repeat operators are not supported
*	Tunneled cannot be specified on the any placeholder
*/

//FIXME: startproto in un set ha senso? 
//FIXME: ammettere un set come target, ma con un unico elemento. serve per unire più predicati espressi sul target

EncapFSA *EncapFSABuilder::BuildRegExpFSA(list<PFLSetExpression*>* innerList,status_t status)
{	
	EncapFSA *fsa = new EncapFSA(m_Alphabet);
	
	EncapFSA::Alphabet_t empty; 		//empty alphabet
	
	set<SymbolProto*>allProtos; //contains each symbol of the alphabet
	EncapFSA::Alphabet_t completeAlphabet = CompleteAlphabet(&allProtos);	//aphabet made up of all the possible symbols
	
	EncapFSA::State *previousState;	
	set<SymbolProto*>previousProtos; //stores the protocols related to the last state built
	
	bool firstStart = StartAsFirst(innerList);
	
	if(!firstStart)
	{
		//the first element of the regexp is not startproto
		EncapStIterator_t eatAllState = fsa->AddState(NULL);
		
		//we look ahead to make an important optimization that avoids the usage of the counter in case we
		//are building the extraction automaton for the elements "proto%1.field[*]" or "proto.field[*]"
		if(status == ACTION_STATE && (*(innerList->rbegin()))->GetHeaderIndex() == 1)
		{
			/*
			*	Optimization: on the self loop we do not put the symbols leading to proto. This way, we can use
			*	a normal transition to link the starting state to the one representing "protocol", i.e. the second
			*	state of the extraction NFA we are building.
			*/
			SymbolProto *protocol = (*(innerList->rbegin()))->GetProtocol();
			EncapFSA::Alphabet_t alph;
			for(EncapFSA::Alphabet_t::iterator it = completeAlphabet.begin(); it != completeAlphabet.end(); it++)
			{
				if((*it).second != protocol)
					alph.insert(*it);
			}
			fsa->AddTrans(eatAllState, eatAllState, alph, false, NULL);
		}
		else
			fsa->AddTrans(eatAllState, eatAllState, completeAlphabet, false, NULL);
		
		fsa->SetInitialState(eatAllState);
		previousState = &(*eatAllState);
		previousProtos.insert(allProtos.begin(),allProtos.end()); 
		//the just created state represents the element .*, hence it is related to any protocol of the database
		PRINT_DOT(fsa, "buildregexpfsa - before header chain", "buildregexpfsa_before_chain");
	}
	else
	{
		//the first element of the regexp is startproto
		EncapStIterator_t startState = fsa->AddState(NULL);
		fsa->SetInitialState(startState);
		previousState = &(*startState);
		previousProtos.insert(m_Graph.GetStartProtoNode().NodeInfo);
		//the just created state represents the element startproto
		PRINT_DOT(fsa, "buildregexpfsa - after first element", "buildregexpfsa_after_startproto");
	}

	
	bool all_ok = true; //it is false when we understand that the regexp can never be verified, in agreement with the protocol encapsulation graph
	
	EncapFSA::State *newState = NULL;
	
	//get an iterator over the elements of the header chain. Each element is a PFLSetExpression
	list<PFLSetExpression*>::reverse_iterator it = innerList->rbegin();
	
	if(firstStart)
		it++;
    	
    AssignProtocolLayer assigner(m_Graph);//needed in case of tunneled
	
	for(; it != innerList->rend(); it++)
	{
		//Prepare the data to create the right links and states on the automaton
	
		PFLInclusionOperator inclusionOp = (*it)->GetInclusionOperator(); 	//DEFAULT - IN - NOTIN
		PFLRepeatOperator repeatOp = (*it)->GetRepeatOperator();			//DEFAULT_ROP - PLUS - STAR - QUESTION 		
		bool any = ((*it)->IsAnyPlaceholder())? true : false;
		bool mandatoryTunnels = (*it)->MandatoryTunnels();					
		set<SymbolProto*>currentProtos; 
		set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates; 
		if(any)
		{
			//in any -> .
			//the current element is the any placeholder, hence it is related to any protocol
			//the any placeholder can be involved only in the "in" operation. the grammar forbids "notin any"
			currentProtos.insert(allProtos.begin(),allProtos.end());
		}
		else
		{
			/***
			*
			*	Grammar checks performed now 
			*
			*	1) each element of the set, must be related to a different protocol;																
			*	2) into the same element of the set, only a protocol can be referred;																
			*	3) into the same element of the set, all the instance of the protocol must have the same value of header indexing					
			*	4) if, into an element of the set, the operators AND, OR, NOT, () have been used, the protocol involved must have a predicate;		
			*	5) if the set has the repeat operator, no header indexing can be specified on its elements											
			*
			***/
			
			/***
			*
			*	The varius elements of a set ar in OR, e.g. {ip,ipv6} means ip OR ipv6
			*	Into the same element, the varius predicates are joined with the operator(s) specified by the user
			*
			***/
			
			list<PFLExpression*> *elements = (*it)->GetElements(); 	//get the protocols specified in the current set 
			list<PFLExpression*>::iterator eIt = elements->begin();
			SymbolProto *protocol = NULL;						   	//the protocol corresponding to the current element
			set<SymbolProto*>found;									//this variable is required to check if, into the current element, a protocol appears more times
			
			/***
			*
			*	notin {p1.a==x and p1.b==y,p2,...} -> [^p1 p2 ...]
			*	in {p1%1.src==X or p1%1.dst==X,p2,...} -> [p1 p2 ...]
			*	the current element could have a repeat operator, the header indexing, a predicate and/or the tunneled. 
			*	an element cannot have both the header indexing and a repeat operator
			*	notin -> the current element is related to any protocol except those specified by the notin
			*	in	  -> the current element is related to the protocols specied by the in	
			*
			***/		
				
			if(inclusionOp==NOTIN)
				currentProtos.insert(allProtos.begin(),allProtos.end());
				
			//iterates over the elements of the set
			for(;eIt != elements->end();eIt++)
			{
				//eIt could be a unary, a binary or a terminal expression
				protocol = (*eIt)->GetProtocol(); 	//get the protocol
				
				if(protocol==NULL)
				{
					//syntax error!	
					m_ErrorRecorder.PFLError("syntax error - the predicates into an element of the set, must refer to the same protocol.");
					return NULL;
				}
				
				if(found.count(protocol)!=0)
				{
					//syntax error!
					m_ErrorRecorder.PFLError("syntax error - into the same set, a protocol cannot appear multiple times.");
					return NULL;
				}
				found.insert(protocol);
				
				PFLExpressionType type = (*eIt)->GetType(); //TERM, UNARY, BINARY
				nbASSERT(type == PFL_TERM_EXPRESSION || type == PFL_UNARY_EXPRESSION || type == PFL_BINARY_EXPRESSION,"There is a bug!");
				
				int headerIndexing = (*eIt)->GetHeaderIndex();
				if(headerIndexing<0)
				{
					//syntax error
					m_ErrorRecorder.PFLError("syntax error - the predicates into an element of the set, must have the same header indexing.");
					return NULL;
				}
								
				if(headerIndexing > 0)
				{
					//header indexing
					if(repeatOp != DEFAULT_ROP)
					{
						//syntax error!
						m_ErrorRecorder.PFLError("syntax error - you cannot specify a repeat operator and the header indexing on the same element.");
						return NULL;
					}
				
					//this element requires the header indexing. Then all the symbol leading to its protocol must have the code to increment a counter
					EncapLabel_t label = std::make_pair<SymbolProto*, SymbolProto*>(NULL, protocol);//label representing all the symbols leading to proto
					
					if(headerIndexing == 1 && status == ACTION_STATE)
					{
						//This is is executed if we are extracting "proto%1.field[*]" or "proto.field[*]"
						//The optimization is depicted at the beginning of this method
						headerIndexing = 0;
					}					
					else
						fsa->AddCode1(label,protocol->Name+".HeaderCounter++",true);
				}
				
				if(mandatoryTunnels)
				{
					//tunneled
					assigner.Classify(); //assign a layer to each protocol of the protocol encapsulation graph
					assigner.StartRequiringProtocolsFrom(protocol->Level);
					
					stringstream ss;
					//the code is "Ln.LevelCounter++"
					ss << "L" << protocol->Level << ".LevelCounter++";

					//assign the counter to all the protocols with a lavel >= of the current one
					while(true)
					{
						set<SymbolProto*> protocols = assigner.GetNext();
						if(protocols.size() == 0)
							break;
						for(set<SymbolProto*>::iterator sp = protocols.begin(); sp != protocols.end(); sp++)
						{
							//this protocol requires the counter for tunneled.
							EncapLabel_t label = std::make_pair<SymbolProto*, SymbolProto*>(NULL, *sp);//label representing all the symbols leading to this protocol 
							fsa->AddCode2(label,ss.str(), (*sp)->Name == protocol->Name);//the check on the ET, must be performed only for the protocol on which the keyword "tunneled" refers
						}
					}
				}
				
				if(type != PFL_TERM_EXPRESSION && !(*eIt)->HasPredicate())
				{
					//syntax error!
					m_ErrorRecorder.PFLError("syntax error - in an element of the set in which the operators AND, OR, NOT, (), have been used, the protocol involved must have a predicate(s).");
					return NULL;
				}
					
				bool term_and_pred = false;
				if(type == PFL_TERM_EXPRESSION)
				{
					PFLTermExpression *term = static_cast<PFLTermExpression*>(*eIt);
					term_and_pred = (term->GetIRExpr()!=NULL);
				}
					
				if(inclusionOp==NOTIN)
				{
					currentProtos.erase(protocol);
			
					//if at least a predicate must be avaluated, store the protocol and the related expression
					if(term_and_pred || type != PFL_TERM_EXPRESSION || headerIndexing > 0 || mandatoryTunnels)
					{
						//this protocol has the predicate
						set<SymbolProto*> aux;
						aux.insert(protocol);
						currentPredicates.insert(make_pair<set<SymbolProto*>,PFLTermExpression*>(aux,static_cast<PFLTermExpression*>(*eIt)));
					}
				}
				else
				{
					if((type == PFL_TERM_EXPRESSION && !term_and_pred) && headerIndexing == 0 && !mandatoryTunnels)
						//this protocol is without predicate
						currentProtos.insert(protocol);
					else 
					{	
						//this protocol has the predicate. it could be on a field, on a counter, or on both
						set<SymbolProto*> aux;
						aux.insert(protocol);
						currentPredicates.insert(make_pair<set<SymbolProto*>,PFLTermExpression*>(aux,static_cast<PFLTermExpression*>(*eIt)));
					}
				
				}
			}//	end for over the elements of the set 		
		}
		
		//now, we know the protocols represented by the current state, and those represented by the previous state	
		newState = CreateLink(fsa,previousProtos,currentProtos,previousState,repeatOp,completeAlphabet,currentPredicates,inclusionOp,mandatoryTunnels);
		
		if(newState == NULL)
		{
			//the regexp cannot be matched in according to the referred encapsulation rules
			all_ok = false;
			break;
		}	
	
		if(repeatOp == PLUS || repeatOp == DEFAULT_ROP)
			//STAR and QUESTION, because the incoming epsilon transition, represent also the protocols of the previous state
			previousProtos.clear();

		previousProtos.insert(currentProtos.begin(),currentProtos.end());
		//we must consider also those protocols reached if a predicate is satisfied
		for(set<pair<set<SymbolProto*>,PFLExpression*> >::iterator prIt = currentPredicates.begin(); prIt != currentPredicates.end(); prIt++)
		{
			set<SymbolProto*> aux = (*prIt).first;
			previousProtos.insert(*(aux.begin()));
		}
		
		previousState = newState;
		
		PRINT_DOT(fsa, "buildregexpfsa - after the management of an element", "buildregexpfsa_after_element_loop");
	
	} //end iteration over the elements of the header chain
	
	//we have to set last state as the final accepting state, in case of actual header chain
	//otherwise, a new state must be created and set as final state. 
	if(all_ok)
	{	
		EncapStIterator_t lastState = fsa->GetStateIterator(*previousState);
		if(status == ACCEPTING_STATE)
		{
			fsa->setFinal(lastState);
			fsa->AddTrans(lastState, lastState, empty, true, NULL);		
			fsa->setAccepting(lastState);
		}
		else	//status == ACTION_STATE
		{
			/*
			*	consider the extraction from ip%1.src. The rightmost state, now, must be set as action
			*	but we cannot put on it a self loop firing with each symbol. In fact, this would mean that we want to extract
			*	for each symbol received after that leading to the rightomost state. Then, a new state is needed, which is a final one
			*	(neither accepting and action)
			*/
			fsa->setAction(lastState);
			EncapStIterator_t furtherState = fsa->AddState(NULL);		
			fsa->AddTrans(lastState, furtherState, completeAlphabet, false, NULL);
			fsa->setFinal(furtherState);
			fsa->AddTrans(furtherState, furtherState, empty, true, NULL);		
		}
	}
	  
	PRINT_DOT(fsa, "buildregexpfsa - automaton completed!", "buildregexpfsa_end");
	return fsa;	
}

//Create a label for each transition of the Protocol Encapsulation Graph, and store each protocol in a set
EncapFSA::Alphabet_t EncapFSABuilder::CompleteAlphabet(set<SymbolProto*> *allProtos)
{
	EncapFSA::Alphabet_t alphabet;
	m_Graph.SortRevPostorder_alternate(m_Graph.GetStartProtoNode());
	for (EncapGraph::SortedIterator i = m_Graph.FirstNodeSorted(); i != m_Graph.LastNodeSorted(); i++)
	{
		allProtos->insert((*i)->NodeInfo);
		list<EncapGraph::GraphNode*> &predecessors = (*i)->GetPredecessors();
		for (list<EncapGraph::GraphNode*>::iterator p = predecessors.begin();p != predecessors.end();p++)
		{
			EncapLabel_t label = std::make_pair<SymbolProto*, SymbolProto*>((*p)->NodeInfo, (*i)->NodeInfo);
			alphabet.insert(label);
		}				
	}
	return alphabet;
}

//I'm sure that "expr" has predicate(s) on protocol's fields
//Create an automaton with an extended transition that evaluates all the predicates
EncapFSA *EncapFSABuilder::AnalyzeExpression(SymbolProto *prevProto, SymbolProto *newProto, PFLExpression *expr, EncapLabel_t label)
{
	nbASSERT(expr!=NULL,"There is a bug!");
	
	EncapFSA *fsa = NULL;
	
	switch(expr->GetType())
	{
		case PFL_BINARY_EXPRESSION:
			fsa = AnalyzeBinaryExpression(prevProto,newProto,expr,label);
			break;
		case PFL_UNARY_EXPRESSION:
			fsa = AnalyzeUnaryExpression(prevProto,newProto,expr,label);
			break;
		case PFL_TERM_EXPRESSION:
		{			
			fsa = AnalyzeTerminalExpression(prevProto,newProto,expr,label);
			
			//The following operations are required to avoid that a field involved in a predicate is merged with otehrs
			string predicate = expr->getAttribute();
			size_t pos = predicate.find(".");
			string proto = predicate.substr(0,pos);
			string field = predicate.substr(pos+1);
#ifdef ENABLE_FIELD_OPT	
			predicatesOnFields.insert(make_pair<string,string>(proto,field));
#endif
			break;
		}
		default:
			nbASSERT(false,"You cannot be here!");
	}
	
	PRINT_DOT(fsa, "buildregexpfsa - Predicate(s) visited", "buildregexpfsa_predicates_visited");
	
	return fsa;	
}

//create a small automaton, useful to build an extended transition
EncapFSA *EncapFSABuilder::AnalyzeTerminalExpression(SymbolProto *prevProto, SymbolProto *newProto, PFLExpression *expr, EncapLabel_t label)
{
	EncapFSA *fsa = new EncapFSA(m_Alphabet);
	PFLTermExpression *tExpr = static_cast<PFLTermExpression*>(expr);
	
	//create the states
	EncapStIterator_t prevState = fsa->AddState(NULL);
	EncapStIterator_t newState = fsa->AddState(NULL);
	EncapStIterator_t failState = fsa->AddState(NULL);

	//assign a role to the states
	fsa->SetInitialState(prevState);
	fsa->setFinal(newState);
	fsa->setFinal(failState);
	fsa->setAccepting(newState);

	//create the extended transition
	fsa->addExtended(prevState, label, newState, failState, tExpr);

	PRINT_DOT(fsa, "buildregexpfsa - Analyzed terminal expression", "buildregexpfsa_analyze_term_expr");

	return fsa;
}

/* 
*	create a small automaton, useful to build an extended transition
*/
EncapFSA *EncapFSABuilder::AnalyzeBinaryExpression(SymbolProto *prevProto, SymbolProto *newProto, PFLExpression *expr, EncapLabel_t label)
{
	EncapFSA *fsa1 = NULL;
	EncapFSA *fsa2 = NULL;
    EncapFSA *result = NULL;
	PFLBinaryExpression *bExpr = static_cast<PFLBinaryExpression*>(expr);

	switch(bExpr->GetOperator()) 
	{
		case BINOP_BOOLAND:
            fsa1 = AnalyzeExpression(prevProto,newProto,bExpr->GetLeftNode(), label);
           	nbASSERT(fsa1 != NULL,"There is a bug! The automaton cannot be NULL");
           
            fsa2 = AnalyzeExpression(prevProto,newProto,bExpr->GetRightNode(), label);
           	nbASSERT(fsa2 != NULL,"There is a bug! The automaton cannot be NULL");
         
            result = EncapFSA::BooleanAND(fsa1,fsa2);
            
            PRINT_DOT(result, "buildregexpfsa - Analyzed binary expression", "buildregexpfsa_after_and_of_two_predicates");
            
            break;
	case BINOP_BOOLOR:
            fsa1 = AnalyzeExpression(prevProto,newProto,bExpr->GetLeftNode(), label);
           	nbASSERT(fsa1 != NULL,"There is a bug! The automaton cannot be NULL");
            
            fsa2 = AnalyzeExpression(prevProto,newProto,bExpr->GetRightNode(), label);
           	nbASSERT(fsa2 != NULL,"There is a bug! The automaton cannot be NULL");
        	
        	result = EncapFSA::BooleanOR(fsa1, fsa2);
			
			PRINT_DOT(result, "buildregexpfsa - Analyzed binary expression", "buildregexpfsa_after_or_of_two_predicates");
			
			break;          
	default:
          nbASSERT(false, "CANNOT BE HERE");
          break;
	}

	return result;
}

/*
*	creates a small automaton, useful to build an extended transition
*/
EncapFSA *EncapFSABuilder::AnalyzeUnaryExpression(SymbolProto *prevProto, SymbolProto *newProto, PFLExpression *expr, EncapLabel_t label)
{
	EncapFSA *fsa = NULL;
	PFLUnaryExpression *uExpr = static_cast<PFLUnaryExpression*>(expr);
	
	switch(uExpr->GetOperator())
	{
		case UNOP_BOOLNOT:
			fsa = AnalyzeExpression(prevProto,newProto,uExpr->GetChild(), label);
			fsa->BooleanNot();
			break;
		default:
			nbASSERT(false, "CANNOT BE HERE");
			break;
	}
	
	PRINT_DOT(fsa, "buildregexpfsa - Analyzed unary expression", "buildregexpfsa_after_not_of_predicate");
	
	return fsa;
}

EncapFSA *EncapFSABuilder::ManagePredicatesOnCounters(SymbolProto *prevProto, SymbolProto *newProto,EncapLabel_t label,EncapFSA *predicates,int headerIndex,RangeOperator_t op,string code)
{
	//create the new automaton
	EncapFSA *fsa = new EncapFSA(m_Alphabet);
	EncapFSA *result = new EncapFSA(m_Alphabet);
	
	//create the states
	EncapStIterator_t prevState = fsa->AddState(NULL);
	EncapStIterator_t newState = fsa->AddState(NULL);
	EncapStIterator_t failState = fsa->AddState(NULL);

	//assign a role to the states
	fsa->SetInitialState(prevState);
	fsa->setFinal(newState);
	fsa->setFinal(failState);
	fsa->setAccepting(newState);

	//create the extended transition
	fsa->addExtended(prevState, label, newState, failState, headerIndex, op, code);
	
	PRINT_DOT(fsa, "buildregexpfsa - ET related to a counter has been created", "buildregexpfsa_counterET_created");
	
	if(predicates == NULL)
		result = fsa;
	else
    	result = EncapFSA::BooleanAND(fsa,predicates);

	PRINT_DOT(fsa, "buildregexpfsa - After managing of a counter", "buildregexpfsa_counter_managed");

	return result;
}

//Move the ET from the mini automaton to the one representing the regular expression
void EncapFSABuilder::MoveET(EncapFSA::State *previousState,EncapFSA::State *newState,EncapFSA::State *failState, EncapFSA *regExp, EncapFSA *predicates, EncapLabel_t label)
{
	nbASSERT(predicates != NULL, "The automaton representing the predicate(s) cannot be NULL!");

	//get the et in the automaton predicates
	EncapStIterator_t initialPred = predicates->GetInitialState();
	list<EncapFSA::Transition*> transitions = predicates->getTransExitingFrom(&(*initialPred));
	nbASSERT(transitions.size()>=2,"There is a bug!");
	list<EncapFSA::Transition*>::iterator transIt = transitions.begin();
	EncapFSA::ExtendedTransition* et = (*transIt)->getIncludingET();
	nbASSERT(et!=NULL,"There is a bug! This automaton has only ET!");
	
	//Get the iterators
	EncapStIterator_t pIt = regExp->GetStateIterator(previousState);
	EncapStIterator_t nIt = regExp->GetStateIterator(newState);
	EncapStIterator_t fIt = regExp->GetStateIterator(failState);

	//change the automaton and the states of the et
	regExp->addExtended(pIt, label, nIt, fIt, et); 
	
	PRINT_DOT(regExp, "buildregexpfsa - after the changing of automaton of an et", "buildregexpfsa_after_change_automaton_to_et");
}

//links two states
//IVANO HATES THE PREDICATES!!!
EncapFSA::State *EncapFSABuilder::CreateLink(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates, PFLInclusionOperator inclusionOp, bool mandatoryTunnels)
{
	switch(repeatOp)
	{ 
		case DEFAULT_ROP:
			return ManageDefaultROP(fsa,previousProtos,currentProtos,previousState,repeatOp,completeAlphabet,currentPredicates, inclusionOp,currentPredicates.size(),mandatoryTunnels);
		case PLUS:
			return ManagePLUS(fsa,previousProtos,currentProtos,previousState,repeatOp,completeAlphabet,currentPredicates,inclusionOp,currentPredicates.size(),mandatoryTunnels);	
		case STAR:
			return ManageSTAR(fsa,previousProtos,currentProtos,previousState,repeatOp,completeAlphabet,currentPredicates,inclusionOp,currentPredicates.size(),mandatoryTunnels);
		case QUESTION:
			return ManageQUESTION(fsa,previousProtos,currentProtos,previousState,repeatOp,completeAlphabet,currentPredicates,inclusionOp,currentPredicates.size(),mandatoryTunnels);	
		default:
			nbASSERT(false,"There is a BUG! You cannot be here!");
	}
	return NULL;//useless but the compiler wants it
}

//manage the case without the repeat operator
//this is the only case that support the header indexing
EncapFSA::State *EncapFSABuilder::ManageDefaultROP(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates,PFLInclusionOperator inclusionOp,int predSize, bool mandatoryTunnels)
{
	EncapStIterator_t newState = fsa->AddState(NULL);
	EncapStIterator_t predecessorState = fsa->GetStateIterator(*previousState);
	
	bool link = false;

	//create the alphabet
	EncapFSA::Alphabet_t alphabet = CreateAlphabet(previousProtos,currentProtos,completeAlphabet);
	if(alphabet.size()==0)
	{
		if(predSize==0)
		{
			//we are sure that the transition will never fire
			fsa->removeState(&(*newState));
			return NULL;
		}
	}
	else
	{
		//add the transition
		fsa->AddTrans(predecessorState, newState, alphabet, false, NULL);		
		link = true;
	}
			
	//create the eventual extended transitions
	if(predSize!=0)
	{
		EncapStIterator_t failState = fsa->AddState(NULL);//create a fail state
	
		for(set<pair<set<SymbolProto*>,PFLExpression*> >::iterator it = currentPredicates.begin(); it != currentPredicates.end(); it++)
		{
			//*it -> protocol - expression
			nbASSERT(((*it).first).size()==1,"There is a bug. We use a set only for conveniente, but there is only one protocol");
			PFLExpression *currentPEx = (*it).second;
			EncapFSA::Alphabet_t extAlphabet = CreateAlphabet(previousProtos,(*it).first,completeAlphabet);
			if(extAlphabet.size()!=0)
			{
				link = true;
				for(EncapFSA::Alphabet_t::iterator aIt = extAlphabet.begin(); aIt != extAlphabet.end(); aIt++)
				{
					EncapFSA *predicates = NULL;
					if(currentPEx->GetType()!=PFL_TERM_EXPRESSION || (currentPEx->GetType()==PFL_TERM_EXPRESSION && currentPEx->HasPredicate()))
						//manage the predicates related to the protocol's fields
						predicates = AnalyzeExpression(previousState->GetInfo(),(&(*newState))->GetInfo(),currentPEx,*aIt);		
				
					if(currentPEx->GetHeaderIndex()>0)
					{
						//manage the predicate on a counter related to the header index
						SymbolProto *p = *(((*it).first).begin());
						string code = fsa->GetCodeToCheck1(make_pair<SymbolProto*, SymbolProto*>(NULL, p));
						code.resize(code.size()-2);
						predicates = ManagePredicatesOnCounters(previousState->GetInfo(),(&(*newState))->GetInfo(),*aIt,predicates,currentPEx->GetHeaderIndex(),EQUAL,code);
					}
					
					if(mandatoryTunnels)
					{
						//manage the predicate on a counter related to the tunneled
						SymbolProto *p = *(((*it).first).begin());
						string code = fsa->GetCodeToCheck2(make_pair<SymbolProto*, SymbolProto*>(NULL, p));
						code.resize(code.size()-2);
						predicates = ManagePredicatesOnCounters(previousState->GetInfo(),(&(*newState))->GetInfo(),*aIt,predicates,2,GREAT_EQUAL_THAN,code);
					}
					
					MoveET(previousState,(inclusionOp!=NOTIN)? &(*newState) : &(*failState),(inclusionOp!=NOTIN)? &(*failState) : &(*newState),fsa,predicates,*aIt);					
				} 
			}
		}

		if(!link)
		{
			//the regular expression will never be mathced!
			fsa->removeState(&(*newState));
			fsa->removeState(&(*failState));
			return NULL;
		}
	}
	return &(*newState);
}

//manage the repeat operator +
EncapFSA::State *EncapFSABuilder::ManagePLUS(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates,PFLInclusionOperator inclusionOp,int predSize, bool mandatoryTunnels)
{
	EncapStIterator_t newState = fsa->AddState(NULL);
	EncapStIterator_t predecessorState = fsa->GetStateIterator(*previousState);
	
	bool link = false;

	//create the alphabet
		
	EncapFSA::Alphabet_t alphabet = CreateAlphabet(previousProtos,currentProtos,completeAlphabet);
	if(alphabet.size()==0)
	{
		if(predSize==0)
		{
			//the transition will never fire
			fsa->removeState(&(*newState));
			return NULL;
		}
	}
	else
	{
		//add the transition
		fsa->AddTrans(predecessorState, newState, alphabet, false, NULL);		
		link = true;
	}
	
	EncapStIterator_t failState = fsa->AddState(NULL);//create a fail state
	
	//create the eventual extended transition from the previous state
	if(predSize!=0)
	{
		for(set<pair<set<SymbolProto*>,PFLExpression*> >::iterator it = currentPredicates.begin(); it != currentPredicates.end(); it++)
		{
			EncapFSA::Alphabet_t extAlphabet = CreateAlphabet(previousProtos,(*it).first,completeAlphabet);
			if(extAlphabet.size()!=0)
			{
				link = true;
				PFLExpression *currentPEx = (*it).second;
				for(EncapFSA::Alphabet_t::iterator aIt = extAlphabet.begin(); aIt != extAlphabet.end(); aIt++)
				{	
					EncapFSA *predicates = NULL;
					if(currentPEx->GetType()!=PFL_TERM_EXPRESSION || (currentPEx->GetType()==PFL_TERM_EXPRESSION && currentPEx->HasPredicate()))
						//manage the predicates related to the protocol's fields
						predicates = AnalyzeExpression(previousState->GetInfo(),(&(*newState))->GetInfo(),currentPEx,*aIt);		
				
					if(mandatoryTunnels)
					{
						//manage the predicate on a counter related to the tunneled
						SymbolProto *p = *(((*it).first).begin());
						string code = fsa->GetCodeToCheck2(make_pair<SymbolProto*, SymbolProto*>(NULL, p));
						code.resize(code.size()-2);
						predicates = ManagePredicatesOnCounters(previousState->GetInfo(),(&(*newState))->GetInfo(),*aIt,predicates,2,GREAT_EQUAL_THAN,code);
					}
					
					MoveET(previousState,(inclusionOp!=NOTIN)? &(*newState) : &(*failState),(inclusionOp!=NOTIN)? &(*failState) : &(*newState),fsa,predicates,*aIt);
		
				} //end iteration over the alphabet
			}
		}

		if(!link)
		{
			fsa->removeState(&(*newState));
			fsa->removeState(&(*failState));
			return NULL;
		}
	}
	
	/*******SELF LOOP*******/
	
	//create the alphabet for the self loop
	set<SymbolProto*> aux;
	aux.insert(currentProtos.begin(),currentProtos.end());
	for(set<pair<set<SymbolProto*>,PFLExpression*> >::iterator it = currentPredicates.begin(); it != currentPredicates.end(); it++)
		aux.insert((*it).first.begin(),(*it).first.end());
	
	//normal transition
	EncapFSA::Alphabet_t alphabetSL = CreateAlphabet(aux,currentProtos,completeAlphabet);
	//EncapFSA::Alphabet_t alphabetSL = CreateAlphabet(currentProtos,currentProtos,completeAlphabet);
	if(alphabetSL.size() != 0)
		//add the self loop
		fsa->AddTrans(newState, newState, alphabetSL, false, NULL);		
		
	//extended transition
	if(predSize!=0)
	{
		for(set<pair<set<SymbolProto*>,PFLExpression*> >::iterator it = currentPredicates.begin(); it != currentPredicates.end(); it++)
		{	
			EncapFSA::Alphabet_t alphabetSLExt = CreateAlphabet(aux,(*it).first,completeAlphabet);
			PFLExpression *currentPEx = (*it).second;	
			for(EncapFSA::Alphabet_t::iterator aIt = alphabetSLExt.begin(); aIt != alphabetSLExt.end(); aIt++)
			{
				EncapFSA *predicates = NULL;
				if(currentPEx->GetType()!=PFL_TERM_EXPRESSION || (currentPEx->GetType()==PFL_TERM_EXPRESSION && currentPEx->HasPredicate()))
					//manage the predicates related to the protocol's fields
					predicates = AnalyzeExpression(/*previousState*/(&(*newState))->GetInfo(),(&(*newState))->GetInfo(),currentPEx,*aIt);		
			
				if(mandatoryTunnels)
				{
					//manage the predicate on a counter related to the tunneled
					SymbolProto *p = *(((*it).first).begin());
					string code = fsa->GetCodeToCheck2(make_pair<SymbolProto*, SymbolProto*>(NULL, p));
					code.resize(code.size()-2);
					predicates = ManagePredicatesOnCounters(/*previousState*/(&(*newState))->GetInfo(),(&(*newState))->GetInfo(),*aIt,predicates,2,GREAT_EQUAL_THAN,code);
				}
				
				MoveET(&(*newState)/*previousState*/,(inclusionOp!=NOTIN)? &(*newState) : &(*failState),(inclusionOp!=NOTIN)? &(*failState) : &(*newState),fsa,predicates,*aIt);
			}
		}
	}
	
	if(predSize==0)
		fsa->removeState(&(*failState));
	return &(*newState);	
}

//manage the repeat operator *
EncapFSA::State *EncapFSABuilder::ManageSTAR(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates,PFLInclusionOperator inclusionOp,int predSize, bool mandatoryTunnels)
{
	EncapStIterator_t newState = fsa->AddState(NULL);
	EncapStIterator_t predecessorState = fsa->GetStateIterator(*previousState);

	//add the epsilon transition
	fsa->AddEpsilon(predecessorState, newState, NULL);
	
	/***
	*
	*	The situation on the self loop is a bit complicated
	*	As starting protocol of the symbols on the transition, we consider also the protocols assigned to the previous state, 
	*	due the incoming epsilon transition.
	*	As destination protocol of the symbols on the transition, we consider only the protocols assigned to the current state.
	*	Consider the fragment "in ip* in eth". The current state is related to the element "ip*". The self loop could have, as origin
	*	protocol of its symbols, both eth end ip. While the target of this symbol can be only ip.
	*
	***/

	previousProtos.insert(currentProtos.begin(),currentProtos.end());
	for(set<pair<set<SymbolProto*>,PFLExpression*> >::iterator it = currentPredicates.begin(); it != currentPredicates.end(); it++)
	{
			previousProtos.insert((*it).first.begin(),(*it).first.end());
	}
	EncapFSA::Alphabet_t alphabet = CreateAlphabet(previousProtos,currentProtos,completeAlphabet);
	if(alphabet.size() != 0)
		//add the self loop
		fsa->AddTrans(newState, newState, alphabet, false, NULL);		
		
	//consider now the extended transitions
	if(predSize!=0)
	{
		EncapStIterator_t failState = fsa->AddState(NULL);//create a fail state
	
		for(set<pair<set<SymbolProto*>,PFLExpression*> >::iterator it = currentPredicates.begin(); it != currentPredicates.end(); it++)
		{	
			EncapFSA::Alphabet_t alphabetSLExt = CreateAlphabet(previousProtos,(*it).first,completeAlphabet);
			PFLExpression *currentPEx = (*it).second;	
			for(EncapFSA::Alphabet_t::iterator aIt = alphabetSLExt.begin(); aIt != alphabetSLExt.end(); aIt++)
			{
				EncapFSA *predicates = NULL;
				if(currentPEx->GetType()!=PFL_TERM_EXPRESSION || (currentPEx->GetType()==PFL_TERM_EXPRESSION && currentPEx->HasPredicate()))
					//manage the predicates related to the protocol's fields
					predicates = AnalyzeExpression((&(*newState))->GetInfo(),(&(*newState))->GetInfo(),currentPEx,*aIt);		
		
				if(mandatoryTunnels)
				{
					//manage the predicate on a counter related to the tunneled
					SymbolProto *p = *(((*it).first).begin());
					string code = fsa->GetCodeToCheck2(make_pair<SymbolProto*, SymbolProto*>(NULL, p));
					code.resize(code.size()-2);
					predicates = ManagePredicatesOnCounters((&(*newState))->GetInfo(),(&(*newState))->GetInfo(),*aIt,predicates,2,GREAT_EQUAL_THAN,code);
				}
				
				MoveET((&(*newState)),(inclusionOp!=NOTIN)? &(*newState) : &(*failState),(inclusionOp!=NOTIN)? &(*failState) : &(*newState),fsa,predicates,*aIt);			
			} 
		}
	}
	
	return &(*newState);	
}

//manage the repeat operator ?
EncapFSA::State *EncapFSABuilder::ManageQUESTION(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates,PFLInclusionOperator inclusionOp,int predSize, bool mandatoryTunnels)
{

	EncapStIterator_t newState = fsa->AddState(NULL);
	EncapStIterator_t predecessorState = fsa->GetStateIterator(*previousState);

	//add the epsilon transition
	fsa->AddEpsilon(predecessorState, newState, NULL);
	//create the alphabet
	EncapFSA::Alphabet_t alphabet = CreateAlphabet(previousProtos,currentProtos,completeAlphabet);
	if(alphabet.size()!=0)
		//add the transition
		fsa->AddTrans(predecessorState, newState, alphabet, false, NULL);						
	
	if(predSize!=0)
	{
		EncapStIterator_t failState = fsa->AddState(NULL);//create a fail state
	
		for(set<pair<set<SymbolProto*>,PFLExpression*> >::iterator it = currentPredicates.begin(); it != currentPredicates.end(); it++)
		{
			nbASSERT(((*it).first).size()==1,"There is a bug. We use a set only for convenience, but there is only one protocol");
			PFLExpression *currentPEx = (*it).second;
			EncapFSA::Alphabet_t extAlphabet = CreateAlphabet(previousProtos,(*it).first,completeAlphabet);
			if(extAlphabet.size()!=0)
			{
				for(EncapFSA::Alphabet_t::iterator aIt = extAlphabet.begin(); aIt != extAlphabet.end(); aIt++)
				{
					EncapFSA *predicates = NULL;
					if(currentPEx->GetType()!=PFL_TERM_EXPRESSION || (currentPEx->GetType()==PFL_TERM_EXPRESSION && currentPEx->HasPredicate()))
						//manage the predicates related to the protocol's fields
						predicates = AnalyzeExpression(previousState->GetInfo(),(&(*newState))->GetInfo(),currentPEx,*aIt);		
			
					if(mandatoryTunnels)
					{
						//manage the predicate on a counter related to the tunneled
						SymbolProto *p = *(((*it).first).begin());
						string code = fsa->GetCodeToCheck2(make_pair<SymbolProto*, SymbolProto*>(NULL, p));
						code.resize(code.size()-2);
						predicates = ManagePredicatesOnCounters(previousState->GetInfo(),(&(*newState))->GetInfo(),*aIt,predicates,2,GREAT_EQUAL_THAN,code);
					}
					MoveET(previousState,(inclusionOp!=NOTIN)? &(*newState) : &(*failState),(inclusionOp!=NOTIN)? &(*failState) : &(*newState),fsa,predicates,*aIt);
					delete(predicates);
				}
			}//end (extAlphabet.size()!=0)
		}//end iteration over the predicates
	}
	
	return &(*newState);	
}

//create an alphabet
EncapFSA::Alphabet_t EncapFSABuilder::CreateAlphabet(set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::Alphabet_t completeAlphabet)
{
	EncapFSA::Alphabet_t alphabet;
	for(set<SymbolProto*>::iterator p = previousProtos.begin(); p != previousProtos.end();p++)
	{
		for(set<SymbolProto*>::iterator c = currentProtos.begin(); c != currentProtos.end();c++)	
		{
			EncapLabel_t label = std::make_pair<SymbolProto*, SymbolProto*>(*p, *c);
			EncapFSA::Alphabet_t::iterator l = completeAlphabet.begin();
			for (; l != completeAlphabet.end(); l++)
			{
				if(*l == label)
				{
					alphabet.insert(label);
					break;
				}
			}		
		}
	}
	return alphabet;
}

/*
*	returns true if the header chain begins with startproto
*/
bool EncapFSABuilder::StartAsFirst(list<PFLSetExpression*>* innerList)
{
	list<PFLSetExpression*>::reverse_iterator it = innerList->rbegin();
	if((*it)->IsAnyPlaceholder())
		return false;
	else
	{
		list<PFLExpression*> *elements = (*it)->GetElements();
		list<PFLExpression*>::iterator element = elements->begin();	
		SymbolProto *protocolOfTheElement = (*element)->GetProtocol();
	
		if(protocolOfTheElement == NULL)
			//this is a NetPFL error, but it will be discovered later
			return false;
	
		if(protocolOfTheElement->Name == "startproto")
			return true;
		return false;
	}
}

/*
*	algorithm assign/prune/expand/prune
*/
void EncapFSABuilder::FixAutomaton(EncapFSA *fsa, status_t status)
{
	std::list<EncapFSA::State*> accepting = fsa->GetAcceptingStates();
	std::list<EncapFSA::State*> action = fsa->GetActionStates();
	if((accepting.size()==0) && (action.size()==0))
		//the automaton has not the final accepting state
		return CreateNoMatchAutomaton(fsa);
		
	//create final not accepting state and add the self loop on it
	EncapStIterator_t finalNotAccept = fsa->AddState(NULL);
	fsa->resetAccepting(finalNotAccept);
	fsa->setFinal(finalNotAccept);
	Alphabet_t empty;
	fsa->AddTrans(finalNotAccept, finalNotAccept, empty, true, NULL);
	EncapFSA::State *fail = &(*finalNotAccept);

	/***IMPORTANT
	*
	* the final accepting state, the final not accepting state, the final action state and the related self loops
	* cannot be removed during the algrithm
	*
	***/

	if(!AssignProtocol(fsa,fail))
	{
		PRINT_DOT(fsa,"fixautomaton - after the assignment failed","regexpfsa_after_assignment_failed");
		return CreateNoMatchAutomaton(fsa); 
	}
	PRINT_DOT(fsa,"fixautomaton - after the assignment","regexpfsa_after_assignment");
	
	if(!ExpandStates(fsa,fail))
	{
		PRINT_DOT(fsa,"fixautomaton - after the expansion failed","regexpfsa_after_expansion_failed");
		return CreateNoMatchAutomaton(fsa);
	}
	PRINT_DOT(fsa,"fixautomaton - after the expansion","regexpfsa_after_expansion");
	
	FindTraps(fsa,fail,status);
	
	PRINT_DOT(fsa,"fixautomaton - after the remotion of the traps","regexpfsa_after_traps_remotion");
	
	LinkToNotAccept(fsa,fail);
	
	PRINT_DOT(fsa,"fixautomaton - end","regexpfsa_end_fixautomaton");
	
	/***
	*
	* In case of filter "startproto", the final not accepting state is useless. However, we do not remove it, since it doesn't create any problem
	*
	***/

}

/*
* Identifies and labels the states associated with a single protocol
*/
bool EncapFSABuilder::AssignProtocol(EncapFSA *fsa, EncapFSA::State *fail)
{
	bool change = true;
	bool ok = true;
	
	//repeat untill there are changes in the automaton
	while(change)
	{
		change = false;
		//iterate over the states of the automaton
		EncapFSA::StateIterator s = fsa->FirstState();
  		for (; s != fsa->LastState();)
  		{	
  			EncapFSA::StateIterator auxIt = s;
  			s++;
   			if((*auxIt).GetInfo() != NULL)
  				//the current state is already related to a protocol
  				continue;
  				
  			if((*auxIt).isFinal() && !(*auxIt).isAccepting() && !(*auxIt).isAction())
  				//the current state is the final not accepting and not action state, hence we do not consider it
  				continue;
  			
  			//if the current state is the starting state of the automaton, and it is without entering transitions
  			//assign this state to startproto
  			//startproto is assigned to the initial state, also in case of it is both initial state and accepting state, i.e. the user wrote the filter "startproto" (or "startproto preferred")
  			if((*auxIt).IsInitial())
  			{  			
  				if(fsa->getTransEnteringTo(&(*auxIt)).size()==0 || (*auxIt).isAccepting()) 
  				{
	  				change = true;
	  				EncapGraph::GraphNode &startNode = m_Graph.GetStartProtoNode();
	  				SymbolProto *startProto = startNode.NodeInfo;
	  				(*auxIt).SetInfo(startProto);
	  				if(!(*auxIt).isAccepting())
	  					//remove the symbols on the outgoing transitions and beginning with a protocol different than startproto
	  					//in case of the state is both final than initial, the only outgoing transition is the self loop.
		  				CutOutgoingTransitions(fsa,&(*auxIt));
		  				
	  				ok = RemoveUselessStates(fsa,fail); //remove the states useless for the automaton
					if(!ok)
						//the regular expression cannot be mathced
						return false;
				}
	  			continue;
  			}
  			
  			//iterate over the incoming transitions of the current state
  			//store the destination protocols of the symbols into a set
  			//after the iteration, if the set has only one element, assign that protocol to the state
  			set<SymbolProto*> incomingProtos;
  			list<EncapFSA::Transition*> transition = fsa->getTransEnteringTo(&(*auxIt));
			for(list<EncapFSA::Transition*>::iterator t = transition.begin(); t != transition.end(); t++)
			{
				Alphabet_t labels = (*t)->GetInfo().first;
				for (Alphabet_t::iterator l = labels.begin();l != labels.end();++l)
      			{
      				SymbolProto *proto = (*l).second;
      				incomingProtos.insert(proto);
      			}//end iteration over the labels
			}//end iteration over the transitions
			if(incomingProtos.size()==1)
			{
				//this state can be assigned to a single protocol
				change = true;
				(*auxIt).SetInfo(*(incomingProtos.begin()));
				if((*auxIt).isAccepting())// || (*auxIt).isAction())
					continue; //we do not remove the self loop on the final accepting state and on the final action state
				CutOutgoingTransitions(fsa,&(*auxIt));//remove the symbols on the outgoing transitions and beginning with a protocol different than the one assigned to the state
				ok = RemoveUselessStates(fsa,fail); //remove the states useless for the automaton
				if(!ok)
					//the regular expression cannot be mathced
					return false;
			}
  		}//end iteration over the states			
  		PRINT_DOT(fsa,"fixautomaton - after an assignment loop","regexpfsa_assignment_loop");

	}
	
	return true;
}

/*
* Removes the symbols (and possibly the transitions) starting with a protocol different from the one related to the
* starting state of the transition
* if this state has an outgoing extended transition, also the extended transition must be removed
*/
void EncapFSABuilder::CutOutgoingTransitions(EncapFSA *fsa, EncapFSA::State *state)
{
	SymbolProto *stateProto = state->GetInfo();
	
	set<EncapFSA::ExtendedTransition*> etToRem;
	
	list<EncapFSA::Transition*> transition = fsa->getTransExitingFrom(state);
	for(list<EncapFSA::Transition*>::iterator t = transition.begin(); t != transition.end();)
	{
		list<EncapFSA::Transition*>::iterator aux = t;//needed in case of the transition must be removed
		Alphabet_t labels = (*t)->GetInfo().first;
		//iterate over the labels of the transition  
		for (Alphabet_t::iterator l = labels.begin();l != labels.end();++l)
      	{
      		SymbolProto *proto = (*l).first;
      		if(stateProto!=proto)
      		{
      			//we have to remove the label from the transition
      			(*t)->RemoveLabel(*l);
      		}
      	}//end iteration over the labels
      	t++;
      	if(((*aux)->GetInfo().first).size()==0)
      	{
      		//we have to remove the transition
      		
      		//store the eventual related extended transition
      		if((*aux)->getIncludingET())
      			etToRem.insert((*aux)->getIncludingET());
      		(*aux)->RemoveEdge();
			delete(*aux);
      	}
	}//end iteration over the transitions
	
	//remove the extended transitions, if needed
	for(set<EncapFSA::ExtendedTransition*>::iterator it = etToRem.begin(); it != etToRem.end();)
	{
		set<EncapFSA::ExtendedTransition*>::iterator aux = it;
		it++;
		delete(*aux);	
	}
}

//remove the states without exiting or entering transitions, obviously except the self loop.
//obviously, the initial state is removed only in case of it has not exiting transitions
//while the final (both accepting and not accepting) states and the action states are removed only if it has not entering transitions
//if a state has an incoming extended transition, we redirect the extended transition toward the final not accepting state
bool EncapFSABuilder::RemoveUselessStates(EncapFSA *fsa, EncapFSA::State *fail)
{
	bool change = true;
	bool ok = true;
	while(change)
	{
		change = false;
		//iterate over the states of teh automaton
		EncapFSA::StateIterator s = fsa->FirstState();
  		for (; s != fsa->LastState();)
  		{
  			if((*s).isFinal() && !(*s).isAccepting())
  			{
  				//this state is the final not accepting state
  				s++;
  				continue;
  			}
  			
  			EncapFSA::StateIterator aux = s; //useful in case of the current state must be removed
  			s++;
			if(!(*aux).IsInitial())
			{
				list<EncapFSA::Transition*> transition = fsa->getTransEnteringTo(&(*aux));
				if(transition.size()==0 || (transition.size()==1 && fsa->ThereIsSelfLoop(transition)))
				{
					//the state has not incoming tranistions, then it must be removed
					change = true;
					
					//first of all, we must remove the outgoing transitions (both normal than extended)
					CutOutgoingTransitions(fsa, &(*aux));
					
					if((*aux).isAccepting())
						//the regular expression will be never mathced, since the final accepting state has been removed
						ok = false;
					fsa->removeState(&(*aux));
					continue;
				}
			}
			if(!(*aux).isAccepting() && !(*aux).isAction())
			{
				list<EncapFSA::Transition*> transition = fsa->getTransExitingFrom(&(*aux));
				if(transition.size()==0 || (transition.size()==1 && fsa->ThereIsSelfLoop(transition)))
				{
					
					change = true;
					
					//first of all, we must remove the normal incoming transitions. the extended must be redirect toward the final not accepting state
					CutIncomingTransitions(fsa,&(*aux),fail);					
					
					//remove the state
					fsa->removeState(&(*aux));
					continue;
				}//end (transition.size()==0)
			}
		}//end iteration over the transitions 
	}
	
	return ok;
}

//given a state, removes its incoming transitions. In case of an extended transition, it is redirected to the final not accepting state
void EncapFSABuilder::CutIncomingTransitions(EncapFSA *fsa, EncapFSA::State *state, EncapFSA::State *fail)
{	
	//renove the incoming transitions, except the extended which are redirected to the final not accepting state
	list<EncapFSA::Transition*> transitionIn = fsa->getTransEnteringTo(state);
	for(list<EncapFSA::Transition*>::iterator it = transitionIn.begin(); it != transitionIn.end();)
	{
		list<EncapFSA::Transition*>::iterator auxTr = it;
		it++;
		
		EncapFSA::ExtendedTransition *et = (*auxTr)->getIncludingET();
		if(et)
		{						
			//we redirect the extended transition toward the final not accepting state
			map<EncapFSA::State*,EncapFSA::State*> ms;

			ms[&(*state)] = fail;
			et->changeGateRefs(NULL,NULL,ms);

			Alphabet_t alph = (*auxTr)->GetLabels();

			EncapStIterator_t failIt = fsa->GetStateIterator(fail);

			EncapStIterator_t fromIt = (*auxTr)->FromState();

			EncapTrIterator_t newTransIt = fsa->AddTrans(fromIt, failIt, alph, false, (*auxTr)->GetPredicate());
			EncapTrIterator_t oldTransIt = fsa->GetTransIterator(*auxTr);
			(*newTransIt).setIncludingET(et);
			et->changeEdgeRef(oldTransIt, newTransIt);
		}
		else
		{
			//this transition is not related to an extended transition, hence we simply remove it
			(*auxTr)->RemoveEdge();
			delete(*auxTr);
		}
	}//end iteration over the incoming transitions
}

/*
*	return true if there is a self loop
*/
bool EncapFSA::ThereIsSelfLoop(list<EncapFSA::Transition*> transition)
{
	for(list<EncapFSA::Transition*>::iterator it = transition.begin(); it != transition.end(); it++)
	{
		if((*it)->FromState() == (*it)->ToState())
			return true;
	}
	return false;
}

//expand a state without protocol in a new state for each destination protocol of the labels on its incoming transitions
bool EncapFSABuilder::ExpandStates(EncapFSA *fsa,EncapFSA::State *fail)
{
	//iterate over the states of teh automaton
	EncapFSA::StateIterator s = fsa->FirstState();
	for (; s != fsa->LastState();)
	{
		if((*s).GetInfo() != NULL)
		{
			//the current state is already related to a protocol
			s++;
			continue;
		}
		
		if((*s).isFinal() && !(*s).isAccepting())
		{
			//the current state is the final not accepting state
			s++;
			continue;
		}
		
		//the current state must be removed and replaced by a new state for each second protocol of the labels on the entering transitions
		
		map<SymbolProto*,EncapFSA::State*> newStates; 					//new states replacing the current one
		Alphabet_t onSelfLoop;						  					//symbols on self loop
		map<int,map<EncapLabel_t,EncapFSA::State*> > labelFromState; 	//symbols on incoming transitions, and related starting state
		map<EncapLabel_t,EncapFSA::State*> labelToState; 				//symbols on outgoing transitions, and related destination state
				
		/************INCOMING*********************/

		//iterate over the incoming transitions, including the eventual self loop. 
		//For each symbol, store the destination state. remove the transition.
		//if the transition is part of an extended transition, simply change the destination state with the right new state
		//for each new destination protocol, create a new state
		
		list<EncapFSA::Transition*> transitionIn = fsa->getTransEnteringTo(&(*s));
		int counter = 0;//it is needed since different entering transitions could have the same label
		for(list<EncapFSA::Transition*>::iterator t = transitionIn.begin(); t != transitionIn.end();)		
		{
			bool selfLoop = ((*t)->ToState() == (*t)->FromState())? true : false;	
			
			Alphabet_t labels = (*t)->GetInfo().first;
			list<EncapFSA::Transition*>::iterator aux = t;
			t++;
			if(!(*aux)->getIncludingET())
			{
				map<EncapLabel_t,EncapFSA::State*> currentLabelFromState;
				for (Alphabet_t::iterator l = labels.begin();l != labels.end();++l)
				{
					if(selfLoop)
						//this is the self loop
						onSelfLoop.insert(*l);
					else
						currentLabelFromState.insert(pair<EncapLabel_t,EncapFSA::State*>(*l,&(*(*aux)->FromState())));
				
					CreateNewStateIfNeeded(fsa,&newStates,*l);

	  			}//end iteration over the alphabet		

				labelFromState.insert(pair<int,map<EncapLabel_t,EncapFSA::State*> >(counter,currentLabelFromState));
				counter++;
			}
			else
			{
				//this incoming transition is part of an extended transition
				//we redirect the extended transition toward the right new state
				nbASSERT(labels.size()==1,"An extended transition with more than one label?");
				EncapLabel_t label = *labels.begin();
				CreateNewStateIfNeeded(fsa,&newStates,label);
				
				map<EncapFSA::State*,EncapFSA::State*> ms;

				map<SymbolProto*,EncapFSA::State*>::iterator newTo = newStates.find(label.second);
				EncapFSA::State *newState = (*newTo).second; //new destination state of the transition

				EncapStIterator_t oldState = (*aux)->ToState(); //old destination state of the transition
				
				ms[&(*oldState)] = newState;
				
				EncapFSA::ExtendedTransition *et = (*aux)->getIncludingET();
				et->changeGateRefs(NULL,NULL,ms);

				EncapStIterator_t newToIt = fsa->GetStateIterator(newState); //iterator of the new destination state of the transition
				EncapStIterator_t fromState = (*aux)->FromState();	//starting state of the transitions (old and new)
				EncapTrIterator_t newTransIt = fsa->AddTrans(fromState, newToIt, labels, false, NULL);
				EncapTrIterator_t oldTransIt = fsa->GetTransIterator(*aux);
				(*newTransIt).setIncludingET(et); //set the new transition as part of the extended transition
				et->changeEdgeRef(oldTransIt, newTransIt);
			}
			//remove the transition
			(*aux)->RemoveEdge();
			delete(*aux);
  		
      	}//end iteration over the incoming transitions
      	
      	//since this state is the initial state of the automaton, it must be replaced also by the a state related to startproto, which must
     	//be set as new initial state
      	if((*s).IsInitial())
      	{
      		EncapGraph::GraphNode &startNode = m_Graph.GetStartProtoNode();
  			SymbolProto *startProto = startNode.NodeInfo;
  			EncapStIterator_t state = this->AddState(*fsa, startNode, true);
  			newStates.insert(pair<SymbolProto*,EncapFSA::State*>(startProto,&(*state)));
  			fsa->SetInitialState(state);
      	}

		/************OUTGOING*********************/
		
		//iterate over the outgoing transitions, including the eventual self loop. 
		//For each symbol, store the starting state. remove the transition.
		//if the transition is part of an extended transition, simply change the starting state with the right new state
		
		set<EncapFSA::ExtendedTransition*> etToRemove;
		list<EncapFSA::Transition*> transitionOut = fsa->getTransExitingFrom(&(*s));
		for(list<EncapFSA::Transition*>::iterator t = transitionOut.begin(); t != transitionOut.end();)		
		{
			Alphabet_t labels = (*t)->GetInfo().first;
			if((*t)->getIncludingET())
			{
				//this outgoing transition is part of an extended transition
				//we change the starting state of the extended transition
				nbASSERT(labels.size()==1,"An extended transition with more than one label?");
				EncapLabel_t label = *labels.begin();
								
				EncapFSA::ExtendedTransition *et = (*t)->getIncludingET();
								
				map<SymbolProto*,EncapFSA::State*>::iterator newFrom = newStates.find(label.first); //new starting state of the transition
				
				if(newFrom != newStates.end())
				{
					//a new state for the starting protocol of the transition exists
					EncapFSA::State *newState = (*newFrom).second;
				
					map<EncapFSA::State*,EncapFSA::State*> ms;//needed to compile the c++ code :D	
					et->changeGateRefs(NULL,newState,ms);
				
					EncapStIterator_t destination = (*t)->ToState();
					EncapStIterator_t source = fsa->GetStateIterator(newState);
					EncapTrIterator_t newTransIt = fsa->AddTrans(source, destination, label, false, NULL);
					(*newTransIt).setIncludingET(et);
				
					EncapTrIterator_t oldTransIt = fsa->GetTransIterator(*t);
				
					et->changeEdgeRef(oldTransIt, newTransIt);
				}
				else
					//there is not a new starting state for this extended transition, then it must be removed
					etToRemove.insert(et);
			}
			else
			{		
				//iterate over the labels, and store each label in labelToState with the destination state of the transition
				for (Alphabet_t::iterator l = labels.begin();l != labels.end();++l)
					labelToState.insert(pair<EncapLabel_t,EncapFSA::State*>(*l,&(*(*t)->ToState())));
			}

			//remove the transition
			list<EncapFSA::Transition*>::iterator aux = t;
			t++;
			(*aux)->RemoveEdge();
			delete(*aux);
		
		}//end iteration over the outgoing transitions
		
		//remove the useless exiting extended transitions
		for(set<EncapFSA::ExtendedTransition*>::iterator etIt = etToRemove.begin(); etIt != etToRemove.end(); )
		{
			set<EncapFSA::ExtendedTransition*>::iterator auxEt = etIt;
			etIt++;
			delete(*auxEt);
		}	
		
		
      	//the current state has been expanded
     	//now we have to put the transitions of the old state on the right new state
     	ReplaceTransitions(fsa,labelToState,labelFromState,onSelfLoop,newStates);
      	
      	//remove the state
     	EncapFSA::StateIterator sAux = s;
     	s++;
     	fsa->removeState(&(*sAux));
     	
     	PRINT_DOT(fsa, "fixautomaton - after an expansion loop", "regexpfsa_expansion_loop");
	}//end iteration over the states
	return RemoveUselessStates(fsa,fail);
}

//Create a new state for the second protocol of the label, if it is not exist yet
void EncapFSABuilder::CreateNewStateIfNeeded(EncapFSA *fsa, map<SymbolProto*,EncapFSA::State*> *newStates, EncapLabel_t label)
{
	if(newStates->count(label.second)==0)
 	{
		//a new state related to this protocol does not exist
		EncapGraph::GraphNode *node = m_Graph.GetNode(label.second);
		EncapStIterator_t state = this->AddState(*fsa, *node, true);
		newStates->insert(pair<SymbolProto*,EncapFSA::State*>(label.second,&(*state)));
  	}
}	

/*
*	link the new states to the rest of the automaton, using the incoming and outgoing transitions of the state that has been replaced
*/
void EncapFSABuilder::ReplaceTransitions(EncapFSA *fsa,map<EncapLabel_t,EncapFSA::State*> labelToState,map<int,map<EncapLabel_t,EncapFSA::State*> > labelFromState,Alphabet_t onSelfLoop,map<SymbolProto*,EncapFSA::State*> newStates)
{
	//replace the selfloop - except the extended transitions
	for (Alphabet_t::iterator l = onSelfLoop.begin();l != onSelfLoop.end();++l)
	{
		SymbolProto *fromProto = (*l).first;
		SymbolProto *toProto = (*l).second;
		map<SymbolProto*,EncapFSA::State*>::iterator from = newStates.find(fromProto);
		map<SymbolProto*,EncapFSA::State*>::iterator to = newStates.find(toProto);
		if(from != newStates.end() && to != newStates.end())
		{
			//the transition must me put in the automaton
			EncapStIterator_t fromState = fsa->GetStateIterator((*from).second);
			EncapStIterator_t toState = fsa->GetStateIterator((*to).second);
			fsa->AddTrans(fromState, toState, *l, false, NULL);
		}
	}
	
	//replace the incoming transitions - except the extended transitions
	map<int,map<EncapLabel_t,EncapFSA::State*> >::iterator superIt = labelFromState.begin();
	for(; superIt != labelFromState.end(); superIt++)
	{
		map<EncapLabel_t,EncapFSA::State*> currentLabelFromState = (*superIt).second;
	
		map<EncapLabel_t,EncapFSA::State*>::iterator fr = currentLabelFromState.begin();
		for(; fr != currentLabelFromState.end(); fr++)
		{
			EncapLabel_t label = (*fr).first;
			map<SymbolProto*,EncapFSA::State*>::iterator to = newStates.find(label.second);
			if(to != newStates.end())
			{
				//the transition must be put on the automaton
				EncapStIterator_t fromState = fsa->GetStateIterator((*fr).second);
				EncapStIterator_t toState = fsa->GetStateIterator((*to).second);
				fsa->AddTrans(fromState, toState, label, false, NULL);
			}
		}
	}
	
	//replace the outgoing transitions - except the extended transitions
	map<EncapLabel_t,EncapFSA::State*>::iterator to = labelToState.begin();
	for(; to != labelToState.end(); to++)
	{
		EncapLabel_t label = (*to).first;
		map<SymbolProto*,EncapFSA::State*>::iterator from = newStates.find(label.first);
		if(from != newStates.end())
		{
			//the transition must be put on the automaton
			EncapStIterator_t fromState = fsa->GetStateIterator((*from).second);
			EncapStIterator_t toState = fsa->GetStateIterator((*to).second);
			fsa->AddTrans(fromState, toState, label, false, NULL);
		}
	}
	
}

bool EncapFSABuilder::FindTraps(EncapFSA *fsa,EncapFSA::State *fail, status_t status)
{
	bool again = false; //if at the end of the method this variable is true, the method is executed again

	//perform a sort reverse postorder visit of the automaton, in order to mark the useful states
	if(status == ACCEPTING_STATE)
		list<EncapFSA::State*> stateList = fsa->SortRevPostorder();
	else
		//status == ACTION_STATE
		list<EncapFSA::State*> stateList = fsa->SortRevPostorderAction();
	
	//iterate over the states and remove te ones not marked
	EncapFSA::StateIterator s = fsa->FirstState();
	for (; s != fsa->LastState();)
	{
		if((*s).HasBeenVisited())// or HasIncomingExtendedTransition(fsa,&(*s)))
		{
			//this state has been visited
			s++;
			continue;
		}
		
		if((*s).isFinal() && !(*s).isAccepting())
		{
			//this is the final not accepting state
			s++;
			continue;
		}
			
		again = true;
			
		//the current state must be removed
		
		EncapFSA::StateIterator aux = s;
		s++;
		
		//remove the outgoing transitions of the state
		list<EncapFSA::Transition*> transitionOut = fsa->getTransExitingFrom(&(*aux));
		CutOutgoingTransitions(fsa, &(*aux));
		
		//remove the incoming transitions, except the extended which are redirected to the final not accepting state
		CutIncomingTransitions(fsa,&(*aux),fail);
		
		//remove the state
		fsa->removeState(&(*aux));
	}

	RemoveUselessStates(fsa,fail);
	
	return (again)? FindTraps(fsa,fail,status) : true;

}

void EncapFSABuilder::LinkToNotAccept(EncapFSA *fsa,EncapFSA::State *fail)
{
	
	EncapStIterator_t finalNotAccept = fsa->GetStateIterator(fail);
	
	//iterate over the states of the automaton
	EncapFSA::StateIterator s = fsa->FirstState();
	for (; s != fsa->LastState();s++)
	{
		if((*s).isFinal() || (*s).isAccepting())
			//the final accepting state and the final not accepting state have not the transition toward the final not accepting state
			continue;
			
		Alphabet_t usedLabels;
		//iterate over the outgoing transitions of the current state
		list<EncapFSA::Transition*> transitionOut = fsa->getTransExitingFrom(&(*s));
		for(list<EncapFSA::Transition*>::iterator t = transitionOut.begin(); t != transitionOut.end();t++)		
		{
			Alphabet_t labels = (*t)->GetInfo().first;
			for (Alphabet_t::iterator l = labels.begin();l != labels.end();++l)
				usedLabels.insert(*l);	
		} //end iteration over the outgoing transitions
		
		//link the current state and the final not accepting state
		EncapStIterator_t state = fsa->GetStateIterator(*s);
		fsa->AddTrans(state, finalNotAccept, usedLabels, true, NULL);	
		
	}//end iteration over the states
}

void EncapFSABuilder::CreateNoMatchAutomaton(EncapFSA *fsa)
{
	//remove each transition and each state of the automaton, except the initial state
	EncapFSA::StateIterator s = fsa->FirstState();
	for (; s != fsa->LastState();)
	{			
		//remove the outgoing transitions of the state	
		CutOutgoingTransitions(fsa, &(*s));
			
		//remove the state
		EncapFSA::StateIterator sAux = s;
		s++;
		if(!(*sAux).IsInitial())
			fsa->removeState(&(*sAux));
	}//end iteration over the states
	
	EncapFSA::StateIterator sIt = fsa->FirstState();
	for (; sIt != fsa->LastState();sIt++)
	{
		//the automaton has now only one state
		if((*sIt).GetInfo() == NULL)
		{
			EncapGraph::GraphNode &startNode = m_Graph.GetStartProtoNode();
  			SymbolProto *startProto = startNode.NodeInfo;
  			(*sIt).SetInfo(startProto);
		}

		//create final not accepting state
	//	EncapStIterator_t finalNotAccept = fsa->AddState(NULL);
		EncapStIterator_t state = fsa->GetStateIterator(*sIt);
		fsa->resetAccepting(state);
		fsa->setFinal(state);
		//add the self loop on the final not accepting state
		Alphabet_t empty;
//		fsa->AddTrans(finalNotAccept, finalNotAccept, empty, true, NULL);
				
		//link the initial state and the final not accepting state
//		EncapStIterator_t state = fsa->GetStateIterator(*sIt);
		fsa->AddTrans(state, state, empty, true, NULL); 
		
		break;
	}
	
	PRINT_DOT(fsa,"fixautomaton - end","regexpfsa_without_accept_and_action_fixautomaton");
}

/*
*  FSA of a single node associated with startproto
*/
EncapFSA *EncapFSABuilder::BuildSingleNodeFSA()
{
	//this automaton is built when the filtering expression has not been specified
	//initial accepting state
	EncapFSA *fsa = new EncapFSA(m_Alphabet);
	EncapGraph::GraphNode &startNode = m_Graph.GetStartProtoNode();
	EncapStIterator_t newState = this->AddState(*fsa, startNode, false);   
	fsa->SetInitialState(newState); 
	fsa->setAccepting(newState); 
	fsa->resetFinal(newState); 
	EncapFSA::Alphabet_t empty;
	fsa->AddTrans(newState, newState, empty, true, NULL);	
	
	//this automaton hasn't the final not accepting state
	
	PRINT_DOT(fsa, "before BuildSingleNodeFSA return", "BuildSingleNodeFSA");     
	return fsa;
}

/*
*	This method remove all those states which are on paths that do not lead
*	to an accepting state. The only exception is the final not accepting state,
*	which is not removed.
*	The method is extremely useful in case of field extraction. In fact usually
*	the complete automaton has a lot of useless states.
*	Even if they are not considered in the code generation anyway, I remove them
*	in order to make clearer the print of the FSA, and also to reduce the data 
*	constituting the automaton.
*/
void EncapFSABuilder::ReduceAutomaton(EncapFSA *fsa)
{
	EncapFSA::StateList_t accepting = fsa->GetAcceptingStates();
	if(accepting.size()==0)
		//No optimization is neede
		return;
	
	list<EncapFSA::State*> stateList = fsa->SortRevPostorder();
	
	//Get the a state to redirect the et leading to removed states,
	//i.e. the final not accepting state if it exists. Otherwise we 
	//are doing field extraction without any filtering condition, and
	//no reduction is possible (since is state accepts the packet).
	EncapFSA::State *well = NULL;
	EncapFSA::StateList_t states = fsa->GetFinalStates();
	for(EncapFSA::StateList_t::iterator it = states.begin(); it != states.end(); it++)
	{	
		if(!(*it)->isAccepting())
		{
			well = *it;
			break;
		}
	}
	
	if(well == NULL)
		return;
		
	//iterate over the states and remove te ones not marked
	EncapFSA::StateIterator s = fsa->FirstState();
	for (; s != fsa->LastState();)
	{
		if((*s).HasBeenVisited() || ( &(*s) == well))
		{
			//this state has been visited
			s++;
			continue;
		}
				
		//the current state must be removed
		EncapFSA::StateIterator stateToRemove = s;
		s++;

		//remove the outgoing transitions of the state
		set<EncapFSA::ExtendedTransition*> etToRem;
		list<EncapFSA::Transition*> transitionOut = fsa->getTransExitingFrom(&(*stateToRemove));
		for(list<EncapFSA::Transition*>::iterator t = transitionOut.begin(); t != transitionOut.end();)
		{
			list<EncapFSA::Transition*>::iterator trans = t;
			t++;
	   		//store the eventual related extended transition
	  		if((*trans)->getIncludingET())
	  			etToRem.insert((*trans)->getIncludingET());
	  		(*trans)->RemoveEdge();
			delete(*trans);
	  	}
	  	//remove the extended transitions, if needed
		for(set<EncapFSA::ExtendedTransition*>::iterator it = etToRem.begin(); it != etToRem.end();)
		{
			set<EncapFSA::ExtendedTransition*>::iterator aux = it;
			it++;
			delete(*aux);	
		}

		//remove the incoming transitions except the extended, which are redirected to the final not accepting state
		CutIncomingTransitions(fsa,&(*stateToRemove),well);

		//remove the state
		fsa->removeState(&(*stateToRemove));

	}
}
