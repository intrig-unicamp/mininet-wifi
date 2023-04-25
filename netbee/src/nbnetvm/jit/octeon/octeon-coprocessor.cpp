/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "octeon-coprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <sstream>


#ifdef WIN32
#undef snprintf
#define snprintf _snprintf
#endif

using namespace std;

namespace jit {
	namespace octeon {
		int32_t nvmOcteonRegExpCoproInjectData (nvmCoprocessorState *c, uint8_t *data);
		int32_t nvmOcteonStringMatchCoproInjectData (nvmCoprocessorState *c, uint8_t *data);
		int32_t control_special_char(char **buffer,uint8_t *data,uint16_t pattern_length);
		
	}
}

jit::octeon::copro_map_entry jit::octeon::octeon_copro_map[AVAILABLE_COPROS_NO] = {
	{"lookup", NULL},
	{"lookupnew", NULL},
	{"lookup_ex", NULL},
	{"regexp", (nvmCoproInitFunct *)jit::octeon::nvmOcteonRegExpCoproInjectData},
	{"stringmatching",(nvmCoproInitFunct *)jit::octeon::nvmOcteonStringMatchCoproInjectData}
};

#define _SIZE_DB (sizeof (uint8_t))
#define _SIZE_DW (sizeof (uint16_t))
#define _SIZE_DD (sizeof (uint32_t))

int32_t jit::octeon::nvmOcteonRegExpCoproInjectData (nvmCoprocessorState *c, uint8_t *data)
{
uint16_t g = 0, patterns_no = 0, pattern_length = 0, flags_length = 0;
int32_t out;
char flags[20], *p;
FILE *regexp;

	patterns_no = *(uint16_t *) data;
	data += _SIZE_DW;

	regexp = fopen("regexp","w");
	if(regexp == NULL)
	{
		fprintf(stderr, "Error opening file\n");
		return nvmFAILURE;
	}

	fprintf(regexp,"{.=[^\\n]}\n");

	for (g = 0; g < patterns_no; g++) {
		flags_length = *(uint16_t *) data;
		data += _SIZE_DW;
		memcpy (flags, data, flags_length);
		flags[flags_length] = '\0';
		data += _SIZE_DB * flags_length;
#ifdef _DEBUG_OCTEON_COPROCESSORS
		printf("* Pattern:\n  - Flags length: %hd\n", flags_length);
		printf("  - Flags: \"%s\"\n", flags);
#endif

		pattern_length = *(uint16_t *) data;
		data += _SIZE_DW;

#ifdef _DEBUG_OCTEON_COPROCESSORS
		printf("  - Pattern length: %hd\n", pattern_length);
#endif
		p = (char *) malloc (sizeof (char) * (pattern_length + 1));
		memcpy (p, data, pattern_length);
		p[pattern_length] = '\0';

#ifdef _DEBUG_OCTEON_COPROCESSORS
		printf("  - Text: \"%s\"\n", p);
#endif
		fprintf (regexp,"%s\n", p);

		free (p);

		/* On with next pattern */
		data += pattern_length * _SIZE_DB;
	}

	fclose(regexp);
	out = nvmSUCCESS;

	return (out);
}


int32_t jit::octeon::control_special_char(char **buffer,uint8_t *data,uint16_t pattern_length)
{
	int i, k=0;
	*buffer = (char *) malloc (sizeof (char) * (5 * pattern_length + 1));
	printf("pat_len:%d\n",pattern_length);

	for(i=0; i< pattern_length; i++)
	{
		//TODO(MB) should I also escape '(', ')', '*', '?', etc. ???
		if (*data >= 128 || *data <= 32)
		{
			k += snprintf(&(*buffer)[k],5,"\\x%02x",*data);
			continue;
		}
		switch (*data) {
			case 34:
				memcpy(&(*buffer)[k],"\\\"",2);
				k+=2;
				break;
			case 36:
				memcpy(&(*buffer)[k],"\\$",2);
				k+=2;
				break;
			case 40:
				memcpy(&(*buffer)[k],"\\(",2);
				k+=2;
				break;
			case 41:
				memcpy(&(*buffer)[k],"\\)",2);
				k+=2;
				break;
			case 42:
				memcpy(&(*buffer)[k],"\\*",2);
				k+=2;
				break;
			case 43:
				memcpy(&(*buffer)[k],"\\+",2);
				k+=2;
				break;
			case 45:
				memcpy(&(*buffer)[k],"\\-",2);
				k+=2;
				break;
			case 46:
				memcpy(&(*buffer)[k],"\\.",2);
				k+=2;
				break;
			case 63:
				memcpy(&(*buffer)[k],"\\?",2);
				k+=2;
				break;
			case 91:
				memcpy(&(*buffer)[k],"\\[",2);
				k+=2;
				break;
			case 92:
				memcpy(&(*buffer)[k],"\\\\",2);
				k+=2;
				break;
			case 93:
				memcpy(&(*buffer)[k],"\\]",2);
				k+=2;
				break;
			case 94:
				memcpy(&(*buffer)[k],"\\^",2);
				k+=2;
				break;
			case 123:
				memcpy(&(*buffer)[k],"\\{",2);
				k+=2;
				break;
			case 125:
				memcpy(&(*buffer)[k],"\\}",2);
				k+=2;
				break;
			case 124:
				memcpy(&(*buffer)[k],"\\|",2);
				k+=2;
				break;
			case 10:
				memcpy(&(*buffer)[k],"\\n",2);
				k+=2;
				break;
			default:
				memcpy(&(*buffer)[k],data,1);
				k++;
				break;
		}
		data++;
	}
	(*buffer)[k]='\0';

	return k;
}

int32_t jit::octeon::nvmOcteonStringMatchCoproInjectData (nvmCoprocessorState *c, uint8_t *data) 
{

	uint32_t  pattern_data;
	uint16_t g, i, patterns_no = 0, pattern_length, pattern_nocase, graphs_no;
	int32_t out,k;
#ifdef _DEBUG_OCTEON_COPROCESSORS
	printf ("String-matching coprocessor initialising\n");
#endif

	ofstream octcode("octeon_stringmatch.c");
	octcode << "#include \"stringmatching.h\"\n";
	octcode << "#include \"graph.h\"\n";
	octcode << "#include \"dfa_info.h\"\n";

	// get number of graphs
	graphs_no = *(uint16_t *) data;
	uint16_t *patterns_number = new uint16_t[graphs_no] ;
	data += _SIZE_DW;
#ifdef _DEBUG_OCTEON_COPROCESSORS
	printf("graph_no:%d\n",graphs_no);
#endif

	char *buffer;

	// Read data for all graphs 
	for (g = 0; g < graphs_no; g++) {
		ostringstream filename;
		filename << "stringmatching" << g <<".pat";
		ofstream stm(filename.str().c_str());

		stm << "{.=[^\\n]}\n";

		//get number of patterns
		patterns_no = *(uint16_t *) data;
		data += _SIZE_DW;
#ifdef _DEBUG_OCTEON_COPROCESSORS
		printf ("* %hd patterns\n", patterns_no);
#endif
		octcode << "static octeonPatternInfo graph"<< g <<"_patterns[]=\n";
		octcode <<"\t{\n";
		patterns_number[g] = patterns_no;

		for (i = 0; i < patterns_no; i++) {
			pattern_length = *(uint16_t *) data;
			data += _SIZE_DW;
#ifdef _DEBUG_OCTEON_COPROCESSORS
			printf ("* Pattern:\n  - Length: %hd\n", pattern_length);
#endif
			pattern_nocase = *(uint16_t *) data;
			data += _SIZE_DW;
#ifdef _DEBUG_OCTEON_COPROCESSORS
			printf ("  - Nocase: %hd\n", pattern_nocase);
#endif
			pattern_data = *(uint32_t *) data;
			data += _SIZE_DD;
#ifdef _DEBUG_OCTEON_COPROCESSORS
			printf ("  - Data: %u\n", pattern_data);
#endif

			k = control_special_char(&buffer, data, pattern_length);

		//	if(i == 0)
		//		stm << "(";
			stm << "(" << buffer << ")" << endl ;

			octcode << "\t{" << pattern_length << ", " << pattern_nocase <<", " << pattern_data << ", \"\"},\n";// << buffer << "\"},\n";  
		//	if (i+1 != patterns_no)
		//		stm << "|";
		//	else
		//		stm << ")";
			data += pattern_length * _SIZE_DB;

			free(buffer);
			buffer = NULL;
		}
		stm << "\n";
		octcode << "\t};\n";
		stm.close();
	}

	octcode << "static octeonGraphInfo graphs[]=\n\t{\n";
	for(g=0;g<graphs_no; g++)
	{
		octcode <<"\t{"<< patterns_number[g] << ", ";
		for (i = 0; i < patterns_no; i++)
			octcode << "graph"<<g<<"_patterns";
		octcode << "},\n";
	}
	octcode << "\t};\n";

	octcode << "octeonStringMatchInternalData smcdata=\n{\n"<< graphs_no << ",\n"
		    << "graphs,\n compiled_graph_name,\n compiled_graph_info,\n" 
		    << " compiled_graph,\n compiled_graph_image_size,\n compiled_graph_llm_size\n,&info\n};\n"; 

	octcode << "info_dfa	info_prova ={\n"
			<< "num_marked_node,\n num_terminal_node,\n	terminal_node_delta_map_lg,\n node_to_expression_lists,\n"
			<< "node_to_expression_lists_overflow,\n size_of_ex_li,\n size_of_ex_li_ov};\n";

	delete[] patterns_number;

	out = nvmSUCCESS;

	return (out);
}

