#pragma once

// ----------------------------
// ----Preprocessing directives----
// ----------------------------
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#pragma warning( disable : 4324 ) // Disable D3DX12.h warning about alignment

// ----------------------------
// ----External Includes----
// ----------------------------
#include <Windows.h>

#include "d3dx12.h"
#include <dxgi1_6.h>
#include "VectorMath.h"
#include <D3Dcompiler.h>

#include <memory>
#include <cassert>
#include <wrl.h>
#include <array>
#include <vector>

#include "Utility.h"
#include "StepTimer.h"

//#include "CompiledShaders/PopotoVertexShader.h"
//#include "CompiledShaders/PopotoPixelShader.h"
//#include "CompiledShaders/ShadowMapVertexShader.h"
//#include "CompiledShaders/ShadowMapPixelShader.h"
