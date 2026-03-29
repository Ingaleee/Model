#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "CouplingAssembly1.h"
#endif

#include "CouplingAssembly1Doc.h"
#include "GostTables.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
constexpr DWORD kArchiveSig = 0x4D463143;
constexpr DWORD kArchiveVer = 2;
constexpr DWORD kArchiveVerMin = 1;

void ArchiveAssembly(CArchive& ar, AssemblyParams& a, DWORD fileVer)
{
	if (ar.IsStoring())
	{
		ar << a.torque;
		ar << a.execution;
		ar << a.shaftDiameter1 << a.shaftDiameter2;
		ar << a.assemblyLengthL << a.envelopeD1 << a.maxSpeedRpm << a.massKg << a.widthB1;
		ar << a.courseVariant;
	}
	else
	{
		ar >> a.torque;
		ar >> a.execution;
		ar >> a.shaftDiameter1 >> a.shaftDiameter2;
		ar >> a.assemblyLengthL >> a.envelopeD1 >> a.maxSpeedRpm >> a.massKg >> a.widthB1;
		if (fileVer >= 2)
			ar >> a.courseVariant;
		else
			a.courseVariant = 1;
	}
}

void ArchiveHalf(CArchive& ar, HalfCouplingParams& h)
{
	if (ar.IsStoring())
	{
		ar << h.gostTableId << h.lugCount;
		ar << h.boreDiameter << h.keywayWidthB << h.keywayDt1 << h.hubOuterD1 << h.outerDiameter;
		ar << h.lengthHubL << h.lengthTotalL1 << h.lengthL2 << h.lengthL3;
		ar << h.faceSlotB << h.faceSlotB1 << h.filletR << h.shoulderRadiusR;
	}
	else
	{
		ar >> h.gostTableId >> h.lugCount;
		ar >> h.boreDiameter >> h.keywayWidthB >> h.keywayDt1 >> h.hubOuterD1 >> h.outerDiameter;
		ar >> h.lengthHubL >> h.lengthTotalL1 >> h.lengthL2 >> h.lengthL3;
		ar >> h.faceSlotB >> h.faceSlotB1 >> h.filletR >> h.shoulderRadiusR;
		h.SyncLegacyAliases();
	}
}

void ArchiveSpider(CArchive& ar, SpiderParams& s)
{
	if (ar.IsStoring())
	{
		ar << s.outerDiameter << s.thickness << s.rays << s.innerDiameter << s.legWidth << s.filletRadius
			<< s.massKg;
	}
	else
	{
		ar >> s.outerDiameter >> s.thickness >> s.rays >> s.innerDiameter >> s.legWidth >> s.filletRadius >>
			s.massKg;
	}
}

}

IMPLEMENT_DYNCREATE(CCouplingAssembly1Doc, CDocument)

BEGIN_MESSAGE_MAP(CCouplingAssembly1Doc, CDocument)
END_MESSAGE_MAP()

CCouplingAssembly1Doc::CCouplingAssembly1Doc() noexcept
	: m_selectedNode(NodeAssembly)
{
}

CCouplingAssembly1Doc::~CCouplingAssembly1Doc()
{
}

BOOL CCouplingAssembly1Doc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_selectedNode = NodeAssembly;
	m_assemblyParams = AssemblyParams();
	m_halfCoupling1Params = HalfCouplingParams();
	m_halfCoupling2Params = HalfCouplingParams();
	m_spiderParams = SpiderParams();
	GostTables::ApplyAssemblyToParts(
		m_assemblyParams,
		m_halfCoupling1Params,
		m_halfCoupling2Params,
		m_spiderParams);

	return TRUE;
}

void CCouplingAssembly1Doc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << kArchiveSig;
		ar << kArchiveVer;
		ar << static_cast<int>(m_selectedNode);
		ArchiveAssembly(ar, m_assemblyParams, kArchiveVer);
		ArchiveHalf(ar, m_halfCoupling1Params);
		ArchiveHalf(ar, m_halfCoupling2Params);
		ArchiveSpider(ar, m_spiderParams);
	}
	else
	{
		DWORD sig = 0;
		ar >> sig;
		if (sig != kArchiveSig)
		{
			AfxMessageBox(
				L"Файл не является документом CouplingAssembly1.",
				MB_OK | MB_ICONWARNING);
			AfxThrowArchiveException(CArchiveException::badIndex);
		}
		DWORD ver = 0;
		ar >> ver;
		if (ver < kArchiveVerMin || ver > kArchiveVer)
		{
			AfxMessageBox(L"Версия файла не поддерживается.", MB_OK | MB_ICONWARNING);
			AfxThrowArchiveException(CArchiveException::badSchema);
		}
		int node = 0;
		ar >> node;
		if (node < NodeAssembly || node > NodeSpider)
			node = NodeAssembly;
		m_selectedNode = static_cast<ESelectedNode>(node);
		ArchiveAssembly(ar, m_assemblyParams, ver);
		ArchiveHalf(ar, m_halfCoupling1Params);
		ArchiveHalf(ar, m_halfCoupling2Params);
		ArchiveSpider(ar, m_spiderParams);
		GostTables::ApplyCourseVariantRule(
			m_assemblyParams,
			m_halfCoupling1Params,
			m_halfCoupling2Params,
			m_spiderParams);
	}
}

void CCouplingAssembly1Doc::ReapplyGostTables()
{
	GostTables::ApplyAssemblyToParts(
		m_assemblyParams,
		m_halfCoupling1Params,
		m_halfCoupling2Params,
		m_spiderParams);
	SetModifiedFlag();
	UpdateAllViews(nullptr);
}

ESelectedNode CCouplingAssembly1Doc::GetSelectedNode() const
{
	return m_selectedNode;
}

void CCouplingAssembly1Doc::SetSelectedNode(ESelectedNode node)
{
	m_selectedNode = node;
}

const AssemblyParams& CCouplingAssembly1Doc::GetAssemblyParams() const
{
	return m_assemblyParams;
}

void CCouplingAssembly1Doc::SetAssemblyParams(const AssemblyParams& params)
{
	m_assemblyParams = params;
	GostTables::ApplyAssemblyToParts(
		m_assemblyParams,
		m_halfCoupling1Params,
		m_halfCoupling2Params,
		m_spiderParams);
}

const HalfCouplingParams& CCouplingAssembly1Doc::GetHalfCoupling1Params() const
{
	return m_halfCoupling1Params;
}

void CCouplingAssembly1Doc::SetHalfCoupling1Params(const HalfCouplingParams& params)
{
	m_halfCoupling1Params = params;
	m_halfCoupling1Params.SyncLegacyAliases();
	GostTables::ApplyCourseVariantRule(
		m_assemblyParams,
		m_halfCoupling1Params,
		m_halfCoupling2Params,
		m_spiderParams);
}

const HalfCouplingParams& CCouplingAssembly1Doc::GetHalfCoupling2Params() const
{
	return m_halfCoupling2Params;
}

void CCouplingAssembly1Doc::SetHalfCoupling2Params(const HalfCouplingParams& params)
{
	m_halfCoupling2Params = params;
	m_halfCoupling2Params.SyncLegacyAliases();
	GostTables::ApplyCourseVariantRule(
		m_assemblyParams,
		m_halfCoupling1Params,
		m_halfCoupling2Params,
		m_spiderParams);
}

const SpiderParams& CCouplingAssembly1Doc::GetSpiderParams() const
{
	return m_spiderParams;
}

void CCouplingAssembly1Doc::SetSpiderParams(const SpiderParams& params)
{
	m_spiderParams = params;
	GostTables::ApplyCourseVariantRule(
		m_assemblyParams,
		m_halfCoupling1Params,
		m_halfCoupling2Params,
		m_spiderParams);
}

#ifdef _DEBUG
void CCouplingAssembly1Doc::AssertValid() const
{
	CDocument::AssertValid();
}

void CCouplingAssembly1Doc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif
