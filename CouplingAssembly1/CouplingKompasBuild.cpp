#include "pch.h"
#include "CouplingKompasBuild.h"
#include "CouplingAssembly1Doc.h"
#include "GostTables.h"
#include "KompasConnect.h"

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

void AddSpiderInnerCylinderFlankFillets(
	ksPartPtr pPart,
	ksDocument3DPtr pDoc,
	ksEntityPtr pBoss,
	double riProfile,
	double thicknessH,
	double filletRmm,
	int nRays,
	CString* err)
{
	if (pPart == nullptr || pDoc == nullptr || pBoss == nullptr || nRays < 3)
		return;
	if (thicknessH < 0.25 || riProfile < 0.35 || filletRmm < 0.06)
		return;

	const double rApply = (std::min)(
		filletRmm,
		(std::max)(0.08, riProfile * 0.42));
	if (rApply < 0.06)
		return;

	try
	{
		pDoc->RebuildDocument();
	}
	catch (const _com_error&)
	{
	}

	const double tolR = (std::max)(0.35, riProfile * 0.035);
	const double tolXY = 0.04;
	const double zSpanMin = thicknessH * 0.82;

	auto pickFlankInnerVerticalEdges = [&](
		double xyTol,
		double zMinF,
		double rTolF,
		bool useValleyFilter) -> std::vector<ksEntityPtr> {
		std::vector<ksEntityPtr> out;
		ksEntityCollectionPtr edges = pPart->EntityCollection(o3d_edge);
		for (long i = 0; i < edges->GetCount(); ++i)
		{
			ksEntityPtr ed = edges->GetByIndex(i);
			ksEdgeDefinitionPtr edef = ed->GetDefinition();
			if (edef == nullptr)
				continue;
			if (edef->GetOwnerEntity() != pBoss)
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
			if (useValleyFilter)
			{
				const double angDeg = std::atan2(y1, x1) * (180.0 / kPi);
				if (SpiderEdgeAtSpiderValleyBisectorDeg(angDeg, nRays))
					continue;
			}
			out.push_back(ed);
		}
		return out;
	};

	try
	{
		const long expectedVert = static_cast<long>(nRays) * 2;
		std::vector<ksEntityPtr> picked = pickFlankInnerVerticalEdges(tolXY, 1.0, 1.0, true);
		if (static_cast<long>(picked.size()) < expectedVert)
		{
			try
			{
				pDoc->RebuildDocument();
			}
			catch (const _com_error&)
			{
			}
			picked = pickFlankInnerVerticalEdges(tolXY * 5.0, 0.75, 1.35, true);
		}
		if (picked.empty())
		{
			if (err != nullptr)
				*err += L"\nКОМПАС: не удалось подобрать рёбра для скругления звезды.";
			return;
		}

		ksEntityPtr pF = pPart->NewEntity(o3d_fillet);
		ksFilletDefinitionPtr fd = pF->GetDefinition();
		fd->radius = rApply;
		ksEntityCollectionPtr ar = fd->array();
		ar->Clear();
		for (const ksEntityPtr& ed : picked)
			ar->Add(ed);
		pF->Create();
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
	double cSh = (std::min)(2.6, (std::max)(0.4, shoulderRadiusR * 0.72));
	cSh = (std::min)(cSh, (R - rHub) * 0.5);
	cSh = (std::min)(cSh, L_jaw * 0.42);
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

static bool SpiderTryOuterCornerFillet(
	double px,
	double py,
	double sx,
	double sy,
	double ex,
	double ey,
	double r,
	double* oT1x,
	double* oT1y,
	double* oT2x,
	double* oT2y,
	double* oMx,
	double* oMy)
{
	if (r < 0.025)
		return false;
	const double d1x = sx - px;
	const double d1y = sy - py;
	const double d2x = ex - sx;
	const double d2y = ey - sy;
	const double l1 = std::hypot(d1x, d1y);
	const double l2 = std::hypot(d2x, d2y);
	if (l1 < 1e-4 || l2 < 1e-4)
		return false;
	const double u1x = d1x / l1;
	const double u1y = d1y / l1;
	const double u2x = d2x / l2;
	const double u2y = d2y / l2;
	const double cphi = u1x * u2x + u1y * u2y;
	const double phi = std::acos((std::max)(-1.0, (std::min)(1.0, cphi)));
	if (phi < 1e-3 || phi > kPi - 1e-3)
		return false;
	double t = r / std::tan(phi * 0.5);
	t = (std::min)(t, l1 * 0.42);
	t = (std::min)(t, l2 * 0.42);
	*oT1x = sx - u1x * t;
	*oT1y = sy - u1y * t;
	*oT2x = sx + u2x * t;
	*oT2y = sy + u2y * t;
	double bx = -u1x + u2x;
	double by = -u1y + u2y;
	const double bl = std::hypot(bx, by);
	if (bl < 1e-8)
		return false;
	bx /= bl;
	by /= bl;
	if (bx * sx + by * sy > 0.0)
	{
		bx = -bx;
		by = -by;
	}
	const double dist = r / std::sin(phi * 0.5);
	*oMx = sx + bx * dist;
	*oMy = sy + by * dist;
	return true;
}

static bool ParallelFlankInnerHit(
	double ox,
	double oy,
	double dx,
	double dy,
	double Ri,
	double* ix,
	double* iy)
{
	const double b = 2.0 * (ox * dx + oy * dy);
	const double c = ox * ox + oy * oy - Ri * Ri;
	const double disc = b * b - 4.0 * c;
	if (disc < 0.0)
		return false;
	const double sd = std::sqrt(disc);
	const double s1 = (-b - sd) * 0.5;
	const double s2 = (-b + sd) * 0.5;
	const double Ro0 = std::hypot(ox, oy);
	double bestS = s1;
	double bestE = 1e300;
	for (double s : {s1, s2})
	{
		const double r = std::hypot(ox + s * dx, oy + s * dy);
		const double e = std::abs(r - Ri);
		if (r < Ro0 - 1e-6 && e < bestE)
		{
			bestE = e;
			bestS = s;
		}
	}
	if (bestE > 1e200)
	{
		for (double s : {s1, s2})
		{
			const double r = std::hypot(ox + s * dx, oy + s * dy);
			const double e = std::abs(r - Ri);
			if (e < bestE)
			{
				bestE = e;
				bestS = s;
			}
		}
	}
	*ix = ox + bestS * dx;
	*iy = oy + bestS * dy;
	return true;
}

static void SpiderMidOnCircleArc(
	double R,
	double x0,
	double y0,
	double x1,
	double y1,
	double* mx,
	double* my)
{
	const double t0 = std::atan2(y0, x0);
	const double t1 = std::atan2(y1, x1);
	double d = t1 - t0;
	while (d > kPi)
		d -= 2.0 * kPi;
	while (d < -kPi)
		d += 2.0 * kPi;
	const double tm = t0 + 0.5 * d;
	*mx = R * std::cos(tm);
	*my = R * std::sin(tm);
}

void DrawSpiderProfile(ksDocument2DPtr p2DDoc, int n, double Ro, double Ri, double filletR, double legWidthB)
{
	const double riDraw = SpiderInnerValleyRadiusMm(Ro, Ri, filletR);
	double capHalfDeg = (180.0 / kPi) * std::asin((std::min)(0.995, legWidthB / (2.0 * Ro)));
	capHalfDeg = (std::max)(6.0, (std::min)(capHalfDeg, 180.0 / n - 1.0));
	const bool fourRayChord90 = (n == 4);
	if (fourRayChord90)
		capHalfDeg = 45.0;

	const double toRad = kPi / 180.0;
	const double rCornerMin = (std::min)(0.10, (std::max)(0.025, Ro * 0.0045));
	const double rCorner = (std::max)(rCornerMin, (std::min)(filletR, Ro * 0.15));

	if (n == 6)
	{
		const double riInner = Ri;
		for (int k = 0; k < 6; ++k)
		{
			const double midDeg = -90.0 + k * 60.0;
			const double mid = midDeg * toRad;
			const double tx = -std::sin(mid);
			const double ty = std::cos(mid);
			const double halfB = legWidthB * 0.5;
			const double sa = (std::min)(0.999, halfB / Ro);
			const double deltaRad = std::asin(sa);
			const double mLdeg = midDeg - deltaRad * 180.0 / kPi;
			const double mRdeg = midDeg + deltaRad * 180.0 / kPi;
			const double xOL = Ro * std::cos(mid - deltaRad);
			const double yOL = Ro * std::sin(mid - deltaRad);
			const double xOR = Ro * std::cos(mid + deltaRad);
			const double yOR = Ro * std::sin(mid + deltaRad);

			const double inDeg = -90.0 + (k + 0.5) * 60.0;
			const double prevInDeg =
				(k == 0) ? (-90.0 + 5.5 * 60.0) : (-90.0 + (k - 0.5) * 60.0);
			const double xV0 = riInner * std::cos(prevInDeg * toRad);
			const double yV0 = riInner * std::sin(prevInDeg * toRad);
			const double xV1 = riInner * std::cos(inDeg * toRad);
			const double yV1 = riInner * std::sin(inDeg * toRad);

			double xIL = xV0;
			double yIL = yV0;
			double xIR = xV1;
			double yIR = yV1;
			(void)ParallelFlankInnerHit(xOL, yOL, tx, ty, riInner, &xIL, &yIL);
			(void)ParallelFlankInnerHit(xOR, yOR, tx, ty, riInner, &xIR, &yIR);

			double xm0, ym0, xm1, ym1;
			SpiderMidOnCircleArc(riInner, xV0, yV0, xIL, yIL, &xm0, &ym0);
			SpiderMidOnCircleArc(riInner, xIR, yIR, xV1, yV1, &xm1, &ym1);
			p2DDoc->ksArcBy3Points(xV0, yV0, xm0, ym0, xIL, yIL, 1);
			p2DDoc->ksLineSeg(xIL, yIL, xOL, yOL, 1);
			p2DDoc->ksArcByAngle(0.0, 0.0, Ro, mLdeg, mRdeg, 1, 1);
			p2DDoc->ksLineSeg(xOR, yOR, xIR, yIR, 1);
			p2DDoc->ksArcBy3Points(xIR, yIR, xm1, ym1, xV1, yV1, 1);
		}
		return;
	}

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

		if (fourRayChord90)
		{
			double t1sx, t1sy, t2sx, t2sy, msx, msy;
			double t3ex, t3ey, t4ex, t4ey, mex, mey;
			const bool okS = SpiderTryOuterCornerFillet(
				xP,
				yP,
				xS,
				yS,
				xE,
				yE,
				rCorner,
				&t1sx,
				&t1sy,
				&t2sx,
				&t2sy,
				&msx,
				&msy);
			const bool okE = SpiderTryOuterCornerFillet(
				xS,
				yS,
				xE,
				yE,
				xI,
				yI,
				rCorner,
				&t3ex,
				&t3ey,
				&t4ex,
				&t4ey,
				&mex,
				&mey);
			if (okS)
			{
				p2DDoc->ksLineSeg(xP, yP, t1sx, t1sy, 1);
				p2DDoc->ksArcBy3Points(t1sx, t1sy, msx, msy, t2sx, t2sy, 1);
			}
			else
				p2DDoc->ksLineSeg(xP, yP, xS, yS, 1);
			if (okE)
			{
				const double ax = okS ? t2sx : xS;
				const double ay = okS ? t2sy : yS;
				p2DDoc->ksLineSeg(ax, ay, t3ex, t3ey, 1);
				p2DDoc->ksArcBy3Points(t3ex, t3ey, mex, mey, t4ex, t4ey, 1);
				p2DDoc->ksLineSeg(t4ex, t4ey, xI, yI, 1);
			}
			else if (okS)
				p2DDoc->ksLineSeg(t2sx, t2sy, xE, yE, 1);
			else
				p2DDoc->ksLineSeg(xS, yS, xE, yE, 1);
			if (!okE && !okS)
				p2DDoc->ksLineSeg(xE, yE, xI, yI, 1);
			else if (!okE && okS)
				p2DDoc->ksLineSeg(xE, yE, xI, yI, 1);
		}
		else
		{
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
			(n == 6) ? Ri : SpiderInnerValleyRadiusMm(Ro, Ri, rf);

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
		KsAxisLineXThroughOriginStyle3(p2DDoc);
		pSketchDef->EndEdit();

		ksEntityPtr pBoss = pPart->NewEntity(o3d_bossExtrusion);
		ksBossExtrusionDefinitionPtr pBossDef = pBoss->GetDefinition();
		pBossDef->SetSketch(pSketch);
		pBossDef->directionType = dtNormal;
		pBossDef->SetSideParam(TRUE, etBlind, H, 0, FALSE);
		pBoss->Create();

		try
		{
			pDoc->RebuildDocument();
		}
		catch (const _com_error&)
		{
		}

		if (n == 4 || n == 6)
			AddSpiderInnerCylinderFlankFillets(
				pPart,
				pDoc,
				pBoss,
				riProfile,
				H,
				s.filletRadius,
				n,
				err);

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
			pPlAngDef->SetAngle(planeRotateDegAroundZ);
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
	CString* err)
{
	if (lugCount < 2 || L_jaw < 1.0)
		return;

	const double R = D * 0.5;
	const double r = d * 0.5;
	if (R <= r + 0.25)
		return;

	const double degStep = 360.0 / static_cast<double>(lugCount);
	const double chordB = (std::max)(faceSlotB, faceSlotB1 * 0.52);
	double halfAngleOuter = std::asin((std::min)(0.995, chordB / (2.0 * R)));
	const double maxHalf = (kPi / static_cast<double>(lugCount)) * 0.46;
	halfAngleOuter = (std::min)(halfAngleOuter, maxHalf);
	halfAngleOuter = (std::max)(
		halfAngleOuter,
		(std::min)(11.0 * kPi / 180.0, maxHalf * 0.58));
	halfAngleOuter = (std::min)(halfAngleOuter, maxHalf);

	double depthUse = (std::max)(1.2, toothRadialDepth);
	depthUse = (std::min)(depthUse, (R - r) * 0.85);
	depthUse = (std::min)(depthUse, R * 0.22);

	const double rOut = (std::min)(R + depthUse, D * 0.52);
	double rIn = (std::max)(r + 0.55, R - depthUse);
	rIn = (std::min)(rIn, rOut - 0.35);
	rIn = (std::max)(rIn, r + 0.35);
	if (rOut <= rIn + 0.2 || rIn <= r + 0.15)
		return;

	double halfAngleInner = std::atan(std::tan(halfAngleOuter) * (rIn / R) * 0.88);
	halfAngleInner = (std::max)(halfAngleInner, 6.5 * kPi / 180.0);
	halfAngleInner = (std::min)(halfAngleInner, halfAngleOuter - 0.035);

	const double toothH = (lengthL3mm > 0.6)
		? (std::min)(lengthL3mm, L_jaw * 0.96)
		: (std::min)(L_jaw * 0.90, (std::max)(3.5, spiderLegWidth * 1.08));
	if (toothH < 0.8)
		return;

	const double a0 = -halfAngleInner;
	const double a1 = halfAngleInner;
	const double b0 = -halfAngleOuter;
	const double b1 = halfAngleOuter;

	try
	{
		ksEntityPtr pSkTooth = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pSkToothDef = pSkTooth->GetDefinition();
		pSkToothDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
		pSkTooth->Create();

		ksDocument2DPtr p2DDoc = pSkToothDef->BeginEdit();
		double point_1[4][2]{};
		point_1[0][0] = rIn * std::cos(a0);
		point_1[0][1] = rIn * std::sin(a0);
		point_1[1][0] = rOut * std::cos(b0);
		point_1[1][1] = rOut * std::sin(b0);
		point_1[2][0] = rOut * std::cos(b1);
		point_1[2][1] = rOut * std::sin(b1);
		point_1[3][0] = rIn * std::cos(a1);
		point_1[3][1] = rIn * std::sin(a1);
		p2DDoc->ksLineSeg(point_1[0][0], point_1[0][1], point_1[1][0], point_1[1][1], 1);
		p2DDoc->ksLineSeg(point_1[1][0], point_1[1][1], point_1[2][0], point_1[2][1], 1);
		p2DDoc->ksLineSeg(point_1[2][0], point_1[2][1], point_1[3][0], point_1[3][1], 1);
		p2DDoc->ksLineSeg(point_1[3][0], point_1[3][1], point_1[0][0], point_1[0][1], 1);
		KsAxisLineXThroughOriginStyle3(p2DDoc);
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
	}
	catch (const _com_error&)
	{
	}

	(void)err;
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
		if (!AddAnnulusBoss(pPart, pXoy, R, r, L_jaw, err))
			return false;

		ksEntityPtr pPlHub = pPart->NewEntity(o3d_planeOffset);
		ksPlaneOffsetDefinitionPtr pPlHubDef = pPlHub->GetDefinition();
		pPlHubDef->SetPlane(pXoy);
		pPlHubDef->direction = VARIANT_TRUE;
		pPlHubDef->offset = L_jaw;
		pPlHub->Create();

		if (!AddAnnulusBoss(pPart, pPlHub, rHub, r, L_hub_seg, err))
			return false;

		AddHalfCouplingShoulderChamferCut(pPart, R, rHub, L_jaw, h.shoulderRadiusR);

		const double keyHalfW = h.keywayWidthB * 0.5;
		const double t1 = (std::max)(0.1, h.keywayDt1 - d);
		const double keyDepth = (std::min)(
			(std::max)(t1, 0.15),
			(std::max)(0.6, (d1 - d) * 0.62));
		double zKey0 = L_jaw + L_hub_seg * 0.16;
		double zKey1 = L1 - L_hub_seg * 0.12;
		if (h.lengthL2 > 0.5)
			zKey0 = L_jaw + (std::min)(h.lengthL2 * 0.22, L_hub_seg * 0.45);
		if (h.lengthL3 > 0.5)
			zKey1 = L1 - (std::min)(h.lengthL3 * 0.32, L_hub_seg * 0.5);
		if (zKey1 <= zKey0 + 2.0)
			zKey1 = L1 - L_hub_seg * 0.1;
		if (zKey1 <= zKey0 + 2.0)
			zKey0 = L_jaw + L_hub_seg * 0.12;
		zKey0 = (std::max)(zKey0, L_jaw + 0.2);
		zKey1 = (std::min)(zKey1, L1 - 0.2);
		if (zKey1 <= zKey0 + 1.8)
			zKey1 = L1 - 0.15;
		const int nLug = (std::max)(2, lugCount);
		const double keywayPlaneYawDeg = (nLug == 2) ? 45.0 : 0.0;
		AddKeywayOnHub(
			pPart,
			rHub,
			zKey0,
			zKey1,
			keyHalfW,
			keyDepth + 0.5,
			h.filletR,
			keywayPlaneYawDeg);
		const double toothDepth = (std::min)(8.5, (std::max)(2.8, spiderLegWidth * 0.62));
		AddRadialLugs(
			pPart,
			D,
			d,
			L_jaw,
			nLug,
			toothDepth,
			h.faceSlotB,
			h.faceSlotB1,
			h.lengthL3,
			spiderLegWidth,
			err);

		const double slotCut = (std::min)((std::min)(L_jaw * 0.52, 7.5), R * 0.35);
		AddFaceSlotsBetweenLugs(
			pPart,
			D,
			d,
			h.faceSlotB,
			h.faceSlotB1,
			slotCut,
			nLug,
			h.gostTableId);

		(void)gostSeriesTorqueNm;

		AddHalfCouplingHubEndAndJawFaceChamferCuts(pPart, R, r, rHub, L_jaw, L1, h.gostTableId);

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

		pAsmDoc->SetPartFromFile(static_cast<LPCWSTR>(pathSpider), nullptr, VARIANT_TRUE);
		pAsmDoc->SetPartFromFile(static_cast<LPCWSTR>(pathHalf1), nullptr, VARIANT_TRUE);
		pAsmDoc->SetPartFromFile(static_cast<LPCWSTR>(pathHalf2), nullptr, VARIANT_TRUE);

		try
		{
			const SpiderParams& sp = doc.GetSpiderParams();
			const double Hsp = (std::max)(1.0, sp.thickness);
			const double gAsm = 1.0;
			const double zStarLow = -0.5 * Hsp;

			ksPartCollectionPtr coll = pAsmDoc->PartCollection(VARIANT_TRUE);
			if (coll != nullptr && coll->GetCount() < 3 && err != nullptr)
				*err += L"\nВ сборку вставлено меньше трёх деталей — имена и расстановка пропущены.";

			if (coll != nullptr && coll->GetCount() >= 3)
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
		const CString prefix = err->IsEmpty() ? CString(L"КОМПАС-3D: построение по ГОСТ 14084-76.") : *err;
		err->Format(
			L"%s\r\n\r\n"
			L"Сохранено в: %s\r\n"
			L"— Звёздочка.m3d, Полумуфта1.m3d, Полумуфта2.m3d\r\n"
			L"— Муфта_Сборка.a3d (в дереве: Звёздочка, Полумуфта 1, Полумуфта 2)\r\n"
			L"Экспорт STEP/IGES: через «Сохранить как» в КОМПАСе при необходимости.",
			static_cast<LPCWSTR>(prefix),
			static_cast<LPCWSTR>(dir));
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