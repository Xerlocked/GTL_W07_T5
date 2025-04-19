#pragma once
#include "Components/SceneComponent.h"


class ULightComponentBase : public USceneComponent
{
    DECLARE_CLASS(ULightComponentBase, USceneComponent)

public:
    ULightComponentBase();
    virtual ~ULightComponentBase() override;
    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void TickComponent(float DeltaTime) override;
    virtual int CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance) override;
    
    FMatrix ViewMatrix;
    FMatrix ProjectionMatrix;

protected:

    FBoundingBox AABB;

public:
    FBoundingBox GetBoundingBox() const {return AABB;}
};
