#include "pch.h"
#include "framework.h"
#include "CouplingAssembly1.h"
#include "SpiderParamsDlg.h"
#include "GostTables.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CSpiderParamsDlg, CDialogEx)

CSpiderParamsDlg::CSpiderParamsDlg(const AssemblyParams& assemblyContext, const SpiderParams& params, CWnd* pParent)
	: CDialogEx(IDD_SPIDER_PARAMS_DLG, pParent)
	, m_gostAssembly(assemblyContext)
	, m_saved(params)
	, m_outerDiameter(params.outerDiameter)
	, m_innerDiameter(params.innerDiameter)
	, m_thickness(params.thickness)
	, m_legWidth(params.legWidth)
	, m_rays(params.rays)
	, m_filletRadius(params.filletRadius)
	, m_massKg(params.massKg)
{
}

CSpiderParamsDlg::~CSpiderParamsDlg()
{
}

void CSpiderParamsDlg::PushToFields(const SpiderParams& s)
{
	m_outerDiameter = s.outerDiameter;
	m_innerDiameter = s.innerDiameter;
	m_thickness = s.thickness;
	m_legWidth = s.legWidth;
	m_rays = s.rays;
	m_filletRadius = s.filletRadius;
	m_massKg = s.massKg;
}

void CSpiderParamsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SPIDER_OUTER, m_outerDiameter);
	DDV_MinMaxDouble(pDX, m_outerDiameter, 5.0, 600.0);
	DDX_Text(pDX, IDC_EDIT_SPIDER_INNER, m_innerDiameter);
	DDV_MinMaxDouble(pDX, m_innerDiameter, 1.0, 500.0);
	DDX_Text(pDX, IDC_EDIT_SPIDER_THICKNESS, m_thickness);
	DDV_MinMaxDouble(pDX, m_thickness, 1.0, 200.0);
	DDX_Text(pDX, IDC_EDIT_SPIDER_LEG, m_legWidth);
	DDV_MinMaxDouble(pDX, m_legWidth, 0.5, 100.0);
	DDX_Text(pDX, IDC_EDIT_SPIDER_RAYS, m_rays);
	DDV_MinMaxInt(pDX, m_rays, 2, 12);
	DDX_Text(pDX, IDC_EDIT_SPIDER_FILLET, m_filletRadius);
	DDV_MinMaxDouble(pDX, m_filletRadius, 0.0, 50.0);
	DDX_Text(pDX, IDC_EDIT_SPIDER_MASS, m_massKg);
	DDV_MinMaxDouble(pDX, m_massKg, 0.0, 500.0);
}

BEGIN_MESSAGE_MAP(CSpiderParamsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_SPIDER_FROM_GOST, &CSpiderParamsDlg::OnSpiderFromGost)
END_MESSAGE_MAP()

BOOL CSpiderParamsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowTextW(L"Шаг 4 из 4 — звёздочка (эластичная вставка)");
	m_rays = (GostTables::ClampExecution(m_gostAssembly.execution) == 1) ? 4 : 6;
	if (CEdit* e = (CEdit*)GetDlgItem(IDC_EDIT_SPIDER_RAYS))
		e->SetReadOnly(TRUE);
	UpdateData(FALSE);
	return TRUE;
}

void CSpiderParamsDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return;
	if (m_outerDiameter <= m_innerDiameter + 0.5)
	{
		AfxMessageBox(
			L"Наружный диаметр D должен быть больше внутреннего d.",
			MB_OK | MB_ICONWARNING);
		return;
	}
	CDialogEx::OnOK();
}

void CSpiderParamsDlg::OnSpiderFromGost()
{
	if (!UpdateData(TRUE))
		return;

	SpiderParams s = GetParams();
	GostTables::LookupSpiderFromGost(m_gostAssembly, s);
	PushToFields(s);
	UpdateData(FALSE);
}

SpiderParams CSpiderParamsDlg::GetParams() const
{
	SpiderParams p = m_saved;
	p.rays = (GostTables::ClampExecution(m_gostAssembly.execution) == 1) ? 4 : 6;
	p.outerDiameter = m_outerDiameter;
	p.innerDiameter = m_innerDiameter;
	p.thickness = m_thickness;
	p.legWidth = m_legWidth;
	p.filletRadius = m_filletRadius;
	p.massKg = m_massKg;
	return p;
}
