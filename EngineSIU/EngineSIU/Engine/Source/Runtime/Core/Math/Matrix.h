#pragma once
#include "Serialization/Archive.h"

struct FVector;
struct FVector4;
struct FRotator;
struct FQuat;

#include "Vector.h"
#include "Vector4.h"

// 4x4 행렬 연산
struct alignas(16) FMatrix
{
public:
    alignas(16) float M[4][4];

public:
    static const FMatrix Identity;

public:
    // 기본 연산자 오버로딩
    FMatrix operator+(const FMatrix& Other) const;
    FMatrix operator-(const FMatrix& Other) const;
    FMatrix operator*(const FMatrix& Other) const;
    FMatrix operator*(float Scalar) const;
    FMatrix operator/(float Scalar) const;
    float* operator[](int row);
    const float* operator[](int row) const;

    // 유틸리티 함수
    static FMatrix Transpose(const FMatrix& Mat);
    static FMatrix Inverse(const FMatrix& Mat);
    static FMatrix CreateRotationMatrix(float roll, float pitch, float yaw);
    static FMatrix CreateScaleMatrix(float scaleX, float scaleY, float scaleZ);
    static FVector TransformVector(const FVector& v, const FMatrix& m);
    static FVector4 TransformVector(const FVector4& v, const FMatrix& m);
    static FMatrix CreateTranslationMatrix(const FVector& position);

    FVector4 TransformFVector4(const FVector4& vector) const;
    FVector TransformPosition(const FVector& vector) const;

    /** 월드 좌표상의 방향 벡터를 가져옵니다.
     * 
     *  @param i : 해당 하는 방향 벡터
     *  
     *  (0: Forward, 1: Right, 2: Up)
     */
    FVector GetAxis(int32 i) const;
    
    static FMatrix GetScaleMatrix(const FVector& InScale);
    static FMatrix GetTranslationMatrix(const FVector& InPosition);
    static FMatrix GetRotationMatrix(const FRotator& InRotation);
    static FMatrix GetRotationMatrix(const FQuat& InRotation);

    FQuat ToQuat(const FMatrix& M) const;
};

inline FArchive& operator<<(FArchive& Ar, FMatrix& M)
{
    Ar << M.M[0][0] << M.M[0][1] << M.M[0][2] << M.M[0][3];
    Ar << M.M[1][0] << M.M[1][1] << M.M[1][2] << M.M[1][3];
    Ar << M.M[2][0] << M.M[2][1] << M.M[2][2] << M.M[2][3];
    Ar << M.M[3][0] << M.M[3][1] << M.M[3][2] << M.M[3][3];
    return Ar;
}
