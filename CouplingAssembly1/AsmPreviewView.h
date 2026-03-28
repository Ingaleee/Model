#pragma once
#include <afxcview.h>

class CCouplingAssembly1Doc;

class CAsmPreviewView : public CView
{
protected:
	CAsmPreviewView() noexcept;
	DECLARE_DYNCREATE(CAsmPreviewView)

public:
	CCouplingAssembly1Doc* GetDocument() const;

public:
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual ~CAsmPreviewView();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline CCouplingAssembly1Doc* CAsmPreviewView::GetDocument() const
{
	return reinterpret_cast<CCouplingAssembly1Doc*>(m_pDocument);
}
#endif