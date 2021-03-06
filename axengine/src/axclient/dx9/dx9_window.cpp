/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#include "dx9_private.h"

static D3DMULTISAMPLE_TYPE d3d9MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
static DWORD d3d9MultiSampleQuality = 0;

AX_BEGIN_NAMESPACE

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


DX9_Window::DX9_Window()
{
	m_swapChain = 0;
	m_backbuffer = 0;
	m_swapChainWnd = 0;
	m_presentInterval = 0;
	m_swapChainSize.set(-1,-1);

	std::wstring ws = u2w("DX9_Window");
	WNDCLASSEXW wcex;	
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = (HINSTANCE)::GetModuleHandle(NULL);
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)WHITE_BRUSH;
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = ws.c_str();
	wcex.hIconSm = 0;

	if (!::RegisterClassExW(&wcex)) {
		Errorf("GLdriver::CreateGLWindow: cann't create OpenGL window");
		return;
	}

	DWORD dwStyle,dwExStyle;
	dwExStyle = 0;		
	dwStyle = WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;

	m_wndId = ::CreateWindowExW(dwExStyle
		, ws.c_str()
		, ws.c_str()
		, dwStyle
		, 0, 0, 640,480
		, NULL, NULL
		, (HINSTANCE)GetModuleHandleW(NULL)
		, NULL);

	m_updatedSize.set(640,480);
}

DX9_Window::DX9_Window(Handle wndId, int width, int height)
{
	m_swapChain = 0;
	m_backbuffer = 0;
	m_swapChainWnd = 0;
	m_presentInterval = 0;
	m_swapChainSize.set(-1,-1);

	m_wndId = (HWND)wndId.toVoidStar();
	m_updatedSize.set(width,height);
}

DX9_Window::~DX9_Window()
{
	SAFE_RELEASE(m_backbuffer);
	SAFE_RELEASE(m_swapChain);
}

// implement ITarget
Size DX9_Window::getSize()
{
	return m_swapChainSize;
}

void DX9_Window::present()
{
	HRESULT hr = m_swapChain->Present(0, 0, 0, 0, D3DPRESENT_DONOTWAIT);

	if (SUCCEEDED(hr)) {
		return;
	}

	if (hr == D3DERR_WASSTILLDRAWING) {
		return;
	}

	if (hr == D3DERR_DEVICELOST) {
		// TODO
		Errorf("D3D9 device lost. not even handle this yet");
		return;
	}

	if (hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY) {
		Errorf("D3D9 out of memory.");
		return;
	}

	if (FAILED(hr)) {
		Errorf("%s(%d): %s", __FILE__, __LINE__, DX9_ErrorString(hr));
	}
}

void DX9_Window::checkSwapChain()
{
	DWORD flags = D3DPRESENT_INTERVAL_IMMEDIATE;

	if (r_vsync.getBool())
		flags = D3DPRESENT_INTERVAL_DEFAULT;


	if (m_swapChain && m_swapChainSize == m_updatedSize && m_wndId == m_swapChainWnd && flags == m_presentInterval)
		return;

	SAFE_RELEASE(m_backbuffer);
	SAFE_RELEASE(m_swapChain);

	D3DPRESENT_PARAMETERS presentparams;
	memset(&presentparams, 0, sizeof(presentparams));
	presentparams.BackBufferWidth = m_updatedSize.width;
	presentparams.BackBufferHeight = m_updatedSize.height;
	presentparams.BackBufferFormat = D3DFMT_X8R8G8B8;
	presentparams.BackBufferCount = 1;
	presentparams.EnableAutoDepthStencil= FALSE;
	presentparams.AutoDepthStencilFormat= D3DFMT_D24S8;
	presentparams.Flags = 0;
	presentparams.Windowed = TRUE;
	presentparams.hDeviceWindow = m_wndId;
	presentparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentparams.PresentationInterval = flags;

	V(dx9_device->CreateAdditionalSwapChain(&presentparams, &m_swapChain));
	V(m_swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_backbuffer));

	m_swapChainSize = m_updatedSize;
	m_swapChainWnd = m_wndId;
	m_presentInterval = flags;
}

void DX9_Window::update(Handle newId, int width, int height)
{
	m_wndId = handle_cast<HWND>(newId);
	m_updatedSize = Size(width, height);
	checkSwapChain();
}

IDirect3DSurface9 * DX9_Window::getSurface()
{
	SAFE_ADDREF(m_backbuffer);
	return m_backbuffer;
}


AX_END_NAMESPACE

