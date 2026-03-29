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
	const double dShaft =
		(m_couplingIndex == 2) ? m_gostAssembly.shaftDiameter2 : m_gostAssembly.shaftDiameter1;
	const int ex = (m_gostAssembly.execution == 1) ? 1 : 2;
	const wchar_t* starTxt =
		(ex == 1) ? L"исп.1: 2 губки, звезда 4 луча" : L"исп.2: 3 губки, звезда 6 лучей";
	CString cap;
	const int step = (m_couplingIndex == 1) ? 2 : 3;
	cap.Format(
		L"Шаг %d из 4 — полумуфта %d (вал %d), d вала %.1f мм. %s",
		step,
		m_couplingIndex,
		m_couplingIndex,
		dShaft,
		starTxt);
	SetWindowTextW(cap);
	m_lugs = (GostTables::ExecutionFromCourseVariant(m_gostAssembly.courseVariant) == 1) ? 2 : 3;
	if (CEdit* e = (CEdit*)GetDlgItem(IDC_EDIT_HALF_LUGS))
		e->SetReadOnly(TRUE);
	UpdateData(FALSE);
	return TRUE;
}

void CHalfCouplingParamsDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return;
	if (m_l1 < m_lhub + 1.0)
	{
		AfxMessageBox(
			L"L₁ должна быть не меньше длины ступицы l хотя бы на 1 мм.",
			MB_OK | MB_ICONWARNING);
		return;
	}
	if (m_outerDiameter <= m_boreDiameter + 0.5)
	{
		AfxMessageBox(
			L"D должен быть больше d (зазор по конструкции).",
			MB_OK | MB_ICONWARNING);
		return;
	}
	if (m_d1 <= m_boreDiameter + 0.5)
	{
		AfxMessageBox(L"d₁ должен быть больше d.", MB_OK | MB_ICONWARNING);
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
			L"Не найдена строка ГОСТ для заданных Мкр, исполнения и диаметра вала.",
			MB_OK | MB_ICONWARNING);
		return;
	}

	PushToFields(trial);
	m_lugs = (GostTables::ExecutionFromCourseVariant(m_gostAssembly.courseVariant) == 1) ? 2 : 3;
	UpdateData(FALSE);
}

HalfCouplingParams CHalfCouplingParamsDlg::GetParams() const
{
	HalfCouplingParams p = m_saved;
	const int ex = GostTables::ExecutionFromCourseVariant(m_gostAssembly.courseVariant);
	p.lugCount = (ex == 1) ? 2 : 3;
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
	p.gostTableId = m_tableId;
	p.SyncLegacyAliases();
	return p;
}
