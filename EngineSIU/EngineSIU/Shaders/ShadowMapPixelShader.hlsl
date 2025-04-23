struct PS_INPUT
{
    float4 Position : SV_POSITION;
};

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float4 Output;

    float Depth = Input.Position.z;
    
    Output.r = Depth;
    Output.g = Depth * Depth;
    Output.b = 0.0f;
    Output.a = 1.0f;
    
    return Output;
}
