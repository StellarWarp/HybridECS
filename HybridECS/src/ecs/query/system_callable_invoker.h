#pragma once
#include "ecs/type/entity.h"
#include "ecs/type/component.h"
#include "ecs/storage/storage_key_registry.h"

namespace hyecs
{
	namespace system_callable_invoker_params
	{
		template<typename GetEntity>
		struct entity_getter
		{
			GetEntity func;
			entity_getter(GetEntity&& func) : func(func)
			{
				static_assert(std::is_invocable_v<GetEntity>, "entity_getter must be invocable");
			}
			entity operator()(){return func();}
		};

		template<typename GetStorageKey>
		struct storage_key_getter
		{
			GetStorageKey func;
			storage_key_getter(GetStorageKey&& func) : func(func)
			{
				static_assert(std::is_invocable_v<GetStorageKey>, "storage_key_getter must be invocable");
			}
			storage_key operator()(){return func();}
		};

		struct typed_index
		{
			size_t index;
			operator size_t() const { return index; }
		};

		struct component_param_index : typed_index{};
		struct tag_param_index : typed_index{};
		struct base_component_param_index : typed_index{};

		// template<typename GetAddress>
		// struct address_getter
		// {
		// 	GetAddress func;
		// 	address_getter(GetAddress&& func) : func(func){}
		// 	void* operator()(type_wrapper<component> type, size_t index)
		// 	{
		// 		return func(type, index);
		// 	}
		// };
	}
	
	template <typename Callable>
	class system_callable_invoker
	{
		Callable m_callable;

	public:
		system_callable_invoker(Callable&& callable)
			: m_callable(std::forward<Callable>(callable))
		{
		}


	private:


		template <typename GetEntity, typename GetStorageKey, typename GetAddress, typename... Args>
		auto invoke_impl(GetEntity get_entity,
			GetStorageKey get_storage_key,
			GetAddress get_address,
			type_list<Args...>)
		{
			using params = type_list<Args...>;
			using component_param = typename params::template filter_with<is_static_component>;
			using non_component_param = typename params::template filter_without<is_static_component>;
			auto get_param = [&](auto type) -> decltype(auto)
			{
				using param_type = typename decltype(type)::type;
				using base_type = std::decay_t<param_type>;

				if constexpr (is_static_component<param_type>::value)
				{
					void* data = get_address(type, component_param::template index_of<param_type>);
					return static_cast<param_type>(*static_cast<base_type*>(data));
				}
				else
				{
					if constexpr (std::is_same_v<param_type, entity>)
						return get_entity();
					else if constexpr (std::is_same_v<param_type, storage_key>)
						return get_storage_key();
					else
						static_assert(!std::is_same_v<param_type, param_type>, "invalid type");
				}
			};
			m_callable(get_param(type_wrapper<Args>{})...);
		}

	public:
		template <typename GetEntity, typename GetStorageKey, typename GetAddress>
		auto invoke(GetEntity&& get_entity, GetStorageKey&& get_storage_key, GetAddress&& get_address)
		{
			using args = typename function_traits<Callable>::args;
			invoke_impl(std::forward<GetEntity>(get_entity),
                        std::forward<GetStorageKey>(get_storage_key),
                        std::forward<GetAddress>(get_address),
                        args{});
		}


	};
}
