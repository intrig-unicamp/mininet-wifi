/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include <nbee_profiler.h>



class CProfilingExecTime : public nbProfiler
{
public:
	CProfilingExecTime();
	virtual ~CProfilingExecTime();

	int Initialize(int MaxNumSamples, int NumRunsPerSample, int DiscardFirst, int DiscardSmallest, int DiscardBiggest);
	int StoreSample(int64_t StartingTime, int64_t EndingTime);
	int ProcessProfilerData();
	_nbProfilerResults* GetRawResults();
	void GetRawSamples(int64_t* &SampleStartTicks, int64_t* &SampleEndTicks);
        int GetNumberOfSamples();
        int GetNumberOfMeaningfulSamples();
	int FormatResults(char* ResultBuffer, int ResultBufferSize, bool PrintCompact=false);

	char *GetLastError() { return m_errbuf; }

private:

	int64_t CalculateMode(double* ModeFrequency, int ValidSamples);
	int64_t CalculateAverage(int ValidSamples);
	double CalculateStdDev(int64_t Average, int ValidSamples);
	int RemoveOutliers(int64_t Average, double StdDev, int ValidSamples);

	int64_t * m_ticksBeforeArray;
	int64_t * m_ticksAfterArray;
	int64_t * m_ticks;
	int64_t m_costRDTSC;

	int m_maxNumSamples;					//!< Maximum number of possible samples. Please note that storage is valid till Ticks[m_numSamples - 1].
	int m_storedSamples;					//!< Number of stored samples. Please note that data is valid till Ticks[m_storedSamples - 1].
	int m_validSamples;						//!< Number of actually valid samples, not counting the data discarder for transient, etc. Please note that storage is valid till Ticks[m_validSamples - 1].
	int m_numRunsPerSample;					//!< Number of runs for each sample (it's a parameter defined by the user when it invokes this class)
	int m_discardFirst;						//!< Number of samples that have to be discarded at the beginning, for avoiding the transient (it's a parameter defined by the user when it invokes this class)
	int m_discardSmallest;					//!< Number of the smallest samples that have to be discarded (it's a parameter defined by the user when it invokes this class)
	int m_discardBiggest;					//!< Number of the biggest samples that have to be discarded (it's a parameter defined by the user when it invokes this class)

	_nbProfilerResults m_profilingResults;	//!< Structure that keeps the results that have to be returned to the user, if requested

	char m_errbuf[2048];					//!< Buffer that keeps the error message (if any)
};

