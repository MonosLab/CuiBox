#include "pch.h"
#include "UsageInfoWnd.h"

CUsageInfoWnd::CUsageInfoWnd(DWORD dwMode)
    : usage_cpu(0.0)
    , usage_gpu(0.0)
    , usage_ram(0.0)
    , usage_vram(0.0)
{
    m_dwMode = dwMode;
    if (m_dwMode == G_NVIDIA)
    {
        m_rectView = UIW_RECT_FULL;
    }
    else
    {
        m_rectView = UIW_RECT_MINI;
    }
}

CUsageInfoWnd::~CUsageInfoWnd()
{
}

BEGIN_MESSAGE_MAP(CUsageInfoWnd, CStatic)
ON_WM_PAINT()
END_MESSAGE_MAP()

void CUsageInfoWnd::OnPaint()
{
	CPaintDC dc(this);
    DrawUsageInfo(&dc);
}

void CUsageInfoWnd::DrawUsageInfo(CDC* pDC)
{
    CDC memDC;
    CBitmap bitmap, *pOldBitmap;

    memDC.CreateCompatibleDC(pDC);
    bitmap.CreateCompatibleBitmap(pDC, UIW_RECT_FULL.Width(), UIW_RECT_FULL.Height());
    pOldBitmap = memDC.SelectObject(&bitmap);
    // 전체 배경색
    memDC.FillSolidRect(&UIW_RECT_FULL, uic.m_crBg);
    // 테두리
    CRgn rgn;
    rgn.CreateRoundRectRgn(m_rectView.left, m_rectView.top, m_rectView.right, m_rectView.bottom, uic.m_nRad, uic.m_nRad);
    memDC.FillRgn(&rgn, &uic.m_brFrameFill);
    memDC.FrameRgn(&rgn, &uic.m_brFrameLine, uic.m_nFrameThick, uic.m_nFrameThick);
    // 텍스트 폰트 및 색상 설정
    memDC.SelectObject(&uic.m_font);
    memDC.SetTextColor(uic.m_crText);
    memDC.SetBkMode(TRANSPARENT);

#ifdef _SAMPLE_
    CRect rect[] = {
        CRect(30, 3, 80, 11),
        CRect(30, 17, 80, 25),
        CRect(125, 3, 175, 11),
        CRect(125, 17, 175, 25)
    };

    memDC.FillSolidRect(rect[0], uic.m_crCpu);
    memDC.FillSolidRect(rect[1], uic.m_crGpu);
    memDC.FillSolidRect(rect[2], uic.m_crRam);
    memDC.FillSolidRect(rect[3], uic.m_crVram);
#else
    TCHAR szCpu[8] = { 0x00, };
    TCHAR szGpu[8] = { 0x00, };
    TCHAR szRam[8] = { 0x00, };
    TCHAR szVram[8] = { 0x00, };

    switch (m_dwMode)
    {
        case G_NVIDIA:
            {
                memDC.TextOut(5, 2, _T("CPU"));
                memDC.TextOut(5, 16, _T("GPU"));
                memDC.TextOut(96, 2, _T("RAM"));
                memDC.TextOut(91, 16, _T("VRAM"));

                CRect rect[] = {
                    CRect(29, 2, 81, 13),
                    CRect(29, 16, 81, 27),
                    CRect(122, 2, 174, 13),
                    CRect(122, 16, 174, 27),
                    CRect(30, 3, (UINT)(30 + usage_cpu / 2), 12),         // MAX POS : 80
                    CRect(30, 17, (UINT)(30 + usage_gpu / 2), 26),        // MAX POS : 80
                    CRect(123, 3, (UINT)(123 + usage_ram / 2), 12),       // MAX POS : 175
                    CRect(123, 17, (UINT)(123 + usage_vram / 2), 26)      // MAX POS : 175
                };

                // 테두리
                CPen pen(PS_SOLID, 1, TTB_LINE_PEN);
                CPen* pOldPen = memDC.SelectObject(&pen);
                memDC.Rectangle(&rect[0]);
                memDC.Rectangle(&rect[1]);
                memDC.Rectangle(&rect[2]);
                memDC.Rectangle(&rect[3]);
                memDC.SelectObject(pOldPen);
                // 사용량
                memDC.FillSolidRect(rect[4], uic.m_crCpu);
                memDC.FillSolidRect(rect[5], uic.m_crGpu);
                memDC.FillSolidRect(rect[6], uic.m_crRam);
                memDC.FillSolidRect(rect[7], uic.m_crVram);
                // 수치 표시
                memset(szCpu, 0x00, sizeof(szCpu));
                memset(szGpu, 0x00, sizeof(szGpu));
                memset(szRam, 0x00, sizeof(szRam));
                memset(szVram, 0x00, sizeof(szVram));
                _stprintf_s(szCpu, _T("%ld%%"), (UINT)usage_cpu);
                _stprintf_s(szGpu, _T("%ld%%"), (UINT)usage_gpu);
                _stprintf_s(szRam, _T("%ld%%"), (UINT)usage_ram);
                _stprintf_s(szVram, _T("%ld%%"), (UINT)usage_vram);
                memDC.DrawText(szCpu, rect[0], DT_CENTER | DT_VCENTER);
                memDC.DrawText(szGpu, rect[1], DT_CENTER | DT_VCENTER);
                memDC.DrawText(szRam, rect[2], DT_CENTER | DT_VCENTER);
                memDC.DrawText(szVram, rect[3], DT_CENTER | DT_VCENTER);
            }
            break;
        default:
            {
                memDC.TextOut(5, 10, _T("CPU"));
                memDC.TextOut(96, 10, _T("RAM"));

                CRect rect[] = {
                    CRect(29, 8, 81, 21),
                    CRect(122, 8, 174, 21),
                    CRect(30, 9, (UINT)(30 + usage_cpu / 2), 20),         // MAX POS : 80
                    CRect(123, 9, (UINT)(123 + usage_ram / 2), 20),       // MAX POS : 173
                };

                // 테두리
                CPen pen(PS_SOLID, 1, TTB_LINE_PEN);
                CPen* pOldPen = memDC.SelectObject(&pen);
                memDC.Rectangle(&rect[0]);
                memDC.Rectangle(&rect[1]);
                memDC.SelectObject(pOldPen);
                // 사용량
                memDC.FillSolidRect(rect[2], uic.m_crCpu);
                memDC.FillSolidRect(rect[3], uic.m_crRam);
                memDC.SetTextColor(RGB(50, 50, 50));
                // 수치 표시
                memset(szCpu, 0x00, sizeof(szCpu));
                memset(szRam, 0x00, sizeof(szRam));
                _stprintf_s(szCpu, _T("%ld%%"), (UINT)usage_cpu);
                _stprintf_s(szRam, _T("%ld%%"), (UINT)usage_ram);
                memDC.DrawText(szCpu, rect[0], DT_CENTER | DT_VCENTER);
                memDC.DrawText(szRam, rect[1], DT_CENTER | DT_VCENTER);
            }
            break;
    }

#endif

    pDC->BitBlt(UIW_RECT_FULL.left, UIW_RECT_FULL.top, UIW_RECT_FULL.Width(), UIW_RECT_FULL.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBitmap);
}

void CUsageInfoWnd::SetRamInfo(UINT ram, UINT vram)
{
    uic.m_nRam = ram;
    uic.m_nVram = vram;
}

void CUsageInfoWnd::UpdateUsage(double cpu, double gpu, double ram, double vram)
{
    UPDATE_USAGE(usage_cpu, cpu);
    UPDATE_USAGE(usage_gpu, gpu);
    UPDATE_USAGE(usage_ram, ram);
    UPDATE_USAGE(usage_vram, vram);
    
    Invalidate();
    UpdateWindow();
}