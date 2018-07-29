#include "stdafx.h"

// ----------------------------
// ----Internal Includes----
// ----------------------------
#include "SystemClass.h"
#include "InputClass.h"
#include "GraphicsClass.h"

// Window procedure globals
namespace FiltyGlobals {
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
		SystemClass* pSystemClassHandle;

		switch (umsg)
		{
		case WM_GETMINMAXINFO: {
			return DefWindowProc(hwnd, umsg, wparam, lparam);
		}
		case WM_NCCREATE: {
			// Store SystemClass pointer in the hwnd on creation
			pSystemClassHandle = static_cast<SystemClass*>(reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);
			if (!SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pSystemClassHandle))) {
				if (GetLastError() != 0) {
					return FALSE;
				}
			}
		}
		default: {
			pSystemClassHandle = reinterpret_cast<SystemClass*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}
		}

		if (!pSystemClassHandle) throw std::exception();
		return pSystemClassHandle->MessageHandler(hwnd, umsg, wparam, lparam);
	}
};

SystemClass::SystemClass(LPCWSTR AppName, HINSTANCE hInst) :
	m_applicationName(AppName),
	m_hInstance(hInst){

	// Initialize win api
	InitializeWindows();

	m_Graphics = std::make_unique<GraphicsClass>(m_ScreenWidth, m_ScreenHeight, m_hWnd);

	m_StepTimer.SetFixedTimeStep(true);
	m_StepTimer.SetTargetElapsedSeconds(1.0 / 60.0);

	m_moveSpeed = 0.35f;
	m_lookSpeed = 1.5f;
}

int SystemClass::Run() {
	MSG msg{};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// GameLoop
		Tick();
	}

	return static_cast<int>(msg.wParam);
}

void SystemClass::Tick() {
	auto updateLoop = [&] {
		Update();
	};

	m_StepTimer.Tick(updateLoop);
	m_Graphics->Render();
}

void SystemClass::Update() {
	if (m_Input->IsKeyDown(VK_ESCAPE)) {
		SendMessage(m_hWnd, WM_CLOSE, 0, 0);
	}

	float t_Dt = static_cast<float>(m_StepTimer.GetElapsedSeconds());

	if (m_Input->IsKeyDown(VK_F1)) {
		std::wstringstream t_SStream;
		t_SStream << "DeltaTime: " << t_Dt << std::endl;
		OutputDebugString(t_SStream.str().c_str());
	}

	auto& camera = m_Graphics->m_Direct3D->m_camera;

	// Update and get camera matrices
	{
		float strafe{}, ascent{}, forward{};
		float pitch{}, yaw{};

		//if (m_Input->IsKeyDown(VK_F2)) {
		//	m_moveSpeed += 0.2f * t_Dt;
		//}
		//if (m_Input->IsKeyDown(VK_F3)) {
		//	m_moveSpeed -= 0.2f * t_Dt;
		//}


		if (m_Input->IsKeyDown('W')) { // W
			forward += m_moveSpeed * t_Dt;
		}

		if (m_Input->IsKeyDown('S')) { // S
			forward -= m_moveSpeed * t_Dt;
		}

		if (m_Input->IsKeyDown('A')) { // A
			strafe -= (m_moveSpeed * 0.65f) * t_Dt;
		}

		if (m_Input->IsKeyDown('D')) { // D
			strafe += (m_moveSpeed * 0.65f) * t_Dt;
		}

		if (m_Input->IsKeyDown('Q') || m_Input->IsKeyDown(VK_SHIFT)) { // Q or shift
			ascent -= m_moveSpeed * t_Dt;
		}

		if (m_Input->IsKeyDown('E') || m_Input->IsKeyDown(VK_SPACE)) { // E or space
			ascent += m_moveSpeed * t_Dt;
		}


		if (m_Input->IsKeyDown(VK_LEFT)) {
			yaw -= m_lookSpeed * t_Dt;
		}

		if (m_Input->IsKeyDown(VK_RIGHT)) {
			yaw += m_lookSpeed * t_Dt;
		}

		if (m_Input->IsKeyDown(VK_UP)) {
			pitch += m_lookSpeed * t_Dt;
		}

		if (m_Input->IsKeyDown(VK_DOWN)) {
			pitch -= m_lookSpeed * t_Dt;
		}

		if (m_Input->IsKeyDown(VK_F5)) {
			std::wstringstream t_SStream;
			const auto& pos = camera->GetPosition();
			const auto& dir = camera->GetForward();
			t_SStream << "Current Pitch: " << m_CurrentPitch << "f, Current Heading: " << m_CurrentHeading << "f" << std::endl;
			t_SStream << "Current Position: " << "{" << pos.GetX() << "f, " << pos.GetY() << "f, " << pos.GetZ() << "f}" << std::endl;
			t_SStream << "Current Direction: " << "{" << dir.GetX() << "f, " << dir.GetY() << "f, " << dir.GetZ() << "f}" << std::endl;
			OutputDebugString(t_SStream.str().c_str());
		}

		m_CurrentPitch += pitch;
		m_CurrentPitch = XMMin(XM_PIDIV2, m_CurrentPitch);
		m_CurrentPitch = XMMax(-XM_PIDIV2, m_CurrentPitch);

		m_CurrentHeading -= yaw;
		if (m_CurrentHeading > XM_PI)
			m_CurrentHeading -= XM_2PI;
		else if (m_CurrentHeading <= -XM_PI)
			m_CurrentHeading += XM_2PI;

		Matrix3 orientation = Matrix3::MakeYRotation(m_CurrentHeading) * Matrix3::MakeXRotation(m_CurrentPitch);
		Vector3 position = orientation * Vector3(strafe, ascent, -forward) + camera->GetPosition();

		camera->SetTransform(AffineTransform(orientation, position));
	}


	// Update Cube 1
	{
		// Create and apply rotation
		//auto& c1 = m_Graphics->m_Direct3D->m_models[0];
		//auto rotation = Matrix3::MakeXRotation(0.001f) * Matrix3::MakeYRotation(0.002f) * Matrix3::MakeZRotation(0.003f);
		//c1.m_Transform.SetRotation(c1.m_Transform.GetRotation() * Quaternion(rotation));
	}
}

void SystemClass::InitializeWindows() {
	// Setup windows class
	WNDCLASSEX windowClass{};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = FiltyGlobals::WndProc;
	windowClass.hInstance = m_hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = m_applicationName;
	Utility::ThrowIfFailed(RegisterClassEx(&windowClass));

	LONG sWidth = 1600;
	LONG sHeight = 900;
	m_ScreenWidth = sWidth;
	m_ScreenHeight = sHeight;
	RECT windowRect{ 0,0,sWidth,sHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
	
	m_hWnd = CreateWindow(
		windowClass.lpszClassName,
		m_applicationName,
		WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		m_hInstance,
		this
	);

	ShowWindow(m_hWnd, SW_SHOW);

	return;
}

LRESULT CALLBACK SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
	switch (umsg)
	{
	case WM_KEYDOWN: {
		m_Input->KeyDown(static_cast<UINT8>(wparam));
		//std::wstringstream t_SStream;
		//t_SStream << "Key: " << static_cast<UINT8>(wparam) << std::endl;
		//OutputDebugString(t_SStream.str().c_str());
		return 0;
	}
	case WM_KEYUP: {
		m_Input->KeyUp(static_cast<UINT8>(wparam));
		return 0;
	}
	case WM_DESTROY:
	case WM_CLOSE: {
		// cleanup and things
		PostQuitMessage(0);
	}
	default: {
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}