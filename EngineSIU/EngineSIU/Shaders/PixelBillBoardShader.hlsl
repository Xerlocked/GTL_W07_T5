
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

cbuffer SubUVConstant : register(b1)
{
    float2 uvOffset;
    float2 uvScale; // sub UV 셀의 크기 (예: 1/CellsPerColumn, 1/CellsPerRow)
}

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

float4 main(PS_Input input) : SV_TARGET
{
    // UV 계산
    float2 UV    = input.UV * uvScale + uvOffset;
    // 텍스처 샘플링 (RGBA)
    float4 Color = Texture.Sample(Sampler, UV);

    // 알파가 아주 작으면(=사실상 0 이면) 픽셀을 버린다
    // - clip(x) 은 x < 0 일 때 픽셀을 드롭
    // - thresholdAlpha 을 0.001 정도로 잡으면, 완전 투명(=0)만 잘라내고
    //   0~0.001 사이의 반투명 픽셀은 유지할 수 있다.
    float thresholdAlpha = 0.001f;
    clip(Color.a - thresholdAlpha);

    // 살아남은 픽셀만 리턴
    return Color;
}
