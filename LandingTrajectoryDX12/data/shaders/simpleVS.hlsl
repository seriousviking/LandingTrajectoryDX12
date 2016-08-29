//-----------------------------------------------------------------------------
// simpleVS.hlsl
// 
// Created by seriousviking at 2016.08.26 11:08
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
// simple vertex shader
struct VS_INPUT
{
	float3 position : POSITION;
	float4 color: COLOR;
};

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float4 color: COLOR;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 wvpMatrix;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = mul(float4(input.position, 1.0), wvpMatrix);
	output.color = input.color;
	return output;
}
