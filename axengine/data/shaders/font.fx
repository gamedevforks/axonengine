/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#include "common.fxh"

static const float	c_UvScale = 1.0 / 512.0;
static const float2 c_SampleOffset[13] = {
																		float2(0.0, -2.0 * c_UvScale),
									float2(-c_UvScale, -c_UvScale),	float2(0.0, -c_UvScale),		float2(c_UvScale, -c_UvScale	),
	float2(-2.0*c_UvScale, 0.0	),	float2(-c_UvScale, 0.0		),		float2(0.0, 0.0		),		float2(c_UvScale, 0			), 	float2(2.0*c_UvScale, 0.0	),
									float2(-c_UvScale, c_UvScale),	float2(0.0, c_UvScale	),		float2(c_UvScale, c_UvScale	),
																		float2(0.0, 2.0 * c_UvScale)
};

static const half c_SampleWeight[13] = {
							1.0 / 20.0,	
				1.0 / 20.0, 2.0 / 20.0, 1.0 / 20.0,
	1.0 / 20.0,	2.0 / 20.0, 4.0 / 20.0, 2.0 / 20.0, 1.0 / 20.0,	
				1.0 / 20.0, 2.0 / 20.0, 1.0 / 20.0,
							1.0 / 20.0,	
};

VertexOut VP_main(BlendVertex IN)
{
    VertexOut OUT = (VertexOut)0;

	OUT.streamTc.xy = IN.streamTc.xy;
	OUT.color = IN.color;

	OUT.hpos = VP_modelToClip(IN.position);

	return OUT;
}

/********* pixel shaders ********/
half4 FP_main(VertexOut IN) : COLOR
{
#if !G_D3D
	half4 c = IN.color.bgra;
#else
	half4 c = IN.color;
#endif
	half4 shadow = 0;

	shadow.rgb = 1 - c.rgb;

	for(int i = 0; i < 13; i++) {
		shadow.a += tex2D(g_diffuseMap, IN.streamTc.xy + c_SampleOffset[i]).a * c_SampleWeight[i];
	}

	c.a *= tex2D(g_diffuseMap, IN.streamTc.xy).a;
#if 0
	c.rgb = lerp(shadow.rgb, c.rgb, c.a);
	c.a += shadow.a;
#endif
	return c;
}

half4 FP_blur(VertexOut IN) : COLOR
{
	half4 c;

	c.rgb = 1 - IN.color.rgb;
	c.a = 0;

	for(int i = 0; i < 13; i++) {
		c.a += tex2D(g_diffuseMap, IN.streamTc.xy + c_SampleOffset[i]).a * c_SampleWeight[i];
	}

	return c;
}

//////////////////////////////////////////////////////////////////////
// TECHNIQUES ////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


technique Main {
#if 0
	pass p0 {
		VertexShader = compile VS_3_0 VP_main();
		PixelShader = compile PS_3_0 FP_blur();
	}
#endif
    pass p1 {
        VertexShader = compile VS_3_0 VP_main();
		PixelShader = compile PS_3_0 FP_main();
    }
}

/***************************** eof ***/
