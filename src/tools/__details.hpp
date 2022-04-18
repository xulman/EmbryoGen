#pragma once

#include <stdexcept>

namespace tools {
namespace details {
inline void boundary_check(std::size_t i, std::size_t size) {
#ifndef NDEBUG
	using namespace std::string_literals;
	if (i >= size)
		throw std::out_of_range("Index "s + std::to_string(i) +
		                        " is >= size (" + std::to_string(size) + ")\n");
#endif
}

template <typename T>
class ptr_iterator {
	T* _ptr = nullptr;

  public:
	ptr_iterator() = default;
	ptr_iterator(T* ptr) : _ptr(ptr) {}

	ptr_iterator& operator++() {
		++_ptr;
		return *this;
	}
	ptr_iterator operator++(int) {
		ptr_iterator cpy = *this;
		++(*this);
		return cpy;
	}
	T& operator*() { return *_ptr; }
	T* operator->() { return _ptr; }
	friend auto operator<=>(const ptr_iterator& lhs,
	                        const ptr_iterator& rhs) = default;
};

template <typename T>
class const_ptr_iterator {
	const T* _ptr = nullptr;

  public:
	const_ptr_iterator() = default;
	const_ptr_iterator(const T* ptr) : _ptr(ptr) {}

	const_ptr_iterator& operator++() {
		++_ptr;
		return *this;
	}
	const_ptr_iterator operator++(int) {
		ptr_iterator cpy = *this;
		++(*this);
		return cpy;
	}
	const T& operator*() const { return *_ptr; }
	const T* operator->() const { return _ptr; }

	friend auto operator<=>(const const_ptr_iterator& lhs,
	                        const const_ptr_iterator& rhs) = default;
};

} // namespace details
} // namespace tools
