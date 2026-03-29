#pragma once

#include "ModelParams.h"

namespace GostTables
{

double SnapTorqueToSeries(double torqueNm);

int ClampExecution(int execution);

int ExecutionFromCourseVariant(int courseVariant);

void ApplyCourseVariantRule(
	AssemblyParams& assembly,
	HalfCouplingParams& half1,
	HalfCouplingParams& half2,
	SpiderParams& spider);

void ApplyAssemblyToParts(
	AssemblyParams& assembly,
	HalfCouplingParams& half1,
	HalfCouplingParams& half2,
	SpiderParams& spider);

void FillAssemblyTable2131(AssemblyParams& assembly);

bool LookupHalfFromGost(const AssemblyParams& assembly, HalfCouplingParams& half, double shaftDiameterMm);

void LookupSpiderFromGost(const AssemblyParams& assembly, SpiderParams& spider);

}
