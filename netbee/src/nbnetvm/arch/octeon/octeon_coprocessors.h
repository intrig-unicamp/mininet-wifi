#ifndef OCTEON_COPROCESSORS_H
#define OCTEON_COPROCESSORS_H

#include "../../coprocessor.h"

int32_t OcteonCoproRegexpCreate(nvmCoprocessorState *regexp);
int32_t nvmCoproLookupEx_Create(nvmCoprocessorState *lookup);
int32_t nvmCoproStringmatchCreate(nvmCoprocessorState *stringmatch);
int32_t nvmCoproLookupCreate(nvmCoprocessorState *lookup);

extern nvmCoprocessorStateMapEntry nvm_copro_map[AVAILABLE_COPROS_NO];

#endif

