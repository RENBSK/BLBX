#include "Shape2D.hlsli"

VS_OUT main(VS_IN input)
{
    VS_OUT output;
    
    output.pos = mul(projection, float4(input.pos, 1.0f));
    
    output.col = input.col;
    
    output.uv = input.uv;
    
    output.type = input.type;
    
    return output;
}