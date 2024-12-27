#if !defined(AFX_X3BUTTON_H__E5364F0B_0C9A_4C07_9420_15DE7B4BDE40__INCLUDED_)
#define AFX_X3BUTTON_H__E5364F0B_0C9A_4C07_9420_15DE7B4BDE40__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// X3Button.h : header file
//
#include "BaseBtn.h"
/////////////////////////////////////////////////////////////////////////////
// CX3Button window

enum
{
	MODE_UNCHECKED = 0,
	MODE_CHECKED,
};

// 3s Button
class CX3Button : public CBaseBtn
{
// Construction
public:
	CX3Button();
	virtual ~CX3Button();
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CX3Button)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	int			m_nTextWidth;
	int			m_nCount;
	int			m_nPos;
	DWORD		m_dwStyle;
	USHORT		m_nState;

	BOOL		m_bPaint;
	BOOL		m_bDrawText;
	BOOL		m_bEnableCheckMode;

	HBITMAP		m_hBkImg;
	HCURSOR		m_hHandCur;

	COLORREF	m_Mask;

	CString		m_strText;

protected:
	HBITMAP GetImage(HDC hDC, const RECT& rect, int nPos);

public:
	void SetBkImage(HBITMAP hBitmap) { m_hBkImg = hBitmap; };

	void SetCheck(int nCheck);
	void EnableCheckButton(BOOL bEnableCheckMode) { m_bEnableCheckMode = bEnableCheckMode; };
	void SetText(TCHAR* pszText, BOOL bDraw = TRUE) { m_strText.Format(_T("%s"), pszText); m_bDrawText = bDraw; }
	// Generated message map functions
protected:
	//{{AFX_MSG(CX3Button)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_X3BUTTON_H__E5364F0B_0C9A_4C07_9420_15DE7B4BDE40__INCLUDED_)
