#pragma once
#include "pch.h"

#include "core/data_registry.h"
#include "lib/std_lib.h"
#include "container/type_hash.h"

using namespace hyecs;

struct A {};


int main()
{

	constexpr static_string str1 = "hello";
	constexpr auto str1_sub = str1.sub_string<1, 4>();
	constexpr static_string str2 = "world";
	constexpr auto str3 = str1 +" "+ str2;
	const char* str = str3;


	constexpr auto hash = type_hash::of<A>;

	std::cout << (uint64_t)hash << std::endl;

}