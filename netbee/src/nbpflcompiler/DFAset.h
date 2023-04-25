/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*
 * DFAset.h
 *
 *  Created on: 27-lug-2009
 *      Author: Nik
 *	Modified on 14/23/24/26-lug-2012 - 01-ago-2012
 *		Author: Ivano
 */

#include "encapfsa.h"

class DFAset
{

private:
	EncapFSA::Alphabet_t symbols;
	SymbolProto *info;
	int id;
    static int next_id;
	std::map<uint32,EncapFSA::State*> states;

    /* The following variable aims at obtaining one useful
     * optimization: if 'states' includes at least one accepting
     * final state, or one action final state,
     * store it in this variable, as the whole DFAset
     * can be compacted into it (and just return it if asked by
     * the outside world).
     */
    EncapFSA::State* states_acceptingfinal;
    EncapFSA::State* states_actionfinal;

public:

 	DFAset()
 		:states()
	{
		this->info = NULL;
		this->id = next_id++;
        this->states_acceptingfinal = NULL;
        this->states_actionfinal = NULL;

	}

	DFAset(const DFAset &other) : symbols(other.symbols), info(other.info),
          id(other.id), states(other.states),
          states_acceptingfinal(other.states_acceptingfinal),
          states_actionfinal(other.states_actionfinal) {}

        DFAset(std::set<EncapFSA::State*> stateset)
        {
          this->info = NULL;
          this->id = next_id++;
          this->states_acceptingfinal = NULL; // default to NULL
          this->states_actionfinal = NULL; // default to NULL
          
          for (std::set<EncapFSA::State*>::iterator i = stateset.begin();
               i != stateset.end();
               ++i)
            // AddState will take care of setting 'states_acceptingfinal' and/or 'states_actionfinal'
            this->AddState(*i);
	}

    DFAset(DFAset *a, DFAset *b)
    {
      this->info = NULL;
      this->id = next_id++;

      this->states.insert(a->states.begin(), a->states.end());
      this->states.insert(b->states.begin(), b->states.end());

      nbASSERT(a->states_acceptingfinal == NULL ||
               b->states_acceptingfinal == NULL ||
               a->states_acceptingfinal == b->states_acceptingfinal,
               "different DFAsets have different final accepting states?");
      this->states_acceptingfinal = (a->states_acceptingfinal ?
                                     a->states_acceptingfinal :
                                     b->states_acceptingfinal );
                                     
      this->states_actionfinal = (a->states_actionfinal ?
      							  a->states_actionfinal :
      							  b->states_actionfinal );
    }

	SymbolProto* GetInfo(void)
	{
		return this->info;
	}

	int GetId(void)
	{
		return this->id;
	}

	/*
	* Resolve internally what is the stateInfo shared among the states, if any.
	* Return it.
	*/
    SymbolProto *AutoSetInfo(void)
    {
      // try and use the information from the states currently inside the set, if consistent
      std::map<uint32,EncapFSA::State*>::iterator s = this->states.begin();
      SymbolProto *tmp_info = NULL;
      for(; s != this->states.end(); ++s) {
        EncapFSA::State *s_ptr = s->second;

	  //FIXME ivano: is it useful?
      //  if (s_ptr->GetInfo() == NULL) 
      //    continue; // skip useless states

        if (tmp_info == NULL)
          tmp_info = s_ptr->GetInfo();
        else if (tmp_info != s_ptr->GetInfo())
          return NULL; // conflict found, abort
      }

      this->info = tmp_info;
      return tmp_info;
}

	std::map<uint32,EncapFSA::State*> GetStates(void)
	{
         if (states_acceptingfinal != NULL) 
         {
            std::map<uint32,EncapFSA::State*> tmp;
            tmp[states_acceptingfinal->GetID()] = states_acceptingfinal;
            return tmp;
         }
          
         if (states_actionfinal != NULL)
         {
	        std::map<uint32,EncapFSA::State*> tmp;
            tmp[states_actionfinal->GetID()] = states_actionfinal;
            return tmp;
         } 
         return this->states;
	}

	EncapFSA::Alphabet_t GetSymbols(void)
	{
		return this->symbols;
	}

	void AddState(EncapFSA::State *s)
	{
		std::pair<uint32, EncapFSA::State*> p = make_pair<uint32, EncapFSA::State*>(s->GetID(), s);
		states.insert(p);

        if (states_acceptingfinal == NULL && s->isAccepting() && s->isFinal() )
          	states_acceptingfinal = s;
        
        if (states_actionfinal == NULL && s->isAction() && s->isFinal() )
          	states_actionfinal = s;
	}

	void AddSymbol(EncapLabel_t s)
	{
		if(!symbols.contains(s))
		{
			symbols.insert(s);
		}
	}

        /*
	void parseAcceptingStatus(void)
	{
		std::map<uint32,EncapFSA::State*>::iterator iter = (this->states).begin();
		while(iter != (this->states).end())
		{
			if((*iter).second->IsAccepting())
			{
				this->Accepting = true;
				this->notAccepting = false;
			}
			iter++;
		}
	}
        */

	bool isAccepting(void)
	{
          if(states_acceptingfinal)
            return true;

          std::map<uint32,EncapFSA::State*>::iterator iter = (this->states).begin();
          while(iter != (this->states).end())
            {
              if((*iter).second->isAccepting())
                return true;
              iter++;
            }
          return false;
	}
	
	bool isAction(void)
	{
		if(states_actionfinal)
			return true;
	
		std::map<uint32,EncapFSA::State*>::iterator iter = (this->states).begin();
        while(iter != (this->states).end())
        {
        	if((*iter).second->isAction())
        		return true;
            iter++;
        }
        return false;
	}
	
	list<SymbolField*> GetExtraInfo()
	{
		nbASSERT(isAction(),"There is a BUG!");
		list<SymbolField*> fields;
		std::map<uint32,EncapFSA::State*>::iterator iter = (this->states).begin();
        while(iter != (this->states).end())
        {
        	if((*iter).second->isAction())
        	{
        		list<SymbolField*> s = (*iter).second->GetExtraInfo();
        		for(list<SymbolField*>::iterator sf = s.begin(); sf != s.end(); sf++)
        			fields.push_back(*sf);
        	}
            iter++;
        }
        
        return fields;
	}

	list<uint32> GetExtraInfoPlus()
	{
		nbASSERT(isAction(),"There is a BUG!");
		list<uint32> positions;
		std::map<uint32,EncapFSA::State*>::iterator iter = (this->states).begin();
        while(iter != (this->states).end())
        {
        	if((*iter).second->isAction())
        	{
        		list<uint32> s = (*iter).second->GetExtraInfoPlus();
        		for(list<uint32>::iterator sf = s.begin(); sf != s.end(); sf++)
        			positions.push_back(*sf);
        	}
            iter++;
        }
        
        return positions;
	}
	
	list<bool> GetOtherInfo()
	{
		nbASSERT(isAction(),"There is a BUG!");
		list<bool> multifield;
		std::map<uint32,EncapFSA::State*>::iterator iter = (this->states).begin();
        while(iter != (this->states).end())
        {
        	if((*iter).second->isAction())
        	{
        		list<bool> s = (*iter).second->GetOtherInfo();
        		for(list<bool>::iterator sf = s.begin(); sf != s.end(); sf++)
        			multifield.push_back(*sf);
        	}
            iter++;
        }
        
        return multifield;
	}

	bool isAcceptingFinal(void)
	{
          return (states_acceptingfinal != NULL);
	}
	
	bool isActionFinal(void)
	{
		return (states_actionfinal != NULL);
	}
	

	//ivano: added the check on the action status of a state
	int32 CompareTo(const DFAset &other) const
	{	
		if (this->states_acceptingfinal != NULL && other.states_acceptingfinal != NULL && this->states_acceptingfinal == other.states_acceptingfinal)
            return 0;

		if (this->states_actionfinal != NULL && other.states_actionfinal != NULL && this->states_actionfinal == other.states_actionfinal)
            return 0;

		std::map<uint32,EncapFSA::State*> s2 = other.states;
		std::map<uint32,EncapFSA::State*> s1 = this->states;

        for (std::map<uint32,EncapFSA::State*>::iterator is2 = s2.begin();is2 != s2.end();++is2)
		{
			// optimization: ignore final-non-accepting or final-non-action states when doing this comparison, as
			// they bring no difference to those DFAsets
			if((*is2).second->isFinal() && !(*is2).second->isAccepting() && !(*is2).second->isAction())
				continue;

			if(s1.find((*is2).first) == s1.end())
				return -1;
		}

		for(std::map<uint32,EncapFSA::State*>::iterator is1 = s1.begin();is1 != s1.end();++is1)
		{
			// optimization: ignore final-non-accepting or final-non-action states when doing this comparison, as
			// they bring no difference to those DFAsets
		  	if((*is1).second->isFinal() && !(*is1).second->isAccepting() && !(*is1).second->isAction())
				continue;

			if(s2.find((*is1).first) == s2.end())
				return 1;
		}
		return 0;
	}

};

int DFAset::next_id = 1;
