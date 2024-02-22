#pragma once
#include "World.h"

namespace hyecs
{

	class Application
	{
		inline static std::vector<IWorld*> m_Worlds;
	public:
		Application() {}
		~Application()
		{
			for (auto& world : m_Worlds)
				delete world;
		}

		void AddWorld(IWorld* world)
		{
			m_Worlds.push_back(world);
		}

		void Run()
		{

		}
	};



	template<typename T>
		requires std::is_base_of_v<IWorld, T>
	struct WorldStaticRegister
	{
		inline static IWorld* world;
	};

	template<typename T>
		requires std::is_base_of_v<IWorld, T>
	inline T& WorldOf()
	{
		return *(T*)WorldStaticRegister<T>::world;
	}


	template<typename T>
		requires std::is_base_of_v<SystemGroup, T>
	struct SystemGroupStaticInfo
	{
		IWorld* world;
	};


	template<typename...T>
	struct UpdateOrder
	{
		using type = type_list<T...>;
	};


	template<
		typename TSystem,
		typename TSystemGroup,
		typename TUpdateAfter,
		typename TUpdateBefore>
	struct SystemGroupRegister;


	template<
		typename TSystemGroup,
		typename TSuperHierarchy,
		typename... TUpdateAfter,
		typename... TUpdateBefore>
	struct SystemGroupRegister<
		TSystemGroup,
		TSuperHierarchy,
		type_list<TUpdateAfter...>,
		type_list<TUpdateBefore...>>
	{
		//inline static SystemGroupStaticInfo<T> info;

		//SystemGroupRegister();
		//{

		//}

	};






	template<
		typename TSystem,
		//typename TSystemGroup,
		typename TUpdateAfter = type_list<>,
		typename TUpdateBefore = type_list<>>
		struct SystemTemplateInfo;

	template<
		typename TSystem,
		//typename TSystemGroup,
		typename... TUpdateAfter,
		typename... TUpdateBefore>
	struct SystemTemplateInfo<
		TSystem,
		//TSystemGroup,
		type_list<TUpdateAfter...>,
		type_list<TUpdateBefore...>>
	{
		//using SuperHierarchySet = TSystemGroup;
		using SystemType = TSystem;
		using UpdateAfterSet = type_list<TUpdateAfter...>;
		using UpdateBeforeSet = type_list<TUpdateBefore...>;

		using UpdateOrderPair = type_list<
			type_list<TUpdateAfter, SystemType>...,
			type_list<SystemType, TUpdateBefore>...>;

		template<typename... T>
		using UpdateAfter = SystemTemplateInfo<
			TSystem,
			//TSystemGroup,
			type_list<TUpdateAfter..., T...>,
			UpdateBeforeSet>;

		template<typename... T>
		using UpdateBefore = SystemTemplateInfo<
			TSystem,
			//TSystemGroup,
			UpdateAfterSet,
			type_list<TUpdateBefore..., T...>>;

	};
	template<typename T>
	struct TemplateInfoAccessor;
	//template<>
	//struct TemplateInfoAccessor<> {
	//	using info = SystemTemplateInfo<void,type_list<>,type_list<>>;
	//};

	template<typename T>
	using system_info_of = typename TemplateInfoAccessor<T>::info;



	template<
		//typename TSuperHierarchy,
		typename TUpdateAfter,
		typename TUpdateBefore,
		typename Children>
	struct SystemGroupTemplateInfo;

	template<
		//typename... TSuperHierarchy,
		typename... TUpdateAfter,
		typename... TUpdateBefore,
		typename... TChild>
	struct SystemGroupTemplateInfo<
		//type_list<TSuperHierarchy...>,
		type_list<TUpdateAfter...>,
		type_list<TUpdateBefore...>,
		type_list<TChild...>>
	{
		//using SuperHierarchySet = type_list<TSuperHierarchy>;
		using UpdateAfterSet = type_list<TUpdateAfter...>;
		using UpdateBeforeSet = type_list<TUpdateBefore...>;
		using ChildrenSet = type_list<TChild...>;

		//template<typename... T>
		//using SuperHierarchy = SystemGroupTemplateInfo<
		//	type_list<T...>,
		//	UpdateAfterSet,
		//	UpdateBeforeSet,
		//	ChildrenSet>;

		template<typename... T>
		using UpdateAfter = SystemGroupTemplateInfo<
			//SuperHierarchySet,
			type_list<TUpdateAfter..., T...>,
			UpdateBeforeSet,
			ChildrenSet>;

		template<typename... T>
		using UpdateBefore = SystemGroupTemplateInfo<
			//SuperHierarchySet,
			UpdateAfterSet,
			type_list<TUpdateBefore..., T...>,
			ChildrenSet>;

		template<typename... T>
		using Systems = SystemGroupTemplateInfo<
			//SuperHierarchySet,
			UpdateAfterSet,
			UpdateBeforeSet,
			type_list<TChild..., T...>>;

		class SystemGroupType : public SystemGroup {};

	private:

		template<typename T>
		using sys_order_pair = typename system_info_of<T>::UpdateOrderPair;

		using OrderRestrictions = type_list<>::cat<sys_order_pair<TChild>...>;

		template<typename TOrderPair>
		using dep_left = typename TOrderPair::template get<0>;
		template<typename TOrderPair>
		using dep_right = typename TOrderPair::template get<1>;

		template<typename TOrderPair, typename T>
		using left_is = std::is_same<dep_left<TOrderPair>, T>;

		template<typename TOrderPair, typename T>
		using right_is = std::is_same<dep_right<TOrderPair>, T>;

		template<int... I>
		struct system_dep_level {};

		template<typename T>
		struct dep_of {
			template<typename TOrderPair>
			using cond = right_is<TOrderPair, T>;
			using type = OrderRestrictions::template filter_with<cond>::template modify_with<dep_left>;
		};

		template<typename T>
		struct sys_dl;

		template<>
		struct sys_dl<type_list<>> {
			static constexpr int value = 0;
		};

		template<typename... T>
		struct sys_dl<type_list<T...>> {
			static constexpr int value = 1 + std::max({ sys_dl<typename dep_of<T>::type>::value... });
		};

		static constexpr type_indexed_array<int, TChild...> sys_dl_list{ sys_dl<typename dep_of<TChild>::type>::value... };

		template<int I>
		struct sys_of_dl {
			template<typename T> struct is_void : std::is_same<T, void> {};
			using type = type_list<
				std::conditional_t<sys_dl_list.template get<TChild>() == I, TChild, void>...
			>::template filter_without<is_void>;
		};


		template<typename SetI, int I>
		struct build_update_order_set;
		template<int I>
		struct build_update_order_set<type_list<>, I> {
			using type = type_list<>;
		};


		template<typename... T, int I>
		struct build_update_order_set<type_list<T...>, I> {
			using next = sys_of_dl<I + 1>::type;

			using type = type_list<T...>::template cat<typename build_update_order_set<next, I + 1>::type>;
		};

	public:

		using UpdateOrder = build_update_order_set<typename sys_of_dl<0>::type, 0>::type;

	};

	using SystemGroupBuilder = SystemGroupTemplateInfo<type_list<>, type_list<>, type_list<>>;

#define REG_SYS(sys,...)\
template<> struct TemplateInfoAccessor<sys> {\
	using info = SystemTemplateInfo<sys>##__VA_ARGS__;\
}

	//sample


	struct System10 {};
	struct System11 {};
	struct System12 {};
	struct System20 {};
	struct System21 {};
	struct System30 {};

	REG_SYS(System10, ::UpdateBefore<System20>);
	REG_SYS(System11, ::UpdateBefore<System21, System20>);
	REG_SYS(System12, ::UpdateBefore<System21>);
	REG_SYS(System20, ::UpdateAfter<System10>);
	REG_SYS(System21, ::UpdateAfter<System11, System12>::UpdateBefore<System30>);
	REG_SYS(System30, ::UpdateAfter<System12, System21>);

	using sysg = SystemGroupBuilder::Systems<System20, System21, System10, System30, System11, System12>;

	using order = sysg::UpdateOrder;





	template<
		typename TSystem,
		typename TSystemGroup,
		typename TUpdateAfter,
		typename TUpdateBefore>
	struct SystemStaticRegister;


	template<
		typename TSystem,
		typename TSystemGroup,
		typename... TUpdateAfter,
		typename... TUpdateBefore>
		requires
	std::is_base_of_v<ISystem, TSystem> ||
		std::is_base_of_v<SystemGroup, TSystemGroup> ||
		(std::is_base_of_v<ISystem, TUpdateAfter> || ...) ||
		(std::is_base_of_v<ISystem, TUpdateBefore> || ...)
		struct SystemStaticRegister<TSystem, TSystemGroup, type_list<TUpdateAfter...>, type_list<TUpdateBefore...>>
	{

	};
}
