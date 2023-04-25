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


#include "anonimize-ip.h"
#include <nbsockutils.h>
#include "../utils/utils.h"

// Global variable for configuration
extern ConfigParams_t ConfigParams;


int InitializeIPAnonymizer(AnonymizationMapTable_t &AnonymizationIPTable, AnonymizationArgumentList_t &AnonymizationIPArgumentList,
							_nbExtractFieldInfo *DescriptorVector, char* ErrBuf, int ErrBufSize)
{
	// Check and initialize list of arguments to be anonymized
	char *tokenizer= strtok(ConfigParams.IPAnonFieldsList, ",");
	while (tokenizer != NULL)
	{
		AnonymizationIPArgumentList.insert(atoi(tokenizer));
		tokenizer= strtok(NULL, ",");
	}

	// Check if anonymized arguments belong to the correct type
	for (AnonymizationArgumentList_t::iterator j = AnonymizationIPArgumentList.begin(); j != AnonymizationIPArgumentList.end(); j++)
	{
		int index= *j - 1;
		if ( strcmp(DescriptorVector[index].Proto, "ip") != 0 ||
			( strcmp(DescriptorVector[index].Name, "dst") != 0 &&
			strcmp(DescriptorVector[index].Name, "src") != 0 ) )
		{
			ssnprintf(ErrBuf, ErrBufSize, (char *) "Warning: anonymization must be performed on IP addresses!");
			return nbFAILURE;
		}
	}

	string line;
	char range[MAX_LINE];
	char ip_address[MAX_LINE];
	int netmask;

	// Read IP anonymization ranges file
	ifstream fp (ConfigParams.IPAnonFileName);
	if (!fp.is_open())
	{
		ssnprintf(ErrBuf, ErrBufSize, (char *) "Error while loading IP ranges file, aborting.");
		return nbFAILURE;
	}

	while (getline(fp, line) != NULL)
	{
		// ignore empty lines
		if (line.size() == 0)
			continue;

		// ignore comments
		if (line.at(0) == '#')
			continue;

		// read ip range
		if (sscanf(line.c_str(), "%s", range) == 1)
		{
			// file input format is <ip/netmask>, i.e. 192.168.1.0/24
			char *tokenizer = strtok(range, "/");
			if (tokenizer != NULL)
			{
				strncpy(ip_address, tokenizer, MAX_LINE);

				tokenizer = strtok(NULL, "/");
				if (tokenizer != NULL)
					netmask = atoi(tokenizer);
				else
				{
					ssnprintf(ErrBuf, ErrBufSize, (char *) "Error while parsing IP ranges file, aborting.");
					return nbFAILURE;
				}
			}
			else
			{
				ssnprintf(ErrBuf, ErrBufSize, (char *) "Error while parsing ip ranges file, aborting.");
				return nbFAILURE;
			}

#ifdef _DEBUG_LOUD
			fprintf(stderr, "%s/%d\n", ip_address, netmask);
#endif

#ifdef PROFILING
                        fprintf(stderr, "  Loop for %s/%d\n", ip_address, netmask);
#endif
                        anonGenAllMappings(AnonymizationIPTable, ip_address, netmask);
		}
	}

	return nbSUCCESS;
}

void anonGenAllMappings(AnonymizationMapTable_t &ip_mappings, char *ip_address, int netmask)
{
  // the number of IP addresses in the requested range
  const int num_of_addresses = (1 << (ADDR_LEN*8 - netmask));

  // generate the XOR mask first, excluding the one with all zeroes, because is not useful
  const uint32_t mask = 1 + rand() % ( num_of_addresses - 1 );

  // compute the first IP address in range
  uint32_t ip_addr;
  sockaddr_storage tmp_ip;
  if (sock_present2network(ip_address, &tmp_ip, AF_INET, NULL, 0) == sockFAILURE) {
    fprintf(stderr, "Error while converting ip address!\n");
    return;
  }
  memcpy(&ip_addr, &(((sockaddr_in *) &tmp_ip)->sin_addr), sizeof(ip_addr));
  ip_addr = ntohl(ip_addr); // handled in host order to ease the following loop

#ifdef PROFILING
  fprintf(stderr, "\tGenerating all IP addresses in range and their anonymized version... ");
  uint64_t before = nbProfilerGetMicro();
#endif

  // while looping on all possible addresses in the provided range...
  for(int i = 0; i < num_of_addresses; ++i, ++ip_addr) {
    // ... mask them and add to the map
    const uint32_t ip_orig_bin = htonl(ip_addr);
    const uint32_t ip_anon_bin = htonl(ip_addr ^ mask);
    char ip_orig[MAX_LINE];
    char ip_anon[MAX_LINE];

#ifdef _WIN32
    // inet_ntop() does not exist prior Vista; using this workaround, which
    // works properly only in case of IPv4 addresses
    ssnprintf(ip_orig, sizeof(ip_orig), "%d.%d.%d.%d",
              *((unsigned char*)&ip_orig_bin),
              *( ((unsigned char*)&ip_orig_bin) + 1 ),
              *( ((unsigned char*)&ip_orig_bin) + 2 ),
              *( ((unsigned char*)&ip_orig_bin) + 3 )
              );
    ssnprintf(ip_anon, sizeof(ip_anon), "%d.%d.%d.%d",
              *((unsigned char*)&ip_anon_bin),
              *( ((unsigned char*)&ip_anon_bin) + 1 ),
              *( ((unsigned char*)&ip_anon_bin) + 2 ),
              *( ((unsigned char*)&ip_anon_bin) + 3 )
              );
#else
    inet_ntop(AF_INET, &ip_orig_bin, ip_orig, MAX_LINE);
    inet_ntop(AF_INET, &ip_anon_bin, ip_anon, MAX_LINE);
#endif
    ip_mappings[ string(ip_orig) ] = string(ip_anon);
  }

#ifdef PROFILING
  uint64_t after = nbProfilerGetMicro();
  fprintf(stderr, "done in %lu usecs\n", after-before, ip_mappings.size());
#endif
}
