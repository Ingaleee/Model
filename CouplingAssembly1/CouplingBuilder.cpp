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
		CString msg = L"КОМПАС-3D: детали и сборка созданы.";
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
	else
		AfxMessageBox(
			L"КОМПАС-3D: не удалось подключиться или построить модель.",
			MB_OK | MB_ICONWARNING);

	return true;
}
