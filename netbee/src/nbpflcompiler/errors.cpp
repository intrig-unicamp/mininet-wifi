/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "errors.h"


void ErrorRecorder::Error(ErrorType kind, string msg)
{
	N_Errors[kind]++;
	m_ErrorList.push_back(ErrorInfo(kind, msg));
}


string ErrorRecorder::Errors2String(void)
{
	string errors;
	list<ErrorInfo>::iterator i;
	for (i = m_ErrorList.begin(); i != m_ErrorList.end(); i++)
	{
		switch((*i).Type)
		{
			case ERR_PFL_ERROR:
				errors += "PFL Error: ";
				break;
			case ERR_PDL_ERROR:
				errors += "NetPDL Error: ";
				break;
			case ERR_FATAL_ERROR:
				errors += "FATAL Error: ";
				break;
			case ERR_PFL_WARNING:
				errors += "PFL Warning: ";
				break;
			case ERR_PDL_WARNING:
				errors += "NetPDL Warning: ";
				break;
		}
		errors += (*i).Msg + string("\n");
	}
	m_ErrorList.clear();
	return errors;
}

void ErrorRecorder::PDLWarning(const string message)
{
	Error(ERR_PDL_WARNING, message);
}


void ErrorRecorder::PDLError(const string message)
{
	Error(ERR_PDL_ERROR, message);
}

void ErrorRecorder::PFLWarning(const string message)
{
	Error(ERR_PFL_WARNING, message);
}

void ErrorRecorder::PFLError(const string message)
{
	Error(ERR_PFL_ERROR, message);
}

void ErrorRecorder::FatalError(const string message)
{
	Error(ERR_FATAL_ERROR, message);
}

bool ErrorRecorder::PDLErrorsOccurred()
{
	return N_Errors[ERR_PDL_ERROR]>0;
}


