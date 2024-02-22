#pragma once

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
	}
}
