#pragma once
#include <array>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>
//constexpr sort : wait for c++20

namespace hyecs
{
	namespace internal
	{
		template <class T>
		constexpr void swap(T& lhs, T& rhs) {
			T tmp = std::move(rhs);
			rhs = std::move(lhs);
			lhs = std::move(tmp);
		}

		template <class InputIt, class UnaryPredicate>
		constexpr InputIt c_find_if_not(InputIt first, InputIt last, UnaryPredicate p) {
			for (; first != last; ++first) {
				if (!p(*first)) {
					return first;
				}
			}
			return last;
		}

		template <class ForwardIt, class UnaryPredicate>
		constexpr ForwardIt c_partition(ForwardIt first,
			ForwardIt last,
			UnaryPredicate p) {
			first = c_find_if_not(first, last, p);
			if (first == last) return first;
			for (ForwardIt i = std::next(first); i != last; ++i) {
				if (p(*i)) {
					swap(*i, *first);
					++first;
				}
			}
			return first;
		}

		template <class RandomIt, class Compare = std::less<>>
		constexpr void quicksort(RandomIt first,
			RandomIt last,
			Compare cmp = Compare{}) {
			if (std::distance(first, last) <= 1) {
				return;
			}
			const auto pivot = *std::prev(last);
			const auto mid1 = c_partition(first, last, [=](const auto& elem) {
				return cmp(elem, pivot);
				});
			const auto mid2 = c_partition(mid1, last, [=](const auto& elem) {
				return !cmp(pivot, elem);
				});
			quicksort(first, mid1, cmp);
			quicksort(mid2, last, cmp);
		}

		template <class Range, class Compare = std::less<>>
		constexpr auto sort(Range&& range , Compare cmp = Compare{}) {
			quicksort(std::begin(range), std::end(range), cmp);
			return std::forward<Range>(range);
		}
	}

}