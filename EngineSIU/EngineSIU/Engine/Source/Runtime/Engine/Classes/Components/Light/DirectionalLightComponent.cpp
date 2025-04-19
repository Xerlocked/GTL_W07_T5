#include "DirectionalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"

UDirectionalLightComponent::UDirectionalLightComponent()
{

    DirectionalLightInfo.Direction = -GetUpVector();
    DirectionalLightInfo.Intensity = 10.0f;

    DirectionalLightInfo.LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

UDirectionalLightComponent::~UDirectionalLightComponent()
{
}


FVector UDirectionalLightComponent::GetDirection()  
{
    FRotator rotator = GetWorldRotation();
    FVector WorldDown = rotator.ToQuaternion().RotateVector(-GetUpVector());
    return WorldDown;  
}

const FDirectionalLightInfo& UDirectionalLightComponent::GetDirectionalLightInfo() const
{
    return DirectionalLightInfo;
}

void UDirectionalLightComponent::SetDirectionalLightInfo(const FDirectionalLightInfo& InDirectionalLightInfo)
{
    DirectionalLightInfo = InDirectionalLightInfo;
}

float UDirectionalLightComponent::GetIntensity() const
{
    return DirectionalLightInfo.Intensity;
}

void UDirectionalLightComponent::SetIntensity(float InIntensity)
{
    DirectionalLightInfo.Intensity = InIntensity;
}

FLinearColor UDirectionalLightComponent::GetLightColor() const
{
    return DirectionalLightInfo.LightColor;
}

void UDirectionalLightComponent::SetLightColor(const FLinearColor& InColor)
{
    DirectionalLightInfo.LightColor = InColor;
}
