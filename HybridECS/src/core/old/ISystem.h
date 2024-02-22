#pragma once

#include "EntityManager.h"

namespace hyecs
{

	class ISystem
	{

	public:

		struct Context
		{
		};

		virtual void Execute(Context& context) = 0;


	};

	class SystemGroup : ISystem
	{
		std::string name;
		std::vector<ISystem*> m_systems;
	public:

		virtual void Execute(Context& context) override
		{
			for (auto system : m_systems)
			{
				system->Execute(context);
			}
		}
	};
}





