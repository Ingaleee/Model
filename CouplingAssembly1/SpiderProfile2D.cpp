#include "pch.h"
#include "SpiderProfile2D.h"

#include <algorithm>
#include <cmath>

namespace SpiderProfile2D
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

void AppendArc3Interior(
	std::vector<std::pair<double, double>>& out,
	double ax,
	double ay,
	double mx,
	double my,
	double bx,
	double by,
	int nInterior)
{
	if (nInterior < 1)
		return;
	const double x1 = ax;
	const double y1 = ay;
	const double x2 = mx;
	const double y2 = my;
	const double x3 = bx;
	const double y3 = by;
	const double d =
		2.0 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
	if (std::fabs(d) < 1e-12)
		return;
	const double ux =
		((x1 * x1 + y1 * y1) * (y2 - y3) + (x2 * x2 + y2 * y2) * (y3 - y1) + (x3 * x3 + y3 * y3) * (y1 - y2)) / d;
	const double uy =
		((x1 * x1 + y1 * y1) * (x3 - x2) + (x2 * x2 + y2 * y2) * (x1 - x3) + (x3 * x3 + y3 * y3) * (x2 - x1)) / d;
	const double R = std::hypot(x1 - ux, y1 - uy);
	if (R < 1e-8)
		return;
	const double tA = std::atan2(y1 - uy, x1 - ux);
	const double tM = std::atan2(y2 - uy, x2 - ux);
	const double tB = std::atan2(y3 - uy, x3 - ux);
	auto normD = [](double a, double b) { return std::atan2(std::sin(b - a), std::cos(b - a)); };
	double dAB = normD(tA, tB);
	double dAM = normD(tA, tM);
	const bool mOnShort =
		(dAB > 0.0 && dAM > 0.0 && dAM < dAB - 1e-6) || (dAB < 0.0 && dAM < 0.0 && dAM > dAB + 1e-6);
	if (!mOnShort)
		dAB = (dAB > 0.0) ? dAB - 2.0 * kPi : dAB + 2.0 * kPi;
	const double tEnd = tA + dAB;
	for (int i = 1; i <= nInterior; ++i)
	{
		const double u = static_cast<double>(i) / static_cast<double>(nInterior + 1);
		const double tt = tA + u * (tEnd - tA);
		out.push_back({ux + R * std::cos(tt), uy + R * std::sin(tt)});
	}
}
}

bool ParallelFlankInnerHit(
	double ox,
	double oy,
	double dx,
	double dy,
	double Ri,
	double* ix,
	double* iy)
{
	const double a = dx * dx + dy * dy;
	if (a < 1e-20)
		return false;
	const double bLin = 2.0 * (ox * dx + oy * dy);
	const double c0 = ox * ox + oy * oy - Ri * Ri;
	const double disc = bLin * bLin - 4.0 * a * c0;
	if (disc < 0.0)
		return false;
	const double sd = std::sqrt(disc);
	const double s1 = (-bLin - sd) / (2.0 * a);
	const double s2 = (-bLin + sd) / (2.0 * a);
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

void Fill46RayInnerOuterPoints(
	int n,
	double Ro,
	double Ri,
	double legWidthB,
	double xIL[8],
	double yIL[8],
	double xOL[8],
	double yOL[8],
	double xOR[8],
	double yOR[8],
	double xIR[8],
	double yIR[8],
	double omx[8],
	double omy[8])
{
	if (n != 4 && n != 6)
		return;
	const double toRad = kPi / 180.0;
	const double stepDeg = 360.0 / static_cast<double>(n);
	const double sectorHalfRad = kPi / static_cast<double>(n);
	const double riInner = Ri;
	for (int k = 0; k < n; ++k)
	{
		const double midDeg = 90.0 + static_cast<double>(k) * stepDeg;
		const double mid = midDeg * toRad;
		const double tx = -std::sin(mid);
		const double ty = std::cos(mid);
		const double halfB = legWidthB * 0.5;
		const double sa = (std::min)(0.999, halfB / Ro);
		const double deltaRad =
			(n == 4)
				? (std::min)(std::asin(sa), sectorHalfRad)
				: (std::min)(std::asin(sa), sectorHalfRad * 0.88);
		xOL[k] = Ro * std::cos(mid - deltaRad);
		yOL[k] = Ro * std::sin(mid - deltaRad);
		xOR[k] = Ro * std::cos(mid + deltaRad);
		yOR[k] = Ro * std::sin(mid + deltaRad);
		omx[k] = Ro * std::cos(mid);
		omy[k] = Ro * std::sin(mid);

		const double inDeg = 90.0 + (static_cast<double>(k) + 0.5) * stepDeg;
		const double prevInDeg =
			(k == 0) ? (90.0 + (static_cast<double>(n) - 0.5) * stepDeg)
					 : (90.0 + (static_cast<double>(k) - 0.5) * stepDeg);
		const double xV0 = riInner * std::cos(prevInDeg * toRad);
		const double yV0 = riInner * std::sin(prevInDeg * toRad);
		const double xV1 = riInner * std::cos(inDeg * toRad);
		const double yV1 = riInner * std::sin(inDeg * toRad);

		xIL[k] = xV0;
		yIL[k] = yV0;
		xIR[k] = xV1;
		yIR[k] = yV1;
		(void)ParallelFlankInnerHit(xOL[k], yOL[k], tx, ty, riInner, &xIL[k], &yIL[k]);
		(void)ParallelFlankInnerHit(xOR[k], yOR[k], tx, ty, riInner, &xIR[k], &yIR[k]);
	}
}

void FlankFilletTargetAnglesDeg(
	int n,
	double Ro,
	double Ri,
	double legWidthB,
	double* outDeg,
	int outCap)
{
	if ((n != 4 && n != 6) || outCap < 2 * n)
		return;
	double xIL[8]{}, yIL[8]{}, xOL[8]{}, yOL[8]{}, xOR[8]{}, yOR[8]{}, xIR[8]{}, yIR[8]{};
	double omx[8]{}, omy[8]{};
	Fill46RayInnerOuterPoints(n, Ro, Ri, legWidthB, xIL, yIL, xOL, yOL, xOR, yOR, xIR, yIR, omx, omy);
	int idx = 0;
	for (int k = 0; k < n; ++k)
	{
		outDeg[idx++] = std::atan2(yIL[k], xIL[k]) * (180.0 / kPi);
		outDeg[idx++] = std::atan2(yIR[k], xIR[k]) * (180.0 / kPi);
	}
}

void AppendClosedContourMm(
	std::vector<std::pair<double, double>>& pts,
	int n,
	double Ro,
	double Ri,
	double legWidthB,
	int arcSegPerCap)
{
	pts.clear();
	if (n != 4 && n != 6)
		return;
	const double toRad = kPi / 180.0;
	const int nSeg = (std::max)(3, arcSegPerCap);
	double xIL[8]{}, yIL[8]{}, xOL[8]{}, yOL[8]{}, xOR[8]{}, yOR[8]{}, xIR[8]{}, yIR[8]{};
	double omx[8]{}, omy[8]{};
	Fill46RayInnerOuterPoints(n, Ro, Ri, legWidthB, xIL, yIL, xOL, yOL, xOR, yOR, xIR, yIR, omx, omy);
	const double stepDeg = 360.0 / static_cast<double>(n);

	pts.push_back({xIL[0], yIL[0]});
	for (int k = 0; k < n; ++k)
	{
		const int kp1 = (k + 1) % n;
		pts.push_back({xOL[k], yOL[k]});
		AppendArc3Interior(pts, xOL[k], yOL[k], omx[k], omy[k], xOR[k], yOR[k], nSeg);
		pts.push_back({xOR[k], yOR[k]});
		pts.push_back({xIR[k], yIR[k]});
		if (n == 4)
			pts.push_back({xIL[kp1], yIL[kp1]});
		else
		{
			const double bisectorDeg = 90.0 + (static_cast<double>(k) + 0.5) * stepDeg;
			const double bisRad = bisectorDeg * toRad;
			const double xVm = Ri * std::cos(bisRad);
			const double yVm = Ri * std::sin(bisRad);
			AppendArc3Interior(pts, xIR[k], yIR[k], xVm, yVm, xIL[kp1], yIL[kp1], nSeg);
			pts.push_back({xIL[kp1], yIL[kp1]});
		}
	}
	if (pts.size() > 2)
	{
		const auto& a = pts.front();
		const auto& b = pts.back();
		if (std::hypot(a.first - b.first, a.second - b.second) < 1e-6)
			pts.pop_back();
	}
}

}
