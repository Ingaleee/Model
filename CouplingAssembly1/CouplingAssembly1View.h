#pragma once

class CCouplingAssembly1View : public CView
{
protected:
	CCouplingAssembly1View() noexcept;
	DECLARE_DYNCREATE(CCouplingAssembly1View)

public:
	CCouplingAssembly1Doc* GetDocument() const;
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

public:
	virtual ~CCouplingAssembly1View();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline CCouplingAssembly1Doc* CCouplingAssembly1View::GetDocument() const
   { return reinterpret_cast<CCouplingAssembly1Doc*>(m_pDocument); }
#endif
