#pragma once

#include "ModelParams.h"

enum ESelectedNode
{
	NodeAssembly = 0,
	NodeHalfCoupling1 = 1,
	NodeHalfCoupling2 = 2,
	NodeSpider = 3
};

class CCouplingAssembly1Doc : public CDocument
{
protected:
	CCouplingAssembly1Doc() noexcept;
	DECLARE_DYNCREATE(CCouplingAssembly1Doc)

public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

	ESelectedNode GetSelectedNode() const;
	void SetSelectedNode(ESelectedNode node);

	const AssemblyParams& GetAssemblyParams() const;
	void SetAssemblyParams(const AssemblyParams& params);

	const HalfCouplingParams& GetHalfCoupling1Params() const;
	void SetHalfCoupling1Params(const HalfCouplingParams& params);

	const HalfCouplingParams& GetHalfCoupling2Params() const;
	void SetHalfCoupling2Params(const HalfCouplingParams& params);

	const SpiderParams& GetSpiderParams() const;
	void SetSpiderParams(const SpiderParams& params);

	void ReapplyGostTables();

public:
	virtual ~CCouplingAssembly1Doc();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	ESelectedNode m_selectedNode;
	AssemblyParams m_assemblyParams;
	HalfCouplingParams m_halfCoupling1Params;
	HalfCouplingParams m_halfCoupling2Params;
	SpiderParams m_spiderParams;

	DECLARE_MESSAGE_MAP()
};
