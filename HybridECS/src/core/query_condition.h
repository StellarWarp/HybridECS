#pragma once
#include "component.h"
#include "archetype.h"
#include "archetype_registry_node.h"

namespace hyecs
{

	class query_condition
	{
		vector<component_type_index> all_;
		vector<component_type_index> any_;
		vector<component_type_index> none_;
		component_bit_set all_bitset_;
		component_bit_set any_bitset_;
		component_bit_set none_bitset_;
		uint64_t hash_;
	public:
		query_condition() = default;

		query_condition(
			sequence_ref<const component_type_index> all,
			sequence_ref<const component_type_index> any, 
			sequence_ref<const component_type_index> none)
			: all_(all), any_(any), none_(none), all_bitset_(all), any_bitset_(any), none_bitset_(none)
		{
			compute_hash();
		}
		query_condition(
			sequence_ref<const component_type_index> all,
			sequence_ref<const component_type_index> any)
			: all_(all), any_(any), all_bitset_(all), any_bitset_(any)
		{
			compute_hash();
		}
		query_condition(initializer_list<component_type_index> all)
			: all_(all), all_bitset_(all)
		{
			compute_hash();
		}

		const vector<component_type_index>& all() const{ return all_; }
		const vector<component_type_index>& any() const{ return any_; }
		const vector<component_type_index>& none() const{ return none_; }

		void add_all(component_type_index type)
		{
			all_.push_back(type);
			all_bitset_.insert(type);
			hash_ += type.hash();
		}

		void add_any(component_type_index type)
		{
			any_.push_back(type);
			any_bitset_.insert(type);
			hash_ += any_hash(type.hash());
		}

		void add_none(component_type_index type)
		{
			none_.push_back(type);
			none_bitset_.insert(type);
			hash_ += none_hash(type.hash());
		}

		query_condition tag_condition() const
		{
			query_condition tag_condition;
			tag_condition.all_.reserve(all_.size());
			tag_condition.any_.reserve(any_.size());
			tag_condition.none_.reserve(none_.size());
			for (auto& comp : all_)
				if (comp.is_tag()) tag_condition.all_.push_back(comp);
			for (auto& comp : any_)
				if (comp.is_tag()) tag_condition.any_.push_back(comp);
			for (auto& comp : none_)
				if (comp.is_tag()) tag_condition.none_.push_back(comp);
			tag_condition.all_bitset_ = {tag_condition.all_};
			tag_condition.any_bitset_ = {tag_condition.any_};
			tag_condition.none_bitset_ = {tag_condition.none_};
			tag_condition.compute_hash();
			return tag_condition;
		}

		query_condition base_condition() const
		{
			query_condition base_condition;
			base_condition.all_.reserve(all_.size());
			base_condition.any_.reserve(any_.size());
			base_condition.none_.reserve(none_.size());
			for (auto& comp : all_)
				if (!comp.is_tag()) base_condition.all_.push_back(comp);
			for (auto& comp : any_)
				if (!comp.is_tag()) base_condition.any_.push_back(comp);
			for (auto& comp : none_)
				if (!comp.is_tag()) base_condition.none_.push_back(comp);
			base_condition.all_bitset_ = {base_condition.all_};
			base_condition.any_bitset_ = {base_condition.any_};
			base_condition.none_bitset_ = {base_condition.none_};
			base_condition.compute_hash();
			return base_condition;
		}
			
	
	private:
		static constexpr uint64_t any_hash(uint64_t type_hash)
		{
			return string_hash(type_hash, "any");
		}
		static constexpr uint64_t none_hash(uint64_t type_hash)
		{
			return string_hash(type_hash, "none");
		}
		uint64_t compute_hash()
		{
			hash_ = 0;
			for (auto comp : all_)
				hash_ += comp.hash();
			for (auto comp : any_)
				hash_ += any_hash(comp.hash());
			for (auto comp : none_)
				hash_ += none_hash(comp.hash());
			return hash_;
		}
	public:
		uint64_t hash() const
		{
			return hash_;
		}

		bool match(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if(!arch_mask.contains(all_bitset_)) return false;
			if(!arch_mask.contains_any(any_bitset_)) return false;
			if(arch_mask.contains_any(none_bitset_)) return false;
			return true;
		}

		bool match_all_none(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if(!arch_mask.contains(all_bitset_)) return false;
			if(arch_mask.contains_any(none_bitset_)) return false;
			return true;
		}

		bool match_any(const archetype_index& arch) const
		{
			if(any_.empty()) return true;
			const auto& arch_mask = arch.component_mask();
			if(!arch_mask.contains_any(any_bitset_)) return false;
			return true;
		}

		bool match_none(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if(arch_mask.contains_any(none_bitset_)) return false;
			return true;
		}


		bool empty() const
		{
			return all_.empty() && any_.empty() && none_.empty();
		}

		//todo optimize
		bool operator==(const hyecs::query_condition& query_condition) const
		{
			return all_ == query_condition.all_ && any_ == query_condition.any_ && none_ == query_condition.none_;
		}
	};

	using query_index = uint64_t;


}
