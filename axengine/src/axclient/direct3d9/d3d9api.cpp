#include "d3d9private.h"

AX_BEGIN_NAMESPACE

extern bool trTexFormat(TexFormat texformat, D3DFORMAT &d3dformat);

static bool CheckIfSupportHardwareMipmapGeneration(D3DFORMAT d3dformat, DWORD d3dusage)
{
	if (d3d9Api->CheckDeviceFormat(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		D3DFMT_X8R8G8B8,
		d3dusage | D3DUSAGE_AUTOGENMIPMAP,
		D3DRTYPE_TEXTURE,
		d3dformat) == S_OK)
	{
		return true;
	}

	return false;
}

Handle D3D9_Api::createTexture2D(TexFormat format, int width, int height, int flags /*= 0*/)
{
	D3DFORMAT d3dformat;
	D3DPOOL d3dpool = D3DPOOL_MANAGED;
	DWORD d3dusage = 0;
	UINT d3dlevels = 0;

	trTexFormat(format, d3dformat);

	if (d3dformat == D3DFMT_UNKNOWN) {
		Errorf("Direct3D don't support texture format '%s'", format.toString());
	}

	if (flags & Texture2::IF_NoMipmap)
		d3dlevels = 1;

	if (flags & Texture2::IF_RenderTarget) {
		d3dpool = D3DPOOL_DEFAULT;
		if (format.isDepth()) {
			d3dusage = D3DUSAGE_DEPTHSTENCIL;
		} else {
			d3dusage = D3DUSAGE_RENDERTARGET;
		}
	}

	if (flags & Texture2::IF_AutoGenMipmap) {
		d3dusage |= D3DUSAGE_AUTOGENMIPMAP;

		bool hardwareGenMipmap = CheckIfSupportHardwareMipmapGeneration(d3dformat, d3dusage);

		if (!hardwareGenMipmap) {
			d3dusage &= ~D3DUSAGE_AUTOGENMIPMAP;
		}
	}

	LPDIRECT3DTEXTURE9 result;

	HRESULT hr;
	V(d3d9Device->CreateTexture(width, height, d3dlevels, d3dusage, d3dformat, d3dpool, &result, 0));
	return Handle(result);
}

void D3D9_Api::uploadTexture(Handle htex, int level, void *pixels, TexFormat format /*= TexFormat::AUTO*/)
{
	LPDIRECT3DTEXTURE9 tex = htex.to<LPDIRECT3DTEXTURE9>();
}

void D3D9_Api::uploadSubTexture(Handle htex, const Rect &rect, const void *pixels, TexFormat format /*= TexFormat::AUTO*/)
{

}

void D3D9_Api::generateMipmap(Handle htex)
{

}

void D3D9_Api::deleteTexture2D(Handle htex)
{
	LPDIRECT3DTEXTURE9 tex = htex.to<LPDIRECT3DTEXTURE9>();
	SAFE_RELEASE(tex);
}

AX_END_NAMESPACE
