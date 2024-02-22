#pragma once
#include "ISystem.h"

namespace hyecs
{
	class SystemManager
	{
		SystemGroup m_mainGroup;

		ISystem::Context m_context;



	public:


		void ExecuteAll()
		{
			m_mainGroup.Execute(m_context);
		}

		/// <summary>
		/// system register
		/// </summary>
	private:

	public:



	};
}



