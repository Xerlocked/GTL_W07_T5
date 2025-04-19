#pragma once
#include "LightComponent.h"
#include "UObject/ObjectMacros.h"

class  UDirectionalLightComponent : public ULightComponentBase
{
    DECLARE_CLASS(UDirectionalLightComponent, ULightComponentBase)

public:
    UDirectionalLightComponent();
    virtual ~UDirectionalLightComponent() override;
    FVector GetDirection();

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;

    const FDirectionalLightInfo& GetDirectionalLightInfo() const;
    void SetDirectionalLightInfo(const FDirectionalLightInfo& InDirectionalLightInfo);

    float GetIntensity() const;
    void SetIntensity(float InIntensity);

    FLinearColor GetLightColor() const;
    void SetLightColor(const FLinearColor& InColor);

private:
    FDirectionalLightInfo DirectionalLightInfo;
};

