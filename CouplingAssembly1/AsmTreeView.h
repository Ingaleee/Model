#pragma once
#include <afxcview.h>

class CCouplingAssembly1Doc;

class CAsmTreeView : public CTreeView
{
protected:
	CAsmTreeView() noexcept;
	DECLARE_DYNCREATE(CAsmTreeView)

public:
	CCouplingAssembly1Doc* GetDocument() const;

public:
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual ~CAsmTreeView();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	afx_msg void OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRclick(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline CCouplingAssembly1Doc* CAsmTreeView::GetDocument() const
{
	return reinterpret_cast<CCouplingAssembly1Doc*>(m_pDocument);
}
#endif
