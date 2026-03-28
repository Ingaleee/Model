#pragma once

#ifndef __AFXWIN_H__
	#error "включить pch.h до включения этого файла в PCH"
#endif

#include "resource.h"

class CCouplingAssemblyApp : public CWinApp
{
public:
	CCouplingAssemblyApp() noexcept;

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CCouplingAssemblyApp theApp;
