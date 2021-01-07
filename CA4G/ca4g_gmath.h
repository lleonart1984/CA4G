#ifndef CA4G_GMATH_H
#define CA4G_GMATH_H

#include "ca4g_math.h"

namespace CA4G {

	#pragma region cross
	static float3 cross(const float3 &pto1, const float3 &pto2)
	{
		return float3(
			pto1.y * pto2.z - pto1.z * pto2.y,
			pto1.z * pto2.x - pto1.x * pto2.z,
			pto1.x * pto2.y - pto1.y * pto2.x);
	}
	#pragma endregion

	#pragma region determinant

	static float determinant(const float1x1 &m)
	{
		return m._m00;
	}

	static float determinant(const float2x2 &m)
	{
		return m._m00 * m._m11 - m._m01 * m._m10;
	}

	static float determinant(const float3x3 &m)
	{
		/// 00 01 02
		/// 10 11 12
		/// 20 21 22
		float Min00 = m._m11 * m._m22 - m._m12 * m._m21;
		float Min01 = m._m10 * m._m22 - m._m12 * m._m20;
		float Min02 = m._m10 * m._m21 - m._m11 * m._m20;

		return Min00 * m._m00 - Min01 * m._m01 + Min02 * m._m02;
	}

	static float determinant(const float4x4 &m)
	{
		float Min00 = m._m11 * m._m22 * m._m33 + m._m12 * m._m23 * m._m31 + m._m13 * m._m21 * m._m32 - m._m11 * m._m23 * m._m32 - m._m12 * m._m21 * m._m33 - m._m13 * m._m22 * m._m31;
		float Min01 = m._m10 * m._m22 * m._m33 + m._m12 * m._m23 * m._m30 + m._m13 * m._m20 * m._m32 - m._m10 * m._m23 * m._m32 - m._m12 * m._m20 * m._m33 - m._m13 * m._m22 * m._m30;
		float Min02 = m._m10 * m._m21 * m._m33 + m._m11 * m._m23 * m._m30 + m._m13 * m._m20 * m._m31 - m._m10 * m._m23 * m._m31 - m._m11 * m._m20 * m._m33 - m._m13 * m._m21 * m._m30;
		float Min03 = m._m10 * m._m21 * m._m32 + m._m11 * m._m22 * m._m30 + m._m12 * m._m20 * m._m31 - m._m10 * m._m22 * m._m31 - m._m11 * m._m20 * m._m32 - m._m12 * m._m21 * m._m30;

		return Min00 * m._m00 - Min01 * m._m01 + Min02 * m._m02 - Min03 * m._m03;
	}

	#pragma endregion

	#pragma region faceNormal
	static float3 faceNormal(float3 normal, float3 direction)
	{
		return dot(normal, direction) > 0 ? normal : -normal;
	}
	#pragma endregion

	#pragma region lit
	static float4 lit(float NdotL, float NdotH, float power)
	{
		return float4(1, NdotL < 0 ? 0 : NdotL, NdotL < 0 || NdotH < 0 ? 0 : pow(NdotH, power), 1);
	}
	#pragma endregion

	#pragma region reflect

	/// <summary>
	/// Performs the reflect function to the specified float3 objects.
	/// Gets the reflection vector. L is direction to Light, N is the surface normal
	/// </summary>
	static float3 reflect(float3 L, float3 N)
	{
		return 2 * dot(L, N) * N - L;
	}

	#pragma endregion

	#pragma region refract

	/// <summary>
	/// Performs the refract function to the specified float3 objects.
	/// Gets the refraction vector.
	/// L is direction to Light, N is the normal, eta is the refraction index factor.
	/// </summary>
	static float3 refract(float3 L, float3 N, float eta)
	{
		float3 I = -1 * L;

		float cosAngle = dot(I, N);
		float delta = 1.0f - eta * eta * (1.0f - cosAngle * cosAngle);

		if (delta < 0)
			return float3(0, 0, 0);

		return normalize(eta * I - N * (eta * cosAngle + sqrt(delta)));
	}

	#pragma endregion

	#pragma region ortho basis

	static float copysign(float f, float t)
	{
		return copysignf(f, t);
	}

	/// <summary>
	/// Given a direction, creates two othonormal vectors to it.
	/// From the paper: Building an Orthonormal Basis, Revisited, Tom Duff, et.al.
	/// </summary>
	static void createOrthoBasis(const float3 &N, float3 &T, float3 &B)
	{
		float sign = copysign(1.0f, N.z);
		float a = -1.0f / (sign + N.z);
		float b = N.x * N.y * a;
		T = float3(1.0f + sign * N.x * N.x * a, sign * b, -sign * N.x);
		B = float3(b, sign + N.y * N.y * a, -N.y);
	}

	#pragma endregion
	
	#pragma region inverse

	static float1x1 inverse(const float1x1& m) {
		if (m._m00 == 0.0f)
			return 0.0f;

		return float1x1(1.0 / m._m00);
	}

	static float2x2 inverse(const float2x2& m) {
		float det = m._m00 * m._m11 - m._m10 * m._m01;
		if (det == 0)
			return 0.0f;

		return float2x2(m._m11, m._m01, m._m10, m._m00) / det;
	}

	static float3x3 inverse(const float3x3& m)
	{
		/// 00 01 02
		/// 10 11 12
		/// 20 21 22
		float Min00 = m._m11 * m._m22 - m._m12 * m._m21;
		float Min01 = m._m10 * m._m22 - m._m12 * m._m20;
		float Min02 = m._m10 * m._m21 - m._m11 * m._m20;

		float det = Min00 * m._m00 - Min01 * m._m01 + Min02 * m._m02;

		if (det == 0)
			return float3x3(0);

		float Min10 = m._m01 * m._m22 - m._m02 * m._m21;
		float Min11 = m._m00 * m._m22 - m._m02 * m._m20;
		float Min12 = m._m00 * m._m21 - m._m01 * m._m20;

		float Min20 = m._m01 * m._m12 - m._m02 * m._m11;
		float Min21 = m._m00 * m._m12 - m._m02 * m._m10;
		float Min22 = m._m00 * m._m11 - m._m01 * m._m10;

		return float3x3(
			(+Min00 / det), (-Min10 / det), (+Min20 / det),
			(-Min01 / det), (+Min11 / det), (-Min21 / det),
			(+Min02 / det), (-Min12 / det), (+Min22 / det));
	}

	static float4x4 inverse(const float4x4& m) {
		float Min00 = m._m11 * m._m22 * m._m33 + m._m12 * m._m23 * m._m31 + m._m13 * m._m21 * m._m32 - m._m11 * m._m23 * m._m32 - m._m12 * m._m21 * m._m33 - m._m13 * m._m22 * m._m31;
		float Min01 = m._m10 * m._m22 * m._m33 + m._m12 * m._m23 * m._m30 + m._m13 * m._m20 * m._m32 - m._m10 * m._m23 * m._m32 - m._m12 * m._m20 * m._m33 - m._m13 * m._m22 * m._m30;
		float Min02 = m._m10 * m._m21 * m._m33 + m._m11 * m._m23 * m._m30 + m._m13 * m._m20 * m._m31 - m._m10 * m._m23 * m._m31 - m._m11 * m._m20 * m._m33 - m._m13 * m._m21 * m._m30;
		float Min03 = m._m10 * m._m21 * m._m32 + m._m11 * m._m22 * m._m30 + m._m12 * m._m20 * m._m31 - m._m10 * m._m22 * m._m31 - m._m11 * m._m20 * m._m32 - m._m12 * m._m21 * m._m30;

		float det = Min00 * m._m00 - Min01 * m._m01 + Min02 * m._m02 - Min03 * m._m03;

		if (det == 0)
			return float4x4(0);

		float Min10 = m._m01 * m._m22 * m._m33 + m._m02 * m._m23 * m._m31 + m._m03 * m._m21 * m._m32 - m._m01 * m._m23 * m._m32 - m._m02 * m._m21 * m._m33 - m._m03 * m._m22 * m._m31;
		float Min11 = m._m00 * m._m22 * m._m33 + m._m02 * m._m23 * m._m30 + m._m03 * m._m20 * m._m32 - m._m00 * m._m23 * m._m32 - m._m02 * m._m20 * m._m33 - m._m03 * m._m22 * m._m30;
		float Min12 = m._m00 * m._m21 * m._m33 + m._m01 * m._m23 * m._m30 + m._m03 * m._m20 * m._m31 - m._m00 * m._m23 * m._m31 - m._m01 * m._m20 * m._m33 - m._m03 * m._m21 * m._m30;
		float Min13 = m._m00 * m._m21 * m._m32 + m._m01 * m._m22 * m._m30 + m._m02 * m._m20 * m._m31 - m._m00 * m._m22 * m._m31 - m._m01 * m._m20 * m._m32 - m._m02 * m._m21 * m._m30;

		float Min20 = m._m01 * m._m12 * m._m33 + m._m02 * m._m13 * m._m31 + m._m03 * m._m11 * m._m32 - m._m01 * m._m13 * m._m32 - m._m02 * m._m11 * m._m33 - m._m03 * m._m12 * m._m31;
		float Min21 = m._m00 * m._m12 * m._m33 + m._m02 * m._m13 * m._m30 + m._m03 * m._m10 * m._m32 - m._m00 * m._m13 * m._m32 - m._m02 * m._m10 * m._m33 - m._m03 * m._m12 * m._m30;
		float Min22 = m._m00 * m._m11 * m._m33 + m._m01 * m._m13 * m._m30 + m._m03 * m._m10 * m._m31 - m._m00 * m._m13 * m._m31 - m._m01 * m._m10 * m._m33 - m._m03 * m._m11 * m._m30;
		float Min23 = m._m00 * m._m11 * m._m32 + m._m01 * m._m12 * m._m30 + m._m02 * m._m10 * m._m31 - m._m00 * m._m12 * m._m31 - m._m01 * m._m10 * m._m32 - m._m02 * m._m11 * m._m30;

		float Min30 = m._m01 * m._m12 * m._m23 + m._m02 * m._m13 * m._m21 + m._m03 * m._m11 * m._m22 - m._m01 * m._m13 * m._m22 - m._m02 * m._m11 * m._m23 - m._m03 * m._m12 * m._m21;
		float Min31 = m._m00 * m._m12 * m._m23 + m._m02 * m._m13 * m._m20 + m._m03 * m._m10 * m._m22 - m._m00 * m._m13 * m._m22 - m._m02 * m._m10 * m._m23 - m._m03 * m._m12 * m._m20;
		float Min32 = m._m00 * m._m11 * m._m23 + m._m01 * m._m13 * m._m20 + m._m03 * m._m10 * m._m21 - m._m00 * m._m13 * m._m21 - m._m01 * m._m10 * m._m23 - m._m03 * m._m11 * m._m20;
		float Min33 = m._m00 * m._m11 * m._m22 + m._m01 * m._m12 * m._m20 + m._m02 * m._m10 * m._m21 - m._m00 * m._m12 * m._m21 - m._m01 * m._m10 * m._m22 - m._m02 * m._m11 * m._m20;

		return float4x4(
			(+Min00 / det), (-Min10 / det), (+Min20 / det), (-Min30 / det),
			(-Min01 / det), (+Min11 / det), (-Min21 / det), (+Min31 / det),
			(+Min02 / det), (-Min12 / det), (+Min22 / det), (-Min32 / det),
			(-Min03 / det), (+Min13 / det), (-Min23 / det), (+Min33 / det));
	}

	#pragma endregion

	class Transforms {
	public:

		/// matrices
	/// <summary>
	/// Builds a mat using specified offsets.
	/// </summary>
	/// <param name="xslide">x offsets</param>
	/// <param name="yslide">y offsets</param>
	/// <param name="zslide">z offsets</param>
	/// <returns>A mat structure that contains a translated transformation </returns>
		static float4x4 Translate(float xoffset, float yoffset, float zoffset)
		{
			return float4x4(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				xoffset, yoffset, zoffset, 1
			);
		}
		/// <summary>
		/// Builds a mat using specified offsets.
		/// </summary>
		/// <param name="vec">A Vector structure that contains the x-coordinate, y-coordinate, and z-coordinate offsets.</param>
		/// <returns>A mat structure that contains a translated transformation </returns>
		static float4x4 Translate(float3 vec)
		{
			return Translate(vec.x, vec.y, vec.z);
		}
		//

		static float4x4 FromAffine(float4x3 affine) {
			return float4x4(
				affine._m00, affine._m01, affine._m02, 0,
				affine._m10, affine._m11, affine._m12, 0,
				affine._m20, affine._m21, affine._m22, 0,
				affine._m30, affine._m31, affine._m32, 1
			);
		}

		// Rotations
		/// <summary>
		/// Rotation mat around Z axis
		/// </summary>
		/// <param name="alpha">value in radian for rotation</param>
		static float4x4 RotateZ(float alpha)
		{
			float cos = cosf(alpha);
			float sin = sinf(alpha);
			return float4x4(
				cos, -sin, 0, 0,
				sin, cos, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			);
		}
		/// <summary>
		/// Rotation mat around Z axis
		/// </summary>
		/// <param name="alpha">value in grades for rotation</param>
		static float4x4 RotateZGrad(float alpha)
		{
			return RotateZ(alpha * PI / 180);
		}
		/// <summary>
		/// Rotation mat around Z axis
		/// </summary>
		/// <param name="alpha">value in radian for rotation</param>
		static float4x4 RotateY(float alpha)
		{
			float cos = cosf(alpha);
			float sin = sinf(alpha);
			return float4x4(
				cos, 0, -sin, 0,
				0, 1, 0, 0,
				sin, 0, cos, 0,
				0, 0, 0, 1
			);
		}
		/// <summary>
		/// Rotation mat around Z axis
		/// </summary>
		/// <param name="alpha">value in grades for rotation</param>
		static float4x4 RotateYGrad(float alpha)
		{
			return RotateY(alpha * PI / 180);
		}
		/// <summary>
		/// Rotation mat around Z axis
		/// </summary>
		/// <param name="alpha">value in radian for rotation</param>
		static float4x4 RotateX(float alpha)
		{
			float cos = cosf(alpha);
			float sin = sinf(alpha);
			return float4x4(
				1, 0, 0, 0,
				0, cos, -sin, 0,
				0, sin, cos, 0,
				0, 0, 0, 1
			);
		}
		/// <summary>
		/// Rotation mat around Z axis
		/// </summary>
		/// <param name="alpha">value in grades for rotation</param>
		static float4x4 RotateXGrad(float alpha)
		{
			return RotateX(alpha * PI / 180);
		}
		static float4x4 Rotate(float angle, const float3& dir)
		{
			float x = dir.x;
			float y = dir.y;
			float z = dir.z;
			float cos = cosf(angle);
			float sin = sinf(angle);

			return float4x4(
				x * x * (1 - cos) + cos, y * x * (1 - cos) + z * sin, z * x * (1 - cos) - y * sin, 0,
				x * y * (1 - cos) - z * sin, y * y * (1 - cos) + cos, z * y * (1 - cos) + x * sin, 0,
				x * z * (1 - cos) + y * sin, y * z * (1 - cos) - x * sin, z * z * (1 - cos) + cos, 0,
				0, 0, 0, 1
			);
		}
		static float4x4 RotateRespectTo(const float3& center, const float3& direction, float angle)
		{
			return mul(Translate(center), mul(Rotate(angle, direction), Translate(center * -1.0f)));
		}
		static float4x4 RotateGrad(float angle, const float3& dir)
		{
			return Rotate(PI * angle / 180, dir);
		}

		//

		// Scale

		static float4x4 Scale(float xscale, float yscale, float zscale)
		{
			return float4x4(
				xscale, 0, 0, 0,
				0, yscale, 0, 0,
				0, 0, zscale, 0,
				0, 0, 0, 1);
		}
		static float4x4 Scale(const float3& size)
		{
			return Scale(size.x, size.y, size.z);
		}

		static float4x4 ScaleRespectTo(const float3& center, const float3& size)
		{
			return mul(mul(Translate(center), Scale(size)), Translate(center * -1));
		}
		static float4x4 ScaleRespectTo(const float3& center, float sx, float sy, float sz)
		{
			return ScaleRespectTo(center, float3(sx, sy, sz));
		}

		//

		// Viewing

		static float4x4 LookAtLH(const float3& camera, const float3& target, const float3& upVector)
		{
			float3 zaxis = normalize(target - camera);
			float3 xaxis = normalize(cross(upVector, zaxis));
			float3 yaxis = cross(zaxis, xaxis);

			return float4x4(
				xaxis.x, yaxis.x, zaxis.x, 0,
				xaxis.y, yaxis.y, zaxis.y, 0,
				xaxis.z, yaxis.z, zaxis.z, 0,
				-dot(xaxis, camera), -dot(yaxis, camera), -dot(zaxis, camera), 1);
		}

		static float4x4 LookAtRH(const float3& camera, const float3& target, const float3& upVector)
		{
			float3 zaxis = normalize(camera - target);
			float3 xaxis = normalize(cross(upVector, zaxis));
			float3 yaxis = cross(zaxis, xaxis);

			return float4x4(
				xaxis.x, yaxis.x, zaxis.x, 0,
				xaxis.y, yaxis.y, zaxis.y, 0,
				xaxis.z, yaxis.z, zaxis.z, 0,
				-dot(xaxis, camera), -dot(yaxis, camera), -dot(zaxis, camera), 1);
		}

		//

		// Projection Methods

		/// <summary>
		/// Returns the near plane distance to a given projection
		/// </summary>
		/// <param name="proj">A mat structure containing the projection</param>
		/// <returns>A float value representing the distance.</returns>
		static float ZnearPlane(const float4x4& proj)
		{
			float4 pos = mul(float4(0, 0, 0, 1), inverse(proj));
			return pos.z / pos.w;
		}

		/// <summary>
		/// Returns the far plane distance to a given projection
		/// </summary>
		/// <param name="proj">A mat structure containing the projection</param>
		/// <returns>A float value representing the distance.</returns>
		static float ZfarPlane(const float4x4& proj)
		{
			float4 targetPos = mul(float4(0, 0, 1, 1), inverse(proj));
			return targetPos.z / targetPos.w;
		}

		static float4x4 PerspectiveFovLH(float fieldOfView, float aspectRatio, float znearPlane, float zfarPlane)
		{
			float h = 1.0f / tanf(fieldOfView / 2);
			float w = h * aspectRatio;

			return float4x4(
				w, 0, 0, 0,
				0, h, 0, 0,
				0, 0, zfarPlane / (zfarPlane - znearPlane), 1,
				0, 0, -znearPlane * zfarPlane / (zfarPlane - znearPlane), 0);
		}

		static float4x4 PerspectiveFovRH(float fieldOfView, float aspectRatio, float znearPlane, float zfarPlane)
		{
			float h = 1.0f / tanf(fieldOfView / 2);
			float w = h * aspectRatio;

			return float4x4(
				w, 0, 0, 0,
				0, h, 0, 0,
				0, 0, zfarPlane / (znearPlane - zfarPlane), -1,
				0, 0, znearPlane * zfarPlane / (znearPlane - zfarPlane), 0);
		}

		static float4x4 PerspectiveLH(float width, float height, float znearPlane, float zfarPlane)
		{
			return float4x4(
				2 * znearPlane / width, 0, 0, 0,
				0, 2 * znearPlane / height, 0, 0,
				0, 0, zfarPlane / (zfarPlane - znearPlane), 1,
				0, 0, znearPlane * zfarPlane / (znearPlane - zfarPlane), 0);
		}

		static float4x4 PerspectiveRH(float width, float height, float znearPlane, float zfarPlane)
		{
			return float4x4(
				2 * znearPlane / width, 0, 0, 0,
				0, 2 * znearPlane / height, 0, 0,
				0, 0, zfarPlane / (znearPlane - zfarPlane), -1,
				0, 0, znearPlane * zfarPlane / (znearPlane - zfarPlane), 0);
		}

		static float4x4 OrthoLH(float width, float height, float znearPlane, float zfarPlane)
		{
			return float4x4(
				2 / width, 0, 0, 0,
				0, 2 / height, 0, 0,
				0, 0, 1 / (zfarPlane - znearPlane), 0,
				0, 0, znearPlane / (znearPlane - zfarPlane), 1);
		}

		static float4x4 OrthoRH(float width, float height, float znearPlane, float zfarPlane)
		{
			return float4x4(
				2 / width, 0, 0, 0,
				0, 2 / height, 0, 0,
				0, 0, 1 / (znearPlane - zfarPlane), 0,
				0, 0, znearPlane / (znearPlane - zfarPlane), 1);
		}
	};

}

#endif