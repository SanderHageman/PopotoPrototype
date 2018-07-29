#pragma once
// ----------------------------
// ----Internal includes----
// ----------------------------
#include "InputClass.h"
#include "GraphicsClass.h"

// ----------------------------
// ----Class Definition----
// ----------------------------
class SystemClass {
public:
	SystemClass(LPCWSTR AppName, HINSTANCE hInst);

	int Run();

	LRESULT CALLBACK MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);


	// Delete functions
	SystemClass(SystemClass const& rhs) = delete;
	SystemClass& operator=(SystemClass const& rhs) = delete;

	SystemClass(SystemClass&& rhs) = delete;
	SystemClass& operator=(SystemClass&& rhs) = delete;

private:
	void Tick();
	void Update();
	void InitializeWindows();

private:
	LPCWSTR m_applicationName{};
	HINSTANCE m_hInstance{};
	HWND m_hWnd{};
	
	std::unique_ptr<InputClass> m_Input = std::make_unique<InputClass>();
	std::unique_ptr<GraphicsClass> m_Graphics;

	DX::StepTimer m_StepTimer{};

private:
	float m_CurrentPitch{ -0.299999f };
	float m_CurrentHeading{ 1.34177f };
	
	float m_moveSpeed{};
	float m_lookSpeed{};

	UINT m_ScreenWidth;
	UINT m_ScreenHeight;
};