#pragma once
#include "component.h"
#include "archetype.h"

namespace hyecs
{

	class query_condition
	{
		vector<component_type_index> storage_;
		sequence_cref<component_type_index> all_;
		sequence_cref<component_type_index> none_;
		small_vector<sequence_cref<component_type_index>> anys_;

		component_bit_set all_bitset_;
		component_bit_set none_bitset_;

		uint64_t hash_;

		static constexpr uint64_t invalid_hash = std::numeric_limits<uint64_t>::max();


	public:
		query_condition():hash_(0) { assert(compute_hash() == 0); };

		query_condition(const query_condition& other) noexcept
			: all_bitset_(other.all_bitset_),
			none_bitset_(other.none_bitset_),
			hash_(other.hash_)
		{
			storage_.reserve(other.storage_.size());
			auto append_cond = [this](
				sequence_cref<component_type_index> cond,
				sequence_cref<component_type_index>& cond_ref
				)
			{
				auto begin = storage_.end();
					storage_.append_range(cond);
				auto end = storage_.end();
				cond_ref = { begin,end };
			};
			append_cond(other.all_, all_);
			append_cond(other.none_, none_);
			for (auto& any : other.anys_)
			{
				auto& any_ = anys_.emplace_back();
				append_cond(any, any_);
			}
		}
		query_condition& operator=(const query_condition&) = delete;
		query_condition(query_condition&&) = default;
		query_condition& operator=(query_condition&&) = default;

		template<typename Filter = nullptr_t, typename Complete = std::true_type>
		query_condition(
			sequence_cref<component_type_index> all,
			auto& anys,//sequence_cref<sequence_cref<component_type_index>>
			sequence_cref<component_type_index> none,
			Filter filter,
			Complete = {}
		)
			: all_bitset_(all), none_bitset_(none)
		{
			size_t total_size = all.size() + none.size();
			for (auto& any : anys) total_size += any.size();
			storage_.reserve(total_size);
			//should not be resize, ref safe

			auto append_cond = [this, &filter](
				sequence_cref<component_type_index> cond,
				sequence_cref<component_type_index>& cond_ref
				)
			{
				auto begin = storage_.end();
				if constexpr (std::is_same_v<Filter, nullptr_t>)
					storage_.append_range(cond);
				else
					for (auto comp : cond)
						if (filter(comp)) storage_.push_back(comp);
				auto end = storage_.end();
				cond_ref = { begin,end };
			};
			append_cond(all, all_);
			append_cond(none, none_);
			for (auto& any : anys)
			{
				if constexpr (Complete::value)
					if (any.empty()) continue;
				auto& any_ = anys_.emplace_back();
				append_cond(any, any_);
			}

			if (Complete::value)
				compute_hash();
			else
				hash_ = invalid_hash;
		}

		query_condition(
			sequence_cref<component_type_index> all,
			auto& anys,
			sequence_cref<component_type_index> none)
			: query_condition(all, anys, none, nullptr) {};

		query_condition(
			sequence_cref<component_type_index> all,
			sequence_cref<sequence_cref<component_type_index>> anys,
			sequence_cref<component_type_index> none)
			: query_condition(all, anys, none, nullptr) {};

		query_condition(
			sequence_cref<component_type_index> all,
			sequence_cref<sequence_cref<component_type_index>> anys)
			: query_condition(all, anys, {}) {}
		query_condition(initializer_list<component_type_index> all)
			: query_condition(all, {}, {}) {}

		const auto& all() const { return all_; }
		const auto& none() const { return none_; }
		const auto& anys() const { return anys_; };

		void filter_anys_by_index(sequence_ref<uint32_t> filter)
		{
			ASSERTION_CODE(hash_ = invalid_hash);
			for (uint32_t i = 0; i < filter.size(); i++)
			{
				anys_[i] = anys_[filter[i]];
			}
			anys_.resize(filter.size());
		}

		void completion()
		{
			compute_hash();
		}


		query_condition tag_condition() const
		{
			return query_condition(all_, anys_, none_, [](component_type_index comp) {
				return comp.is_tag();
			});
		}

		query_condition base_condition() const
		{
			return query_condition(all_, anys_, none_, [](component_type_index comp) {
				return !comp.is_tag();
			});
		}

		query_condition tag_incomplete_condition() const
		{
			return query_condition(all_, anys_, none_, [](component_type_index comp) {
				return comp.is_tag();
			}, std::false_type{});
		}

		query_condition base_incomplete_condition() const
		{
			return query_condition(all_, anys_, none_, [](component_type_index comp) {
				return !comp.is_tag();
			}, std::false_type{});
		}

	private:
		static constexpr uint64_t any_hash(uint64_t type_hash)
		{
			return string_hash(type_hash, "[any]");
		}
		static constexpr uint64_t none_hash(uint64_t type_hash)
		{
			return string_hash(type_hash, "[none]");
		}
		uint64_t compute_hash()
		{
			hash_ = 0;
            // c1,c2,c3...
			for (auto comp : all_)
				hash_ += comp.hash();
            // any<c1>,any<c2>,...
			for (auto any : anys_)
			{
				assert(!any.empty());
				uint64_t h = 0;
				for (auto comp : any) h += comp.hash();
				hash_ += any_hash(h);
			}
            // none<c1,c2,c3...>
			uint64_t n_h = 0;
			if (!none_.empty())
			{
				for (auto comp : none_) n_h += comp.hash();
				hash_ += none_hash(n_h);
			}
			return hash_;
		}

		bool match_any_(const component_bit_set& arch) const
		{
			for (auto& any : anys_)
			{
				bool has_any = false;
				for (auto comp : any)
				{
					if (arch.contains(comp))
					{
						has_any = true;
						break;
					}
				}
				if (!has_any) return false;
			}
			return true;
		}
	public:
		static bool match_any(const component_bit_set& arch, sequence_cref<component_type_index> any)
		{
			if (any.empty()) return true;
			for (auto comp : any)
				if (arch.contains(comp))
					return true;
			return false;
		}
	public:
		uint64_t hash() const
		{
			assert(hash_ != invalid_hash);
			return hash_;
		}

		bool match(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if (!arch_mask.contains(all_bitset_)) return false;
			if (!match_any_(arch_mask)) return false;
			if (arch_mask.contains_any(none_bitset_)) return false;
			return true;
		}

		bool match_all_none(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if (!arch_mask.contains(all_bitset_)) return false;
			if (arch_mask.contains_any(none_bitset_)) return false;
			return true;
		}



		bool match_any(const archetype_index& arch) const
		{
			if (anys_.size() == 0) return true;
			const auto& arch_mask = arch.component_mask();
			return match_any_(arch_mask);
		}

		bool match_none(const archetype_index& arch) const
		{
			const auto& arch_mask = arch.component_mask();
			if (arch_mask.contains_any(none_bitset_)) return false;
			return true;
		}


		bool empty() const
		{
			return all_.size() == 0 && anys_.size() == 0 && none_.size() == 0;
		}

		bool operator==(const hyecs::query_condition& query_condition) const
		{
			return query_condition.hash() == hash();
		}
	};

	using query_index = uint64_t;


}
