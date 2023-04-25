#include "octeon_coprocessors.h"

nvmCoprocessorStateMapEntry nvm_copro_map[AVAILABLE_COPROS_NO] = {
	{"lookup",  nvmCoproLookupCreate},
	{"lookupnew", NULL}, /*not available for octeon, AFAIK*/
	{"lookup_ex", nvmCoproLookupEx_Create},
	{"regexp", OcteonCoproRegexpCreate },
	{"stringmatching", nvmCoproStringmatchCreate}
};


