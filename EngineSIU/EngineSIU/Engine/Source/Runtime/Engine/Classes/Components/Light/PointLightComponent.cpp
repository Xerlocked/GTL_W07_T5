#include "PointLightComponent.h"
#include "UObject/Casts.h"

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

UObject* UPointLightComponent::Duplicate(UObject* InOuter)
{

    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));
    if (NewComponent)
    {
        NewComponent->PointLightInfo = PointLightInfo;
    }
    return NewComponent;
}

void UPointLightComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("AttenuationRadius"), FString::Printf(TEXT("%f"), PointLightInfo.AttenuationRadius));
    OutProperties.Add(TEXT("LightColor"), FString::Printf(TEXT("%s"), *PointLightInfo.LightColor.ToString()));
    OutProperties.Add(TEXT("Intensity"), FString::Printf(TEXT("%f"), PointLightInfo.Intensity));
    OutProperties.Add(TEXT("Type"), FString::Printf(TEXT("%d"), PointLightInfo.Type));
    OutProperties.Add(TEXT("Falloff"), FString::Printf(TEXT("%f"), PointLightInfo.Falloff));
    OutProperties.Add(TEXT("Position"), FString::Printf(TEXT("%s"), *PointLightInfo.Position.ToString()));
}

void UPointLightComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("AttenuationRadius"));
    if (TempStr)
    {
        PointLightInfo.AttenuationRadius = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("LightColor"));
    if (TempStr)
    {
        PointLightInfo.LightColor.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Intensity"));
    if (TempStr)
    {
        PointLightInfo.Intensity = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Type"));
    if (TempStr)
    {
        PointLightInfo.Type = FString::ToInt(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Falloff"));
    if (TempStr)
    {
        PointLightInfo.Falloff = FString::ToInt(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Position"));
    if (TempStr)
    {
        PointLightInfo.Position.InitFromString(*TempStr);
    }

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
