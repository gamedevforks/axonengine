/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/



#ifndef AX_RENDER_TEXTURE_H
#define AX_RENDER_TEXTURE_H

namespace Axon { namespace Render {

	class Texture : public Asset {
	public:
		enum { AssetType = Asset::kTexture };

		enum InitFlag {
			IF_NoMipmap = 1,
			IF_NoDownsample = 2,
			IF_AutoGenMipmap = 4,
			IF_RenderTarget = 8
		};

		typedef Flags_<InitFlag> InitFlags;

		enum TexType {
			TT_2D,
			TT_3D,
			TT_CUBE,
		};

		enum ClampMode {
			CM_Repeat,
			CM_Clamp,
			CM_ClampToEdge,	// only used in engine internal
			CM_ClampToBorder // only used in engine internal
		};


		enum FilterMode {
			FM_Nearest,
			FM_Linear,
			FM_Bilinear,
			FM_Trilinear
		};

		virtual void initialize(TexFormat format, int width, int height, InitFlags flags = 0) = 0;
		virtual void getSize(int& width, int& height, int& depth) = 0;
		virtual void uploadSubTexture(const Rect& rect, const void* pixels, TexFormat format = TexFormat::AUTO) = 0;
		virtual void setClampMode(ClampMode clampmwode) = 0;
		virtual void setFilterMode(FilterMode filtermode) = 0;
		virtual void setBorderColor(const Rgba& color) = 0;
		virtual void setHardwareShadowMap(bool enable) = 0;

		virtual void saveToFile(const String& filename) = 0;
		virtual void generateMipmap() = 0;
		virtual TexFormat getFormat() = 0;

	protected:
		virtual ~Texture() {}
	};

	AX_DECLARE_REFPTR(Texture);

	class TextureManager {
	public:
		TextureManager();
		virtual ~TextureManager();

		// called in main thread
		TexturePtr loadTexture(const FixedString& texname, Texture::InitFlags flags=0);
		TexturePtr createTexture(const String& debugname, TexFormat format, int width, int height, Texture::InitFlags flags = 0);
		bool isExist(const FixedString& texname);

		// called in draw thread
		virtual TexturePtr createObject() = 0;

	protected:
		struct LoadCmd {
			Texture* texture;
			FixedString texName;	// if filename is empty, than use format, width, height to init
			Texture::InitFlags initFlags;
			TexFormat format;
			int width;
			int height;
		};
		typedef List<LoadCmd> LoadCmdList;

		struct UploadCmd {
			Texture* texture;
			Rect rect;
			void* pixel;
			TexFormat format;
			Rgba color; // if pixel is null, then use this color
		};
		typedef List<UploadCmd> UploadCmdList;

		struct FreeCmd {
			Texture* texture;
			int frameId;
		};
		typedef List<FreeCmd> FreeCmdList;

		typedef Dict<FixedString,bool> ExistDict;

		LoadCmdList m_loadCmdList;
		UploadCmdList m_uploadCmdList;
		FreeCmdList m_freeCmdList;
		int m_frameId;
		ExistDict m_existDict;
	};

}} // namespace Axon::Render


#endif // AX_RENDER_TEXTURE_H
