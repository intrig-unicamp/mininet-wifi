/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


// This file defines the exported functions, which may be imported by
// other modules within the NetBee library that cannot use the C++ exports
// So, we export here only the functions, which can be used also from 
// other C modules

uint64_t nbProfilerGetTime();
int64_t nbProfilerGetMeasureCost();
