/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#include "common.fxh"

VertexOut VP_main(MeshVertex IN)
{
    VertexOut OUT = (VertexOut)0;

	OUT.color = IN.color;

	OUT.worldPos = IN.position;

#if G_REFLECTION	// HACK
	OUT.worldPos.z -= 20.0f;
#endif

	OUT.hpos = mul(g_viewProjNoTranslate, float4(OUT.worldPos,1));

	VP_final(IN, OUT);

    return OUT;
}

GBufferOut FP_gpass(VertexOut IN)
{
	GBufferData OUT = (GBufferData)0;
	OUT.emission = IN.color;
	OUT.shiness = g_matShiness;

#if M_DIFFUSE
	OUT.emission *= tex2D(g_diffuseMap, IN.streamTc.xy);
#endif

	return GB_Output(OUT);
}

half4 FP_main(VertexOut IN) : COLOR
{
	half4 result;
	result = IN.color;

#if M_DIFFUSE
	result *= tex2D(g_diffuseMap, IN.streamTc.xy);
#endif

	float3 viewDir = normalize(IN.worldPos);
#if G_FOG
	result.xyz = lerp(g_fogParams.xyz, result.xyz, saturate(viewDir.z * 4));
#endif
	half LdotE = saturate(dot(viewDir, g_globalLightPos.xyz));
	half sun = pow(LdotE, 100) * MAX_EXPOSURE;
	result.xyz += sun;

	result.xyz = FinalOutput(result.xyz);

	return result;
}

technique GeoFill
{
	pass p0 {
		VertexShader = compile VS_3_0 VP_main();
		PixelShader = compile PS_3_0 FP_gpass();
	}
}


technique Main
{
    pass p0 {
        VertexShader = compile VS_3_0 VP_main();
		PixelShader = compile PS_3_0 FP_main();
    }
}

/***************************** eof ***/
