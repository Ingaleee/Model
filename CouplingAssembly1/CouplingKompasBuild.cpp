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

using namespace KompasAPI;

namespace
{
constexpr double kPi = 3.14159265358979323846;

double SetscrewHoleRadiusMm(double seriesTorqueNm)
{
	if (seriesTorqueNm <= 31.5 + 1e-9)
		return 2.05;
	if (seriesTorqueNm <= 125.0 + 1e-9)
		return 2.55;
	return 3.15;
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
	dir += L"CouplingAssembly1_kompas\\";
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

	ksDocument2DPtr p2d = pSketchDef->BeginEdit();
	p2d->ksCircle(0.0, 0.0, outerR, 1);
	p2d->ksCircle(0.0, 0.0, innerR, 1);
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
	try
	{
		ksEntityPtr pSk = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pDef = pSk->GetDefinition();
		pDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOZ));
		pSk->Create();
		ksDocument2DPtr p2d = pDef->BeginEdit();
		p2d->ksLineSeg(xa, za, xb, zb, 1);
		p2d->ksLineSeg(xb, zb, xc, zc, 1);
		p2d->ksLineSeg(xc, zc, xa, za, 1);
		pDef->EndEdit();
		ksEntityPtr pCut = pPart->NewEntity(o3d_cutRotated);
		ksCutRotatedDefinitionPtr pCutDef = pCut->GetDefinition();
		pCutDef->SetSketch(pSk);
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
	double cSh = (std::min)(2.2, (std::max)(0.35, shoulderRadiusR * 0.55));
	cSh = (std::min)(cSh, (R - rHub) * 0.42);
	cSh = (std::min)(cSh, L_jaw * 0.35);
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
	const double cJaw = (std::min)(cEnd, 1.0);

	if (cEnd > 0.08 && rHub > rBore + cEnd + 0.15 && L1 > L_jaw + cEnd)
	{
		TryMeridianRevolveCutTriangle(pPart, rHub - cEnd, L1 - cEnd, rHub, L1 - cEnd, rHub, L1);
		TryMeridianRevolveCutTriangle(pPart, rBore, L1, rBore, L1 - cEnd, rBore + cEnd, L1 - cEnd);
	}

	if (cJaw > 0.05 && L_jaw > cJaw + 0.2)
		TryMeridianRevolveCutTriangle(pPart, R, 0.0, R - cJaw, 0.0, R, cJaw);
}

void DrawSpiderProfile(ksDocument2DPtr p2d, int n, double Ro, double Ri, double filletR, double legWidthB)
{
	const double riDraw =
		(filletR > 0.02) ? (std::max)(Ro * 0.22, Ri - (std::min)(filletR * 1.55, Ri * 0.28)) : Ri;
	double capHalfDeg = (180.0 / kPi) * std::asin((std::min)(0.995, legWidthB / (2.0 * Ro)));
	capHalfDeg = (std::max)(6.0, (std::min)(capHalfDeg, 180.0 / n - 1.0));

	const double toRad = kPi / 180.0;

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

		p2d->ksLineSeg(xP, yP, xS, yS, 1);
		p2d->ksArcByAngle(0.0, 0.0, Ro, a1, a2, 1, 1);
		p2d->ksLineSeg(xE, yE, xI, yI, 1);
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

		ksDocument3DPtr pDoc = app->Document3D();
		pDoc->Create(VARIANT_FALSE, VARIANT_TRUE);
		app->Visible = VARIANT_TRUE;

		try
		{
			pDoc->Putcomment(_bstr_t(L"\u0413\u041e\u0421\u0422 14084-76 \u2014 \u0437\u0432\u0451\u0437\u0434\u043e\u0447\u043a\u0430 (\u044d\u043b\u0430\u0441\u0442\u0438\u0447\u043d\u0430\u044f)"));
		}
		catch (const _com_error&)
		{
		}

		ksPartPtr pPart = pDoc->GetPart(pTop_Part);

		ksEntityPtr pSketch = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pSketchDef = pSketch->GetDefinition();
		pSketchDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
		pSketch->Create();

		ksDocument2DPtr p2d = pSketchDef->BeginEdit();
		DrawSpiderProfile(p2d, n, Ro, Ri, rf, s.legWidth);
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
	double bottomCornerRadius)
{
	if (keyHalfWidth < 0.1 || cutDepth < 0.1 || z1 <= z0 + 0.1)
		return;

	ksEntityPtr pPlKey = pPart->NewEntity(o3d_planeOffset);
	ksPlaneOffsetDefinitionPtr pPlKeyDef = pPlKey->GetDefinition();
	pPlKeyDef->SetPlane(pPart->GetDefaultEntity(o3d_planeYOZ));
	pPlKeyDef->direction = VARIANT_TRUE;
	pPlKeyDef->offset = hubR;
	pPlKey->Create();

	ksEntityPtr pSkKey = pPart->NewEntity(o3d_sketch);
	ksSketchDefinitionPtr pSkKeyDef = pSkKey->GetDefinition();
	pSkKeyDef->SetPlane(pPlKey);
	pSkKey->Create();

	ksDocument2DPtr p2dK = pSkKeyDef->BeginEdit();
	double rkCap = (std::min)(keyHalfWidth * 0.42, cutDepth * 0.38);
	rkCap = (std::min)(rkCap, (z1 - z0) * 0.2);
	const double rk = (std::min)((std::max)(0.0, bottomCornerRadius), rkCap);
	if (rk < 0.12)
	{
		p2dK->ksLineSeg(-keyHalfWidth, z0, keyHalfWidth, z0, 1);
		p2dK->ksLineSeg(keyHalfWidth, z0, keyHalfWidth, z1, 1);
		p2dK->ksLineSeg(keyHalfWidth, z1, -keyHalfWidth, z1, 1);
		p2dK->ksLineSeg(-keyHalfWidth, z1, -keyHalfWidth, z0, 1);
	}
	else
	{
		p2dK->ksLineSeg(-keyHalfWidth, z1, -keyHalfWidth, z0 + rk, 1);
		p2dK->ksArcByAngle(-keyHalfWidth + rk, z0 + rk, rk, 180.0, 270.0, -1, 1);
		p2dK->ksLineSeg(-keyHalfWidth + rk, z0, keyHalfWidth - rk, z0, 1);
		p2dK->ksArcByAngle(keyHalfWidth - rk, z0 + rk, rk, 270.0, 360.0, -1, 1);
		p2dK->ksLineSeg(keyHalfWidth, z0 + rk, keyHalfWidth, z1, 1);
		p2dK->ksLineSeg(keyHalfWidth, z1, -keyHalfWidth, z1, 1);
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
	const double degStep = 360.0 / static_cast<double>(lugCount);
	const double chordB = (std::max)(faceSlotB, faceSlotB1 * 0.52);
	double halfAngleOuter = std::asin((std::min)(0.995, chordB / (2.0 * R)));
	const double maxHalf = (kPi / static_cast<double>(lugCount)) * 0.48;
	halfAngleOuter = (std::min)(halfAngleOuter, maxHalf);
	halfAngleOuter = (std::max)(halfAngleOuter, 13.0 * kPi / 180.0);

	const double rIn = (std::max)(r + 0.6, R - toothRadialDepth);
	const double rOut = R + toothRadialDepth;
	double halfAngleInner = std::atan(std::tan(halfAngleOuter) * (rIn / R) * 0.82);
	halfAngleInner = (std::max)(halfAngleInner, 7.0 * kPi / 180.0);
	halfAngleInner = (std::min)(halfAngleInner, halfAngleOuter - 0.02);

	const double toothH = (lengthL3mm > 0.6)
		? (std::min)(lengthL3mm, L_jaw * 0.98)
		: (std::min)(L_jaw * 0.92, (std::max)(4.0, spiderLegWidth * 1.15));

	const double a0 = -halfAngleInner;
	const double a1 = halfAngleInner;
	const double b0 = -halfAngleOuter;
	const double b1 = halfAngleOuter;

	ksEntityPtr pSkTooth = pPart->NewEntity(o3d_sketch);
	ksSketchDefinitionPtr pSkToothDef = pSkTooth->GetDefinition();
	pSkToothDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
	pSkTooth->Create();

	ksDocument2DPtr p2dT = pSkToothDef->BeginEdit();
	const double x0 = rIn * std::cos(a0);
	const double y0 = rIn * std::sin(a0);
	const double x1o = rOut * std::cos(b0);
	const double y1o = rOut * std::sin(b0);
	const double x2o = rOut * std::cos(b1);
	const double y2o = rOut * std::sin(b1);
	const double x3 = rIn * std::cos(a1);
	const double y3 = rIn * std::sin(a1);
	p2dT->ksLineSeg(x0, y0, x1o, y1o, 1);
	p2dT->ksLineSeg(x1o, y1o, x2o, y2o, 1);
	p2dT->ksLineSeg(x2o, y2o, x3, y3, 1);
	p2dT->ksLineSeg(x3, y3, x0, y0, 1);
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

	(void)err;
}

void AddFaceSlotsSmallExec(
	ksPartPtr pPart,
	double D,
	double d,
	double cutDepth)
{
	const double R = D * 0.5;
	const double r = d * 0.5;
	const double span = 11.0 * (kPi / 180.0);
	for (int k = 0; k < 2; ++k)
	{
		const double base = kPi / 4.0 + k * kPi;
		const double a0 = base - span * 0.5;
		const double a1 = base + span * 0.5;
		const double r0 = r + 0.8;
		const double r1 = R + 1.5;

		ksEntityPtr pSk = pPart->NewEntity(o3d_sketch);
		ksSketchDefinitionPtr pDef = pSk->GetDefinition();
		pDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
		pSk->Create();
		ksDocument2DPtr p2d = pDef->BeginEdit();
		const double x0 = r0 * std::cos(a0);
		const double y0 = r0 * std::sin(a0);
		const double x1 = r1 * std::cos(a0);
		const double y1 = r1 * std::sin(a0);
		const double x2 = r1 * std::cos(a1);
		const double y2 = r1 * std::sin(a1);
		const double x3 = r0 * std::cos(a1);
		const double y3 = r0 * std::sin(a1);
		p2d->ksLineSeg(x0, y0, x1, y1, 1);
		p2d->ksLineSeg(x1, y1, x2, y2, 1);
		p2d->ksLineSeg(x2, y2, x3, y3, 1);
		p2d->ksLineSeg(x3, y3, x0, y0, 1);
		pDef->EndEdit();

		ksEntityPtr pCut = pPart->NewEntity(o3d_cutExtrusion);
		ksCutExtrusionDefinitionPtr pCutDef = pCut->GetDefinition();
		pCutDef->SetSketch(pSk);
		pCutDef->directionType = dtNormal;
		pCutDef->SetSideParam(TRUE, etBlind, cutDepth, 0, FALSE);
		pCut->Create();
	}
}

void AddFaceSlotsLargeExec(
	ksPartPtr pPart,
	double D,
	double d,
	double B_slot,
	double B1_slot,
	double cutDepth)
{
	const double R = D * 0.5;
	const double r = d * 0.5;
	const double bEff = (std::max)(B_slot, (std::max)(0.0, B1_slot) * 0.72);
	const double spanDeg = (std::max)(12.0, (std::min)(32.0, bEff * 4.8));
	const double span = spanDeg * (kPi / 180.0);
	const double r0 = (std::max)(r + 1.0, R - bEff * 2.2);
	const double r1 = R + 1.2;

	ksEntityPtr pSk = pPart->NewEntity(o3d_sketch);
	ksSketchDefinitionPtr pDef = pSk->GetDefinition();
	pDef->SetPlane(pPart->GetDefaultEntity(o3d_planeXOY));
	pSk->Create();
	ksDocument2DPtr p2d = pDef->BeginEdit();
	const double a0 = -span * 0.5;
	const double a1 = span * 0.5;
	const double x0 = r0 * std::cos(a0);
	const double y0 = r0 * std::sin(a0);
	const double x1 = r1 * std::cos(a0);
	const double y1 = r1 * std::sin(a0);
	const double x2 = r1 * std::cos(a1);
	const double y2 = r1 * std::sin(a1);
	const double x3 = r0 * std::cos(a1);
	const double y3 = r0 * std::sin(a1);
	p2d->ksLineSeg(x0, y0, x1, y1, 1);
	p2d->ksLineSeg(x1, y1, x2, y2, 1);
	p2d->ksLineSeg(x2, y2, x3, y3, 1);
	p2d->ksLineSeg(x3, y3, x0, y0, 1);
	pDef->EndEdit();

	ksEntityPtr pCut = pPart->NewEntity(o3d_cutExtrusion);
	ksCutExtrusionDefinitionPtr pCutDef = pCut->GetDefinition();
	pCutDef->SetSketch(pSk);
	pCutDef->directionType = dtNormal;
	pCutDef->SetSideParam(TRUE, etBlind, cutDepth, 0, FALSE);
	pCut->Create();

	ksEntityPtr pAxis = pPart->GetDefaultEntity(o3d_axisOZ);
	ksEntityPtr pCirc = pPart->NewEntity(o3d_circularCopy);
	ksCircularCopyDefinitionPtr pCircDef = pCirc->GetDefinition();
	pCircDef->SetAxis(pAxis);
	pCircDef->count1 = 1;
	pCircDef->count2 = 3;
	pCircDef->step1 = 120.0;
	ksEntityCollectionPtr ops = pCircDef->GetOperationArray();
	ops->Clear();
	ops->Add(pCut);
	pCirc->Create();
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
	ksDocument2DPtr p2d = pDef->BeginEdit();
	p2d->ksCircle(hubR - 1.0, 0.0, holeR, 1);
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
				L"\u0413\u041e\u0421\u0422 14084-76 \u2014 \u043f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 %d (d=%.0f, D=%.0f \u043c\u043c)",
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
		AddKeywayOnHub(pPart, rHub, zKey0, zKey1, keyHalfW, keyDepth + 0.5, h.filletR);

		const double slotCut = (std::min)(L_jaw * 0.55, 8.0);
		if (h.gostTableId == 1)
			AddFaceSlotsSmallExec(pPart, D, d, slotCut);
		else
			AddFaceSlotsLargeExec(pPart, D, d, h.faceSlotB, h.faceSlotB1, slotCut);

		const int nLug = (std::max)(2, lugCount);
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

		if (h.gostTableId >= 2 && h.lengthL2 > 1.0)
		{
			const double zHole = L_jaw + (std::min)(h.lengthL2 * 0.45, L_hub_seg * 0.85);
			AddSetScrewHole(pPart, rHub, zHole, SetscrewHoleRadiusMm(gostSeriesTorqueNm));
		}

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
		const CStringW pathSpider = dir + L"Zvezdochka.m3d";
		const CStringW pathHalf1 = dir + L"Polymufta1.m3d";
		const CStringW pathHalf2 = dir + L"Polymufta2.m3d";
		const CStringW pathAsmOut = dir + L"Mufta_Sborka.a3d";

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
				_bstr_t(L"\u041c\u0443\u0444\u0442\u0430 \u0437\u0443\u0431\u0447\u0430\u0442\u043e-\u0437\u0432\u0451\u0437\u0434\u043e\u0447\u043d\u0430\u044f, \u0413\u041e\u0421\u0422 14084-76 \u2014 \u0441\u0431\u043e\u0440\u043a\u0430"));
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
				*err += L"\n\u0412 \u0441\u0431\u043e\u0440\u043a\u0443 \u0432\u0441\u0442\u0430\u0432\u043b\u0435\u043d\u043e \u043c\u0435\u043d\u044c\u0448\u0435 \u0442\u0440\u0451\u0445 \u0434\u0435\u0442\u0430\u043b\u0435\u0439 \u2014 \u0438\u043c\u0435\u043d\u0430 \u0438 \u0440\u0430\u0441\u0441\u0442\u0430\u043d\u043e\u0432\u043a\u0430 \u043f\u0440\u043e\u043f\u0443\u0449\u0435\u043d\u044b.";

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
							p->Putname(_bstr_t(L"\u0417\u0432\u0451\u0437\u0434\u043e\u0447\u043a\u0430"));
							p->Putmarking(_bstr_t(L"\u0413\u041e\u0421\u0422 14084-76"));
						}
						else if (k == 1)
						{
							p->Putname(_bstr_t(L"\u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 1"));
							p->Putmarking(_bstr_t(L"\u0412\u0430\u043b 1"));
						}
						else
						{
							p->Putname(_bstr_t(L"\u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 2"));
							p->Putmarking(_bstr_t(L"\u0412\u0430\u043b 2"));
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
	const CStringW pSpider = dir + L"Zvezdochka.m3d";
	const CStringW pHalf1 = dir + L"Polymufta1.m3d";
	const CStringW pHalf2 = dir + L"Polymufta2.m3d";

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
			L"\u0421\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u043e \u0432: %s\r\n"
			L"\u2014 Zvezdochka.m3d, Polymufta1.m3d, Polymufta2.m3d\r\n"
			L"\u2014 Mufta_Sborka.a3d (\u0432 \u0434\u0435\u0440\u0435\u0432\u0435: \u0417\u0432\u0451\u0437\u0434\u043e\u0447\u043a\u0430, \u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 1, \u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 2)\r\n"
			L"\u042d\u043a\u0441\u043f\u043e\u0440\u0442 STEP/IGES: \u0447\u0435\u0440\u0435\u0437 \u00ab\u0421\u043e\u0445\u0440\u0430\u043d\u0438\u0442\u044c \u043a\u0430\u043a\u00bb \u0432 \u041a\u041e\u041c\u041f\u0410\u0421\u0435 \u043f\u0440\u0438 \u043d\u0435\u043e\u0431\u0445\u043e\u0434\u0438\u043c\u043e\u0441\u0442\u0438.",
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
			L"Включите SDK (см. CouplingAssembly1.vcxproj), укажите путь к SDK и пересоберите.";
	}
	return false;
}

#endif
