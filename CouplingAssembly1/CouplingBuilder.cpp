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
			L"Готово к демонстрации: в КОМПАС-3D созданы три детали (три документа):\n"
			L"— звёздочка: замкнутый контур и выдавливание на толщину H;\n"
			L"— каждая полумуфта: кольцо под губки (D, d), ступица (d₁), круговой "
			L"массив губок (по числу лучей звёздочки), шпоночный паз на ступице.\n\n"
			L"Сохраните файлы и при необходимости соберите сборку в КОМПАСе.\n"
			L"В сборке компоненты именованы: Звёздочка, Полумуфта 1, Полумуфта 2.";
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
		L"\n\nКОМПАС-3D: модель не создана (см. сообщение выше, если было). "
		L"Справа — условный вид.";
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
		L"Заглушка построения (размеры по ряду ГОСТ 14084-76).\n\n"
		L"Сборка (табл. 21.3.1)\n"
		L"  Момент (заданный): %.2f Н·м\n"
		L"  Ряд таблицы: %.1f Н·м\n"
		L"  Исполнение ГОСТ: %d (%s)  |  вариант задания: %d\n"
		L"  Параметризация: варианты 2 и 5 → исп.1 и 4 луча; 1,3,4,6,7,8 → исп.2 и 6 лучей.\n"
		L"  Валы 1 и 2 (две полумуфты при одном исполнении): %.2f / %.2f мм\n"
		L"  L=%.2f мм  D₁=%.2f мм  n_max=%.0f об/мин\n"
		L"  m=%.3f кг  b₁=%.2f мм\n\n"
		L"Полумуфта 1 — вал 1 (табл. 1 или 2)\n"
		L"  d=%.2f  D=%.2f  L₁=%.2f  l=%.2f\n"
		L"  d₁=%.2f  b=%.2f  d+t₁=%.2f  B=%.2f  B₁=%.2f\n"
		L"  l₂=%.2f  l₃=%.2f  r=%.2f  губки=%d\n\n"
		L"Полумуфта 2 — вал 2\n"
		L"  d=%.2f  D=%.2f  L₁=%.2f  l=%.2f\n"
		L"  d₁=%.2f  b=%.2f  d+t₁=%.2f  B=%.2f  B₁=%.2f\n"
		L"  l₂=%.2f  l₃=%.2f  r=%.2f  губки=%d\n\n"
		L"Звёздочка\n"
		L"  D=%.2f  d=%.2f  H=%.2f\n"
		L"  B=%.2f  лучей=%d  r=%.2f  m=%.3f кг",
		assembly.torque,
		tRow,
		assembly.execution,
		(assembly.execution == 1) ? L"2 губки, звезда 4 луча" : L"3 губки, звезда 6 лучей",
		assembly.courseVariant,
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
