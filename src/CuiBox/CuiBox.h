
// CuiBox.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CCuiBoxApp:
// See CuiBox.cpp for the implementation of this class
//

class CCuiBoxApp : public CWinApp
{
public:
	CCuiBoxApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();

public:
	ULONG_PTR m_gdiplusToken;

public:
	DWORD GetPythonPid();
	void ReplaceFiles();
	bool CheckMutex();
};

extern CCuiBoxApp theApp;
