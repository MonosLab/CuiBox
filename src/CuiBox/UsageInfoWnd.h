#ifndef __USAGE_INFO_WINDOW_20241213__
#define __USAGE_INFO_WINDOW_20241213__

#define	UIW_RECT_FULL	CRect(0, 0, 180, 30)
#define	UIW_RECT_MINI	CRect(0, 5, 180, 25)

struct UsageInfoConfig
{
	UINT m_nRam;			// Memory
	UINT m_nVram;			// Video Memory
	UINT m_nRad;			// Radian
	UINT m_nFrameThick;		// frame border size
	CFont m_font;			// 폰트 객체 추가
	CBrush m_brFrameFill;
	CBrush m_brFrameLine;
	COLORREF m_crBg;
	COLORREF m_crCpu;
	COLORREF m_crGpu;
	COLORREF m_crRam;
	COLORREF m_crVram;
	COLORREF m_crText;

	UsageInfoConfig()
		: m_nRam(0)
		, m_nVram(0)
		, m_nRad(3)
		, m_nFrameThick(1)
		, m_crBg(RGB(255, 255, 255))
		, m_crCpu(RGB(255, 200, 200))
		, m_crGpu(RGB(200, 255, 200))
		, m_crRam(RGB(200, 200, 255))
		, m_crVram(RGB(200, 255, 255))
		, m_crText(RGB(0, 0, 0))
	{
		m_font.CreatePointFont(70, _T("Arial"));
		m_brFrameLine.CreateSolidBrush(RGB(200, 200, 200));
		m_brFrameFill.CreateSolidBrush(RGB(240, 240, 230));
	}
};

class CUsageInfoWnd : public CStatic
{
public:
	CUsageInfoWnd(DWORD dwMode = G_INTEL);
	~CUsageInfoWnd();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();

public:
	void SetRamInfo(UINT ram, UINT vram);
	void SetConfig(UsageInfoConfig u) { memcpy(&uic, &u, sizeof(UsageInfoConfig)); }
	void UpdateUsage(double cpu, double gpu, double ram, double vram);
	void ViewMode(DWORD dwMode) { m_dwMode = dwMode; }

public:
	double usage_cpu;
	double usage_gpu;
	double usage_ram;
	double usage_vram;

private:
	UsageInfoConfig uic;
	DWORD m_dwMode;
	CRect m_rectView;

private:
	void DrawUsageInfo(CDC* pDC);
};

#endif	// #ifdef __USAGE_INFO_WINDOW_20241213__