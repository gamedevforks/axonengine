/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#include "common.fxh"
#include "light.fxh"
#include "fog.fxh"

#define S_DECAL M_FEATURE0
#define S_ALPHATEST M_FEATURE1
#define S_TWOSIDES M_FEATURE2

float Script : STANDARDSGLOBAL <
	// technique
	string TechniqueGeoFill = "gfill";
#if S_DECAL
	string TechniqueShadowGen = "";
#else
	string TechniqueShadowGen = "shadowgen";
#endif
	string TechniqueMain = "main";
	string TechniqueGlow = "";
	string TechniqueLayer = "";
> = 0.8;

// UN-TWEAKABLES

struct ShadowVertexOut {
	float4 hpos			: POSITION;
	float4 diffuseTc	: TEXCOORD0;
	float3 worldPos		: TEXCOORD1;
};

ShadowVertexOut VP_zpass(MeshVertex IN)
{
	ShadowVertexOut OUT;
	OUT.worldPos = VP_modelToWorld(IN, IN.position);
	OUT.hpos = VP_worldToClip(OUT.worldPos);
	OUT.diffuseTc.xy = IN.streamTc.xy;
	OUT.diffuseTc.zw = OUT.hpos.zw / g_zrecoverParam.y;
	return OUT;
}

float4 FP_zpass(ShadowVertexOut IN) : COLOR
{
	float4 OUT;
	float depth = length(IN.worldPos - g_cameraPos.xyz) / g_zrecoverParam.y;

	// depth bias
	depth += abs(ddx(depth)) + abs(ddy(depth));
	depth += 1.0 / 10000;
	OUT = depth;

#if S_ALPHATEST && M_DIFFUSE
	half alpha = tex2D(g_diffuseMap, IN.diffuseTc.xy).a;

	if (alpha < 0.5)
		discard;
#endif
	return OUT;
}

Gbuffer FP_gpass(VertexOut IN)
{
	Gbuffer OUT=(Gbuffer)0;

	half3 detail = 0;
#if M_DETAIL
	detail = tex2D(g_detailMap, IN.streamTc.xy * g_detailScale.xx).xyz - 0.5;
#endif

#if M_DIFFUSE
    OUT.albedo.xyz = tex2D(g_diffuseMap, IN.streamTc.xy).xyz;
#else
	OUT.albedo.xyz = half3(1, 1, 1);
#endif
	OUT.albedo.xyz += detail;
	OUT.albedo.xyz *= IN.color.rgb;

#if S_DECAL || S_ALPHATEST
#if M_DIFFUSE
	half alpha = tex2D(g_diffuseMap, IN.streamTc.xy).a;
#else
	half alpha = 1;
#endif
#endif
#if S_ALPHATEST
	if (alpha < 0.5)
		discard;
#endif

	half3 N;

#if NO_NORMALMAPPING
	OUT.normal.xyz = normalize(IN.normal);
#else
	OUT.normal.xyz = FP_GetNormal(IN.normal, IN.tangent, IN.binormal, IN.streamTc.xy);
#endif
	OUT.normal.xyz = OUT.normal.xyz * 0.5f + 0.5f;

	OUT.misc.w = g_matShiness;

	half3 spec;
#if M_SPECULAR
	spec = tex2D(g_specularMap, IN.streamTc.xy).xyz + detail;
#else
#if M_NORMAL
	spec = tex2D(g_normalMap, IN.streamTc.xy).a + detail;
#else
	spec = OUT.albedo.xyz;
#endif
#endif
	OUT.albedo.w = Rgb2Lum(spec);

#if M_EMISSION
	OUT.accum.xyz = tex2D(g_emissionMap, IN.streamTc.xy).xyz;
#endif

#if S_DECAL || S_ALPHATEST
	OUT.accum.a = alpha;
#else
	OUT.accum.a = 1;
#endif

	OUT.accum = OUT.normal;

	return OUT;
}

/********* pixel shaders ********/
half4 FP_main(VertexOut IN) : COLOR0
{
#if 0 && M_DETAIL_NORMAL
	return tex2D(g_detailNormalMap, IN.streamTc.xy);
#endif

	LightParams lps;

#if G_D3D
	lps = (LightParams)0;
#endif

	lps.worldpos = IN.worldPos;

	lps.calcSpecular = false;
	lps.screenTc = IN.screenTc;

	half3 detail = 0;
#if M_DETAIL
	detail = tex2D(g_detailMap, IN.streamTc.xy * g_detailScale.xx) - 0.5;
#endif

#if M_DIFFUSE
    lps.Cd = tex2D(g_diffuseMap, IN.streamTc.xy).xyz;
#else
	lps.Cd = half3(1, 1, 1);
#endif
	lps.Cd += detail;
	lps.Cd *= IN.color.rgb;

#if S_DECAL || S_ALPHATEST
#if M_DIFFUSE
	half alpha = tex2D(g_diffuseMap, IN.streamTc.xy).a;
#else
	half alpha = 1;
#endif
#endif
#if S_ALPHATEST
	if (alpha < 0.5)
		discard;
#endif

#if M_LIGHTMAP
	lps.Cd *= saturate(tex2D(g_lightMap, IN.streamTc.zw).x);
//	return tex2D(g_lightMap, IN.streamTc.zw);
#endif
	lps.Ca.xyz = lps.Cd;
	lps.Ca.w = 0.2f;

	half3 N;

#if NO_NORMALMAPPING
	lps.normal = normalize(IN.normal);
#else
#if 0
	half3 normal = GetNormal(g_normalMap, IN.streamTc.xy).xyz;

	half3x3 axis = half3x3(IN.tangent,IN.binormal, IN.normal);
	N = mul(normal, axis);
	lps.normal = normalize(N);
#else
	lps.normal = FP_GetNormal(IN.normal, IN.tangent, IN.binormal, IN.streamTc.xy);
#endif
#endif

#if !G_DISABLE_SPECULAR
	lps.calcSpecular = true;
	lps.viewDir = normalize(g_cameraPos.xyz - IN.worldPos);
	lps.shiness = 20;
#else
	lps.calcSpecular = false;
#endif

#if M_SPECULAR
	lps.Cs = tex2D(g_specularMap, IN.streamTc.xy).xyz + detail;
#else
#if M_NORMAL
	lps.Cs = tex2D(g_normalMap, IN.streamTc.xy).a + detail;
#else
	lps.Cs = Dif2Spec(lps.Cd);
#endif
#endif

#if G_FOG
	lps.fog = IN.fog;
#else
	lps.fog = 1;
#endif

	half4 final;
	final.rgb = LT_calcAllLights(lps);

#if M_EMISSION
	final.rgb += tex2D(g_emissionMap, IN.streamTc.xy).xyz;
#endif

#if S_DECAL || S_ALPHATEST
	final.a = alpha;
#else
	final.a = 1;
#endif

	return final;
}


//----------------------------------------------------------------------------

technique gfill {
    pass p0 {
        VertexShader = compile VS_3_0 OutputMeshVertex();
		PixelShader = compile PS_3_0 FP_gpass();
    }
}

technique shadowgen {
    pass p0 {
        VertexShader = compile VS_3_0 VP_zpass();
		PixelShader = compile PS_3_0 FP_zpass();
    }
}


technique main {
    pass p0 {
        VertexShader = compile VS_3_0 OutputMeshVertex();
        PixelShader = compile PS_3_0 FP_main();
    }
}

/***************************** eof ***/
