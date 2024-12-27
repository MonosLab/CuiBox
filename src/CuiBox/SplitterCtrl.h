//==========================================================
// Author: Baradzenka Aleh (baradzenka@gmail.com) (https://github.com/baradzenka/SplitterCtrl)
// Modifier: Kwangsoo Seo (https://monoslab.github.io)
//==========================================================

#ifndef __SPLITTERCTRL_20240531__
#define __SPLITTERCTRL_20240531__

#if (!defined(_MSC_VER) && __cplusplus < 201103L) || (defined(_MSC_VER) && _MSC_VER < 1900)   // C++11 is not supported.
	#define nullptr  NULL
	#define override
#endif

// Splitter의 border값 수정 가능하도록 함수 제공
#define MODIFIED_BY_MONO_20230227


class CSplitterCtrl : public CWnd
{
	DECLARE_DYNCREATE(CSplitterCtrl)

public:
	struct Draw
	{
		virtual void DrawBegin(CSplitterCtrl const * /*ctrl*/, CDC * /*dc*/) {}
		virtual void DrawSplitter(CSplitterCtrl const * /*ctrl*/, CDC * /*dc*/, bool /*horz*/, int /*idx*/, CRect const * /*rect*/) {}
		virtual void DrawDragRect(CSplitterCtrl const * /*ctrl*/, CDC * /*dc*/, bool /*horz*/, bool /*firstTime*/, CRect const * /*rectOld*/, CRect const * /*rectNew*/) {}
		virtual void DrawBorder(CSplitterCtrl const * /*ctrl*/, CDC * /*dc*/, CRect const * /*rect*/) {}
		virtual void DrawEnd(CSplitterCtrl const * /*ctrl*/, CDC * /*dc*/) {}
	};
	interface IRecalc
	{
		virtual int GetBorderWidth(CSplitterCtrl const *ctrl, IRecalc *base) = 0;
		virtual int GetVertSplitterWidth(CSplitterCtrl const *ctrl, IRecalc *base) = 0;   // width of vertical splitter.
		virtual int GetHorzSplitterHeight(CSplitterCtrl const *ctrl, IRecalc *base) = 0;   // height of horizontal splitter.
	};

public:
	CSplitterCtrl();
	~CSplitterCtrl();

public:
	bool Create(CWnd *parentWnd, DWORD style, RECT const &rect, UINT id);
#ifdef MODIFIED_BY_MONO_20230227
	void SetSplitterBorderWidth(int nBorder = 1, int nVert = 4, int nHorz = 4)
	{
		m_nBorder = nBorder;
		m_nVertBorderWidth = nVert;
		m_nHorzBorderWidth = nHorz;
	}
#endif
	bool AddRow();
	bool AddColumn();
	bool InsertRow(int row);   // insert before row.
	bool InsertColumn(int col);   // insert before col.
	void DeleteRow(int row);
	void DeleteColumn(int col);
	void DeleteAll();
	void Update();   // recalculate control.

	void SetDrawManager(Draw *p/*or null*/);
	Draw *GetDrawManager();
	void SetRecalcManager(IRecalc *p/*or null*/);   // null for default manager.
	IRecalc *GetRecalcManager();

	void SetCursors(HCURSOR horz, HCURSOR vert, HCURSOR cross);
	void SetCursors(HMODULE module, UINT horz, UINT vert, UINT cross);

	void SetWindow(int row, int col, HWND hWnd);
	HWND GetWindow(int row, int col) const;

	void SetColumnWidthForStatic(int col, int width);   // only for ResizeStatic and ResizeStaticFull.
	void SetRowHeightForStatic(int row, int height);   // only for ResizeStatic and ResizeStaticFull.

	void SetColumnWidthForDynamic(int col, float percent);   // for ResizeDynamic (for sure use Update() after each call).
	void SetColumnWidthsForDynamic(int const *percent/*in*/);   // set width for every column (number items in 'percent'==GetNumberColumn()).
	void SetRowHeightForDynamic(int row, float percent);   // for ResizeDynamic (for sure use Update() after each call).
	void SetRowHeightsForDynamic(int const *percent/*in*/);   // set height for every row (number items in 'percent'==GetNumberRow()).

	void SetRowsEqualHeight();
	void SetColumnsEqualWidth();

	void SetWindowMinWidth(int width);   // min width of every window.
	int GetWindowMinWidth() const;
	void SetWindowMinHeight(int height);   // min height of every window.
	int GetHeightMinWindow() const;

	int GetNumberRow() const;
	int GetNumberColumn() const;
	void HitTest(CPoint point, int *horz/*out,or null*/, int *vert/*out,or null*/) const;   // searching splitter in point.
	CRect GetWindowRect(int row, int col) const;
	CRect GetSplitterRect(bool horz, int idx) const;

	enum Snap
	{
		SnapLeftTop,
		SnapLeftBottom,
		SnapRightTop,
		SnapRightBottom
	};
	void SetSnapMode(Snap mode);
	Snap GetSnapMode() const;

	enum Resize
	{
		ResizeStatic,
		ResizeStaticFull,
		ResizeDynamic
	};
	void SetResizeMode(Resize mode);
	Resize GetResizeMode() const;

	enum Dragging
	{
		DraggingStatic,
		DraggingDynamic
	};
	void SetDraggingMode(Dragging mode);
	Dragging GetDraggingMode() const;

	bool IsDragging() const;
	void GetDraggingState(bool *dragHorz/*out,or null*/, bool *dragVert/*out,or null*/, bool *dragCross/*out,or null*/) const;
	void CancelDragging();

	void ActivateHorzSplitter(int idx, bool active);
	bool IsHorzSplitterActive(int idx) const;
	void ActivateVertSplitter(int idx, bool active);
	bool IsVertSplitterActive(int idx) const;

	void ShowBorder(bool show);
	bool IsBorderVisible() const;

	int GetBorderWidth() const;
	int GetVertSplitterWidth() const;
	int GetHorzSplitterHeight() const;

	bool LoadState(CWinApp *app, TCHAR const *section, TCHAR const *entry);   // load state from registry.
	bool SaveState(CWinApp *app, TCHAR const *section, TCHAR const *entry) const;   // save state in registry.
	void LoadState(CArchive *ar);
	void SaveState(CArchive *ar) const;

private:
	class Private;
	Private &priv;
#ifdef MODIFIED_BY_MONO_20230227
	int m_nBorder;
	int m_nVertBorderWidth;
	int m_nHorzBorderWidth;
#endif

protected:
	DECLARE_MESSAGE_MAP()
	BOOL Create(LPCTSTR className, LPCTSTR windowName, DWORD style, const RECT &rect, CWnd *parentWnd, UINT id, CCreateContext *context) override;
	afx_msg void OnDestroy();
	afx_msg void OnNcPaint();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS *lpncsp);
	afx_msg BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg LRESULT OnNcHitTest(CPoint point);
};

struct CSplitterCtrlStyle1 : CSplitterCtrl::Draw, CSplitterCtrl::IRecalc
{
	void Install(CSplitterCtrl *ctrl);

private:
	// CSplitterCtrl::Draw.
	void DrawSplitter(CSplitterCtrl const *ctrl, CDC *dc, bool horz, int idx, CRect const *rect) override;
	void DrawDragRect(CSplitterCtrl const *ctrl, CDC *dc, bool horz, bool firstTime, CRect const *rectOld, CRect const *rectNew) override;
	void DrawBorder(CSplitterCtrl const *ctrl, CDC *dc, CRect const *rect) override;
	void DrawEnd(CSplitterCtrl const *ctrl, CDC *dc) override;

	// CSplitterCtrl::IRecalc.
	int GetBorderWidth(CSplitterCtrl const *ctrl, IRecalc *base) override;
	int GetVertSplitterWidth(CSplitterCtrl const *ctrl, IRecalc *base) override;
	int GetHorzSplitterHeight(CSplitterCtrl const *ctrl, IRecalc *base) override;

protected:
	virtual COLORREF GetBackgroundColor() { return ::GetSysColor(COLOR_WINDOW); }
	virtual bool IsInnerBorderVisible() { return true; }
	virtual COLORREF GetInnerBorderColor() { return ::GetSysColor(COLOR_BTNSHADOW); }
	virtual COLORREF GetOuterBorderColor() { return ::GetSysColor(COLOR_BTNSHADOW); }
	virtual bool IsDotsVisible() { return true; }
	virtual COLORREF GetDotsColor() { return ::GetSysColor(COLOR_BTNSHADOW); }
	virtual CBrush *GetDragBrush() { return nullptr; }

	void DrawSplitterDots(CDC *dc, CRect const *rect, bool horz, int number, int size, COLORREF color) const;
};

class CSplitterCtrlStyle2 : public CSplitterCtrlStyle1
{
	bool IsDotsVisible() override { return false; }
};

class CSplitterCtrlStyle3 : public CSplitterCtrlStyle1
{
	COLORREF GetBackgroundColor() override { return ::GetSysColor(COLOR_BTNFACE); }
	bool IsInnerBorderVisible() override { return false; }
};

class CSplitterCtrlStyle4 : public CSplitterCtrlStyle3
{
	bool IsDotsVisible() override { return false; }
};

class CSplitterCtrlStyle5 : public CSplitterCtrlStyle1
{
	// CSplitterCtrl::IRecalc.
	int GetVertSplitterWidth(CSplitterCtrl const * /*ctrl*/, IRecalc * /*base*/) override { return 6; }
	int GetHorzSplitterHeight(CSplitterCtrl const * /*ctrl*/, IRecalc * /*base*/) override { return 6; }

	COLORREF GetBackgroundColor() override { return RGB(45,64,94); }
	bool IsInnerBorderVisible() override { return false; }
	COLORREF GetOuterBorderColor() override { return RGB(45,64,94); }
	COLORREF GetDotsColor() override { return RGB(206,212,223); }
	CBrush *GetDragBrush() override { static CBrush br(RGB(128,128,128)); return &br; }
};

class CSplitterCtrlStyle6 : public CSplitterCtrlStyle5
{
	bool IsDotsVisible() override { return false; }
};

class CSplitterCtrlStyle7 : public CSplitterCtrlStyle1
{
	// CSplitterCtrl::Draw.
	void DrawSplitter(CSplitterCtrl const *ctrl, CDC *dc, bool horz, int idx, CRect const *rect) override;
	void DrawEnd(CSplitterCtrl const *ctrl, CDC *dc) override;

	bool IsInnerBorderVisible() override { return false; }
	COLORREF GetOuterBorderColor() override { return RGB(77,115,61); }
	bool IsDotsVisible() override { return false; }

protected:
	void DrawGradient(CDC *dc, CRect const *rc, bool horz, COLORREF clrTop, COLORREF clrBottom) const;
};

class CSplitterCtrlStyle8 : public CSplitterCtrlStyle1
{
	// CSplitterCtrl::Draw.
	void DrawSplitter(CSplitterCtrl const *ctrl, CDC *dc, bool horz, int idx, CRect const *rect) override;
	void DrawBorder(CSplitterCtrl const *ctrl, CDC *dc, CRect const *rect) override;
	void DrawEnd(CSplitterCtrl const *ctrl, CDC *dc) override;

		// CSplitterCtrl::IRecalc.
	int GetBorderWidth(CSplitterCtrl const * /*ctrl*/, IRecalc * /*base*/) override { return 2; }

	bool IsInnerBorderVisible() override { return false; }
};

template<typename STYLE>
struct CSplitterCtrlEx : CSplitterCtrl
{
	CSplitterCtrlEx()
	{
		style.Install(this);
	}
	STYLE style;
};

#endif	// #ifndef __SPLITTERCTRL_20240531__