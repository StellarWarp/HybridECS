#pragma once
#include "ICompomemt.h"
#include "../../math/Math.h"

namespace hyecs
{

	struct Transform
	{
		float3 position;
		quaternion rotation;
		float3 scale;
	};

	struct PhysicsMass
	{
		float mass;

	};

	namespace
	{
		extern __declspec(selectany) TypeRegister reg_Transform(
			&Transform::position,
			&Transform::rotation,
			&Transform::scale);

		extern __declspec(selectany) TypeRegister reg_PhysicsMass(
			&PhysicsMass::mass);
	}


}
