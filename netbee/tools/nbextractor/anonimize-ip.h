/*
 * Copyright (c) 2002 - 2011
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition
 * is met:
 *
 * Neither the name of the Politecnico di Torino nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <nbee.h>

#include "configparams.h"

#include <bitset>
#include <iostream>
#include <fstream>
#include <sstream>
#if (defined(_WIN32) || defined(_WIN64))
#include <unordered_map>
#include <unordered_set>
#else
#include <arpa/inet.h>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#endif


#define ADDR_LEN 4
#define MAX_LINE 1024


#if (defined(_WIN32) || defined(_WIN64))
typedef unordered_map<string, string> AnonymizationMapTable_t;
typedef unordered_set<int> AnonymizationArgumentList_t;
#else
typedef std::tr1::unordered_map<string, string> AnonymizationMapTable_t;
typedef std::tr1::unordered_set<int> AnonymizationArgumentList_t;
#endif


int InitializeIPAnonymizer(AnonymizationMapTable_t &AnonymizationIPTable, AnonymizationArgumentList_t &AnonymizationIPArgumentList, _nbExtractFieldInfo *DescriptorVector, char* ErrBuf, int ErrBufSize);

void anonGenAllMappings(AnonymizationMapTable_t &ip_mappings, char *ip_address, int netmask);
