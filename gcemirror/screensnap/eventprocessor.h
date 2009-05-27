// eventprocessor.h: interface for the EventProcessor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EVENTPROCESSOR_H__A73B79C0_9F1E_492D_AEF7_37C5B5ED1F14__INCLUDED_)
#define AFX_EVENTPROCESSOR_H__A73B79C0_9F1E_492D_AEF7_37C5B5ED1F14__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Winsock.h>

class EventProcessor  
{
public:
	EventProcessor(SOCKET cs, HANDLE clientEvent);
	void start();
	virtual ~EventProcessor();

private:
	SOCKET cs;
	HANDLE clientEvent;
};

#endif // !defined(AFX_EVENTPROCESSOR_H__A73B79C0_9F1E_492D_AEF7_37C5B5ED1F14__INCLUDED_)
