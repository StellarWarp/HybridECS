#pragma once
#include "vaildref_map.h"

namespace hyecs
{
	namespace generic
	{
		using default_constructor_ptr = void* (*)(void*);
		using copy_constructor_ptr = void* (*)(void*, const void*);
		using move_constructor_ptr = void* (*)(void*, void*);
		using destructor_ptr = void(*)(void*);

		template<typename T>
		constexpr void* default_constructor(void* addr)
		{
			return (T*)new (addr) T();
		}

		template<typename T>
		constexpr void* move_constructor(void* dest, void* src)
		{
			return (T*)new (dest) T(std::move(*(T*)src));
		}

		template<typename T>
		constexpr void* copy_constructor(void* dest, const void* src)
		{
			return (T*)new (dest) T(*(T*)src);
		}

		template<typename T>
		constexpr void destructor(void* addr)
		{
			((T*)addr)->~T();
		}

		template<typename T>
		constexpr auto nullable_move_constructor()
		{
			if constexpr (std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>)
				return move_constructor<T>;
			else
				return nullptr;
		}

		template<typename T>
		constexpr auto nullable_copy_constructor()
		{
			if constexpr (std::is_copy_constructible_v<T>)
				return copy_constructor<T>;
			else
				return nullptr;
		}

		template<typename T>
		constexpr auto nullable_destructor()
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
				return destructor<T>;
			else
				return nullptr;
		}


		struct type_info
		{
			size_t size;
			copy_constructor_ptr copy_constructor;
			move_constructor_ptr move_constructor;
			destructor_ptr destructor;
			uint64_t hash;

		private:
			type_info() = default;
		public:

			template<typename T>
			static type_info from_template()
			{
				//wait for c++20
				type_info info;
				info.size = std::is_empty_v<T> ? 0 : sizeof(T);
				info.copy_constructor = nullable_copy_constructor<T>();
				info.move_constructor = nullable_move_constructor<T>();
				info.destructor = nullable_destructor<T>();
				info.hash = typeid(T).hash_code();
				return info;
			}
		};

		template<typename... Args>
		using constructor_pointer = void* (*)(void*, Args...);

		template<typename... Args>
		struct construct_param
		{
			constructor_pointer<Args...> constructor;
			std::tuple<Args...> args;

			void* operator()(void* ptr)
			{
				return constructor(ptr, std::get<Args>(args)...);
			}
		};


		class type_index
		{
			type_info* m_info;

		public:
			const type_info* info() const noexcept
			{
				return m_info;
			}

			type_index(type_info* info) : m_info(info) {}

			constexpr size_t size() const noexcept
			{
				return m_info->size;
			}

			constexpr uint64_t hash() const noexcept
			{
				return m_info->hash;
			}

			void* copy_constructor(void* dest, const void* src) const
			{
				return m_info->copy_constructor(dest, src);
			}

			void* move_constructor(void* dest, void* src) const
			{
				return m_info->move_constructor(dest, src);
			}

			void destructor(void* addr) const
			{
				m_info->destructor(addr);
			}

			void destructor(void* addr, size_t count) const
			{
				if (m_info->destructor == nullptr)
					return;
				for (size_t i = 0; i < count; i++)
				{
					m_info->destructor(addr);
					addr = (uint8_t*)addr + m_info->size;
				}
			}

			void destructor(void* begin, void* end) const
			{
				if (m_info->destructor == nullptr)
					return;
				for (; begin != end; begin = (uint8_t*)begin + m_info->size)
				{
					m_info->destructor(begin);
				}
			}




			[[nodiscard]] bool is_trivially_destructible() const noexcept
			{
				return m_info->destructor == nullptr;
			}

		};

		class type_index_container_cached
		{
			type_index m_index;
			//cached info
			size_t m_size;
			copy_constructor_ptr m_copy_constructor;
			move_constructor_ptr m_move_constructor;
			destructor_ptr m_destructor;
		public:
			type_index_container_cached(type_info* info)
				: m_index(info),
				m_size(info->size),
				m_copy_constructor(info->copy_constructor),
				m_move_constructor(info->move_constructor),
				m_destructor(info->destructor) {}

			type_index_container_cached(type_index index)
				: m_index(index),
				m_size(index.size()),
				m_copy_constructor(index.info()->copy_constructor),
				m_move_constructor(index.info()->move_constructor),
				m_destructor(index.info()->destructor) {}

			constexpr size_t size() const noexcept
			{
				return m_size;
			}

			constexpr uint64_t hash() const noexcept
			{
				return m_index.hash();
			}

			void* copy_constructor(void* dest, const void* src) const
			{
				assert(m_copy_constructor != nullptr);
				return m_copy_constructor(dest, src);
			}

			void* move_constructor(void* dest, void* src) const
			{
				assert(m_move_constructor != nullptr);
				return m_move_constructor(dest, src);
			}

			void destructor(void* addr) const
			{
				if (!m_destructor) return;
				m_destructor(addr);
			}

			void destructor(void* addr, size_t count) const
			{
				if (m_destructor == nullptr) return;
				for (size_t i = 0; i < count; i++)
				{
					m_destructor(addr);
					addr = (uint8_t*)addr + m_size;
				}
			}

			void destructor(void* begin, void* end) const
			{
				if (m_destructor == nullptr) return;
				for (; begin != end; begin = (uint8_t*)begin + m_size)
				{
					m_destructor(begin);
				}
			}


			[[nodiscard]] bool is_trivially_destructible() const noexcept
			{
				return m_destructor == nullptr;
			}
		};

		class type_registry
		{
			inline static vaildref_map<uint64_t, type_info> m_type_info;

		public:

			template<typename T>
			static type_index register_type()
			{
				type_info info = type_info::from_template<T>();
				if (m_type_info.contains(info.hash))
					return type_index(&m_type_info.at(info.hash));
				m_type_info.emplace( info.hash, info );
				return type_index(&m_type_info.at(info.hash));
			}

		};


	}
}

