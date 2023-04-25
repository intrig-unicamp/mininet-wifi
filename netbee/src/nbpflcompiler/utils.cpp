/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "defs.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//Ivano: added the following definitions to compile on windows
#ifdef WIN32
 #define WINDOWS
#elif WIN64
 #define WINDOWS
#endif

int32 IPaddr2int(const char *addr, uint32 *num)
{
	uint8 tmp[4];
	uint32 i, byte;
	char *dot, *endp;

	for (i = 0; i < 4; i++)
	{
		if (i == 3)
		{
			dot = (char *)strchr(addr, '\0');
		}
		else
		{
			dot = (char *)strchr(addr, '.');
			if (dot == NULL)
				return -1;
		}

		byte = strtol(addr, &endp, 10);
		if (endp != dot)
			return -1;
		if (byte > 255)
			return -1;
		tmp[i] = byte;

		addr = dot + 1;
	}

	*num = (tmp[0] << 24) + (tmp[1] << 16) + (tmp[2] << 8) + tmp[3];

	return 0;

}

int32 MACaddr2int(const char *addr, uint32 *num)
{
	nbASSERT(false,"MAC address not supported now");

	// uint8 tmp[6];
	uint32 byte;
	char aux[3];

	for (int i = 0, j = 0; i < 6; i++,j+=3)
	{
		aux[0]=addr[j];
		aux[1]=addr[j+1];
		byte=0;
		for(int k=0;k<2;k++){
			uint32 numero;
			switch(aux[k]){
#ifdef WINDOWS
			case '0': case '1': case '2': case '3': case '4':case '5':case '6':case '7':case '8':case '9':
#else
				case '0' ... '9':
#endif
					numero = aux[k]-'0';
					break;	
#ifdef WINDOWS
				case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
#else
				case 'A' ... 'F':
#endif
					numero = aux[k]-'A'+10;
					break;	
#ifdef WINDOWS
				case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
#else
				case 'a' ... 'f':			
#endif
					numero = aux[k]-'a'+10;				
					break;
			}
			if(k==0)
				numero*=10;
			byte+=numero;
				
		}
		if (byte > 255)
			return -1;
		// tmp[i] = byte;
	}

	//num = (tmp[0] << 40) + (tmp[1] << 32) + (tmp[2] << 24) + (tmp[3] << 16) + (tmp[4] << 8) +tmp[5];
	return 0;

}

int32 str2int(const char *s, uint32 *num, uint8 base)
{
	uint32 n;
	char *endp;
	

	n = strtoul(s, &endp, base);
	if ((*endp != '\0') || (errno == ERANGE))
		return -1;

	*num = n;

	return 0;

}


string int2str(const uint32 num, uint8 base)
{
	char str[12];
	snprintf(str, 12, "%u", num);
	//ultoa(num, str, base);
	return string(str);
}
