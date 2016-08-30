//-----------------------------------------------------------------------------
// simplePS.hlsl
// 
// Created by seriousviking at 2016.08.26 11:08
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
// simple pixel shader
Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float2 texCoord: TEXCOORD;
};

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
	return t1.Sample(s1, input.texCoord);
}
