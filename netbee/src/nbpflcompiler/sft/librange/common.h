/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

/* == ancillary declarations == */
enum RangeOperator_t
  {
    LESS_THAN = 0,
    NOT_EQUAL,
    LESS_EQUAL_THAN,
    GREAT_THAN,
    GREAT_EQUAL_THAN,
    EQUAL,
    MATCH,
    CONTAINS,
    INVALID = -1
  };

#endif /* COMMON_H_INCLUDED */
