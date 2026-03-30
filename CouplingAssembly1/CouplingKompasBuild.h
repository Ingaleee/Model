#pragma once

#include <afxstr.h>

class CCouplingAssembly1Doc;

bool CouplingBuildInKompas(const CCouplingAssembly1Doc& doc, CString* err = nullptr);

CStringW CouplingKompasOutputDirectory();
