
// CuiBoxDlg.h : header file
//

#pragma once

#include "SplitterCtrl.h"
#include "UsageInfoWnd.h"
#include "../MonosUtil/XMutex.h"
#include "../MonosUtil/X3Button.h"
#include "../BrowserHelper/BrowserHelper.h"

extern _THREAD_INFO g_threadInfo;
extern CWnd* g_pMainWnd;
extern string g_strLang;
extern string g_strRunMode;
extern string g_strSavePath;
extern bool g_bUpdate;
extern bool g_bDebugMode;
extern bool g_bDefineSavePath;

extern DWORD g_dwGraphic;
extern double g_nCpu;
extern double g_nGpu;
extern double g_nRam;
extern double g_nVram;

// CCuiBoxDlg dialog
class CCuiBoxDlg : public CDialogEx
{
// Construction
public:
	CCuiBoxDlg(CWnd* pParent = nullptr);	// standard constructor
	~CCuiBoxDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CUIBOX_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual void PreSubclassWindow();
	virtual BOOL OnInitDialog();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnDestroy();
	afx_msg void OnNcPaint();
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg BOOL OnNcActivate(BOOL bActive);
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnBnClickedBtnMin();
	afx_msg void OnBnClickedBtnMax();
	afx_msg void OnBnClickedBtnClose();
	afx_msg void OnBnClickedBtnShowConsole();
	afx_msg void OnBnClickedBtnPosConsole();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnChangeSplitter(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNavigate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTrayShow(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMutexOpen(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	std::unique_ptr<CWebBrowser> m_pWebBrowser{};
	HMODULE m_hNvml;

public:
	CStatic m_sttBrowser;
	CStatic m_sttConsole;
	CSplitterCtrl m_ctlSplitter;
	CSplitterCtrlStyle1 m_SplitterStyle;
	CRichEditCtrl* m_pRichEdit;
	CUsageInfoWnd* m_pUsageInfo;
	CX3Button m_btnClose;
	CX3Button m_btnMax;
	CX3Button m_btnMin;
	CX3Button m_btnShowConsole;
	CX3Button m_btnPosConsole;

	int m_nShowConsole;	// 0 : Unchecked (Hide), 1 : Checked (Show)
	int m_nPosConsole;	// 0 : Unchecked (Vertical), 1 : Checked (Horizontal)

	CXMutex m_Mutex;
	
	CFont m_FontDefault;
	CFont m_FontTitleBold;
	CFont m_FontTitleThin;

	UINT m_nLastLine;
	UINT m_nRceWidth;
	UINT m_nRceHeight;
	CRect m_rect;
	CSize m_szIcon;
	CImage m_imgLogo;
	CDC* m_pLogoDC;

	HBITMAP m_hBitmap;
	TCHAR m_szTitle[128];				// App name
	
	bool m_bStartup;
	bool m_bTrayStatus;

public:
	bool CreateControls();
	void RedrawControls(CRect rect, UINT nCaller = CALLER::CALL_MAIN);
	bool RunConsole();
	bool CloseConsole();
	void OutputData();
	void ParserText(char* pszData, DWORD dwSize);
	void RaiseError(char* pszData, DWORD dwSize);
	void MakeAnsiText(char* pszText, DWORD dwSize, UINT nType = TR_NORMAL);
	list<TEXT_INFO_DATA *> MakeCuiText(LPCTSTR lpText, DWORD dwSize, UINT nType);
	void CuiTextOut(LPCTSTR lpText);
	void CuiTextOut(LPCTSTR lpText, COLORREF crText, COLORREF crBg = RCE_BG_COLOR);
	void AutoScroll();
	CRect GetInitWindowPos();
	void RegisterWindowPos();
	void RegisterRichEditPos(int nType);
	void RegisterDirection(TYPE_DIRECTION td = TYPE_DIRECTION::TD_HORZ);
	void RegisterShowWindow(bool bShow = true);
	void SetPid(DWORD dwPid = 0);
	void OnInit();
	void InitBackgroundImage(int cx = 0, int cy = 0);
	void SetWndPos();
	void GetConfig();
	bool MakeMainFile();

	HRESULT GetVideoCardInfo();
	bool CheckVersion();

	int SetTrayIcon();
	int ModifyTrayIcon();
	
	// Menu
	void OnOpenMenu();
	void OnOpenOutputMenu();
	void OnCloseMenu();
	void OnResetMenu();
	void OnRegisterStartup();
	void OnApplyChanges();
	void OnRunCpu();
	void OnRunGpu();
	void OnLangKo();
	void OnLangEn();
	void OnLangJa();
	void OnLangZh();
	void OnLangVi();

	// Restart program
	void RestartApp();
	// Save settings
	void SaveJson();
	// Load NVIDIA Library.
	bool LoadNvidiaLib();
};
