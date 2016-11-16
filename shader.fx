//--------------------------------------------------------------------------------------
// File: lecture 8.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register(t0);
Texture2D txDepth : register(t1);
SamplerState samLinear : register(s0);

cbuffer ConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 info;
	float4 colorChanger; //-ML
	float4 enemyColorChanger; //-SH
	float4 bulletColorChanger; //-SH

};
//must be mult number four floats


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = mul(input.Pos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;

	return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	float4 color = txDiffuse.Sample(samLinear, input.Tex);
	float depth = saturate(input.Pos.z / input.Pos.w);
	//depth = pow(depth,0.97);
	//color = depth;// (depth*0.9 + 0.02);
	return color;
}

//PIXEL SHADER FOR FRAME -ML
//////////////////////////////////////////////////////////////////////
float4 PS_frame(PS_INPUT input) : SV_Target
{
	float4 color = txDiffuse.Sample(samLinear, input.Tex);

	color.rgb = colorChanger.xyz;
	/*
	if (colorChanger.x == 1) {
	color.rgb = colorChanger.xyz;
	//color *= 0.5;
	} else if (colorChanger.x == 0) {
	color.r = 0;
	color.g = 0;
	color.b = 1;
	color *= 5;
	}
	*/

	//int instead if one or two 
	//constantbuffer.rand = xm4 1 0 0 0
	//pass color directly
	//float4 billboard.
	//float depth = saturate(input.Pos.z / input.Pos.w);
	//depth = pow(depth,0.97);
	//color = depth;// (depth*0.9 + 0.02);
	return color;
}
///////////////////////////////////////////////////////////////////////

//PIXEL SHADER FOR ENEMIES -SH
////////////////////////////////////////////////////////////////////////
float4 PS_enemies(PS_INPUT input) : SV_Target
{
	float4 color = txDiffuse.Sample(samLinear, input.Tex);

	//if color is blue
	if (enemyColorChanger.b == 1) {
		color.b = .5;
	}

	//if color is red
	if (enemyColorChanger.r == 1) {
		color.r = .5;
	}
	return color;
}
//////////////////////////////////////////////////////////////////////////

//PIXEL SHADER FOR BULLET -SH
////////////////////////////////////////////////////////////////////////
float4 PS_bullets(PS_INPUT input) : SV_Target
{
	float4 color = txDiffuse.Sample(samLinear, input.Tex);

	//if color is blue
	if (bulletColorChanger.b == 1) {
		color.b = .5;
	}

	//if color is red
	if (bulletColorChanger.r == 1) {
		color.r = 1;
	}
	return color;
}
//////////////////////////////////////////////////////////////////////////