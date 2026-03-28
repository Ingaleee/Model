#include "pch.h"
#include "KompasConnect.h"

#if COUPLING_USE_KOMPAS_SDK

#include "KompasApi.h"
#include <wrl/client.h>

using namespace KompasAPI;
using Microsoft::WRL::ComPtr;

static ComPtr<KompasObject> g_pKompasApp5;

bool IsKompasConnected()
{
	return g_pKompasApp5.Get() != nullptr;
}

bool TryConnectKompas3D(CString* pErrorMessage)
{
	auto fail = [&](LPCWSTR msg) -> bool {
		if (pErrorMessage != nullptr)
			*pErrorMessage = msg;
		else
			AfxMessageBox(msg, MB_ICONWARNING);
		return false;
	};

	if (g_pKompasApp5.Get() != nullptr)
		return true;

	CLSID clsid{};
	HRESULT hr = CLSIDFromProgID(L"KOMPAS.Application.5", &clsid);
	if (FAILED(hr))
		return fail(L"Не найден CLSID Компаса");

	IUnknown* pUnkRaw = nullptr;
	hr = GetActiveObject(clsid, nullptr, &pUnkRaw);
	if (FAILED(hr))
	{
		hr = CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER, IID_IUnknown, reinterpret_cast<void**>(&pUnkRaw));
		if (FAILED(hr))
			return fail(L"Не удалось запустить Компас-3D");
	}
	ComPtr<IUnknown> pUnk;
	pUnk.Attach(pUnkRaw);

	KompasObject* pKompasRaw = nullptr;
	hr = pUnk->QueryInterface(__uuidof(KompasObject), reinterpret_cast<void**>(&pKompasRaw));
	if (FAILED(hr))
		return fail(L"Не удалось получить интерфейс KompasObject");

	g_pKompasApp5.Attach(pKompasRaw);
	g_pKompasApp5->Visible = VARIANT_TRUE;

	return true;
}

KompasObject* Coupling_GetKompasApp()
{
	return g_pKompasApp5.Get();
}

#else

bool IsKompasConnected()
{
	return false;
}

bool TryConnectKompas3D(CString* pErrorMessage)
{
	const wchar_t* msg =
		L"Интеграция с КОМПАС-3D отключена в сборке (COUPLING_USE_KOMPAS_SDK=0).\n"
		L"Чтобы включить: установите КОМПАС с SDK, укажите KompasSdkRoot в CouplingAssembly1.vcxproj "
		L"или переменную среды KOMPASSDKROOT (папка …\\SDK), затем в проекте задайте COUPLING_USE_KOMPAS_SDK=1 "
		L"и добавьте в каталоги включения …\\SDK\\Include и …\\Bin (для kAPI5.tlb).";

	if (pErrorMessage != nullptr)
		*pErrorMessage = msg;
	else
		AfxMessageBox(msg, MB_ICONINFORMATION);

	return false;
}

#endif
