#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "CouplingAssembly1.h"
#endif

#include "CouplingAssembly1Doc.h"
#include "AsmTreeView.h"
#include "AssemblyParamsDlg.h"
#include "HalfCouplingParamsDlg.h"
#include "SpiderParamsDlg.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CAsmTreeView, CTreeView)

BEGIN_MESSAGE_MAP(CAsmTreeView, CTreeView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CAsmTreeView::OnTvnSelchanged)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CAsmTreeView::OnNMDblclk)
	ON_NOTIFY_REFLECT(NM_RCLICK, &CAsmTreeView::OnNMRclick)
END_MESSAGE_MAP()

CAsmTreeView::CAsmTreeView() noexcept
{
}

CAsmTreeView::~CAsmTreeView()
{
}

BOOL CAsmTreeView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CTreeView::PreCreateWindow(cs);
}

void CAsmTreeView::OnDraw(CDC* pDC)
{
	CCouplingAssembly1Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;
}

void CAsmTreeView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();

	CTreeCtrl& tree = GetTreeCtrl();
	tree.DeleteAllItems();

	HTREEITEM hRoot = tree.InsertItem(
		L"\u0421\u0431\u043e\u0440\u043a\u0430");
	tree.SetItemData(hRoot, NodeAssembly);

	HTREEITEM hHalf1 = tree.InsertItem(
		L"\u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 1",
		hRoot);
	tree.SetItemData(hHalf1, NodeHalfCoupling1);

	HTREEITEM hHalf2 = tree.InsertItem(
		L"\u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 2",
		hRoot);
	tree.SetItemData(hHalf2, NodeHalfCoupling2);

	HTREEITEM hSpider = tree.InsertItem(
		L"\u0417\u0432\u0451\u0437\u0434\u043e\u0447\u043a\u0430",
		hRoot);
	tree.SetItemData(hSpider, NodeSpider);

	tree.Expand(hRoot, TVE_EXPAND);
	tree.SelectItem(hRoot);
}

void CAsmTreeView::OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItem = tree.GetSelectedItem();

	if (hItem != nullptr)
	{
		ESelectedNode node = (ESelectedNode)tree.GetItemData(hItem);

		CCouplingAssembly1Doc* pDoc = GetDocument();
		if (pDoc != nullptr)
		{
			pDoc->SetSelectedNode(node);
			pDoc->UpdateAllViews(this);
		}
	}

	*pResult = 0;
}

void CAsmTreeView::OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItem = tree.GetSelectedItem();

	if (hItem != nullptr)
	{
		ESelectedNode node = (ESelectedNode)tree.GetItemData(hItem);

		CCouplingAssembly1Doc* pDoc = GetDocument();
		if (pDoc != nullptr)
		{
			switch (node)
			{
			case NodeAssembly:
			{
				CAssemblyParamsDlg dlg(pDoc->GetAssemblyParams(), this);
				if (dlg.DoModal() == IDOK)
				{
					pDoc->SetAssemblyParams(dlg.GetParams());
					pDoc->UpdateAllViews(this);
				}
				break;
			}

			case NodeHalfCoupling1:
			{
				CHalfCouplingParamsDlg dlg(pDoc->GetAssemblyParams(), pDoc->GetHalfCoupling1Params(), 1, this);
				if (dlg.DoModal() == IDOK)
				{
					pDoc->SetHalfCoupling1Params(dlg.GetParams());
					pDoc->UpdateAllViews(this);
				}
				break;
			}

			case NodeHalfCoupling2:
			{
				CHalfCouplingParamsDlg dlg(pDoc->GetAssemblyParams(), pDoc->GetHalfCoupling2Params(), 2, this);
				if (dlg.DoModal() == IDOK)
				{
					pDoc->SetHalfCoupling2Params(dlg.GetParams());
					pDoc->UpdateAllViews(this);
				}
				break;
			}

			case NodeSpider:
			{
				CSpiderParamsDlg dlg(pDoc->GetAssemblyParams(), pDoc->GetSpiderParams(), this);
				if (dlg.DoModal() == IDOK)
				{
					pDoc->SetSpiderParams(dlg.GetParams());
					pDoc->UpdateAllViews(this);
				}
				break;
			}

			default:
				break;
			}
		}
	}

	*pResult = 0;
}

void CAsmTreeView::OnNMRclick(NMHDR* pNMHDR, LRESULT* pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);

	CPoint screenPoint;
	GetCursorPos(&screenPoint);

	CPoint clientPoint = screenPoint;
	GetTreeCtrl().ScreenToClient(&clientPoint);

	UINT flags = 0;
	HTREEITEM hItem = GetTreeCtrl().HitTest(clientPoint, &flags);

	if (hItem != nullptr)
	{
		GetTreeCtrl().SelectItem(hItem);
	}

	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(
		MF_STRING,
		ID_PARAMS_CURRENT,
		L"\u041f\u0430\u0440\u0430\u043c\u0435\u0442\u0440\u044b...");
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(
		MF_STRING,
		ID_BUILD_COUPLING,
		L"\u041f\u043e\u0441\u0442\u0440\u043e\u0438\u0442\u044c");

	menu.TrackPopupMenu(
		TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		screenPoint.x,
		screenPoint.y,
		AfxGetMainWnd());

	*pResult = 0;
}

#ifdef _DEBUG
void CAsmTreeView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CAsmTreeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CCouplingAssembly1Doc* CAsmTreeView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCouplingAssembly1Doc)));
	return (CCouplingAssembly1Doc*)m_pDocument;
}
#endif
