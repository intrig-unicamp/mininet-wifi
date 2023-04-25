/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once


#ifdef NBEE_EXPORTS
	// We may have include header files of some other libraries, which do not have the right xxxx_EXPORTS
	// and then defined DLL_EXPORT as ""; So, let's define this in any case, and let's suppress the warning
	#pragma warning(disable: 4005)
	#define DLL_EXPORT __declspec(dllexport)
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif


#include <stdint.h>				// for int64_t

#ifdef _MSC_VER
#include <intrin.h>				// needed for intrinsic
//#pragma intrinsic(__rdtsc)
#endif



/*!
	\file nbee_profiler.h

	Contains some utilities that can be used to profile the execution of programs/functions at the CPU tick level.
*/


/*!
	\defgroup NetBeeProfiling NetBee Profiling Utilities

	The NetBee profiling classes and functions can be to profile the execution
	of programs/functions at the CPU tick level.
	They offer a high-level interface (albeit efficient) in order to take CPU ticks
	measurements in the user-level software.

	This module can be used in two modes:
	- (either) the user can collect several measures, and then process the data with some well-known
	statistical tool (in this case he can use the nbProfiler class)
	- (or) the user can proceed with 'one-shot' measures, by calling the nbProfilerGetTime() before
	and after its code, and then subtract the cost of the measure through the nbProfilerGetMeasureCost()
	function. More details in the documenttion of the nbProfilerGetMeasureCost() function.
*/


/*! \addtogroup NetBeeProfiling
	\{
*/


#ifdef __cplusplus
extern "C" {
#endif



//! Get the RDTSC and return the current value of the CPU counter.
DLL_EXPORT uint64_t nbProfilerGetTime();


/*!
	\brief It returns the cost of the measure.

	This function can be used if the user is interested in 'one-shot' measures.
	In this case, the user can call the nbProfilerGetTime() before and after its
	code, and then ask this function to determine the cost of the measure.
	Consequently, the cost of the code will be (EndigTime - StartingTime - MeasureCost).

	\return The cost of the measure.
*/
DLL_EXPORT int64_t nbProfilerGetMeasureCost();



/*!
	\brief It returns the current timestamp in microseconds.

	This function can be used as a one-shot measure and it
	returns a time identifier that has a resolution of
	microseconds (10^-6 s). The meaning of its absolute value
	is implementation dependant. If two different 'snapshots'
	are taken with this method, the difference between the
	returned values is (a close approximation of) the time
	elapsed between both measurements, in microseconds.

	\warning The precision of this function in Windows drops
	to milliseconds, although the returned value is always
	a microsecond.
*/
DLL_EXPORT uint64_t nbProfilerGetMicro();


//! Structure that contains the 'raw' results of the profiling.
typedef struct _nbProfilerResults
{
	int64_t Average;		//!< Average value among the samples (in case of multiple runs, it refers to a single run).
	double StdDev;			//!< Standard deviation among the samples (in case of multiple runs, it refers to a single run).
	int64_t Median;		//!< Median value among the samples (in case of multiple runs, it refers to a single run). Median: having the ordered list of results, we take the one that is in the middle.
	int64_t Mode;			//!< Modal value among the samples (in case of multiple runs, it refers to a single run). Mode: the value that has the highest number of hits.
	double ModeFrequency;		//!< Frequency of the modal value.

	int64_t AverageWithoutOutliers;	//!< Average value among the samples, excluding transient and outliers.
	double StdDevWithoutOutliers;		//!< Standard deviation among the samples, excluding transient and outliers.
	int64_t MedianWithoutOutliers;		//!< Median value among the samples, excluding transient and outliers.
	int64_t ModeWithoutOutliers;		//!< Modal value among the samples, excluding transient and outliers.
	double ModeFrequencyWithoutOutliers;	//!< Frequency of the modal value, excluding transient and outliers.

	int ValidSamples;					//!< Number of valid samples, excluding transient and outliers (smallest/biggest samples, which the user requested not to consider).
	int ChavenetOutliers;				//!< Number of samples that have been discarded due to the Chauvenet criterion (this value + smallest/biggest samples + transient = number of samples stored in this class).
} _nbProfilerResults;



/*!
	\brief Class that defines the NetBee profiler.

	Measurements are currently based on the 'rdtsc' assembly instruction available on
	Pentium-class CPUs. Results are the 'ticks' returned by this instruction, whose meaning
	changes in different CPU (it was the number of CPU ticks in earlier processors, while
	it counts the number of ticks in the frontend bus in the latters).

	In order to use this class, you must:
	- instantiate the class through the nbAllocateProfiler()
	- initialize the class ( nbProfiler::Initialize() )
	- get the time before/after the section of code you want to measure ( nbProfilerGetTime() )
	- store those times in the nbProfiler class ( nbProfiler::StoreSample() )

	When you stored all the samples, you have to process the data and return the result.
	For that, you have to:
	- process the data ( nbProfiler::ProcessProfilerData() )
	- (either) return the results to the caller ( nbProfiler::GetRawResults() )
	- (or) print the results on a buffer ( nbProfiler::FormatResults() )

	Finally, the class can be deallocated with the nbDeallocateProfiler().
*/
class DLL_EXPORT nbProfiler
{
public:
	nbProfiler() {};
	virtual ~nbProfiler() {};

	/*!
		\brief It initializes the class, giving the user the possibility to customize some parameters.

		This function specifies how many 'samples' we can store in this profiling instance.
		Samples that will exceed this value are discarded.
		Vice versa, if the number of samples is smaller than the maximum allocated space, only
		the valid samples are considered in the processing.

		Please note that the number of samples actually considers also the number of runs for each sample.
		In other words, if we want to store 10 samples, but each sample is taken 5 times (5 runs), we have
		to dimension the MaxNumSamples equal to 10x5= 50.

		\param MaxNumSamples Max number of samples we expect to store in the current profiling instance.
		\param NumRunsPerSample Number of runs for each sample.
		\param DiscardFirst Number of samples at the beginning of our measurement that have to be discarded (for avoiding the transient).
		\param DiscardSmallest Number of the smallest samples that have to be discarded, in order to avoid 'fake' measurements.
		\param DiscardBiggest Number of the biggest samples that have to be discarded, in order to avoid 'fake' measurements.

		\return nbSUCCESS if the everything is fine, nbFAILURE in case of errors
		(e.g., some critical errors in allocating the internal structures).
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int Initialize(int MaxNumSamples, int NumRunsPerSample= 1, int DiscardFirst= 0, int DiscardSmallest= 0, int DiscardBiggest= 0)= 0;


	/*!
		\brief It stores a new sample.

		\param StartingTime The value of the time before the code that has to be measured, as read by the nbProfilerGetTime().
		\param EndingTime The value of the time after the code that has to be measured, as read by the nbProfilerGetTime().

		\return nbSUCCESS if the everything is fine, nbFAILURE in case of errors
		(e.g. not enough space for the current sample).
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int StoreSample(int64_t StartingTime, int64_t EndingTime)= 0;


	/*!
		\brief It processes profiling data.

		This method processes all the samples stored in the current class instance and calculates
		the results (average value, etc).
		This method must be called *once*; in other words we cannot recycle the same class for taking
		other measurements after having processed the internal data.

		\return nbSUCCESS if the everything is fine, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int ProcessProfilerData()= 0;


	/*!
		\brief Returns a pointer to the results calculated by the ProcessProfilerData().

		\return A pointer to the _nbProfilerResults structure if the everything is fine,
		NULL in case of errors. The pointer is valid till the class is live.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\note This function can be used if the user wants to have access to the raw data.
		Otherwise, the FormatResults() function can be used, which prints all the results
		in a buffer ready to be printed on screen.
	*/
	virtual _nbProfilerResults* GetRawResults()= 0;

	/*!
		\brief Returns a pointer to the array that contains the raw samples.

		\param SampleStartTicks Reference to a (int64_t *) variable that will contain
		the array of the 'starting ticks' (i.e., the value of the CPU counter when we
		started the measure of the current sample).

		\param SampleEndTicks Reference to a (int64_t *) variable that will contain
		the array of the 'ending ticks' (i.e., the value of the CPU counter when we
		stopped the measure of the current sample).

		\note The size of the returned arrays is equal to the product between the
		'MaxNumSamples' and 'NumRunsPerSample' parameters of the Initialize() function.
        */
        virtual void GetRawSamples(int64_t* &SampleStartTicks, int64_t* &SampleEndTicks)= 0;

        // Returns the number of samples stored
        virtual int GetNumberOfSamples() = 0;

        /* Returns the number of samples that are actually used for statistical computations.
         * This might be smaller than the value returned by GetNumberOfSamples() because
         * the user might have specified to discard some samples (at the beginning of the measurements,
         * or the smaller/bigger ones).
         *
         * The value returned is negative if there are no meaningful samples.
         */
        virtual int GetNumberOfMeaningfulSamples() = 0;

	/*!
		\brief It prints the result of the profiler on a buffer, ready to be printed on screen.

		\param ResultBuffer Pointer to a user-allocated buffer that will contain the printed results.
		\param ResultBufferSize Size of the previous buffer.
		\param PrintCompact 'true' if we want a 'compact' presentation of the results (one per line)
		or 'false' if we want to have all the details of the profiling.

		\return nbSUCCESS if the everything is fine, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int FormatResults(char* ResultBuffer, int ResultBufferSize, bool PrintCompact=false)= 0;


	/*!
		\brief Returns a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	virtual char *GetLastError()= 0;
};


/*!
	\brief It creates a new instance of a nbProfiler class and it returns it to the caller.

	This function is needed in order to create the proper object derived from nbProfiler.
	For instance, nbProfiler is a virtual class and it cannot be instantiated.
	The returned object has the same interface of nbProfiler, but it is a real object.

	nbProfiler is a class that contains some utilities for providing a CPU-tick accurate profiling.

	\param ErrBuf User-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize Size of the previous buffer.

	\return A pointer to a nbProfiler object, or NULL if something was wrong.
	In that case, the error message will be returned in the ErrBuf buffer.

	\warning Be carefully that the returned object must be deallocated through the nbDeallocateProfiler().
*/
DLL_EXPORT nbProfiler* nbAllocateProfiler(char *ErrBuf, int ErrBufSize);


/*!
	\brief It deallocates the instance of nbProfiler created through the nbAllocateProfiler().

	\param Profiler Pointer to the object that has to be deallocated.
*/
DLL_EXPORT void nbDeallocateProfiler(nbProfiler* Profiler);


#ifdef __cplusplus
}  /* extern "C" */
#endif


/*!
	\}
*/
