/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include <string>

using namespace std;



int32 IPaddr2int(const char *addr, uint32 *num);

int32 MACaddr2int(const char *addr, uint32 *num);

int32 str2int(const char *s, uint32 *num, uint8 base);

string int2str(const uint32 num, uint8 base);

