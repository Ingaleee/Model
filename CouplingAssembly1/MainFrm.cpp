#include "pch.h"
#include "framework.h"
#include "CouplingAssembly1.h"
#include "MainFrm.h"
#include "AsmTreeView.h"
#include "AsmPreviewView.h"
#include "CouplingAssembly1Doc.h"
#include "AssemblyParamsDlg.h"
#include "HalfCouplingParamsDlg.h"
#include "SpiderParamsDlg.h"
#include "CouplingBuilder.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_COMMAND(ID_PARAMS_CURRENT, &CMainFrame::OnParamsCurrent)
	ON_COMMAND(ID_PARAMS_ASSEMBLY, &CMainFrame::OnParamsAssembly)
	ON_COMMAND(ID_PARAMS_HALF1, &CMainFrame::OnParamsHalf1)
	ON_COMMAND(ID_PARAMS_HALF2, &CMainFrame::OnParamsHalf2)
	ON_COMMAND(ID_PARAMS_SPIDER, &CMainFrame::OnParamsSpider)
	ON_COMMAND(ID_REAPPLY_GOST_ALL, &CMainFrame::OnReapplyGostAll)
	ON_UPDATE_COMMAND_UI(ID_REAPPLY_GOST_ALL, &CMainFrame::OnUpdateReapplyGostAll)
	ON_COMMAND(ID_BUILD_COUPLING, &CMainFrame::OnBuildCoupling)
	ON_UPDATE_COMMAND_UI(ID_BUILD_COUPLING, &CMainFrame::OnUpdateBuildCoupling)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

CMainFrame::CMainFrame() noexcept
{
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CAsmTreeView), CSize(250, 100), pContext))
		return FALSE;

	if (!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CAsmPreviewView), CSize(800, 100), pContext))
		return FALSE;

	return TRUE;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(
			this,
			TBSTYLE_FLAT,
			WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Не удалось создать панель инструментов\n");
		return -1;
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Не удалось создать строку состояния\n");
		return -1;
	}

	if (!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT)))
	{
		TRACE0("Не удалось установить индикаторы строки состояния\n");
		return -1;
	}

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	CMenu* pMainMenu = GetMenu();
	if (pMainMenu != nullptr)
	{
		m_menuParameters.CreatePopupMenu();
		m_menuParameters.AppendMenu(MF_STRING, ID_PARAMS_ASSEMBLY, L"Параметры сборки");
		m_menuParameters.AppendMenu(MF_STRING, ID_PARAMS_HALF1, L"Параметры полумуфты 1");
		m_menuParameters.AppendMenu(MF_STRING, ID_PARAMS_HALF2, L"Параметры полумуфты 2");
		m_menuParameters.AppendMenu(MF_STRING, ID_PARAMS_SPIDER, L"Параметры звездочки");
		m_menuParameters.AppendMenu(MF_SEPARATOR, 0, _T(""));
		m_menuParameters.AppendMenu(MF_STRING, ID_REAPPLY_GOST_ALL, L"Пересчитать всё по ГОСТ");

		pMainMenu->AppendMenu(MF_POPUP, (UINT_PTR)m_menuParameters.GetSafeHmenu(), L"Параметры");
		pMainMenu->AppendMenu(MF_STRING, ID_BUILD_COUPLING, L"Построить");

		DrawMenuBar();
	}

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWnd::PreCreateWindow(cs))
		return FALSE;

	return TRUE;
}

CCouplingAssembly1Doc* CMainFrame::GetDocumentSafe()
{
	return DYNAMIC_DOWNCAST(CCouplingAssembly1Doc, GetActiveDocument());
}

void CMainFrame::EditCurrentNodeParams()
{
	CCouplingAssembly1Doc* pDoc = GetDocumentSafe();
	if (pDoc == nullptr)
		return;

	switch (pDoc->GetSelectedNode())
	{
	case NodeAssembly:
		EditAssemblyParams();
		break;
	case NodeHalfCoupling1:
		EditHalfCoupling1Params();
		break;
	case NodeHalfCoupling2:
		EditHalfCoupling2Params();
		break;
	case NodeSpider:
		EditSpiderParams();
		break;
	default:
		break;
	}
}

void CMainFrame::EditAssemblyParams()
{
	CCouplingAssembly1Doc* pDoc = GetDocumentSafe();
	if (pDoc == nullptr)
		return;

	CAssemblyParamsDlg dlg(pDoc->GetAssemblyParams(), this);
	if (dlg.DoModal() == IDOK)
	{
		pDoc->SetAssemblyParams(dlg.GetParams());
		pDoc->UpdateAllViews(nullptr);
	}
}

void CMainFrame::EditHalfCoupling1Params()
{
	CCouplingAssembly1Doc* pDoc = GetDocumentSafe();
	if (pDoc == nullptr)
		return;

	CHalfCouplingParamsDlg dlg(pDoc->GetAssemblyParams(), pDoc->GetHalfCoupling1Params(), 1, this);
	if (dlg.DoModal() == IDOK)
	{
		pDoc->SetHalfCoupling1Params(dlg.GetParams());
		pDoc->UpdateAllViews(nullptr);
	}
}

void CMainFrame::EditHalfCoupling2Params()
{
	CCouplingAssembly1Doc* pDoc = GetDocumentSafe();
	if (pDoc == nullptr)
		return;

	CHalfCouplingParamsDlg dlg(pDoc->GetAssemblyParams(), pDoc->GetHalfCoupling2Params(), 2, this);
	if (dlg.DoModal() == IDOK)
	{
		pDoc->SetHalfCoupling2Params(dlg.GetParams());
		pDoc->UpdateAllViews(nullptr);
	}
}

void CMainFrame::EditSpiderParams()
{
	CCouplingAssembly1Doc* pDoc = GetDocumentSafe();
	if (pDoc == nullptr)
		return;

	CSpiderParamsDlg dlg(pDoc->GetAssemblyParams(), pDoc->GetSpiderParams(), this);
	if (dlg.DoModal() == IDOK)
	{
		pDoc->SetSpiderParams(dlg.GetParams());
		pDoc->UpdateAllViews(nullptr);
	}
}

void CMainFrame::OnParamsCurrent()
{
	EditCurrentNodeParams();
}

void CMainFrame::OnParamsAssembly()
{
	EditAssemblyParams();
}

void CMainFrame::OnParamsHalf1()
{
	EditHalfCoupling1Params();
}

void CMainFrame::OnParamsHalf2()
{
	EditHalfCoupling2Params();
}

void CMainFrame::OnParamsSpider()
{
	EditSpiderParams();
}

void CMainFrame::OnReapplyGostAll()
{
	CCouplingAssembly1Doc* pDoc = GetDocumentSafe();
	if (pDoc == nullptr)
		return;
	pDoc->ReapplyGostTables();
}

void CMainFrame::OnUpdateReapplyGostAll(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocumentSafe() != nullptr);
}

void CMainFrame::OnBuildCoupling()
{
	CCouplingAssembly1Doc* pDoc = GetDocumentSafe();
	if (pDoc == nullptr)
	{
		AfxMessageBox(L"Документ не найден.");
		return;
	}

	CCouplingBuilder builder;
	builder.Build(*pDoc);
}

void CMainFrame::OnUpdateBuildCoupling(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocumentSafe() != nullptr);
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif
