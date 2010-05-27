#ifndef AX_RENDERINTERFACE_H
#define AX_RENDERINTERFACE_H

AX_BEGIN_NAMESPACE

typedef Handle *phandle_t;


class RenderApi
{
public:
	// resource management
	static void (*createTexture2D)(phandle_t h, TexFormat format, int width, int height, int flags);
	static void (*uploadTexture)(phandle_t h, int level, void *pixels, TexFormat format);
	static void (*uploadSubTexture)(phandle_t h, const Rect &rect, const void *pixels, TexFormat format);
	static void (*generateMipmap)(phandle_t h);
	static void (*deleteTexture2D)(phandle_t h);

	static void (*createVertexBuffer)(phandle_t h, int datasize, Primitive::Hint hint);
	static void (*uploadVertexBuffer)(phandle_t h, int datasize, void *p);
	static void (*deleteVertexBuffer)(phandle_t h);

	static void (*createIndexBuffer)(phandle_t h, int datasize, Primitive::Hint hint);
	static void (*uploadIndexBuffer)(phandle_t h, int datasize, void *p);
	static void (*deleteIndexBuffer)(phandle_t h);

	static void (*createWindowTarget)(phandle_t h, Handle hwnd);
	static void (*deleteWindowTarget)(phandle_t h);

	static void (*createSamplerState)(phandle_t h, const SamplerStateDesc &samplerState);
	static void (*deleteSamplerState)(phandle_t h);

	static void (*createBlendState)(phandle_t h, const BlendStateDesc &src);
	static void (*deleteBlendState)(phandle_t h);

	static void (*createDepthStencilState)(phandle_t h, const DepthStencilStateDesc &src);
	static void (*deleteDepthStencilState)(phandle_t h);

	static void (*createRasterizerState)(phandle_t h, const RasterizerStateDesc &src);
	static void (*deleteRasterizerState)(phandle_t h);

	static void (*setShader)(const FixedString &name, const ShaderMacro &sm, Technique tech);
	static void (*setVsConst)(const FixedString &name, int count, float *value);
	static void (*setPsConst)(const FixedString &name, int count, float *value);

	static void (*setVertices)(phandle_t vb, VertexType vt, int vertcount);
	static void (*setInstanceVertices)(phandle_t vb, VertexType vt, int vertcount, Handle inb, int incount);
	static void (*setIndices)(phandle_t ib);

//	static void dip(ElementType et, int offset, int vertcount, int indices_count) = 0;
	static void (*dp)();

	// actions
	static void (*clear)(const RenderClearer &clearer);
};


class ApiWrap
{
public:
	ApiWrap();
	~ApiWrap();

	// api wrapper interface
	void createTexture2D(phandle_t result, TexFormat format, int width, int height, int flags = 0);
	void uploadTexture(phandle_t h, int level, void *pixels, TexFormat format);
	void uploadSubTexture(phandle_t h, const Rect &rect, const void *pixels, TexFormat format);
	void generateMipmap(phandle_t h);
	void deleteTexture2D(phandle_t h);

	void createVertexBuffer(phandle_t result, int datasize, Primitive::Hint hint);
	void uploadVertexBuffer(phandle_t h, int datasize, void *p);
	void deleteVertexBuffer(phandle_t h);

	void createIndexBuffer(phandle_t result, int datasize, Primitive::Hint hint);
	void uploadIndexBuffer(phandle_t h, int datasize, void *p);
	void deleteIndexBuffer(phandle_t h);

	void createWindowTarget(phandle_t h, Handle hwnd);
	void updateWindowTarget(phandle_t h, Handle newWndId);
	void deleteWindowTarget(phandle_t h);

	void createQuery(phandle_t h);
	void issueQuery(phandle_t h, AsioQuery *asioQuery);
	void deleteQuery(phandle_t h);

	void createSamplerState(phandle_t h, const SamplerStateDesc &desc);
	void deleteSamplerState(phandle_t h);

	void createBlendState(phandle_t h, const BlendStateDesc &desc);
	void deleteBlendState(phandle_t h);

	void createDepthStencilState(phandle_t h, const DepthStencilStateDesc &desc);
	void deleteDepthStencilState(phandle_t h);

	void createRasterizerState(phandle_t h, const RasterizerStateDesc &desc);
	void deleteRasterizerState(phandle_t h);

	void setRenderTarget(int index, phandle_t h);
	void setDepthStencil(phandle_t h);

	void setShader(const FixedString & name, const ShaderMacro &sm, Technique tech);
	void setShaderConst(Uniforms::ItemName name, const UniformData &data);
	void setShaderConst(const FixedString &name, const UniformData &data);

	void setVertices(phandle_t vb, VertexType vt, int vertcount);
	void setInstanceVertices(phandle_t vb, VertexType vt, int vertcount, Handle inb, int incount);
	void setIndices(phandle_t ib, ElementType et, int offset, int vertcount, int indicescount);

	void setVerticesUP(const void *vb, VertexType vt, int vertcount);
	void setIndicesUP(const void *ib, ElementType et, int offset, int vertcount, int indicescount);

	void dp();

	void clear(const RenderClearer &clearer);

	byte_t *allocRingBuf(int size);
	int getWritePos() { return m_writePos; }

	template <class Q>
	Q *allocType(int n=1)
	{
		Q *result = reinterpret_cast<Q *>(allocRingBuf(n * sizeof(Q)));
		return result;
	}

	template <class Q>
	Q *allocCommand()
	{
		struct LinkedCmd {
			int cmdLink;
			int endPos;
			Q cmd;
		};
		int size = sizeof(LinkedCmd);
		byte_t *pbuf = allocRingBuf(size);
		LinkedCmd *ptr = reinterpret_cast<LinkedCmd *>(pbuf);

		ptr->cmdLink = -1;
		ptr->endPos = m_writePos;

		if (m_lastLinkPos != -1) {
			*(int *)m_ringBuffer[m_lastLinkPos] = pbuf - m_ringBuffer;
		}
		m_lastLinkPos = pbuf - m_ringBuffer;
		return &ptr->cmd;
	}

protected:
	typedef void (*delete_func_t)(phandle_t);
	void addObjectDeletion(delete_func_t func, phandle_t h);

	void waitToPos(int pos);

private:
	enum {
		RING_BUFFER_SIZE = 4 * 1024 * 1024,
		MAX_DELETE_COMMANDS = 8 * 1024,
		MAX_POS = 0x70000000
	};

	struct ObjectDeletion {
		delete_func_t func;
		phandle_t handle;
	};

	byte_t m_ringBuffer[RING_BUFFER_SIZE];

	volatile int m_readPos, m_writePos;
	volatile int m_lastLinkPos;

	int m_numObjectDeletions;
	ObjectDeletion m_objectDeletions[MAX_DELETE_COMMANDS];
};

class WrapObject;

class RenderContext
{
public:
	SamplerStatePtr findSamplerState(const SamplerStateDesc &desc);
	BlendStatePtr findBlendState(const BlendStateDesc &desc);
	DepthStencilStatePtr findDepthStencilState(const DepthStencilStateDesc &desc);
	RasterizerStatePtr findRasterizerState(const RasterizerStatePtr &desc);

	void issueQueue(RenderQueue *rq);

protected:
	void beginFrame();
	void drawScene(QueuedScene *scene, const RenderClearer &clearer);
	void setupScene(QueuedScene *scene, const RenderClearer *clearer = 0, RenderTarget *target = 0, RenderCamera *camera = 0);
//	void unsetScene(QueuedScene *scene, const RenderClearer *clearer = 0, RenderTarget *target = 0, RenderCamera *camera = 0);
	void drawPrimitive(int prim_id);
	void drawInteraction(Interaction *ia);
	void endFrame();

	void drawGlobalLight(QueuedScene *scene, QueuedLight *light);
	void drawLocalLight(QueuedScene *scene, QueuedLight *light);

	void drawPass_gfill(QueuedScene *scene);
	void drawPass_overlay(QueuedScene *scene);
	void drawPass_composite(QueuedScene *scene);
	void drawPass_shadowGen(QueuedScene *scene);
	void drawPass_lights(QueuedScene *scene);
	void drawPass_postprocess(QueuedScene *scene);

	void drawScene_world(QueuedScene *scene, const RenderClearer &clearer);
	void drawScene_worldSub(QueuedScene *scene);
	void drawScene_noworld(QueuedScene *scene, const RenderClearer &clearer);

	void issueVisQuery();
	void issueShadowQuery();

private:
	RenderTarget *d3d9FrameWnd;
	RenderTarget *s_gbuffer;
	RenderTarget *s_lbuffer;
	RenderTarget *d3d9WorldTarget;
	bool m_isStatistic;
	Technique s_technique;
};

extern RenderContext *g_renderContext;

AX_END_NAMESPACE

#endif // AX_RENDERINTERFACE_H
