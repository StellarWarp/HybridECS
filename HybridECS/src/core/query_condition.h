#pragma once
#include "component.h"
#include "archetype.h"
#include "archetype_registry_node.h"

namespace hyecs
{

	struct query_condition
	{
		vector<component_type_index> all;
		vector<component_type_index> any;
		vector<component_type_index> none;
		component_bit_set all_bitset;
		component_bit_set any_bitset;
		component_bit_set none_bitset;

		query_condition() = default;

		query_condition(
			sequence_ref<const component_type_index> all,
			sequence_ref<const component_type_index> any, 
			sequence_ref<const component_type_index> none)
			: all(all), any(any), none(none), all_bitset(all), any_bitset(any), none_bitset(none) {}
		query_condition(
			sequence_ref<const component_type_index> all,
			sequence_ref<const component_type_index> any)
			: all(all), any(any), all_bitset(all), any_bitset(any) {}
		query_condition(initializer_list<component_type_index> all)
			: all(all), all_bitset(all) {}

	private:
		static constexpr uint64_t any_hash(uint64_t type_hash)
		{
			return string_hash(type_hash, "any");
		}
		static constexpr uint64_t none_hash(uint64_t type_hash)
		{
			return string_hash(type_hash, "none");
		}
	public:
		uint64_t hash() const
		{
			uint64_t hash = 0;
			for (auto comp : all)
				hash += comp.hash();
			for (auto comp : any)
				hash += any_hash(comp.hash());
			for (auto comp : none)
				hash += none_hash(comp.hash());

			return hash;
		}

		bool match(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if(!arch_mask.contains(all_bitset)) return false;
			if(!arch_mask.contains_any(any_bitset)) return false;
			if(arch_mask.contains_any(none_bitset)) return false;
			return true;
		}

		bool match_all_none(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if(!arch_mask.contains(all_bitset)) return false;
			if(arch_mask.contains_any(none_bitset)) return false;
			return true;
		}

		bool match_any(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if(!arch_mask.contains_any(any_bitset)) return false;
			return true;
		}

		bool match_none(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if(arch_mask.contains_any(none_bitset)) return false;
			return true;
		}


		bool empty() const
		{
			return all.empty() && any.empty() && none.empty();
		}

		//todo optimize
		bool operator==(const hyecs::query_condition& query_condition) const
		{
			return all == query_condition.all && any == query_condition.any && none == query_condition.none;
		}
	};

	using query_index = uint64_t;


}
