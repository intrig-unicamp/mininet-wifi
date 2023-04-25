/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "defs.h"
#include <string>
#include <list>

using namespace std;



enum ErrorType
{
	ERR_PFL_ERROR = 0,
	ERR_PDL_ERROR,
	ERR_PFL_WARNING,
	ERR_PDL_WARNING,
	ERR_FATAL_ERROR
};


struct ErrorInfo
{
	ErrorType	Type;
	string		Msg;

	ErrorInfo(ErrorType type, const string &msg)
		:Type(type), Msg(msg){}

	ErrorInfo(ErrorType type, const char* msg)
		:Type(type), Msg(msg){}
};


class ErrorRecorder
{

	list<ErrorInfo> m_ErrorList;
	uint32	N_Errors[ERR_FATAL_ERROR + 1];

public:
	ErrorRecorder()
	{
		Clear();	
	}
	
	~ErrorRecorder()
	{
	}

	void Clear(void)
	{
		for (int i = 0; i <= ERR_FATAL_ERROR; i++)
			N_Errors[i] = 0;
		m_ErrorList.clear();
	}
	
	void PDLWarning(const string message);
	void PDLError(const string message);
	void PFLWarning(const string message);
	void PFLError(const string message);
	void FatalError(const string message);
	
	bool PDLErrorsOccurred();

	void Error(ErrorType kind, string msg = 0);

	uint32 TotalErrors(void)
	{
		uint32 sum = 0;
		for (uint32 i = 0; i <= ERR_FATAL_ERROR; i++)
			sum += N_Errors[i];
		return sum;
	}


	uint32 NumWarnings(void)
	{
		return N_Errors[ERR_PFL_WARNING] + N_Errors[ERR_PDL_WARNING];
	}

	uint32 NumErrors(void)
	{
		return (uint32)TotalErrors() - NumWarnings();
	}

	list <ErrorInfo> &GetErrorList(void)
	{
		return m_ErrorList;
	}

	string Errors2String(void);
	
};
