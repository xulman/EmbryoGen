#pragma once

#include "../__details.hpp"
#include "StaticVector.hpp"
#include <cstring>
#include <vector>

namespace tools {

namespace structures {

template <typename T, std::size_t N>
class SmallVector {
  private:
	std::vector<T> _dynamic_data;
	StaticVector<T, N> _static_data;

	bool _in_dynamic = false;

	void static_to_dynamic() {
		_dynamic_data.reserve(N * 2);
		for (std::size_t i = 0; i < N; ++i)
			_dynamic_data.push_back(std::move(_static_data[i]));

		_in_dynamic = true;
	}

	T* valid_data() {
		if (_in_dynamic)
			return &_dynamic_data[0];
		return _static_data.begin();
	}

	const T* valid_data() const {
		if (_in_dynamic)
			return &_dynamic_data[0];
		return _static_data.begin();
	}

  public:
	using value_type = T;
	constexpr static std::size_t static_size = N;

	SmallVector() = default;
	SmallVector(const SmallVector&) = default;
	SmallVector(SmallVector&&) = default;
	SmallVector<T, N>& operator=(const SmallVector&) = default;
	SmallVector<T, N>& operator=(SmallVector&&) = default;

	std::size_t size() const noexcept {
		if (_in_dynamic)
			return _dynamic_data.size();
		return _static_data.size();
	}

	void push_back(T elem) {
		if (!_in_dynamic) {
			if (_static_data.size() < N) {
				_static_data.push_back(std::move(elem));
				return;
			}

			static_to_dynamic();
		}
		_dynamic_data.push_back(std::move(elem));
	}

	void erase_at(std::size_t idx) {
		details::boundary_check(idx, size(),
		                        "SmallVector: erase_at() on invalid index");

		if (_in_dynamic)
			_dynamic_data.erase(_dynamic_data.begin() + idx);
		else
			_static_data.erase_at(idx);
	}

	T& back() { return (*this)[size() - 1]; }
	const T& back() const { return (*this)[size() - 1]; }

	template <typename... Args>
	T& emplace_back(Args&&... args) {
		push_back(T(std::forward<Args>(args)...));
		return back();
	}

	void pop_back() {
		details::boundary_check(0, size(), "SmallVector: pop_back() failed");

		if (_in_dynamic)
			_dynamic_data.pop_back();
		else
			_static_data.pop_back();
	}

	void clear() {
		_static_data.clear();
		_dynamic_data.clear();
		_in_dynamic = false;
	}

	auto begin() noexcept { return details::ptr_iterator(valid_data()); }
	auto end() noexcept { return details::ptr_iterator(valid_data() + size()); }
	auto cbegin() const noexcept { return details::ptr_iterator(valid_data()); }
	auto cend() const noexcept {
		return details::ptr_iterator(valid_data() + size());
	}
	auto begin() const noexcept { return cbegin(); }
	auto end() const noexcept { return cend(); }

	T& operator[](std::size_t i) {
		details::boundary_check(i, size(),
		                        "SmallVector: operator[] on invalid index");
		return valid_data()[i];
	}

	const T& operator[](std::size_t i) const {
		details::boundary_check(
		    i, size(), "SmallVector: cosnt operator[] on invalid index");
		return valid_data()[i];
	}
};

template <typename T>
using SmallVector3 = SmallVector<T, 3>;

template <typename T>
using SmallVector4 = SmallVector<T, 4>;

template <typename T>
using SmallVector5 = SmallVector<T, 5>;

template <typename T>
using SmallVector10 = SmallVector<T, 10>;

} // namespace structures

} // namespace tools
