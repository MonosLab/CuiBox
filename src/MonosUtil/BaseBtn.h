#pragma once

#include "MonosUtil.h"

// CBaseBtn
class CBaseBtn : public CButton
{
public:
	CBaseBtn();
	virtual ~CBaseBtn();

protected:
	int m_nWidth;
	int m_nHeight;
	int m_nImgHeight;
	HBITMAP m_hImg;
	COLORREF m_crMask;
	COLORREF m_crBg;

	BOOL m_bMouseOver;
	Bitmap * m_pImage;

//Overrides
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void SetBkColor(COLORREF crBg) { m_crBg = crBg; }
	virtual BOOL SetBitmap(TCHAR *pszFile, COLORREF crMask);
	virtual BOOL SetImage(TCHAR *pszFile);
	virtual int GetWidth();
	virtual BOOL GetImgSize(int &cx, int &cy);
	
protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);


private:
	BOOL LoadBitmap(IMAGEDATA &imgData, TCHAR *pszFile);
	BOOL LoadImage(TCHAR *pszFile);

	DECLARE_MESSAGE_MAP()
};


