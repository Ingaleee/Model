#include "pch.h"
#include "CouplingKompasBuild.h"
#include "CouplingAssembly1Doc.h"
#include "GostTables.h"
#include "KompasConnect.h"
#include "SpiderProfile2D.h"

#if COUPLING_USE_KOMPAS_SDK

#include "KompasConnectSdk.h"
#include <algorithm>
#include <cmath>
#include <comdef.h>
#include <ShlObj.h>
#include <vector>

using namespace KompasAPI;

namespace
{
constexpr double kPi = 3.14159265358979323846;

inline double NormDeg360(double a)
{
	double x = std::fmod(a, 360.0);
	if (x < 0.0)
		x += 360.0;
	return x;
}

inline bool DegOnCCWArc(double as, double ae, double t)
{
	as = NormDeg360(as);
	ae = NormDeg360(ae);
	t = NormDeg360(t);
	if (as <= ae + 1e-9)
		return t + 1e-6 >= as && t - 1e-6 <= ae;
	return t + 1e-6 >= as || t - 1e-6 <= ae;
}

inline bool KompasSourceFileExists(LPCWSTR path)
{
	if (path == nullptr || path[0] == 0)
		return false;
	const DWORD a = GetFileAttributesW(path);
	return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

inline void KsAxisLineXThroughOriginStyle3(ksDocument2DPtr p2DDoc)
{
	if (p2DDoc == nullptr)
		return;
	p2DDoc->ksLineSeg(-10.0, 0.0, 10.0, 0.0, 3);
}

double SpiderInnerValleyRadiusMm(double Ro, double Ri, double filletR)
{
	if (filletR > 0.02)
		return (std::max)(Ro * 0.22, Ri - (std::min)(filletR * 1.55, Ri * 0.28));
	return Ri;
}

void AppendComError(CString* err, const _com_error& e)
{
	if (err == nullptr)
		return;
	CString msg = e.ErrorMessage();
	if (msg.IsEmpty())
		msg = L"Ошибка COM при обращении к КОМПАС-3D";
	if (!err->IsEmpty())
		*err += L"\n";
	*err += msg;
}

static bool SpiderEdgeAtSpiderValleyBisectorDeg(double angDeg, int nRays)
{
	for (int k = 0; k < nRays; ++k)
	{
		const double vk =
			-90.0 + (static_cast<double>(k) + 0.5) * (360.0 / static_cast<double>(nRays));
		double d = angDeg - vk;
		while (d > 180.0)
			d -= 360.0;
		while (d < -180.0)
			d += 360.0;
		if (std::fabs(d) < 4.0)
			return true;
	}
	return false;
}

static double SpiderAbsDiffDeg(double aDeg, double bDeg)
{
	const double d = std::fmod(aDeg - bDeg + 540.0, 360.0) - 180.0;
	return std::fabs(d);
}

void AddSpiderInnerCylinderFlankFillets(
	ksPartPtr pPart,
	ksDocument3DPtr pDoc,
	ksEntityPtr pBoss,
	double riProfile,
	double thicknessH,
	double filletRmm,
	int nRays,
	const double* targetAnglesDeg,
	int nTargetAngles,
	CString* err)
{
	if (pPart == nullptr || pDoc == nullptr || pBoss == nullptr || nRays < 3)
		return;
	if (thicknessH < 0.25 || riProfile < 0.35 || filletRmm < 0.015)
		return;

	const double rApply = (std::min)(
		filletRmm,
		(std::max)(0.08, riProfile * 0.42));
	if (rApply < 0.04)
		return;

	try
	{
		pDoc->RebuildDocument();
	}
	catch (const _com_error&)
	{
	}

	const double tolR = (std::max)(0.5, riProfile * 0.05);
	const double zSpanMin = thicknessH * 0.82;
	const long want = static_cast<long>(nRays) * 2;
	const bool useAngles =
		targetAnglesDeg != nullptr && nTargetAngles == static_cast<int>(want) && want > 0;

	struct SpiderInnerEdgeCand
	{
		ksEntityPtr ed;
		double angDeg;
	};

	auto collectVerticalInnerBossEdges = [&](double xyTol, double zMinF, double rTolF, bool bisectorSkip) {
		std::vector<SpiderInnerEdgeCand> out;
		ksEntityCollectionPtr edges = pPart->EntityCollection(o3d_edge);
		for (long i = 0; i < edges->GetCount(); ++i)
		{
			ksEntityPtr ed = edges->GetByIndex(i);
			ksEdgeDefinitionPtr edef = ed->GetDefinition();
			if (edef == nullptr)
				continue;
			ksEntityPtr owner = edef->GetOwnerEntity();
			if (owner != nullptr && owner != pBoss)
				continue;
			if (edef->IsCircle() != VARIANT_FALSE)
				continue;

			ksVertexDefinitionPtr v1 = edef->GetVertex(true);
			ksVertexDefinitionPtr v2 = edef->GetVertex(false);
			if (v1 == nullptr || v2 == nullptr)
				continue;
			double x1, y1, z1, x2, y2, z2;
			v1->GetPoint(&x1, &y1, &z1);
			v2->GetPoint(&x2, &y2, &z2);
			const double dxy = std::hypot(x2 - x1, y2 - y1);
			if (dxy > xyTol)
				continue;
			const double dz = std::fabs(z2 - z1);
			if (dz < zSpanMin * zMinF)
				continue;
			const double r1 = std::hypot(x1, y1);
			const double r2 = std::hypot(x2, y2);
			if (std::fabs(r1 - riProfile) > tolR * rTolF ||
				std::fabs(r2 - riProfile) > tolR * rTolF)
				continue;
			const double angDeg = std::atan2(0.5 * (y1 + y2), 0.5 * (x1 + x2)) * (180.0 / kPi);
			if (bisectorSkip && SpiderEdgeAtSpiderValleyBisectorDeg(angDeg, nRays))
				continue;
			out.push_back({ed, angDeg});
		}
		return out;
	};

	auto pickByTargets = [&](const std::vector<SpiderInnerEdgeCand>& cands, double angTol1, double angTol2) {
		std::vector<ksEntityPtr> r;
		if (!useAngles || cands.empty() || want <= 0)
			return r;
		std::vector<char> used(cands.size(), 0);
		std::vector<ksEntityPtr> bySlot(static_cast<size_t>(want), nullptr);
		for (int pass = 0; pass < 2; ++pass)
		{
			const double angTol = (pass == 0) ? angTol1 : angTol2;
			for (long j = 0; j < want; ++j)
			{
				if (bySlot[static_cast<size_t>(j)] != nullptr)
					continue;
				double bestD = 1e300;
				size_t bestI = cands.size();
				for (size_t i = 0; i < cands.size(); ++i)
				{
					if (used[i])
						continue;
					const double d = SpiderAbsDiffDeg(cands[i].angDeg, targetAnglesDeg[j]);
					if (d < bestD)
					{
						bestD = d;
						bestI = i;
					}
				}
				if (bestI < cands.size() && bestD <= angTol)
				{
					used[bestI] = 1;
					bySlot[static_cast<size_t>(j)] = cands[bestI].ed;
				}
			}
		}
		for (long j = 0; j < want; ++j)
		{
			if (bySlot[static_cast<size_t>(j)] != nullptr)
				r.push_back(bySlot[static_cast<size_t>(j)]);
		}
		return r;
	};

	try
	{
		std::vector<ksEntityPtr> picked;

		if (useAngles)
		{
			picked = pickByTargets(collectVerticalInnerBossEdges(0.12, 0.72, 1.35, false), 11.0, 22.0);
			if (static_cast<long>(picked.size()) < want)
				picked = pickByTargets(collectVerticalInnerBossEdges(0.22, 0.65, 1.85, false), 14.0, 28.0);
			if (static_cast<long>(picked.size()) < want)
				picked.clear();
		}

		if (picked.empty() && useAngles)
		{
			picked = pickByTargets(collectVerticalInnerBossEdges(0.04, 1.0, 1.0, true), 18.0, 34.0);
			if (static_cast<long>(picked.size()) < want)
			{
				try
				{
					pDoc->RebuildDocument();
				}
				catch (const _com_error&)
				{
				}
				picked = pickByTargets(collectVerticalInnerBossEdges(0.2, 0.75, 1.35, true), 22.0, 42.0);
			}
		}

		if (static_cast<long>(picked.size()) < want && useAngles)
			picked = pickByTargets(collectVerticalInnerBossEdges(0.28, 0.62, 2.1, false), 26.0, 52.0);
		if (static_cast<long>(picked.size()) < want && useAngles)
			picked = pickByTargets(collectVerticalInnerBossEdges(0.45, 0.48, 3.2, false), 38.0, 88.0);

		if (static_cast<long>(picked.size()) < want && useAngles)
		{
			auto wide = collectVerticalInnerBossEdges(0.55, 0.45, 4.0, false);
			if (static_cast<long>(wide.size()) >= want)
			{
				std::sort(
					wide.begin(),
					wide.end(),
					[](const SpiderInnerEdgeCand& a, const SpiderInnerEdgeCand& b) {
						return a.angDeg < b.angDeg;
					});
				picked.clear();
				for (long j = 0; j < want; ++j)
				{
					const size_t idx =
						(static_cast<size_t>(j) * wide.size()) / static_cast<size_t>(want);
					picked.push_back(wide[idx].ed);
				}
			}
		}

		if (picked.empty())
		{
			if (err != nullptr)
				*err += L"\nКОМПАС: не удалось подобрать рёбра для скругления звезды.";
			return;
		}

		auto tryFilletAll = [&](double r) -> bool {
			ksEntityPtr pF = pPart->NewEntity(o3d_fillet);
			ksFilletDefinitionPtr fd = pF->GetDefinition();
			fd->radius = r;
			ksEntityCollectionPtr ar = fd->array();
			ar->Clear();
			for (const ksEntityPtr& ed : picked)
				ar->Add(ed);
			pF->Create();
			return true;
		};

		bool filletOk = false;
		double rTry = rApply;
		for (int attempt = 0; attempt < 8 && !filletOk; ++attempt)
		{
			try
			{
				filletOk = tryFilletAll(rTry);
			}
			catch (const _com_error&)
			{
				filletOk = false;
				rTry *= 0.5;
				if (rTry < 0.02)
					break;
			}
		}

		if (!filletOk)
		{
			const double rOne = (std::max)(0.03, (std::min)(rApply * 0.4, riProfile * 0.18));
			int okOne = 0;
			for (const ksEntityPtr& ed : picked)
			{
				try
				{
					ksEntityPtr pF = pPart->NewEntity(o3d_fillet);
					ksFilletDefinitionPtr fd = pF->GetDefinition();
					fd->radius = rOne;
					ksEntityCollectionPtr ar = fd->array();
					ar->Clear();
					ar->Add(ed);
					pF->Create();
					++okOne;
				}
				catch (const _com_error&)
				{
				}
			}
			if (err != nullptr)
			{
				if (okOne == 0)
					*err += L"\nКОМПАС: скругление по ГОСТ на звезде не выполнено (уменьшите радиус fillet или проверьте модель).";
				else if (okOne < static_cast<int>(picked.size()))
					*err += L"\nКОМПАС: скругление звезды выполнено не на всех рёбрах.";
			}
		}
	}
	catch (const _com_error& e)
	{
		AppendComError(err, e);
	}
}

ksPartPtr PartFromCollection(ksPartCollectionPtr coll, int idxZeroBased)
{
	if (coll == nullptr)
		return nullptr;
	ksPartPtr p;
	try
	{
		p = coll->GetByIndex(static_cast<long>(idxZeroBased));
	}
	catch (const _com_error&)
	{
		p = nullptr;
	}
	if (p == nullptr)
	{
		try
		{
			p = coll->GetByIndex(static_cast<long>(idxZeroBased + 1));
		}
		catch (const _com_error&)
		{
			p = nullptr;
		}
	}
	return p;
}

CStringW BuildOutputFolder()
{
	wchar_t temp[MAX_PATH]{};
	GetTempPathW(static_cast<DWORD>(std::size(temp)), temp);
	CStringW dir(temp);
	dir += L"Муфта_КОМПАС\\";
	::CreateDirectoryW(dir, nullptr);
	return dir;
}

bool AddAnnulusBoss(
	ksPartPtr pPart,
	ksEntityPtr planeForSketch,
	double outerR,
	double innerR,
	double height,
	CString* err)
{
	if (outerR <= innerR + 0.05 || height < 0.01)
	{
		if (err != nullptr)
			*err = L"КОМПАС: некорректные размеры кольца (D, d или длина).";
		return false;
	}

	ksEntityPtr pSketch = pPart->NewEntity(o3d_sketch);
	ksSketchDefinitionPtr pSketchDef = pSketch->GetDefinition();
	pSketchDef->SetPlane(planeForSketch);
	pSketch->Create();

	ksDocument2DPtr p2DDoc = pSketchDef->BeginEdit();
	p2DDoc->ksCircle(0.0, 0.0, outerR, 1);
	p2DDoc->ksCircle(0.0, 0.0, innerR, 1);
	KsAxisLineXThroughOriginStyle3(p2DDoc);
	pSketchDef->EndEdit();

	ksEntityPtr pBoss = pPart->NewEntity(o3d_bossExtrusion);
	ksBossExtrusionDefinitionPtr pBossDef = pBoss->GetDefinition();
	pBossDef->SetSketch(pSketch);
	pBossDef->directionType = dtNormal;
	pBossDef->SetSideParam(TRUE, etBlind, height, 0, FALSE);
	pBoss->Create();
	return true;
}

bool AddSolidCylinderBoss(
	ksPartPtr pPart,
	ksEntityPtr planeForSketch,
	double radiusR,
	double height,
	CString* err)
{
	if (radiusR < 0.05 || height < 0.01)
	{
		if (err != nullptr)
			*err = L"КОМПАС: некорректные размеры сплошного цилиндра (R, высота).";
		return false;
	}

	try
	{
		ksEntityPtr pSketch = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pSketchDef = pSketch->GetDefinition();
		pSketchDef->SetPlane(planeForSketch);
		pSketch->Create();

		ksDocument2DPtr p2DDoc = pSketchDef->BeginEdit();
		p2DDoc->ksCircle(0.0, 0.0, radiusR, 1);
		KsAxisLineXThroughOriginStyle3(p2DDoc);
		pSketchDef->EndEdit();

		ksEntityPtr pBoss = pPart->NewEntity(o3d_bossExtrusion);
		ksBossExtrusionDefinitionPtr pBossDef = pBoss->GetDefinition();
		pBossDef->SetSketch(pSketch);
		pBossDef->directionType = dtNormal;
		pBossDef->SetSideParam(TRUE, etBlind, height, 0, FALSE);
		pBoss->Create();
		return true;
	}
	catch (const _com_error& e)
	{
		AppendComError(err, e);
		return false;
	}
}

void AddThroughAllAxisBoreCut(ksPartPtr pPart, double holeR)
{
	if (holeR < 0.05)
		return;
	try
	{
		ksEntityPtr pSketch = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pSketchDef = pSketch->GetDefinition();
		pSketchDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
		pSketch->Create();

		ksDocument2DPtr p2DDoc = pSketchDef->BeginEdit();
		p2DDoc->ksCircle(0.0, 0.0, holeR, 1);
		KsAxisLineXThroughOriginStyle3(p2DDoc);
		pSketchDef->EndEdit();

		ksEntityPtr pCut = pPart->NewEntity(o3d_cutExtrusion);
		ksCutExtrusionDefinitionPtr pCutDef = pCut->GetDefinition();
		pCutDef->SetSketch(pSketch);
		pCutDef->directionType = dtReverse;
		pCutDef->SetSideParam(FALSE, etThroughAll, 0, 0, FALSE);
		pCut->Create();
	}
	catch (const _com_error&)
	{
	}
}

void TryMeridianRevolveCutTriangle(
	ksPartPtr pPart,
	double xa,
	double za,
	double xb,
	double zb,
	double xc,
	double zc)
{
	if (xa < 0.01 || xb < 0.01 || xc < 0.01)
		return;
	const double cross =
		(xb - xa) * (zc - za) - (zb - za) * (xc - xa);
	if (std::fabs(cross) < 1e-6)
		return;
	try
	{
		ksEntityPtr pSketch = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pSketchDef = pSketch->GetDefinition();
		pSketchDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOZ));
		pSketch->Create();

		double point_1[3][2]{};
		point_1[0][0] = xa;
		point_1[0][1] = za;
		point_1[1][0] = xb;
		point_1[1][1] = zb;
		point_1[2][0] = xc;
		point_1[2][1] = zc;

		ksDocument2DPtr p2DDoc = pSketchDef->BeginEdit();
		p2DDoc->ksLineSeg(point_1[0][0], point_1[0][1], point_1[1][0], point_1[1][1], 1);
		p2DDoc->ksLineSeg(point_1[1][0], point_1[1][1], point_1[2][0], point_1[2][1], 1);
		p2DDoc->ksLineSeg(point_1[2][0], point_1[2][1], point_1[0][0], point_1[0][1], 1);
		KsAxisLineXThroughOriginStyle3(p2DDoc);
		pSketchDef->EndEdit();

		ksEntityPtr pCut = pPart->NewEntity(o3d_cutRotated);
		ksCutRotatedDefinitionPtr pCutDef = pCut->GetDefinition();
		pCutDef->SetSketch(pSketch);
		pCutDef->SetSideParam(VARIANT_TRUE, 360.0);
		pCut->Create();
	}
	catch (const _com_error&)
	{
	}
}

void AddHalfCouplingShoulderChamferCut(
	ksPartPtr pPart,
	double R,
	double rHub,
	double L_jaw,
	double shoulderRadiusR)
{
	if (R <= rHub + 0.2)
		return;
	double cSh = (std::min)(3.2, (std::max)(0.5, shoulderRadiusR * 0.92));
	cSh = (std::min)(cSh, (R - rHub) * 0.55);
	cSh = (std::min)(cSh, L_jaw * 0.48);
	TryMeridianRevolveCutTriangle(pPart, R, L_jaw, R - cSh, L_jaw, R, L_jaw + cSh);
}

void AddHalfCouplingHubEndAndJawFaceChamferCuts(
	ksPartPtr pPart,
	double R,
	double rBore,
	double rHub,
	double L_jaw,
	double L1,
	int gostTableId)
{
	const double cEnd = (gostTableId >= 2) ? 1.6 : 1.0;
	double cJaw = (std::min)(cEnd, 1.0);
	cJaw = (std::min)(cJaw, (std::max)(0.15, L_jaw * 0.22));

	if (cEnd > 0.08 && rHub > rBore + cEnd + 0.15 && L1 > L_jaw + cEnd)
	{
		TryMeridianRevolveCutTriangle(pPart, rHub - cEnd, L1 - cEnd, rHub, L1 - cEnd, rHub, L1);
		TryMeridianRevolveCutTriangle(pPart, rBore, L1, rBore, L1 - cEnd, rBore + cEnd, L1 - cEnd);
	}

	if (cJaw > 0.05 && L_jaw > cJaw + 0.2)
		TryMeridianRevolveCutTriangle(pPart, R, 0.0, R - cJaw, 0.0, R, cJaw);
}

void DrawSpiderProfile(ksDocument2DPtr p2DDoc, int n, double Ro, double Ri, double filletR, double legWidthB)
{
	if (n == 4 || n == 6)
	{
		double xIL[8]{}, yIL[8]{}, xOL[8]{}, yOL[8]{}, xOR[8]{}, yOR[8]{}, xIR[8]{}, yIR[8]{};
		double omx[8]{}, omy[8]{};
		SpiderProfile2D::Fill46RayInnerOuterPoints(n, Ro, Ri, legWidthB, xIL, yIL, xOL, yOL, xOR, yOR, xIR, yIR, omx, omy);

		const double stepDeg = 360.0 / static_cast<double>(n);
		for (int k = 0; k < n; ++k)
		{
			const int kp1 = (k + 1) % n;
			const double midDeg = -90.0 + static_cast<double>(k) * stepDeg;
			const double halfB = legWidthB * 0.5;
			const double sa = (std::min)(0.999, halfB / Ro);
			const double deltaRad =
				(std::min)(std::asin(sa), kPi / static_cast<double>(n));
			const double deltaDeg = deltaRad * (180.0 / kPi);
			const double aOut1 = midDeg - deltaDeg;
			const double aOut2 = midDeg + deltaDeg;
			const double valleyDeg = -90.0 + (static_cast<double>(k) + 0.5) * stepDeg;
			const double aInnerIR = std::atan2(yIR[k], xIR[k]) * (180.0 / kPi);
			const double aInnerIL1 = std::atan2(yIL[kp1], xIL[kp1]) * (180.0 / kPi);
			const int innerDir = DegOnCCWArc(aInnerIR, aInnerIL1, valleyDeg) ? 1 : -1;

			p2DDoc->ksLineSeg(xIL[k], yIL[k], xOL[k], yOL[k], 1);
			p2DDoc->ksArcByAngle(0.0, 0.0, Ro, aOut1, aOut2, 1, 1);
			p2DDoc->ksLineSeg(xOR[k], yOR[k], xIR[k], yIR[k], 1);
			p2DDoc->ksArcByAngle(0.0, 0.0, Ri, aInnerIR, aInnerIL1, innerDir, 1);
		}
		(void)filletR;
		(void)omx;
		(void)omy;
		return;
	}

	const double toRad = kPi / 180.0;
	const double riDraw = SpiderInnerValleyRadiusMm(Ro, Ri, filletR);
	double capHalfDeg = (180.0 / kPi) * std::asin((std::min)(0.995, legWidthB / (2.0 * Ro)));
	capHalfDeg = (std::max)(6.0, (std::min)(capHalfDeg, 180.0 / n - 1.0));

	for (int k = 0; k < n; ++k)
	{
		const double midDeg = -90.0 + k * (360.0 / n);
		const double a1 = midDeg - capHalfDeg;
		const double a2 = midDeg + capHalfDeg;
		const double inDeg = -90.0 + (k + 0.5) * (360.0 / n);
		const double prevInDeg =
			(k == 0) ? (-90.0 + (n - 0.5) * (360.0 / n)) : (-90.0 + (k - 0.5) * (360.0 / n));

		const double xP = riDraw * std::cos(prevInDeg * toRad);
		const double yP = riDraw * std::sin(prevInDeg * toRad);
		const double xS = Ro * std::cos(a1 * toRad);
		const double yS = Ro * std::sin(a1 * toRad);
		const double xE = Ro * std::cos(a2 * toRad);
		const double yE = Ro * std::sin(a2 * toRad);
		const double xI = riDraw * std::cos(inDeg * toRad);
		const double yI = riDraw * std::sin(inDeg * toRad);

		double point_1[2][2]{};
		point_1[0][0] = xP;
		point_1[0][1] = yP;
		point_1[1][0] = xS;
		point_1[1][1] = yS;
		p2DDoc->ksLineSeg(point_1[0][0], point_1[0][1], point_1[1][0], point_1[1][1], 1);
		p2DDoc->ksArcByAngle(0.0, 0.0, Ro, a1, a2, 1, 1);
		point_1[0][0] = xE;
		point_1[0][1] = yE;
		point_1[1][0] = xI;
		point_1[1][1] = yI;
		p2DDoc->ksLineSeg(point_1[0][0], point_1[0][1], point_1[1][0], point_1[1][1], 1);
	}
}

static void AddSpiderCylindricalBoreCut(ksPartPtr pPart, double boreR, double bossHeightMm)
{
	if (pPart == nullptr || boreR < 0.2)
		return;
	const double depth = (bossHeightMm > 0.25) ? (bossHeightMm + 4.0) : 40.0;
	try
	{
		ksEntityPtr pSk = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pSkDef = pSk->GetDefinition();
		pSkDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
		pSk->Create();
		ksDocument2DPtr p2 = pSkDef->BeginEdit();
		p2->ksCircle(0.0, 0.0, boreR, 1);
		KsAxisLineXThroughOriginStyle3(p2);
		pSkDef->EndEdit();

		ksEntityPtr pCut = pPart->NewEntity(o3d_cutExtrusion);
		ksCutExtrusionDefinitionPtr pCutDef = pCut->GetDefinition();
		pCutDef->SetSketch(pSk);
		pCutDef->directionType = dtNormal;
		pCutDef->SetSideParam(TRUE, etBlind, depth, 0, FALSE);
		pCut->Create();
	}
	catch (const _com_error&)
	{
	}
}

bool SaveActiveDoc(ksDocument3DPtr pDoc, const wchar_t* path, CString* err)
{
	try
	{
		const VARIANT_BOOL ok = pDoc->SaveAs(path);
		if (ok == VARIANT_FALSE && err != nullptr)
			*err += L"\nНе удалось сохранить файл КОМПАСа.";
		return ok != VARIANT_FALSE;
	}
	catch (const _com_error& e)
	{
		AppendComError(err, e);
		return false;
	}
}

bool BuildSpiderPart(
	KompasObject* app,
	const SpiderParams& s,
	const wchar_t* savePath,
	CString* err)
{
	try
	{
		const int n = (std::max)(3, s.rays);
		const double Ro = s.outerDiameter * 0.5;
		double Ri = s.innerDiameter * 0.5;
		if (Ri >= Ro - 0.05)
			Ri = (std::max)(Ro * 0.35, Ro - s.legWidth * 1.5);
		const double H = (s.thickness > 0.01) ? s.thickness : 1.0;
		const double rf = (std::max)(0.0, (std::min)(s.filletRadius, Ri * 0.4));
		const double riProfile =
			(n == 4 || n == 6) ? Ri : SpiderInnerValleyRadiusMm(Ro, Ri, rf);

		ksDocument3DPtr pDoc = app->Document3D();
		pDoc->Create(VARIANT_FALSE, VARIANT_TRUE);
		app->Visible = VARIANT_TRUE;

		try
		{
			pDoc->Putcomment(_bstr_t(L"ГОСТ 14084-76 — звёздочка (эластичная)"));
		}
		catch (const _com_error&)
		{
		}

		ksPartPtr pPart = pDoc->GetPart(pTop_Part);

		ksEntityPtr pSketch = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pSketchDef = pSketch->GetDefinition();
		pSketchDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
		pSketch->Create();

		ksDocument2DPtr p2DDoc = pSketchDef->BeginEdit();
		DrawSpiderProfile(p2DDoc, n, Ro, Ri, rf, s.legWidth);
		pSketchDef->EndEdit();

		ksEntityPtr pBoss = pPart->NewEntity(o3d_bossExtrusion);
		ksBossExtrusionDefinitionPtr pBossDef = pBoss->GetDefinition();
		pBossDef->SetSketch(pSketch);
		pBossDef->directionType = dtNormal;
		pBossDef->SetSideParam(TRUE, etBlind, H, 0, FALSE);
		pBoss->Create();

		AddSpiderCylindricalBoreCut(pPart, Ri, H);

		try
		{
			pDoc->RebuildDocument();
		}
		catch (const _com_error&)
		{
		}

		if (n == 4 || n == 6)
		{
			double flankAngDeg[16]{};
			SpiderProfile2D::FlankFilletTargetAnglesDeg(n, Ro, Ri, s.legWidth, flankAngDeg, 16);
			AddSpiderInnerCylinderFlankFillets(
				pPart,
				pDoc,
				pBoss,
				riProfile,
				H,
				s.filletRadius,
				n,
				flankAngDeg,
				2 * n,
				err);
		}

		try
		{
			pDoc->RebuildDocument();
		}
		catch (const _com_error&)
		{
		}

		if (savePath != nullptr && savePath[0] != 0)
			SaveActiveDoc(pDoc, savePath, err);
		return true;
	}
	catch (const _com_error& e)
	{
		AppendComError(err, e);
		return false;
	}
}

void AddKeywayOnHub(
	ksPartPtr pPart,
	double hubR,
	double z0,
	double z1,
	double keyHalfWidth,
	double cutDepth,
	double bottomCornerRadius,
	double planeRotateDegAroundZ)
{
	if (keyHalfWidth < 0.1 || cutDepth < 0.1 || z1 <= z0 + 0.1)
		return;

	ksEntityPtr pPlBase = pPart->NewEntity(o3d_planeOffset);
	ksPlaneOffsetDefinitionPtr pPlBaseDef = pPlBase->GetDefinition();
	pPlBaseDef->SetPlane(pPart->GetDefaultEntity(o3d_planeYOZ));
	pPlBaseDef->direction = VARIANT_TRUE;
	pPlBaseDef->offset = hubR;
	pPlBase->Create();

	ksEntityPtr pPlKey = pPlBase;
	if (std::fabs(planeRotateDegAroundZ) > 0.05)
	{
		try
		{
			ksEntityPtr pPlAng = pPart->NewEntity(o3d_planeAngle);
			ksPlaneAngleDefinitionPtr pPlAngDef = pPlAng->GetDefinition();
			pPlAngDef->SetPlane(pPlBase);
			pPlAngDef->SetAxis(pPart->GetDefaultEntity(o3d_axisOZ));
			pPlAngDef->angle = planeRotateDegAroundZ;
			pPlAng->Create();
			pPlKey = pPlAng;
		}
		catch (const _com_error&)
		{
			pPlKey = pPlBase;
		}
	}

	ksEntityPtr pSkKey = pPart->NewEntity(o3d_sketch);
	ksSketchDefinitionPtr pSkKeyDef = pSkKey->GetDefinition();
	pSkKeyDef->SetPlane(pPlKey);
	pSkKey->Create();

	ksDocument2DPtr p2DDoc = pSkKeyDef->BeginEdit();
	double rkCap = (std::min)(keyHalfWidth * 0.42, cutDepth * 0.38);
	rkCap = (std::min)(rkCap, (z1 - z0) * 0.2);
	const double rk = (std::min)((std::max)(0.0, bottomCornerRadius), rkCap);
	if (rk < 0.12)
	{
		double point_1[4][2]{};
		point_1[0][0] = -keyHalfWidth;
		point_1[0][1] = z0;
		point_1[1][0] = keyHalfWidth;
		point_1[1][1] = z0;
		point_1[2][0] = keyHalfWidth;
		point_1[2][1] = z1;
		point_1[3][0] = -keyHalfWidth;
		point_1[3][1] = z1;
		p2DDoc->ksLineSeg(point_1[0][0], point_1[0][1], point_1[1][0], point_1[1][1], 1);
		p2DDoc->ksLineSeg(point_1[1][0], point_1[1][1], point_1[2][0], point_1[2][1], 1);
		p2DDoc->ksLineSeg(point_1[2][0], point_1[2][1], point_1[3][0], point_1[3][1], 1);
		p2DDoc->ksLineSeg(point_1[3][0], point_1[3][1], point_1[0][0], point_1[0][1], 1);
		KsAxisLineXThroughOriginStyle3(p2DDoc);
	}
	else
	{
		p2DDoc->ksLineSeg(-keyHalfWidth, z1, -keyHalfWidth, z0 + rk, 1);
		p2DDoc->ksArcByAngle(-keyHalfWidth + rk, z0 + rk, rk, 180.0, 270.0, -1, 1);
		p2DDoc->ksLineSeg(-keyHalfWidth + rk, z0, keyHalfWidth - rk, z0, 1);
		p2DDoc->ksArcByAngle(keyHalfWidth - rk, z0 + rk, rk, 270.0, 360.0, -1, 1);
		p2DDoc->ksLineSeg(keyHalfWidth, z0 + rk, keyHalfWidth, z1, 1);
		p2DDoc->ksLineSeg(keyHalfWidth, z1, -keyHalfWidth, z1, 1);
		KsAxisLineXThroughOriginStyle3(p2DDoc);
	}
	pSkKeyDef->EndEdit();

	ksEntityPtr pKeyCut = pPart->NewEntity(o3d_cutExtrusion);
	ksCutExtrusionDefinitionPtr pKeyCutDef = pKeyCut->GetDefinition();
	pKeyCutDef->SetSketch(pSkKey);
	pKeyCutDef->directionType = dtReverse;
	pKeyCutDef->SetSideParam(FALSE, etBlind, cutDepth + 1.0, 0, FALSE);
	pKeyCut->Create();
}

static bool DrawHalfCouplingTwoLugAnnulusSector(
	ksDocument2DPtr p2,
	double R,
	double rInner,
	double Bmm)
{
	if (p2 == nullptr || R <= rInner + 0.2 || Bmm < 0.15)
		return false;
	double B = Bmm;
	B = (std::min)(B, rInner - 0.18);
	B = (std::min)(B, R - 0.18);
	B = (std::max)(B, 0.35);
	if (B >= rInner - 0.12 || B >= R - 0.12)
		return false;
	const double ri2 = rInner * rInner - B * B;
	const double ro2 = R * R - B * B;
	if (ri2 <= 0.0 || ro2 <= 0.0)
		return false;
	const double sri = std::sqrt(ri2);
	const double sro = std::sqrt(ro2);
	const double ti = std::atan2(B, sri) * (180.0 / kPi);
	const double tj = std::atan2(sri, B) * (180.0 / kPi);
	const double ul = std::atan2(B, sro) * (180.0 / kPi);
	const double uh = std::atan2(sro, B) * (180.0 / kPi);
	p2->ksArcByAngle(0.0, 0.0, rInner, ti, tj, 1, 1);
	p2->ksLineSeg(B, sri, B, sro, 1);
	p2->ksArcByAngle(0.0, 0.0, R, ul, uh, 1, 1);
	p2->ksLineSeg(sro, B, sri, B, 1);
	return true;
}

void AddRadialLugs(
	ksPartPtr pPart,
	double D,
	double d,
	double L_jaw,
	int lugCount,
	double toothRadialDepth,
	double faceSlotB,
	double faceSlotB1,
	double lengthL3mm,
	double spiderLegWidth,
	double hubOuterR,
	double* outToothHeight,
	CString* err)
{
	if (outToothHeight != nullptr)
		*outToothHeight = 0.0;
	if (lugCount < 2 || L_jaw < 1.0)
		return;

	const double R = D * 0.5;
	const double r = d * 0.5;
	if (R <= r + 0.25)
		return;

	if (lugCount == 2)
	{
		const double rInnerFace =
			(std::max)(r + 0.45, (std::min)(hubOuterR, R - 0.55));
		double Bmm = (std::max)(faceSlotB, faceSlotB1 * 0.55);
		Bmm = (std::max)(0.8, (std::min)(Bmm, rInnerFace * 0.72));
		const double toothH = (lengthL3mm > 0.6)
			? (std::min)((std::max)(lengthL3mm, L_jaw * 0.86), L_jaw - 0.1)
			: (std::min)(
				  (std::max)(L_jaw * 0.95, (std::max)(3.8, spiderLegWidth * 1.04)),
				  L_jaw - 0.1);
		if (toothH < 0.75)
			return;
		try
		{
			ksEntityPtr pSkTooth = pPart->NewEntity(o3d_sketch);
			ksSketchDefinitionPtr pSkToothDef = pSkTooth->GetDefinition();
			pSkToothDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
			pSkTooth->Create();
			ksDocument2DPtr p2DDoc = pSkToothDef->BeginEdit();
			const bool drawn = DrawHalfCouplingTwoLugAnnulusSector(p2DDoc, R, rInnerFace, Bmm);
			pSkToothDef->EndEdit();
			if (!drawn)
				return;
			ksEntityPtr pToothBoss = pPart->NewEntity(o3d_bossExtrusion);
			ksBossExtrusionDefinitionPtr pToothDef = pToothBoss->GetDefinition();
			pToothDef->SetSketch(pSkTooth);
			pToothDef->directionType = dtNormal;
			pToothDef->SetSideParam(TRUE, etBlind, toothH, 0, FALSE);
			pToothBoss->Create();
			ksEntityPtr pAxis = pPart->GetDefaultEntity(o3d_axisOZ);
			ksEntityPtr pCirc = pPart->NewEntity(o3d_circularCopy);
			ksCircularCopyDefinitionPtr pCircDef = pCirc->GetDefinition();
			pCircDef->SetAxis(pAxis);
			pCircDef->count1 = 1;
			pCircDef->count2 = 2;
			pCircDef->step1 = 180.0;
			ksEntityCollectionPtr ops = pCircDef->GetOperationArray();
			ops->Clear();
			ops->Add(pToothBoss);
			pCirc->Create();
			if (outToothHeight != nullptr)
				*outToothHeight = toothH;
		}
		catch (const _com_error&)
		{
		}
		(void)err;
		(void)toothRadialDepth;
		return;
	}

	const double degStep = 360.0 / static_cast<double>(lugCount);
	const double chordB = (std::max)(faceSlotB, faceSlotB1 * 0.52);

	double depthUse = (std::max)(1.2, toothRadialDepth);
	depthUse = (std::min)(depthUse, (R - r) * 0.85);
	depthUse = (std::min)(depthUse, R * 0.22);

	const double rOut = (std::min)(R + depthUse, D * 0.52);
	double rIn = (std::max)(r + 0.55, R - depthUse);
	rIn = (std::min)(rIn, rOut - 0.35);
	rIn = (std::max)(rIn, r + 0.35);
	if (rOut <= rIn + 0.2 || rIn <= r + 0.15)
		return;

	const double maxDelta = (kPi / static_cast<double>(lugCount)) * 0.46;
	double deltaRad = std::asin((std::min)(0.999, chordB / (2.0 * rOut)));
	deltaRad = (std::min)(deltaRad, maxDelta);
	deltaRad = (std::max)(
		deltaRad,
		(std::min)(11.0 * kPi / 180.0, maxDelta * 0.58));
	deltaRad = (std::min)(deltaRad, maxDelta);

	const double toothH = (lengthL3mm > 0.6)
		? (std::min)((std::max)(lengthL3mm, L_jaw * 0.86), L_jaw - 0.1)
		: (std::min)((std::max)(L_jaw * 0.95, (std::max)(3.8, spiderLegWidth * 1.04)), L_jaw - 0.1);
	if (toothH < 0.75)
		return;

	try
	{
		ksEntityPtr pSkTooth = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pSkToothDef = pSkTooth->GetDefinition();
		pSkToothDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
		pSkTooth->Create();

		ksDocument2DPtr p2DDoc = pSkToothDef->BeginEdit();
		const double mid = 0.0;
		const double urx = std::cos(mid);
		const double ury = std::sin(mid);
		const double xOL = rOut * std::cos(-deltaRad);
		const double yOL = rOut * std::sin(-deltaRad);
		const double xOR = rOut * std::cos(deltaRad);
		const double yOR = rOut * std::sin(deltaRad);
		const double omx = rOut * std::cos(mid);
		const double omy = rOut * std::sin(mid);
		double xIL = xOL;
		double yIL = yOL;
		double xIR = xOR;
		double yIR = yOR;
		(void)SpiderProfile2D::ParallelFlankInnerHit(xOL, yOL, urx, ury, rIn, &xIL, &yIL);
		(void)SpiderProfile2D::ParallelFlankInnerHit(xOR, yOR, urx, ury, rIn, &xIR, &yIR);
		const double imx = rIn * std::cos(mid);
		const double imy = rIn * std::sin(mid);
		p2DDoc->ksLineSeg(xIL, yIL, xOL, yOL, 1);
		p2DDoc->ksArcBy3Points(xOL, yOL, omx, omy, xOR, yOR, 1);
		p2DDoc->ksLineSeg(xOR, yOR, xIR, yIR, 1);
		p2DDoc->ksArcBy3Points(xIR, yIR, imx, imy, xIL, yIL, 1);
		pSkToothDef->EndEdit();

		ksEntityPtr pToothBoss = pPart->NewEntity(o3d_bossExtrusion);
		ksBossExtrusionDefinitionPtr pToothDef = pToothBoss->GetDefinition();
		pToothDef->SetSketch(pSkTooth);
		pToothDef->directionType = dtNormal;
		pToothDef->SetSideParam(TRUE, etBlind, toothH, 0, FALSE);
		pToothBoss->Create();

		ksEntityPtr pAxis = pPart->GetDefaultEntity(o3d_axisOZ);
		ksEntityPtr pCirc = pPart->NewEntity(o3d_circularCopy);
		ksCircularCopyDefinitionPtr pCircDef = pCirc->GetDefinition();
		pCircDef->SetAxis(pAxis);
		pCircDef->count1 = 1;
		pCircDef->count2 = lugCount;
		pCircDef->step1 = degStep;
		ksEntityCollectionPtr ops = pCircDef->GetOperationArray();
		ops->Clear();
		ops->Add(pToothBoss);
		pCirc->Create();
		if (outToothHeight != nullptr)
			*outToothHeight = toothH;
	}
	catch (const _com_error&)
	{
	}

	(void)err;
}

void AddHalfCouplingJawFlankFillets(
	ksPartPtr pPart,
	ksDocument3DPtr pDoc,
	double toothH,
	double rBore,
	double R,
	double D,
	double d,
	int lugCount,
	double faceSlotB,
	double faceSlotB1,
	double toothRadialDepth,
	double hubOuterR,
	double filletRmm,
	CString* err)
{
	(void)err;
	if (pPart == nullptr || pDoc == nullptr || lugCount < 2 || toothH < 0.65)
		return;
	if (filletRmm < 0.06)
		return;

	const double r = d * 0.5;

	long want = 0;
	std::vector<double> target;
	double rLo = 0.0;
	double rHi = 0.0;
	bool targetsFromSector2 = false;
	if (lugCount == 2 && hubOuterR > rBore + 0.2)
	{
		const double rInnerFace = (std::max)(rBore + 0.45, (std::min)(hubOuterR, R - 0.55));
		double Bmm = (std::max)(faceSlotB, faceSlotB1 * 0.55);
		Bmm = (std::max)(0.8, (std::min)(Bmm, rInnerFace * 0.72));
		if (Bmm < rInnerFace - 0.12 && Bmm < R - 0.12)
		{
			const double ri2 = rInnerFace * rInnerFace - Bmm * Bmm;
			const double ro2 = R * R - Bmm * Bmm;
			if (ri2 > 0.0 && ro2 > 0.0)
			{
				const double sri = std::sqrt(ri2);
				const double sro = std::sqrt(ro2);
				const double ym = 0.5 * (sri + sro);
				const double xm = ym;
				const double angV = std::atan2(ym, Bmm) * (180.0 / kPi);
				const double angH = std::atan2(Bmm, xm) * (180.0 / kPi);
				target.push_back(angV);
				target.push_back(angH);
				target.push_back(angV + 180.0);
				target.push_back(angH + 180.0);
				want = 4;
				rLo = (std::max)(rBore + 0.05, rInnerFace - 1.6);
				rHi = R + 2.5;
				targetsFromSector2 = true;
			}
		}
	}

	if (!targetsFromSector2)
	{
		const double chordB = (std::max)(faceSlotB, faceSlotB1 * 0.52);
		double depthUse = (std::max)(1.2, toothRadialDepth);
		depthUse = (std::min)(depthUse, (R - r) * 0.85);
		depthUse = (std::min)(depthUse, R * 0.22);
		const double rOut = (std::min)(R + depthUse, D * 0.52);
		double rIn = (std::max)(r + 0.55, R - depthUse);
		rIn = (std::min)(rIn, rOut - 0.35);
		rIn = (std::max)(rIn, r + 0.35);
		if (rOut <= rIn + 0.12)
			return;

		const double maxDelta = (kPi / static_cast<double>(lugCount)) * 0.46;
		double deltaRad = std::asin((std::min)(0.999, chordB / (2.0 * rOut)));
		deltaRad = (std::min)(deltaRad, maxDelta);
		deltaRad = (std::max)(
			deltaRad,
			(std::min)(11.0 * kPi / 180.0, maxDelta * 0.58));
		deltaRad = (std::min)(deltaRad, maxDelta);
		const double deltaDeg = deltaRad * (180.0 / kPi);

		want = static_cast<long>(lugCount) * 2;
		if (want < 4 || want > 48)
			return;

		target.resize(static_cast<size_t>(want));
		for (int k = 0; k < lugCount; ++k)
		{
			const double c = static_cast<double>(k) * (360.0 / static_cast<double>(lugCount));
			target[static_cast<size_t>(k) * 2] = c - deltaDeg;
			target[static_cast<size_t>(k) * 2 + 1] = c + deltaDeg;
		}

		rLo = (std::max)(rBore + 0.1, rIn - 0.95);
		rHi = (std::min)(rOut + 1.8, R + (rOut - R) + 2.0);
	}

	if (want < 4 || static_cast<size_t>(want) != target.size())
		return;

	const double rApply = (std::min)(
		filletRmm,
		(std::max)(
			0.18,
			(std::min)(toothH * 0.26, (std::min)(4.5, (R - r) * 0.16))));
	if (rApply < 0.12)
		return;

	try
	{
		pDoc->RebuildDocument();
	}
	catch (const _com_error&)
	{
	}

	struct JawEdgeCand
	{
		ksEntityPtr ed;
		double angDeg;
	};

	auto collect = [&](double xyTol, double zSpanF, double zLo, double zHi, double rLo, double rHi) {
		std::vector<JawEdgeCand> out;
		ksEntityCollectionPtr edges = pPart->EntityCollection(o3d_edge);
		for (long i = 0; i < edges->GetCount(); ++i)
		{
			ksEntityPtr ed = edges->GetByIndex(i);
			ksEdgeDefinitionPtr edef = ed->GetDefinition();
			if (edef == nullptr)
				continue;
			if (edef->IsCircle() != VARIANT_FALSE)
				continue;
			ksVertexDefinitionPtr v1 = edef->GetVertex(true);
			ksVertexDefinitionPtr v2 = edef->GetVertex(false);
			if (v1 == nullptr || v2 == nullptr)
				continue;
			double x1, y1, z1, x2, y2, z2;
			v1->GetPoint(&x1, &y1, &z1);
			v2->GetPoint(&x2, &y2, &z2);
			const double dxy = std::hypot(x2 - x1, y2 - y1);
			if (dxy > xyTol)
				continue;
			const double dz = std::fabs(z2 - z1);
			if (dz < toothH * zSpanF)
				continue;
			const double zm = 0.5 * (z1 + z2);
			if (zm < zLo || zm > zHi)
				continue;
			const double xm = 0.5 * (x1 + x2);
			const double ym = 0.5 * (y1 + y2);
			const double rad = std::hypot(xm, ym);
			if (rad < rLo || rad > rHi)
				continue;
			const double angDeg = std::atan2(ym, xm) * (180.0 / kPi);
			out.push_back({ed, angDeg});
		}
		return out;
	};

	const double zLo = toothH * 0.08;
	const double zHi = toothH * 0.94;

	auto pickFrom = [&](const std::vector<JawEdgeCand>& cands, double a1, double a2) {
		std::vector<ksEntityPtr> picked;
		const size_t w = static_cast<size_t>(want);
		if (cands.empty() || target.size() < w)
			return picked;
		std::vector<char> used(cands.size(), 0);
		std::vector<ksEntityPtr> bySlot(w, nullptr);
		for (int pass = 0; pass < 2; ++pass)
		{
			const double angTol = (pass == 0) ? a1 : a2;
			for (long j = 0; j < want; ++j)
			{
				if (bySlot[static_cast<size_t>(j)] != nullptr)
					continue;
				double bestD = 1e300;
				size_t bestI = cands.size();
				for (size_t i = 0; i < cands.size(); ++i)
				{
					if (used[i])
						continue;
					const double dAng = SpiderAbsDiffDeg(cands[i].angDeg, target[static_cast<size_t>(j)]);
					if (dAng < bestD)
					{
						bestD = dAng;
						bestI = i;
					}
				}
				if (bestI < cands.size() && bestD <= angTol)
				{
					used[bestI] = 1;
					bySlot[static_cast<size_t>(j)] = cands[bestI].ed;
				}
			}
		}
		for (long j = 0; j < want; ++j)
		{
			if (bySlot[static_cast<size_t>(j)] != nullptr)
				picked.push_back(bySlot[static_cast<size_t>(j)]);
		}
		return picked;
	};

	try
	{
		std::vector<ksEntityPtr> picked =
			pickFrom(collect(0.18, 0.56, zLo, zHi, rLo, rHi), 15.0, 30.0);
		if (static_cast<long>(picked.size()) < want)
			picked = pickFrom(
				collect(0.32, 0.45, zLo * 0.85, (std::min)(zHi * 1.05, toothH - 0.02), rLo - 0.55, rHi + 0.9),
				24.0,
				48.0);

		if (static_cast<long>(picked.size()) < want)
			return;

		auto tryFillet = [&](double rad) {
			ksEntityPtr pF = pPart->NewEntity(o3d_fillet);
			ksFilletDefinitionPtr fd = pF->GetDefinition();
			fd->radius = rad;
			ksEntityCollectionPtr ar = fd->array();
			ar->Clear();
			for (const ksEntityPtr& ed : picked)
				ar->Add(ed);
			pF->Create();
		};

		double rTry = rApply;
		for (int t = 0; t < 8; ++t)
		{
			try
			{
				tryFillet(rTry);
				break;
			}
			catch (const _com_error&)
			{
				rTry *= 0.52;
				if (rTry < 0.09)
					break;
			}
		}
	}
	catch (const _com_error&)
	{
	}
}

void AddFaceSlotsBetweenLugs(
	ksPartPtr pPart,
	double D,
	double d,
	double faceSlotB,
	double faceSlotB1,
	double cutDepth,
	int lugCount,
	int gostTableId)
{
	if (lugCount < 2 || cutDepth < 0.12)
		return;

	const double R = D * 0.5;
	const double r = d * 0.5;
	if (R <= r + 0.4)
		return;

	const double bEff = (std::max)(faceSlotB, (std::max)(0.0, faceSlotB1) * 0.72);
	double spanDeg;
	if (gostTableId >= 2)
		spanDeg = (std::max)(13.0, (std::min)(34.0, bEff * 4.6));
	else
		spanDeg = (std::max)(8.5, (std::min)(17.0, (std::max)(faceSlotB, 2.5) * 3.4));

	const double angularPitch = 2.0 * kPi / static_cast<double>(lugCount);
	const double maxSpan = angularPitch * 0.42;
	const double span = (std::min)(spanDeg * (kPi / 180.0), maxSpan);

	double r1 = (std::min)(R + 1.65, D * 0.515);
	double r0 = (std::max)(r + 0.75, R - bEff * 2.05);
	r0 = (std::min)(r0, r1 - 1.15);
	if (r0 <= r + 0.25 || r0 >= r1 - 0.25)
		return;

	const double cutUse = (std::min)(cutDepth, R * 0.38);
	if (cutUse < 0.15)
		return;

	for (int k = 0; k < lugCount; ++k)
	{
		const double center = (2.0 * static_cast<double>(k) + 1.0) * (kPi / static_cast<double>(lugCount));
		const double a0 = center - span * 0.5;
		const double a1 = center + span * 0.5;

		try
		{
			ksEntityPtr pSk = pPart->NewEntity(o3d_sketch);
			ksSketchDefinitionPtr pDef = pSk->GetDefinition();
			pDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
			pSk->Create();
			ksDocument2DPtr p2DDoc = pDef->BeginEdit();
			double point_1[4][2]{};
			point_1[0][0] = r0 * std::cos(a0);
			point_1[0][1] = r0 * std::sin(a0);
			point_1[1][0] = r1 * std::cos(a0);
			point_1[1][1] = r1 * std::sin(a0);
			point_1[2][0] = r1 * std::cos(a1);
			point_1[2][1] = r1 * std::sin(a1);
			point_1[3][0] = r0 * std::cos(a1);
			point_1[3][1] = r0 * std::sin(a1);
			p2DDoc->ksLineSeg(point_1[0][0], point_1[0][1], point_1[1][0], point_1[1][1], 1);
			p2DDoc->ksLineSeg(point_1[1][0], point_1[1][1], point_1[2][0], point_1[2][1], 1);
			p2DDoc->ksLineSeg(point_1[2][0], point_1[2][1], point_1[3][0], point_1[3][1], 1);
			p2DDoc->ksLineSeg(point_1[3][0], point_1[3][1], point_1[0][0], point_1[0][1], 1);
			KsAxisLineXThroughOriginStyle3(p2DDoc);
			pDef->EndEdit();

			ksEntityPtr pCut = pPart->NewEntity(o3d_cutExtrusion);
			ksCutExtrusionDefinitionPtr pCutDef = pCut->GetDefinition();
			pCutDef->SetSketch(pSk);
			pCutDef->directionType = dtNormal;
			pCutDef->SetSideParam(TRUE, etBlind, cutUse, 0, FALSE);
			pCut->Create();
		}
		catch (const _com_error&)
		{
		}
	}
}

void AddSetScrewHole(
	ksPartPtr pPart,
	double hubR,
	double zPos,
	double holeR)
{
	if (holeR < 0.2 || zPos < 0.5)
		return;
	ksEntityPtr pPl = pPart->NewEntity(o3d_planeOffset);
	ksPlaneOffsetDefinitionPtr pPlDef = pPl->GetDefinition();
	pPlDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
	pPlDef->direction = VARIANT_TRUE;
	pPlDef->offset = zPos;
	pPl->Create();

	ksEntityPtr pSk = pPart->NewEntity(o3d_sketch);
	ksSketchDefinitionPtr pDef = pSk->GetDefinition();
	pDef->SetPlane(pPl);
	pSk->Create();
	ksDocument2DPtr p2DDoc = pDef->BeginEdit();
	p2DDoc->ksCircle(hubR - 1.0, 0.0, holeR, 1);
	pDef->EndEdit();

	ksEntityPtr pCut = pPart->NewEntity(o3d_cutExtrusion);
	ksCutExtrusionDefinitionPtr pCutDef = pCut->GetDefinition();
	pCutDef->SetSketch(pSk);
	pCutDef->directionType = dtReverse;
	pCutDef->SetSideParam(FALSE, etThroughAll, 0, 0, FALSE);
	pCut->Create();
}

bool BuildHalfCouplingPart(
	KompasObject* app,
	const HalfCouplingParams& h,
	int lugCount,
	double spiderLegWidth,
	double gostSeriesTorqueNm,
	int halfNumber,
	const wchar_t* savePath,
	CString* err)
{
	try
	{
		const double D = h.outerDiameter;
		const double d = h.boreDiameter;
		double d1 = h.hubOuterD1;
		double L1 = h.lengthTotalL1;
		const double l_hub = h.lengthHubL;

		if (L1 < 1.0)
			L1 = 1.0;
		const double R = D * 0.5;
		const double r = d * 0.5;
		if (r >= R - 0.05)
		{
			if (err != nullptr)
				*err = L"КОМПАС: d слишком велик относительно D.";
			return false;
		}

		if (d1 >= D - 0.5)
			d1 = D - 1.0;
		if (d1 <= d + 0.5)
			d1 = d + 2.0;

		double L_jaw = L1 - l_hub;
		if (L_jaw < 2.0)
			L_jaw = (std::max)(2.0, L1 * 0.35);
		const double L_hub_seg = L1 - L_jaw;
		if (L_hub_seg < 2.0)
		{
			if (err != nullptr)
				*err = L"КОМПАС: некорректные длины l / l1 полумуфты по ГОСТ.";
			return false;
		}

		ksDocument3DPtr pDoc = app->Document3D();
		pDoc->Create(VARIANT_FALSE, VARIANT_TRUE);
		app->Visible = VARIANT_TRUE;

		try
		{
			CStringW cmt;
			cmt.Format(
				L"ГОСТ 14084-76 — полумуфта %d (d=%.0f, D=%.0f мм)",
				(halfNumber == 2) ? 2 : 1,
				d,
				D);
			pDoc->Putcomment(_bstr_t(static_cast<LPCWSTR>(cmt)));
		}
		catch (const _com_error&)
		{
		}

		ksPartPtr pPart = pDoc->GetPart(pTop_Part);
		ksEntityPtr pXoy = pPart->GetDefaultEntity(o3d_planeXOY);

		const double rHub = d1 * 0.5;
		if (!AddSolidCylinderBoss(pPart, pXoy, R, L_jaw, err))
			return false;

		ksEntityPtr pPlHub = pPart->NewEntity(o3d_planeOffset);
		ksPlaneOffsetDefinitionPtr pPlHubDef = pPlHub->GetDefinition();
		pPlHubDef->SetPlane(pXoy);
		pPlHubDef->direction = VARIANT_TRUE;
		pPlHubDef->offset = L_jaw;
		pPlHub->Create();

		if (!AddSolidCylinderBoss(pPart, pPlHub, rHub, L_hub_seg, err))
			return false;

		AddThroughAllAxisBoreCut(pPart, r);

		const double keyHalfW = h.keywayWidthB * 0.5;
		const double t1 = (std::max)(0.1, h.keywayDt1 - d);
		const double keyDepth = (std::min)(
			(std::max)(t1, 0.15),
			(std::max)(0.6, (d1 - d) * 0.62));
		const double hubZ0 = L_jaw;
		const double hubZ1 = L1;
		const double mHub = (std::max)(0.3, (std::min)(L_hub_seg * 0.12, 4.0));
		double zKey0 = hubZ0 + mHub;
		double zKey1 = hubZ1 - mHub;
		if (h.lengthL2 > 0.5)
			zKey0 = hubZ0 + (std::min)(h.lengthL2 * 0.35, L_hub_seg * 0.55);
		if (h.lengthL3 > 0.5)
			zKey1 = hubZ1 - (std::min)(h.lengthL3 * 0.35, L_hub_seg * 0.55);
		if (zKey1 <= zKey0 + 1.5)
		{
			zKey0 = hubZ0 + 0.35;
			zKey1 = hubZ1 - 0.35;
		}
		AddKeywayOnHub(
			pPart,
			rHub,
			zKey0,
			zKey1,
			keyHalfW,
			keyDepth + 0.5,
			0.0,
			0.0);

		(void)gostSeriesTorqueNm;
		(void)lugCount;
		(void)spiderLegWidth;

		try
		{
			pDoc->RebuildDocument();
		}
		catch (const _com_error&)
		{
		}

		if (savePath != nullptr && savePath[0] != 0)
			SaveActiveDoc(pDoc, savePath, err);
		return true;
	}
	catch (const _com_error& e)
	{
		AppendComError(err, e);
		return false;
	}
}

bool TryBuildAssemblyDocument(
	KompasObject* app,
	const CStringW& dir,
	const CCouplingAssembly1Doc& doc,
	CString* err)
{
	try
	{
		const CStringW pathSpider = dir + L"Звёздочка.m3d";
		const CStringW pathHalf1 = dir + L"Полумуфта1.m3d";
		const CStringW pathHalf2 = dir + L"Полумуфта2.m3d";
		const CStringW pathAsmOut = dir + L"Муфта_Сборка.a3d";

		ksDocument3DPtr pAsmDoc = app->Document3D();
		VARIANT_BOOL created = pAsmDoc->Create(VARIANT_FALSE, VARIANT_FALSE);
		if (created == VARIANT_FALSE)
			created = pAsmDoc->Create(VARIANT_FALSE, VARIANT_TRUE);
		if (created == VARIANT_FALSE)
		{
			if (err != nullptr)
				*err += L"\nНе удалось создать сборочный документ КОМПАС-3D.";
			return false;
		}

		app->Visible = VARIANT_TRUE;

		try
		{
			pAsmDoc->Putcomment(
				_bstr_t(L"Муфта зубчато-звёздочная, ГОСТ 14084-76 — сборка"));
		}
		catch (const _com_error&)
		{
		}

		if (!KompasSourceFileExists(pathSpider) || !KompasSourceFileExists(pathHalf1) ||
			!KompasSourceFileExists(pathHalf2))
		{
			if (err != nullptr)
				*err +=
					L"\nСборка: на диске нет одного из файлов Звёздочка.m3d / Полумуфта1.m3d / Полумуфта2.m3d "
					L"(папка временных файлов или путь с кириллицей — сохраните детали вручную и проверьте доступ).";
		}

		auto insertPartFromFile = [&](LPCWSTR path, VARIANT_BOOL keepExternalLink) {
			if (!KompasSourceFileExists(path))
				return;
			try
			{
				pAsmDoc->SetPartFromFile(path, nullptr, keepExternalLink);
			}
			catch (const _com_error&)
			{
			}
		};

		auto insertAllParts = [&](VARIANT_BOOL keepExternalLink) {
			insertPartFromFile(static_cast<LPCWSTR>(pathSpider), keepExternalLink);
			insertPartFromFile(static_cast<LPCWSTR>(pathHalf1), keepExternalLink);
			insertPartFromFile(static_cast<LPCWSTR>(pathHalf2), keepExternalLink);
		};

		auto rebuildAsm = [&]() {
			try
			{
				pAsmDoc->RebuildDocument();
			}
			catch (const _com_error&)
			{
			}
		};

		auto partCountBest = [&]() -> long {
			long n = 0;
			ksPartCollectionPtr c = pAsmDoc->PartCollection(VARIANT_TRUE);
			if (c != nullptr)
			{
				try
				{
					n = c->GetCount();
				}
				catch (const _com_error&)
				{
					n = 0;
				}
			}
			if (n >= 3)
				return n;
			c = pAsmDoc->PartCollection(VARIANT_FALSE);
			if (c != nullptr)
			{
				try
				{
					const long n2 = c->GetCount();
					return (std::max)(n, n2);
				}
				catch (const _com_error&)
				{
					return n;
				}
			}
			return n;
		};

		insertAllParts(VARIANT_TRUE);
		rebuildAsm();

		if (partCountBest() < 3)
		{
			pAsmDoc = app->Document3D();
			created = pAsmDoc->Create(VARIANT_FALSE, VARIANT_FALSE);
			if (created == VARIANT_FALSE)
				created = pAsmDoc->Create(VARIANT_FALSE, VARIANT_TRUE);
			if (created != VARIANT_FALSE)
			{
				try
				{
					pAsmDoc->Putcomment(
						_bstr_t(L"Муфта зубчато-звёздочная, ГОСТ 14084-76 — сборка"));
				}
				catch (const _com_error&)
				{
				}
				insertAllParts(VARIANT_FALSE);
				rebuildAsm();
			}
		}

		try
		{
			const SpiderParams& sp = doc.GetSpiderParams();
			const double Hsp = (std::max)(1.0, sp.thickness);
			const double gAsm = 1.0;
			const double zStarLow = -0.5 * Hsp;

			ksPartCollectionPtr collT = pAsmDoc->PartCollection(VARIANT_TRUE);
			ksPartCollectionPtr collF = pAsmDoc->PartCollection(VARIANT_FALSE);
			long nT = 0;
			long nF = 0;
			if (collT != nullptr)
			{
				try
				{
					nT = collT->GetCount();
				}
				catch (const _com_error&)
				{
					collT = nullptr;
				}
			}
			if (collF != nullptr)
			{
				try
				{
					nF = collF->GetCount();
				}
				catch (const _com_error&)
				{
					collF = nullptr;
				}
			}
			ksPartCollectionPtr coll = (nF > nT) ? collF : collT;
			const long collCount = (nF > nT) ? nF : nT;

			if (coll != nullptr && collCount < 3 && err != nullptr)
				*err += L"\nВ сборку вставлено меньше трёх деталей — имена и расстановка пропущены.";

			if (coll != nullptr && collCount >= 3)
			{
				for (int k = 0; k < 3; ++k)
				{
					ksPartPtr p = PartFromCollection(coll, k);
					if (p == nullptr)
						continue;
					try
					{
						if (k == 0)
						{
							p->Putname(_bstr_t(L"Звёздочка"));
							p->Putmarking(_bstr_t(L"ГОСТ 14084-76"));
						}
						else if (k == 1)
						{
							p->Putname(_bstr_t(L"Полумуфта 1"));
							p->Putmarking(_bstr_t(L"Вал 1"));
						}
						else
						{
							p->Putname(_bstr_t(L"Полумуфта 2"));
							p->Putmarking(_bstr_t(L"Вал 2"));
						}
					}
					catch (const _com_error&)
					{
					}
					try
					{
						ksPlacementPtr pl = p->GetPlacement();
						if (pl == nullptr)
							continue;
						if (k == 0)
						{
							pl->SetAxes(1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
							pl->SetOrigin(0.0, 0.0, zStarLow);
						}
						else if (k == 1)
						{
							pl->SetAxes(1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
							pl->SetOrigin(0.0, 0.0, zStarLow - gAsm);
						}
						else
						{
							pl->SetAxes(-1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
							pl->SetOrigin(0.0, 0.0, 0.5 * Hsp + gAsm);
						}
						p->SetPlacement(pl);
						try
						{
							p->UpdatePlacementEx(VARIANT_TRUE);
						}
						catch (const _com_error&)
						{
							p->UpdatePlacement();
						}
					}
					catch (const _com_error&)
					{
					}
				}
			}
		}
		catch (const _com_error&)
		{
		}

		try
		{
			pAsmDoc->RebuildDocument();
		}
		catch (const _com_error&)
		{
		}

		SaveActiveDoc(pAsmDoc, static_cast<LPCWSTR>(pathAsmOut), err);
		return true;
	}
	catch (const _com_error& e)
	{
		AppendComError(err, e);
		return false;
	}
}

}

bool CouplingBuildInKompas(const CCouplingAssembly1Doc& doc, CString* err)
{
	if (!TryConnectKompas3D(err))
		return false;

	KompasObject* app = Coupling_GetKompasApp();
	if (app == nullptr)
	{
		if (err != nullptr)
			*err = L"КОМПАС: внутренняя ошибка (приложение не инициализировано).";
		return false;
	}

	const CStringW dir = BuildOutputFolder();
	const CStringW pSpider = dir + L"Звёздочка.m3d";
	const CStringW pHalf1 = dir + L"Полумуфта1.m3d";
	const CStringW pHalf2 = dir + L"Полумуфта2.m3d";

	const HalfCouplingParams& h1 = doc.GetHalfCoupling1Params();
	const HalfCouplingParams& h2 = doc.GetHalfCoupling2Params();
	const int lug1 = (std::max)(2, h1.lugCount);
	const int lug2 = (std::max)(2, h2.lugCount);
	const double legW = doc.GetSpiderParams().legWidth;
	const double tSeries = GostTables::SnapTorqueToSeries(doc.GetAssemblyParams().torque);

	if (!BuildSpiderPart(app, doc.GetSpiderParams(), static_cast<LPCWSTR>(pSpider), err))
		return false;
	if (!BuildHalfCouplingPart(app, h1, lug1, legW, tSeries, 1, static_cast<LPCWSTR>(pHalf1), err))
		return false;
	if (!BuildHalfCouplingPart(app, h2, lug2, legW, tSeries, 2, static_cast<LPCWSTR>(pHalf2), err))
		return false;

	TryBuildAssemblyDocument(app, dir, doc, err);

	if (err != nullptr)
	{
		if (err->IsEmpty())
			err->Format(L"Папка: %s", static_cast<LPCWSTR>(dir));
	}

	return true;
}

#else

bool CouplingBuildInKompas(const CCouplingAssembly1Doc& doc, CString* err)
{
	(void)doc;
	if (err != nullptr)
	{
		*err =
			L"Сборка в КОМПАС-3D отключена: в проекте задано COUPLING_USE_KOMPAS_SDK=0. "
			L"Подключите SDK в настройках проекта, укажите путь к папке SDK и пересоберите решение.";
	}
	return false;
}

#endif