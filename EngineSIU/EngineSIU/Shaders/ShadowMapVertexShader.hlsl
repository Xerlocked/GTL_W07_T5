cbuffer ObjectBuffer : register(b0)
{
    row_major matrix WorldMatrix;
    row_major matrix InverseTransposedWorld;
    
    float4 UUID;
    
    bool bIsSelected;
    float3 ObjectPadding;
};

cbuffer CameraBuffer : register(b1)
{
    row_major matrix ViewMatrix;
    row_major matrix InvViewMatrix;
    
    row_major matrix ProjectionMatrix;
    row_major matrix InvProjectionMatrix;
    
    float3 ViewWorldLocation; // TODO: 가능하면 버퍼에서 빼기
    float ViewPadding;
    
    float NearClip;
    float FarClip;
    float2 ProjectionPadding;
}

struct PS_INPUT
{
    float4 Position : SV_POSITION;
};

struct VS_INPUT_StaticMesh
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    uint MaterialIndex : MATERIAL_INDEX;
};

PS_INPUT mainVS(VS_INPUT_StaticMesh Input)
{
    PS_INPUT Output;

    Output.Position = float4(Input.Position, 1.0);
    Output.Position = mul(Output.Position, WorldMatrix);
    Output.Position = mul(Output.Position, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);
    
    return Output;
}
