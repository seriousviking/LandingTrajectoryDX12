//-----------------------------------------------------------------------------
// simpleVS.hlsl
// 
// Created by seriousviking at 2016.08.26 11:08
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
// simple vertex shader
float4 mainVS(float3 pos : POSITION) : SV_POSITION
{
	return float4(pos, 1.0f);
}
