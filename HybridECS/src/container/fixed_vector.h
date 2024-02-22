#pragma once
#include <memory>

namespace hyecs
{
#if defined _MSC_VER
#define no_unique_address msvc::no_unique_address
#endif
	/// <summary>
	/// light wieght vector with fixed capacity
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <typeparam name="Alloc"></typeparam>
	template<typename T, typename Alloc = std::allocator<T>>
	class fixed_vector
	{
		static_assert(!_ENFORCE_MATCHING_ALLOCATORS || std::is_same_v<T, typename Alloc::value_type>,
			_MISMATCHED_ALLOCATOR_MESSAGE("vector<T, Allocator>", "T"));
		static_assert(std::is_object_v<T>, "The C++ Standard forbids containers of non-object types "
			"because of [container.requirements].");

		using AllocTrait = std::allocator_traits<Alloc>;

	public:

		using value_type = T;
		using allocator_type = Alloc;
		using pointer = typename AllocTrait::pointer;
		using const_pointer = typename AllocTrait::const_pointer;
		using reference = T&;
		using const_reference = const T&;
		using size_type = typename AllocTrait::size_type;
		using difference_type = typename AllocTrait::difference_type;

		using iterator = pointer;
		using const_iterator = const_pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	private:
		pointer begin_addr;
		pointer end_addr;
		const size_t capacity;
		[[no_unique_address]] Alloc alloc;

	public:

		fixed_vector(uint32_t capacity) : capacity(capacity)
		{
			begin_addr = alloc.allocate(capacity);
		}
		~fixed_vector()
		{
			alloc.deallocate(begin_addr, capacity);
		}
		fixed_vector(const fixed_vector& other) : capacity(other.capacity)
		{
			begin_addr = alloc.allocate(capacity);
			end_addr = begin_addr;
			for (auto it = other.begin(); it != other.end(); it++)
				push_back(*it);
		}
		fixed_vector(fixed_vector&& other) noexcept : capacity(other.capacity)
		{
			begin_addr = other.begin_addr;
			end_addr = other.end_addr;
			other.begin_addr = nullptr;
			other.end_addr = nullptr;
			other.capacity = 0;
		}
		fixed_vector& operator=(const fixed_vector& other)
		{
			if (this == &other)
				return *this;
			clear();
			for (auto it = other.begin(); it != other.end(); it++)
				push_back(*it);
			return *this;
		}

		pointer data() noexcept
		{
			return begin_addr;
		}


		T& operator[](uint32_t index)
		{
			return begin_addr[index];
		}

		const T& operator[](uint32_t index) const
		{
			return begin_addr[index];
		}

		size_t size() const noexcept
		{
			return end_addr - begin_addr;
		}

		template<typename... Args>
		void emplace_back(Args&&... args)
		{
			AllocTrait::construct(alloc, end_addr, std::forward<Args>(args)...);
			end_addr++;
		}

		void push_back(const T& value)
		{
			AllocTrait::construct(alloc, end_addr, value);
			end_addr++;
		}

		void push_back(T&& value)
		{
			AllocTrait::construct(alloc, end_addr, std::move(value));
			end_addr++;
		}

		void pop_back()
		{
			end_addr--;
			AllocTrait::destroy(alloc, end_addr);
		}

		void clear()
		{

			for (auto it = begin(); it != end(); it++)
				AllocTrait::destroy(alloc, it);
			end_addr = begin_addr;
		}

		iterator begin() noexcept
		{
			return begin_addr;
		}

		const_iterator begin() const noexcept
		{
			return begin_addr;
		}

		const_iterator cbegin() const noexcept
		{
			return begin_addr;
		}

		iterator end() noexcept
		{
			return end_addr;
		}

		const_iterator end() const noexcept
		{
			return end_addr;
		}

		const_iterator cend() const noexcept
		{
			return end_addr;
		}

		reverse_iterator rbegin() noexcept
		{
			return reverse_iterator(end_addr);
		}

		const_reverse_iterator rbegin() const noexcept
		{
			return reverse_iterator(end_addr);
		}

		const_reverse_iterator crbegin() const noexcept
		{
			return reverse_iterator(end_addr);
		}

		reverse_iterator rend() noexcept
		{
			return reverse_iterator(begin_addr);
		}

		const_reverse_iterator rend() const noexcept
		{
			return reverse_iterator(begin_addr);
		}

		const_reverse_iterator crend() const noexcept
		{
			return reverse_iterator(begin_addr);
		}

		bool empty() const noexcept
		{
			return begin_addr == end_addr;
		}
	};
}