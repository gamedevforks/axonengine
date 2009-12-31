/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/



#ifndef AX_RENDER_TEXTURE_H
#define AX_RENDER_TEXTURE_H

AX_BEGIN_NAMESPACE

	//--------------------------------------------------------------------------
	// class Texture
	//--------------------------------------------------------------------------

	AX_DECLARE_REFPTR(Texture);

	class Texture : public RefObject {
	public:
		friend class TextureManager;

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

		void uploadSubTexture(const Rect &rect, const void *pixels, TexFormat format = TexFormat::AUTO);
		void generateMipmap();

		// implement RefObject
		virtual void deleteThis();

		// Texture interface, need be implement in render driver
		virtual void initialize(const FixedString &name, InitFlags flags) = 0;
		virtual void initialize(TexFormat format, int width, int height, InitFlags flags = 0) = 0;

		// just instanced this class, not even create real hardware texture
		virtual bool isInitialized() = 0;

		// get some info
		virtual void getSize(int &width, int &height, int &depth) = 0;

		// texture parameters
		virtual void setClampMode(ClampMode clampmwode) = 0;
		virtual void setFilterMode(FilterMode filtermode) = 0;
		virtual void setBorderColor(const Rgba &color) = 0;
		virtual void setHardwareShadowMap(bool enable) = 0;
		virtual TexFormat getFormat() = 0;

		virtual void saveToFile(const String &filename) = 0;
		virtual void uploadSubTextureIm(const Rect &rect, const void *pixels, TexFormat format = TexFormat::AUTO) = 0;
		virtual void generateMipmapIm() = 0;

		// management
		static TextureRp load(const String &name, InitFlags flags=0);
		static TextureRp create(const String &debugname, TexFormat format, int width, int height, InitFlags flags = 0);
		static bool isExist(const String &name);
		static void initManager();
		static void finalizeManager();
		static FixedString normalizeKey(const String &name);
		static void texlist_f(const CmdArgs &args);

	protected:
		Texture();
		virtual ~Texture();

	protected:
		Link<Texture> m_needFreeLink;
		Link<Texture> m_needGenMipmapLink;
	};

	//--------------------------------------------------------------------------
	// class TextureManager
	//--------------------------------------------------------------------------

	class TextureManager {
	public:
		TextureManager();
		virtual ~TextureManager();

		// called in main thread
		TextureRp loadTexture(const String &texname, Texture::InitFlags flags=0);
		TextureRp createTexture(const String &debugname, TexFormat format, int width, int height, Texture::InitFlags flags = 0);
		bool isExist(const FixedString &key);

		// called in draw thread
		virtual TextureRp createObject() = 0;

	private:
		friend class Texture;
		// internal use
		void uploadSubTexture(Texture *tex, const Rect &rect, const void *pixels, TexFormat format = TexFormat::AUTO);
		void generateMipmap(Texture *tex);
		void freeTexture(Texture *tex);
		void texlist_f(const CmdArgs &args);
		// end internal use

	protected:
		struct LoadCmd {
			Texture *texture;
			FixedString texName;	// if filename is empty, than use format, width, height to init
			Texture::InitFlags initFlags;
			TexFormat format;
			int width;
			int height;
		};
		typedef List<LoadCmd> LoadCmdList;

		struct UploadCmd {
			Texture *texture;
			Rect rect;
			const void *pixel;
			TexFormat format;
			Rgba color; // if pixel is null, then use this color
		};
		typedef List<UploadCmd> UploadCmdList;

		struct FreeCmd {
			Texture *texture;
			int frameId;
		};
		typedef List<FreeCmd> FreeCmdList;

		typedef Dict<FixedString,bool> ExistDict;
		typedef Dict<FixedString,Texture*> TextureDict;

		// command buffer
		LoadCmdList m_loadCmdList;
		UploadCmdList m_uploadCmdList;
		FreeCmdList m_freeCmdList;
		Link<Texture> m_needGenMipmapHead;
		Link<Texture> m_needFreeHead;

		// texture manager's hash table
		ExistDict m_existDict;
		TextureDict m_textureDict;
	};

AX_END_NAMESPACE


#endif // AX_RENDER_TEXTURE_H

