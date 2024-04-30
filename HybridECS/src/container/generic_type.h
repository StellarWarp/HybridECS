#pragma once
#include "vaildref_map.h"
#include "../meta/meta_utils.h"
#include "type_hash.h"

namespace hyecs
{
	namespace generic
	{
		using default_constructor_ptr = void* (*)(void*);
		using copy_constructor_ptr = void* (*)(void*, const void*);
		using move_constructor_ptr = void* (*)(void*, void*);
		using destructor_ptr = void(*)(void*);

		template <typename T>
		constexpr void* default_constructor(void* addr)
		{
			return (T*)new(addr) T();
		}

		template <typename T>
		constexpr void* move_constructor(void* dest, void* src)
		{
			return (T*)new(dest) T(std::move(*(T*)src));
		}

		template <typename T>
		constexpr void* copy_constructor(void* dest, const void* src)
		{
			return (T*)new(dest) T(*(T*)src);
		}

		template <typename T>
		constexpr void destructor(void* addr)
		{
			((T*)addr)->~T();
		}

		template <typename T>
		constexpr auto nullable_move_constructor()
		{
			if constexpr (std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>)
				return move_constructor<T>;
			else
				return nullptr;
		}

		template <typename T>
		constexpr auto nullable_copy_constructor()
		{
			if constexpr (std::is_copy_constructible_v<T>)
				return copy_constructor<T>;
			else
				return nullptr;
		}

		template <typename T>
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
			type_hash hash;
			const char* name;

		private:
			type_info() = delete;
			type_info(const type_info&) = delete;
			type_info(type_info&&) = delete;
			type_info& operator=(const type_info&) = delete;

		public:
			template <typename T>
			static const type_info& of()
			{
				static_assert(std::is_same_v<T, std::decay_t<T>>, "T must be a decayed type");
				//wait c++20
				const static type_info info{
					std::is_empty_v<T> ? 0 : sizeof(T),
					nullable_copy_constructor<T>(),
					nullable_move_constructor<T>(),
					nullable_destructor<T>(),
					type_hash::of<T>,
					type_name<T>
				};
				return info;
			}
		};


		class type_index
		{
			const type_info* m_info;

		public:
			//type_index() : m_info(nullptr) {}
			type_index(const type_info& info) : m_info(&info)
			{
			}

			const type_info* info() const noexcept { return m_info; }
			constexpr size_t size() const noexcept { return m_info->size; }
			constexpr bool is_empty() const noexcept { return m_info->size == 0; }
			constexpr type_hash hash() const noexcept { return m_info->hash; }

			bool operator==(const type_index& other) const noexcept
			{
				ASSERTION_CODE(
					if (m_info != nullptr && other.m_info != nullptr && m_info != other.m_info)
					assert(m_info->hash != other.m_info->hash); //multiple location of same type is not allowed
				)
				return m_info == other.m_info; //a type should only have one instance of type_info
			}

			bool operator!=(const type_index& other) const noexcept { return !operator==(other); }

			const char* name() const noexcept { return m_info->name; }

			void* copy_constructor(void* dest, const void* src) const { return m_info->copy_constructor(dest, src); }
			void* move_constructor(void* dest, void* src) const { return m_info->move_constructor(dest, src); }
			void destructor(void* addr) const { m_info->destructor(addr); }

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

			bool operator<(const type_index& other) const noexcept { return m_info->hash < other.m_info->hash; }
			bool operator>(const type_index& other) const noexcept { return m_info->hash > other.m_info->hash; }
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
			type_index_container_cached(const type_info& info)
				: m_index(info),
				  m_size(info.size),
				  m_copy_constructor(info.copy_constructor),
				  m_move_constructor(info.move_constructor),
				  m_destructor(info.destructor)
			{
			}

			type_index_container_cached(type_index index)
				: m_index(index),
				  m_size(index.size()),
				  m_copy_constructor(index.info()->copy_constructor),
				  m_move_constructor(index.info()->move_constructor),
				  m_destructor(index.info()->destructor)
			{
			}

			constexpr size_t size() const noexcept { return m_size; }
			constexpr type_hash hash() const noexcept { return m_index.hash(); }
			const char* name() const noexcept { return m_index.name(); }

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


			bool is_trivially_destructible() const noexcept
			{
				return m_destructor == nullptr;
			}

			operator generic::type_index() const { return m_index; }
			bool operator==(const type_index_container_cached& other) const noexcept { return m_index == other.m_index; }
			bool operator!=(const type_index_container_cached& other) const noexcept { return m_index != other.m_index; }
			bool operator<(const type_index_container_cached& other) const noexcept { return m_index < other.m_index; }
			bool operator>(const type_index_container_cached& other) const noexcept { return m_index > other.m_index; }
		};

		class type_registry
		{
			inline static vaildref_map<uint64_t, type_info> m_type_info;

		public:
			template <typename T>
			static type_index register_type()
			{
				type_info info = type_info::of<T>();
				if (m_type_info.contains(info.hash))
					return type_index(&m_type_info.at(info.hash));
				m_type_info.emplace(info.hash, info);
				return type_index(&m_type_info.at(info.hash));
			}
		};


		class constructor
		{
			type_index m_type;
			std::function<void*(void*)> m_constructor;

			// template <typename T>
			// struct lambda_forward_wrapper
			// {
			// 	using type = std::decay_t<T>;
			// 	uint8_t data[std::is_empty_v<type> ? 0 : sizeof(type)];
			// 	lambda_forward_wrapper(T&& val)
			// 	{
			// 		if constexpr (!std::is_empty_v<type>)
			// 			new(reinterpret_cast<std::decay_t<T>*>(data)) std::decay_t<T>(std::forward<T>(val));
			// 	}
			// 	T&& forward()
			// 	{
			// 		if constexpr (!std::is_empty_v<type>)
			// 			return std::forward<T>(*reinterpret_cast<std::decay_t<T>*>(data));
			// 		else return T{};
			// 	}
			// };

		public:
			constructor() : m_type(type_info::of<nullptr_t>())
			{
			}

			constructor(type_index type, std::function<void*(void*)> constructor)
				: m_type(type), m_constructor(constructor)
			{
			}

			template <typename T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, constructor>, int>  = 0>
			constructor(T&& src)
				:m_type(type_info::of<std::decay_t<T>>())
			{
				if constexpr (!std::is_empty_v<std::decay_t<T>>)
					m_constructor = [src = std::forward<T>(src)](void* ptr) mutable
						{
							return new(ptr) std::decay_t<T>(src);
						};
			}

			void* operator()(void* ptr) const
			{
				if(m_constructor) return m_constructor(ptr);
				return ptr;
			}
			type_index type() const { return m_type; }

			template <typename T, typename... Args>
			static constructor of(Args&&... args)
			{
				return constructor(type_info::of<T>(), [args...](void* ptr) { return new(ptr) T(args...); });
			}
		};
	}
}

template <>
struct std::hash<hyecs::generic::type_index>
{
	size_t operator()(const hyecs::generic::type_index& index) const noexcept
	{
		return index.hash().operator size_t();
	}
};
