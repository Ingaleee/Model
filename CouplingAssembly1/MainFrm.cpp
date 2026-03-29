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
	ON_COMMAND(ID_HELP_WORKFLOW, &CMainFrame::OnHelpWorkflow)
	ON_UPDATE_COMMAND_UI(ID_PARAMS_CURRENT, &CMainFrame::OnUpdateParamsCurrent)
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

	if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CAsmTreeView), CSize(288, 100), pContext))
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
		m_menuAssignment.CreatePopupMenu();
		m_menuAssignment.AppendMenu(
			MF_STRING,
			ID_PARAMS_ASSEMBLY,
			L"1) Сборка — момент, исполнение ГОСТ, валы, вариант…");
		m_menuAssignment.AppendMenu(
			MF_STRING,
			ID_PARAMS_HALF1,
			L"2) Полумуфта 1 (под вал 1)…");
		m_menuAssignment.AppendMenu(
			MF_STRING,
			ID_PARAMS_HALF2,
			L"3) Полумуфта 2 (под вал 2)…");
		m_menuAssignment.AppendMenu(MF_STRING, ID_PARAMS_SPIDER, L"4) Звёздочка…");
		m_menuAssignment.AppendMenu(MF_SEPARATOR, 0, _T(""));
		m_menuAssignment.AppendMenu(
			MF_STRING,
			ID_PARAMS_CURRENT,
			L"Параметры выбранного узла (как двойной клик по дереву)");
		m_menuAssignment.AppendMenu(MF_SEPARATOR, 0, _T(""));
		m_menuAssignment.AppendMenu(
			MF_STRING,
			ID_REAPPLY_GOST_ALL,
			L"Подставить ГОСТ во все узлы (сборка + обе полумуфты + звезда)");
		m_menuAssignment.AppendMenu(MF_SEPARATOR, 0, _T(""));
		m_menuAssignment.AppendMenu(
			MF_STRING,
			ID_BUILD_COUPLING,
			L"Готово: построить в КОМПАС-3D и показать сводку");

		pMainMenu->InsertMenu(1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)m_menuAssignment.GetSafeHmenu(), L"Задание");
		pMainMenu->AppendMenu(MF_STRING, ID_BUILD_COUPLING, L"КОМПАС-3D: построить муфту");

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

void CMainFrame::OnUpdateParamsCurrent(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocumentSafe() != nullptr);
}

void CMainFrame::OnHelpWorkflow()
{
	static const wchar_t kHelp[] =
		L"Краткий порядок для защиты / демонстрации\n\n"
		L"Параметризация по варианту задания (1…8): только варианты 2 и 5 — звезда 4 луча и исполнение "
		L"ГОСТ 1 (две губки); во всех остальных вариантах — 6 лучей и исполнение 2 (три губки). "
		L"Номер исполнения вводить не нужно: он выставляется автоматически.\n\n"
		L"1. Меню «Задание» → пункт 1) Сборка: момент, вариант задания (1…8), диаметры валов. "
		L"Нажмите «Подставить L, D₁… из табл. 21.3.1», проверьте серые поля — ОК.\n\n"
		L"2. Пункты 2) и 3): полумуфты. В каждом окне кнопка «Строка из таблицы ГОСТ под этот вал».\n\n"
		L"3. Пункт 4): звёздочка — «Из таблицы звезды ГОСТ» подбирает D, d, H, число лучей.\n\n"
		L"4. По желанию: «Подставить ГОСТ во все узлы» — обновит всё разом после смены момента или валов.\n\n"
		L"5. Слева в дереве клик — превью справа; двойной клик — те же параметры.\n\n"
		L"6. Меню «КОМПАС-3D: построить муфту» или конец списка «Задание» — создать детали в КОМПАСе "
		L"и показать текстовую сводку (или отчёт, если КОМПАС не запущен).\n\n"
		L"Сохраняйте документ Файл → Сохранить, чтобы не потерять параметры.";
	AfxMessageBox(kHelp, MB_OK | MB_ICONINFORMATION);
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
