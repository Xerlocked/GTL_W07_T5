#pragma once
#include "LightComponent.h"

class UPointLightComponent :public ULightComponentBase
{

    DECLARE_CLASS(UPointLightComponent, ULightComponentBase)
public:
    UPointLightComponent();
    virtual ~UPointLightComponent() override;

   virtual UObject* Duplicate(UObject* InOuter) override;
    
    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;

    const FPointLightInfo& GetPointLightInfo() const;
    void SetPointLightInfo(const FPointLightInfo& InPointLightInfo);

    float GetAttenuationRadius() const;
    void SetAttenuationRadius(const float InRadius);

    FLinearColor GetLightColor() const;
    void SetLightColor(const FLinearColor& InColor);


    float GetIntensity() const;
    void SetIntensity(float InIntensity);

    float GetFalloff() const;
    void SetFalloff(float InFalloff);

    int GetType() const;
    void SetType(int InType);

private:
    FPointLightInfo PointLightInfo;
};


