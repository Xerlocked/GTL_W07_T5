#pragma once
#include <cassert>
#include "MathUtility.h"
#include "Serialization/Archive.h"

#include "Rotator.h"

struct FVector2D
{
	float X, Y;

    FVector2D() : X(0), Y(0) {}
	FVector2D(float InX, float InY) : X(InX), Y(InY) {}
    FVector2D(float Scalar) : X(Scalar), Y(Scalar) {}

    static const FVector2D ZeroVector;
    static const FVector2D OneVector;

public:
    FVector2D operator+(const FVector2D& Rhs) const
    {
        return {
            X + Rhs.X,
            Y + Rhs.Y
        };
    }

    FVector2D operator-(const FVector2D& Rhs) const
    {
        return {
            X - Rhs.X,
            Y - Rhs.Y
        };
    }

    FVector2D operator*(float Scalar) const
    {
        return {
            X * Scalar,
            Y * Scalar
        };
    }

    FVector2D operator/(float Scalar) const
    {
        return {
            X / Scalar,
            Y / Scalar
        };
    }

    FVector2D& operator+=(const FVector2D& Rhs)
    {
        X += Rhs.X;
        Y += Rhs.Y;
        return *this;
    }

    bool operator==(const FVector2D& Vector2D) const = default;
    bool operator!=(const FVector2D& Vector2D) const = default;

    /**
    * Get a textual representation of the vector.
    *
    * @return Text describing the vector.
    */
    FString ToString() const;

    /**
    * Initialize this Vector based on an FString. The String is expected to contain X=, Y=.
    * The TVector2<T> will be bogus when InitFromString returns false.
    *
    * @param	InSourceString	FString containing the vector values.
    * @return true if the X,Y values were read successfully; false otherwise.
    */
    bool InitFromString(const FString& InSourceString);
};

// 3D 벡터
struct FVector
{
    float X, Y, Z;

    FVector() : X(0), Y(0), Z(0) {}
    FVector(float X, float Y, float Z) : X(X), Y(Y), Z(Z) {}
    explicit FVector(float Scalar) : X(Scalar), Y(Scalar), Z(Scalar) {}

    
    FVector(const FRotator& InRotator);

    // Vector(0, 0, 0)
    static const FVector ZeroVector;

    // Vector(1, 1, 1)
    static const FVector OneVector;

    // Vector(0, 0, 1)
    static const FVector UpVector;

    // Vector(0, 0, -1)
    static const FVector DownVector;

    // Vector(1, 0, 0)
    static const FVector ForwardVector;

    // Vector(-1, 0, 0)
    static const FVector BackwardVector;

    // Vector(0, 1, 0)
    static const FVector RightVector;

    // Vector(0, -1, 0)
    static const FVector LeftVector;

    // Unit X Axis Vector (1, 0, 0)
    static const FVector XAxisVector;
    // Unit Y Axis Vector (0, 1, 0)
    static const FVector YAxisVector;
    // Unit Z Axis Vector (0, 0, 1)
    static const FVector ZAxisVector;

public:
    static inline FVector Zero() { return ZeroVector; }
    static inline FVector One() { return OneVector; }

    static inline FVector UnitX() { return XAxisVector; }
    static inline FVector UnitY() { return YAxisVector; }
    static inline FVector UnitZ() { return ZAxisVector; }

    static float Distance(const FVector& V1, const FVector& V2);

    /** Dot Product */
    float operator|(const FVector& Other) const;
    float Dot(const FVector& Other) const;
    static float DotProduct(const FVector& A, const FVector& B);

    /** Cross Product */
    FVector operator^(const FVector& Other) const;
    FVector Cross(const FVector& Other) const;
    static FVector CrossProduct(const FVector& A, const FVector& B);

    FVector operator+(const FVector& Other) const;
    FVector& operator+=(const FVector& Other);

    FVector operator-(const FVector& Other) const;
    FVector& operator-=(const FVector& Other);

    FVector operator*(const FVector& Other) const;
    FVector operator*(float Scalar) const;
    FVector& operator*=(float Scalar);

    FVector operator/(const FVector& Other) const;
    FVector operator/(float Scalar) const;
    FVector& operator/=(float Scalar);

    FVector operator-() const;

    bool operator==(const FVector& Other) const;
    bool operator!=(const FVector& Other) const;

    float& operator[](int Index);
    const float& operator[](int Index) const;

    
public:
    bool Equals(const FVector& V, float Tolerance = KINDA_SMALL_NUMBER) const;
    bool AllComponentsEqual(float Tolerance = KINDA_SMALL_NUMBER) const;

    float Length() const;
    float LengthSquared() const;

    bool Normalize(float Tolerance = SMALL_NUMBER);

    FVector GetUnsafeNormal() const;
    FVector GetSafeNormal(float Tolerance = SMALL_NUMBER) const;

    FVector ComponentMin(const FVector& Other) const;
    FVector ComponentMax(const FVector& Other) const;

    bool IsNearlyZero(float Tolerance = SMALL_NUMBER) const;
    bool IsZero() const;

    
    FString ToString() const;
    bool InitFromString(const FString& InSourceString);
};

inline FVector::FVector(const FRotator& InRotator)
    : X(FMath::DegreesToRadians(InRotator.Roll)), Y(FMath::DegreesToRadians(InRotator.Pitch)), Z(FMath::DegreesToRadians(InRotator.Yaw))
{
}

inline float FVector::Distance(const FVector& V1, const FVector& V2)
{
    return FMath::Sqrt(
        FMath::Square(V2.X - V1.X)
        + FMath::Square(V2.Y - V1.Y)
        + FMath::Square(V2.Z - V1.Z)
    );
}

inline float FVector::operator|(const FVector& Other) const
{
    return X * Other.X + Y * Other.Y + Z * Other.Z;
}

inline float FVector::Dot(const FVector& Other) const
{
    return *this | Other;
}

inline float FVector::DotProduct(const FVector& A, const FVector& B)
{
    return A | B;
}

inline FVector FVector::operator^(const FVector& Other) const
{
    return {
        Y * Other.Z - Z * Other.Y,
        Z * Other.X - X * Other.Z,
        X * Other.Y - Y * Other.X
    };
}

inline FVector FVector::Cross(const FVector& Other) const
{
    return *this ^ Other;
}

inline FVector FVector::CrossProduct(const FVector& A, const FVector& B)
{
    return A ^ B;
}

inline FVector FVector::operator+(const FVector& Other) const
{
    return {X + Other.X, Y + Other.Y, Z + Other.Z};
}

inline FVector& FVector::operator+=(const FVector& Other)
{
    X += Other.X; Y += Other.Y; Z += Other.Z;
    return *this;
}

inline FVector FVector::operator-(const FVector& Other) const
{
    return {X - Other.X, Y - Other.Y, Z - Other.Z};
}

inline FVector& FVector::operator-=(const FVector& Other)
{
    X -= Other.X; Y -= Other.Y; Z -= Other.Z;
    return *this;
}

inline FVector FVector::operator*(const FVector& Other) const
{
    return {X * Other.X, Y * Other.Y, Z * Other.Z};
}

inline FVector FVector::operator*(float Scalar) const
{
    return {X * Scalar, Y * Scalar, Z * Scalar};
}

inline FVector& FVector::operator*=(float Scalar)
{
    X *= Scalar; Y *= Scalar; Z *= Scalar;
    return *this;
}

inline FVector FVector::operator/(const FVector& Other) const
{
    return {X / Other.X, Y / Other.Y, Z / Other.Z};
}

inline FVector FVector::operator/(float Scalar) const
{
    return {X / Scalar, Y / Scalar, Z / Scalar};
}

inline FVector& FVector::operator/=(float Scalar)
{
    X /= Scalar; Y /= Scalar; Z /= Scalar;
    return *this;
}

inline FVector FVector::operator-() const
{
    return {-X, -Y, -Z};
}

inline bool FVector::operator==(const FVector& Other) const
{
    return X == Other.X && Y == Other.Y && Z == Other.Z;
}

inline bool FVector::operator!=(const FVector& Other) const
{
    return X != Other.X || Y != Other.Y || Z != Other.Z;
}

inline float& FVector::operator[](int Index)
{
    assert(0 <= Index && Index <= 2);
    return reinterpret_cast<float*>(this)[Index];
}

inline const float& FVector::operator[](int Index) const
{
    assert(0 <= Index && Index <= 2);
    return reinterpret_cast<const float*>(this)[Index];
}

inline bool FVector::Equals(const FVector& V, float Tolerance) const
{
    return FMath::Abs(X-V.X) <= Tolerance && FMath::Abs(Y-V.Y) <= Tolerance && FMath::Abs(Z-V.Z) <= Tolerance;
}

inline bool FVector::AllComponentsEqual(float Tolerance) const
{
    return FMath::Abs(X - Y) <= Tolerance && FMath::Abs(X - Z) <= Tolerance && FMath::Abs(Y - Z) <= Tolerance;
}

inline float FVector::Length() const
{
    return FMath::Sqrt(X * X + Y * Y + Z * Z);
}

inline float FVector::LengthSquared() const
{
    return X * X + Y * Y + Z * Z;
}

inline bool FVector::Normalize(float Tolerance)
{
    const float SquareSum = X * X + Y * Y + Z * Z;
    if (SquareSum > Tolerance)
    {
        const float Scale = FMath::InvSqrt(SquareSum);
        X *= Scale; Y *= Scale; Z *= Scale;
        return true;
    }
    return false;
}

inline FVector FVector::GetUnsafeNormal() const
{
    const float Scale = FMath::InvSqrt(X*X + Y*Y + Z*Z);
    return {X * Scale, Y * Scale, Z * Scale};
}

inline FVector FVector::GetSafeNormal(float Tolerance) const
{
    const float SquareSum = X*X + Y*Y + Z*Z;

    // Not sure if it's safe to add tolerance in there. Might introduce too many errors
    if (SquareSum == 1.f)
    {
        return *this;
    }
    else if (SquareSum < Tolerance)
    {
        return ZeroVector;
    }
    const float Scale = FMath::InvSqrt(SquareSum);
    return {X * Scale, Y * Scale, Z * Scale};
}

inline FVector FVector::ComponentMin(const FVector& Other) const
{
    return {
        FMath::Min(X, Other.X),
        FMath::Min(Y, Other.Y),
        FMath::Min(Z, Other.Z)
    };
}

inline FVector FVector::ComponentMax(const FVector& Other) const
{
    return {
        FMath::Max(X, Other.X),
        FMath::Max(Y, Other.Y),
        FMath::Max(Z, Other.Z)
    };
}

inline bool FVector::IsNearlyZero(float Tolerance) const
{
    return
        FMath::Abs(X)<=Tolerance
        &&	FMath::Abs(Y)<=Tolerance
        &&	FMath::Abs(Z)<=Tolerance;
}

inline bool FVector::IsZero() const
{
    return X==0.f && Y==0.f && Z==0.f;
}



inline FArchive& operator<<(FArchive& Ar, FVector2D& V)
{
    return Ar << V.X << V.Y;
}

inline FArchive& operator<<(FArchive& Ar, FVector& V)
{
    return Ar << V.X << V.Y << V.Z;
}


