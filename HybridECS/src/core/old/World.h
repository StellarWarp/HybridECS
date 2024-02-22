#pragma once
#include "ISystem.h"
#include "EntityManager.h"
#include "SystemManager.h"
namespace hyecs
{
	struct IWorld
	{
		EntityManager entityManager;
		SystemManager systemManager;



	};
}


