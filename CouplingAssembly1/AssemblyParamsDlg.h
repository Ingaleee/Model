#pragma once

#include <afxwin.h>
#include <afxdialogex.h>
#include "resource.h"
#include "ModelParams.h"

class CAssemblyParamsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAssemblyParamsDlg)

public:
	CAssemblyParamsDlg(const AssemblyParams& params, CWnd* pParent = nullptr);
	virtual ~CAssemblyParamsDlg();

	enum { IDD = IDD_ASSEMBLY_PARAMS_DLG };

	AssemblyParams GetParams() const;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	void RefreshDerivedFromGost();
	void SetupTorqueCombo();
	void SyncTorqueFromCombo();
	void OnAsmFromGost();
	afx_msg void OnCbnSelchangeAssemblyTorque();

private:
	AssemblyParams m_saved;
	double m_torque;
	double m_shaft1;
	double m_shaft2;
	double m_derL;
	double m_derD1;
	double m_derN;
	double m_derM;
	double m_derB1;
	CComboBox m_comboTorqueSeries;
};
