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

    int index;
    
    /**
     * Shadow
     */
    bool bCastShadow = true;

    /**
     * Todo: Maybe this properties move to the ULightComponent
     * Sharpen 뺴고는 구현 안되었습니다. 죄송합니다. 사죄의 의미로 500연차를 준비했습니다.
     * Coupon: JUNGLE500
     */
    int32 ShadowResolutionScale = 4096;
    float ShadowBias = 0.0f;
    float ShadowSlopeBias = 0.0f;
    int32 ShadowSharpen = 0;
    
protected:

    FBoundingBox AABB;

public:
    FBoundingBox GetBoundingBox() const {return AABB;}
};
