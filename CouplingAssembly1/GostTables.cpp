#include "pch.h"
#include "framework.h"
#include "GostTables.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
constexpr double kTorqueSeriesNm[] = {2.5, 6.3, 16.0, 25.0, 31.5, 63.0, 125.0, 250.0, 400.0};
constexpr int kTorqueSeriesNmCount = static_cast<int>(sizeof(kTorqueSeriesNm) / sizeof(kTorqueSeriesNm[0]));
}

namespace GostTables
{

struct Asm2131Row
{
	double mkp;
	int execution;
	double nmax;
	double D;
	double D1;
	double L;
	double L1_half;
	double l_hub_asm;
	double l1_asm;
	double l2_asm;
	double B_spider_asm;
	double b1;
	double R_asm;
	double mass;
};

struct HalfFullRow
{
	double mkp;
	double d;
	unsigned char execMask;
	int tableId;
	double dt1_e1;
	double dt1_e2;
	double b;
	double d1;
	double D;
	double l_e1;
	double l_e2;
	double l1_e1;
	double l1_e2;
	double l2;
	double l3;
	double B;
	double B1;
	double r;
};

struct SpiderRow
{
	double mkp;
	int rays;
	double D;
	double d_inner;
	double B;
	double H;
	double r;
	double mass;
};

constexpr Asm2131Row kAsm2131[] = {
	{2.5, 1, 6500, 32, 30, 45.5, 28, 10.5, 16, 6, 8.5, 4, 1.2, 0.16},
	{6.3, 1, 5000, 45, 42, 59.5, 35, 10.5, 16, 8, 10.5, 5, 1.6, 0.30},
	{16.0, 2, 4500, 53, 50, 81.0, 48, 15, 28, 10, 10.5, 5, 1.6, 0.60},
	{25.0, 2, 4000, 63, 60, 91.0, 53, 15, 28, 10, 12.5, 6, 1.6, 0.75},
	{31.5, 2, 4000, 71, 70, 101.0, 58, 15, 28, 10, 12.5, 6, 1.6, 0.90},
	{63.0, 2, 3500, 85, 80, 128.0, 75, 22, 40, 12, 14.5, 7, 2.0, 1.90},
	{125.0, 2, 3000, 105, 100, 148.0, 85, 22, 40, 12, 16.5, 8, 2.0, 3.50},
	{250.0, 2, 2000, 135, 130, 191.0, 108, 25, 48, 12, 18.5, 9, 3.0, 8.00},
	{400.0, 2, 1500, 166, 160, 196.0, 113, 30, 56, 12, 20.5, 10, 3.0, 12.00},
};

constexpr HalfFullRow kHalf[] = {
	{2.5, 6, 1, 1, 7.0, 7.0, 2, 20, 32, 16, 16, 28, 28, 0, 0, 4, 16, 0.1},
	{2.5, 7, 1, 1, 8.0, 8.0, 2, 20, 32, 16, 16, 28, 28, 0, 0, 4, 16, 0.1},
	{6.3, 10, 3, 1, 11.4, 11.4, 3, 22, 45, 23, 20, 35, 32, 0, 0, 5, 20, 0.1},
	{6.3, 11, 3, 1, 12.8, 12.8, 4, 22, 45, 23, 20, 35, 32, 0, 0, 5, 20, 0.1},
	{6.3, 12, 3, 1, 13.8, 13.8, 4, 24, 45, 30, 25, 42, 37, 0, 0, 5, 20, 0.1},
	{6.3, 14, 3, 1, 16.3, 16.3, 5, 26, 45, 30, 25, 42, 37, 0, 0, 5, 20, 0.2},

	{16.0, 12, 3, 2, 13.8, 13.8, 4, 26, 53, 30, 25, 48, 43, 28, 15, 5, 14, 0.1},
	{16.0, 14, 3, 2, 16.3, 16.3, 5, 26, 53, 40, 28, 58, 46, 28, 15, 5, 14, 0.1},
	{16.0, 16, 3, 2, 18.3, 18.3, 5, 26, 53, 40, 28, 58, 46, 28, 15, 5, 14, 0.1},
	{16.0, 18, 3, 2, 20.8, 20.8, 6, 26, 53, 40, 28, 58, 46, 28, 15, 5, 14, 0.1},

	{25.0, 14, 3, 2, 16.3, 16.3, 5, 28, 63, 30, 25, 48, 43, 28, 15, 5, 14, 0.1},
	{25.0, 16, 3, 2, 18.3, 18.3, 5, 28, 63, 40, 28, 58, 46, 28, 15, 5, 14, 0.1},
	{25.0, 18, 3, 2, 20.8, 20.8, 6, 28, 63, 40, 28, 58, 46, 28, 15, 5, 14, 0.1},
	{25.0, 19, 3, 2, 21.8, 21.8, 6, 28, 63, 40, 28, 58, 46, 28, 15, 5, 14, 0.1},
	{25.0, 20, 3, 2, 22.8, 22.8, 6, 30, 63, 50, 36, 68, 54, 28, 15, 6, 16, 0.2},

	{31.5, 16, 3, 2, 18.3, 18.3, 5, 30, 71, 40, 28, 58, 46, 28, 15, 6, 16, 0.2},
	{31.5, 18, 3, 2, 20.8, 20.8, 6, 30, 71, 40, 28, 58, 46, 28, 15, 6, 16, 0.2},
	{31.5, 19, 3, 2, 21.8, 21.8, 6, 30, 71, 40, 28, 58, 46, 28, 15, 6, 16, 0.2},
	{31.5, 20, 3, 2, 22.8, 22.8, 6, 34, 71, 50, 36, 68, 54, 28, 15, 6, 16, 0.2},
	{31.5, 22, 3, 2, 24.8, 24.8, 6, 34, 71, 50, 36, 68, 54, 28, 15, 6, 16, 0.2},

	{63.0, 20, 3, 2, 22.8, 22.8, 6, 36, 85, 50, 36, 75, 61, 40, 22, 7, 21, 0.2},
	{63.0, 22, 3, 2, 24.8, 24.8, 6, 36, 85, 50, 36, 75, 61, 40, 22, 7, 21, 0.2},
	{63.0, 24, 3, 2, 27.3, 27.3, 8, 36, 85, 50, 36, 75, 61, 40, 22, 7, 21, 0.2},
	{63.0, 25, 3, 2, 28.3, 28.3, 8, 42, 85, 60, 42, 85, 67, 40, 22, 7, 21, 0.2},
	{63.0, 28, 3, 2, 31.3, 31.3, 8, 42, 85, 60, 42, 85, 67, 40, 22, 7, 21, 0.2},

	{125.0, 25, 3, 2, 28.3, 28.3, 8, 45, 105, 60, 42, 85, 67, 40, 22, 8, 25, 0.2},
	{125.0, 28, 3, 2, 31.3, 31.3, 8, 45, 105, 60, 42, 85, 67, 40, 22, 8, 25, 0.2},
	{125.0, 30, 3, 2, 33.3, 33.8, 8, 45, 105, 60, 42, 85, 67, 40, 22, 8, 25, 0.2},
	{125.0, 32, 3, 2, 35.3, 35.8, 10, 48, 105, 80, 58, 105, 83, 40, 22, 8, 25, 0.2},
	{125.0, 35, 3, 2, 38.3, 38.8, 10, 52, 105, 80, 58, 105, 83, 40, 22, 8, 25, 0.2},
	{125.0, 36, 3, 2, 39.3, 39.8, 10, 52, 105, 80, 58, 105, 83, 40, 22, 8, 25, 0.2},

	{250.0, 32, 3, 2, 35.3, 35.8, 10, 55, 135, 80, 58, 108, 86, 48, 25, 9, 32, 0.3},
	{250.0, 35, 3, 2, 38.3, 38.8, 10, 60, 135, 80, 58, 108, 86, 48, 25, 9, 32, 0.3},
	{250.0, 36, 3, 2, 39.3, 39.8, 10, 60, 135, 80, 58, 108, 86, 48, 25, 9, 32, 0.3},
	{250.0, 38, 3, 2, 41.3, 41.8, 10, 60, 135, 80, 58, 108, 86, 48, 25, 9, 32, 0.3},
	{250.0, 40, 3, 2, 43.3, 44.4, 12, 60, 135, 80, 58, 108, 86, 48, 25, 9, 32, 0.3},
	{250.0, 42, 3, 2, 45.3, 46.4, 12, 65, 135, 110, 82, 138, 110, 48, 25, 9, 32, 0.3},
	{250.0, 45, 3, 2, 48.8, 49.9, 14, 70, 135, 110, 82, 138, 110, 48, 25, 9, 32, 0.3},

	{400.0, 38, 3, 2, 41.3, 41.8, 10, 63, 166, 80, 58, 113, 91, 56, 30, 10, 38, 0.3},
	{400.0, 40, 3, 2, 43.3, 44.4, 12, 63, 166, 80, 58, 113, 91, 56, 30, 10, 38, 0.3},
	{400.0, 42, 3, 2, 45.3, 46.4, 12, 70, 166, 110, 82, 143, 115, 56, 30, 10, 38, 0.3},
	{400.0, 45, 3, 2, 48.8, 49.9, 14, 70, 166, 110, 82, 143, 115, 56, 30, 10, 38, 0.3},
	{400.0, 48, 3, 2, 51.8, 52.9, 14, 75, 166, 110, 82, 143, 115, 56, 30, 10, 38, 0.3},
};

constexpr SpiderRow kSpider4[] = {
	{2.5, 4, 30, 14, 8.5, 10.5, 1.25, 0.009},
	{6.3, 4, 42, 20, 10.5, 10.5, 1.6, 0.012},
	{16.0, 4, 50, 26, 10.5, 15.0, 1.6, 0.032},
	{25.0, 4, 60, 30, 12.5, 15.0, 1.6, 0.040},
	{31.5, 4, 67, 30, 12.5, 15.0, 1.6, 0.043},
	{63.0, 4, 80, 36, 14.5, 22.0, 2.0, 0.090},
	{125.0, 4, 100, 45, 16.5, 22.0, 2.0, 0.135},
	{250.0, 4, 130, 56, 18.5, 25.0, 3.0, 0.264},
	{400.0, 4, 160, 67, 20.5, 30.0, 3.0, 0.485},
};

constexpr SpiderRow kSpider6[] = {
	{6.3, 6, 42, 20, 10.5, 10.5, 1.6, 0.012},
	{16.0, 6, 50, 26, 10.5, 15.0, 1.6, 0.032},
	{25.0, 6, 60, 30, 12.5, 15.0, 1.6, 0.040},
	{31.5, 6, 67, 30, 12.5, 15.0, 1.6, 0.043},
	{63.0, 6, 80, 36, 14.5, 22.0, 2.0, 0.090},
	{125.0, 6, 100, 45, 16.5, 22.0, 2.0, 0.135},
	{250.0, 6, 130, 56, 18.5, 25.0, 3.0, 0.264},
	{400.0, 6, 160, 67, 20.5, 30.0, 3.0, 0.485},
};

const Asm2131Row* FindAsm2131(double mkp, int execution)
{
	const Asm2131Row* best = nullptr;
	double bestDiff = 1e300;
	for (const auto& r : kAsm2131)
	{
		if (r.execution != execution)
			continue;
		const double d = std::abs(r.mkp - mkp);
		if (d < bestDiff)
		{
			bestDiff = d;
			best = &r;
		}
	}
	if (best != nullptr)
		return best;
	bestDiff = 1e300;
	for (const auto& r : kAsm2131)
	{
		const double d = std::abs(r.mkp - mkp);
		if (d < bestDiff)
		{
			bestDiff = d;
			best = &r;
		}
	}
	return best;
}

static const Asm2131Row* ResolveAsm2131RowForTorque(double t, int exec)
{
	exec = ClampExecution(exec);
	for (const auto& r : kAsm2131)
	{
		if (std::abs(r.mkp - t) < 1e-6 && r.execution == exec)
			return &r;
	}
	for (const auto& r : kAsm2131)
	{
		if (std::abs(r.mkp - t) < 1e-6)
			return &r;
	}
	const Asm2131Row* ar = FindAsm2131(t, exec);
	if (ar != nullptr && exec == 1 && std::abs(ar->mkp - t) > 1e-3)
		ar = FindAsm2131(t, 2);
	return ar;
}

const HalfFullRow* FindHalfRow(double mkp, int execution, double dShaft)
{
	const int wantMask = (execution == 1) ? 1 : 2;
	const HalfFullRow* best = nullptr;
	double bestDd = 1e300;
	for (const auto& r : kHalf)
	{
		if (std::abs(r.mkp - mkp) > 0.001)
			continue;
		if ((r.execMask & wantMask) == 0)
			continue;
		const double dd = std::abs(r.d - dShaft);
		if (dd < bestDd)
		{
			bestDd = dd;
			best = &r;
		}
	}
	return best;
}

const SpiderRow* FindSpider4(double mkp)
{
	const SpiderRow* best = nullptr;
	double bestDiff = 1e300;
	for (const auto& r : kSpider4)
	{
		const double d = std::abs(r.mkp - mkp);
		if (d < bestDiff)
		{
			bestDiff = d;
			best = &r;
		}
	}
	return best;
}

const SpiderRow* FindSpider6(double mkp)
{
	const SpiderRow* best = nullptr;
	double bestDiff = 1e300;
	for (const auto& r : kSpider6)
	{
		const double d = std::abs(r.mkp - mkp);
		if (d < bestDiff)
		{
			bestDiff = d;
			best = &r;
		}
	}
	return best;
}

void FillHalfFromRow(HalfCouplingParams& h, const HalfFullRow& r, double snappedTorqueNm)
{
	h.gostTableId = r.tableId;
	h.boreDiameter = r.d;
	h.keywayWidthB = r.b;
	h.hubOuterD1 = r.d1;
	h.outerDiameter = r.D;
	h.lengthL2 = r.l2;
	h.lengthL3 = r.l3;
	h.faceSlotB = r.B;
	h.faceSlotB1 = r.B1;
	h.filletR = r.r;
	h.shoulderRadiusR = (r.tableId == 1) ? 1.0 : 2.0;

	const bool useE1 = (r.tableId == 1) ? true : HalfCouplingJawUsesDims11(snappedTorqueNm);
	if (useE1)
	{
		h.keywayDt1 = r.dt1_e1;
		h.lengthHubL = r.l_e1;
		h.lengthTotalL1 = r.l1_e1;
	}
	else
	{
		h.keywayDt1 = r.dt1_e2;
		h.lengthHubL = r.l_e2;
		h.lengthTotalL1 = r.l1_e2;
	}

	h.lugCount = (r.tableId == 1) ? 2 : 3;
	h.SyncLegacyAliases();
}

void FillSpiderFromRow(SpiderParams& s, const SpiderRow& r)
{
	s.outerDiameter = r.D;
	s.innerDiameter = r.d_inner;
	s.legWidth = r.B;
	s.thickness = r.H;
	s.filletRadius = r.r;
	s.massKg = r.mass;
	s.rays = r.rays;
}

double SnapTorqueToSeries(double torqueNm)
{
	double best = kTorqueSeriesNm[0];
	double bestDiff = std::abs(torqueNm - kTorqueSeriesNm[0]);
	for (int i = 0; i < kTorqueSeriesNmCount; ++i)
	{
		const double t = kTorqueSeriesNm[i];
		const double d = std::abs(torqueNm - t);
		if (d < bestDiff)
		{
			bestDiff = d;
			best = t;
		}
	}
	return best;
}

int TorqueSeriesCount()
{
	return kTorqueSeriesNmCount;
}

double TorqueSeriesValue(int indexFromZero)
{
	if (indexFromZero < 0 || indexFromZero >= kTorqueSeriesNmCount)
		return kTorqueSeriesNm[0];
	return kTorqueSeriesNm[indexFromZero];
}

int TorqueSeriesIndexNearest(double torqueNm)
{
	const double b = SnapTorqueToSeries(torqueNm);
	for (int i = 0; i < kTorqueSeriesNmCount; ++i)
	{
		if (std::abs(kTorqueSeriesNm[i] - b) < 1e-9)
			return i;
	}
	return 0;
}

int ClampExecution(int execution)
{
	if (execution < 1)
		return 1;
	if (execution > 2)
		return 2;
	return execution;
}

int ExecutionFromCourseVariant(int courseVariant)
{
	if (courseVariant <= 2)
		return 1;
	return 2;
}

bool HalfCouplingJawUsesDims11(double snappedTorqueNm)
{
	const double t = snappedTorqueNm;
	return std::abs(t - 2.5) < 0.05 || std::abs(t - 6.3) < 0.05;
}

double SetscrewHoleRadiusMm(double seriesTorqueNm)
{
	if (seriesTorqueNm <= 31.5 + 1e-9)
		return 2.05;
	if (seriesTorqueNm <= 125.0 + 1e-9)
		return 2.55;
	return 3.15;
}

void ApplyCourseVariantRule(
	AssemblyParams& assembly,
	HalfCouplingParams& half1,
	HalfCouplingParams& half2,
	SpiderParams& spider)
{
	const int ex = ClampExecution(assembly.execution);
	assembly.execution = ex;
	spider.rays = (ex == 1) ? 4 : 6;
	const int lugs = (ex == 1) ? 2 : 3;
	half1.lugCount = lugs;
	half2.lugCount = lugs;
	half1.SyncLegacyAliases();
	half2.SyncLegacyAliases();
}

void FillAssemblyTable2131(AssemblyParams& assembly)
{
	const double t = SnapTorqueToSeries(assembly.torque);
	const int exec = ClampExecution(assembly.execution);
	const Asm2131Row* ar = ResolveAsm2131RowForTorque(t, exec);
	if (ar != nullptr)
	{
		assembly.execution = ClampExecution(ar->execution);
		assembly.assemblyLengthL = ar->L;
		assembly.envelopeD1 = ar->D1;
		assembly.maxSpeedRpm = ar->nmax;
		assembly.massKg = ar->mass;
		assembly.widthB1 = ar->b1;
	}
}

bool LookupHalfFromGost(const AssemblyParams& assembly, HalfCouplingParams& half, double shaftDiameterMm)
{
	const double t = SnapTorqueToSeries(assembly.torque);
	const int exec = ClampExecution(assembly.execution);
	const HalfFullRow* h = FindHalfRow(t, exec, shaftDiameterMm);
	if (h == nullptr && exec == 2 && t <= 6.3 + 1e-9)
		h = FindHalfRow(16.0, exec, shaftDiameterMm);
	if (h == nullptr && exec == 1)
		h = FindHalfRow(t, 2, shaftDiameterMm);
	if (h == nullptr)
		return false;
	FillHalfFromRow(half, *h, t);
	return true;
}

void LookupSpiderFromGost(const AssemblyParams& assembly, SpiderParams& spider)
{
	const double t = SnapTorqueToSeries(assembly.torque);
	const int exec = ClampExecution(assembly.execution);
	const SpiderRow* sr = (exec == 1) ? FindSpider4(t) : FindSpider6(t);
	if (sr != nullptr)
		FillSpiderFromRow(spider, *sr);
	spider.rays = (exec == 1) ? 4 : 6;

	const Asm2131Row* ar = ResolveAsm2131RowForTorque(t, exec);
	if (ar != nullptr && ar->R_asm > 0.04)
		spider.filletRadius = (std::max)(spider.filletRadius, ar->R_asm);
}

void ApplyAssemblyToParts(
	AssemblyParams& assembly,
	HalfCouplingParams& half1,
	HalfCouplingParams& half2,
	SpiderParams& spider)
{
	FillAssemblyTable2131(assembly);
	ApplyCourseVariantRule(assembly, half1, half2, spider);
	LookupHalfFromGost(assembly, half1, assembly.shaftDiameter1);
	LookupHalfFromGost(assembly, half2, assembly.shaftDiameter2);
	LookupSpiderFromGost(assembly, spider);
}

}
