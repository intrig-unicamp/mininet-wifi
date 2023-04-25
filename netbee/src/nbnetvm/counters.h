#ifndef COUNTERS_H
#define COUNTERS_H

#include <nbnetvm_bittypes.h>

#ifdef __cplusplus
extern "C" {
#endif
extern u_int32_t n_pkt_check;
extern u_int32_t n_info_check;
extern u_int32_t n_data_check;
extern u_int32_t n_copro_check;
extern u_int32_t n_stack_check;
extern u_int32_t n_initedmem_check;
extern u_int32_t n_jump_check;
extern u_int32_t n_instruction;
#ifdef __cplusplus
}
#endif

#endif // COUNTERS_H