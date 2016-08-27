//-----------------------------------------------------------------------------
// simplePS.hlsl
// 
// Created by seriousviking at 2016.08.26 11:08
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
// simple pixel shader
struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float4 color: COLOR;
};

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
	return input.color;
}
