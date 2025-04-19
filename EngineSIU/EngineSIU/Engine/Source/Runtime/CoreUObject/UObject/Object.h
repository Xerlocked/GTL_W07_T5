#pragma once
#include "EngineLoop.h"
#include "NameTypes.h"

extern FEngineLoop GEngineLoop;

class UClass;
class UWorld;


class UObject
{
private:
    UObject(const UObject&) = delete;
    UObject& operator=(const UObject&) = delete;
    UObject(UObject&&) = delete;
    UObject& operator=(UObject&&) = delete;

public:
    using Super = UObject;
    using ThisClass = UObject;

    static UClass* StaticClass();

private:
    friend class FObjectFactory;
    friend class FSceneMgr;
    friend class UClass;

    uint32 UUID;
    uint32 InternalIndex; // Index of GUObjectArray

    FName NamePrivate;
    UClass* ClassPrivate = nullptr;
    UObject* OuterPrivate = nullptr;

public:
    UObject();
    virtual ~UObject() = default;

    virtual UObject* Duplicate(UObject* InOuter);
    void SetFName(const FName& InName) { NamePrivate = InName; }
    UObject* GetOuter() const { return OuterPrivate; }
    virtual UWorld* GetWorld() const;
    virtual void Serialize(FArchive& Ar);

    FName GetFName() const { return NamePrivate; }
    FString GetName() const { return NamePrivate.ToString(); }

    uint32 GetUUID() const { return UUID; }
    uint32 GetInternalIndex() const { return InternalIndex; }

    UClass* GetClass() const { return ClassPrivate; }


    /** this가 SomeBase인지, SomeBase의 자식 클래스인지 확인합니다. */
    bool IsA(const UClass* SomeBase) const;

    template <typename T>
        requires std::derived_from<T, UObject>
    bool IsA() const
    {
        return IsA(T::StaticClass());
    }

public:
    void* operator new(size_t size)
    {
        UE_LOG(LogLevel::Display, "UObject Created : %d", size);

        void* RawMemory = FPlatformMemory::Malloc<EAT_Object>(size);
        UE_LOG(
            LogLevel::Display,
            "TotalAllocationBytes : %d, TotalAllocationCount : %d",
            FPlatformMemory::GetAllocationBytes<EAT_Object>(),
            FPlatformMemory::GetAllocationCount<EAT_Object>()
        );
        return RawMemory;
    }

    void operator delete(void* ptr, size_t size)
    {
        UE_LOG(LogLevel::Display, "UObject Deleted : %d", size);
        FPlatformMemory::Free<EAT_Object>(ptr, size);
    }

    FVector4 EncodeUUID() const {
        FVector4 result;

        result.X = UUID % 0xFF;
        result.Y = UUID >> 8 & 0xFF;
        result.Z = UUID >> 16 & 0xFF;
        result.W = UUID >> 24 & 0xFF;

        return result;
    }
};
