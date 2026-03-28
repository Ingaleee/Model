#pragma once
#include <afxext.h>

class CCouplingAssembly1Doc;

class CMainFrame : public CFrameWnd
{
protected:
	CMainFrame() noexcept;
	DECLARE_DYNCREATE(CMainFrame)

	CSplitterWnd m_wndSplitter;
	CMenu m_menuParameters;

	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

	CCouplingAssembly1Doc* GetDocumentSafe();
	void EditCurrentNodeParams();
	void EditAssemblyParams();
	void EditHalfCoupling1Params();
	void EditHalfCoupling2Params();
	void EditSpiderParams();

public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual ~CMainFrame();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CToolBar m_wndToolBar;
	CStatusBar m_wndStatusBar;

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void OnParamsCurrent();
	afx_msg void OnParamsAssembly();
	afx_msg void OnParamsHalf1();
	afx_msg void OnParamsHalf2();
	afx_msg void OnParamsSpider();
	afx_msg void OnReapplyGostAll();
	afx_msg void OnUpdateReapplyGostAll(CCmdUI* pCmdUI);
	afx_msg void OnBuildCoupling();
	afx_msg void OnUpdateBuildCoupling(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};
