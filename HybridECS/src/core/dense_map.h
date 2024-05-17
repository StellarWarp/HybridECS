#pragma once
#include "container/container.h"
#include "entity.h"
namespace hyecs
{


	template<typename T>
	class entity_sparse_table
	{
		//using T = void*;

		struct empty_t {};

		using value_type = std::conditional_t<std::is_void_v<T>, empty_t, T>;

		struct entity_value_pair
		{
			entity e;
			[[no_unique_address]] value_type value;
		};


		static constexpr size_t page_byte_size = 4096;



		static constexpr bool is_map = !std::is_empty_v<value_type>;
		static constexpr bool is_set = std::is_empty_v<value_type>;
		static constexpr size_t page_capacity = page_byte_size / sizeof(entity_value_pair);
		static constexpr size_t page_offset_bits = std::bit_width(page_capacity - 1);
		using table_offset_t = uint32_t;
		static constexpr table_offset_t page_offset_mask = (1 << page_offset_bits) - 1;
		static constexpr table_offset_t page_index_mask = ~page_offset_mask;

		struct table_location
		{
			uint32_t page_index;
			uint32_t page_offset;

			table_location(table_offset_t offset)
				: page_index(offset / page_capacity), page_offset(offset % page_capacity)
			{}
		};

		struct page
		{
		private:
			uint8_t data[page_byte_size];
		public:
			page()
			{
				for (size_t i = 0; i < page_capacity; i++)
				{
					auto& pair = at(i);
					pair.e = null_entity;
				}
			}

			entity_value_pair& at(uint32_t offset)
			{
				assert(offset < page_capacity);
				auto byte_offset = offset * sizeof(entity_value_pair);
				assert(byte_offset < page_byte_size);
				return *reinterpret_cast<entity_value_pair*>(&data[byte_offset]);
			}
			
			template <class... Args>
			entity_value_pair& emplace(uint32_t offset, Args&&... args)
			{
				auto& pair = at(offset);
				new(&pair) entity_value_pair{ std::forward<Args>(args)... };
				return pair;
			}

			void erase(uint32_t offset)
			{
				auto& pair = at(offset);
				pair.e = entity();
				pair.value.~value_type();
			}


		};

		vector<page*> pages;

	public:

		entity_sparse_table() {};
		entity_sparse_table(const entity_sparse_table& other)
		{
			pages.resize(other.pages.size());
			for (size_t i = 0; i < other.pages.size(); i++)
			{
				if (other.pages[i])
				{
					pages[i] = new page(*other.pages[i]);
				}
			}
		}
		entity_sparse_table(entity_sparse_table&& other) noexcept
		{
			pages = std::move(other.pages);
			other.pages.clear();
		}
		~entity_sparse_table()
		{
			for (auto page : pages)
				delete page;
		}

	private:

		auto page_for_index(uint32_t page_index)
		{
			if (pages.size() <= page_index)
				pages.resize(page_index + 1);
			if (!pages[page_index])
				pages[page_index] = new page();
			return pages[page_index];
		}


		auto pair_for_location(table_location loc)
		{
			return page_for_index(loc.page_index)->at(loc.page_offset);
		}

	public:


		template<typename... Args>
		void emplace(Args... args)
		{
			//e is first argument
			auto e = std::get<0>(std::forward_as_tuple(args...));
			auto [page_index, page_offset] = table_location(e.id());
			auto& pair = page_for_index(page_index)->emplace(page_offset, std::forward<Args>(args)...);
		}

		void insert(entity_value_pair&& pair)
		{
			auto [page_index, page_offset] = table_location(pair.e.id());
			auto& pair = page_for_index(page_index)->emplace(page_offset, std::move(pair));
		}

		void insert(entity e)
		{
			emplace(e);
		}

		void erase(entity e)
		{
			auto [page_index, page_offset] = table_location(e.id());
			assert(pages.size() > page_index && pages[page_index]);
			auto& pair = pages[page_index]->at(page_offset);
			pair.e = null_entity;
		}

		bool contains(entity e)
		{
			auto [page_index, page_offset] = table_location(e.id());
			if (pages.size() <= page_index || !pages[page_index])
				return false;
			auto& pair = pages[page_index]->at(page_offset);
			assert(pair.e == null_entity || pair.e.version() == e.version());
			return pair.e != null_entity;
		}

		value_type& at(entity e)
		{
			auto [page_index, page_offset] = table_location(e.id());
			auto& pair = pages[page_index]->at(page_offset);
			assert(pair.e != null_entity);
			assert(pair.e.version() == e.version());
			return pair.value;
		}

		value_type& operator[](entity e)
		{
			auto& pair = pair_for_location(table_location(e.id()));
			return pair.value;
		}


	};


	class entity_sparse_set : protected entity_sparse_table<void>
	{
		using super = entity_sparse_table<void>;
	public:
		using super::super;
		using super::contains;
		using super::insert;
		using super::erase;
	};

	template<typename T>
	using entity_sparse_map = entity_sparse_table<T>;


	class raw_entity_dense_map
	{
		entity_sparse_map<std::pair<void*, raw_segmented_vector::index_t>> m_sparse;
		raw_segmented_vector m_dense;// element type <entity, value>



	public:

		struct entity_value
		{
			entity& e;
			void* value;
			entity_value(void* ptr) :
				e(*(entity*)ptr),
				value((uint8_t*)ptr + sizeof(entity))
			{}
		};

		raw_entity_dense_map(size_t value_size) : m_sparse(), m_dense(sizeof(entity) + value_size) {}

		void* allocate_value(entity e)
		{
			auto info = m_dense.allocate_value();
			m_sparse.emplace(e, info);
			entity_value ref(info.first);
			ref.e = e;
			return ref.value;
		}

		void deallocate_value(entity e)
		{
			const auto& [ptr,index] = m_sparse.at(e);
			if (m_dense.deallocate_value(ptr, index))
			{
				entity& swap_remap = *(entity*)ptr;
				m_sparse.at(swap_remap) = { ptr,index };
			}
			m_sparse.erase(e);
		}

		bool contains(entity e)
		{
			return m_sparse.contains(e);
		}

		void* at(entity e)
		{
			return entity_value(m_sparse.at(e).first).value;
		}

		void erase(entity e)
		{
			deallocate_value(e);
		}

		class iterator : public raw_segmented_vector::iterator
		{
			using super = raw_segmented_vector::iterator;
		public:
			iterator(super&& it) : super(std::move(it)) {}
			entity_value operator*()
			{
				return entity_value(raw_segmented_vector::iterator::operator*());
			}
		};


		using end_iterator = raw_segmented_vector::end_iterator;

		iterator begin()
		{
			return iterator{m_dense.begin()};
		}

		end_iterator end()
		{
			return m_dense.end();
		}
	};


	class entity_dense_set : protected raw_entity_dense_map
	{
		using super = raw_entity_dense_map;
	public:
		entity_dense_set() : super(0) {}
		void insert(entity e)
		{
			allocate_value(e);
		}
		void erase(entity e)
		{
			deallocate_value(e);
		}

		class iterator : public raw_entity_dense_map::iterator
		{
			using super = raw_entity_dense_map::iterator;
		public:
			iterator(super&& it) : super(std::move(it)) {}
			entity operator*()
			{
				return raw_entity_dense_map::iterator::operator*().e;
			}
		};

		iterator begin()
		{
			return iterator{super::begin()};
		}

		end_iterator end()
		{
			return super::end();
		}
	};

}
