#pragma once
#include "MathUtility.h"
#include "Vector.h"
#include "Vector4.h"
#include "Container/String.h"
#include "HAL/PlatformType.h"


/**
 * 0~255 사이의 값을 가지는 색상 구조체
 * BGRA 순서로 저장됨
 * 감마가 적용된 색상
 * sRGB 색상 공간을 사용
 * GPU 연산 시에는 이 값이 더 빠름.
 */
struct FColor
{
    union
    {
        struct
        {
            uint8 B;
            uint8 G;
            uint8 R;
            uint8 A;
        };
        uint32 Bits;
    };

    uint32& DWColor() { return Bits; }
    const uint32& DWColor() const { return Bits; }

    FColor() : B(0), G(0), R(0), A(255) {}
    FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255) : B(InB), G(InG), R(InR), A(InA) {}
    FColor(uint32 InColor)
    {
        DWColor() = InColor;
    }

    // Serializer
    friend FArchive& operator<<(FArchive& Ar, FColor& Color)
    {
        return Ar << Color.DWColor();
    }

    bool Serialize(FArchive& Ar)
    {
        Ar << *this;
        return true;
    }

    // Operators
    FORCEINLINE bool operator==(const FColor& Other) const
    {
        return DWColor() == Other.DWColor();
    }

    FORCEINLINE bool operator!=(const FColor& Other) const
    {
        return DWColor() != Other.DWColor();
    }

    FORCEINLINE void operator+=(const FColor& Other)
    {
        R = (uint8)FMath::Min((int32)R + (int32)Other.R, 255);
        G = (uint8)FMath::Min((int32)G + (int32)Other.G, 255);
        B = (uint8)FMath::Min((int32)B + (int32)Other.B, 255);
        A = (uint8)FMath::Min((int32)A + (int32)Other.A, 255);
    }


    static const FColor White;
    static const FColor Black;
    static const FColor Transparent;
    static const FColor Red;
    static const FColor Green;
    static const FColor Blue;
    static const FColor Yellow;
    static const FColor Cyan;
    static const FColor Magenta;
    static const FColor Orange;
    static const FColor Purple;
    static const FColor Turquoise;
    static const FColor Silver;
    static const FColor Emerald;
};

struct FLinearColor
{
    float R;
    float G;
    float B;
    float A;
    FLinearColor() : R(0), G(0), B(0), A(0) {}
    constexpr FLinearColor(float InR, float InG, float InB, float InA = 1.0f) : R(InR), G(InG), B(InB), A(InA) {}
    FLinearColor(const FString& SourceString)
        : R(0), G(0), B(0), A(0)
    {
        InitFromString(SourceString);
    }
    

    // FVector4 생성자와의 혼동을 피하기 위해 explicit으로 유지할 것.
    explicit FLinearColor(const FVector& InVector) : R(InVector.X), G(InVector.Y), B(InVector.Z), A(1.0f) {}

    // FVector 생성자와의 혼동을 피하기 위해 explicit으로 유지할 것.
    explicit FLinearColor(const FVector4& InVector) : R(InVector.X), G(InVector.Y), B(InVector.Z), A(InVector.W) {}

    constexpr FLinearColor(const FColor& InColor)
        : R(InColor.R / 255.0f), G(InColor.G / 255.0f), B(InColor.B / 255.0f), A(InColor.A / 255.0f) {
    }

    // Serializer
    friend FArchive& operator<<(FArchive& Ar, FLinearColor& Color)
    {
        return Ar << Color.R << Color.G << Color.B << Color.A;
    }

    bool Serialize(FArchive& Ar)
    {
        Ar << *this;
        return true;
    }

    // Default Colors
    static const FLinearColor White;
    static const FLinearColor Gray;
    static const FLinearColor Black;
    static const FLinearColor Transparent;
    static const FLinearColor Red;
    static const FLinearColor Green;
    static const FLinearColor Blue;
    static const FLinearColor Yellow;

    FORCEINLINE static FLinearColor FromColor(const FColor& InColor)
    {
        return FLinearColor(InColor);
    }

    // Operators
    FORCEINLINE FLinearColor operator+(const FLinearColor& Other) const
    {
        return FLinearColor(R + Other.R, G + Other.G, B + Other.B, A + Other.A);
    }

    FORCEINLINE FLinearColor& operator+=(const FLinearColor& Other)
    {
        R += Other.R;
        G += Other.G;
        B += Other.B;
        A += Other.A;
        return *this;
    }

    FORCEINLINE FLinearColor operator-(const FLinearColor& Other) const
    {
        return FLinearColor(R - Other.R, G - Other.G, B - Other.B, A - Other.A);
    }

    FORCEINLINE FLinearColor& operator-=(const FLinearColor& Other)
    {
        R -= Other.R;
        G -= Other.G;
        B -= Other.B;
        A -= Other.A;
        return *this;
    }

    FORCEINLINE FLinearColor operator*(const FLinearColor& Other) const
    {
        return FLinearColor(
            R * Other.R,
            G * Other.G,
            B * Other.B,
            A * Other.A
        );
    }

    FORCEINLINE FLinearColor& operator*=(const FLinearColor& Other)
    {
        R *= Other.R;
        G *= Other.G;
        B *= Other.B;
        A *= Other.A;
        return *this;
    }

    FORCEINLINE FLinearColor operator*(float Scalar) const
    {
        return FLinearColor(
            R * Scalar,
            G * Scalar,
            B * Scalar,
            A * Scalar
        );
    }

    FORCEINLINE FLinearColor& operator*=(float Scalar)
    {
        R *= Scalar;
        G *= Scalar;
        B *= Scalar;
        A *= Scalar;
        return *this;
    }

    FORCEINLINE FLinearColor operator/(const FLinearColor& Other) const
    {
        return FLinearColor(R / Other.R, G / Other.G, B / Other.B, A / Other.A);
    }

    FORCEINLINE FLinearColor& operator/=(const FLinearColor& Other)
    {
        R /= Other.R;
        G /= Other.G;
        B /= Other.B;
        A /= Other.A;
        return *this;
    }

    FORCEINLINE FLinearColor operator/(float Scalar) const
    {
        const float InvScalar = 1.0f / Scalar;
        return FLinearColor(
            this->R * InvScalar,
            this->G * InvScalar,
            this->B * InvScalar,
            this->A * InvScalar
        );
    }

    FORCEINLINE FLinearColor& operator/=(float Scalar)
    {
        const float InvScalar = 1.0f / Scalar;
        R /= InvScalar;
        G /= InvScalar;
        B /= InvScalar;
        A /= InvScalar;
        return *this;
    }

    FORCEINLINE bool operator==(const FLinearColor& Other) const
    {
        return (R == Other.R && G == Other.G && B == Other.B && A == Other.A);
    }

    FORCEINLINE bool operator!=(const FLinearColor& Other) const
    {
        return !(*this == Other);
    }

    FORCEINLINE FLinearColor GetClamp(float InMin = 0.0f, float InMax = 1.0f) const
    {
        return FLinearColor(
            FMath::Clamp(R, InMin, InMax),
            FMath::Clamp(G, InMin, InMax),
            FMath::Clamp(B, InMin, InMax),
            FMath::Clamp(A, InMin, InMax)
        );
    }

    FORCEINLINE bool Equals(const FLinearColor& Other, float Tolerance = KINDA_SMALL_NUMBER) const
    {
        return (FMath::Abs(R - Other.R) <= Tolerance &&
            FMath::Abs(G - Other.G) <= Tolerance &&
            FMath::Abs(B - Other.B) <= Tolerance &&
            FMath::Abs(A - Other.A) <= Tolerance);
    }

    FORCEINLINE void Lerp(const FLinearColor& Start, const FLinearColor& End, float Alpha)
    {
        R = FMath::Lerp(Start.R, End.R, Alpha);
        G = FMath::Lerp(Start.G, End.G, Alpha);
        B = FMath::Lerp(Start.B, End.B, Alpha);
        A = FMath::Lerp(Start.A, End.A, Alpha);
    }

    FORCEINLINE float GetMax() const
    {
        return FMath::Max(R, FMath::Max(G, FMath::Max(B, A)));
    }

    FORCEINLINE float GetMin() const
    {
        return FMath::Min(R, FMath::Min(G, FMath::Min(B, A)));
    }

    FString ToString() const;
    bool InitFromString(const FString& SourceString);
};
