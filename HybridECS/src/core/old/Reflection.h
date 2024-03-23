#pragma once
#include "../meta/meta_utils.h"



namespace hyecs
{
	struct TypeInfo
	{
		size_t size;
		function_traits<void(void*)>::pointer constructor;
		function_traits<void(void*)>::pointer destructor;
		size_t alignment;
		const char* type_name;
		size_t fields_count;
		struct FieldInfo
		{
			const char* name;
			const char* type_name;
			size_t offset;
			size_t size;
			const TypeInfo* type;


		};
		FieldInfo* fields;

		TypeInfo()
			: type_name(nullptr), size(0), alignment(0), fields_count(0), fields(nullptr)
		{}

		TypeInfo(
			const char* type_name,
			size_t size,
			size_t alignment,
			size_t fields_count,
			FieldInfo* fields,
			void(*constructor)(void*) = nullptr,
			void(*destructor)(void*) = nullptr
		)
			: type_name(type_name),
			size(size),
			alignment(alignment),
			fields_count(fields_count),
			fields(fields),
			constructor(constructor),
			destructor(destructor)
		{}

		TypeInfo& operator=(const TypeInfo& other)
		{
			type_name = other.type_name;
			size = other.size;
			alignment = other.alignment;
			fields_count = other.fields_count;
			fields = other.fields;
			constructor = other.constructor;
			destructor = other.destructor;
			return *this;
		}

	};



	template<typename T>
	struct TypeInfoStaticAccessor
	{
		inline static const TypeInfo* info = nullptr;
	};

	template<typename T>
	inline const TypeInfo& TypeInfoOf()
	{
		tag_assert(TypeInfoStaticAccessor<T>::info != nullptr, "Type is not registered");
		return *TypeInfoStaticAccessor<T>::info;
	}

	class ReflectionManager
	{
		static inline unordered_map<type_hash, TypeInfo> m_types;

	public:

		template<typename T>
		static void RegisterType(const TypeInfo& info)
		{
			type_hash hash = typeid(T);
			m_types[hash] = info;
		}

		template<typename T>
		static const TypeInfo& GetTypeInfo()
		{
			return m_types.at(typeid(T));
		}

		static const TypeInfo& GetTypeInfo(const type_info& id)
		{
			return m_types.at(id);
		}

		static const TypeInfo& GetTypeInfo(type_hash hash)
		{
			return m_types.at(hash);
		}

		static size_t Sizeof(type_hash hash)
		{
			return m_types.at(hash).size;
		}

		template<typename T>
		static bool IsRegistered()
		{
			type_hash info = typeid(T);
			return m_types.find(info) != m_types.end();
		}

		static void Constructor(type_hash hash, void* ptr)
		{
			m_types.at(hash).constructor(ptr);
		}

		static void Destructor(type_hash hash, void* ptr)
		{
			m_types.at(hash).destructor(ptr);
		}

		static void* Create(type_hash hash)
		{
			void* ptr = std::malloc(m_types.at(hash).size);
			m_types.at(hash).constructor(ptr);
			return ptr;
		}

		static void Destroy(type_hash hash, void* ptr)
		{
			m_types.at(hash).destructor(ptr);
			std::free(ptr);
		}

	};





	template<typename T, typename... Field>
	struct TypeRegister
	{
		const size_t fields_count = sizeof...(Field);

		inline static type_indexed_array<TypeInfo::FieldInfo, Field...> fields;

		inline static TypeInfo info;


		TypeRegister(Field T::*... field)
		{
			fields =
			{
				TypeInfo::FieldInfo{
					"",
					typeid(Field).name(),
					(size_t) & (((T*)0)->*field),
					sizeof(Field),
					TypeInfoStaticAccessor<Field>::info
				}...
			};
			info =
			{
				typeid(T).name(),
				sizeof(T),
				alignof(T),
				sizeof...(Field),
				fields.data(),
				[](void* ptr) {new(ptr) T(); },
				[](void* ptr) {((T*)ptr)->~T(); }
			};

			ReflectionManager::RegisterType<T>(info);
			TypeInfoStaticAccessor<T>::info = &info;

		}
	};


	//#define REGISTER_TYPE(T, ...) static TypeRegister<T> reg##T(T::__VA_ARGS__);
	//#define REGISTER_TYPE(T, ...) static TypeRegister reg##T(__VA_ARGS__);
}


namespace std
{
	using namespace hyecs;
	namespace _
	{

		string depth_tabs(int depth)
		{
			string tabs;
			for (int i = 0; i < depth; i++)
				tabs += "\t";
			return tabs;
		};

		string to_string_type(const TypeInfo* info, int depth)
		{
			if (info == nullptr)
				return "";

			string str = depth_tabs(depth) + info->type_name + "\n";
			depth += 1;
			str += depth_tabs(depth) + "size: " + to_string(info->size) + "\n";
			str += depth_tabs(depth) + "alignment: " + to_string(info->alignment) + "\n";
			str += depth_tabs(depth) + "fields_count: " + to_string(info->fields_count) + "\n";
			for (size_t i = 0; i < info->fields_count; i++)
			{
				str += depth_tabs(depth) + "field: " + info->fields[i].name + "\n";
				if (info->fields[i].type == nullptr)
				{
					str += depth_tabs(depth + 1) + "type: " + info->fields[i].type_name + "\n";
					str += depth_tabs(depth + 1) + "offset: " + to_string(info->fields[i].offset) + "\n";
					str += depth_tabs(depth + 1) + "size: " + to_string(info->fields[i].size) + "\n";
				}
				else
				{
					str += depth_tabs(depth + 1) + "offset: " + to_string(info->fields[i].offset) + "\n";
					str += depth_tabs(depth + 1) + "size: " + to_string(info->fields[i].size) + "\n";
					str += to_string_type(info->fields[i].type, depth + 1) + "\n";
				}

			}

			return str;
		}

		string to_string_base_type(const char* type_name, void* ptr)
		{
			if (strcmp(type_name, "int") == 0)
				return to_string(*(int*)ptr);
			else if (strcmp(type_name, "float") == 0)
				return to_string(*(float*)ptr);
			else if (strcmp(type_name, "double") == 0)
				return to_string(*(double*)ptr);
			else if (strcmp(type_name, "bool") == 0)
				return to_string(*(bool*)ptr);
			else if (strcmp(type_name, "char") == 0)
				return to_string(*(char*)ptr);
			else if (strcmp(type_name, "unsigned char") == 0)
				return to_string(*(unsigned char*)ptr);
			else if (strcmp(type_name, "short") == 0)
				return to_string(*(short*)ptr);
			else if (strcmp(type_name, "unsigned short") == 0)
				return to_string(*(unsigned short*)ptr);
			else if (strcmp(type_name, "long") == 0)
				return to_string(*(long*)ptr);
			else if (strcmp(type_name, "unsigned long") == 0)
				return to_string(*(unsigned long*)ptr);
			else if (strcmp(type_name, "long long") == 0)
				return to_string(*(long long*)ptr);
			else if (strcmp(type_name, "unsigned long long") == 0)
				return to_string(*(unsigned long long*)ptr);
			else if (strcmp(type_name, "string") == 0)
				return *(string*)ptr;
			else
				return "";
		}

		string to_string_type(const TypeInfo& info, void* ptr)
		{
			string str;
			if (info.fields_count > 0)
			{
				str += "(";
				for (size_t i = 0; i < info.fields_count; i++)
				{
					string field_str;
					field_str += string(info.fields[i].name) + ": ";
					if (info.fields[i].type != nullptr)
						field_str += to_string_type(*info.fields[i].type, (char*)ptr + info.fields[i].offset);
					else
						field_str += to_string_base_type(info.fields[i].type_name, (char*)ptr + info.fields[i].offset);
					if (i < info.fields_count - 1)
						field_str += ", ";
					if (field_str.size() > 40)
						field_str = "\n" + field_str + "\n";
					str += field_str;
				}
				str += ")";
			}
			if (info.fields_count == 0)
			{
				str += to_string_base_type(info.type_name, ptr);
			}

			return str;
		}
	}

	string to_string(const TypeInfo& info)
	{
		return _::to_string_type(&info, 0);
	}



	template<typename T>
	string to_string(const T& obj)
	{
		auto& info = TypeInfoOf<T>();
		return _::to_string_type(info, (void*)&obj);
	}


}