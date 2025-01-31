
// CuiBoxDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "CuiBox.h"
#include "CuiBoxDlg.h"
#include "Version.h"
#include "json/json.h"
#include "../MonosUtil/Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

_THREAD_INFO g_threadInfo{ false, false, false };

CWnd* g_pMainWnd = nullptr;
string g_strLang = "ko";
string g_strRunMode = "cpu";
string g_strSavePath = "ComfyUI\\output\\";
bool g_bUpdate = true;
bool g_bDebugMode = false;
bool g_bDefineSavePath = false;
DWORD g_dwGraphic = G_UNKNOWN;

double g_nCpu = 0.0;
double g_nGpu = 0.0;
double g_nRam = 0.0;
double g_nVram = 0.0;

HANDLE g_hStdOutRead = NULL;
HANDLE g_hStdOutWrite = NULL;

#ifdef PEEK_NAMED_PIPE
HANDLE g_hStdErrRead = NULL;
HANDLE g_hStdErrWrite = NULL;
_PIPE_INFO* g_ppiOutput = NULL;
_PIPE_INFO* g_ppiError = NULL;
#endif

PROCESS_INFORMATION g_pi = { 0x00, };

queue<CONSOLE_DATA*> g_qData = { };
bool g_bProcessing = false;

// 메시지 등록 과정
UINT g_uShellRestart = ::RegisterWindowMessage(_T("TaskbarCreated"));

// NVML Library
typedef nvmlReturn_t(*NvmlInit_t)();
typedef nvmlReturn_t(*NvmlShutdown_t)();
typedef const char* (*NvmlErrorString_t)(nvmlReturn_t result);
typedef nvmlReturn_t(*NvmlDeviceGetCount_t)(unsigned int* deviceCount);
typedef nvmlReturn_t(*NvmlDeviceGetHandleByIndex_t)(unsigned int index, nvmlDevice_t* device);
typedef nvmlReturn_t(*NvmlDeviceGetUtilizationRates_t)(nvmlDevice_t device, nvmlUtilization_t* utilization);

NvmlInit_t NvmlInit;
NvmlShutdown_t NvmlShutdown;
NvmlErrorString_t NvmlErrorString;
NvmlDeviceGetCount_t NvmlDeviceGetCount;
NvmlDeviceGetHandleByIndex_t NvmlDeviceGetHandleByIndex;
NvmlDeviceGetUtilizationRates_t NvmlDeviceGetUtilizationRates;
//////////////////////////////////////////////////////////////////////////////

void CallChangeSplitter(bool bHorz)
{
	::PostMessage(g_pMainWnd->m_hWnd, UM_CHANGE_SPLITTER, 0, bHorz ? 0L : 1L);
}

inline void PremultiplyBitmapAlpha(HDC hDC, HBITMAP hBmp)
{
	BITMAP bm = { 0 };
	GetObject(hBmp, sizeof(bm), &bm);
	BITMAPINFO* pBi = (BITMAPINFO*)_malloca(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
	if (pBi)
	{
		::ZeroMemory(pBi, sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
		pBi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		BOOL bRes = ::GetDIBits(hDC, hBmp, 0, bm.bmHeight, NULL, pBi, DIB_RGB_COLORS);
		if (!bRes || pBi->bmiHeader.biBitCount != 32) return;
		LPBYTE pBitData = (LPBYTE) ::LocalAlloc(LPTR, bm.bmWidth * bm.bmHeight * sizeof(DWORD));
		if (pBitData == NULL) return;
		LPBYTE pData = pBitData;
		::GetDIBits(hDC, hBmp, 0, bm.bmHeight, pData, pBi, DIB_RGB_COLORS);
		for (int y = 0; y < bm.bmHeight; y++) {
			for (int x = 0; x < bm.bmWidth; x++) {
				pData[0] = (BYTE)((DWORD)pData[0] * pData[3] / 255);
				pData[1] = (BYTE)((DWORD)pData[1] * pData[3] / 255);
				pData[2] = (BYTE)((DWORD)pData[2] * pData[3] / 255);
				pData += 4;
			}
		}
		::SetDIBits(hDC, hBmp, 0, bm.bmHeight, pBitData, pBi, DIB_RGB_COLORS);
		::LocalFree(pBitData);
	}
}

inline CString GetHTML(const int& RESOURCE_ID)
{
	CString str;
	HRSRC hrsrc = FindResource(NULL, MAKEINTRESOURCE(RESOURCE_ID), RT_HTML);
	if (hrsrc != NULL)
	{
		HGLOBAL pResource = LoadResource(NULL, hrsrc);
		if (pResource != NULL)
		{
			const char* psz = static_cast<const char*>(LockResource(pResource));
			if (psz != NULL)
			{
				int nLen = (int)strlen(psz);
				TCHAR* pszResource = new TCHAR[nLen + 1];
				memset(pszResource, 0x00, sizeof(TCHAR) * (nLen + 1));
				if (CharToWideChar((char*)psz, pszResource, nLen) != 0)
				{
					str = CString(pszResource);
				}
				delete[] pszResource;
				pszResource = NULL;
				UnlockResource(pResource);
				FreeResource(pResource);
				return str;
			}
			UnlockResource(pResource);
			FreeResource(pResource);
		}
	}
	return NULL;
}

UINT ProcNvidiaInfo(LPVOID lpVoid)
{
	g_threadInfo.exe_gpu = true;

	unsigned int i = 0;
	unsigned int deviceCount = 0;
	nvmlReturn_t result = NVML_SUCCESS;

	result = NvmlInit();
	if (NVML_SUCCESS != result)
	{
		PrintOutA(">> Failed to initialize NVML: %s\r\n", NvmlErrorString(result));
		g_threadInfo.exe_gpu = false;
		return ERR_NVML_INIT;
	}

	result = NvmlDeviceGetCount(&deviceCount);
	if (NVML_SUCCESS != result)
	{
		PrintOutA(">> Failed to get device count: %s\r\n", NvmlErrorString(result));
		NvmlShutdown();
		g_threadInfo.exe_gpu = false;
		return ERR_NVML_COUNT;
	}

	DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> Device count : %.ld\r\n"), deviceCount));

	double fGpu = 0.0;
	double fVram = 0.0;
	if (deviceCount > 0)
	{
		while (!g_threadInfo.is_exit)
		{
			fGpu = 0.0;
			fVram = 0.0;
			for (i = 0; i < deviceCount; ++i)
			{
				nvmlDevice_t device;
				result = NvmlDeviceGetHandleByIndex(i, &device);
				if (NVML_SUCCESS == result)
				{
					nvmlUtilization_t util;
					result = NvmlDeviceGetUtilizationRates(device, &util);
					if (NVML_SUCCESS == result)
					{
						fGpu += util.gpu;
						fVram += util.memory;
						DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> [%ld] GPU : %.2f%%, VRAM 사용량 : %.2f%%\r\n"), i, util.gpu, util.memory));
#if 0
						nvmlMemory_t memory;
						result = NvmlDeviceGetMemoryInfo(device, &memory);
						if (NVML_SUCCESS != result)
						{
							double memoryUsage = (double)(memory.used) / (1024 * 1024); // MB 단위로 변환
							double memoryTotal = (double)(memory.total) / (1024 * 1024); // MB 단위로 변환
							double memoryUsagePercentage = ((double)memory.used / (double)memory.total) * 100.0;

							DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> GPU : %ld, VRAM 사용량 : %ld MB / %ld MB (%ld%%)\r\n"), memoryUsage, memoryTotal, memoryUsagePercentage))
						}
						else
						{
							PrintOutA(">> Failed to get memory info for device #%ld : %s\r\n", i, NvmlErrorString(result));
						}
#endif
					}
					else
					{
						PrintOutA(">> Failed to get utilization rates #%ld : %s\r\n", i, NvmlErrorString(result));
					}
				}
				else
				{
					PrintOutA(">> Failed to get handle for device #%ld : %s\r\n", i, NvmlErrorString(result));
				}
			}

			g_nGpu = (double)(fGpu / deviceCount);
			g_nVram = (double)(fVram / deviceCount);
			DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> [-] GPU : %.2f%%, VRAM 사용량 : %.2f%%\r\n"), g_nGpu, g_nVram));

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	result = NvmlShutdown();
	if (NVML_SUCCESS != result)
	{
		PrintOutA(">> Failed to shut down NVML: %s\r\n", NvmlErrorString(result));
	}

	g_threadInfo.exe_gpu = false;
	return ERR_SUCCESS;
}

UINT ProcCpuInfo(LPVOID lpVoid)
{
	g_threadInfo.exe_cpu = true;

	PDH_HQUERY cpuQuery;
	PDH_HCOUNTER cpuTotal;
	PDH_FMT_COUNTERVALUE counterVal;
	
	if (PdhOpenQuery(NULL, NULL, &cpuQuery) != ERROR_SUCCESS)
	{
		PrintOut(_T(">> Failed to open PDH query.\r\n"));
		g_threadInfo.exe_cpu = false;
		return ERR_PDH_OPEN;
	}

	if (PdhAddEnglishCounter(cpuQuery, TEXT("\\Processor(_Total)\\% Processor Time"), NULL, &cpuTotal) != ERROR_SUCCESS)
	{
		PrintOut(_T(">> Failed to add PDH counter.\r\n"));
		PdhCloseQuery(cpuQuery);
		g_threadInfo.exe_cpu = false;
		return ERR_PDH_COUNTER;
	}

	if (PdhCollectQueryData(cpuQuery) != ERROR_SUCCESS)
	{
		PrintOut(_T(">> Failed to collect PDH query data. [first]\r\n"));
		PdhCloseQuery(cpuQuery);
		g_threadInfo.exe_cpu = false;
		return ERR_PDH_QUERY;
	}

	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);

	while (!g_threadInfo.is_exit)
	{
		if (PdhCollectQueryData(cpuQuery) != ERROR_SUCCESS)
		{
			PrintOut(_T(">> Failed to collect PDH query data. [other]\r\n"));
			PdhCloseQuery(cpuQuery);
			g_threadInfo.exe_cpu = false;
			return ERR_PDH_QUERY;
		}

		if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal) != ERROR_SUCCESS)
		{
			PrintOut(_T(">> Failed to get formatted counter value.\r\n"));
			PdhCloseQuery(cpuQuery);
			g_threadInfo.exe_cpu = false;
			return ERR_PDH_FORMAT;
		}

		g_nCpu = counterVal.doubleValue;

		// 메모리 사용률 측정
		if (!GlobalMemoryStatusEx(&memInfo))
		{
			PrintOut(_T(">> Failed to get memory status.\r\n"));
		}
		else
		{
			g_nRam = memInfo.dwMemoryLoad;
		}

		DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> CPU 사용률 : %.2f%%, 메모리 사용률 : %.2f%%\r\n"), g_nCpu, g_nRam));
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	PdhCloseQuery(cpuQuery);
	g_threadInfo.exe_cpu = false;

	return ERR_SUCCESS;
}

UINT ProcShowInfo(LPVOID lpVoid)
{
	CCuiBoxDlg* pDlg = (CCuiBoxDlg*)lpVoid;

	while (!g_threadInfo.is_exit)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		if (pDlg->m_pUsageInfo->GetSafeHwnd())
		{
			switch (g_dwGraphic)
			{
				case G_NVIDIA :
					pDlg->m_pUsageInfo->UpdateUsage(g_nCpu, g_nGpu, g_nRam, g_nVram);
					break;
				default :
					pDlg->m_pUsageInfo->UpdateUsage(g_nCpu, 0.0, g_nRam, 0.0);
					break;
			}
			
		}
	}

	return 0;
}

// CCuiBoxDlg dialog
CCuiBoxDlg::CCuiBoxDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CUIBOX_DIALOG, pParent)
	, m_pRichEdit(NULL)
	, m_nLastLine(0)
	, m_nRceWidth(SIZE_RCE_CONSOLEW)
	, m_nRceHeight(SIZE_RCE_CONSOLEH)
	, m_pLogoDC(NULL)
	, m_szIcon(0, 0)
	, m_hBitmap(NULL)
	, m_hNvml(NULL)
	, m_nShowConsole(MODE_CHECKED)
	, m_nPosConsole(MODE_CHECKED)
	, m_bStartup(false)
	, m_bTrayStatus(false)
{
	//m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	TCHAR szPath[512];
	memset(szPath, 0x00, sizeof(szPath));
	_stprintf_s(szPath, _T("%s%s"), CUtil::GetAppRootPath(), LOGO_IMAGE);
	PrintOut(_T(">> [CuiBox] IconPath : %s\r\n"), szPath);
	Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(szPath, FALSE);
	bitmap->GetHICON(&m_hIcon);
	// title
	memset(m_szTitle, 0x00, sizeof(m_szTitle));
	_stprintf_s(m_szTitle, _T("%s"), APP_NAME);

	m_rect = GetInitWindowPos();
}

CCuiBoxDlg::~CCuiBoxDlg()
{
	g_threadInfo.is_exit = true;

	if (m_bTrayStatus)
	{
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(nid);
		nid.hWnd = m_hWnd;
		nid.uID = IDR_MAINFRAME;
		nid.uFlags = NULL;
		Shell_NotifyIcon(NIM_DELETE, &nid);

		m_bTrayStatus = false;
	}

	if (m_hNvml)
	{
		FreeLibrary(m_hNvml);
		m_hNvml = NULL;
	}
}

void CCuiBoxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_CLOSE, m_btnClose);
	DDX_Control(pDX, IDC_BTN_MAX, m_btnMax);
	DDX_Control(pDX, IDC_BTN_MIN, m_btnMin);
	DDX_Control(pDX, IDC_BTN_SHOW_CONSOLE, m_btnShowConsole);
	DDX_Control(pDX, IDC_BTN_POS_CONSOLE, m_btnPosConsole);
}

BEGIN_MESSAGE_MAP(CCuiBoxDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_DESTROY()
	//ON_WM_NCPAINT()
	ON_WM_NCHITTEST()
	ON_WM_NCCALCSIZE()
	ON_WM_NCACTIVATE()
	ON_WM_NCLBUTTONDBLCLK()
	ON_MESSAGE(UM_CHANGE_SPLITTER, OnChangeSplitter)
	ON_MESSAGE(UM_EXEC_NAVIGATE, OnNavigate)
	ON_BN_CLICKED(IDC_BTN_MIN, &CCuiBoxDlg::OnBnClickedBtnMin)
	ON_BN_CLICKED(IDC_BTN_MAX, &CCuiBoxDlg::OnBnClickedBtnMax)
	ON_BN_CLICKED(IDC_BTN_CLOSE, &CCuiBoxDlg::OnBnClickedBtnClose)
	ON_BN_CLICKED(IDC_BTN_SHOW_CONSOLE, &CCuiBoxDlg::OnBnClickedBtnShowConsole)
	ON_BN_CLICKED(IDC_BTN_POS_CONSOLE, &CCuiBoxDlg::OnBnClickedBtnPosConsole)
	ON_WM_ERASEBKGND()
	ON_WM_GETMINMAXINFO()
	ON_COMMAND(UDM_OPEN, OnOpenMenu)
	ON_COMMAND(UDM_OPEN_OUTPUT, OnOpenOutputMenu)
	ON_COMMAND(UDM_CLOSE, OnCloseMenu)
	ON_COMMAND(UDM_RESET, OnResetMenu)
	ON_COMMAND(UDM_STARTUP, OnRegisterStartup)
	ON_COMMAND(UDM_APPLY_CHANGES, OnApplyChanges)
	ON_COMMAND(UDM_RUN_CPU, OnRunCpu)
	ON_COMMAND(UDM_RUN_GPU, OnRunGpu)
	ON_COMMAND(UDM_SET_KO, OnLangKo)
	ON_COMMAND(UDM_SET_EN, OnLangEn)
	ON_COMMAND(UDM_SET_JA, OnLangJa)
	ON_COMMAND(UDM_SET_ZH, OnLangZh)
	ON_COMMAND(UDM_SET_VI, OnLangVi)
	ON_MESSAGE(UM_TRAYICON, OnTrayIcon)
	ON_MESSAGE(UM_MUTEX_OPEN, OnMutexOpen)
	ON_REGISTERED_MESSAGE(g_uShellRestart, OnTrayShow)
END_MESSAGE_MAP()

void CCuiBoxDlg::PreSubclassWindow()
{
	DWORD dwStyle = /*WS_OVERLAPPED |*/ WS_THICKFRAME | WS_CLIPSIBLINGS | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
	// borderless shadow
	ModifyStyle(WS_CAPTION, dwStyle, SWP_FRAMECHANGED);

	CDialogEx::PreSubclassWindow();
}

void CCuiBoxDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	if (m_pLogoDC->GetSafeHdc())
	{
		ReleaseDC(m_pLogoDC);
		m_pLogoDC = NULL;
	}
	
	if (!m_imgLogo.IsNull())
	{
		m_imgLogo.Detach();
	}

	if (m_pWebBrowser != nullptr)
	{
		m_pWebBrowser.reset();
	}

	CloseConsole();
}

void CCuiBoxDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	HMONITOR hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFOEX mi;
	mi.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMon, &mi);

	RECT rc = mi.rcWork;
	RECT rcm = mi.rcMonitor;

	UINT nWorkAreaHeight = abs(rc.bottom - rc.top);
	UINT nWorkAreaWidth = abs(rc.right - rc.left);
	UINT nMonitorHeight = abs(rcm.bottom - rcm.top);
	UINT nMonitorWidth = abs(rcm.right - rcm.left);

	// Get taskbar info.
	if ((nMonitorHeight - nWorkAreaHeight) >= PX_TASKBAR_HEIGHT)
	{
		lpMMI->ptMaxPosition.x = PX_CALIBRATE;
		lpMMI->ptMaxPosition.y = /*rc.top +*/ PX_CALIBRATE;
	}
	else if (nMonitorWidth - nWorkAreaWidth >= PX_TASKBAR_HEIGHT)
	{
		lpMMI->ptMaxPosition.x = /*rc.left +*/ PX_CALIBRATE;
		lpMMI->ptMaxPosition.y = PX_CALIBRATE;
	}

	lpMMI->ptMaxSize.x = abs(rc.right - rc.left) + 15;		// 그림자 여백(20) 보정
	lpMMI->ptMaxSize.y = abs(rc.bottom - rc.top) + 17;		// 37 - 그림자 여백(20) 보정

	lpMMI->ptMinTrackSize.x = WND_MIN_X;
	lpMMI->ptMinTrackSize.y = WND_MIN_Y;

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

// 사용 안함. (윈도우의 그림자 기능 때문에...)
void CCuiBoxDlg::OnNcPaint()
{
	CRect rcWindow;
	GetWindowRect(&rcWindow);
	rcWindow.NormalizeRect();
	rcWindow.OffsetRect(-rcWindow.left, -rcWindow.top);

	CDC* pDC = GetWindowDC();
	
	// 테두리 영역만을 위해 가운데 영역 제외
	DWORD dwArea = pDC->ExcludeClipRect(rcWindow.left + SIZE_EXCLUDE, rcWindow.top + SIZE_EXCLUDE, rcWindow.right - SIZE_EXCLUDE, rcWindow.bottom - SIZE_EXCLUDE);
	switch (dwArea)
	{
		case NULLREGION :
			PrintOut(_T(">> Null region.\r\n"), GetLastError());
			break;
		case SIMPLEREGION :
		case COMPLEXREGION :
			// 테두리에 그릴 내용
			pDC->FillSolidRect(&rcWindow, TTB_BRD_BRUSH);
			break;
		case ERROR :
			PrintOut(_T(">> Error : %ld\r\n"), GetLastError());
			break;
	}

	ReleaseDC(pDC);
}

LRESULT CCuiBoxDlg::OnNcHitTest(CPoint point)
{
	CRect rc;
	long x = 0;
	long y = 0;

	x = point.x;
	y = point.y;

	// 리사이즈 영역 체크
	GetWindowRect(&rc);

	if ((rc.top <= y) && (y <= rc.top + TTB_BORDER))
	{
		if ((rc.left <= x) && (x <= rc.left + TTB_BORDER))
		{
			return HTTOPLEFT;
		}
		if ((rc.right >= x) && (x >= rc.right - TTB_BORDER))
		{
			return HTTOPRIGHT;
		}
		return HTTOP;
	}
	else if ((rc.bottom >= y) && (y >= rc.bottom - TTB_BORDER))
	{
		if ((rc.left <= x) && (x <= rc.left + TTB_BORDER))
		{
			return HTBOTTOMLEFT;
		}
		if ((rc.right >= x) && (x >= rc.right - TTB_BORDER))
		{
			return HTBOTTOMRIGHT;
		}
		return HTBOTTOM;
	}
	else if ((rc.left <= x) && (x <= rc.left + TTB_BORDER))
	{
		if ((rc.top <= y) && (y <= rc.top + TTB_BORDER))
		{
			return HTTOPLEFT;
		}
		if ((rc.bottom >= y) && (y >= rc.bottom - TTB_BORDER))
		{
			return HTBOTTOMLEFT;
		}
		return HTLEFT;
	}
	else if ((rc.right >= x) && (x >= rc.right - TTB_BORDER))
	{
		if ((rc.top <= y) && (y <= rc.top + TTB_BORDER))
		{
			return HTTOPRIGHT;
		}
		if ((rc.bottom >= y) && (y >= rc.bottom - TTB_BORDER))
		{
			return HTBOTTOMRIGHT;
		}
		return HTRIGHT;
	}

	// Non-Client 영역 체크
	rc.bottom = rc.top + TTB_HEIGHT;

	POINT pt;
	pt.x = x;
	pt.y = y;

	if (rc.PtInRect(pt))
	{
		return HTCAPTION;
	}

	return CDialogEx::OnNcHitTest(point);
}

void CCuiBoxDlg::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	CDialogEx::OnNcCalcSize(bCalcValidRects, lpncsp);

	// 창의 좌표
	lpncsp->rgrc[0].top -= TTB_BORDER_TOP_MARGIN;
	//lpncsp->rgrc[0].right += 0;
	//lpncsp->rgrc[0].bottom += 0;
	//lpncsp->rgrc[0].left += 0;
}

BOOL CCuiBoxDlg::OnNcActivate(BOOL bActive)
{
	CRect rect;
	GetClientRect(rect);
	rect.bottom = rect.top + TTB_BORDER_TOP_MARGIN;
	InvalidateRect(rect, FALSE);

	return CDialogEx::OnNcActivate(bActive);
}

void CCuiBoxDlg::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
	if (this->IsZoomed())
	{
		PostMessage(WM_SYSCOMMAND, SC_RESTORE);
	}
	else
	{
		PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE);
	}

	//CDialogEx::OnNcLButtonDblClk(nHitTest, point);
}

// CCuiBoxDlg message handlers
BOOL CCuiBoxDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	g_pMainWnd = this;

	RegistCallback(CallChangeSplitter); // Callback 등록
	GetVideoCardInfo();
	// [ 테스트용 ]
	//g_dwGraphic = G_NVIDIA;
	GetConfig();
	if (g_bUpdate)
	{
		if (CheckVersion())
		{
			PrintOut(_T(">> You have to update files.\r\n"));
			TCHAR szUpdate[512];
			memset(szUpdate, 0x00, sizeof(szUpdate));
			_stprintf_s(szUpdate, _T("%s%s"), (LPCTSTR)CUtil::GetAppRootPath(), APP_UPDATE_FILE);
			PrintOut(_T(">> Update : %s\r\n"), szUpdate);
			// 1. 업데이트
			ShellExecute(NULL, _T("open"), szUpdate, APP_EXE_FILE, NULL, SW_SHOWDEFAULT);
			// 2. 프로그램 종료
			CDialogEx::OnCancel();
		}
	}
	
	SetTrayIcon();

	// Initializes the window rectangle.
	MoveWindow(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height());
	CreateControls();

	RunConsole();
	InitBackgroundImage();
	
	switch (g_dwGraphic)
	{
		case G_NVIDIA:
			PrintOut(_T("■■■■■■ NVIDIA Geforce ■■■■■■\r\n"));
			if (LoadNvidiaLib())
			{
				AfxBeginThread(ProcNvidiaInfo, this);
			}
			else
			{
				AfxMessageBox(CResourceEx::GetInstance()->LoadStringEx(IDS_INSTALL_NVIDIA_DRV));
			}			
			break;
		case G_AMD:
			PrintOut(_T("■■■■■■ AMD Radeon ■■■■■■\r\n"));
			PrintOut(_T("Only NVIDIA graphics cards display GPU and VRAM usage percentages.\r\n"));
			break;
		case G_INTEL:
			PrintOut(_T("■■■■■■ INTEL Graphics (Arc, UHD, Iris) ■■■■■■\r\n"));
			PrintOut(_T("Only NVIDIA graphics cards display GPU and VRAM usage percentages.\r\n"));
			break;
		default:
			PrintOut(_T("■■■■■■ Unknown Graphics ■■■■■■\r\n"));
			PrintOut(_T("Only NVIDIA graphics cards display GPU and VRAM usage percentages.\r\n"));
			break;
	}
	AfxBeginThread(ProcCpuInfo, this);
	AfxBeginThread(ProcShowInfo, this);

	return TRUE;
}

void CCuiBoxDlg::OnInit()
{
	// Font 설정
	LOGFONT lf;
	memset(&lf, NULL, sizeof(LOGFONT));
	lf.lfHeight = -MulDiv(MN_FONT_TITLE_SIZE * 10, 72, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY));
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = HANGEUL_CHARSET;
	lf.lfOutPrecision = OUT_STROKE_PRECIS;
	lf.lfClipPrecision = CLIP_STROKE_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;// PROOF_QUALITY;
	lf.lfPitchAndFamily = (FIXED_PITCH | FF_MODERN);
	_stprintf(lf.lfFaceName, _T("%s"), (LPCTSTR)CResourceEx::GetInstance()->LoadStringEx(IDS_FONT_DEFAULTFACE));
	//_stprintf_s(lf.lfFaceName, _T("Malgun Gothic"));
	m_FontTitleThin.CreatePointFontIndirect(&lf);
	lf.lfWeight = FW_BOLD/*FW_HEAVY*/;
	m_FontTitleBold.CreatePointFontIndirect(&lf);
	lf.lfHeight = -MulDiv(MN_FONT_DEFAULTSIZE * 10, 72, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY));
	lf.lfWeight = FW_NORMAL;
	m_FontDefault.CreatePointFontIndirect(&lf);

	TCHAR szPath[512];
	memset(szPath, 0x00, sizeof(szPath));
	_stprintf_s(szPath, _T("%s%s"), CUtil::GetAppRootPath(), LOGO_IMAGE);

	// Load title bar icon (png)
	if (m_imgLogo.Load(szPath) == S_OK)
	{
		// Get width and height of the image(png)
		m_szIcon.cx = m_imgLogo.GetWidth();
		m_szIcon.cy = m_imgLogo.GetHeight();

		if (m_pLogoDC->GetSafeHdc())
		{
			ReleaseDC(m_pLogoDC);
		}

		HDC hdc = m_imgLogo.GetDC();
		m_pLogoDC = CDC::FromHandle(hdc);

		CDC memDC;
		memDC.CreateCompatibleDC(this->GetDC());
		PremultiplyBitmapAlpha(memDC, m_imgLogo);
	}
}

BOOL CCuiBoxDlg::OnEraseBkgnd(CDC* pDC)
{
#if true
	// 상단 배경의 깜빡임 방지...
	// OnPaint에서 6px(TTB_BORDER_TOP_MARGIN) 이상부터 그려지기 때문에 배경의 깜빡임 방지를 위해 아래의 코드를 삽입.
	CRect rc;
	GetClientRect(&rc);
	rc.bottom = TTB_BORDER_TOP_MARGIN;
	pDC->FillSolidRect(rc, TTB_BGC_BRUSH);
	
	return FALSE;			// Prevents browser flickering when resizing.
#else
	return CDialogEx::OnEraseBkgnd(pDC);
#endif
}

void CCuiBoxDlg::InitBackgroundImage(int cx, int cy)
{
	CDC* pDC = GetDC();
	if (pDC)
	{
		CRect rc;
		GetClientRect(&rc);
		rc.bottom = rc.top + TTB_HEIGHT;

		if (m_hBitmap)
		{
			DeleteObject(m_hBitmap);
			m_hBitmap = NULL;
		}

		m_hBitmap = CreateCompatibleBitmap(pDC->m_hDC, rc.Width(), rc.Height());
		if (m_hBitmap != NULL)
		{
			CDC memDC;
			CBrush brush(TTB_BGC_BRUSH);
			memDC.CreateCompatibleDC(pDC);

			HGDIOBJ hOldObj = memDC.SelectObject(m_hBitmap);
			int nOldBkMode = memDC.SetBkMode(TRANSPARENT);
			memDC.FillRect(rc, &brush);
			memDC.SetBkMode(nOldBkMode);
			memDC.SelectObject(hOldObj);
			memDC.DeleteDC();
		}
		else
		{
			PrintOut(_T(">> Bitmap is null."));
		}

		ReleaseDC(pDC);
	}

	SetWndPos();
}

void CCuiBoxDlg::SetWndPos()
{
	CRect rc;
	GetClientRect(&rc);
#if 0
	if (m_ctlSplitter.GetSafeHwnd() != nullptr && m_nShowConsole == MODE_UNCHECKED)
	{
		CRect rcTemp, rcWeb, rcPos;
		GetClientRect(&rcTemp);
		rcTemp.top += (TTB_HEIGHT + 1);

		switch (m_nPosConsole)
		{
			case MODE_CHECKED :
				rcPos = m_ctlSplitter.GetWindowRect(0, 0);
				if (rcTemp.Width() != rcPos.Width())
				{
					m_ctlSplitter.SetColumnWidthForStatic(0, rcTemp.Width());
					m_ctlSplitter.SetColumnWidthForStatic(1, 0);
				}
				break;
			case MODE_UNCHECKED :
			default:
				rcPos = m_ctlSplitter.GetWindowRect(0, 0);
				if (rcTemp.Height() != rcPos.Height())
				{
					m_ctlSplitter.SetRowHeightForStatic(0, rcTemp.Height());
					m_ctlSplitter.SetRowHeightForStatic(1, 0);
				}
				break;
		}

		if (m_pWebBrowser->GetSafeHwnd())
		{
			rcWeb = m_ctlSplitter.GetWindowRect(0, 0);
			m_pWebBrowser->MoveWindow(rcWeb);
		}
	}
#endif
	RedrawControls(rc, CALLER::CALL_SPLITTER);

	int nGap = 5;
	int nX = 0;
	int nY = TTB_BORDER;

	nX = rc.Width() - (nGap + TTB_BTN_X);
	if (m_btnClose.GetSafeHwnd())
	{
		m_btnClose.MoveWindow(nX, nY, TTB_BTN_X, TTB_BTN_Y);
		m_btnClose.SetImage(PATH_BTN_CLOSE);
		m_btnClose.SetBkImage(m_hBitmap);
		m_btnClose.Invalidate();
	}
	nX -= TTB_BTN_X;
	if (m_btnMax.GetSafeHwnd())
	{
		m_btnMax.MoveWindow(nX, nY, TTB_BTN_X, TTB_BTN_Y);
		m_btnMax.SetImage(PATH_BTN_MAX);
		m_btnMax.SetBkImage(m_hBitmap);
		m_btnMax.Invalidate();
	}
	nX -= TTB_BTN_X;
	if (m_btnMin.GetSafeHwnd())
	{
		m_btnMin.MoveWindow(nX, nY, TTB_BTN_X, TTB_BTN_Y);
		m_btnMin.SetImage(PATH_BTN_MIN);
		m_btnMin.SetBkImage(m_hBitmap);
		m_btnMin.Invalidate();
	}
	nX -= TTB_BTN_X;
	if (m_btnShowConsole.GetSafeHwnd())
	{
		m_btnShowConsole.MoveWindow(nX, nY, TTB_BTN_X, TTB_BTN_Y);
		m_btnShowConsole.SetImage(PATH_BTN_SHOW_CONSOLE);
		m_btnShowConsole.SetBkImage(m_hBitmap);
		if (m_nShowConsole == MODE_CHECKED)
		{
			m_btnShowConsole.SetCheck(BST_CHECKED);
		}
		else
		{
			m_btnShowConsole.SetCheck(BST_UNCHECKED);
		}
		m_btnShowConsole.Invalidate();
	}
	nX -= TTB_BTN_X;
	if (m_btnPosConsole.GetSafeHwnd())
	{
		m_btnPosConsole.MoveWindow(nX, nY, TTB_BTN_X, TTB_BTN_Y);
		m_btnPosConsole.SetImage(PATH_BTN_POS_CONSOLE);
		m_btnPosConsole.SetBkImage(m_hBitmap);
		if (m_nPosConsole == MODE_CHECKED)
		{
			m_btnPosConsole.SetCheck(BST_CHECKED);
		}
		else
		{
			m_btnPosConsole.SetCheck(BST_UNCHECKED);
		}
		m_btnPosConsole.Invalidate();
	}
	nX -= TTB_USAGE_INFO;
	if (m_pUsageInfo->GetSafeHwnd())
	{
		m_pUsageInfo->MoveWindow(nX, 6, TTB_USAGE_INFO, 29);
		m_pUsageInfo->SetRamInfo(1024, 1024);
		m_pUsageInfo->Invalidate();
	}
}

void CCuiBoxDlg::OnPaint()
{
#if 0
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
#else
	CPaintDC dc(this);

	CRect rc;
	GetClientRect(rc);

	int nX = rc.left;
	int nY = rc.top;
	int nWidth = rc.Width();
	int nHeight = nY + TTB_HEIGHT + 1;	// 아래의 선때문에 높이에 +1을 더해준다.

	// Double buffering
	CDC bmDC;
	CDC memDC;
	CBitmap bitmap;
	CBitmap* pOldBitmap;
	CPen* pOldPen;
	CBrush* pOldBrush;
	CPen pen(PS_SOLID, 0, TTB_BGC_PEN);
	CBrush brush(TTB_BGC_BRUSH);

	// Create DC
	bmDC.CreateCompatibleDC(&dc);
	bitmap.CreateCompatibleBitmap(&dc, nWidth, nHeight);
	pOldPen = bmDC.SelectObject(&pen);
	pOldBrush = bmDC.SelectObject(&brush);
	pOldBitmap = bmDC.SelectObject(&bitmap);
	// Draws the background and bottom line.
	bmDC.PatBlt(nX, nY, nWidth, nHeight, PATCOPY);
	bmDC.MoveTo(0, nHeight - 1);
	bmDC.LineTo(nWidth, nHeight - 1);
	// Draw an icon.
	memDC.CreateCompatibleDC(&bmDC);

	if (!m_pLogoDC->GetSafeHdc())
	{
		TCHAR szPath[512];
		memset(szPath, 0x00, sizeof(szPath));
		_stprintf_s(szPath, _T("%s%s"), CUtil::GetAppRootPath(), LOGO_IMAGE);

		if (m_imgLogo.Load(szPath) == S_OK)
		{
			// Get width and height of the image(png)
			m_szIcon.cx = m_imgLogo.GetWidth();
			m_szIcon.cy = m_imgLogo.GetHeight();
		}

		HDC hdc = m_imgLogo.GetDC();
		m_pLogoDC = CDC::FromHandle(hdc);
		PremultiplyBitmapAlpha(memDC, m_imgLogo);
	}

	BLENDFUNCTION TTB_ICON_BF = { AC_SRC_OVER, 0x00, 0xFF, AC_SRC_ALPHA };
	bmDC.AlphaBlend(TTB_ICON_MARGIN, TTB_ICON_MARGIN + 2, m_szIcon.cx, m_szIcon.cy, m_pLogoDC, 0, 0, m_szIcon.cx, m_szIcon.cy, TTB_ICON_BF);

	int nPos = TTB_ICON_MARGIN;
	// Draw a text
	CRect rcText = rc;
	rcText.left += (m_szIcon.cx + nPos + 5);
	//rcText.top += TTB_TF_PLUS_BORDER;
	rcText.top += TTB_TEXT_TOP_POS;

	HGDIOBJ hOldObj;
	if (_tcslen(APP_FOR_COMFYUI) > 0)
	{
		CRect rectText(0, 0, 0, 0);
		hOldObj = bmDC.SelectObject(m_FontTitleBold.GetSafeHandle());
		bmDC.DrawText(m_szTitle, rcText, DT_LEFT | DT_END_ELLIPSIS);
		bmDC.DrawText(m_szTitle, -1, &rectText, DT_LEFT | DT_CALCRECT);
		hOldObj = bmDC.SelectObject(m_FontTitleThin.GetSafeHandle());
		rcText.left += rectText.Width() + 5;
		bmDC.DrawText(APP_FOR_COMFYUI, rcText, DT_LEFT | DT_END_ELLIPSIS);
	}
	else
	{
		hOldObj = bmDC.SelectObject(m_FontTitleBold.GetSafeHandle());
		bmDC.DrawText(m_szTitle, rcText, DT_LEFT | DT_END_ELLIPSIS);
	}

	dc.BitBlt(nX, nY, nWidth, nHeight, &bmDC, 0, 0, SRCCOPY);

	// Release the used DC
	bmDC.SelectObject(hOldObj);
	bmDC.SelectObject(pOldPen);
	bmDC.SelectObject(pOldBrush);
	bmDC.SelectObject(pOldBitmap);
	bitmap.DeleteObject();
	memDC.DeleteDC();
	bmDC.DeleteDC();
#endif
}

HCURSOR CCuiBoxDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

bool CCuiBoxDlg::CreateControls()
{
	CRect rc;
	GetClientRect(&rc);
	rc.top += (TTB_HEIGHT + 1);		// bottom line = 1
	
	m_pUsageInfo = new CUsageInfoWnd(g_dwGraphic);
	m_pUsageInfo->Create(_T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, CRect(0, 0, 0, 0), this, IDC_STT_USAGEINFO);
	
	if (m_ctlSplitter.Create(this, WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, rc, IDC_CTL_SPLITTER))
	{
		int nWidth = 0;
		int nHeight = 0;

		m_sttBrowser.Create(_T(""), WS_CLIPCHILDREN | WS_VISIBLE /* | WS_CLIPSIBLINGS | SS_ETCHEDHORZ*/, CRect(0, 0, 0, 0), &m_ctlSplitter, IDC_STT_LWINDOW);
		m_sttConsole.Create(_T(""), WS_CLIPCHILDREN | WS_VISIBLE /* | WS_CLIPSIBLINGS | SS_ETCHEDHORZ*/, CRect(0, 0, 0, 0), &m_ctlSplitter, IDC_STT_RWINDOW);

		switch (m_nPosConsole)
		{
			case MODE_CHECKED :
				m_ctlSplitter.AddRow();
				m_ctlSplitter.AddColumn();
				m_ctlSplitter.AddColumn();
				m_ctlSplitter.SetWindow(0, 0, m_sttBrowser.GetSafeHwnd());
				m_ctlSplitter.SetWindow(0, 1, m_sttConsole.GetSafeHwnd());
				m_ctlSplitter.SetWindowMinWidth(CHILD_MIN_X);
				m_ctlSplitter.SetWindowMinHeight(CHILD_MIN_Y);
				m_ctlSplitter.ShowBorder(false);
				m_ctlSplitter.SetSplitterBorderWidth(SIZE_SPLITTER_BORDER, 0, 0);
				if (m_nShowConsole == MODE_CHECKED)
				{
					// Index of col :    0     1
					nWidth = rc.Width() - m_nRceWidth - (SIZE_SPLITTER_GAP * 2);	// Border(4)    : |~~~~~||~~~|
					m_ctlSplitter.SetColumnWidthForStatic(0, nWidth);
					m_ctlSplitter.SetColumnWidthForStatic(1, m_nRceWidth);
				}
				else
				{
					m_ctlSplitter.SetColumnWidthForStatic(0, rc.Width());
					m_ctlSplitter.SetColumnWidthForStatic(1, 0);
				}
				break;
			case MODE_UNCHECKED :
				m_ctlSplitter.AddColumn();
				m_ctlSplitter.AddRow();
				m_ctlSplitter.AddRow();
				m_ctlSplitter.SetWindow(0, 0, m_sttBrowser.GetSafeHwnd());
				m_ctlSplitter.SetWindow(1, 0, m_sttConsole.GetSafeHwnd());
				m_ctlSplitter.SetWindowMinWidth(CHILD_MIN_X);
				m_ctlSplitter.SetWindowMinHeight(CHILD_MIN_Y);
				m_ctlSplitter.ShowBorder(false);
				m_ctlSplitter.SetSplitterBorderWidth(SIZE_SPLITTER_BORDER, 0, 0);
				if (m_nShowConsole == MODE_CHECKED)
				{
					// Index of row :    0     1
					nHeight = rc.Height() - m_nRceHeight - (SIZE_SPLITTER_GAP * 2);	// Border(4)    : |~~~~~||~~~|
					m_ctlSplitter.SetRowHeightForStatic(0, nHeight);
					m_ctlSplitter.SetRowHeightForStatic(1, m_nRceHeight);
				}
				else
				{
					m_ctlSplitter.SetRowHeightForStatic(0, rc.Height());
					m_ctlSplitter.SetRowHeightForStatic(1, 0);
				}
				break;
		}
		
		m_SplitterStyle.Install(&m_ctlSplitter);

		for (int r = 0; r < m_ctlSplitter.GetNumberRow() - 1; ++r)
		{
			m_ctlSplitter.ActivateHorzSplitter(r, false);
		}
		for (int c = 0; c < m_ctlSplitter.GetNumberColumn() - 1; ++c)
		{
			m_ctlSplitter.ActivateVertSplitter(c, true);
		}

		m_ctlSplitter.SetResizeMode(CSplitterCtrl::ResizeStatic);
		m_ctlSplitter.Update();
	}

	m_pWebBrowser = std::make_unique<CWebBrowser>();
	if (m_pWebBrowser != nullptr)
	{
		CRect rcCli;
		m_sttBrowser.GetClientRect(rcCli);
		CWnd* pBrowser = FromHandle(m_sttBrowser.GetSafeHwnd());
		CString strData = GetHTML(IDR_HTML_INTRO);
		m_pWebBrowser->CreateAsync(WS_VISIBLE | WS_CHILD, rcCli, pBrowser, IDC_CTL_BROWSER,
			[this, pBrowser, strData]() {
				m_pWebBrowser->SetParentView(pBrowser);
				m_pWebBrowser->Settings(TU_CUI_BROWSER, TRUE, TRUE, TRUE);
				m_pWebBrowser->NavigateToString(strData);
			});
	}

	m_pRichEdit = new CRichEditCtrl();
	if (m_pRichEdit != nullptr)
	{
		CRect rcCli;
		m_sttConsole.GetClientRect(rcCli);
		m_pRichEdit->Create(WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL, rcCli , &m_sttConsole, IDC_CTL_RICHEDIT);
		m_pRichEdit->SetBackgroundColor(FALSE, RCE_BG_COLOR);
		CHARFORMAT cf;
		memset(&cf, 0, sizeof(CHARFORMAT));
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_FACE | CFM_COLOR;
		cf.crTextColor = RCE_FG_COLOR;
		_tcscpy(cf.szFaceName, _T("Courier New"));	// or "Times New Roman", "Arial" etc etc
		m_pRichEdit->SetDefaultCharFormat(cf);
		m_pRichEdit->SendMessage(EM_SETLANGOPTIONS, 0, (LPARAM)(m_pRichEdit->SendMessage(EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOFONT));
		m_pRichEdit->ShowWindow(SW_SHOW);
	}

	if (m_ctlSplitter.GetSafeHwnd())
	{
		m_ctlSplitter.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}

	return true;
}

void CCuiBoxDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

#if 0
	CRect rect_window;
	this->GetWindowRect(&rect_window);
	rect_window.NormalizeRect();
	rect_window.OffsetRect(-rect_window.left, -rect_window.top);

	CRgn rgn;
	rgn.CreateRoundRectRgn(0, 5, rect_window.Width(), rect_window.Height() - 5, 0, 0);
	this->SetWindowRgn(static_cast<HRGN>(rgn.GetSafeHandle()), TRUE);
#endif

	InitBackgroundImage();

	if (cx > 0 && cy > 0)
	{
		RegisterWindowPos();
	}
}

void CCuiBoxDlg::OnMove(int x, int y)
{
	CDialogEx::OnMove(x, y);

	CRect r;
	BOOL bSpi = SystemParametersInfo(SPI_GETWORKAREA, 0, r, 0);
	if (bSpi)
	{
		if (x > 0 && y > 0 && y < r.bottom)
		{
			RegisterWindowPos();
		}
	}
}

void CCuiBoxDlg::RedrawControls(CRect rect, UINT nCaller)
{
	if (m_ctlSplitter.GetSafeHwnd())
	{
		int nX = 0;
		int nY = rect.top + TTB_HEIGHT + 1;
		int nWidth = rect.Width();
		int nHeight = rect.Height() - (TTB_HEIGHT + 1);

		m_ctlSplitter.MoveWindow(nX, nY, nWidth, nHeight);

		switch (nCaller)
		{
			case CALLER::CALL_MAIN:
				{
					CRect rc = m_ctlSplitter.GetWindowRect(0, 0);
					m_pWebBrowser->MoveWindow(rc);

					CRect rcTemp = m_ctlSplitter.GetWindowRect(0, 1);
					CRect rc2 = CRect(0, 0, rcTemp.Width(), rcTemp.Height());
					m_pRichEdit->MoveWindow(rc2);
				}
				break;
			case CALLER::CALL_SPLITTER:
				if(m_nPosConsole == MODE_CHECKED)		// Horizontal
				{
					if (m_nShowConsole == MODE_CHECKED)	// Show
					{
						CRect rc = m_ctlSplitter.GetWindowRect(0, 0);
						DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> [Checked1] LEFT : %ld, TOP : %ld, RIGHT : %ld, BOTTOM : %ld\r\n"), rc.left, rc.top, rc.right, rc.bottom));
						m_pWebBrowser->MoveWindow(rc);

						CRect rcTemp = m_ctlSplitter.GetWindowRect(0, 1);
						CRect rc2 = CRect(0, 0, rcTemp.Width(), rcTemp.Height());
						DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> [Checked2] LEFT : %ld, TOP : %ld, RIGHT : %ld, BOTTOM : %ld\r\n"), rc2.left, rc2.top, rc2.right, rc2.bottom));
						m_pRichEdit->MoveWindow(rc2);

						if (rc2.Width() > 0)
						{
							m_nRceWidth = rc2.Width();
						}
					}
					else	// Hide
					{
						CRect rcTemp;
						GetClientRect(&rcTemp);
						m_ctlSplitter.SetColumnWidthForStatic(0, rcTemp.Width());
						m_ctlSplitter.SetColumnWidthForStatic(1, 0);
						m_pWebBrowser->MoveWindow(rcTemp);
						m_pRichEdit->MoveWindow(0, 0, rcTemp.Width(), 0);
					}
				}
				else									// Vertical
				{
					if (m_nShowConsole == MODE_CHECKED)	// Show
					{
						CRect rc = m_ctlSplitter.GetWindowRect(0, 0);
						DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> [Unchecked1] LEFT : %ld, TOP : %ld, RIGHT : %ld, BOTTOM : %ld\r\n"), rc.left, rc.top, rc.right, rc.bottom));
						m_pWebBrowser->MoveWindow(rc);

						CRect rcTemp = m_ctlSplitter.GetWindowRect(1, 0);
						CRect rc2 = CRect(0, 0, rcTemp.Width(), rcTemp.Height());
						DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> [Unchecked2] LEFT : %ld, TOP : %ld, RIGHT : %ld, BOTTOM : %ld\r\n"), rc2.left, rc2.top, rc2.right, rc2.bottom));
						m_pRichEdit->MoveWindow(rc2);

						if (rc2.Height() > 0)
						{
							m_nRceHeight = rc2.Height();
						}
					}
					else	// Hide
					{
						CRect rcTemp;
						GetClientRect(&rcTemp);
						m_ctlSplitter.SetRowHeightForStatic(0, rcTemp.Height());
						m_ctlSplitter.SetRowHeightForStatic(1, 0);
						m_pWebBrowser->MoveWindow(rcTemp);
						m_pRichEdit->MoveWindow(0, 0, 0, rcTemp.Height());
					}
				}
				break;
			case CALLER::CALL_SHOW_HIDE :
				if (m_nPosConsole == MODE_CHECKED)		// Horizontal
				{
					CRect rc = m_ctlSplitter.GetWindowRect(0, 0);
					m_pWebBrowser->MoveWindow(rc);

					CRect rcTemp = m_ctlSplitter.GetWindowRect(0, 1);
					CRect rc2 = CRect(0, 0, rcTemp.Width(), rcTemp.Height());
					m_pRichEdit->MoveWindow(rc2);
				}
				else									// Vertical
				{
					CRect rc = m_ctlSplitter.GetWindowRect(0, 0);
					m_pWebBrowser->MoveWindow(rc);

					CRect rcTemp = m_ctlSplitter.GetWindowRect(1, 0);
					CRect rc2 = CRect(0, 0, rcTemp.Width(), rcTemp.Height());
					m_pRichEdit->MoveWindow(rc2);
				}
				break;
		}

		m_ctlSplitter.Update();
		m_ctlSplitter.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		
		RegisterRichEditPos(m_nPosConsole);
	}
}

LRESULT CCuiBoxDlg::OnChangeSplitter(WPARAM wParam, LPARAM lParam)
{
	DEBUG_VIEW(g_bDebugMode, PrintOut(_T(">> Change splitter type : %d (0 : Horz, 1 : Vert, else : None)\r\n"), (int)lParam));
	SetWndPos();
	
	return 0;
}

LRESULT CCuiBoxDlg::OnNavigate(WPARAM wParam, LPARAM lParam)
{
	TCHAR* pszUrl = (TCHAR*)wParam;
	PrintOut(_T(">> URL : %s\r\n"), pszUrl);
	m_pWebBrowser->Navigate(pszUrl, nullptr);
	//m_pWebBrowser->Navigate(_T("http://localhost:8188"), nullptr);
	return 0;
}

void CCuiBoxDlg::OutputData()
{
	g_bProcessing = true;

	while (!g_qData.empty())
	{
		CONSOLE_DATA* pData = g_qData.front();

		if (pData == nullptr) break;
#if 0
		char hex[2048];
		memset(hex, 0x00, sizeof(hex));
		string_to_hex(pData->szData, hex, sizeof(hex));
		PrintOutA(">> [%ld] %s\r\n", pData->dwSize, hex);
#endif
		PrintOutA(">> [%ld] %s (%ld)\r\n", pData->nType, pData->szData, pData->dwSize);
		switch (pData->nType)
		{
			case TR_NORMAL :
			case TR_EMPHASIS:
				ParserText(pData->szData, pData->dwSize);
				break;
			case TR_ERROR :
				RaiseError(pData->szData, pData->dwSize);
				break;
		}

		delete[] pData;
		pData = NULL;

		g_qData.pop();
		//PrintOut(_T(">> Pop : %ld\r\n"), g_qData.size());
	}

	g_bProcessing = false;
}

char* findAndCut(const char* input, const char* word)
{
	const char* found = strstr(input, word);	// 단어가 문자열 내에 있는지 찾기
	if (!found)
	{
		return nullptr;							// 단어를 찾지 못하면 nullptr 반환
	}
	
	size_t length = strlen(found) + 1;			// +1은 null terminator를 포함
	char* result = new char[length];			// 새 문자열을 위한 메모리 할당
	strcpy(result, found);						// found부터 끝까지의 문자열 복사

	return result;
}

void CCuiBoxDlg::ParserText(char* pszData, DWORD dwSize)
{
	if (dwSize > SIZE_CALL_COMFYUI)
	{
		char* pData = findAndCut(pszData, TU_CALL_COMFYUI);
		if (pData)
		{
			PrintOut(_T(">> Execute navigator for comfyui."));
			int nSize = (int)strlen(pData) - SIZE_CALL_COMFYUI + 1;
			char* pBuff = new char[nSize];
			memset(pBuff, 0x00, nSize);
			snprintf(pBuff, nSize, "%s", pData + SIZE_CALL_COMFYUI);
			if (m_pWebBrowser != nullptr)
			{
				TCHAR szUrl[256];
				memset(szUrl, 0x00, sizeof(szUrl));
				WCHAR* pWUrl = new WCHAR[nSize];
				memset(pWUrl, 0x00, sizeof(WCHAR) * (nSize));
				if (CharToWideChar(pBuff, pWUrl, nSize) != 0)
				{
					_stprintf_s(szUrl, _T("%s"), pWUrl);
					SendMessage(UM_EXEC_NAVIGATE, (WPARAM)szUrl, NULL);
				}

				delete[] pWUrl;
				pWUrl = NULL;
			}

			delete[] pBuff;
			delete pData;
			return;
		}
	}

	MakeAnsiText(pszData, dwSize);
}

void CCuiBoxDlg::RaiseError(char* pszData, DWORD dwSize)
{
	MakeAnsiText(pszData, dwSize, TR_ERROR);
}

UINT ReadFromPipe(LPVOID lpVoid)
{
	_PIPE_INFO* ppi = (_PIPE_INFO*)lpVoid;
	CCuiBoxDlg* pDlg = (CCuiBoxDlg *)ppi->pWnd;

	char buffer[SIZE_CONSOLE_DATA];
	DWORD bytesRead;
	while (true)
	{
		if (PeekNamedPipe(ppi->hReadPipe, NULL, 0, NULL, &bytesRead, NULL) && bytesRead > 0)
		{
			memset(buffer, 0x00, sizeof(buffer));
			if (ReadFile(ppi->hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
			{
				buffer[bytesRead] = '\0';

				CONSOLE_DATA* pData = new CONSOLE_DATA;
				pData->nType = ppi->bError ? TR_ERROR : TR_NORMAL;
				pData->dwSize = bytesRead;
				memset(pData->szData, 0x00, sizeof(pData->szData));
				sprintf(pData->szData, "%s", buffer);
				g_qData.push(pData);

				if (!g_bProcessing && !g_qData.empty())
				{
					pDlg->OutputData();
				}
			}
		}
		Sleep(100);
	}

	return 0;
}

UINT ProcConsole(LPVOID lpVoid)
{
	CCuiBoxDlg* pDlg = (CCuiBoxDlg*)lpVoid;

	BOOL bResult = FALSE;

	const int nMaxReadBuff = SIZE_CONSOLE_DATA - 1;
	char szBuff[SIZE_CONSOLE_DATA];
	DWORD dwRead = 0, dwOut = 0, dwErr = 0;

	STARTUPINFO si;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	CreatePipe(&g_hStdOutRead, &g_hStdOutWrite, &sa, 0);  // 콘솔에 출력되는 정보와 연결할 파이프 생성

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdInput = NULL;
	si.hStdOutput = g_hStdOutWrite;
	si.hStdError = g_hStdOutWrite;
	si.wShowWindow = SW_HIDE;
	if (!CreateProcess(COMFYUI, (LPWSTR)(g_strRunMode.compare("gpu") == 0 ? PARAMS[OPT_RUN::NVIDIA] : PARAMS[OPT_RUN::CPU]), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &g_pi))
	{
		memset(szBuff, 0x00, sizeof(szBuff));
		sprintf_s(szBuff, "LastError : %ld", GetLastError());
		pDlg->RaiseError(szBuff, (DWORD)strlen(szBuff));
	}

	pDlg->SetPid(g_pi.dwProcessId);
	PrintOut(_T(">> Last pid : %ld\r\n"), g_pi.dwProcessId);
	
	do
	{
		memset(szBuff, 0x00, sizeof(szBuff));
		bResult = ::ReadFile(g_hStdOutRead, szBuff, nMaxReadBuff, &dwRead, 0);
		if (bResult)
		{
			szBuff[dwRead] = 0x00;
			if (strlen(szBuff) > 0)
			{
				CONSOLE_DATA* pData = new CONSOLE_DATA;
				pData->nType = TR_NORMAL;
				pData->dwSize = dwRead;
				memset(pData->szData, 0x00, sizeof(pData->szData));
				sprintf(pData->szData, "%s", szBuff);
				g_qData.push(pData);
			}
		}

		if (!g_bProcessing && !g_qData.empty())
		{
			pDlg->OutputData();
		}
	} while (bResult);

	pDlg->CloseConsole();

	return 0;
}

/*
UINT ProcConsole(LPVOID lpVoid)
{
	CCuiBoxDlg* pDlg = (CCuiBoxDlg*)lpVoid;
	
	BOOL bResult = FALSE;

	const int nMaxReadBuff = SIZE_CONSOLE_DATA - 1;
	char szBuff[SIZE_CONSOLE_DATA];
	DWORD dwRead = 0, dwOut = 0, dwErr = 0;

	STARTUPINFO si;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	CreatePipe(&g_hStdOutRead, &g_hStdOutWrite, &sa, 0);  // 콘솔에 출력되는 정보와 연결할 파이프 생성
#ifdef PEEK_NAMED_PIPE
	CreatePipe(&g_hStdErrRead, &g_hStdErrWrite, &sa, 0);  // 콘솔에 출력되는 오류와 연결할 파이프 생성
#endif

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdInput = NULL;
	si.hStdOutput = g_hStdOutWrite;
#ifdef PEEK_NAMED_PIPE
	si.hStdError = g_hStdErrWrite;
#else
	si.hStdError = g_hStdOutWrite;
#endif
	si.wShowWindow = SW_HIDE;
	if (!CreateProcess(COMFYUI, (LPWSTR)(g_strRunMode.compare("gpu") == 0 ? PARAMS[OPT_RUN::NVIDIA] : PARAMS[OPT_RUN::CPU]), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &g_pi))
	{
		memset(szBuff, 0x00, sizeof(szBuff));
		sprintf_s(szBuff, "LastError : %ld", GetLastError());
		pDlg->RaiseError(szBuff, (DWORD)strlen(szBuff));
	}

#ifdef PEEK_NAMED_PIPE
	while (PeekNamedPipe(g_hStdOutRead, NULL, 0, NULL, &dwOut, NULL) ||	PeekNamedPipe(g_hStdErrRead, NULL, 0, NULL, &dwErr, NULL))  // 읽을 데이터가 있는지 체크
	{
		if (dwOut <= 0 && dwErr <= 0 && WaitForSingleObject(g_pi.hProcess, 0) != WAIT_TIMEOUT)
			break;  // 콘솔 프로그램이 종료된 경우 loop를 빠져나간다.

		while (PeekNamedPipe(g_hStdOutRead, NULL, 0, NULL, &dwOut, NULL) && dwOut > 0)
		{
			memset(szBuff, 0x00, sizeof(szBuff));
			bResult = ReadFile(g_hStdOutRead, szBuff, min(nMaxReadBuff, dwOut), &dwRead, NULL);
			if (bResult)
			{
				if (dwRead == 0) break;
				szBuff[dwRead] = 0x00;
				if (strlen(szBuff) > 0)
				{
					PrintOutA("★★★[N] %s\r\n", szBuff);
					CONSOLE_DATA* pData = new CONSOLE_DATA;
					pData->nType = TR_NORMAL;
					pData->dwSize = dwRead;
					memset(pData->szData, 0x00, sizeof(pData->szData));
					sprintf(pData->szData, "%s", szBuff);
					g_qData.push(pData);
				}
			}
		}

		while (PeekNamedPipe(g_hStdErrRead, NULL, 0, NULL, &dwErr, NULL) && dwErr > 0)
		{
			memset(szBuff, 0x00, sizeof(szBuff));
			bResult = ReadFile(g_hStdErrRead, szBuff, min(nMaxReadBuff, dwErr), &dwRead, NULL);
			if (bResult)
			{
				szBuff[dwRead] = 0x00;
				if (strlen(szBuff) > 0)
				{
					if (dwRead == 0) break;

					PrintOutA("★★★[E] %s\r\n", szBuff);
					CONSOLE_DATA* pData = new CONSOLE_DATA;
					pData->nType = TR_ERROR;
					pData->dwSize = dwRead;
					memset(pData->szData, 0x00, sizeof(pData->szData));
					sprintf(pData->szData, "%s", szBuff);
					g_qData.push(pData);
				}
			}
		}

		if (!g_bProcessing && !g_qData.empty())
		{
			pDlg->OutputData();
		}
	}
#else
	do
	{
		memset(szBuff, 0x00, sizeof(szBuff));
		bResult = ::ReadFile(g_hStdOutRead, szBuff, nMaxReadBuff, &dwRead, 0);
		if (bResult)
		{
			szBuff[dwRead] = 0x00;
			if (strlen(szBuff) > 0)
			{
				CONSOLE_DATA* pData = new CONSOLE_DATA;
				pData->nType = TR_NORMAL;
				pData->dwSize = dwRead;
				memset(pData->szData, 0x00, sizeof(pData->szData));
				sprintf(pData->szData, "%s", szBuff);
				g_qData.push(pData);
			}
		}

		if (!g_bProcessing && !g_qData.empty())
		{
			pDlg->OutputData();
		}
	} while (bResult);
#endif

	pDlg->CloseConsole();

	return 0;
}
*/

bool CCuiBoxDlg::RunConsole()
{
	TCHAR szPath[512];
	memset(szPath, 0x00, sizeof(szPath));
	_stprintf_s(szPath, _T("%s%s"), (LPCTSTR)CUtil::GetAppRootPath(), CUIBOX_MAINW);
	if (!PathFileExists(szPath))
	{
		MakeMainFile();
	}

#ifndef PEEK_NAMED_PIPE
	AfxBeginThread(ProcConsole, this);
#else
	const int nMaxReadBuff = SIZE_CONSOLE_DATA - 1;
	char szBuff[SIZE_CONSOLE_DATA];
	DWORD dwRead = 0, dwOut = 0, dwErr = 0;

	STARTUPINFO si;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	CreatePipe(&g_hStdOutRead, &g_hStdOutWrite, &sa, 0);  // 콘솔에 출력되는 정보와 연결할 파이프 생성
	CreatePipe(&g_hStdErrRead, &g_hStdErrWrite, &sa, 0);  // 콘솔에 출력되는 오류와 연결할 파이프 생성

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdInput = NULL;
	si.hStdOutput = g_hStdOutWrite;
	si.hStdError = g_hStdErrWrite;
	si.wShowWindow = SW_HIDE;
	if (!CreateProcess(COMFYUI, (LPWSTR)(g_strRunMode.compare("gpu") == 0 ? PARAMS[OPT_RUN::NVIDIA] : PARAMS[OPT_RUN::CPU]), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &g_pi))
	{
		memset(szBuff, 0x00, sizeof(szBuff));
		sprintf_s(szBuff, "LastError : %ld", GetLastError());
		RaiseError(szBuff, (DWORD)strlen(szBuff));
		return false;
	}

	SetPid(g_pi.dwProcessId);
	PrintOut(_T(">> Last pid : %ld\r\n"), g_pi.dwProcessId);

	SAFE_CLOSE_HANDLE(g_hStdOutWrite);
	SAFE_CLOSE_HANDLE(g_hStdErrWrite);
//	CloseHandle(g_hStdOutWrite);
//	CloseHandle(g_hStdErrWrite);
	
	g_ppiOutput = new _PIPE_INFO;
	g_ppiOutput->pWnd = this;
	g_ppiOutput->hReadPipe = g_hStdOutRead;
	g_ppiOutput->bError = false;
	AfxBeginThread(ReadFromPipe, g_ppiOutput);

	g_ppiError = new _PIPE_INFO;
	g_ppiError->pWnd = this;
	g_ppiError->hReadPipe = g_hStdErrRead;
	g_ppiError->bError = true;
	AfxBeginThread(ReadFromPipe, g_ppiError);
#endif
	return true;
}

bool CCuiBoxDlg::CloseConsole()
{
	TerminateProcess(g_pi.hProcess, 0);

	SAFE_CLOSE_HANDLE(g_pi.hThread);
	SAFE_CLOSE_HANDLE(g_pi.hProcess);

	SAFE_CLOSE_HANDLE(g_hStdOutRead);
	SAFE_CLOSE_HANDLE(g_hStdOutWrite);
#ifdef PEEK_NAMED_PIPE
	SAFE_CLOSE_HANDLE(g_hStdErrRead);
	SAFE_CLOSE_HANDLE(g_hStdErrWrite);
#endif

	return true;
}

list<TEXT_INFO_DATA*> CCuiBoxDlg::MakeCuiText(LPCTSTR lpText, DWORD dwSize, UINT nType)
{
	list<TEXT_INFO_DATA*> plist;
	plist.clear();

	if (lpText == NULL) return plist;

	TEXT_INFO_DATA* pData = new TEXT_INFO_DATA();
	pData->dwTextStyle = 0;
	pData->dwForeColor = 0;
	pData->dwBackColor = 0;

	for (DWORD i = 0; i < dwSize; ++i)
	{
		if(lpText[i] == 0x1B)
		{
			++i;
			if (lpText[i] == '[' && lpText[i + 1] == '0' && lpText[i + 2] == 'm')
			{
				plist.push_back(pData);
				i += 3;
				if (i < dwSize)
				{
					pData = new TEXT_INFO_DATA();
					pData->dwTextStyle = 0;
					pData->dwForeColor = 0;
					pData->dwBackColor = 0;
				}
			}
			else if (lpText[i] == '[')
			{
				if (i > 1)
				{
					plist.push_back(pData);

					pData = new TEXT_INFO_DATA();
					pData->dwTextStyle = 0;
					pData->dwForeColor = 0;
					pData->dwBackColor = 0;
				}

				DWORD j = ++i;
				for (; j < dwSize && (lpText[j] == ';' || (lpText[j] >= '0' && lpText[j] <= '9')); ++j);
				if (lpText[j] == 'm')
				{
					wstring temp(_T(""));
					for (; i < j; ++i)
					{
						if (lpText[i] == ';')
						{
							int val = _wtoi(temp.c_str());
							// PrintOut(_T(">> Value : %ld\r\n"), val);
							switch (val)
							{
								CASE_TEXT_STYLE:
									pData->dwTextStyle = val;
									break;
								CASE_FOREGROUND_COLOR:
									pData->dwForeColor = val;
									break;
								CASE_BACKROUND_COLOR:
									pData->dwBackColor = val;
									break;
							}
							temp = _T("");
						}
						else if (i == (j - 1))
						{
							temp.push_back(lpText[i]);
							int val = _wtoi(temp.c_str());
							// PrintOut(_T(">> Value : %ld\r\n"), val);
							switch (val)
							{
								CASE_TEXT_STYLE:
									pData->dwTextStyle = val;
									break;
								CASE_FOREGROUND_COLOR:
									pData->dwForeColor = val;
									break;
								CASE_BACKROUND_COLOR:
									pData->dwBackColor = val;
									break;
							}
							temp = _T("");
						}
						else
						{
							temp.push_back(lpText[i]);
						}
					}
				}
				else
				{
					pData->data.push_back(lpText[i]);
				}
			}
		}
		else
		{
			pData->data.push_back(lpText[i]);
		}

		if (i == (dwSize - 1))
		{
			plist.push_back(pData);
		}
	}

	return plist;
}

void CCuiBoxDlg::MakeAnsiText(char* pszText, DWORD dwSize, UINT nType)
{
	BOOL isAnsi = CUtil::IsAnsiCommand(pszText);

	TCHAR szData[SIZE_CONSOLE_DATA];
	memset(szData, 0x00, sizeof(szData));
	WCHAR* pWData = new WCHAR[dwSize + 1];
	memset(pWData, 0x00, sizeof(WCHAR) * (dwSize + 1));
	if (CharToWideChar(pszText, pWData, dwSize + 1) != 0)
	{
		_stprintf_s(szData, _T("%s"), pWData);

		if (isAnsi)
		{
			//PrintOut(_T(">> Ansi\r\n>> %s\r\n"), szData);
			list<TEXT_INFO_DATA*> plist = MakeCuiText(szData, dwSize, nType);

			list<TEXT_INFO_DATA*>::iterator its = plist.begin();
			list<TEXT_INFO_DATA*>::iterator ite = plist.end();

			for (its; its != ite; ++its)
			{
				TEXT_INFO_DATA* pTid = (*its);
				//PrintOut(_T(">> String : %s\r\n"), pTid->data.c_str());
				COLORREF crForeColor = RCE_FG_COLOR;
				COLORREF crBackColor = RCE_BG_COLOR;
				if (pTid->dwForeColor > 0)
				{
					crForeColor = CUtil::GetAnsiColor(pTid->dwForeColor);
				}
				if (pTid->dwBackColor > 0)
				{
					crBackColor = CUtil::GetAnsiColor(pTid->dwBackColor);
				}
				CuiTextOut(pTid->data.c_str(), crForeColor, crBackColor);
				delete (pTid);
				its = plist.erase(its);
			}
			//PrintOut(_T(">> =====================================================\r\n"));
		}
		else
		{
			// Ansi Escape Code가 없는 경우는 
			switch (nType)
			{
				case TR_NORMAL:
					CuiTextOut(szData);
					break;
				case TR_EMPHASIS:
					CuiTextOut(szData, CUtil::GetAnsiColor(FGC_YELLOW));
					break;
				case TR_ERROR:
					CuiTextOut(szData, CUtil::GetAnsiColor(FGC_LIGHTRED));
					break;
			}
		}
	}
	delete[] pWData;
	pWData = NULL;
}

void CCuiBoxDlg::CuiTextOut(LPCTSTR lpText)
{
	CString str(lpText);
	str.Replace(_T("뻽"), _T("■"));
	str.Replace(_T("뼂"), _T(" "));
	m_nLastLine = m_pRichEdit->GetLineCount();
	//m_pRichEdit->LineScroll(m_nLastLine);
	m_pRichEdit->SetSel(-1, -1);
	m_pRichEdit->ReplaceSel(str);
	
	AutoScroll();
}

void CCuiBoxDlg::CuiTextOut(LPCTSTR lpText, COLORREF crText, COLORREF crBg)
{
	CString str(lpText);
	str.Replace(_T("뻽"), _T("■"));
	str.Replace(_T("뼂"), _T(" "));

	CHARFORMAT2 cf;
	memset(&cf, 0x00, sizeof(CHARFORMAT2));

	cf.cbSize		= sizeof(CHARFORMAT2);
	cf.dwMask		= CFM_COLOR;
	cf.crTextColor	= crText;
	cf.crBackColor	= crBg;

	m_nLastLine = m_pRichEdit->GetLineCount();
	//m_pRichEdit->LineScroll(m_nLastLine);
	m_pRichEdit->SetSel(-1, -1);
	m_pRichEdit->SetSelectionCharFormat(cf);
	m_pRichEdit->ReplaceSel(str);
	
	AutoScroll();
}

void  CCuiBoxDlg::AutoScroll()
{
	UINT nNewLine = m_pRichEdit->GetLineCount();
	UINT nScroll = nNewLine - m_nLastLine;
	m_pRichEdit->SetCaretPos(CPoint(0, m_nLastLine - 1));
	m_pRichEdit->SetFocus();
	m_pRichEdit->LineScroll(nScroll);
	m_nLastLine = nNewLine;			// Save last line

	//CRect rect;
	//m_pRichEdit->GetRect(rect);
	//::InvalidateRect(this->GetSafeHwnd(), &rect, FALSE);
}

CRect CCuiBoxDlg::GetInitWindowPos()
{
	CString s;
	CRect r(0, 0, 0, 0);
	CRegistry reg;

	s.Format(_T("%s\\%s"), REGPATH_CUIBOX, REGPATH_POS);
	if (reg.Open(s, HKEY_CURRENT_USER))
	{
		r.left = reg.ReadInt(REGKEY_POS_MAIN_L);
		r.top = reg.ReadInt(REGKEY_POS_MAIN_T);
		r.right = reg.ReadInt(REGKEY_POS_MAIN_R);
		r.bottom = reg.ReadInt(REGKEY_POS_MAIN_B);
		PrintOut(_T(">> [REG] Left : %ld, Top : %ld, Right : %ld, Bottom : %ld\n"), r.left, r.top, r.right, r.bottom);

		if (reg.IsValid(REGKEY_POS_RCE_W))
		{
			m_nRceWidth = reg.ReadInt(REGKEY_POS_RCE_W);
		}
		else
		{
			m_nRceWidth = SIZE_RCE_CONSOLEW;
		}
		if (reg.IsValid(REGKEY_POS_RCE_H))
		{
			m_nRceHeight = reg.ReadInt(REGKEY_POS_RCE_H);
		}
		else
		{
			m_nRceHeight = SIZE_RCE_CONSOLEH;
		}
		if (reg.IsValid(REGKEY_POS_DIRECT))
		{
			switch (reg.ReadInt(REGKEY_POS_DIRECT))
			{
				case (int)TYPE_DIRECTION::TD_VERT:
					m_nPosConsole = MODE_UNCHECKED;
					m_btnPosConsole.SetCheck(BST_UNCHECKED);
					break;
				case (int)TYPE_DIRECTION::TD_HORZ:
				default:
					m_nPosConsole = MODE_CHECKED;
					m_btnPosConsole.SetCheck(BST_CHECKED);
					break;
			}
		}
		if (reg.IsValid(REGKEY_POS_SHOWWINDOW))
		{
			switch (reg.ReadInt(REGKEY_POS_SHOWWINDOW))
			{
				case 1:
					m_nShowConsole = MODE_CHECKED;
					m_btnShowConsole.SetCheck(BST_CHECKED);
					break;
				case 0:
				default:
					m_nShowConsole = MODE_UNCHECKED;
					m_btnShowConsole.SetCheck(BST_UNCHECKED);
					break;
			}
		}
		
		reg.Close();

		CRect r2;
		if (r.top < 0 || !SystemParametersInfo(SPI_GETWORKAREA, 0, r2, 0) || r.top > r2.bottom)
			r.SetRectEmpty();
	}

	if (r.IsRectNull() || r.IsRectEmpty())
	{
		if (SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0))
		{
			int i = 0;
			i = (int)((r.Width() - MAINWINDOW_WIDTH) / 2);
			r.left = (i > 0) ? i : r.right - MAINWINDOW_WIDTH - 50;
			i = (int)((r.Height() - MAINWINDOW_HEIGHT) / 2);
			r.top = (i > 0) ? i : 0;
			r.right = r.left + MAINWINDOW_WIDTH;
			r.bottom = r.top + MAINWINDOW_HEIGHT;
			m_nRceWidth = SIZE_RCE_CONSOLEW;
			m_nRceHeight = SIZE_RCE_CONSOLEH;
		}
	}

	return r;
}

void CCuiBoxDlg::RegisterWindowPos()
{
	if (this->IsZoomed()) return;

	CString s;
	CRect r(0, 0, 0, 0);
	CRegistry reg;

	GetWindowRect(r);
	s.Format(_T("%s\\%s"), REGPATH_CUIBOX, REGPATH_POS);
	if (reg.Open(s, HKEY_CURRENT_USER))
	{
		// Left 위치값 보정.
		if (r.left < 0)
		{
			int nGap = r.left * (-1);
			r.left = 0;
			r.right += nGap;
		}
		// Top 위치값 보정.
		if (r.top < 0)
		{
			int nGap = r.top * (-1);
			r.top = 0;
			r.bottom += nGap;
		}

		reg.Write(REGKEY_POS_MAIN_L, (int)r.left);
		reg.Write(REGKEY_POS_MAIN_T, (int)r.top);
		reg.Write(REGKEY_POS_MAIN_R, (int)r.right);
		reg.Write(REGKEY_POS_MAIN_B, (int)r.bottom);
		reg.Close();
	}
}

void CCuiBoxDlg::RegisterRichEditPos(int nType)
{
	CString s;
	CRegistry reg;
	s.Format(_T("%s\\%s"), REGPATH_CUIBOX, REGPATH_POS);
	if (reg.Open(s, HKEY_CURRENT_USER))
	{
		switch (nType)
		{
			case 0 :		// vertical
				reg.Write(REGKEY_POS_RCE_H, (int)m_nRceHeight);
				break;
			case 1 :		// horizontal
				reg.Write(REGKEY_POS_RCE_W, (int)m_nRceWidth);
				break;
			case 2 :
				reg.Write(REGKEY_POS_RCE_H, (int)m_nRceHeight);
				reg.Write(REGKEY_POS_RCE_W, (int)m_nRceWidth);
				break;
		}
		reg.Close();
	}
}

void CCuiBoxDlg::RegisterDirection(TYPE_DIRECTION td)
{
	CString s;
	CRegistry reg;
	s.Format(_T("%s\\%s"), REGPATH_CUIBOX, REGPATH_POS);
	if (reg.Open(s, HKEY_CURRENT_USER))
	{
		reg.Write(REGKEY_POS_DIRECT, (int)td);
		reg.Close();
	}
}

void CCuiBoxDlg::RegisterShowWindow(bool bShow)
{
	CString s;
	CRegistry reg;
	s.Format(_T("%s\\%s"), REGPATH_CUIBOX, REGPATH_POS);
	if (reg.Open(s, HKEY_CURRENT_USER))
	{
		reg.Write(REGKEY_POS_SHOWWINDOW, bShow ? 1 : 0);
		reg.Close();
	}
}

void CCuiBoxDlg::SetPid(DWORD dwPid)
{
	CRegistry reg;
	if (reg.Open(REGPATH_CUIBOX, HKEY_CURRENT_USER))
	{
		reg.Write(REGKEY_PID, (int)dwPid);
		reg.Close();
	}
}

void CCuiBoxDlg::OnBnClickedBtnMin()
{
	PostMessage(WM_SYSCOMMAND, SC_MINIMIZE);
}

void CCuiBoxDlg::OnBnClickedBtnMax()
{
	DWORD dwStyle = this->GetStyle();
	if (dwStyle & WS_MAXIMIZE)
	{
		PostMessage(WM_SYSCOMMAND, SC_RESTORE);
	}
	else
	{
		PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE);
	}
}

void CCuiBoxDlg::OnBnClickedBtnClose()
{
	ShowWindow(SW_HIDE);
	// CDialogEx::OnClose();
}

void CCuiBoxDlg::OnBnClickedBtnShowConsole()
{
	if (m_ctlSplitter.GetSafeHwnd())
	{
		CRect rc, rcTemp;
		GetClientRect(&rc);
		rcTemp.CopyRect(rc);
		rcTemp.top += (TTB_HEIGHT + 1);		// bottom line = 1

		int nBorderSize = SIZE_SPLITTER_GAP * 2;
		switch (m_nShowConsole)
		{
			case MODE_CHECKED :				// Show -> Hide
				switch (m_nPosConsole)
				{
					case MODE_CHECKED :		// [HORZ] 우측
						m_ctlSplitter.SetColumnWidthForStatic(0, rcTemp.Width());
						m_ctlSplitter.SetColumnWidthForStatic(1, 0);
						break;
					case MODE_UNCHECKED :	// [VERT] 하단
						m_ctlSplitter.SetRowHeightForStatic(0, rcTemp.Height());
						m_ctlSplitter.SetRowHeightForStatic(1, 0);
						break;
				}
				break;
			case MODE_UNCHECKED :			// Hide -> Show
				switch (m_nPosConsole)
				{
					case MODE_CHECKED:		// [HORZ] 우측
						m_ctlSplitter.SetColumnWidthForStatic(0, rcTemp.Width() - m_nRceWidth - nBorderSize);
						m_ctlSplitter.SetColumnWidthForStatic(1, m_nRceWidth);
						break;
					case MODE_UNCHECKED:	// [VERT] 하단
						m_ctlSplitter.SetRowHeightForStatic(0, rcTemp.Height() - m_nRceHeight - nBorderSize);
						m_ctlSplitter.SetRowHeightForStatic(1, m_nRceHeight);
						break;
				}
				break;
		}

		m_nShowConsole = (m_nShowConsole == MODE_CHECKED) ? MODE_UNCHECKED : MODE_CHECKED;

		m_ctlSplitter.Update();
		m_SplitterStyle.Install(&m_ctlSplitter);

		RedrawControls(rc, CALLER::CALL_SHOW_HIDE);
		RegisterShowWindow(m_nShowConsole == MODE_CHECKED);
		m_btnShowConsole.SetCheck(m_nShowConsole);
	}

}

void CCuiBoxDlg::OnBnClickedBtnPosConsole()
{
	if (m_ctlSplitter.GetSafeHwnd())
	{
		CRect rc, rcTemp;
		GetClientRect(&rc);
		rcTemp.CopyRect(rc);
		rcTemp.top += (TTB_HEIGHT + 1);		// bottom line = 1
		//PrintOut(_T(">> Get rce width : %ld, Get rce height : %ld\r\n"), m_nRceWidth, m_nRceHeight);

		int nBorderSize = SIZE_SPLITTER_GAP * 2;
		PrintOut(_T(">> Coloum : %ld, Row : %ld\r\n"), m_ctlSplitter.GetNumberColumn(), m_ctlSplitter.GetNumberRow());
		switch (m_nPosConsole)
		{
			case MODE_CHECKED:				// horz -> vert
				m_ctlSplitter.DeleteColumn(1);
				m_ctlSplitter.AddRow();
				if (m_nShowConsole == MODE_CHECKED)
				{
//					PrintOut(_T(">> [VERT] Show console.\r\n"));
					m_ctlSplitter.SetRowHeightForStatic(0, rcTemp.Height() - m_nRceHeight - nBorderSize);
					m_ctlSplitter.SetRowHeightForStatic(1, m_nRceHeight);
				}
				else
				{
//					PrintOut(_T(">> [VERT] Hide console.\r\n"));
					m_ctlSplitter.SetRowHeightForStatic(0, rcTemp.Height());
					m_ctlSplitter.SetRowHeightForStatic(1, 0);
				}
				m_ctlSplitter.SetWindow(1, 0, m_sttConsole.GetSafeHwnd());

				for (int r = 0; r < m_ctlSplitter.GetNumberRow() - 1; ++r)
				{
					m_ctlSplitter.ActivateHorzSplitter(r, true);
				}
				for (int c = 0; c < m_ctlSplitter.GetNumberColumn() - 1; ++c)
				{
					m_ctlSplitter.ActivateVertSplitter(c, false);
				}
				RegisterDirection(TYPE_DIRECTION::TD_VERT);
				break;
			case MODE_UNCHECKED:				// vert -> horz
				m_ctlSplitter.DeleteRow(1);
				m_ctlSplitter.AddColumn();
				if (m_nShowConsole == MODE_CHECKED)
				{
//					PrintOut(_T(">> [HORZ] Show console.\r\n"));
					m_ctlSplitter.SetColumnWidthForStatic(0, rcTemp.Width() - m_nRceWidth - nBorderSize);
					m_ctlSplitter.SetColumnWidthForStatic(1, m_nRceWidth);
				}
				else
				{
//					PrintOut(_T(">> [HORZ] Hide console.\r\n"));
					m_ctlSplitter.SetColumnWidthForStatic(0, rcTemp.Width());
					m_ctlSplitter.SetColumnWidthForStatic(1, 0);
				}
				m_ctlSplitter.SetWindow(0, 1, m_sttConsole.GetSafeHwnd());

				for (int r = 0; r < m_ctlSplitter.GetNumberRow() - 1; ++r)
				{
					m_ctlSplitter.ActivateHorzSplitter(r, false);
				}
				for (int c = 0; c < m_ctlSplitter.GetNumberColumn() - 1; ++c)
				{
					m_ctlSplitter.ActivateVertSplitter(c, true);
				}
				RegisterDirection(TYPE_DIRECTION::TD_HORZ);
				break;
		}

		// MODE_CHECKED : Horizontal(우측)
		// MODE_UNCHECKED : Vertical(하단)
		m_nPosConsole = (m_nPosConsole == MODE_CHECKED) ? MODE_UNCHECKED : MODE_CHECKED;

		//PrintOut(_T(">> Get rows : %ld, Get cols : %ld\r\n"), m_ctlSplitter.GetNumberRow(), m_ctlSplitter.GetNumberColumn());
		m_ctlSplitter.SetResizeMode(CSplitterCtrl::ResizeStatic);
		m_ctlSplitter.Update();

		m_SplitterStyle.Install(&m_ctlSplitter);
		RedrawControls(rc, CALLER::CALL_SPLITTER);
		
		m_btnPosConsole.SetCheck(m_nPosConsole);
	}
}

void CCuiBoxDlg::GetConfig()
{
	// 윈도우 시작시 자동 실행 유무 확인
	CRegistry reg;
	if (reg.Open(REGPATH_AUTORUN))
	{
		if (!reg.ReadStr(APP_NAME).IsEmpty())
		{
			m_bStartup = TRUE;
		}
		reg.Close();
	}

	TCHAR path[MAX_PATH2];
	memset(path, 0x00, sizeof(path));
	GetModuleFileName(NULL, path, MAX_PATH2);
	CString strRootPath = CUtil::GetAppRootPath();
	std::string sRootPath = std::string(CT2CA(strRootPath));
	CString strPath(path);
	int i = strPath.ReverseFind('\\');			// 실행 파일 이름을 지우기 위해서 왼쪽에 있는 '/'를 찾는다.
	strPath = strPath.Left(i + 1);				// 뒤에 있는 현재 실행 파일 이름을 지운다.
	strPath += FLJ_CUIBOX;

	CFile file;
	if (!file.Open(strPath, CFile::modeRead))
	{
		PrintOut(_T(">> Failed to open config file.\r\n"));
		return;
	}

	UINT nFileLength = (UINT)file.GetLength();
	char* data = new char[nFileLength + 1];
	memset(data, 0x00, sizeof(char) * (nFileLength + 1));
	file.Read(data, nFileLength);
	file.Close();

	Json::Value root;
	Json::Reader jsonDoc;
	std::string stdStr(data);
	if (jsonDoc.parse(stdStr, root))
	{
		if (root.isMember("dinfo"))
		{
			g_bDebugMode = true;
		}
		g_strLang = root.get("lang", "ko").asString();
		g_strRunMode = root.get("run", "cpu").asString();
		g_bUpdate = root.get("auto_update", true).asBool();
		if (root.isMember("save_path"))
		{
			g_strSavePath = root.get("save_path", "").asString();
			g_bDefineSavePath = true;
		}
		PrintOutA("lang : %s, run : %s\r\n", g_strLang.c_str(), g_strRunMode.c_str());

		if		(!g_strLang.compare("ko"))	CResourceEx::GetInstance()->SetLanguage(KO);
		else if (!g_strLang.compare("en"))	CResourceEx::GetInstance()->SetLanguage(EN);
		else if (!g_strLang.compare("ja"))	CResourceEx::GetInstance()->SetLanguage(JA);
		else if (!g_strLang.compare("zh"))	CResourceEx::GetInstance()->SetLanguage(ZH);
		else if (!g_strLang.compare("vi"))	CResourceEx::GetInstance()->SetLanguage(VI);
		else								CResourceEx::GetInstance()->SetLanguage(KO);
	}

	SAFE_DELETE_ARRAY(data);
}

bool CCuiBoxDlg::MakeMainFile()
{
	const int nLen = _MAX_DIR * 2;
	std::string inputFile;
	std::string outputFile;
	char szPath[nLen];
	memset(szPath, 0x00, sizeof(szPath));
	char szBuf[_MAX_DIR * 2] = { NULL };
	char drive[_MAX_DRIVE] = { NULL };
	char dir[_MAX_DIR] = { NULL };
	char fname[_MAX_FNAME] = { NULL };
	char ext[_MAX_EXT] = { NULL };
	GetModuleFileNameA(NULL, szBuf, nLen);
	_splitpath_s(szBuf, drive, dir, fname, ext);
	
	inputFile.append(drive);
	inputFile.append(dir);
	inputFile.append(COMFYUI_MAINA);
	outputFile.append(drive);
	outputFile.append(dir);
	outputFile.append(CUIBOX_MAINA);

	std::ifstream inf(inputFile);
	std::ofstream outf(outputFile);

	if (!inf || !outf)
	{
		PrintOut(_T(">> Error opening files.\r\n"));
		return false;
	}

	std::string line;
	std::string line1;
	std::string line2;
	std::string line3;
	std::string line4;
	std::string line5;

	while (std::getline(inf, line))
	{
		//	import webbrowser
		if (line.find(find_map[0]) != std::string::npos)
		{
			std::getline(inf, line1);					//	if os.name == 'nt' and address == '0.0.0.0':
			if (line1.find(find_map[1]) != std::string::npos)
			{
				std::getline(inf, line2);				//		address = '127.0.0.1'
				if (line2.find(find_map[2]) != std::string::npos)
				{
					std::getline(inf, line3);			//	"if ':' in address:
					if (line3.find(find_map[3]) != std::string::npos)
					{
						std::getline(inf, line4);		//		address = "[{}]".format(address)
						if (line4.find(find_map[4]) != std::string::npos)
						{
							std::getline(inf, line5);	//	webbrowser.open(f"{scheme}://{address}:{port}")
							if (line5.find(find_map[5]) != std::string::npos)
							{
#ifdef VER_FIRST
								outf << line1 << std::endl;
								size_t pos = line2.find(find_map[2]);
								line2.replace(pos, find_map[2].length(), replace_map[0]);
								outf << line2 << std::endl;
								pos = line3.find(find_map[3]);
								line3.replace(pos, find_map[3].length(), replace_map[1]);
								outf << line3 << std::endl;
#endif
#ifdef VER_0_2_7
								size_t pos = line1.find(find_map[1]);
								line1.replace(pos, find_map[1].length(), replace_map[0]);
								outf << line1 << std::endl;
								pos = line3.find(find_map[3]);
								line3.replace(pos, find_map[3].length(), replace_map[1]);
								outf << line3 << std::endl;
#endif
							}
							else
							{
								outf << line1 << std::endl;
								outf << line2 << std::endl;
								outf << line3 << std::endl;
								outf << line4 << std::endl;
								outf << line5 << std::endl;
							}
						}
						else
						{
							outf << line1 << std::endl;
							outf << line2 << std::endl;
							outf << line3 << std::endl;
							outf << line4 << std::endl;
						}
					}
					else
					{
						outf << line1 << std::endl;
						outf << line2 << std::endl;
						outf << line3 << std::endl;
					}
				}
				else
				{
					outf << line1 << std::endl;
					outf << line2 << std::endl;
				}
			}
			else
			{
				outf << line2 << std::endl;
			}
		}
		else
		{
			outf << line << std::endl;
		}
	}

	return true;
}

HRESULT CCuiBoxDlg::GetVideoCardInfo()
{
	IWbemLocator* pLoc = nullptr;
	IWbemServices* pSvc = nullptr;
	HRESULT hr = 0L;

	hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
	{
		PrintOut(_T(">> Failed to initialize COM library. Error code = 0x%X\r\n"), hr);
		return -1;
	}

	hr = CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities
		NULL                         // Reserved
	);
	if (FAILED(hr))
	{
		PrintOut(_T(">> Failed to initialize security. Error code = 0x%X"), hr);
		CoUninitialize();
		return -1;                    // Program has failed.
	}

	hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
	if (FAILED(hr))
	{
		PrintOut(_T(">> Failed to create WbemLocator: %s\r\n"), _com_error(hr).ErrorMessage());
		CoUninitialize();
		return -1;
	}

	hr = pLoc->ConnectServer(_T("ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, 0, &pSvc);
	if (FAILED(hr))
	{
		PrintOut(_T(">> Failed to connect to WMI: %s\r\n"), _com_error(hr).ErrorMessage());
		pLoc->Release();
		CoUninitialize();
		return -2;
	}

	// Query
	BSTR query = SysAllocString(L"SELECT * FROM Win32_VideoController");
	IEnumWbemClassObject* pEnumerator = nullptr;
	hr = pSvc->ExecQuery(L"WQL", query, WBEM_FLAG_FORWARD_ONLY, nullptr, &pEnumerator);
	if (FAILED(hr))
	{
		PrintOut(_T(">> Query failed: %s\r\n"), _com_error(hr).ErrorMessage());
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return -3;
	}

	vector<GPUInfo> gpuInfo;
	IWbemClassObject* pObject = nullptr;
	ULONG uReturn = 0;
	while (pEnumerator)
	{
		hr = pEnumerator->Next(WBEM_INFINITE, 1, &pObject, &uReturn);
		if (uReturn == 0)
			break;

		GPUInfo info;
		VARIANT varName;
		VARIANT varDesc;
		VARIANT varProcessor;
		VARIANT varVMType;				// Video Memory Type
		VARIANT varRam;
		VARIANT varHres;
		VARIANT varVres;
		VariantInit(&varName);
		VariantInit(&varDesc);
		VariantInit(&varProcessor);
		VariantInit(&varVMType);
		VariantInit(&varRam);
		VariantInit(&varHres);
		VariantInit(&varVres);

		hr = pObject->Get(L"Name", 0, &varName, nullptr, nullptr);
		if (SUCCEEDED(hr) && varName.vt == VT_BSTR)
		{
			info.name = _bstr_t(varName.bstrVal, false);
		}
		hr = pObject->Get(L"Description", 0, &varDesc, nullptr, nullptr);
		if (SUCCEEDED(hr) && varDesc.vt == VT_BSTR)
		{
			info.descript = _bstr_t(varDesc.bstrVal, false);
		}
		hr = pObject->Get(L"VideoProcessor", 0, &varProcessor, nullptr, nullptr);
		if (SUCCEEDED(hr) && varProcessor.vt == VT_BSTR)
		{
			info.vprocessor = _bstr_t(varProcessor.bstrVal, false);
		}
		hr = pObject->Get(L"VideoMemoryType", 0, &varVMType, nullptr, nullptr);
		if (SUCCEEDED(hr) && varVMType.vt == VT_I4)
		{
			info.ramtype = wstring(RAM_INFO[varVMType.llVal].szName);
		}
		hr = pObject->Get(L"AdapterRAM", 0, &varRam, nullptr, nullptr);
		if (SUCCEEDED(hr) && varRam.vt == VT_I4)
		{
			info.ram = (UINT)varRam.llVal;
		}
		hr = pObject->Get(L"CurrentHorizontalResolution", 0, &varHres, nullptr, nullptr);
		if (SUCCEEDED(hr) && varHres.vt == VT_I4)
		{
			info.hres = (UINT)varHres.llVal;
		}
		hr = pObject->Get(L"CurrentVerticalResolution", 0, &varVres, nullptr, nullptr);
		if (SUCCEEDED(hr) && varVres.vt == VT_I4)
		{
			info.vres = (UINT)varVres.llVal;
		}

		// Push gpu information.
		gpuInfo.push_back(info);
		// Clear variant.
		VariantClear(&varName);
		VariantClear(&varDesc);
		VariantClear(&varProcessor);
		VariantClear(&varVMType);
		VariantClear(&varRam);
		VariantClear(&varHres);
		VariantClear(&varVres);
		// Release object.
		pObject->Release();
	}

	pEnumerator->Release();
	pSvc->Release();
	pLoc->Release();
	CoUninitialize();

	vector<GPUInfo>::iterator its = gpuInfo.begin();
	vector<GPUInfo>::iterator ite = gpuInfo.end();
	for (its; its != ite; ++its)
	{
		wstring name = its->name;
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);

		if (g_dwGraphic != G_NVIDIA)
		{
			// Intel(R) Iris(R) Xe Graphics
			if (name.find(_T("intel")) != wstring::npos)
			{
				if (g_dwGraphic == G_UNKNOWN)
				{
					g_dwGraphic = G_INTEL;
				}
			}
			else if (name.find(_T("amd")) != wstring::npos)
			{
				g_dwGraphic = G_AMD;
			}
			else if (name.find(_T("nvidia")) != wstring::npos)
			{
				g_dwGraphic = G_NVIDIA;
			}
		}

		// g_dwGraphic
		PrintOut(_T(">> Name : %s\n   Desc : %s\n   Processor : %s\n   RAM : %.2f GB (%s)\n   HRes : %ld\n   VRes : %ld\n"),
			its->name.c_str(), its->descript.c_str(), its->vprocessor.c_str(), ((float)(its->ram) / SIZE_GIGA), its->ramtype, its->hres, its->vres);
	}
	// else... G_UNKNOWN

	return hr;
}

bool CCuiBoxDlg::CheckVersion()
{
	CVersion ver;
	return ver.CheckUpdate(URL_GITHUB, PATH_VER);
}

// TrayIcon
int CCuiBoxDlg::SetTrayIcon()
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(nid);
	nid.hWnd = m_hWnd;							// Set MainWindow handle
	nid.uID = IDR_MAINFRAME;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = UM_TRAYICON;			// Set Callback
	nid.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	TCHAR szTip[256];
	memset(szTip, 0x00, sizeof(szTip));
	_stprintf(szTip, _T("%s\r\n%s"), APP_NAME, (LPCTSTR)CUtil::GetVersion(RES_ENGLISH));
	lstrcpy(nid.szTip, szTip);

	Shell_NotifyIcon(NIM_ADD, &nid);

	SendMessage(WM_SETICON, (WPARAM)TRUE, (LPARAM)nid.hIcon);
	m_bTrayStatus = true;

	return 0;
}

int CCuiBoxDlg::ModifyTrayIcon()
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(nid);
	nid.hWnd = m_hWnd;							// Set MainWindow handle
	nid.uID = IDR_MAINFRAME;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = UM_TRAYICON;			// Set Callback
	nid.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	TCHAR szTip[256];
	memset(szTip, 0x00, sizeof(szTip));
	_stprintf(szTip, _T("%s\r\n%s"), APP_NAME, (LPCTSTR)CUtil::GetVersion(RES_ENGLISH));
	lstrcpy(nid.szTip, szTip);

	Shell_NotifyIcon(NIM_MODIFY, &nid);

	SendMessage(WM_SETICON, (WPARAM)TRUE, (LPARAM)nid.hIcon);
	m_bTrayStatus = true;

	return 0;
}

LRESULT CCuiBoxDlg::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = GetSafeHwnd();

	switch (lParam)
	{
		case WM_LBUTTONDBLCLK :
		{
			DWORD dwStyle = GetStyle();
			DWORD dwResult = dwStyle & WS_MINIMIZE;
			if (dwResult)
			{
				ShowWindow(SW_RESTORE);
			}
			else
			{
				ShowWindow(SW_SHOW);
			}
			::SetForegroundWindow(hWnd);
		}
		break;
		case WM_RBUTTONDOWN :
		{
			CPoint pt;
			HMENU hMenu = CreatePopupMenu();
			GetCursorPos(&pt);
			AppendMenu(hMenu, MF_STRING, UDM_OPEN, CResourceEx::GetInstance()->LoadStringEx(IDS_OPEN));
			AppendMenu(hMenu, MF_STRING, UDM_OPEN_OUTPUT, CResourceEx::GetInstance()->LoadStringEx(IDS_OPEN_OUTPUT));
			AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
			AppendMenu(hMenu, MF_STRING, UDM_STARTUP, CResourceEx::GetInstance()->LoadStringEx(IDS_STARTUP));
			AppendMenu(hMenu, MF_STRING, UDM_RESET, CResourceEx::GetInstance()->LoadStringEx(IDS_RESET));
			AppendMenu(hMenu, MF_STRING, UDM_APPLY_CHANGES, CResourceEx::GetInstance()->LoadStringEx(IDS_APPLY_CHANGES));
			AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
			// 실행 메뉴 생성
			HMENU hRunMenu = CreatePopupMenu();
			AppendMenu(hRunMenu, MF_STRING, UDM_RUN_CPU, _T("CPU"));
			AppendMenu(hRunMenu, MF_STRING, UDM_RUN_GPU, _T("GPU (for NVIDIA)"));
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hRunMenu, CResourceEx::GetInstance()->LoadStringEx(IDS_RUN_MODE));
			AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
			CheckMenuItem(hMenu, (g_strRunMode.compare("gpu") == 0) ? UDM_RUN_GPU : UDM_RUN_CPU, MF_CHECKED | MF_BYCOMMAND);
			// 언어 설정 메뉴 생성
			HMENU hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, UDM_SET_KO, _T("한국어"));
			AppendMenu(hSubMenu, MF_STRING, UDM_SET_EN, _T("English"));
			AppendMenu(hSubMenu, MF_STRING, UDM_SET_JA, _T("日本語"));
			AppendMenu(hSubMenu, MF_STRING, UDM_SET_ZH, _T("汉语"));
			AppendMenu(hSubMenu, MF_STRING, UDM_SET_VI, _T("Tiếng Việt"));
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, CResourceEx::GetInstance()->LoadStringEx(IDS_SELECT_LANGUAGE));
			AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
			AppendMenu(hMenu, MF_STRING, UDM_CLOSE, CResourceEx::GetInstance()->LoadStringEx(IDS_CLOSE));

			if (m_bStartup)
			{
				CheckMenuItem(hMenu, UDM_STARTUP, MF_CHECKED | MF_BYCOMMAND);
			}

			if		(g_strLang.compare("ko") == 0) { CheckMenuItem(hMenu, UDM_SET_KO, MF_CHECKED | MF_BYCOMMAND); }
			else if (g_strLang.compare("en") == 0) { CheckMenuItem(hMenu, UDM_SET_EN, MF_CHECKED | MF_BYCOMMAND); }
			else if (g_strLang.compare("ja") == 0) { CheckMenuItem(hMenu, UDM_SET_JA, MF_CHECKED | MF_BYCOMMAND); }
			else if (g_strLang.compare("zh") == 0) { CheckMenuItem(hMenu, UDM_SET_ZH, MF_CHECKED | MF_BYCOMMAND); }
			else if (g_strLang.compare("vi") == 0) { CheckMenuItem(hMenu, UDM_SET_VI, MF_CHECKED | MF_BYCOMMAND); }

			::SetForegroundWindow(hWnd);

			TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
		}
		break;
	}

	return 0L;
}

LRESULT CCuiBoxDlg::OnTrayShow(WPARAM wParam, LPARAM lParam)
{
	if (m_bTrayStatus)
	{
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(nid);
		nid.hWnd = m_hWnd;
		nid.uID = IDR_MAINFRAME;
		nid.uFlags = NULL;
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}

	m_bTrayStatus = false;

	SetTrayIcon();

	return 0;
}

LRESULT CCuiBoxDlg::OnMutexOpen(WPARAM wParam, LPARAM lParam)
{
	DWORD dwStyle = GetStyle();
	DWORD dwResult = dwStyle & WS_MINIMIZE;
	if (dwResult)
	{
		ShowWindow(SW_RESTORE);
	}

	this->ShowWindow(SW_SHOW);
	::SetWindowPos(this->m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	::SetWindowPos(this->m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

	return 0;
}

void CCuiBoxDlg::OnOpenMenu()
{
	HWND hWnd = GetSafeHwnd();

	DWORD dwStyle = GetStyle();
	DWORD dwResult = dwStyle & WS_MINIMIZE;
	if (dwResult)
	{
		ShowWindow(SW_RESTORE);
	}
	else
	{
		ShowWindow(SW_SHOW);
	}
	::SetForegroundWindow(hWnd);
}

void CCuiBoxDlg::OnOpenOutputMenu()
{
	TCHAR szPath[512];
	memset(szPath, 0x00, sizeof(szPath));
	if (g_bDefineSavePath)
	{
		wstring wstr_save(g_strSavePath.begin(), g_strSavePath.end());
		_stprintf_s(szPath, _T("%s%s"), (LPCTSTR)CUtil::GetAppRootPath(), wstr_save.c_str());
		if (PathFileExists(szPath))
		{
			ShellExecute(NULL, _T("open"), szPath, NULL, NULL, SW_SHOW);
		}
		else
		{
			AfxMessageBox(_T("The save folder does not exist. Specify and create a folder to save the settings file."));
		}
	}
	else
	{
		_stprintf_s(szPath, _T("%s%s"), (LPCTSTR)CUtil::GetAppRootPath(), PATH_OUTPUT_FOLDER);
		if (PathFileExists(szPath))
		{
			ShellExecute(NULL, _T("open"), szPath, NULL, NULL, SW_SHOW);
		}
		else
		{
			AfxMessageBox(_T("The save folder does not exist. Create a folder to save."));
		}
	}
}

void CCuiBoxDlg::OnCloseMenu()
{
	g_threadInfo.is_exit = true;

	if (m_bTrayStatus)
	{
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(nid);
		nid.hWnd = m_hWnd;
		nid.uID = IDR_MAINFRAME;
		nid.uFlags = NULL;
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}

	m_bTrayStatus = false;

	CDialogEx::OnOK();
}

void CCuiBoxDlg::OnResetMenu()
{
	CRect r;
	if (SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0))
	{
		int i = 0;
		i = (int)((r.Width() - MAINWINDOW_WIDTH) / 2);
		r.left = (i > 0) ? i : r.right - MAINWINDOW_WIDTH - 50;
		i = (int)((r.Height() - MAINWINDOW_HEIGHT) / 2);
		r.top = (i > 0) ? i : 0;
		r.right = r.left + MAINWINDOW_WIDTH;
		r.bottom = r.top + MAINWINDOW_HEIGHT;
	}
	this->MoveWindow(r);
	
	CRect rc;
	GetClientRect(&rc);

	m_nRceWidth = SIZE_RCE_CONSOLEW;
	m_nRceHeight = SIZE_RCE_CONSOLEH;
	RegisterRichEditPos(2);				// Save all.

	if (m_nPosConsole == MODE_UNCHECKED)
	{
		m_nPosConsole = MODE_CHECKED;

		m_ctlSplitter.DeleteRow(1);
		m_ctlSplitter.AddColumn();

		m_btnPosConsole.SetCheck(m_nPosConsole);
		RegisterDirection(TYPE_DIRECTION::TD_HORZ);
	}

	int nBorderSize = SIZE_SPLITTER_GAP * 2;
	m_ctlSplitter.SetColumnWidthForStatic(0, rc.Width() - m_nRceWidth - nBorderSize);
	m_ctlSplitter.SetColumnWidthForStatic(1, m_nRceWidth);

	m_ctlSplitter.Update();
	RedrawControls(rc, CALLER::CALL_SPLITTER);
}

void CCuiBoxDlg::OnRegisterStartup()
{
	CRegistry reg;
	if (reg.Open(REGPATH_AUTORUN))
	{
		if (!m_bStartup)
		{
			TCHAR szPath[MAX_PATH2];
			memset(szPath, 0x00, sizeof(szPath));
			GetModuleFileName(NULL, szPath, MAX_PATH2);
			reg.Write(APP_NAME, szPath);
			m_bStartup = TRUE;
		}
		else
		{
			reg.Delete(APP_NAME);
			m_bStartup = FALSE;
		}
		reg.Close();
	}
}

void CCuiBoxDlg::OnApplyChanges()
{
	// If failed to make file, remove cuibox_main.py.
	if (!MakeMainFile())
	{
		// Remove cuibox_main.py
		TCHAR szPath[512];
		memset(szPath, 0x00, sizeof(szPath));
		_stprintf_s(szPath, _T("%s%s"), (LPCTSTR)CUtil::GetAppRootPath(), CUIBOX_MAINW);
		DeleteFile(szPath);
	}

	MessageBox(CResourceEx::GetInstance()->LoadStringEx(IDS_RESTART_PROGRAM), MB_OK);
	RestartApp();		// Restart program.
}

void CCuiBoxDlg::OnRunCpu()
{
	g_strRunMode = "cpu";
	SaveJson();
	RestartApp();		// Restart program.
}

void CCuiBoxDlg::OnRunGpu()
{
	g_strRunMode = "gpu";
	SaveJson();
	RestartApp();		// Restart program.
}

void CCuiBoxDlg::OnLangKo()
{
	g_strLang = "ko";
	CResourceEx::GetInstance()->SetLanguage(KO);
	SaveJson();
}
void CCuiBoxDlg::OnLangEn()
{
	g_strLang = "en";
	CResourceEx::GetInstance()->SetLanguage(EN);
	SaveJson();
}
void CCuiBoxDlg::OnLangJa()
{
	g_strLang = "ja";
	CResourceEx::GetInstance()->SetLanguage(JA);
	SaveJson();
}
void CCuiBoxDlg::OnLangZh()
{
	g_strLang = "zh";
	CResourceEx::GetInstance()->SetLanguage(ZH);
	SaveJson();
}
void CCuiBoxDlg::OnLangVi()
{
	g_strLang = "vi";
	CResourceEx::GetInstance()->SetLanguage(VI);
	SaveJson();
}

void CCuiBoxDlg::RestartApp()
{
	// Restart program.
	PostMessage(WM_CLOSE);
	ShellExecute(nullptr, L"open", AfxGetApp()->m_pszExeName, nullptr, nullptr, SW_SHOWNORMAL);
}

//	JSON
//	{
//		"lang": "ko",
//		"mode_comment": "h(horizontal) or v(vertical)",
//		"mode": "horz",
//		"run_comment": "cpu, gpu(Nvidia)",
//		"run": "cpu",
// 	    "auto_update": true
//	}
void CCuiBoxDlg::SaveJson()
{
	std::stringstream ss;
	if (g_bDefineSavePath)
	{
		ss << "{\n"
			<< "\t\"lang\": \"" << g_strLang << "\",\n"
			<< "\t\"mode_comment\": \"h(horizontal) or v(vertical)\",\n"
			<< "\t\"mode\": \"horz\",\n"
			<< "\t\"run_comment\": \"cpu, gpu(Nvidia)\",\n"
			<< "\t\"run\": \"" << g_strRunMode << "\",\n"
			<< "\t\"auto_update\": true\n"
			<< "\t\"save_path\": \"" << g_strSavePath << "\",\n"
			<< "}\n"
			<< endl;
	}
	else
	{
		ss << "{\n"
			<< "\t\"lang\": \"" << g_strLang << "\",\n"
			<< "\t\"mode_comment\": \"h(horizontal) or v(vertical)\",\n"
			<< "\t\"mode\": \"horz\",\n"
			<< "\t\"run_comment\": \"cpu, gpu(Nvidia)\",\n"
			<< "\t\"run\": \"" << g_strRunMode << "\",\n"
			<< "\t\"auto_update\": true\n"
			<< "}\n"
			<< endl;
	}

	string jsonFile = CUtil::GetAppRootPathA();
	jsonFile.append(FLJ_CUIBOXA);
	std::ofstream outFile(jsonFile.c_str(), std::ios::out | std::ios::binary);
	if (outFile.is_open())
	{
		outFile << ss.str();
		outFile.close();
	}
	else
	{
		PrintOut(_T(">> 파일을 열 수 없습니다.\r\n"));
	}
}

bool CCuiBoxDlg::LoadNvidiaLib()
{
	m_hNvml = LoadLibrary(L"nvml.dll");
	if (!m_hNvml)
	{
		PrintOut(_T(">> Failed to load NVIDIA library.\r\n"));
		return false;
	}

	NvmlInit = (NvmlInit_t)GetProcAddress(m_hNvml, "nvmlInit");
	if (!NvmlInit)
	{
		FreeLibrary(m_hNvml);
		return false;
	}

	NvmlShutdown = (NvmlShutdown_t)GetProcAddress(m_hNvml, "nvmlShutdown");
	if (!NvmlShutdown)
	{
		FreeLibrary(m_hNvml);
		return false;
	}

	NvmlErrorString = (NvmlErrorString_t)GetProcAddress(m_hNvml, "nvmlErrorString");
	if (!NvmlErrorString)
	{
		FreeLibrary(m_hNvml);
		return false;
	}

	NvmlDeviceGetCount = (NvmlDeviceGetCount_t)GetProcAddress(m_hNvml, "nvmlDeviceGetCount");
	if (!NvmlDeviceGetCount)
	{
		FreeLibrary(m_hNvml);
		return false;
	}

	NvmlDeviceGetHandleByIndex = (NvmlDeviceGetHandleByIndex_t)GetProcAddress(m_hNvml, "nvmlDeviceGetHandleByIndex");
	if (!NvmlDeviceGetHandleByIndex)
	{
		FreeLibrary(m_hNvml);
		return false;
	}

	NvmlDeviceGetUtilizationRates = (NvmlDeviceGetUtilizationRates_t)GetProcAddress(m_hNvml, "nvmlDeviceGetUtilizationRates");
	if (!NvmlDeviceGetUtilizationRates)
	{
		FreeLibrary(m_hNvml);
		return false;
	}

	return true;
}