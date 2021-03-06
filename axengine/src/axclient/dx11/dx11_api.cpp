#include "dx11_private.h"

extern "C" {;
extern int WINAPI D3DPERF_BeginEvent(DWORD col, LPCWSTR wszName);
extern int WINAPI D3DPERF_EndEvent( void );
}

AX_DX11_BEGIN_NAMESPACE

namespace {
	enum {
		VERTEX_UP_SIZE = 128 * 1024,
		INDEX_UP_SIZE = 64 * 1024,
	};
}

FastParams dx11_curParams1;
FastParams dx11_curParams2;
phandle_t dx11_curGlobalTextures[GlobalTextureId::MaxType];
SamplerDesc dx11_curGlobalTextureSamplerDescs[GlobalTextureId::MaxType];
FastTextureParams dx11_curMaterialTextures;
bool dx11_curInstanced = false;
VertexType dx11_curVertexType;

static DX11_Shader *s_curShader;
static Technique s_curTechnique;

static phandle_t s_tempVertexBuffer;
static phandle_t s_tempIndexBuffer;
static int s_curVertexBufferPos;
static int s_curIndexBufferPos;

static int s_curNumVertices;
static int s_curNumIndices;
static int s_curNumInstances;
static int s_curPrimitiveCount;
static Size s_curRenderTargetSize;

ID3D11DepthStencilView *s_dsv;
ID3D11RenderTargetView *s_rtv[RenderTargetSet::MaxColorTarget];
int s_numView = 0;

void dx11CreateTextureFromFileInMemory(phandle_t h, IoRequest *asioRequest)
{
	ID3D11Resource *texture;
	V(D3DX11CreateTextureFromMemory(dx11_device, asioRequest->fileData(), asioRequest->fileSize(), 0, 0, &texture, 0));
	DX11_Resource *resource = new DX11_Resource(DX11_Resource::kImmutableTexture, texture);
	V(dx11_device->CreateShaderResourceView(texture, 0, &resource->m_shaderResourceView));
	*h = resource;
	delete asioRequest;
}

void dx11CreateTexture(phandle_t h, TexType type, TexFormat format, int width, int height, int depth, int flags)
{
	DXGI_FORMAT d3dformat = DX11_Driver::trTexFormat(format);
	AX_RELEASE_ASSERT(d3dformat != DXGI_FORMAT_UNKNOWN);
	AX_RELEASE_ASSERT(g_renderDriverInfo.textureFormatSupports[format]);

	D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
	UINT bindflags = D3D11_BIND_SHADER_RESOURCE;
	UINT miscflags = 0;
	UINT cpuAccessFlags = 0;
	int miplevels = 1;

	if (flags & Texture::RenderTarget) {
		AX_RELEASE_ASSERT(g_renderDriverInfo.renderTargetFormatSupport[format]);
		if (format.isDepth()) {
			bindflags |= D3D11_BIND_DEPTH_STENCIL;
			miscflags |= D3D11_RESOURCE_MISC_SHARED;
		} else {
			bindflags |= D3D11_BIND_RENDER_TARGET;
		}
	} else {
		//usage = D3D11_USAGE_DYNAMIC;
		//cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}

	if (flags & Texture::AutoGenMipmap) {
		bool m_hardwareGenMipmap = g_renderDriverInfo.autogenMipmapSupports[format] && (flags & Texture::RenderTarget);

		if (m_hardwareGenMipmap) {
			miscflags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		miplevels = 0;
	}

	int array_size = 1;
	if (type == TexType::CUBE) {
		miscflags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
		array_size = 6;
	}

	ID3D11Resource *d3dresource = 0;
	ID3D11Texture2D *texture2D = 0;
	ID3D11Texture3D *texture3D = 0;
	DX11_Resource *app_resource = 0;
	if (type == TexType::_2D || type == TexType::CUBE) {
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;//miplevels;
		desc.ArraySize = array_size;
		desc.Format = d3dformat;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = usage;
		desc.BindFlags = bindflags;
		desc.CPUAccessFlags = cpuAccessFlags;
		desc.MiscFlags = miscflags;

		V(dx11_device->CreateTexture2D(&desc, 0, &texture2D));

		d3dresource = texture2D;
		app_resource = new DX11_Resource(DX11_Resource::kDynamicTexture, texture2D);
	} else if (type == TexType::_3D) {
		D3D11_TEXTURE3D_DESC desc;
		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;
		desc.MipLevels = miplevels;
		desc.Format = d3dformat;
		desc.Usage = usage;
		desc.BindFlags = bindflags;
		desc.CPUAccessFlags = cpuAccessFlags;
		desc.MiscFlags = miscflags;

		V(dx11_device->CreateTexture3D(&desc, 0, &texture3D));
		d3dresource = texture3D;
		app_resource = new DX11_Resource(DX11_Resource::kDynamicTexture, texture3D);
	}

	app_resource->m_isDynamic = true;
	app_resource->m_dynamicTextureData->texFormat = format;
	app_resource->m_dynamicTextureData->texType = type;
	app_resource->m_dynamicTextureData->width = width;
	app_resource->m_dynamicTextureData->height = height;
	app_resource->m_dynamicTextureData->depth = depth;

	if (flags & Texture::RenderTarget) {
		if (format.isDepth()) {
			ID3D11DepthStencilView *view = 0;
			D3D11_DEPTH_STENCIL_VIEW_DESC desc = {
				DX11_Driver::trRenderTargetFormat(format),
				D3D11_DSV_DIMENSION_TEXTURE2D,
				0
			};
			if (array_size == 1) {
				V(dx11_device->CreateDepthStencilView(d3dresource, &desc, &view));
				app_resource->m_dynamicTextureData->m_depthStencilViews.push_back(view);
			} else {
				desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.MipSlice = 0;
				desc.Texture2DArray.FirstArraySlice = 0;
				desc.Texture1DArray.ArraySize = 1;
				for (int i = 0; i < array_size; i++) {
					desc.Texture2DArray.FirstArraySlice = i;
					V(dx11_device->CreateDepthStencilView(d3dresource, &desc, &view));
					app_resource->m_dynamicTextureData->m_depthStencilViews.push_back(view);
				}
			}
		} else {
			ID3D11RenderTargetView *view = 0;
			if (array_size == 1) {
				V(dx11_device->CreateRenderTargetView(d3dresource, 0, &view));
				app_resource->m_dynamicTextureData->m_renderTargetViews.push_back(view);
			} else {
				D3D11_RENDER_TARGET_VIEW_DESC desc = {
					DX11_Driver::trRenderTargetFormat(format), D3D11_RTV_DIMENSION_TEXTURE2DARRAY
				};
				desc.Texture2DArray.MipSlice = 0;
				desc.Texture2DArray.FirstArraySlice = 0;
				desc.Texture2DArray.ArraySize = 1;
				for (int i = 0; i < array_size; i++) {
					desc.Texture2DArray.FirstArraySlice = i;
					V(dx11_device->CreateRenderTargetView(d3dresource, &desc, &view));
					app_resource->m_dynamicTextureData->m_renderTargetViews.push_back(view);
				}
			}
		}
	}

	if (format.isDepth()) {
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		TypeZero(&desc);
		desc.Format = DX11_Driver::trShaderResourceViewFormat(format);
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = 1;
		V(dx11_device->CreateShaderResourceView(texture2D, &desc, &app_resource->m_shaderResourceView));
	} else {
		V(dx11_device->CreateShaderResourceView(d3dresource, 0, &app_resource->m_shaderResourceView));
	}
	*h = app_resource;
}

void dx11UploadTexture(phandle_t h, const void *pixels, TexFormat format)
{

}

void dx11UploadSubTexture(phandle_t h, const Rect &rect, const void *pixels, TexFormat format)
{
	DX11_Resource *apiResource = h->castTo<DX11_Resource *>();
	AX_RELEASE_ASSERT(apiResource->m_type == DX11_Resource::kDynamicTexture);
	AX_RELEASE_ASSERT(format == apiResource->m_dynamicTextureData->texFormat);

	D3D11_BOX box;
	box.left = rect.x;
	box.top = rect.y;
	box.right = rect.xMax();
	box.bottom = rect.yMax();
	box.front = 0;
	box.back = 1;
	UINT rowPitch = format.calculateDataSize(rect.width, 1);

	dx11_context->UpdateSubresource(apiResource->m_dx11Resource, 0, &box, pixels, rowPitch, 0);
}

void dx11GenerateMipmap(phandle_t h)
{
	// TODO
}

void dx11DeleteTexture(phandle_t h)
{
	DX11_Resource *apiResource = h->castTo<DX11_Resource *>();
	delete apiResource;
	delete h;
}

void dx11CreateVertexBuffer(phandle_t h, int datasize, Primitive::Hint hint, const void *p)
{
	bool is_dynamic = hint != Primitive::HintStatic;

	// Fill in a buffer description.
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.ByteWidth = datasize;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = p;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// Create the vertex buffer.
	ID3D11Buffer *buffer;
	if (is_dynamic) {
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		if (p) {
			dx11_device->CreateBuffer(&bufferDesc, &InitData, &buffer);
		} else {
			dx11_device->CreateBuffer(&bufferDesc, 0, &buffer);
		}
	} else {
		AX_RELEASE_ASSERT(p);
		dx11_device->CreateBuffer(&bufferDesc, &InitData, &buffer);
	}

	DX11_Resource *apiRes = new DX11_Resource(DX11_Resource::kVertexBuffer, buffer);
	apiRes->m_isDynamic = is_dynamic;
	*h = apiRes;
}

void dx11UploadVertexBuffer(phandle_t h, int offset, int datasize, const void *p)
{
	DX11_Resource *apiRes = h->castTo<DX11_Resource *>();
	AX_RELEASE_ASSERT(apiRes->m_type == DX11_Resource::kVertexBuffer);

	D3D11_MAP mapType = D3D11_MAP_WRITE_DISCARD;
	if (offset != 0)
		mapType = D3D11_MAP_WRITE_NO_OVERWRITE;

	D3D11_MAPPED_SUBRESOURCE mapped;
	dx11_context->Map(apiRes->m_dx11Resource, 0, mapType, 0, &mapped);
	memcpy((byte_t *)mapped.pData + offset, p, datasize);
	dx11_context->Unmap(apiRes->m_dx11Resource, 0);
}

void dx11DeleteVertexBuffer(phandle_t h)
{
	DX11_Resource *apiResource = h->castTo<DX11_Resource *>();
	delete apiResource;
	delete h;
}

void dx11CreateIndexBuffer(phandle_t h, int datasize, Primitive::Hint hint, const void *p)
{
	bool is_dynamic = hint != Primitive::HintStatic;

	// Fill in a buffer description.
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.ByteWidth = datasize;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = p;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// Create the vertex buffer.
	ID3D11Buffer *buffer;
	if (is_dynamic) {
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		if (p) {
			dx11_device->CreateBuffer(&bufferDesc, &InitData, &buffer);
		} else {
			dx11_device->CreateBuffer(&bufferDesc, 0, &buffer);
		}
	} else {
		AX_RELEASE_ASSERT(p);
		dx11_device->CreateBuffer(&bufferDesc, &InitData, &buffer);
	}

	DX11_Resource *apiRes = new DX11_Resource(DX11_Resource::kIndexBuffer, buffer);
	apiRes->m_isDynamic = (hint != Primitive::HintStatic);
	*h = apiRes;
}

void dx11UploadIndexBuffer(phandle_t h, int offset, int datasize, const void *p)
{
	DX11_Resource *apiRes = h->castTo<DX11_Resource *>();
	AX_RELEASE_ASSERT(apiRes->m_type == DX11_Resource::kIndexBuffer);

	D3D11_MAP mapType = D3D11_MAP_WRITE_DISCARD;
	if (offset != 0)
		mapType = D3D11_MAP_WRITE_NO_OVERWRITE;

	D3D11_MAPPED_SUBRESOURCE mapped;
	dx11_context->Map(apiRes->m_dx11Resource, 0, mapType, 0, &mapped);
	memcpy((byte *)mapped.pData + offset, p, datasize);
	dx11_context->Unmap(apiRes->m_dx11Resource, 0);
}

void dx11DeleteIndexBuffer(phandle_t h)
{
	DX11_Resource *apiResource = h->castTo<DX11_Resource *>();
	delete apiResource;
	delete h;
}

void dx11CreateWindowTarget(phandle_t h, Handle hwnd, int width, int height)
{
	DX11_Window *window = new DX11_Window(hwnd, width, height);
	DX11_Resource *resource = new DX11_Resource(DX11_Resource::kWindow, window);
	*h = resource;
}

void dx11UpdateWindowTarget(phandle_t h, Handle newHwnd, int width, int height)
{
	DX11_Resource *resource = h->castTo<DX11_Resource *>();
	AX_ASSERT(resource->m_type == DX11_Resource::kWindow);
	DX11_Window *window = resource->m_window;

	window->update(newHwnd, width, height);
}

void dx11DeleteWindowTarget(phandle_t h)
{
	DX11_Resource *resource = h->castTo<DX11_Resource *>();
	AX_ASSERT(resource->m_type == DX11_Resource::kWindow);
	delete resource;
	delete h;
}

static std::list<ID3D11Query *> s_freeQueries;

ID3D11Query *dx11AllocQuery()
{
	ID3D11Query *query = 0;

	if (!s_freeQueries.empty()) {
		query = s_freeQueries.front();
		s_freeQueries.pop_front();
		return query;
	}
	
	D3D11_QUERY_DESC desc;
	desc.Query = D3D11_QUERY_OCCLUSION;
	desc.MiscFlags = 0;
	dx11_device->CreateQuery(&desc, &query);
	return query;
}

void dx11FreeQuery(ID3D11Query *query)
{
	s_freeQueries.push_front(query);
}

void dx11BeginPerfEvent(const char *pixname)
{
	D3DPERF_BeginEvent(0, u2w(pixname).c_str());
}
void dx11EndPerfEvent()
{
	D3DPERF_EndEvent();
}

static bool s_isTargetSizeSet = false;

static inline ID3D11DepthStencilView *getDSV(phandle_t h, int slice)
{
	if (!h) return 0;
	DX11_Resource *apiRes = h->castTo<DX11_Resource *>();
	AX_ASSERT(apiRes->m_type == DX11_Resource::kDynamicTexture);
	AX_ASSERT(apiRes->m_dynamicTextureData->texFormat.isDepth());
	if (!s_isTargetSizeSet) {
		s_curRenderTargetSize.width = apiRes->m_dynamicTextureData->width;
		s_curRenderTargetSize.height = apiRes->m_dynamicTextureData->height;
		s_isTargetSizeSet = true;
	}
	return apiRes->m_dynamicTextureData->m_depthStencilViews[slice];
}

static inline ID3D11RenderTargetView *getRTV(phandle_t h, int slice)
{
	if (!h) return 0;
	DX11_Resource *apiRes = h->castTo<DX11_Resource *>();
	if (apiRes->m_type == DX11_Resource::kDynamicTexture) {
		AX_ASSERT(apiRes->m_dynamicTextureData->texFormat.isColor());
		if (!s_isTargetSizeSet) {
			s_curRenderTargetSize.width = apiRes->m_dynamicTextureData->width;
			s_curRenderTargetSize.height = apiRes->m_dynamicTextureData->height;
			s_isTargetSizeSet = true;
		}
		return apiRes->m_dynamicTextureData->m_renderTargetViews[slice];
	} else if (apiRes->m_type == DX11_Resource::kWindow) {
		if (!s_isTargetSizeSet) {
			s_curRenderTargetSize = apiRes->m_window->swapChainSize();
			s_isTargetSizeSet = true;
		}
		return apiRes->m_window->getRenderTargetView();
	} else {
		AX_WRONGPLACE;
		return 0;
	}
}

void dx11SetTargetSet(phandle_t targetSet[RenderTargetSet::MaxTarget], int slices[RenderTargetSet::MaxTarget])
{
	s_isTargetSizeSet = false;

	dx11_stateManager->unsetAllTextures();

	s_dsv = getDSV(targetSet[0], slices[0]);
	s_numView = 0;

	for (int i = 0; i < RenderTargetSet::MaxColorTarget; i++) {
		s_rtv[i] = getRTV(targetSet[i+1], slices[i+1]);
		if (!s_rtv[i])
			break;
		s_numView++;
	}

	dx11_context->OMSetRenderTargets(s_numView, s_rtv, s_dsv);
}

void dx11SetViewport(const Rect &rect, const Vector2 & depthRange)
{
	D3D11_VIEWPORT d3dviewport;
	d3dviewport.TopLeftX = rect.x;
	if (rect.y < 0)
		d3dviewport.TopLeftY = s_curRenderTargetSize.height + rect.y - rect.height;
	else
		d3dviewport.TopLeftY = rect.y;

	d3dviewport.Width = rect.width;
	d3dviewport.Height = rect.height;
	d3dviewport.MinDepth = depthRange.x;
	d3dviewport.MaxDepth = depthRange.y;

	D3D11_RECT d3drect;
	d3drect.left = d3dviewport.TopLeftX;
	d3drect.top = d3dviewport.TopLeftY;
	d3drect.right = d3dviewport.TopLeftX + d3dviewport.Width;
	d3drect.bottom = d3dviewport.TopLeftY + d3dviewport.Height;

	dx11_context->RSSetViewports(1, &d3dviewport);
	dx11_context->RSSetScissorRects(1, &d3drect);
}

void dx11SetShader(const FixedString &name, const GlobalMacro &gm, const MaterialMacro &mm, Technique tech)
{
	s_curShader = dx11_shaderManager->findShader(name, gm, mm);
	s_curTechnique = tech;
}

void dx11SetConstBuffer(ConstBuffers::Type type, int size, const void *data)
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	dx11_context->Map(dx11_constBuffers[type], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, data, size);
	dx11_context->Unmap(dx11_constBuffers[type], 0);
}

void dx11SetParameters(const FastParams *params1, const FastParams *params2)
{
	if (params1)
		dx11_curParams1 = *params1;
	else
		dx11_curParams1.clear();

	if (params2)
		dx11_curParams2 = *params2;
	else
		dx11_curParams2.clear();
}

void dx11SetVertices(phandle_t vb, VertexType vt, int offset, int vert_count)
{
	DX11_Resource *resource = vb->castTo<DX11_Resource *>();
	AX_ASSERT(resource->m_type == DX11_Resource::kVertexBuffer);

	dx11_curInstanced = false;
	dx11_curVertexType = vt;
	s_curNumVertices = vert_count;

	UINT stride = vt.stride();
	UINT uoffset = offset;
	dx11_context->IASetVertexBuffers(0, 1, &resource->m_vertexBuffer, &stride, &uoffset);
}

void dx11SetInstanceVertices(phandle_t vb, VertexType vt, int offset, int vert_count, phandle_t inb, int inoffset, int incount)
{
	DX11_Resource *resV = vb->castTo<DX11_Resource *>();
	AX_ASSERT(resV->m_type == DX11_Resource::kVertexBuffer);
	DX11_Resource *resI = inb->castTo<DX11_Resource *>();
	AX_ASSERT(resI->m_type == DX11_Resource::kVertexBuffer);

	dx11_curInstanced = true;
	dx11_curVertexType = vt;
	s_curNumVertices = vert_count;
	s_curNumInstances = incount;

	ID3D11Buffer *buffers[2] = {resV->m_vertexBuffer, resI->m_vertexBuffer};
	UINT strides[2] = { vt.stride(), 64 };
	UINT offsets[2] = { offset, inoffset };

	dx11_context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
}

static D3D11_PRIMITIVE_TOPOLOGY trElementType(ElementType et)
{
	switch (et) {
	case ElementType_PointList: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case ElementType_LineList: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case ElementType_TriList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case ElementType_TriStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	default: AX_WRONGPLACE; return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

void dx11SetIndices(phandle_t ib, ElementType et, int offset, int indicescount)
{
	DX11_Resource *resource = ib->castTo<DX11_Resource *>();
	AX_ASSERT(resource->m_type == DX11_Resource::kIndexBuffer);

	s_curNumIndices = indicescount;

	dx11_context->IASetIndexBuffer(resource->m_indexBuffer, DXGI_FORMAT_R16_UINT, offset);
	dx11_stateManager->setPrimitiveTopology(trElementType(et));
}

void dx11SetVerticesUP(const void *vb, VertexType vt, int vertcount)
{
	int dataSize = vt.stride() * vertcount;
	int offset = s_curVertexBufferPos;
	if (offset + dataSize > VERTEX_UP_SIZE)
		offset = 0;

	dx11UploadVertexBuffer(s_tempVertexBuffer, offset, dataSize, vb);
	dx11SetVertices(s_tempVertexBuffer, vt, offset, vertcount);
}

void dx11SetIndicesUP(const void *ib, ElementType et, int indicescount)
{
	int data_size = indicescount * sizeof(ushort_t);
	int offset = s_curIndexBufferPos;
	if (offset + data_size > INDEX_UP_SIZE)
		offset = 0;

	dx11UploadIndexBuffer(s_tempIndexBuffer, offset, data_size, ib);
	dx11SetIndices(s_tempIndexBuffer, et, offset, indicescount);
}

void dx11SetGlobalTexture(GlobalTextureId id, phandle_t h, const SamplerDesc &samplerState)
{
	dx11_curGlobalTextures[id] = h;
	dx11_curGlobalTextureSamplerDescs[id] = samplerState;
}

void dx11SetMaterialTexture(const FastTextureParams *textures)
{
	dx11_curMaterialTextures = *textures;
}

void dx11SetRenderState(const DepthStencilDesc &dsd, const RasterizerDesc &rd, const BlendDesc &bd)
{
	dx11_stateManager->setDepthStencilState(dsd);
	dx11_stateManager->setRasterizerState(rd);
	dx11_stateManager->setBlendState(bd);
}

static void setupBoundingBoxIndices()
{
	static ushort_t indices[] = {
		0, 2, 1, 1, 2, 3,
		2, 6, 3, 3, 6, 7,
		6, 4, 7, 7, 4, 5,
		4, 0, 5, 5, 0, 1,
		1, 3, 5, 5, 3, 7,
		0, 4, 2, 2, 4, 6
	};

	dx11SetIndicesUP(indices, ElementType_TriList, ArraySize(indices));
}

static void setupBoundingBoxVertices(const BoundingBox &bbox)
{
	static Vector3 verts[8];
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			for (int k = 0; k < 2; k++) {
				verts[i*4+j*2+k].x = i == 0 ? bbox.min.x : bbox.max.x; 
				verts[i*4+j*2+k].y = j == 0 ? bbox.min.y : bbox.max.y; 
				verts[i*4+j*2+k].z = k == 0 ? bbox.min.z : bbox.max.z; 
			}
		}
	}
	dx11SetVerticesUP(verts, VertexType::kChunk, ArraySize(verts));
}

static std::list<Query *> s_activeQueries;

static void dx11IssueQueries(int n, Query *queries[])
{
	setupBoundingBoxIndices();

	s_curShader->begin(s_curTechnique);
	s_curShader->beginPass(0);

	for (int i = 0; i < n; i++) {
		Query *query = queries[i];
		AX_ASSERT(!query->m_handle);
		ID3D11Query *d3dquery = dx11AllocQuery();
		query->m_handle = d3dquery;

		// setup vertice
		setupBoundingBoxVertices(queries[i]->m_bbox);

		dx11_context->Begin(d3dquery);
		// draw
		dx11_context->DrawIndexed(s_curNumIndices, 0, 0);

		dx11_context->End(d3dquery);

		s_activeQueries.push_back(query);
	}

	s_curShader->endPass();
	s_curShader->end();
}

static void dx11CheckQueryResult()
{
	dx11BeginPerfEvent("CheckQueryResult");

	std::list<Query *>::iterator it = s_activeQueries.begin();

	while (it != s_activeQueries.end()) {
		Query *query = *it;
		ID3D11Query *d3dquery = query->m_handle.castTo<ID3D11Query *>();

		UINT64 queryData; // This data type is different depending on the query type

		HRESULT hr = dx11_context->GetData(d3dquery, &queryData, sizeof(queryData), 0);

		if (hr == S_OK) {
			it = s_activeQueries.erase(it);
			dx11FreeQuery(d3dquery);
			query->m_handle = 0;
			query->m_result = queryData;
			continue;
		}

		++it;
	}

	dx11EndPerfEvent();
}


void dx11Draw()
{
	UINT npass = s_curShader->begin(s_curTechnique);
	for (int i = 0; i < npass; i++) {
		s_curShader->beginPass(i);

		if (!dx11_curInstanced) {
			dx11_context->DrawIndexed(s_curNumIndices, 0, 0);
		} else {
			dx11_context->DrawIndexedInstanced(s_curNumIndices, s_curNumInstances, 0, 0, 0);
		}

		s_curShader->endPass();
	}
	s_curShader->end();
}

void dx11Clear(const RenderClearer &clearer)
{
	if (s_dsv) {
		UINT flags = 0;
		if (clearer.isClearDepth) flags |= D3D11_CLEAR_DEPTH;
		if (clearer.isClearStencil) flags |= D3D11_CLEAR_STENCIL;
		if (flags)
			dx11_context->ClearDepthStencilView(s_dsv, flags, clearer.depth, clearer.stencil);
	}

	if (clearer.isClearColor) {
		Color4 color(clearer.color);
		for (int i = 0; i < s_numView; i++) {
			dx11_context->ClearRenderTargetView(s_rtv[i], color.c_ptr());
		}
	}
}

void dx11Present(phandle_t window)
{
	AX_ASSERT(window);
	DX11_Resource *resrouce = window->castTo<DX11_Resource *>();
	AX_ASSERT(resrouce->m_type == DX11_Resource::kWindow);
	resrouce->m_window->present();
	dx11CheckQueryResult();

	s_curVertexBufferPos = 0;
	s_curIndexBufferPos = 0;
}

void dx11AssignRenderApi()
{
	RenderApi::createTextureFromFileInMemory = &dx11CreateTextureFromFileInMemory;
	RenderApi::createTexture = &dx11CreateTexture;
	RenderApi::uploadSubTexture = &dx11UploadSubTexture;
	RenderApi::generateMipmap = &dx11GenerateMipmap;
	RenderApi::deleteTexture = &dx11DeleteTexture;

	RenderApi::createVertexBuffer = &dx11CreateVertexBuffer;
	RenderApi::uploadVertexBuffer = &dx11UploadVertexBuffer;
	RenderApi::deleteVertexBuffer = &dx11DeleteVertexBuffer;

	RenderApi::createIndexBuffer = &dx11CreateIndexBuffer;
	RenderApi::uploadIndexBuffer = &dx11UploadIndexBuffer;
	RenderApi::deleteIndexBuffer = &dx11DeleteIndexBuffer;

	RenderApi::createWindowTarget = &dx11CreateWindowTarget;
	RenderApi::updateWindowTarget = &dx11UpdateWindowTarget;
	RenderApi::deleteWindowTarget = &dx11DeleteWindowTarget;

	RenderApi::issueQueries = &dx11IssueQueries;

	RenderApi::beginPerfEvent = &dx11BeginPerfEvent;
	RenderApi::endPerfEvent = &dx11EndPerfEvent;

	RenderApi::setTargetSet = &dx11SetTargetSet;

	RenderApi::setViewport = &dx11SetViewport;

	RenderApi::setShader = &dx11SetShader;
	RenderApi::setConstBuffer = &dx11SetConstBuffer;
	RenderApi::setParameters = &dx11SetParameters;

	RenderApi::setVertices = &dx11SetVertices;
	RenderApi::setInstanceVertices = &dx11SetInstanceVertices;
	RenderApi::setIndices = &dx11SetIndices;

	RenderApi::setVerticesUP = &dx11SetVerticesUP;
	RenderApi::setIndicesUP = &dx11SetIndicesUP;

	RenderApi::setGlobalTexture = &dx11SetGlobalTexture;
	RenderApi::setMaterialTexture = &dx11SetMaterialTexture;

	RenderApi::setRenderState = &dx11SetRenderState;
	RenderApi::draw = &dx11Draw;

	RenderApi::clear = &dx11Clear;

	RenderApi::present = &dx11Present;
}

void dx11InitApi()
{
	// initialize temp buffer
	s_tempVertexBuffer = new Handle;
	s_tempIndexBuffer = new Handle;
	dx11CreateVertexBuffer(s_tempVertexBuffer, VERTEX_UP_SIZE, Primitive::HintDynamic, 0);
	dx11CreateIndexBuffer(s_tempIndexBuffer, INDEX_UP_SIZE, Primitive::HintDynamic, 0);
	s_curVertexBufferPos = 0;
	s_curIndexBufferPos = 0;

	// initialize const buffer
	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	desc.ByteWidth = g_constBuffers.getBuffer(0)->getByteSize();
	dx11_device->CreateBuffer(&desc, 0, &dx11_constBuffers[0]);

	desc.ByteWidth = g_constBuffers.getBuffer(1)->getByteSize();
	dx11_device->CreateBuffer(&desc, 0, &dx11_constBuffers[1]);

	dx11_context->VSSetConstantBuffers(0, 2, dx11_constBuffers);
	dx11_context->PSSetConstantBuffers(0, 2, dx11_constBuffers);

	for (int i = 0; i < PRIMITIVECONST_COUNT; i++) {
		desc.ByteWidth = (i+1) * 16;
		dx11_device->CreateBuffer(&desc, 0, &dx11_primConstBuffers[i]);
	}

	dx11AssignRenderApi();
}

AX_DX11_END_NAMESPACE
