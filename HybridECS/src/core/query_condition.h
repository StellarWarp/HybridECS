#pragma once
#include "component.h"

namespace hyecs
{
	struct query_condition
	{
		vector<component_type_index> all;
		vector<component_type_index> any;
		vector<component_type_index> none;

		query_condition() = default;
		query_condition(
			sequence_ref<const component_type_index> all,
			sequence_ref<const component_type_index> any,
			sequence_ref<const component_type_index> none) :
			all(all.begin(), all.end()),
			any(any.begin(), any.end()),
			none(none.begin(), none.end()) {};
		query_condition(
			sequence_ref<const component_type_index> all,
			sequence_ref<const component_type_index> any) :
			all(all.begin(), all.end()),
			any(any.begin(), any.end()) {};
		query_condition(
			sequence_ref<const component_type_index> all) :
			all(all.begin(), all.end()) {};

		uint64_t hash() const
		{
			//temp
			uint64_t hash = 0;
			for (auto& c : all)
				hash += c.hash();
			for (auto& c : any)
				hash += c.hash() * 37;
			for (auto& c : none)
				hash += c.hash() * 73;
			return hash;
		}

		bool empty() const
		{
			return all.empty() && any.empty() && none.empty();
		}


		bool match(archetype_index arch)
		{
			//todo
		}

	};

	using query_index = uint64_t;


}