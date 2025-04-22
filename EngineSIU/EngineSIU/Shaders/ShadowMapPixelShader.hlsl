struct PS_INPUT
{
    float4 Position : SV_POSITION;
};

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float4 Output;

    float3 NDC = Input.Position.xyz / Input.Position.w;

    float Depth = NDC.z;
    
    Output.r = Depth;
    Output.g = Depth * Depth;
    Output.b = 0.0f;
    Output.a = 1.0f;
    
    return Output;
}
