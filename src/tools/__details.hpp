#pragma once

#include <stdexcept>
#include <string>

namespace tools {
namespace details {
#ifdef NDEBUG
constexpr bool debug_macro = false;
#else
constexpr bool debug_macro = true;
#endif

inline void
boundary_check(std::size_t i, std::size_t size, const std::string& msg = "") {
	if (!debug_macro)
		return;

	using namespace std::string_literals;
	if (i >= size)
		throw std::out_of_range("Index "s + std::to_string(i) +
		                        " is >= size: " + std::to_string(size) + "\n" +
		                        msg + "\n");
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

	operator T*() { return _ptr; }
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

	operator const T*() { return _ptr; }
};

} // namespace details
} // namespace tools