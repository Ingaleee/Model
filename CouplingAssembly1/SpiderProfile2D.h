#pragma once

#include <utility>
#include <vector>

namespace SpiderProfile2D
{

bool ParallelFlankInnerHit(
	double ox,
	double oy,
	double dx,
	double dy,
	double Ri,
	double* ix,
	double* iy);

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
	double omy[8]);

void FlankFilletTargetAnglesDeg(
	int n,
	double Ro,
	double Ri,
	double legWidthB,
	double* outDeg,
	int outCap);

void AppendClosedContourMm(
	std::vector<std::pair<double, double>>& pts,
	int n,
	double Ro,
	double Ri,
	double legWidthB,
	int arcSegPerCap,
	int innerRiArcSegPerValley = 0);

}
