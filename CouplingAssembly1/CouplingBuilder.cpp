#include "pch.h"
#include "framework.h"
#include "CouplingBuilder.h"
#include "CouplingAssembly1Doc.h"
#include "CouplingKompasBuild.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool CCouplingBuilder::Build(const CCouplingAssembly1Doc& doc) const
{
	CString kompasErr;
	if (CouplingBuildInKompas(doc, &kompasErr))
	{
		const CStringW spiderPath = CouplingKompasOutputDirectory() + L"Звёздочка.m3d";
		CString msg;
		msg.Format(L"КОМПАС: записано.\n\nОткрыть в КОМПАСе:\n%s", static_cast<LPCWSTR>(spiderPath));
		if (!kompasErr.IsEmpty())
		{
			msg += L"\n\n";
			msg += kompasErr;
		}
		UINT ic = MB_ICONINFORMATION;
		if (kompasErr.Find(L"не создан") >= 0 || kompasErr.Find(L"не выполнено") >= 0 ||
			kompasErr.Find(L"SetSketch") >= 0 || kompasErr.Find(L"исключение COM") >= 0)
			ic = MB_ICONWARNING;
		AfxMessageBox(msg, MB_OK | ic);
		return true;
	}

	if (!kompasErr.IsEmpty())
		AfxMessageBox(kompasErr, MB_OK | MB_ICONWARNING);
	else
		AfxMessageBox(
			L"КОМПАС-3D: не удалось подключиться или построить модель.",
			MB_OK | MB_ICONWARNING);

	return false;
}
