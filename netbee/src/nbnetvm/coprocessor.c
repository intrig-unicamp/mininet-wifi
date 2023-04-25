#include "nbnetvm.h"
#include "coprocessor.h"

/* The following are standard coprocessor register I/O functions that can be used by coprocessor implementations if they
 * do not need anything fancy. */
int32_t nvmCoproStandardRegWrite (nvmCoprocessorState *c, uint32_t reg, uint32_t *val) {
	(c -> registers)[reg] = *val;

	return (nvmSUCCESS);
}


int32_t nvmCoproStandardRegRead (nvmCoprocessorState *c, uint32_t reg, uint32_t *val) {
	*val = (c -> registers)[reg];

	return (nvmSUCCESS);
}
