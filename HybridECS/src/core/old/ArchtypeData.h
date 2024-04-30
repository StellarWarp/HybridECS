#pragma once

#include "Archtype.h"

namespace hyecs
{

	struct SortKey
	{
		uint32_t archetypeIndex;
		uint32_t arrayIndex;

		//SortKey(uint32_t archetypeIndex, uint32_t arrayIndex)
		//	: archetypeIndex(archetypeIndex), arrayIndex(arrayIndex)
		//{

		//}
	};

	template<typename T>
	struct is_sort_key : std::is_same<T, SortKey> {};




	class ArchetypeData
	{
	private:
		static struct component_table_chunk_traits
		{
			static const size_t size = 128 * 1024;//128kb
		};

		template<size_t SIZE = component_table_chunk_traits::size>
		struct Chunk
		{
			static const size_t CHUNK_SIZE = SIZE;
			std::array<uint8_t, SIZE> m_data;
			std::vector<Entity> m_entities;
			std::vector<bool> m_mask;
			uint32_t m_size = 0;

			uint32_t size() const
			{
				return m_size;
			}

			Chunk(uint32_t capacity)
				: m_mask(capacity, false)
			{
				m_entities.resize(capacity);
			}

			uint32_t AddEntity(Entity entity)
			{
				assert(size() < m_mask.size());
				m_mask[size()] = true;
				m_entities[size()] = entity;
				m_size++;
				return size() - 1;
			}

			void RemoveEntity(uint32_t index)
			{
				assert(index < size());
				m_mask[index] = false;
			}

			void InsertEntity(Entity entity, uint32_t index)//reuse
			{
				assert(index < size());
				assert(!m_mask[index]);
				m_mask[index] = true;
				m_entities[index] = entity;
			}
		};

		//template<size_t SIZE = component_table_chunk_traits::size>
		//struct DataChunk
		//{
		//	std::array<uint8_t, SIZE> m_data;
		//	const std::vector<bool> m_mask;
		//	uint32_t m_size = 0;

		//	DataChunk(uint32_t capacity)
		//		: m_mask(capacity, false)
		//	{

		//	}
		//};
		//template<size_t ChunkSize = component_table_chunk_traits::size>
		//struct ChunkArray
		//{

		//	std::vector<DataChunk<ChunkSize>*> data;


		//	ChunkArray(const type_hash type)
		//	{
		//		uint32_t capacity = ChunkSize / ReflectionManager::Sizeof(type);
		//	}
		//};

	private:
		Archetype m_archType;
		uint32_t m_archTypeIndex;

		std::unordered_map<type_hash, uint32_t> m_componentToIndex;

		std::vector<uint32_t> m_componentOffsets;

		std::vector<uint32_t> m_freeList;

		uint32_t m_chunkCapacity;
		std::vector<Chunk<>*> m_chunks;
		//const std::vector<ChunkArray<>> m_componentArrays;

		//raw_array<raw_array<Chunk<>>> m_chunkGrid;

	private:
		//chunk api
		void AddChunk()
		{
			m_chunks.push_back(new Chunk<>(m_chunkCapacity));
		}

		SortKey GetSortKey(uint32_t chunkIndex, uint32_t index)
		{
			return SortKey{ m_archTypeIndex, chunkIndex * m_chunkCapacity + index };
		}
	public:
		ArchetypeData(const Archetype& archType, uint32_t archTypeIndex)
			: m_archType(archType), m_archTypeIndex(archTypeIndex)
		{
			size_t columnSize = archType.ColumnSize();
			m_chunkCapacity = Chunk<>::CHUNK_SIZE / columnSize;
			m_componentOffsets.reserve(archType.ComponentCount());
			m_componentOffsets.push_back(0);

			auto& components = archType.Components();

			for (size_t i = 0; i < components.size() - 1; i++)
			{
				size_t componentSize = ReflectionManager::Sizeof(components[i]);
				m_componentOffsets.push_back(m_componentOffsets.back() + componentSize * m_chunkCapacity);
				m_componentToIndex[components[i]] = i;
			}
			m_componentToIndex[components.back()] = components.size() - 1;



		}
		~ArchetypeData()
		{
			for (auto chunk : m_chunks)
			{
				for (size_t i = 0; i < m_archType.ComponentCount(); i++)
				{
					auto& typeinfo = ReflectionManager::GetTypeInfo(m_archType.Components()[i]);
					size_t type_size = typeinfo.size;
					uint32_t offset = m_componentOffsets[i];

					for (uint32_t index = 0; index < chunk->size(); index++)
					{
						if (chunk->m_mask[index])
						{
							typeinfo.destructor(
								&chunk->m_data[offset + index * type_size]
							);
						}
					}
				}

				delete chunk;
			}
		}

		const Archetype& GetArchType() const
		{
			return m_archType;
		}

		//uint32_t GetComponentIndex(const type_hash& type)
		//{
		//	return m_componentToIndex[type];
		//}

		//template<typename T>
		//uint32_t GetComponentIndex()
		//{
		//	return m_componentToIndex[typeid(T).hash_code()];
		//}

		void ConstructComponents(Chunk<>* chunk, uint32_t index)
		{
			for (size_t i = 0; i < m_archType.ComponentCount(); i++)
			{
				auto& typeinfo = ReflectionManager::GetTypeInfo(m_archType.Components()[i]);
				size_t type_size = typeinfo.size;
				uint32_t offset = m_componentOffsets[i];

				typeinfo.constructor(
					&chunk->m_data[offset + index * type_size]
				);
			}
		}

		void DestructComponents(Chunk<>* chunk, uint32_t index)
		{
			for (size_t i = 0; i < m_archType.ComponentCount(); i++)
			{
				auto& typeinfo = ReflectionManager::GetTypeInfo(m_archType.Components()[i]);
				size_t type_size = typeinfo.size;
				uint32_t offset = m_componentOffsets[i];

				typeinfo.destructor(
					&chunk->m_data[offset + index * type_size]
				);
			}
		}


		///entity api
	public:

		std::tuple<uint32_t, uint32_t> GetEntityChunkIndex(uint32_t index)
		{
			uint32_t chunkIndex = index / m_chunkCapacity;
			uint32_t chunkOffset = index % m_chunkCapacity;
			return { chunkIndex,chunkOffset };
		}

		// todo not complete
		SortKey AddEntity(Entity entity)
		{
			if (!m_freeList.empty())
			{
				uint32_t index = m_freeList.back();
				m_freeList.pop_back();
				auto [chunkIndex, chunkOffset] = GetEntityChunkIndex(index);
				auto chunk = m_chunks[chunkIndex];
				chunk->InsertEntity(entity, chunkOffset);
				ConstructComponents(chunk, chunkOffset);
				return { m_archTypeIndex, index };
			}
			//add back
			if (m_chunks.empty() || m_chunks.back()->size() == m_chunkCapacity)
			{
				AddChunk();
			}
			uint32_t chunkIndex = m_chunks.size() - 1;
			auto chunk = m_chunks[chunkIndex];
			assert(chunk->size() < m_chunkCapacity);
			uint32_t chunkOffset = chunk->AddEntity(entity);
			ConstructComponents(chunk, chunkOffset);
			return { m_archTypeIndex, chunkIndex * m_chunkCapacity + chunk->size() - 1 };
		}

		void RemoveEntity(uint32_t index)
		{
			m_freeList.push_back(index);
			auto [chunkIndex, chunkOffset] = GetEntityChunkIndex(index);
			auto chunk = m_chunks[chunkIndex];
			DestructComponents(chunk, chunkOffset);
			chunk->RemoveEntity(chunkOffset);
		}


		///access components
	private:
		template<size_t COUNT, std::size_t... I, typename ...T>
		void AccessComponentsRW_impl(uint32_t index,
			std::index_sequence<I...>,
			std::array<uint32_t, COUNT> componentIndex,
			T& ...components)
		{
			uint32_t chunkIndex = index / m_chunkCapacity;
			uint32_t chunkOffset = index % m_chunkCapacity;

			Chunk<>* chunk = m_chunks[chunkIndex];

			((components = reinterpret_cast<T&>(
				*(chunk->m_data[m_componentOffsets[componentIndex[I]] + chunkOffset * sizeof(T)]))
				), ...);
		}
	private:
		//todo this can be remove
		template<typename ...T>
		struct component_offset_t_helper;
		template<typename ...T>
		struct component_offset_t_helper <type_list<T...>> {
			using type = type_indexed_array<uint32_t, T...>;
			//using type = std::tuple<std::remove_cvref_t<T>...>;
		};
		template<typename ...T>
		struct component_offset_t_helper {
			using type = type_indexed_array<uint32_t, T...>;
			//using type = std::tuple<std::remove_cvref_t<T>...>;
		};

	public:

		template<typename ...T>
		using component_offset_t = type_indexed_array<uint32_t, T...>;

		template<typename... T>
		std::array<uint32_t, sizeof...(T)> GetComponentIndex() const
		{
			return { m_componentToIndex.at(typeid(T))... };
		}

		template<typename... T>
		//requires (std::is_same_v<T, std::remove_cvref_t<T>> && ...)
		component_offset_t<T...> GetComponentOffset() const
		{
			return {
					m_componentOffsets[m_componentToIndex.at(typeid(T))]...
			};
		}

		template<typename... T>
		//requires (std::is_same_v<T, std::remove_cvref_t<T>> && ...)
		component_offset_t<T...> GetComponentOffset(type_list<T...>) const
		{
			return {
					m_componentOffsets[m_componentToIndex.at(typeid(T))]...
			};
		}

		template<typename ...T>
		void AccessComponentsRW(uint32_t index,
			std::array<uint32_t, sizeof...(T)> componentIndex,
			T& ...components)
		{
			AccessComponentsRW_impl(index, std::index_sequence_for<T>(), componentIndex, components...);
		}

		template<typename ...T>
		void AccessComponentsRW(uint32_t index, T& ...components)
		{
			AccessComponentsRW(index, GetComponentIndex<T...>(), components...);
		}



	private:

		template<typename U>
		using raw_component_to_const_ref_t = std::conditional_t<
			//is_component<U>::value,
			!std::is_same_v<std::decay_t<U>, Entity> && !std::is_same_v<std::decay_t<U>, SortKey>,
			std::add_lvalue_reference_t<std::add_const_t<U>>,
			U
		>;

		template<typename Arg, typename ...ComType>
		__forceinline constexpr auto AccessData(
			const component_offset_t<ComType...>& componentOffsets,
			Chunk<>* chunk,
			uint32_t chunkIndex,
			uint32_t i) const
		{
			using raw_type = std::remove_cvref_t<Arg>;
			if constexpr (is_entity<raw_type>{})
			{
				return chunk->m_entities[i];
			}
			else if constexpr (is_sort_key<raw_type>{})
			{
				return SortKey{ m_archTypeIndex, chunkIndex * m_chunkCapacity + i };
			}
			else
			{
				return std::ref(*reinterpret_cast<raw_type*>(
					&chunk->m_data[componentOffsets.get<raw_type>() + i * sizeof(raw_type)]
					));
			}
		}

		template <typename Callable, typename... ComType, typename ...T>
			requires std::is_invocable_v<Callable, std::tuple<T...>>
		__forceinline void ForEach_impl(
			type_list<T...>,
			Callable f,
			const component_offset_t<ComType...>& componentOffsets)
		{
			//print_type(componentOffsets);
			//print_type(type_list<T...>{});
			//((std::cout << typeid(decltype(AccessData<T, ComType...>(componentOffsets, nullptr, 0, 0))).name() << std::endl), ...);

			for (uint32_t chunkIndex = 0; chunkIndex < m_chunks.size(); chunkIndex++)
			{
				Chunk<>* chunk = m_chunks[chunkIndex];
				for (uint32_t i = 0; i < m_chunkCapacity; i++)
				{
					if (chunk->m_mask[i])
					{
						f(std::tuple<raw_component_to_const_ref_t<T>...>(
							AccessData<T>(componentOffsets, chunk, chunkIndex, i)...
						));
					}
				}
			}
		}

	public:

		template<typename Callable, typename... ComType>
		__forceinline void ForEach(Callable f, const component_offset_t<ComType...>& componentOffsets)
		{
			//f is of type void(const std::tuple<Args...>&)
			using Args = typename type_list<>::template from_tuple<typename function_traits<Callable>::args::template get<0>>;
			//print_type(Args);
			ForEach_impl<Callable, ComType...>(Args{}, f, componentOffsets);
		}

		template<typename AccessList, typename CompList>
		struct ForwardAccessor;


		template<typename ...CompType, typename ...AccessType>
		struct ForwardAccessor<type_list< AccessType...>, type_list<CompType...>>
		{
		private:
			ForwardAccessor() = default;
		public:
			static constexpr ForwardAccessor<type_list< AccessType...>, type_list<CompType...>> empty() { return {}; };
		public:
			const ArchetypeData* archType;
			component_offset_t<CompType...> m_componentOffsets;

			ForwardAccessor(const ArchetypeData& archtypeData)
				: archType(&archtypeData),
				m_componentOffsets(archtypeData.GetComponentOffset<AccessType...>())
			{

			}


			ForwardAccessor(const ArchetypeData& archtypeData, const component_offset_t<CompType...>& componentOffsets)
				: archType(&archtypeData),
				m_componentOffsets(componentOffsets)
			{

			}



			auto operator=(const ForwardAccessor& rhs)
			{
				archType = rhs.archType;
				m_componentOffsets = rhs.m_componentOffsets;
			}

			struct end_iterator {};

			struct iterator
			{
			private:
				iterator() = default;
			public:
				static constexpr iterator empty() { return {}; };
			private:
				ForwardAccessor const* accessor;
				friend struct ForwardAccessor;

				uint32_t m_chunkIndex;
				uint32_t m_index;

				constexpr const ArchetypeData* archtype_array() const
				{
					return accessor->archType;
				}

				constexpr auto offset() const
				{
					return accessor->m_componentOffsets;
				}

				void inc_index()
				{
					m_index++;
					if (m_index >= archtype_array()->m_chunkCapacity)
					{
						m_chunkIndex++;
						m_index = 0;
					}
				}

				bool is_end() const
				{
					if (m_chunkIndex >= archtype_array()->m_chunks.size())
					{
						return true;
					}
					return false;
				}

				iterator(const ForwardAccessor& forwardAccessor, uint32_t chunkIndex, uint32_t index)
					: accessor(&forwardAccessor), m_chunkIndex(chunkIndex), m_index(index)
				{

				}


			public:

				iterator(const ForwardAccessor& forwardAccessor)
					: accessor(&forwardAccessor), m_chunkIndex(0), m_index(0)
				{

				}

				iterator& operator++()
				{
					inc_index();
					auto chunk = archtype_array()->m_chunks[m_chunkIndex];
					while (!is_end() && chunk->m_mask[m_index] == 0)
					{
						inc_index();
					}
					return *this;
				}

				iterator operator++(int)
				{
					iterator tmp = *this;
					++(*this);
					return tmp;
				}

				bool operator==(const iterator& rhs) const
				{
					return m_chunkIndex == rhs.m_chunkIndex && m_index == rhs.m_index;
				}

				bool operator!=(const iterator& rhs) const
				{
					return m_chunkIndex != rhs.m_chunkIndex || m_index != rhs.m_index;
				}

				bool operator==(const end_iterator& rhs) const
				{
					return is_end();
				}

				bool operator!=(const end_iterator& rhs) const
				{
					return !is_end();
				}

				std::tuple<AccessType...> operator*()
				{
					auto chunk = archtype_array()->m_chunks[m_chunkIndex];

					std::tuple<AccessType...> tuple(
						archtype_array()->AccessData<AccessType>(offset(), chunk, m_chunkIndex, m_index)...
					);

					return tuple;
				}
			};


			iterator begin()
			{
				return iterator(*this);
			}

			end_iterator end()
			{
				return end_iterator{};
			}
		};





		template< typename AccessList>
		ForwardAccessor<AccessList, AccessList> GetAccessor()
		{
			return ForwardAccessor<AccessList, AccessList>(*this);
		}

		template<typename AccessList, typename CompList, typename ...T>
		ForwardAccessor<AccessList, CompList> GetAccessor(component_offset_t<T...> componentOffsets)
		{
			return ForwardAccessor<AccessList, CompList>(*this, componentOffsets);
		}




	};

}
