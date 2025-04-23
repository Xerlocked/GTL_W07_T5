#pragma once
#include "LightComponent.h"
#include "Math/Vector.h"
class USpotLightComponent :public ULightComponentBase
{

    DECLARE_CLASS(USpotLightComponent, ULightComponentBase)
public:
    USpotLightComponent();
    virtual ~USpotLightComponent();

    virtual UObject* Duplicate(UObject* InOuter) override;

    void GetProperties(TMap<FString, FString>& OutProperties) const override;
    void SetProperties(const TMap<FString, FString>& InProperties) override;

    /**
     * 
     * @return Degree
     */
    FVector GetDirection();

    const FSpotLightInfo& GetSpotLightInfo() const;
    void SetSpotLightInfo(const FSpotLightInfo& InSpotLightInfo);

    float GetAttenuationRadius() const;
    void SetAttenuationRadius(float InRadius);

    FLinearColor GetLightColor() const;
    void SetLightColor(const FLinearColor& InColor);

    float GetIntensity() const;
    void SetIntensity(float InIntensity);

    int GetType() const;
    void SetType(int InType);

    float GetInnerRad() const;
    void SetInnerRad(float InInnerCos);

    float GetOuterRad() const;
    void SetOuterRad(float InOuterCos);

    float GetInnerDegree() const;
    void SetInnerDegree(float InInnerDegree);

    float GetOuterDegree() const;
    void SetOuterDegree(float InOuterDegree);

    float GetFalloff() const;
    void SetFalloff(float InFalloff);

    FVector2D GetSubUVScale() { return SubUVScale; }
    void SetSUbUVScale(FVector2D inSubUVScale)
    {
        SubUVScale = inSubUVScale;
    }
    FVector2D GetSubUVOffset() { return SubUVOffset; }
    void SetSUbUVOffset(FVector2D inSubUVOffset)
    {
        SubUVOffset = inSubUVOffset;
    }

private:
    FSpotLightInfo SpotLightInfo;
    FVector2D SubUVScale;
    FVector2D SubUVOffset;
};

