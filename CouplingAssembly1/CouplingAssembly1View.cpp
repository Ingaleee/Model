#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "CouplingAssembly1.h"
#endif

#include "CouplingAssembly1Doc.h"
#include "CouplingAssembly1View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CCouplingAssembly1View, CView)

BEGIN_MESSAGE_MAP(CCouplingAssembly1View, CView)
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
END_MESSAGE_MAP()

CCouplingAssembly1View::CCouplingAssembly1View() noexcept
{
}

CCouplingAssembly1View::~CCouplingAssembly1View()
{
}

BOOL CCouplingAssembly1View::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

void CCouplingAssembly1View::OnDraw(CDC*)
{
	CCouplingAssembly1Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;
}

BOOL CCouplingAssembly1View::OnPreparePrinting(CPrintInfo* pInfo)
{
	return DoPreparePrinting(pInfo);
}

void CCouplingAssembly1View::OnBeginPrinting(CDC*, CPrintInfo*)
{
}

void CCouplingAssembly1View::OnEndPrinting(CDC*, CPrintInfo*)
{
}

#ifdef _DEBUG
void CCouplingAssembly1View::AssertValid() const
{
	CView::AssertValid();
}

void CCouplingAssembly1View::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CCouplingAssembly1Doc* CCouplingAssembly1View::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCouplingAssembly1Doc)));
	return (CCouplingAssembly1Doc*)m_pDocument;
}
#endif
