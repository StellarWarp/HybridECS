#pragma once
#include "../core/Reflection.h"
namespace hyecs
{
	using float3 = glm::vec3;
	extern __declspec(selectany) TypeRegister float3TypeRegister(&float3::x, &float3::y, &float3::z);

	using double3 = glm::dvec3;
	extern __declspec(selectany) TypeRegister double3TypeRegister(&double3::x, &double3::y, &double3::z);

	using int3 = glm::ivec3;
	extern __declspec(selectany) TypeRegister int3TypeRegister(&int3::x, &int3::y, &int3::z);

	using quaternion = glm::quat;
	extern __declspec(selectany) TypeRegister quaternionTypeRegister(&quaternion::x, &quaternion::y, &quaternion::z, &quaternion::w);
}
