#pragma once
//#include "type_hash.h"
//#include "../container/container.h"
//
//namespace hyecs
//{
//	struct func_id
//	{
//		uint64_t hash;
//		func_id() : hash(typeid(void).hash_code()) {}
//		func_id(uint64_t hash) : hash(hash) {}
//
//		template<typename... Args>
//		static func_id from_hash(uint64_t ret, Args... args) {
//			func_id id;
//			id.hash = ret;
//			((id.hash += args), ...);
//			return id;
//		}
//
//		template<typename Ret, typename... Args>
//		static func_id from_template() {
//			func_id id;
//			id.hash = typeid(Ret).hash_code();
//			((id.hash += typeid(Args).hash_code()), ...);
//			return id;
//		}
//
//		bool operator==(const func_id& other) const
//		{
//			return hash == other.hash;
//		}
//	};
//
//
//}
//
//template<>
//struct std::hash<hyecs::func_id>
//{
//	size_t operator()(const hyecs::func_id& id) const
//	{
//		return id.hash;
//	}
//};
//
//namespace hyecs
//{
//
//}

