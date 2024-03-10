#pragma once
#include "archetype_storage.h"

namespace hyecs
{
	class component_storage
	{
		component_type_index m_component_type_index;
		//reqire dynamic type densemap container
		generic_dense_map m_storage;
		
	};
}