#include "pch.h"
#include "framework.h"
#include "CouplingAssembly1.h"
#include "HalfCouplingParamsDlg.h"
#include "GostTables.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CHalfCouplingParamsDlg, CDialogEx)

CHalfCouplingParamsDlg::CHalfCouplingParamsDlg(
	const AssemblyParams& assemblyContext,
	const HalfCouplingParams& params,
	int couplingIndex,
	CWnd* pParent)
	: CDialogEx(IDD_HALFCOUPLING_PARAMS_DLG, pParent)
	, m_gostAssembly(assemblyContext)
	, m_couplingIndex(couplingIndex == 2 ? 2 : 1)
	, m_saved(params)
	, m_boreDiameter(params.boreDiameter)
	, m_outerDiameter(params.outerDiameter)
	, m_l1(params.lengthTotalL1)
	, m_lhub(params.lengthHubL)
	, m_d1(params.hubOuterD1)
	, m_keyB(params.keywayWidthB)
	, m_keyDt1(params.keywayDt1)
	, m_l2(params.lengthL2)
	, m_l3(params.lengthL3)
	, m_faceB(params.faceSlotB)
	, m_faceB1(params.faceSlotB1)
	, m_r(params.filletR)
	, m_lugs(params.lugCount)
	, m_tableId(params.gostTableId)
{
}

CHalfCouplingParamsDlg::~CHalfCouplingParamsDlg()
{
}

void CHalfCouplingParamsDlg::PushToFields(const HalfCouplingParams& p)
{
	m_boreDiameter = p.boreDiameter;
	m_outerDiameter = p.outerDiameter;
	m_l1 = p.lengthTotalL1;
	m_lhub = p.lengthHubL;
	m_d1 = p.hubOuterD1;
	m_keyB = p.keywayWidthB;
	m_keyDt1 = p.keywayDt1;
	m_l2 = p.lengthL2;
	m_l3 = p.lengthL3;
	m_faceB = p.faceSlotB;
	m_faceB1 = p.faceSlotB1;
	m_r = p.filletR;
	m_lugs = p.lugCount;
	m_tableId = p.gostTableId;
}

void CHalfCouplingParamsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_HALF_BORE, m_boreDiameter);
	DDV_MinMaxDouble(pDX, m_boreDiameter, 1.0, 500.0);
	DDX_Text(pDX, IDC_EDIT_HALF_OUTER, m_outerDiameter);
	DDV_MinMaxDouble(pDX, m_outerDiameter, 2.0, 600.0);
	DDX_Text(pDX, IDC_EDIT_HALF_L1, m_l1);
	DDV_MinMaxDouble(pDX, m_l1, 2.0, 400.0);
	DDX_Text(pDX, IDC_EDIT_HALF_LHUB, m_lhub);
	DDV_MinMaxDouble(pDX, m_lhub, 1.0, 300.0);
	DDX_Text(pDX, IDC_EDIT_HALF_D1, m_d1);
	DDV_MinMaxDouble(pDX, m_d1, 2.0, 500.0);
	DDX_Text(pDX, IDC_EDIT_HALF_KEY_B, m_keyB);
	DDV_MinMaxDouble(pDX, m_keyB, 0.5, 50.0);
	DDX_Text(pDX, IDC_EDIT_HALF_DT1, m_keyDt1);
	DDV_MinMaxDouble(pDX, m_keyDt1, 1.0, 600.0);
	DDX_Text(pDX, IDC_EDIT_HALF_L2, m_l2);
	DDV_MinMaxDouble(pDX, m_l2, 0.0, 200.0);
	DDX_Text(pDX, IDC_EDIT_HALF_L3, m_l3);
	DDV_MinMaxDouble(pDX, m_l3, 0.0, 200.0);
	DDX_Text(pDX, IDC_EDIT_HALF_FACE_B, m_faceB);
	DDV_MinMaxDouble(pDX, m_faceB, 0.5, 80.0);
	DDX_Text(pDX, IDC_EDIT_HALF_FACE_B1, m_faceB1);
	DDV_MinMaxDouble(pDX, m_faceB1, 0.5, 120.0);
	DDX_Text(pDX, IDC_EDIT_HALF_R, m_r);
	DDV_MinMaxDouble(pDX, m_r, 0.0, 20.0);
	DDX_Text(pDX, IDC_EDIT_HALF_LUGS, m_lugs);
	DDV_MinMaxInt(pDX, m_lugs, 2, 6);
	DDX_Text(pDX, IDC_EDIT_HALF_TABLEID, m_tableId);
}

BEGIN_MESSAGE_MAP(CHalfCouplingParamsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_HALF_FROM_GOST, &CHalfCouplingParamsDlg::OnHalfFromGost)
END_MESSAGE_MAP()

BOOL CHalfCouplingParamsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowTextW(L"\u041f\u0430\u0440\u0430\u043c\u0435\u0442\u0440\u044b \u043f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u044b");
	return TRUE;
}

void CHalfCouplingParamsDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return;
	if (m_l1 < m_lhub + 1.0)
	{
		AfxMessageBox(
			L"L\u2081 \u0434\u043e\u043b\u0436\u043d\u0430 \u0431\u044b\u0442\u044c \u043d\u0435 \u043c\u0435\u043d\u044c\u0448\u0435 "
			L"\u0434\u043b\u0438\u043d\u044b \u0441\u0442\u0443\u043f\u0438\u0446\u044b l \u0445\u043e\u0442\u044f \u0431\u044b \u043d\u0430 1 \u043c\u043c.",
			MB_OK | MB_ICONWARNING);
		return;
	}
	if (m_outerDiameter <= m_boreDiameter + 0.5)
	{
		AfxMessageBox(
			L"D \u0434\u043e\u043b\u0436\u0435\u043d \u0431\u044b\u0442\u044c \u0431\u043e\u043b\u044c\u0448\u0435 d (\u0437\u0430\u0437\u043e\u0440 \u043f\u043e \u043a\u043e\u043d\u0441\u0442\u0440\u0443\u043a\u0446\u0438\u0438).",
			MB_OK | MB_ICONWARNING);
		return;
	}
	if (m_d1 <= m_boreDiameter + 0.5)
	{
		AfxMessageBox(
			L"d\u2081 \u0434\u043e\u043b\u0436\u0435\u043d \u0431\u044b\u0442\u044c \u0431\u043e\u043b\u044c\u0448\u0435 d.",
			MB_OK | MB_ICONWARNING);
		return;
	}
	CDialogEx::OnOK();
}

void CHalfCouplingParamsDlg::OnHalfFromGost()
{
	if (!UpdateData(TRUE))
		return;

	const double dShaft = (m_couplingIndex == 2) ? m_gostAssembly.shaftDiameter2 : m_gostAssembly.shaftDiameter1;
	HalfCouplingParams trial = GetParams();
	if (!GostTables::LookupHalfFromGost(m_gostAssembly, trial, dShaft))
	{
		AfxMessageBox(
			L"\u041d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d\u0430 \u0441\u0442\u0440\u043e\u043a\u0430 \u0413\u041e\u0421\u0422 \u0434\u043b\u044f \u0437\u0430\u0434\u0430\u043d\u043d\u044b\u0445 "
			L"M\u043a\u0440, \u0438\u0441\u043f\u043e\u043b\u043d\u0435\u043d\u0438\u044f \u0438 \u0434\u0438\u0430\u043c\u0435\u0442\u0440\u0430 \u0432\u0430\u043b\u0430.",
			MB_OK | MB_ICONWARNING);
		return;
	}

	PushToFields(trial);
	UpdateData(FALSE);
}

HalfCouplingParams CHalfCouplingParamsDlg::GetParams() const
{
	HalfCouplingParams p = m_saved;
	p.boreDiameter = m_boreDiameter;
	p.outerDiameter = m_outerDiameter;
	p.lengthTotalL1 = m_l1;
	p.lengthHubL = m_lhub;
	p.hubOuterD1 = m_d1;
	p.keywayWidthB = m_keyB;
	p.keywayDt1 = m_keyDt1;
	p.lengthL2 = m_l2;
	p.lengthL3 = m_l3;
	p.faceSlotB = m_faceB;
	p.faceSlotB1 = m_faceB1;
	p.filletR = m_r;
	p.lugCount = m_lugs;
	p.gostTableId = m_tableId;
	p.SyncLegacyAliases();
	return p;
}
