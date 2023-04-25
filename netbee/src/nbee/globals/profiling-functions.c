/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#if(defined (WIN32) || defined(_WIN64))
#include <sys/timeb.h>		// ftime()
#else
#include <sys/time.h>		// gettimeofday()
#endif


// What about looking at the High Precision Evet Timer? http://en.wikipedia.org/wiki/HPET
// http://stackoverflow.com/questions/771867/how-to-make-a-cross-platform-c-inline-assembly-language
// GCC Assembly HowTo: http://ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html

#ifdef _MSC_VER
// For Microsoft Visual Studio, we use intrinsics
uint64_t nbProfilerGetTime()
{
	 return __rdtsc();
}
#else

#ifdef __OCTEON__
uint64_t nbProfilerGetTime()
{
	return 0L;
}

#else // x86
// For Gnu GCC, we use inline assembly
uint64_t nbProfilerGetTime()
{
  volatile uint32_t lo, hi; // This variable can be changed and it is not under the contro of GCC; so, better use 'volatile'
// We should use the cpuid instruction in order to flush the pipeline and prevent out-of-order execution, but
// it seems to me that it increases quite a lot the cost of the measurement (from 70 to 300 ticks) and
// we do not have a corresponding increase in the precision
//	__asm__ __volatile__ ("cpuid":  : );

// This assembly statement must execute where we put it, (i.e. must not be moved out of a loop as an optimization), 
// so we use the  keyword __volatile__ after __asm__
  // OLD VERSION: __asm__ __volatile__ ("rdtsc" : "=A" (ret) : ); 
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}
#endif
#endif



// This function is used to have a valid estimation of the cost of the rdtsc function.
// It cycles till it gets the same value of the cost of the measurement function in ten consecutive runs.
int64_t nbProfilerGetMeasureCost()
{
int64_t TicksDeltaArray[10];
int64_t time1, time2, total;
int i;

	while (1)
	{
		total= 0;

		// This is a fake reading for warming up structures
		time1= nbProfilerGetTime();

		for (i= 0; i < 10; i++)
		{
			time1= nbProfilerGetTime();
			time2= nbProfilerGetTime();
			TicksDeltaArray[i]= time2 - time1;
		}

		for (i= 0; i < 10; i++)
			total+= TicksDeltaArray[i];

		if ((total / 10) == TicksDeltaArray[0])
			break;
	}

	return TicksDeltaArray[0];
}



uint64_t nbProfilerGetMicro()
{
#if (defined _WIN32) || (defined (_WIN64))
struct _timeb timebuffer;

	_ftime(&timebuffer);
	return (timebuffer.time * 1000000L + timebuffer.millitm * 1000);
#else
struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000L + tv.tv_usec;
#endif
}
