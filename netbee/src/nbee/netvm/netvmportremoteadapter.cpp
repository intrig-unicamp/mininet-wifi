/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <nbee_netvm.h>
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"


#ifndef WIN32
#include <string.h>
#endif


nbNetVmPortRemoteAdapter::nbNetVmPortRemoteAdapter()
{
	Host= NULL;
	Port= NULL;
	AuthType= 0;
	AuthToken1= NULL;
	AuthToken2= NULL;
	AuthToken3= NULL;
}

nbNetVmPortRemoteAdapter::~nbNetVmPortRemoteAdapter()
{
	DELETE_PTR(Host);
	DELETE_PTR(Port);
	DELETE_PTR(AuthToken1);
	DELETE_PTR(AuthToken2);
	DELETE_PTR(AuthToken3);
}


int nbNetVmPortRemoteAdapter::Initialize(char *Name, char *Description, int Flags, char *Host, char *Port, int AuthType, char *AuthToken1, char *AuthToken2, char *AuthToken3)
{
	this->Flags= Flags;

	if (nbNetVmPortDataDevice::Initialize(Name, Description) == nbFAILURE)
		return nbFAILURE;

	if (Host)
	{
		this->Host= new char [strlen(Host) + 1];
		if (this->Host == NULL) goto AllocMemFailed;
		strcpy(this->Host, Host);
	}
	if (Port)
	{
		this->Port= new char [strlen(Port) + 1];
		if (this->Port == NULL) goto AllocMemFailed;
		strcpy(this->Port, Port);
	}

	this->AuthType= AuthType;
	if (AuthToken1)
	{
		this->AuthToken1= new char [strlen(AuthToken1) + 1];
		if (this->AuthToken1 == NULL) goto AllocMemFailed;
		strcpy(this->AuthToken1, AuthToken1);
	}
	if (AuthToken2)
	{
		this->AuthToken2= new char [strlen(AuthToken2) + 1];
		if (this->AuthToken2 == NULL) goto AllocMemFailed;
		strcpy(this->AuthToken2, AuthToken2);
	}
	if (AuthToken3)
	{
		this->AuthToken3= new char [strlen(AuthToken3) + 1];
		if (this->AuthToken3 == NULL) goto AllocMemFailed;
		strcpy(this->AuthToken3, AuthToken3);
	}

AllocMemFailed:
	ssnprintf(m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
	return nbFAILURE;
}


nbNetVmPortDataDevice *nbNetVmPortRemoteAdapter::Clone(char *Name, char *Description, int Flags)
{
	nbNetVmPortRemoteAdapter * RemoteAdapter;

	RemoteAdapter= new nbNetVmPortRemoteAdapter;

	if (RemoteAdapter == NULL)
	{
		ssnprintf(m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
		return NULL;
	}

	if (RemoteAdapter->Initialize(Name, Description, Flags, Host, Port, AuthType, AuthToken1, AuthToken2, AuthToken3) == nbFAILURE)
		return NULL;

	// Copy the remaining info present in this class in the new object
	strcpy(RemoteAdapter->m_errbuf, m_errbuf);

	// Copy embedded objects, in case these are present
	if (AddressList) 
	{
		RemoteAdapter->AddressList= AddressList->Clone();
		if (RemoteAdapter->AddressList == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), __FUNCTION__ " %s", AddressList->GetLastError() );
			delete RemoteAdapter;
			return NULL;
		}
	}

	if (Next)
	{
		RemoteAdapter->Next= ((nbNetVmPortRemoteAdapter*)Next)->Clone(Name, Description, Flags);
		if (RemoteAdapter->Next == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), __FUNCTION__ " %s", Next->GetLastError() );
			delete RemoteAdapter;
			return NULL;
		}
	}

	return RemoteAdapter;
};

int nbNetVmPortRemoteAdapter::Close()
{
	//! \todo Close() not implemented;
	return nbFAILURE;
}

int nbNetVmPortRemoteAdapter::OpenAsDataSrc()
{
	//! \todo OpenAsDataSrc() not implemented;
	return nbFAILURE;
}

int nbNetVmPortRemoteAdapter::OpenAsDataDst()
{
	//! \todo OpenAsDataDst() not implemented;
	return nbFAILURE;
}