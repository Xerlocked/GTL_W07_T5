#include "HeightFogActor.h"

#include "Components/HeightFogComponent.h"

AHeightFogActor::AHeightFogActor()
{
    HeightFogComponent = AddComponent<UHeightFogComponent>("UHeightFogComponent_0");

    
}
