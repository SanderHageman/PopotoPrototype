#include "stdafx.h"
#include "SystemClass.h"

int WINAPI WinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE /*hPrevInstance*/, __in PSTR /*pScmdline*/, __in int /*iCmdshow*/) {
#if defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	const auto system = std::make_unique<SystemClass>(L"Popoto Propoto", hInstance);
	return system->Run();
}