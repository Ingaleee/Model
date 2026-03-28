#pragma once

#include <afxdialogex.h>
#include "resource.h"
#include "ModelParams.h"

class CSpiderParamsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSpiderParamsDlg)

public:
	CSpiderParamsDlg(const AssemblyParams& assemblyContext, const SpiderParams& params, CWnd* pParent = nullptr);
	virtual ~CSpiderParamsDlg();

	enum { IDD = IDD_SPIDER_PARAMS_DLG };

	SpiderParams GetParams() const;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	void PushToFields(const SpiderParams& s);
	void OnSpiderFromGost();
	void OnOK() override;

private:
	AssemblyParams m_gostAssembly;
	SpiderParams m_saved;
	double m_outerDiameter;
	double m_innerDiameter;
	double m_thickness;
	double m_legWidth;
	int m_rays;
	double m_filletRadius;
	double m_massKg;
};
