cbuffer ConstantBuffer : register(b0)
{
    float4x4 projection;
};

struct VS_IN
{
    float3 pos : POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD0;
    float type : TEXCOORD1;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD0;
    float type : TEXCOORD1;
};



