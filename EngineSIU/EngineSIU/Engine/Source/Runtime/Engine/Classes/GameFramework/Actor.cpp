#include "Actor.h"
#include "World/World.h"


UObject* AActor::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewActor->Owner = Owner;
    NewActor->bTickInEditor = bTickInEditor;
    // 기본적으로 있던 컴포넌트 제거
    TSet CopiedComponents = NewActor->OwnedComponents;
    TArray<FName> DefaultCopiedComponentNames;

    for (UActorComponent* Components : CopiedComponents)
    {
        DefaultCopiedComponentNames.Add(Components->GetFName());
        Components->DestroyComponent();
    }
    NewActor->OwnedComponents.Empty();


    // 부모-자식 관계 저장용 맵
    TMap<USceneComponent*, USceneComponent*> ParentChildMap;

    // 컴포넌트 복제 및 부모-자식 관계 추적
    TMap<USceneComponent*, USceneComponent*> OriginalToDuplicateMap; // 원본 -> 복제된 컴포넌트 매핑

    for (UActorComponent* Component : OwnedComponents)
    {
        UActorComponent* NewComponent = Cast<UActorComponent>(Component->Duplicate(NewActor));
        NewComponent->OwnerPrivate = NewActor;
        NewActor->OwnedComponents.Add(NewComponent);

        for (const auto DefaultCopiedName : DefaultCopiedComponentNames)
        {
            if (DefaultCopiedName == Component->GetFName())
            {
                NewComponent->SetFName(DefaultCopiedName);
            }
        }
        // RootComponent 설정
        if (RootComponent == Component)
        {
            NewActor->RootComponent = static_cast<USceneComponent*>(NewComponent);
        }

        // 부모-자식 관계 저장 (USceneComponent만 해당)
        if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
        {
            OriginalToDuplicateMap.Add(SceneComp, static_cast<USceneComponent*>(NewComponent));
            if (USceneComponent* ParentComp = SceneComp->GetAttachParent())
            {
                ParentChildMap.Add(static_cast<USceneComponent*>(NewComponent), ParentComp);
            }
        }

        // 컴포넌트 초기화
        NewComponent->InitializeComponent();
    }

    // 부모-자식 관계 복원
    for (const auto& [ChildComp, OriginalParentComp] : ParentChildMap)
    {
        // 복제된 부모를 찾아 설정
        if (USceneComponent** NewParentComp = OriginalToDuplicateMap.Find(OriginalParentComp))
        {
            ChildComp->AttachToComponent(*NewParentComp);
        }
    }

    return NewActor;
}

void AActor::BeginPlay()
{
    // TODO: 나중에 삭제를 Pending으로 하던가 해서 복사비용 줄이기
    const auto CopyComponents = OwnedComponents;
    for (UActorComponent* Comp : CopyComponents)
    {
        Comp->BeginPlay();
    }
}

void AActor::Tick(float DeltaTime)
{
    // TODO: 임시로 Actor에서 Tick 돌리기
    // TODO: 나중에 삭제를 Pending으로 하던가 해서 복사비용 줄이기
    const auto CopyComponents = OwnedComponents;

    for (UActorComponent* Comp : CopyComponents)
    {
        Comp->TickComponent(DeltaTime);
    }
}

void AActor::Destroyed()
{
    // Actor가 제거되었을 때 호출하는 EndPlay
    EndPlay(EEndPlayReason::Destroyed);
}

void AActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 본인이 소유하고 있는 모든 컴포넌트의 EndPlay 호출
    for (UActorComponent* Component : GetComponents())
    {
        if (Component->HasBegunPlay())
        {
            Component->EndPlay(EndPlayReason);
        }
    }
    UninitializeComponents();
}

bool AActor::Destroy()
{
    if (!IsActorBeingDestroyed())
    {
        if (UWorld* World = GetWorld())
        {
            World->DestroyActor(this);
            bActorIsBeingDestroyed = true;
        }
    }

    return IsActorBeingDestroyed();
}

UActorComponent* AActor::AddComponent(UClass* InClass, FName InName, bool bTryRootComponent)
{

    if (!InClass)
    {
        UE_LOG(LogLevel::Error, TEXT("UActorComponent failed: ComponentClass is null."));
        return nullptr;
    }
    
    if (InClass->IsChildOf<UActorComponent>())
    {
        UActorComponent* Component = static_cast<UActorComponent*>(FObjectFactory::ConstructObject(InClass, this, InName));

        if (!Component)
        {
            UE_LOG(LogLevel::Error, TEXT("UActorComponent failed: Class '%s' is not derived from AActor."), *InClass->GetName());
            return nullptr;
        }
        
        OwnedComponents.Add(Component);
        Component->OwnerPrivate = this;

        // 만약 SceneComponent를 상속 받았다면

        if (bTryRootComponent)
        {
            if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
            {
                if (RootComponent == nullptr)
                {
                    RootComponent = SceneComp;
                }
            // TODO: 나중에 RegisterComponent() 생기면 주석 해제
                // else
                // {
                //     SceneComp->SetupAttachment(RootComponent);
                // }
            }
        }

        // TODO: RegisterComponent() 생기면 제거
        Component->InitializeComponent();

        return Component;
    }
    
    UE_LOG(LogLevel::Error, TEXT("UActorComponent failed: ComponentClass is null."));
    return nullptr;
}

void AActor::RemoveOwnedComponent(UActorComponent* Component)
{
    OwnedComponents.Remove(Component);
}

void AActor::InitializeComponents()
{
    TSet<UActorComponent*> Components = GetComponents();
    for (UActorComponent* ActorComp : Components)
    {
        if (ActorComp->bAutoActive && !ActorComp->IsActive())
        {
            ActorComp->Activate();
        }

        if (!ActorComp->HasBeenInitialized())
        {
            ActorComp->InitializeComponent();
        }
    }
}

void AActor::UninitializeComponents()
{
    TSet<UActorComponent*> Components = GetComponents();
    for (UActorComponent* ActorComp : Components)
    {
        if (ActorComp->HasBeenInitialized())
        {
            ActorComp->UninitializeComponent();
        }
    }
}

bool AActor::SetRootComponent(USceneComponent* NewRootComponent)
{
    if (NewRootComponent == nullptr || NewRootComponent->GetOwner() == this)
    {
        if (RootComponent != NewRootComponent)
        {
            USceneComponent* OldRootComponent = RootComponent;
            RootComponent = NewRootComponent;

            if (OldRootComponent)
            {
                OldRootComponent->SetupAttachment(RootComponent);
            }
        }
        return true;
    }
    return false;
}

FVector AActor::GetActorLocation() const
{
    return RootComponent ? RootComponent->GetRelativeLocation() : FVector(FVector::ZeroVector); 
}

FRotator AActor::GetActorRotation() const
{
    return RootComponent ? RootComponent->GetRelativeRotation() : FRotator(FVector::ZeroVector);
}

FVector AActor::GetActorScale() const
{
    return RootComponent ? RootComponent->GetRelativeScale3D() : FVector(FVector::OneVector); 
}

bool AActor::SetActorLocation(const FVector& NewLocation)
{
    if (RootComponent)
    {
       RootComponent->SetRelativeLocation(NewLocation);
        return true;
    }
    return false;
}

bool AActor::SetActorRotation(const FRotator& NewRotation)
{
    if (RootComponent)
    {
        RootComponent->SetRelativeRotation(NewRotation);
        return true;
    }
    return false;
}

bool AActor::SetActorScale(const FVector& NewScale)
{
    if (RootComponent)
    {
        RootComponent->SetRelativeScale3D(NewScale);
        return true;
    }
    return false;
}

void AActor::SetActorTickInEditor(bool InbInTickInEditor)
{
    bTickInEditor = InbInTickInEditor;
}
