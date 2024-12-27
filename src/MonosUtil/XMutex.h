// XMutex.h: interface for the CXMutex class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMUTEX_H__6487691F_917B_4E05_9CB0_A8523C987CEC__INCLUDED_)
#define AFX_XMUTEX_H__6487691F_917B_4E05_9CB0_A8523C987CEC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CXMutex  
{
public:
	BOOL Unlock();
	BOOL Lock();
	CXMutex();
	virtual ~CXMutex();

protected:
	HANDLE m_Handle;
};

#endif // !defined(AFX_XMUTEX_H__6487691F_917B_4E05_9CB0_A8523C987CEC__INCLUDED_)
