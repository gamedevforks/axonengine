#include "dx9_private.h"

AX_BEGIN_NAMESPACE

inline bool trTexFormat(TexFormat texformat, D3DFORMAT &d3dformat)
{
	d3dformat = D3DFMT_UNKNOWN;

	switch (texformat) {
	case TexFormat::NULLTARGET:
		d3dformat = (D3DFORMAT)MAKEFOURCC('N','U','L','L');
		break;

	case TexFormat::R5G6B5:
		d3dformat = D3DFMT_R5G6B5;
		break;

	case TexFormat::RGB10A2:
		d3dformat = D3DFMT_A2R10G10B10;
		break;

	case TexFormat::RG16:
		d3dformat = D3DFMT_G16R16;
		break;

	case TexFormat::L8:
		d3dformat = D3DFMT_L8;
		break;

	case TexFormat::LA8:
		d3dformat = D3DFMT_A8L8;
		break;

	case TexFormat::A8:
		d3dformat = D3DFMT_A8;
		break;

	case TexFormat::BGR8:
		d3dformat = D3DFMT_R8G8B8;
		break;

	case TexFormat::BGRA8:
		d3dformat = D3DFMT_A8R8G8B8;
		break;

	case TexFormat::BGRX8:
		d3dformat = D3DFMT_X8R8G8B8;
		break;

	case TexFormat::DXT1:
		d3dformat = D3DFMT_DXT1;
		break;

	case TexFormat::DXT3:
		d3dformat = D3DFMT_DXT3;
		break;

	case TexFormat::DXT5:
		d3dformat = D3DFMT_DXT5;
		break;

	case TexFormat::L16:
		d3dformat = D3DFMT_L16;
		break;


		// 16 bits float texture
	case TexFormat::R16F:
		d3dformat = D3DFMT_R16F;
		break;

	case TexFormat::RG16F:
		d3dformat = D3DFMT_G16R16F;
		break;

	case TexFormat::RGB16F:
		//			d3dformat = GL_RGB16F;
		break;

	case TexFormat::RGBA16F:
		d3dformat = D3DFMT_A16B16G16R16F;
		break;


		// 32 bits float texture
	case TexFormat::R32F:
		d3dformat = D3DFMT_R32F;
		break;

	case TexFormat::RG32F:
		d3dformat = D3DFMT_G32R32F;
		break;

	case TexFormat::RGB32F:
		//			d3dformat = D3DFMT_A32B32G32R32F;
		break;

	case TexFormat::RGBA32F:
		d3dformat = D3DFMT_A32B32G32R32F;
		break;

	case TexFormat::D16:
		d3dformat = D3DFMT_D16;
		break;

	case TexFormat::D24:
		d3dformat = D3DFMT_D24X8;
		break;

	case TexFormat::D32:
		d3dformat = D3DFMT_D32;
		break;

	case TexFormat::D24S8:
		d3dformat = D3DFMT_D24S8;
		break;

	case TexFormat::DF16:
		d3dformat = (D3DFORMAT)MAKEFOURCC('D','F','1','6');
		break;

	case TexFormat::DF24:
		d3dformat = (D3DFORMAT)MAKEFOURCC('D','F','2','4');
		break;

	case TexFormat::RAWZ:
		d3dformat = (D3DFORMAT)MAKEFOURCC('R','A','W','Z');
		break;

	case TexFormat::INTZ:
		d3dformat = (D3DFORMAT)MAKEFOURCC('I','N','T','Z');
		break;

	default:
		Errorf("trTexFormat: bad enum");
	}

	if (d3dformat == D3DFMT_UNKNOWN) {
		return false;
	}

	return true;
}

bool CheckIfSupportHardwareMipmapGeneration(D3DFORMAT d3dformat, DWORD d3dusage)
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

void dx9CreateTexture2D(phandle_t h, TexFormat format, int width, int height, int flags)
{
	bool mipmap = false;
	D3DFORMAT d3dformat;
	trTexFormat(format, d3dformat);

	if (d3dformat == D3DFMT_UNKNOWN) {
		Errorf("Direct3D don't support texture format '%s'", format.toString());
	}

	D3DPOOL d3dpool = D3DPOOL_MANAGED;
	DWORD d3dusage = 0;

	if (flags & Texture::IF_RenderTarget) {
		d3dpool = D3DPOOL_DEFAULT;
		if (format.isDepth()) {
			d3dusage = D3DUSAGE_DEPTHSTENCIL;
		} else {
			d3dusage = D3DUSAGE_RENDERTARGET;
		}
	}

	if (flags & Texture::IF_AutoGenMipmap) {
		d3dusage |= D3DUSAGE_AUTOGENMIPMAP;

		bool m_hardwareGenMipmap = CheckIfSupportHardwareMipmapGeneration(d3dformat, d3dusage);

		if (!m_hardwareGenMipmap) {
			d3dusage &= ~D3DUSAGE_AUTOGENMIPMAP;
		}
	}

	int m_videoMemoryUsed = format.calculateDataSize(width, height);

	V(d3d9Device->CreateTexture(width, height, 1, d3dusage, d3dformat, d3dpool, (LPDIRECT3DTEXTURE9*)h, 0));

	stat_textureMemory.add(m_videoMemoryUsed);
}

void dx9UploadTexture(phandle_t h, int level, const void *pixels, TexFormat format)
{
	LPDIRECT3DTEXTURE9 obj = h->to<LPDIRECT3DTEXTURE9>();
	LPDIRECT3DSURFACE9 surface;

	V(obj->GetSurfaceLevel(level, &surface));
	D3DSURFACE_DESC surfdesc;
	obj->GetLevelDesc(0, &surfdesc);

	D3DFORMAT d3dformat = D3DFMT_UNKNOWN;
	if (format == TexFormat::AUTO) {
		d3dformat = surfdesc.Format;
	} else {
		trTexFormat(format, d3dformat);
	}

	if (d3dformat == D3DFMT_UNKNOWN)
		return;

	RECT srcrect;
	srcrect.left = 0; srcrect.top = 0; srcrect.right = surfdesc.Width; srcrect.bottom = surfdesc.Height;
	UINT srcpitch = format.calculateDataSize(surfdesc.Width, 1);

	V(D3DXLoadSurfaceFromMemory(surface, 0, 0, pixels, d3dformat, srcpitch, 0, &srcrect, 0, 0));
	SAFE_RELEASE(surface);
}

void dx9UploadSubTexture(phandle_t h, const Rect &rect, const void *pixels, TexFormat format)
{
	if (rect.isEmpty())
		return;

	if (!format)
		return;

	D3DFORMAT d3dformat;
	trTexFormat(format, d3dformat);

	if (d3dformat == D3DFMT_UNKNOWN) {
		return;
	}

	LPDIRECT3DSURFACE9 surface;
	LPDIRECT3DTEXTURE9 obj = h->to<LPDIRECT3DTEXTURE9>();
	V(obj->GetSurfaceLevel(0, &surface));

	RECT d3drect;
	d3drect.left = rect.x; d3drect.top = rect.y; d3drect.right = rect.xMax(); d3drect.bottom = rect.yMax();

	RECT d3dsrcrect;
	d3dsrcrect.left = 0; d3dsrcrect.top = 0; d3dsrcrect.right = rect.width; d3dsrcrect.bottom = rect.height;

	UINT srcpitch = format.calculateDataSize(rect.width, 1);
	DWORD d3dfilter = D3DX_FILTER_NONE;

	V(D3DXLoadSurfaceFromMemory(surface, 0, &d3drect, pixels, d3dformat, srcpitch, 0, &d3dsrcrect, d3dfilter, 0));

	SAFE_RELEASE(surface);
}

void dx9GenerateMipmap(phandle_t h)
{
	LPDIRECT3DTEXTURE9 obj = h->to<LPDIRECT3DTEXTURE9>();
	LPDIRECT3DSURFACE9 surface;

	V(obj->GetSurfaceLevel(0, &surface));
	D3DSURFACE_DESC surfdesc;
	obj->GetLevelDesc(0, &surfdesc);
	bool hardwaremipmap = CheckIfSupportHardwareMipmapGeneration(surfdesc.Format, surfdesc.Usage);

	if (hardwaremipmap) {
		obj->GenerateMipSubLevels();
		return;
	}

	// software convert.
	// convert to BGRA8 first, then use hardware mipmap generation, then convert back
	if (surfdesc.Format < D3DFMT_DXT1 || surfdesc.Format > D3DFMT_DXT5) {
		// currently, only support dxtc
		return;
	}

	D3DLOCKED_RECT lockedRect;
	V(obj->LockRect(0, &lockedRect, 0, 0));
	V(obj->UnlockRect(0));

	Handle dum;
	dx9CreateTexture2D(&dum, TexFormat::BGRA8, surfdesc.Width, surfdesc.Height, Texture::IF_AutoGenMipmap);
	dx9UploadTexture(&dum, 0, lockedRect.pBits, surfdesc.Format);

	LPDIRECT3DTEXTURE9 dummyobj = dum.to<LPDIRECT3DTEXTURE9>();

	V(dummyobj->LockRect(0, &lockedRect, 0, 0));
	Image image;
	image.initImage(TexFormat::BGRA8, surfdesc.Width, surfdesc.Height);
	image.setData(0, lockedRect.pBits, image.getDataSize(0));
	V(dummyobj->UnlockRect(0));

	image.generateMipmaps();

	int width = surfdesc.Width;
	int height = surfdesc.Height;
	for (DWORD i = 0; i < obj->GetLevelCount(); i++) {
		if (i >= (DWORD)image.getNumMipmapLevels()) {
			break;
		}
		dx9UploadTexture(h, i, image.getData(i), TexFormat::BGR8);

		width >>= 1;
		height >>= 1;
		if (width < 1) width = 1;
		if (height < 1) height = 1;
	}

	dx9DeleteTexture2D(&dum);

}

void dx9DeleteTexture2D(phandle_t h)
{
	LPDIRECT3DTEXTURE9 obj = h->to<LPDIRECT3DTEXTURE9>();

	SAFE_RELEASE(obj);
}

void dx9CreateVertexBuffer(phandle_t h, int datasize, Primitive::Hint hint)
{
	DWORD d3dusage = D3DUSAGE_WRITEONLY;
	D3DPOOL d3dpool = D3DPOOL_DEFAULT;

	if (hint != Primitive::HintStatic) {
		d3dusage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
	}

	V(d3d9Device->CreateVertexBuffer(datasize, d3dusage, 0, d3dpool, (IDirect3DVertexBuffer9 **)h, 0));

	stat_numVertexBuffers.inc();
	stat_vertexBufferMemory.add(datasize);
}

void dx9UploadVertexBuffer(phandle_t h, int datasize, void *p)
{
	IDirect3DVertexBuffer9 *obj = h->to<IDirect3DVertexBuffer9 *>();

	void *dst = 0;
	V(obj->Lock(0, datasize, &dst, D3DLOCK_DISCARD));
	memcpy(dst, p, datasize);
	V(obj->Unlock());
}

void dx9DeleteVertexBuffer(phandle_t h)
{
	IDirect3DVertexBuffer9 *obj = h->to<IDirect3DVertexBuffer9 *>();

	SAFE_RELEASE(obj);
}

void dx9CreateIndexBuffer(phandle_t h, int datasize, Primitive::Hint hint)
{
	DWORD d3dusage = D3DUSAGE_WRITEONLY;
	D3DPOOL d3dpool = D3DPOOL_DEFAULT;

	if (hint != Primitive::HintStatic) {
		d3dusage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
	}

	V(d3d9Device->CreateIndexBuffer(datasize, d3dusage, D3DFMT_INDEX16, d3dpool, (IDirect3DIndexBuffer9 **)h, 0));

	stat_numIndexBuffers.inc();
	stat_indexBufferMemory.add(datasize);
}

void dx9UploadIndexBuffer(phandle_t h, int datasize, void *p)
{
	IDirect3DIndexBuffer9 *obj = h->to<IDirect3DIndexBuffer9 *>();

	void *dst = 0;
	V(obj->Lock(0, datasize, &dst, D3DLOCK_DISCARD));
	memcpy(dst, p, datasize);
	V(obj->Unlock());
}

void dx9DeleteIndexBuffer(phandle_t h)
{
	IDirect3DIndexBuffer9 *obj = h->to<IDirect3DIndexBuffer9 *>();

	SAFE_RELEASE(obj);
}

void dx9CreateWindowTarget(phandle_t h, Handle hwnd)
{
	DX9_Window *window = new DX9_Window(hwnd);
	*h = window;
}

void dx9DeleteWindowTarget(phandle_t h)
{
	DX9_Window *window = handle_cast<DX9_Window*>(*h);
	SAFE_RELEASE(window);
}

void dx9CreateSamplerState( phandle_t h, const SamplerStateDesc &desc )
{
	*h = new DX9_SamplerState(desc);
}

void dx9DeleteSamplerState( phandle_t h )
{
	DX9_SamplerState *obj = handle_cast<DX9_SamplerState*>(*h);
	SAFE_RELEASE(obj);
}

void dx9CreateBlendState( phandle_t h, const BlendStateDesc &src )
{
	IDirect3DStateBlock9 *stateblock;
	d3d9Device->BeginStateBlock();
	d3d9Device->EndStateBlock(&stateblock);

	*h = stateblock;
}

void dx9DeleteBlendState( phandle_t h )
{
	IDirect3DStateBlock9 *stateblock = handle_cast<IDirect3DStateBlock9 *>(*h);
	SAFE_RELEASE(stateblock);
}

void dx9CreateDepthStencilState( phandle_t h, const DepthStencilStateDesc &src )
{
	IDirect3DStateBlock9 *stateblock;
	d3d9Device->BeginStateBlock();
	d3d9Device->EndStateBlock(&stateblock);

	*h = stateblock;
}

void dx9DeleteDepthStencilState( phandle_t h )
{
	IDirect3DStateBlock9 *stateblock = handle_cast<IDirect3DStateBlock9 *>(*h);
	SAFE_RELEASE(stateblock);
}

void dx9CreateRasterizerState( phandle_t h, const RasterizerStateDesc &src )
{
	IDirect3DStateBlock9 *stateblock;
	d3d9Device->BeginStateBlock();
	d3d9Device->EndStateBlock(&stateblock);

	*h = stateblock;
}

void dx9DeleteRasterizerState( phandle_t h )
{
	IDirect3DStateBlock9 *stateblock = handle_cast<IDirect3DStateBlock9 *>(*h);
	SAFE_RELEASE(stateblock);
}

AX_END_NAMESPACE