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
//			func_id m_id;
//			m_id.hash = ret;
//			((m_id.hash += args), ...);
//			return m_id;
//		}
//
//		template<typename Ret, typename... Args>
//		static func_id from_template() {
//			func_id m_id;
//			m_id.hash = typeid(Ret).hash_code();
//			((m_id.hash += typeid(Args).hash_code()), ...);
//			return m_id;
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
//	size_t operator()(const hyecs::func_id& m_id) const
//	{
//		return m_id.hash;
//	}
//};
//
//namespace hyecs
//{
//
//}

