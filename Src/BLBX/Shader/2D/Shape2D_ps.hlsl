#include "Shape2D.hlsli"

float4 main(VS_OUT input) : SV_TARGET
{
    //‰~‚Ì‚¾‚¯
    if (input.type > 0.5f)
    {
        float2 center = float2(0.5, 0.5);

        float dist = distance(input.uv, center);

        if (dist > 0.5)
            discard;
    }

    return input.col;
}