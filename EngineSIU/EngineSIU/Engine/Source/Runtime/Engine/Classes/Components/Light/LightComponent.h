#pragma once
#include "Components/SceneComponent.h"


class ULightComponentBase : public USceneComponent
{
    DECLARE_CLASS(ULightComponentBase, USceneComponent)

public:
    ULightComponentBase();
    virtual ~ULightComponentBase() override;
    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;

    virtual void TickComponent(float DeltaTime) override;
    virtual int CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance) override;
    
    FMatrix ViewMatrix[6];
    FMatrix ProjectionMatrix;
    FVector LightCameraPos;


    /**
     * Shadow
     */
    bool bCastShadow = true;

    /**
     * Todo: Maybe this properties move to the ULightComponent 
     */
    int32 ShadowResolutionScale = 4096.f;
    float ShadowBias = 0.0f;
    float ShadowSlopeBias = 0.0f;
    int32 ShadowSharpen = 0.0f;
    
protected:

    FBoundingBox AABB;

public:
    FBoundingBox GetBoundingBox() const {return AABB;}
};
