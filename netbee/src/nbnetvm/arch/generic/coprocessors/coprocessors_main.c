/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



/** @file coprocessors_main.c
 *	\brief This file contains main structures of coprocessors implementation
 */

#include "coprocessor.h"

int32_t nvmCoproLookupEx_Create(nvmCoprocessorState *lookup);
int32_t nvmCoproLookupNewCreate(nvmCoprocessorState *lookup);
int32_t nvmCoproRegexpCreate(nvmCoprocessorState *regexp);
int32_t nvmCoproStringmatchCreate(nvmCoprocessorState *stringmatch);
int32_t nvmCoproLookupCreate(nvmCoprocessorState *lookup);
int32_t nvmCoproAssociative_Create(nvmCoprocessorState *associative);

/*
	XXX
	Whenever you modify the coprocessor map, whether you add or remove or 
	change name to a processor, please also update the nvm_copro_map in

	nbnetvm/arch/octeon/octeon_coprocessors.c
	nbnetvm/jit/octeon/octeon-coprocessor.cpp

	the same number of coprocessors must be defined in all these structures,
	moreover in the same order, as some information about coprocessor is passed
	around using the index in this map.

	Probably a more centralized way should be found to define this map, or a
	test should be set up at compilation time to ensure that everything is ok.
	For the time being let's live with this.
*/

nvmCoprocessorStateMapEntry nvm_copro_map[] = {
/*
	Lookup coprocessors are a little bit a mess. We have three because:
	- the original one is still used by out NetVMSnort
	- the new (and better) one is lookupnew. which is more efficient and has
	  a completely different way to be programmed (e.g. it does not require
	  the incremental loading of the key)
	- lookup_ex was used by Davide Schiera in his experimental implementation
	  of the application-layer filtering, because he needed several lookup
	  tables. Since you cannot instantiate multiple coprocessors at the same
	  time, this was the trick used.
	  
	Obviously, we should reduce the coprocessors to one only, sooner or later...
*/
	{"lookup", nvmCoproLookupCreate},
	{"lookupnew", nvmCoproLookupNewCreate},
	{"lookup_ex", nvmCoproLookupEx_Create},
	{"regexp", nvmCoproRegexpCreate},
	{"stringmatching", nvmCoproStringmatchCreate}
};


