/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#if G_D3D
//#	define for if (0) else for
#	pragma warning(disable:3205) // warning X3205: conversion from larger type to smaller, possible loss of data
#	pragma warning(disable:3078) // warning X3078: loop control variable conflicts with a previous declaration in the outer scope; most recent declaration will be used
#	pragma pack_matrix(row_major)
#endif

#include "shared.fxh"
#include "constant.fxh"
#include "mathlib.fxh"

// material samplers
#define DECL_SAMPLER(type, name) \
	texture name##_tex; \
	type name = sampler_state { texture = <name##_tex>; };

// material
DECL_SAMPLER(sampler2D, g_diffuseMap);
DECL_SAMPLER(sampler2D, g_normalMap);
DECL_SAMPLER(sampler2D, g_specularMap);
DECL_SAMPLER(sampler2D, g_emissionMap);
DECL_SAMPLER(sampler2D, g_opacitMap);
DECL_SAMPLER(sampler2D, g_displacementMap);
DECL_SAMPLER(sampler2D, g_envMap);
// terrain
DECL_SAMPLER(sampler2D, g_terrainColor);
DECL_SAMPLER(sampler2D, g_terrainNormal);

DECL_SAMPLER(sampler2D, g_detailMap);
DECL_SAMPLER(sampler2D, g_detailMap1);
DECL_SAMPLER(sampler2D, g_detailMap2);
DECL_SAMPLER(sampler2D, g_detailMap3);
DECL_SAMPLER(sampler2D, g_detailNormalMap);
DECL_SAMPLER(sampler2D, g_detailNormalMap1);
DECL_SAMPLER(sampler2D, g_detailNormalMap2);
DECL_SAMPLER(sampler2D, g_detailNormalMap3);
DECL_SAMPLER(sampler2D, g_layerAlpha);
DECL_SAMPLER(sampler2D, g_layerAlpha1);
DECL_SAMPLER(sampler2D, g_layerAlpha2);
DECL_SAMPLER(sampler2D, g_layerAlpha3);

// engine
DECL_SAMPLER(sampler2D, g_reflectionMap);
DECL_SAMPLER(sampler2D, g_lightMap);
// global
DECL_SAMPLER(sampler2D, g_rtDepth);
DECL_SAMPLER(sampler2D, g_rt0);
DECL_SAMPLER(sampler2D, g_rt1);
DECL_SAMPLER(sampler2D, g_rt2);
DECL_SAMPLER(sampler2D, g_rt3);
DECL_SAMPLER(sampler2D, g_lightBuffer);
DECL_SAMPLER(sampler2D, g_sceneColor);

DECL_SAMPLER(samplerCUBE, g_shadowMapCube);

#if G_DX11
Texture2D g_shadowMap_tex;
SamplerComparisonState g_shadowMap;
#define tex2DShadow(sampler, tc) sampler##_tex.SampleCmpLevelZero(sampler, tc.xy, tc.z)
#else
DECL_SAMPLER(sampler2D, g_shadowMap);
#define tex2DShadow(sampler, tc) tex2Dproj(sampler, tc)
#endif

// zparam, for recover view space z, Zview = 2*f*n/((f+n)-Zbuf(f-n)), where Zbuf is [-1,1]
// if Zbuf is [0,1], acultly we use, the equation should be
//
//				f * n
// Zview = -------------------
//			f - Zbuf(f-n)
// faster than use matrix multiply, but only for perspective projection

#if 0
struct ZrecoverParam {
	float near, far, farXnear, nearSUBfar;
};
#endif

float ZR_GetViewSpace(float zbuf)
{
	return g_zrecoverParam.z / (g_zrecoverParam.y + zbuf * g_zrecoverParam.w);
}

// vertex struct
/* data from application vertex buffer */
struct SkinVertex {
	float3 position		: POSITION;
	float4 color		: COLOR0;
	float2 streamTc		: TEXCOORD0;
	float4 normal		: TEXCOORD1;
	float3 tangent		: TEXCOORD2;
	float3 oldPosition	: TEXCOORD3;
};

struct MeshVertex {
    float3 position		: POSITION;
	float4 color		: COLOR0;
    float4 streamTc		: TEXCOORD0;
	float4 normal		: TEXCOORD1;
	float4 tangent		: TEXCOORD2;

	// instance
#if G_GEOMETRY_INSTANCING
	float4 matrixX		: TEXCOORD4;
	float4 matrixY		: TEXCOORD5;
	float4 matrixZ		: TEXCOORD6;
	float4 userDefined	: TEXCOORD7;
#endif
};

// debug vertex input
struct DebugVertex {
    float3 position		: POSITION;
	float4 color		: COLOR0;

#if G_GEOMETRY_INSTANCING
	// instance
	float4 matrixX		: TEXCOORD4;
	float4 matrixY		: TEXCOORD5;
	float4 matrixZ		: TEXCOORD6;
	float4 userDefined	: TEXCOORD7;
#endif
};

// blend vertex
struct BlendVertex {
	float3 position		: POSITION;
	float4 color		: COLOR0;
	float2 streamTc		: TEXCOORD0;
};

struct ChunkVertex {
	float3 position		: POSITION;
};


/* data passed from vertex shader to pixel shader */
struct VertexOut {
    float4 hpos			: POSITION;
	float4 color		: COLOR;
	float4 screenTc		: TEXCOORD0;
    float4 streamTc		: TEXCOORD1;
	float3 worldPos		: TEXCOORD2;
	float3 normal		: TEXCOORD3;

#if !NO_NORMALMAPPING
	float3 tangent		: TEXCOORD4;
	float3 binormal		: TEXCOORD5;
#endif

#if G_FOG
	float fog			: TEXCOORD6;
#endif
};

struct GBufferOut {
	half4 accum			: COLOR0;
	half4 normal		: COLOR1;
	half4 albedo		: COLOR2;
	half4 misc			: COLOR3;
};

struct GBufferData {
	float viewDepth;
	half3 normal;
	half3 diffuse;
	half3 specular;
	half3 emission;
	half shiness;
	half opacity;
	half2 motionVector;
};

struct DeferredData {
	float viewDepth;
	float3 worldPos;
	half3 normal;
	half3 diffuse;
	half specular;
	half shiness;
	half ssao;
};

GBufferOut GB_Output(GBufferData IN)
{
	GBufferOut OUT=(GBufferOut)0;
	OUT.accum.xyz = IN.emission;
	OUT.normal.xyz = IN.normal.xyz * 0.5 + 0.5;
	OUT.normal.z = log2(IN.shiness) / 8;
	OUT.normal.w = IN.normal.z >= 0;
	OUT.albedo.xyz = IN.diffuse;
	OUT.albedo.w = Rgb2Lum(IN.specular);
	return OUT;
}

DeferredData GB_Input(float4 viewDir, float4 screenTc)
{
	DeferredData OUT;

	float depth = tex2Dproj(g_rtDepth, screenTc).r;
	OUT.viewDepth = ZR_GetViewSpace(depth);
	OUT.worldPos = g_cameraPos.xyz + viewDir.xyz / viewDir.w * OUT.viewDepth;

	half4 gnormal = tex2Dproj(g_rt1, screenTc);
	half4 galbedo = tex2Dproj(g_rt2, screenTc);
	//galbedo = pow(galbedo, 2.2);

	OUT.normal.xyz = gnormal.xyz * 2 - 1;
	OUT.normal.z = sqrt(abs(1 - dot(OUT.normal.xy, OUT.normal.xy))) * (gnormal.w > 0 ? 1 : -1);
	OUT.diffuse = galbedo.xyz;
	OUT.specular = galbedo.w;
	OUT.shiness = exp2(gnormal.z * 8);
	OUT.ssao = 1;
	return OUT;
}

#include "texgen.fxh"
#include "fragment.fxh"
#include "vertex.fxh"
