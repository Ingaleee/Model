#include "pch.h"
#include "framework.h"
#include "CouplingBuilder.h"
#include "CouplingAssembly1Doc.h"
#include "CouplingKompasBuild.h"
#include "GostTables.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool CCouplingBuilder::Build(const CCouplingAssembly1Doc& doc) const
{
	CString kompasErr;
	if (CouplingBuildInKompas(doc, &kompasErr))
	{
		CString msg =
			L"\u0412 \u041a\u041e\u041c\u041f\u0410\u0421-3D \u0441\u043e\u0437\u0434\u0430\u043d\u044b \u0442\u0440\u0438 "
			L"\u043d\u043e\u0432\u044b\u0445 \u0434\u0435\u0442\u0430\u043b\u0438 (\u0442\u0440\u0438 \u0434\u043e\u043a\u0443\u043c\u0435\u043d\u0442\u0430):\n"
			L"\u2014 \u0437\u0432\u0451\u0437\u0434\u043e\u0447\u043a\u0430: \u0437\u0430\u043c\u043a\u043d\u0443\u0442\u044b\u0439 \u043a\u043e\u043d\u0442\u0443\u0440 "
			L"\u0438 \u0432\u044b\u0434\u0430\u0432\u043b\u0438\u0432\u0430\u043d\u0438\u0435 \u043d\u0430 \u0442\u043e\u043b\u0449\u0438\u043d\u0443 H;\n"
			L"\u2014 \u043a\u0430\u0436\u0434\u0430\u044f \u043f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430: \u043a\u043e\u043b\u044c\u0446\u043e \u043f\u043e\u0434 "
			L"\u0433\u0443\u0431\u043a\u0438 (D, d), \u0441\u0442\u0443\u043f\u0438\u0446\u0430 (d1), \u043a\u0440\u0443\u0433\u043e\u0432\u043e\u0439 "
			L"\u043c\u0430\u0441\u0441\u0438\u0432 \u0433\u0443\u0431\u043e\u043a (\u043f\u043e \u0447\u0438\u0441\u043b\u0443 \u043b\u0443\u0447\u0435\u0439 "
			L"\u0437\u0432\u0451\u0437\u0434\u043e\u0447\u043a\u0438), \u0448\u043f\u043e\u043d\u043e\u0447\u043d\u044b\u0439 \u043f\u0430\u0437 \u043d\u0430 "
			L"\u0441\u0442\u0443\u043f\u0438\u0446\u0435.\n\n"
			L"\u0421\u043e\u0445\u0440\u0430\u043d\u0438\u0442\u0435 \u0444\u0430\u0439\u043b\u044b \u0438 \u043f\u0440\u0438 \u043d\u0435\u043e\u0431\u0445\u043e\u0434\u0438\u043c\u043e\u0441\u0442\u0438 "
			L"\u0441\u043e\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0431\u043e\u0440\u043a\u0443 \u0432 \u041a\u041e\u041c\u041f\u0410\u0421\u0435.\n"
			L"\u0412 \u0441\u0431\u043e\u0440\u043a\u0435 \u043a\u043e\u043c\u043f\u043e\u043d\u0435\u043d\u0442\u044b \u0438\u043c\u0435\u043d\u043e\u0432\u0430\u043d\u044b: \u0417\u0432\u0451\u0437\u0434\u043e\u0447\u043a\u0430, \u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 1, \u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 2.";
		if (!kompasErr.IsEmpty())
		{
			msg += L"\n\n";
			msg += kompasErr;
		}
		AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
		return true;
	}

	if (!kompasErr.IsEmpty())
		AfxMessageBox(kompasErr, MB_OK | MB_ICONWARNING);

	CString text = MakeBuildMessage(doc);
	text +=
		L"\n\n\u041a\u041e\u041c\u041f\u0410\u0421-3D: \u043c\u043e\u0434\u0435\u043b\u044c \u043d\u0435 \u0441\u043e\u0437\u0434\u0430\u043d\u0430 "
		L"(\u0441\u043c. \u0441\u043e\u043e\u0431\u0449\u0435\u043d\u0438\u0435 \u0432\u044b\u0448\u0435, \u0435\u0441\u043b\u0438 \u0431\u044b\u043b\u043e). "
		L"\u0421\u043f\u0440\u0430\u0432\u0430 \u2014 \u0443\u0441\u043b\u043e\u0432\u043d\u044b\u0439 \u0432\u0438\u0434.";
	AfxMessageBox(text, MB_OK | MB_ICONINFORMATION);

	return true;
}

CString CCouplingBuilder::MakeBuildMessage(const CCouplingAssembly1Doc& doc) const
{
	const AssemblyParams& assembly = doc.GetAssemblyParams();
	const HalfCouplingParams& half1 = doc.GetHalfCoupling1Params();
	const HalfCouplingParams& half2 = doc.GetHalfCoupling2Params();
	const SpiderParams& spider = doc.GetSpiderParams();
	const double tRow = GostTables::SnapTorqueToSeries(assembly.torque);

	CString text;
	text.Format(
		L"\u0417\u0430\u0433\u043b\u0443\u0448\u043a\u0430 \u043f\u043e\u0441\u0442\u0440\u043e\u0435\u043d\u0438\u044f (\u0440\u0430\u0437\u043c\u0435\u0440\u044b \u043f\u043e \u0440\u044f\u0434\u0443 \u0413\u041e\u0421\u0422 14084-76).\n\n"
		L"\u0421\u0431\u043e\u0440\u043a\u0430 (\u0442\u0430\u0431\u043b. 21.3.1)\n"
		L"  \u041c\u043e\u043c\u0435\u043d\u0442 (\u0437\u0430\u0434\u0430\u043d\u043d\u044b\u0439): %.2f \u041d\u00b7\u043c\n"
		L"  \u0420\u044f\u0434 \u0442\u0430\u0431\u043b\u0438\u0446\u044b: %.1f \u041d\u00b7\u043c\n"
		L"  \u0418\u0441\u043f\u043e\u043b\u043d\u0435\u043d\u0438\u0435: %d\n"
		L"  \u0412\u0430\u043b 1 / \u0432\u0430\u043b 2: %.2f / %.2f \u043c\u043c\n"
		L"  L=%.2f \u043c\u043c  D\u2081=%.2f \u043c\u043c  n_max=%.0f \u043e\u0431/\u043c\u0438\u043d\n"
		L"  m=%.3f \u043a\u0433  b\u2081=%.2f \u043c\u043c\n\n"
		L"\u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 1 (\u0442\u0430\u0431\u043b. 1 \u0438\u043b\u0438 2)\n"
		L"  d=%.2f  D=%.2f  L\u2081=%.2f  l=%.2f\n"
		L"  d\u2081=%.2f  b=%.2f  d+t\u2081=%.2f  B=%.2f  B\u2081=%.2f\n"
		L"  l\u2082=%.2f  l\u2083=%.2f  r=%.2f  \u0433\u0443\u0431\u043a\u0438=%d\n\n"
		L"\u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 2\n"
		L"  d=%.2f  D=%.2f  L\u2081=%.2f  l=%.2f\n"
		L"  d\u2081=%.2f  b=%.2f  d+t\u2081=%.2f  B=%.2f  B\u2081=%.2f\n"
		L"  l\u2082=%.2f  l\u2083=%.2f  r=%.2f  \u0433\u0443\u0431\u043a\u0438=%d\n\n"
		L"\u0417\u0432\u0435\u0437\u0434\u043e\u0447\u043a\u0430\n"
		L"  D=%.2f  d=%.2f  H=%.2f\n"
		L"  B=%.2f  \u043b\u0443\u0447\u0435\u0439=%d  r=%.2f  m=%.3f \u043a\u0433",
		assembly.torque,
		tRow,
		assembly.execution,
		assembly.shaftDiameter1,
		assembly.shaftDiameter2,
		assembly.assemblyLengthL,
		assembly.envelopeD1,
		assembly.maxSpeedRpm,
		assembly.massKg,
		assembly.widthB1,
		half1.boreDiameter,
		half1.outerDiameter,
		half1.lengthTotalL1,
		half1.lengthHubL,
		half1.hubOuterD1,
		half1.keywayWidthB,
		half1.keywayDt1,
		half1.faceSlotB,
		half1.faceSlotB1,
		half1.lengthL2,
		half1.lengthL3,
		half1.filletR,
		half1.lugCount,
		half2.boreDiameter,
		half2.outerDiameter,
		half2.lengthTotalL1,
		half2.lengthHubL,
		half2.hubOuterD1,
		half2.keywayWidthB,
		half2.keywayDt1,
		half2.faceSlotB,
		half2.faceSlotB1,
		half2.lengthL2,
		half2.lengthL3,
		half2.filletR,
		half2.lugCount,
		spider.outerDiameter,
		spider.innerDiameter,
		spider.thickness,
		spider.legWidth,
		spider.rays,
		spider.filletRadius,
		spider.massKg);

	return text;
}
