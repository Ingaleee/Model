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
	, m_execution(params.execution)
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

void CAssemblyParamsDlg::RefreshDerivedFromGost()
{
	AssemblyParams a = m_saved;
	a.torque = m_torque;
	a.execution = m_execution;
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
	DDX_Text(pDX, IDC_EDIT_ASSEMBLY_TORQUE, m_torque);
	DDV_MinMaxDouble(pDX, m_torque, 0.01, 1.0e5);
	DDX_Text(pDX, IDC_EDIT_ASSEMBLY_EXECUTION, m_execution);
	DDV_MinMaxInt(pDX, m_execution, 1, 2);
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
END_MESSAGE_MAP()

BOOL CAssemblyParamsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowTextW(L"\u041f\u0430\u0440\u0430\u043c\u0435\u0442\u0440\u044b \u0441\u0431\u043e\u0440\u043a\u0438");
	RefreshDerivedFromGost();
	UpdateData(FALSE);
	return TRUE;
}

void CAssemblyParamsDlg::OnAsmFromGost()
{
	if (!UpdateData(TRUE))
		return;
	RefreshDerivedFromGost();
	UpdateData(FALSE);
}

AssemblyParams CAssemblyParamsDlg::GetParams() const
{
	AssemblyParams p = m_saved;
	p.torque = m_torque;
	p.execution = m_execution;
	p.shaftDiameter1 = m_shaft1;
	p.shaftDiameter2 = m_shaft2;
	GostTables::FillAssemblyTable2131(p);
	return p;
}
