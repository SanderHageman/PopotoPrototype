#include "stdafx.h"
#include "GraphicsClass.h"


GraphicsClass::GraphicsClass(int sWidth, int sHeight, HWND hWnd) {
	m_Direct3D = std::make_unique<D3DClass>(sWidth, sHeight, 10.0f, 0.001f, true, hWnd);
}

void GraphicsClass::Render() {
	m_Direct3D->Render();
}