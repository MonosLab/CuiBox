
// CuiBox.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "CuiBox.h"
#include "CuiBoxDlg.h"
#include "../MonosUtil/Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCuiBoxApp

BEGIN_MESSAGE_MAP(CCuiBoxApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CCuiBoxApp construction

CCuiBoxApp::CCuiBoxApp()
	: m_gdiplusToken(ULONG_PTR(0))
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CCuiBoxApp object

CCuiBoxApp theApp;


// CCuiBoxApp initialization

BOOL CCuiBoxApp::InitInstance()
{
	if (!CheckMutex())
	{
		exit(0);
		return FALSE;
	}

	DWORD dwPid = GetPythonPid();
	if (dwPid != 0 && CUtil::ExistsLastPython(dwPid))
	{
		CUtil::TerminateProcessByPID(dwPid);
	}

	// CuiBox.rc 파일에 CLASS 추가 필요. ( ex> CLASS "클래스명" )
	WNDCLASS wc;
	memset(&wc, 0x00, sizeof(wc));
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = APP_CLASS;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOWFRAME);
	if (!AfxRegisterClass(&wc))
	{
		PrintOut(_T(">> Window Class NOT Registered!\r\n"));
	}

	//::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	//::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	HRESULT _hr = ::CoInitialize(nullptr);

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	ReplaceFiles();

	// Initialize GDI+
	PrintOut(_T(">> Initialize GDI+\r\n"));
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	AfxInitRichEdit2();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CCuiBoxDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Delete the shell manager created above.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CCuiBoxApp::ExitInstance()
{
	::CoUninitialize();
	
	if (m_gdiplusToken)
	{
		Gdiplus::GdiplusShutdown(m_gdiplusToken);
	}

	return CWinApp::ExitInstance();
}

DWORD CCuiBoxApp::GetPythonPid()
{
	DWORD dwPid = 0;
	CRegistry reg;
	if (reg.Open(REGPATH_CUIBOX, HKEY_CURRENT_USER))
	{
		dwPid = (DWORD)reg.ReadInt(REGKEY_PID);
		reg.Close();
	}
	return dwPid;
}

void CCuiBoxApp::ReplaceFiles()
{
	TCHAR szUpdateNew[1024];
	memset(szUpdateNew, 0x00, sizeof(szUpdateNew));
	_stprintf(szUpdateNew, _T("%s%s"), CUtil::GetAppRootPath(), APP_UPDATENEW_FILE);
	HANDLE hUpdateNew = CreateFile(szUpdateNew, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hUpdateNew != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hUpdateNew);
		TCHAR szUpdate[1024];
		memset(szUpdate, 0x00, sizeof(szUpdate));
		_stprintf(szUpdate, _T("%s%s"), CUtil::GetAppRootPath(), APP_UPDATE_FILE);

		DeleteFile((LPCTSTR)szUpdate);
		MoveFile(szUpdateNew, szUpdate);
	}
}

bool CCuiBoxApp::CheckMutex()
{
	HANDLE hMutex = ::CreateMutex(NULL, TRUE, APP_MUTEX);
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if (hMutex) ::ReleaseMutex(hMutex);
		PrintOut(_T(">> Already exists... (CuiBox)\r\n"));
		HWND hWnd = FindWindow(APP_CLASS, NULL);
		if (hWnd != NULL)
		{
			::SendMessage(hWnd, UM_MUTEX_OPEN, 0, 0L);
		}
		return false;
	}
	return true;
}