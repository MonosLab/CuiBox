#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxinet.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars

///////////////////////////////////////////////////////////////////////////
// GDI+
#include <gdiplus.h>
#include <gdiplusbase.h>
#include <gdipluscolor.h>
#include <gdipluspen.h>
#include <gdiplusbrush.h>
#include <gdipluspath.h>
#include <gdiplusgraphics.h>

using namespace Gdiplus;

#pragma comment(lib, "lib/gdiplus.lib")

// Get video card info.
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

#include <winrt/base.h>

#include <nvml.h>			// NVIDIA NVML API 함수를 사용하기 위한 헤더 파일 (※ CUDA Toolkit 설치 필요)
#include <Pdh.h>			// PDH(Performance Data Helper) API를 사용하기 위한 헤더 파일
//#pragma comment(lib, "x64/nvml.lib")
#pragma comment(lib, "pdh.lib")

#include <list>
#include <queue>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;


#include "Define.h"
#include "../MonosUtil/MonosUtil.h"
#include "resource.h"
#include "ResourceEx.h"



#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


