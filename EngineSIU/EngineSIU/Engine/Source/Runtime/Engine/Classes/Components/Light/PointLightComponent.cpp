#include "PointLightComponent.h"

UPointLightComponent::UPointLightComponent()
{
    PointLightInfo.Position = GetWorldLocation();
    PointLightInfo.AttenuationRadius = 0.f;

    PointLightInfo.LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    PointLightInfo.Intensity = 1000.f;
    PointLightInfo.Type = ELightType::POINT_LIGHT;
    PointLightInfo.Falloff = 0.0f;
}

UPointLightComponent::~UPointLightComponent()
{
}

const FPointLightInfo& UPointLightComponent::GetPointLightInfo() const
{
    return PointLightInfo;
}

void UPointLightComponent::SetPointLightInfo(const FPointLightInfo& InPointLightInfo)
{
    PointLightInfo = InPointLightInfo;
}


float UPointLightComponent::GetAttenuationRadius() const
{
    return PointLightInfo.AttenuationRadius;
}

void UPointLightComponent::SetAttenuationRadius(float InRadius)
{
    PointLightInfo.AttenuationRadius = InRadius;
}

FLinearColor UPointLightComponent::GetLightColor() const
{
    return PointLightInfo.LightColor;
}

void UPointLightComponent::SetLightColor(const FLinearColor& InColor)
{
    PointLightInfo.LightColor = InColor;
}


float UPointLightComponent::GetIntensity() const
{
    return PointLightInfo.Intensity;
}

void UPointLightComponent::SetIntensity(float InIntensity)
{
    PointLightInfo.Intensity = InIntensity;
}

int UPointLightComponent::GetType() const
{
    return PointLightInfo.Type;
}

void UPointLightComponent::SetType(int InType)
{
    PointLightInfo.Type = InType;
}

float UPointLightComponent::GetFalloff() const
{
    return PointLightInfo.Falloff;
}

void UPointLightComponent::SetFalloff(float InFalloff)
{
    PointLightInfo.Falloff = InFalloff;
}
