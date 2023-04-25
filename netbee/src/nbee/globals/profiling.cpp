/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "globals.h"
#include "utils.h"
#include "debug.h"
#include "profiling.h"


//! Standard constuctor
CProfilingExecTime::CProfilingExecTime()
{
	m_ticksBeforeArray= NULL;
	m_ticksAfterArray= NULL;
	m_ticks= NULL;
}


//! Standard destuctor
CProfilingExecTime::~CProfilingExecTime()
{
	if (m_ticksBeforeArray)
		free(m_ticksBeforeArray);
	if (m_ticksAfterArray)
		free(m_ticksAfterArray);
	if (m_ticks)
		free(m_ticks);
}


// Commented in the base class
int CProfilingExecTime::Initialize(int MaxNumSamples, int NumRunsPerSample, int DiscardFirst, int DiscardSmallest, int DiscardBiggest)
{
	m_ticksBeforeArray= (int64_t *) malloc(sizeof(int64_t) * MaxNumSamples);
	if (m_ticksBeforeArray == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Profiling error: error in allocating profiling data.");
		return nbFAILURE;
	}

	m_ticksAfterArray= (int64_t *) malloc(sizeof(int64_t) * MaxNumSamples);
	if (m_ticksAfterArray == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Profiling error: error in allocating profiling data.");

		return nbFAILURE;
	}

	m_ticks= (int64_t *) malloc(sizeof(int64_t) * MaxNumSamples);
	if (m_ticks == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Profiling error: error in allocating profiling data.");

		return nbFAILURE;
	}

	if (MaxNumSamples < ((DiscardFirst + DiscardSmallest + DiscardBiggest) * 2))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Profiling error: the number of samples is too small to have statistical validity. Try increasing the number of samples in the code.");

		return nbFAILURE;
	}

	memset(m_ticksBeforeArray, 0, sizeof(int64_t) * MaxNumSamples);
	memset(m_ticksAfterArray, 0, sizeof(int64_t) * MaxNumSamples);
	memset(m_ticks, 0, sizeof(int64_t) * MaxNumSamples);

	m_maxNumSamples= MaxNumSamples;
	if (NumRunsPerSample == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Profiling error: the number numer of runs per sample cannot be zero.");

		return nbFAILURE;
	}

	m_numRunsPerSample= NumRunsPerSample;
	m_discardFirst= DiscardFirst;
	m_discardSmallest= DiscardSmallest;
	m_discardBiggest= DiscardBiggest;

	// Calculate the cost of the RDTSC
	m_costRDTSC= nbProfilerGetMeasureCost();

	m_validSamples= 0;
	m_storedSamples= 0;

	// Initialize the structure that contains the results
	memset(&m_profilingResults, 0, sizeof(m_profilingResults));

	return nbSUCCESS;
}


// Commented in the base class
int CProfilingExecTime::StoreSample(int64_t StartingTime, int64_t EndingTime)
{

	if ((m_storedSamples) >= m_maxNumSamples)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Profiling error: the number of samples exceeds the data allocated for them.");

		return nbFAILURE;
	}

	if ((m_ticksBeforeArray == NULL) || (m_ticksAfterArray == NULL) || (m_ticks == NULL))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Profiling error: trying to store a sample, but the internal storage has not been allocated. Did you initialize this class?");

		return nbFAILURE;
	}

	m_ticksBeforeArray[m_storedSamples]= StartingTime;
	m_ticksAfterArray[m_storedSamples]= EndingTime;

	m_storedSamples++;

	return nbSUCCESS;
}


// Commented in the base class
_nbProfilerResults* CProfilingExecTime::GetRawResults()
{
	if (m_storedSamples <= (m_discardFirst + m_discardSmallest + m_discardBiggest))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"The number of samples in the profiling code is not enough to perform a statistical measurement.");
		return NULL;
	}

	return 	&(m_profilingResults);
}

// Commented in the base class
void CProfilingExecTime::GetRawSamples(int64_t* &SampleStartTicks, int64_t* &SampleEndTicks)
{
    SampleStartTicks= m_ticksBeforeArray;
    SampleEndTicks= m_ticksAfterArray;
}

int CProfilingExecTime::GetNumberOfSamples()
{
  return m_storedSamples;
}

int CProfilingExecTime::GetNumberOfMeaningfulSamples()
{
  return m_storedSamples - (m_discardFirst + m_discardSmallest + m_discardBiggest);
}

// Commented in the base class
int CProfilingExecTime::ProcessProfilerData()
{
int i, j;

	// We have room for m_maxNumSamples, but the user may ask us to store a smaller number of samples
	// So, we have to check that the number of samples stored is meaningful
	if (m_storedSamples <= (m_discardFirst + m_discardSmallest + m_discardBiggest))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"The number of samples in the profiling code is not enough to perform a statistical measurement.");
		return nbFAILURE;
	}

	// Copy data into the final array
	for (i= m_discardFirst; i < m_storedSamples; i++)
		m_ticks[i-m_discardFirst]= m_ticksAfterArray[i] - m_ticksBeforeArray[i] - m_costRDTSC;

	m_validSamples= m_storedSamples - m_discardFirst;

	// Sort data
	for (i= 0; i < m_validSamples; i++)
	{
		for (j= i+1; j < m_validSamples; j++)
		{
		int64_t Temp;

			if (m_ticks[i] > m_ticks[j])
			{
				Temp= m_ticks[j];
				m_ticks[j]= m_ticks[i];
				m_ticks[i]= Temp;
			}
		}
	}

	// Remove smallest and biggest samples
	for (i= 0; i < (m_validSamples - m_discardSmallest - 1); i++)
		m_ticks[i]= m_ticks[i + m_discardSmallest];

	m_validSamples= m_validSamples - m_discardSmallest - m_discardBiggest;
	// Save this data in the variable used to return the results
	m_profilingResults.ValidSamples= m_validSamples;

	// Calculate median value
	if (m_validSamples % 2)
		// We have an odd number of samples
		m_profilingResults.Median= m_ticks[m_validSamples / 2] / m_numRunsPerSample;
	else
		// We have an even number of samples
		m_profilingResults.Median= (m_ticks[m_validSamples / 2] + m_ticks[m_validSamples / 2 + 1] ) / (2 * m_numRunsPerSample);

	// Calculate mode (i.e. the value that happens with the highest frequency)
	m_profilingResults.Mode= CalculateMode(&m_profilingResults.ModeFrequency, m_validSamples);

	// Calculate the average
	m_profilingResults.Average= CalculateAverage(m_validSamples);

	// Calculate standard deviation
	m_profilingResults.StdDev= CalculateStdDev(m_profilingResults.Average, m_validSamples);

	m_profilingResults.ChavenetOutliers= RemoveOutliers(m_profilingResults.Average, m_profilingResults.StdDev, m_validSamples);
	m_validSamples= m_validSamples - m_profilingResults.ChavenetOutliers;

	// Re-calculate all parameters without outliers
	// Calculate median value
	if (m_validSamples % 2)
		// We have an odd number of samples
		m_profilingResults.MedianWithoutOutliers= m_ticks[m_validSamples / 2] / m_numRunsPerSample;
	else
		// We have an even number of samples
		m_profilingResults.MedianWithoutOutliers= (m_ticks[m_validSamples / 2] + m_ticks[m_validSamples / 2 + 1] ) / (2 * m_numRunsPerSample);

	// Calculate mode (i.e. the value that happens with the highest frequency)
	m_profilingResults.ModeWithoutOutliers= CalculateMode(&m_profilingResults.ModeFrequencyWithoutOutliers, m_validSamples);

	// Calculate the average
	m_profilingResults.AverageWithoutOutliers= CalculateAverage(m_validSamples);

	// Calculate standard deviation
	m_profilingResults.StdDevWithoutOutliers= CalculateStdDev(m_profilingResults.AverageWithoutOutliers, m_validSamples);


	return nbSUCCESS;
}


/*!
	\brief Calculate modal value

	\param ModeFrequency Used to return to the called the frequency of the modal value.
	\param ValidSamples Number of valid samples at the beginning of the m_ticks array.

	\return The most frequent value among the samples (i.e., the mode).
*/
int64_t CProfilingExecTime::CalculateMode(double* ModeFrequency, int ValidSamples)
{
int i;
int64_t Mode= 0;
int ModeNSamples= 0;
int64_t CurrentValue= 0;
int CurrentNumSamples= 0;

	for (i= 0; i < ValidSamples; i++)
	{
		if (m_ticks[i] == CurrentValue)
			CurrentNumSamples++;
		else
		{
			if (CurrentNumSamples > ModeNSamples)
			{
				// Update the current mode, if needed
				Mode= CurrentValue;
				ModeNSamples= CurrentNumSamples;
			}
			else
			{
				// Move to the next value in the list
				CurrentValue= m_ticks[i];
				CurrentNumSamples= 1;
			}
		}
	}
	// This is required in order to update the value in case the most frequent is the highest value in the list
	if (CurrentNumSamples > ModeNSamples)
	{
		// Update the current mode, if needed
		Mode= CurrentValue;
		ModeNSamples= CurrentNumSamples;
	}

	*ModeFrequency= ModeNSamples * (double) 100.0 / ValidSamples;
	return (Mode / m_numRunsPerSample);
}


/*!
	\brief Calculate average value

	\param ValidSamples Number of valid samples at the beginning of the m_ticks array.

	\return The average value among the samples.
*/
int64_t CProfilingExecTime::CalculateAverage(int ValidSamples)
{
int i;
int64_t SumTicks;

	SumTicks= 0;

	for (i= 0; i < ValidSamples; i++)
		SumTicks+=  m_ticks[i];

	return (SumTicks / m_numRunsPerSample / ValidSamples);
}


/*!
	\brief Calculate standard deviation

	\param Average The average value among the samples (which must calculated before calling this function).
	\param ValidSamples Number of valid samples at the beginning of the m_ticks array.

	\return The standard deviation among the samples.
*/
double CProfilingExecTime::CalculateStdDev(int64_t Average, int ValidSamples)
{
int i;
double SumSquare;
double StdDev;

	SumSquare= 0;

	for (i= 0; i < ValidSamples; i++)
		SumSquare+=  pow( ((double)m_ticks[i]/m_numRunsPerSample - Average), 2);

	StdDev= sqrt (SumSquare / ValidSamples);

	return StdDev;
}



/*!
	\brief Removes outliers according to the Chauvenet criterion.

	In fact, ee use here a criteria similar to the Chauvenet criterion
	http://en.wikipedia.org/wiki/Chauvenet%27s_criterion
	to remove outliers ( http://en.wikipedia.org/wiki/Outlier ).
	Our criteria is a little bit simpler because
	it purges data that are larger or smaller than (avg +/- StdAvg).

	\param Average Average value of our samples.
	\param StdDev Standard Deviation of our samples.
	\param ValidSamples Number of valid samples at the beginning of the m_ticks array.

	\return Number of outliers according to the Chauvenet criterion.
*/
int CProfilingExecTime::RemoveOutliers(int64_t Average, double StdDev, int ValidSamples)
{
int i, nOutliers, nValidSamples;

	i = 0;
	nOutliers= 0;
	nValidSamples= 0;

	// Outliers must be either at the beginning, or at the end of the list (we have a sorted list of samples, here)

	// Remove outliers at the beginning of the list
	if ((m_ticks[nOutliers]/m_numRunsPerSample) < (Average - 2 * StdDev))
	{
		while ((m_ticks[nOutliers]/m_numRunsPerSample) < (Average - 2 * StdDev))
			nOutliers++;

		for (i= 0; i< ValidSamples - nOutliers; i++)
			m_ticks[i]= m_ticks[i+nOutliers];
	}

	// Remove outliers at the end of the list
	nValidSamples= 0;
	while (((m_ticks[nValidSamples]/m_numRunsPerSample) <= (Average + 2 * StdDev)) && (nValidSamples < (ValidSamples - nOutliers)))
		nValidSamples++;

	// if this test is 'true', it means that we found outliers at the end of the list
	if (nValidSamples < (ValidSamples - nOutliers))
		nOutliers= ValidSamples - nValidSamples;

	return nOutliers;
}


// Commented in the base class
int CProfilingExecTime::FormatResults(char* ResultBuffer, int ResultBufferSize, bool PrintCompact)
{
char Buffer[2048];

	if (m_validSamples == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Cannot find data to print. Did you store the data before calling this function?");
		return nbFAILURE;
	}

	if (PrintCompact)
	{
		ssnprintf(ResultBuffer, ResultBufferSize, "Valid samples: %d, ", m_validSamples);
		ssnprintf(Buffer, sizeof(Buffer), "Cost RDTSC: %llu, ", m_costRDTSC);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);

		ssnprintf(Buffer, sizeof(Buffer), "Median: %llu, ", m_profilingResults.MedianWithoutOutliers);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "Mode: %llu (in %.2f%% of the samples), ", m_profilingResults.ModeWithoutOutliers, m_profilingResults.ModeFrequencyWithoutOutliers);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "Avg: %llu, ", m_profilingResults.AverageWithoutOutliers);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "Std Dev: %.2f\n", m_profilingResults.StdDevWithoutOutliers);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);

	}
	else
	{
		ssnprintf(ResultBuffer, ResultBufferSize, "\nNumber of runs per sample:\t %d\n", m_numRunsPerSample);
		ssnprintf(Buffer, sizeof(Buffer), "Total number of samples:\t %d\n", m_storedSamples);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "Number of valid samples, excluding transient and biggest/smallers:\t %d\n", m_profilingResults.ValidSamples);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "Number of valid samples, excluding also Chavenet's outliers:\t %d\n", m_validSamples);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Transient (number of samples discarded at the beginning):\t %d\n", m_discardFirst);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Outliers (# of the smallest samples discarded):\t %d\n", m_discardSmallest);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Outliers (# of the biggest samples discarded):\t %d\n", m_discardBiggest);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Chavenet outliers (# discarded because are < or > than (Avg +- 2*StdDev)):\t %d\n", m_profilingResults.ChavenetOutliers);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "Cost RDTSC:\t %llu\n", m_costRDTSC);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);

		ssnprintf(Buffer, sizeof(Buffer), "\nOriginal results (with outliers)\n");
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Median:\t %llu\n", m_profilingResults.Median);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Mode:\t %llu (in %.2f%% of the samples)\n", m_profilingResults.Mode, m_profilingResults.ModeFrequency);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Average:\t %llu\n", m_profilingResults.Average);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Standard Deviation:\t %.2f\n", m_profilingResults.StdDev);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);

		ssnprintf(Buffer, sizeof(Buffer), "\nNew results (without outliers)\n");
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Median:\t %llu\n", m_profilingResults.MedianWithoutOutliers);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Mode:\t %llu (in %.2f%% of the samples)\n", m_profilingResults.ModeWithoutOutliers, m_profilingResults.ModeFrequencyWithoutOutliers);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Average:\t %llu\n", m_profilingResults.AverageWithoutOutliers);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
		ssnprintf(Buffer, sizeof(Buffer), "  Standard Deviation:\t %.2f\n", m_profilingResults.StdDevWithoutOutliers);
		sstrncat(ResultBuffer, Buffer, ResultBufferSize);
	}

	// Probably the buffer size was not enough to print the results; '-1' to take into account the \n' at the end
	if ((ResultBufferSize == 0) || ( (int) strlen(ResultBuffer) == (ResultBufferSize - 1)))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"The buffer that should contain the resuls is too small. Please try increasing the result buffer.");
		return nbFAILURE;
	}

	return nbSUCCESS;
}

