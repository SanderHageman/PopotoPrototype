#pragma once
#include "D3DClass.h"

// ----------------------------
// ----Class definition----
// ----------------------------
class InputClass;
class GraphicsClass {
public:
	GraphicsClass(int sWidth, int sHeight, HWND hWnd);
	void Render();

	// Delete functions
	GraphicsClass(GraphicsClass const& rhs) = delete;
	GraphicsClass& operator=(GraphicsClass const& rhs) = delete;

	GraphicsClass(GraphicsClass&& rhs) = delete;
	GraphicsClass& operator=(GraphicsClass&& rhs) = delete;

	std::unique_ptr<D3DClass> m_Direct3D;
};