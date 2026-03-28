#pragma once

struct AssemblyParams
{
	double torque;
	int execution;
	double shaftDiameter1;
	double shaftDiameter2;

	double assemblyLengthL;
	double envelopeD1;
	double maxSpeedRpm;
	double massKg;
	double widthB1;

	AssemblyParams()
		: torque(16.0),
		  execution(2),
		  shaftDiameter1(16.0),
		  shaftDiameter2(16.0),
		  assemblyLengthL(81.0),
		  envelopeD1(50.0),
		  maxSpeedRpm(4500.0),
		  massKg(0.6),
		  widthB1(5.0)
	{
	}
};

struct HalfCouplingParams
{
	int gostTableId;
	int lugCount;
	double boreDiameter;
	double keywayWidthB;
	double keywayDt1;
	double hubOuterD1;
	double outerDiameter;
	double lengthHubL;
	double lengthTotalL1;
	double lengthL2;
	double lengthL3;
	double faceSlotB;
	double faceSlotB1;
	double filletR;
	double shoulderRadiusR;

	double hubDiameter;
	double length;
	double jawWidth;
	double filletRadius;

	void SyncLegacyAliases()
	{
		hubDiameter = hubOuterD1;
		length = lengthTotalL1;
		jawWidth = faceSlotB;
		filletRadius = filletR;
	}

	HalfCouplingParams()
		: gostTableId(2),
		  lugCount(3),
		  boreDiameter(16.0),
		  keywayWidthB(5.0),
		  keywayDt1(18.3),
		  hubOuterD1(26.0),
		  outerDiameter(53.0),
		  lengthHubL(28.0),
		  lengthTotalL1(58.0),
		  lengthL2(28.0),
		  lengthL3(15.0),
		  faceSlotB(5.0),
		  faceSlotB1(14.0),
		  filletR(0.1),
		  shoulderRadiusR(2.0),
		  hubDiameter(26.0),
		  length(58.0),
		  jawWidth(5.0),
		  filletRadius(0.1)
	{
	}
};

struct SpiderParams
{
	double outerDiameter;
	double thickness;
	int rays;
	double innerDiameter;
	double legWidth;
	double filletRadius;
	double massKg;

	SpiderParams()
		: outerDiameter(50.0),
		  thickness(15.0),
		  rays(6),
		  innerDiameter(26.0),
		  legWidth(10.5),
		  filletRadius(1.6),
		  massKg(0.032)
	{
	}
};
