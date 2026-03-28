#pragma once

#include <afxdialogex.h>
#include "resource.h"
#include "ModelParams.h"

class CHalfCouplingParamsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHalfCouplingParamsDlg)

public:
	CHalfCouplingParamsDlg(
		const AssemblyParams& assemblyContext,
		const HalfCouplingParams& params,
		int couplingIndex,
		CWnd* pParent = nullptr);
	virtual ~CHalfCouplingParamsDlg();

	enum { IDD = IDD_HALFCOUPLING_PARAMS_DLG };

	HalfCouplingParams GetParams() const;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	void PushToFields(const HalfCouplingParams& p);
	void OnHalfFromGost();
	void OnOK() override;

private:
	AssemblyParams m_gostAssembly;
	int m_couplingIndex;
	HalfCouplingParams m_saved;

	double m_boreDiameter;
	double m_outerDiameter;
	double m_l1;
	double m_lhub;
	double m_d1;
	double m_keyB;
	double m_keyDt1;
	double m_l2;
	double m_l3;
	double m_faceB;
	double m_faceB1;
	double m_r;
	int m_lugs;
	int m_tableId;
};
