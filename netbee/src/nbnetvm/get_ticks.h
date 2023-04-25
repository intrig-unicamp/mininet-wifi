/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/






#ifndef __GET_TICKS_H__
#define __GET_TICKS_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined WIN32


#define TICK_TYPE ULONGLONG

#define get_ticks(a) __get_ticks((unsigned long*)(a))


__inline void __get_ticks(unsigned long *tmp)
{
  //59 colpi, circa
	__asm
	{
		push eax
		push edx
		push ecx
		rdtsc
		mov ecx, tmp
		mov [ecx+4], edx
		mov [ecx], eax
		pop ecx
		pop edx
		pop eax
	}
}

#define GET_TICKS_CALL 59

#else  /*!WIN32*/

#define TICK_TYPE long long

static inline void get_ticks(unsigned long long *ptr)
{

	typedef struct
	{
		unsigned long low,high;
	}
	MY_LARGE;


	__asm__ __volatile__ ( "
				pushl %eax \n
				pushl %edx \n");
	__asm__ __volatile__ ( "
				rdtsc \n"
				: "=a" (((MY_LARGE*)ptr)->low),"=d" (((MY_LARGE*)ptr)->high)
				);
	__asm__ __volatile__ ( "
				popl %edx \n
				popl %eax \n");

//COSTA circa 55 colpi di clock

}
#define GET_TICKS_CALL 55




#endif /*WIN32*/

#ifdef __cplusplus
}
#endif


#endif