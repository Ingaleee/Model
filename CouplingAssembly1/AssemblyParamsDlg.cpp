#include "pch.h"
#include "framework.h"
#include "CouplingAssembly1.h"
#include "AssemblyParamsDlg.h"
#include "GostTables.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CAssemblyParamsDlg, CDialogEx)

CAssemblyParamsDlg::CAssemblyParamsDlg(const AssemblyParams& params, CWnd* pParent)
	: CDialogEx(IDD_ASSEMBLY_PARAMS_DLG, pParent)
	, m_saved(params)
	, m_torque(params.torque)
	, m_shaft1(params.shaftDiameter1)
	, m_shaft2(params.shaftDiameter2)
	, m_derL(params.assemblyLengthL)
	, m_derD1(params.envelopeD1)
	, m_derN(params.maxSpeedRpm)
	, m_derM(params.massKg)
	, m_derB1(params.widthB1)
{
}

CAssemblyParamsDlg::~CAssemblyParamsDlg()
{
}

void CAssemblyParamsDlg::SyncTorqueFromCombo()
{
	const int i = m_comboTorqueSeries.GetCurSel();
	if (i >= 0)
		m_torque = GostTables::TorqueSeriesValue(i);
}

void CAssemblyParamsDlg::ApplyTorqueFromUi()
{
	if (!UpdateData(TRUE))
		return;
	SyncTorqueFromCombo();
	m_torque = GostTables::SnapTorqueToSeries(m_torque);
	RefreshDerivedFromGost();
	UpdateData(FALSE);
}

void CAssemblyParamsDlg::SetupTorqueCombo()
{
	m_comboTorqueSeries.ResetContent();
	const int n = GostTables::TorqueSeriesCount();
	for (int i = 0; i < n; ++i)
	{
		CString s;
		s.Format(L"%.1f", GostTables::TorqueSeriesValue(i));
		m_comboTorqueSeries.AddString(s);
	}
	const int idx = GostTables::TorqueSeriesIndexNearest(m_torque);
	m_comboTorqueSeries.SetCurSel(idx);
	m_torque = GostTables::TorqueSeriesValue(idx);
}

void CAssemblyParamsDlg::RefreshDerivedFromGost()
{
	AssemblyParams a = m_saved;
	a.torque = m_torque;
	a.courseVariant = m_saved.courseVariant;
	a.execution = m_saved.execution;
	a.shaftDiameter1 = m_shaft1;
	a.shaftDiameter2 = m_shaft2;
	GostTables::FillAssemblyTable2131(a);
	m_derL = a.assemblyLengthL;
	m_derD1 = a.envelopeD1;
	m_derN = a.maxSpeedRpm;
	m_derM = a.massKg;
	m_derB1 = a.widthB1;
}

void CAssemblyParamsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_ASSEMBLY_TORQUE, m_comboTorqueSeries);
	DDX_Text(pDX, IDC_EDIT_ASSEMBLY_SHAFT1, m_shaft1);
	DDV_MinMaxDouble(pDX, m_shaft1, 1.0, 500.0);
	DDX_Text(pDX, IDC_EDIT_ASSEMBLY_SHAFT2, m_shaft2);
	DDV_MinMaxDouble(pDX, m_shaft2, 1.0, 500.0);
	DDX_Text(pDX, IDC_EDIT_ASM_DER_L, m_derL);
	DDX_Text(pDX, IDC_EDIT_ASM_DER_D1, m_derD1);
	DDX_Text(pDX, IDC_EDIT_ASM_DER_NMAX, m_derN);
	DDX_Text(pDX, IDC_EDIT_ASM_DER_MASS, m_derM);
	DDX_Text(pDX, IDC_EDIT_ASM_DER_B1, m_derB1);
}

BEGIN_MESSAGE_MAP(CAssemblyParamsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_ASM_FROM_GOST, &CAssemblyParamsDlg::OnAsmFromGost)
	ON_CBN_SELCHANGE(IDC_COMBO_ASSEMBLY_TORQUE, &CAssemblyParamsDlg::OnCbnSelchangeAssemblyTorque)
	ON_CBN_CLOSEUP(IDC_COMBO_ASSEMBLY_TORQUE, &CAssemblyParamsDlg::OnCbnCloseupAssemblyTorque)
END_MESSAGE_MAP()

BOOL CAssemblyParamsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowTextW(L"Шаг 1 из 4 — сборка (табл. 21.3.1 ГОСТ 2131)");
	SetupTorqueCombo();
	SyncTorqueFromCombo();
	m_torque = GostTables::SnapTorqueToSeries(m_torque);
	RefreshDerivedFromGost();
	UpdateData(FALSE);
	return TRUE;
}

void CAssemblyParamsDlg::OnCbnSelchangeAssemblyTorque()
{
	ApplyTorqueFromUi();
}

void CAssemblyParamsDlg::OnCbnCloseupAssemblyTorque()
{
	ApplyTorqueFromUi();
}

void CAssemblyParamsDlg::OnAsmFromGost()
{
	ApplyTorqueFromUi();
}

AssemblyParams CAssemblyParamsDlg::GetParams() const
{
	double torqueUse = m_torque;
	if (m_comboTorqueSeries.GetSafeHwnd() != nullptr)
	{
		const int i = m_comboTorqueSeries.GetCurSel();
		if (i >= 0)
			torqueUse = GostTables::TorqueSeriesValue(i);
	}
	torqueUse = GostTables::SnapTorqueToSeries(torqueUse);
	AssemblyParams p = m_saved;
	p.torque = torqueUse;
	p.courseVariant = m_saved.courseVariant;
	p.execution = m_saved.execution;
	p.shaftDiameter1 = m_shaft1;
	p.shaftDiameter2 = m_shaft2;
	GostTables::FillAssemblyTable2131(p);
	return p;
}
