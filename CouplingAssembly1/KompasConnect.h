#pragma once

#include <afxstr.h>

bool IsKompasConnected();
bool TryConnectKompas3D(CString* pErrorMessage = nullptr);
