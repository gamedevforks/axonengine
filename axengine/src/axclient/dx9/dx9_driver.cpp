/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#include "dx9_private.h"


AX_BEGIN_NAMESPACE

AX_IMPLEMENT_FACTORY(DX9_Driver)

// console command
AX_BEGIN_COMMAND_MAP(DX9_Driver)
AX_END_COMMAND_MAP()

DX9_Driver::DX9_Driver()
{
	m_depthStencilSurfaceSize.set(-1,-1);
	m_nullSurfaceSize.set(-1,-1);
	m_depthStencilSurface = 0;
	m_nullSurface = 0;
}

DX9_Driver::~DX9_Driver()
{}

bool DX9_Driver::initialize()
{
	g_renderDriver = this;
	dx9_driver = this;

	dx9AssignRenderApi();

	Printf("..Initializing DX9_Driver...\n");

	dx9_internalWindow = new DX9_Window();
	g_renderDriverInfo.driverType = RenderDriverInfo::DX9;
	g_renderDriverInfo.vendor = "unknown";
	g_renderDriverInfo.renderer = "unknown";
	g_renderDriverInfo.version = "unknown";
	g_renderDriverInfo.extension = "unknown";

	Printf("...Calling Direct3DCreate9(D3D_SDK_VERSION = %d)...", D3D_SDK_VERSION);
	dx9_api = Direct3DCreate9(D3D_SDK_VERSION);

	if (!dx9_api) {
		Errorf("DX9_Driver::initialize: Direct3DCreate9 error\nMaybe you should install DirectX 9 or later version runtime");
		return false;
	}

	D3DCAPS9 caps;

	HRESULT hr = dx9_api->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	if (FAILED(hr)) {
		Errorf("DX9_Driver::initialize: GetDeviceCaps failed %s", DX9_ErrorString(hr));
	}

	g_renderDriverInfo.maxTextureSize = std::min(caps.MaxTextureWidth, caps.MaxTextureHeight);
	g_renderDriverInfo.max3DTextureSize = caps.MaxVolumeExtent;
	g_renderDriverInfo.maxCubeMapTextureSize = g_renderDriverInfo.maxTextureSize;

	g_renderDriverInfo.transposeMatrix = true;
	g_renderDriverInfo.support2DArray = false;
	g_renderDriverInfo.supportCubeArray = false;

	// Get the current desktop format
	D3DDISPLAYMODE dispmode;
	hr = dx9_api->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dispmode);
	if (FAILED(hr)) {
		Errorf("GetAdapterDisplayMode failed %s", DX9_ErrorString(hr));
	}

	// create d3d9 device
	D3DPRESENT_PARAMETERS d3dpp;
	memset(&d3dpp, 0, sizeof(d3dpp));
	d3dpp.BackBufferWidth = 0;
	d3dpp.BackBufferHeight = 0;
	d3dpp.BackBufferFormat = dispmode.Format;
	d3dpp.BackBufferCount = 1;
	d3dpp.EnableAutoDepthStencil= FALSE;
	d3dpp.AutoDepthStencilFormat= D3DFMT_D24S8;
	d3dpp.Flags = 0;
	d3dpp.Windowed = TRUE;
	d3dpp.hDeviceWindow = dx9_internalWindow->getHandle();
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	DWORD BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_PUREDEVICE;

	hr = dx9_api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dx9_internalWindow->getHandle(), BehaviorFlags, &d3dpp, &dx9_device);
	if (FAILED(hr)) {
		Errorf("CreateDevice failed %s", DX9_ErrorString(hr));
	}

	checkFormats();

	createVertexDecl();

	g_globalMacro.setMacro(GlobalMacro::G_D3D);
	dx9_shaderManager = new DX9_ShaderManager;
	dx9_stateManager = new DX9_StateManager;

	dx9_device->BeginScene();
	dx9InitState();

	Printf("ok\n");
	return true;
}

void DX9_Driver::finalize()
{}

void DX9_Driver::reset(int width, int height, bool fullscreen)
{
	onDeviceLost();
	onReset();
}

void DX9_Driver::onReset()
{
#if 0
	d3d9PrimitiveManager->onReset();
	d3d9TargetManager->onReset();
#endif
}

void DX9_Driver::onDeviceLost()
{
#if 0
	d3d9TargetManager->onDeviceLost();
	d3d9PrimitiveManager->onDeviceLost();
#endif
}

#if 0
bool D3D9Driver::isInRenderingThread()
{
	return d3d9Thread->isCurrentThread();
}

#endif

IDirect3DSurface9 *DX9_Driver::getDepthStencil(const Size &size)
{
	if (size <= m_depthStencilSurfaceSize)
		return m_depthStencilSurface;

	SAFE_RELEASE(m_depthStencilSurface);
	V(dx9_device->CreateDepthStencilSurface(size.width, size.height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &m_depthStencilSurface, 0));
	m_depthStencilSurfaceSize = size;
	return m_depthStencilSurface;
}

IDirect3DSurface9 *DX9_Driver::getNullTarget(const Size &size)
{
	if (size <= m_nullSurfaceSize)
		return m_nullSurface;

	SAFE_RELEASE(m_nullSurface);

	TexFormat nullformat = TexFormat::NULLTARGET;
	if (!g_renderDriverInfo.renderTargetFormatSupport[nullformat]) {
		AX_ASSERT(g_renderDriverInfo.renderTargetFormatSupport[TexFormat::R5G6B5]);
		nullformat = TexFormat::R5G6B5;
	}
	D3DFORMAT d3dfmt;
	AX_RELEASE_ASSERT(trTexFormat(nullformat, d3dfmt));
	V(dx9_device->CreateRenderTarget(size.width, size.height, d3dfmt, D3DMULTISAMPLE_NONE, 0, FALSE, &m_nullSurface, 0));
	m_nullSurfaceSize = size;
	return m_nullSurface;
}

const ShaderInfo * DX9_Driver::findShaderInfo(const FixedString &key)
{
	return dx9_shaderManager->findShaderInfo(key);
}

const ShaderInfo * DX9_Driver::findShaderInfo( const FixedString &key, const GlobalMacro &gm, const MaterialMacro &mm )
{
	return 0;
}


bool DX9_Driver::checkTextureFormatSupport(TexFormat format, D3DFORMAT d3dformat)
{
	HRESULT hr;
	if (format.isDepth() || format.isStencil()) {
		hr = dx9_api->CheckDeviceFormat(D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			D3DFMT_X8R8G8B8,
			0,
			D3DRTYPE_TEXTURE,
			d3dformat);
	} else {
		hr = dx9_api->CheckDeviceFormat(D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			D3DFMT_X8R8G8B8,
			0,
			D3DRTYPE_TEXTURE,
			d3dformat);
	}

	return SUCCEEDED(hr);
}

bool DX9_Driver::checkHardwareMipmapGenerationSupport(TexFormat format, D3DFORMAT d3dformat)
{
	if (dx9_api->CheckDeviceFormat(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		D3DFMT_X8R8G8B8,
		D3DUSAGE_AUTOGENMIPMAP,
		D3DRTYPE_TEXTURE,
		d3dformat) == S_OK)
	{
		return true;
	} else {
		return false;
	}
}


bool DX9_Driver::checkRenderTargetFormatSupport(TexFormat format, D3DFORMAT d3dformat)
{
	HRESULT hr;
	if (format.isDepth() || format.isStencil()) {
		hr = dx9_api->CheckDeviceFormat(D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			D3DFMT_X8R8G8B8,
			D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_TEXTURE,
			d3dformat);
	} else {
		hr = dx9_api->CheckDeviceFormat(D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			D3DFMT_X8R8G8B8,
			D3DUSAGE_RENDERTARGET,
			D3DRTYPE_TEXTURE,
			d3dformat);
	}

	return SUCCEEDED(hr);
}


void DX9_Driver::checkFormats()
{
	D3DFORMAT d3dformat;
	for (int i=0; i<TexFormat::MAX_NUMBER; i++) {
		trTexFormat((TexFormat::Type)i, d3dformat);
		g_renderDriverInfo.textureFormatSupports[i] = checkTextureFormatSupport((TexFormat::Type)i, d3dformat);
		g_renderDriverInfo.renderTargetFormatSupport[i] = checkRenderTargetFormatSupport((TexFormat::Type)i, d3dformat);
		g_renderDriverInfo.autogenMipmapSupports[i] = checkHardwareMipmapGenerationSupport((TexFormat::Type)i, d3dformat);
	}

	g_renderDriverInfo.suggestFormats[RenderDriverInfo::SuggestedFormat_DepthStencil] = TexFormat::D24S8;
	g_renderDriverInfo.suggestFormats[RenderDriverInfo::SuggestedFormat_DepthStencilTexture] = TexFormat::INTZ;
	g_renderDriverInfo.suggestFormats[RenderDriverInfo::SuggestedFormat_SceneColor] = TexFormat::BGRA8;
	g_renderDriverInfo.suggestFormats[RenderDriverInfo::SuggestedFormat_HdrLightBuffer] = TexFormat::RGBA16F;
	g_renderDriverInfo.suggestFormats[RenderDriverInfo::SuggestedFormat_HdrSceneColor] = TexFormat::RGBA16F;
	g_renderDriverInfo.suggestFormats[RenderDriverInfo::SuggestedFormat_NormalBuffer] = TexFormat::RGB10A2;
	g_renderDriverInfo.suggestFormats[RenderDriverInfo::SuggestedFormat_ShadowMap] = TexFormat::D24S8;
	g_renderDriverInfo.suggestFormats[RenderDriverInfo::SuggestedFormat_CubeShadowMap] = TexFormat::R32F;
}

namespace {
	D3DVERTEXELEMENT9 s_veSkin[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,	0},
		{0, 16,	D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		{0, 24, D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	1},
		{0, 40, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	2},
		{0, 52, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	3},
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 s_veMesh[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,	0},
		{0, 16,	D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		{0, 32, D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	1},
		{0, 48, D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	2},
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 s_veDebug[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,	0},
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 s_veBlend[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,	0},
		{0, 16,	D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 s_veChunk[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 s_veInstance[] = {
		{1, 0,	D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	4},
		{1, 16, D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 5},
		{1, 32, D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 6},
		{1, 48, D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 7},
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 *s_veInfos[] = {
		s_veSkin, s_veMesh, s_veDebug, s_veBlend, s_veChunk
	};
} // namespace

void DX9_Driver::createVertexDecl()
{
	for (int i = 0; i < VertexType::MaxType; i++) {
		D3DVERTEXELEMENT9 *veinfo = s_veInfos[i];

		// apply offset
		int c = 0;
		std::vector<D3DVERTEXELEMENT9> veiseq;

		for (D3DVERTEXELEMENT9 *ve = veinfo; ve->Stream != 0xff; ve++, c++) {
			veiseq.push_back(*ve);
		}

		// add instanced vert elem to veInstanced
		for (size_t j = 0; j < ArraySize(s_veInstance); j++) {
			veiseq.push_back(s_veInstance[j]);
		}

		V(dx9_device->CreateVertexDeclaration(veinfo, &dx9_vertexDeclarations[i]));
		V(dx9_device->CreateVertexDeclaration(&veiseq[0], &dx9_vertexDeclarationsInstanced[i]));
	}
}

AX_END_NAMESPACE

