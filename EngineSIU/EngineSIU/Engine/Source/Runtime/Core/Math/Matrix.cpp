#include "Matrix.h"

#include "MathSSE.h"
#include "MathUtility.h"
#include "Vector.h"
#include "Vector4.h"
#include "Quat.h"
#include "Rotator.h"
#include "HAL/PlatformType.h"
#include <cmath>


// 단위 행렬 정의
const FMatrix FMatrix::Identity = { {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
} };

// 행렬 덧셈
FMatrix FMatrix::operator+(const FMatrix& Other) const {
    FMatrix Result;
    for (int32 i = 0; i < 4; i++)
        for (int32 j = 0; j < 4; j++)
            Result.M[i][j] = M[i][j] + Other.M[i][j];
    return Result;
}

// 행렬 뺄셈
FMatrix FMatrix::operator-(const FMatrix& Other) const {
    FMatrix Result;
    for (int32 i = 0; i < 4; i++)
        for (int32 j = 0; j < 4; j++)
            Result.M[i][j] = M[i][j] - Other.M[i][j];
    return Result;
}

// 행렬 곱셈
FMatrix FMatrix::operator*(const FMatrix& Other) const {
    FMatrix Result = {};
    SSE::VectorMatrixMultiply(&Result, this, &Other);
    return Result;
}

// 스칼라 곱셈
FMatrix FMatrix::operator*(float Scalar) const {
    FMatrix Result;
    for (int32 i = 0; i < 4; ++i)
    {
        Result.M[i][0] = M[i][0] * Scalar;
        Result.M[i][1] = M[i][1] * Scalar;
        Result.M[i][2] = M[i][2] * Scalar;
        Result.M[i][3] = M[i][3] * Scalar;
    }
    return Result;
}

// 스칼라 나눗셈
FMatrix FMatrix::operator/(float Scalar) const {
    FMatrix Result;
    for (int32 i = 0; i < 4; i++)
        for (int32 j = 0; j < 4; j++)
            Result.M[i][j] = M[i][j] / Scalar;
    return Result;
}

float* FMatrix::operator[](int row) {
    return M[row];
}

const float* FMatrix::operator[](int row) const
{
    return M[row];
}

// 전치 행렬
FMatrix FMatrix::Transpose(const FMatrix& Mat) {
    FMatrix Result;
    for (int32 i = 0; i < 4; i++)
        for (int32 j = 0; j < 4; j++)
            Result.M[i][j] = Mat.M[j][i];
    return Result;
}

FMatrix FMatrix::Inverse(const FMatrix& Mat)
{
	FMatrix Result;
	FMatrix Tmp;
	float Det[4];

	Tmp[0][0] = Mat[2][2] * Mat[3][3] - Mat[2][3] * Mat[3][2];
	Tmp[0][1] = Mat[1][2] * Mat[3][3] - Mat[1][3] * Mat[3][2];
	Tmp[0][2] = Mat[1][2] * Mat[2][3] - Mat[1][3] * Mat[2][2];

	Tmp[1][0] = Mat[2][2] * Mat[3][3] - Mat[2][3] * Mat[3][2];
	Tmp[1][1] = Mat[0][2] * Mat[3][3] - Mat[0][3] * Mat[3][2];
	Tmp[1][2] = Mat[0][2] * Mat[2][3] - Mat[0][3] * Mat[2][2];

	Tmp[2][0] = Mat[1][2] * Mat[3][3] - Mat[1][3] * Mat[3][2];
	Tmp[2][1] = Mat[0][2] * Mat[3][3] - Mat[0][3] * Mat[3][2];
	Tmp[2][2] = Mat[0][2] * Mat[1][3] - Mat[0][3] * Mat[1][2];

	Tmp[3][0] = Mat[1][2] * Mat[2][3] - Mat[1][3] * Mat[2][2];
	Tmp[3][1] = Mat[0][2] * Mat[2][3] - Mat[0][3] * Mat[2][2];
	Tmp[3][2] = Mat[0][2] * Mat[1][3] - Mat[0][3] * Mat[1][2];

	Det[0] = Mat[1][1] * Tmp[0][0] - Mat[2][1] * Tmp[0][1] + Mat[3][1] * Tmp[0][2];
	Det[1] = Mat[0][1] * Tmp[1][0] - Mat[2][1] * Tmp[1][1] + Mat[3][1] * Tmp[1][2];
	Det[2] = Mat[0][1] * Tmp[2][0] - Mat[1][1] * Tmp[2][1] + Mat[3][1] * Tmp[2][2];
	Det[3] = Mat[0][1] * Tmp[3][0] - Mat[1][1] * Tmp[3][1] + Mat[2][1] * Tmp[3][2];

	const float Determinant = Mat[0][0] * Det[0] - Mat[1][0] * Det[1] + Mat[2][0] * Det[2] - Mat[3][0] * Det[3];
	
    if ( Determinant == 0.0f || !std::isfinite(Determinant) )
	{
		return Identity;
	}

	const float	RDet = 1.0f / Determinant;

	Result[0][0] = RDet * Det[0];
	Result[0][1] = -RDet * Det[1];
	Result[0][2] = RDet * Det[2];
	Result[0][3] = -RDet * Det[3];
	Result[1][0] = -RDet * (Mat[1][0] * Tmp[0][0] - Mat[2][0] * Tmp[0][1] + Mat[3][0] * Tmp[0][2]);
	Result[1][1] = RDet * (Mat[0][0] * Tmp[1][0] - Mat[2][0] * Tmp[1][1] + Mat[3][0] * Tmp[1][2]);
	Result[1][2] = -RDet * (Mat[0][0] * Tmp[2][0] - Mat[1][0] * Tmp[2][1] + Mat[3][0] * Tmp[2][2]);
	Result[1][3] = RDet * (Mat[0][0] * Tmp[3][0] - Mat[1][0] * Tmp[3][1] + Mat[2][0] * Tmp[3][2]);
	Result[2][0] = RDet * (
		Mat[1][0] * (Mat[2][1] * Mat[3][3] - Mat[2][3] * Mat[3][1]) -
		Mat[2][0] * (Mat[1][1] * Mat[3][3] - Mat[1][3] * Mat[3][1]) +
		Mat[3][0] * (Mat[1][1] * Mat[2][3] - Mat[1][3] * Mat[2][1])
    );
	Result[2][1] = -RDet * (
		Mat[0][0] * (Mat[2][1] * Mat[3][3] - Mat[2][3] * Mat[3][1]) -
		Mat[2][0] * (Mat[0][1] * Mat[3][3] - Mat[0][3] * Mat[3][1]) +
		Mat[3][0] * (Mat[0][1] * Mat[2][3] - Mat[0][3] * Mat[2][1])
    );
	Result[2][2] = RDet * (
		Mat[0][0] * (Mat[1][1] * Mat[3][3] - Mat[1][3] * Mat[3][1]) -
		Mat[1][0] * (Mat[0][1] * Mat[3][3] - Mat[0][3] * Mat[3][1]) +
		Mat[3][0] * (Mat[0][1] * Mat[1][3] - Mat[0][3] * Mat[1][1])
    );
	Result[2][3] = -RDet * (
		Mat[0][0] * (Mat[1][1] * Mat[2][3] - Mat[1][3] * Mat[2][1]) -
		Mat[1][0] * (Mat[0][1] * Mat[2][3] - Mat[0][3] * Mat[2][1]) +
		Mat[2][0] * (Mat[0][1] * Mat[1][3] - Mat[0][3] * Mat[1][1])
    );
	Result[3][0] = -RDet * (
		Mat[1][0] * (Mat[2][1] * Mat[3][2] - Mat[2][2] * Mat[3][1]) -
		Mat[2][0] * (Mat[1][1] * Mat[3][2] - Mat[1][2] * Mat[3][1]) +
		Mat[3][0] * (Mat[1][1] * Mat[2][2] - Mat[1][2] * Mat[2][1])
    );
	Result[3][1] = RDet * (
		Mat[0][0] * (Mat[2][1] * Mat[3][2] - Mat[2][2] * Mat[3][1]) -
		Mat[2][0] * (Mat[0][1] * Mat[3][2] - Mat[0][2] * Mat[3][1]) +
		Mat[3][0] * (Mat[0][1] * Mat[2][2] - Mat[0][2] * Mat[2][1])
    );
	Result[3][2] = -RDet * (
		Mat[0][0] * (Mat[1][1] * Mat[3][2] - Mat[1][2] * Mat[3][1]) -
		Mat[1][0] * (Mat[0][1] * Mat[3][2] - Mat[0][2] * Mat[3][1]) +
		Mat[3][0] * (Mat[0][1] * Mat[1][2] - Mat[0][2] * Mat[1][1])
    );
	Result[3][3] = RDet * (
		Mat[0][0] * (Mat[1][1] * Mat[2][2] - Mat[1][2] * Mat[2][1]) -
		Mat[1][0] * (Mat[0][1] * Mat[2][2] - Mat[0][2] * Mat[2][1]) +
		Mat[2][0] * (Mat[0][1] * Mat[1][2] - Mat[0][2] * Mat[1][1])
    );

    return Result;
}

FMatrix FMatrix::CreateRotationMatrix(float roll, float pitch, float yaw)
{
    float radRoll = roll * (PI / 180.0f);
    float radPitch = pitch * (PI / 180.0f);
    float radYaw = yaw * (PI / 180.0f);

    float cosRoll = FMath::Cos(radRoll), sinRoll = FMath::Sin(radRoll);
    float cosPitch = FMath::Cos(radPitch), sinPitch = FMath::Sin(radPitch);
    float cosYaw = FMath::Cos(radYaw), sinYaw = FMath::Sin(radYaw);

    // Z축 (Yaw) 회전
    FMatrix rotationZ = { {
        { cosYaw, sinYaw, 0, 0 },
        { -sinYaw, cosYaw, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    } };

    // Y축 (Pitch) 회전
    FMatrix rotationY = { {
        { cosPitch, 0, sinPitch, 0 },
        { 0, 1, 0, 0 },
        { -sinPitch, 0, cosPitch, 0 },
        { 0, 0, 0, 1 }
    } };

    // X축 (Roll) 회전
    FMatrix rotationX = { {
        { 1, 0, 0, 0 },
        { 0, cosRoll, -sinRoll, 0 },
        { 0, sinRoll, cosRoll, 0 },
        { 0, 0, 0, 1 }
    } };

    // DirectX 표준 순서: Z(Yaw) → Y(Pitch) → X(Roll)  
    return rotationX * rotationY * rotationZ;  // 이렇게 하면  오른쪽 부터 적용됨
}


// 스케일 행렬 생성
FMatrix FMatrix::CreateScaleMatrix(float scaleX, float scaleY, float scaleZ)
{
    return { {
        { scaleX, 0, 0, 0 },
        { 0, scaleY, 0, 0 },
        { 0, 0, scaleZ, 0 },
        { 0, 0, 0, 1 }
    } };
}

FMatrix FMatrix::CreateTranslationMatrix(const FVector& position)
{
    FMatrix translationMatrix = FMatrix::Identity;
    translationMatrix.M[3][0] = position.X;
    translationMatrix.M[3][1] = position.Y;
    translationMatrix.M[3][2] = position.Z;
    return translationMatrix;
}

FVector FMatrix::TransformVector(const FVector& v, const FMatrix& m)
{
    FVector result;

    // 4x4 행렬을 사용하여 벡터 변환 (W = 0으로 가정, 방향 벡터)
    result.X = v.X * m.M[0][0] + v.Y * m.M[1][0] + v.Z * m.M[2][0] + 0.0f * m.M[3][0];
    result.Y = v.X * m.M[0][1] + v.Y * m.M[1][1] + v.Z * m.M[2][1] + 0.0f * m.M[3][1];
    result.Z = v.X * m.M[0][2] + v.Y * m.M[1][2] + v.Z * m.M[2][2] + 0.0f * m.M[3][2];


    return result;
}

// FVector4를 변환하는 함수
FVector4 FMatrix::TransformVector(const FVector4& v, const FMatrix& m)
{
    FVector4 result;
    result.X = v.X * m.M[0][0] + v.Y * m.M[1][0] + v.Z * m.M[2][0] + v.W * m.M[3][0];
    result.Y = v.X * m.M[0][1] + v.Y * m.M[1][1] + v.Z * m.M[2][1] + v.W * m.M[3][1];
    result.Z = v.X * m.M[0][2] + v.Y * m.M[1][2] + v.Z * m.M[2][2] + v.W * m.M[3][2];
    result.W = v.X * m.M[0][3] + v.Y * m.M[1][3] + v.Z * m.M[2][3] + v.W * m.M[3][3];
    return result;
}

FVector4 FMatrix::TransformFVector4(const FVector4& vector) const
{
    return FVector4(
        M[0][0] * vector.X + M[1][0] * vector.Y + M[2][0] * vector.Z + M[3][0] * vector.W,
        M[0][1] * vector.X + M[1][1] * vector.Y + M[2][1] * vector.Z + M[3][1] * vector.W,
        M[0][2] * vector.X + M[1][2] * vector.Y + M[2][2] * vector.Z + M[3][2] * vector.W,
        M[0][3] * vector.X + M[1][3] * vector.Y + M[2][3] * vector.Z + M[3][3] * vector.W
    );
}

FVector FMatrix::TransformPosition(const FVector& vector) const
{
    float x = M[0][0] * vector.X + M[1][0] * vector.Y + M[2][0] * vector.Z + M[3][0];
    float y = M[0][1] * vector.X + M[1][1] * vector.Y + M[2][1] * vector.Z + M[3][1];
    float z = M[0][2] * vector.X + M[1][2] * vector.Y + M[2][2] * vector.Z + M[3][2];
    float w = M[0][3] * vector.X + M[1][3] * vector.Y + M[2][3] * vector.Z + M[3][3];
    return w != 0.0f ? FVector{x / w, y / w, z / w} : FVector{x, y, z};
}

FVector FMatrix::GetAxis(int32 i) const
{
    return {M[0][i], M[1][i], M[2][i]};
}

FMatrix FMatrix::GetScaleMatrix(const FVector& InScale)
{
    return CreateScaleMatrix(InScale.X, InScale.Y, InScale.Z);
}

FMatrix FMatrix::GetTranslationMatrix(const FVector& position)
{
    return CreateTranslationMatrix(position);
}

FMatrix FMatrix::GetRotationMatrix(const FRotator& InRotation)
{
    return CreateRotationMatrix(InRotation.Roll, InRotation.Pitch, InRotation.Yaw);
}

FMatrix FMatrix::GetRotationMatrix(const FQuat& InRotation)
{
    // 쿼터니언 요소 추출
    const float QuatX = InRotation.X, QuatY = InRotation.Y, QuatZ = InRotation.Z, w = InRotation.W;

    // 중간 계산값
    const float xx = QuatX * QuatX;
    float yy = QuatY * QuatY;
    float zz = QuatZ * QuatZ;
    
    const float xy = QuatX * QuatY;
    float xz = QuatX * QuatZ;
    float yz = QuatY * QuatZ;
    const float wx = w * QuatX;
    const float wy = w * QuatY;
    const float wz = w * QuatZ;

    // 회전 행렬 구성
    FMatrix Result;

    Result.M[0][0] = 1.0f - 2.0f * (yy + zz);
    Result.M[0][1] = 2.0f * (xy - wz);
    Result.M[0][2] = 2.0f * (xz + wy);
    Result.M[0][3] = 0.0f;

    Result.M[1][0] = 2.0f * (xy + wz);
    Result.M[1][1] = 1.0f - 2.0f * (xx + zz);
    Result.M[1][2] = 2.0f * (yz - wx);
    Result.M[1][3] = 0.0f;

    Result.M[2][0] = 2.0f * (xz - wy);
    Result.M[2][1] = 2.0f * (yz + wx);
    Result.M[2][2] = 1.0f - 2.0f * (xx + yy);
    Result.M[2][3] = 0.0f;

    Result.M[3][0] = 0.0f;
    Result.M[3][1] = 0.0f;
    Result.M[3][2] = 0.0f;
    Result.M[3][3] = 1.0f; // 4x4 행렬이므로 마지막 값은 1

    return Result;
}

FQuat FMatrix::ToQuat(const FMatrix& M) const
{
    return FQuat(M);
}
