#pragma once
#include <afxwin.h>

class CCouplingAssembly1Doc;

class CCouplingBuilder
{
public:
	bool Build(const CCouplingAssembly1Doc& doc) const;

private:
	CString MakeBuildMessage(const CCouplingAssembly1Doc& doc) const;
};