#include "SpotLightComponent.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"
USpotLightComponent::USpotLightComponent()
{
    SpotLightInfo.Position = GetWorldLocation();
    SpotLightInfo.Radius = 30.0f;
    SpotLightInfo.Direction = GetForwardVector();
    SpotLightInfo.LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    SpotLightInfo.Intensity = 1000.0f;
    SpotLightInfo.Type = ELightType::SPOT_LIGHT;
    SpotLightInfo.InnerRad = 0.2618;
    SpotLightInfo.OuterRad = 0.5236;
    SpotLightInfo.Attenuation = 20.0f;
}

USpotLightComponent::~USpotLightComponent()
{
}

FVector USpotLightComponent::GetDirection()
{
    //여기 딱 FORWARD주는지 봐야함
    FRotator rotator = GetWorldRotation();
    FVector WorldForward = rotator.ToQuaternion().RotateVector(GetForwardVector());
    return WorldForward;
}

const FSpotLightInfo& USpotLightComponent::GetSpotLightInfo() const
{
    return SpotLightInfo;
}

void USpotLightComponent::SetSpotLightInfo(const FSpotLightInfo& InSpotLightInfo)
{
    SpotLightInfo = InSpotLightInfo;
}

float USpotLightComponent::GetRadius() const
{
    return SpotLightInfo.Radius;
}

void USpotLightComponent::SetRadius(float InRadius)
{
    SpotLightInfo.Radius = InRadius;
}

FLinearColor USpotLightComponent::GetLightColor() const
{
    return SpotLightInfo.LightColor;
}

void USpotLightComponent::SetLightColor(const FLinearColor& InColor)
{
    SpotLightInfo.LightColor = InColor;
}



float USpotLightComponent::GetIntensity() const
{
    return SpotLightInfo.Intensity;
}

void USpotLightComponent::SetIntensity(float InIntensity)
{
    SpotLightInfo.Intensity = InIntensity;
}

int USpotLightComponent::GetType() const
{
    return SpotLightInfo.Type;
}

void USpotLightComponent::SetType(int InType)
{
    SpotLightInfo.Type = InType;
}

float USpotLightComponent::GetInnerRad() const
{
    return SpotLightInfo.InnerRad;
}

void USpotLightComponent::SetInnerRad(float InInnerCos)
{
    SpotLightInfo.InnerRad = InInnerCos;
}

float USpotLightComponent::GetOuterRad() const
{
    return SpotLightInfo.OuterRad;
}

void USpotLightComponent::SetOuterRad(float InOuterCos)
{
    SpotLightInfo.OuterRad = InOuterCos;
}

float USpotLightComponent::GetInnerDegree() const
{
    return SpotLightInfo.InnerRad * (180.0f / PI);
}

void USpotLightComponent::SetInnerDegree(float InInnerDegree)
{
    SpotLightInfo.InnerRad = InInnerDegree * (PI / 180.0f);
}

float USpotLightComponent::GetOuterDegree() const
{
    return SpotLightInfo.OuterRad * (180 / PI);
}

void USpotLightComponent::SetOuterDegree(float InOuterDegree)
{
    SpotLightInfo.OuterRad = InOuterDegree * (PI / 180.0f);
}
