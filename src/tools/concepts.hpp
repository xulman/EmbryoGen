#pragma once

#include <concepts>
#include <type_traits>

namespace tools {
namespace concepts {
template <typename T>
concept basic_container = requires(T a,typename T::value_type b) {
	{ a.size() } -> std::convertible_to<std::size_t>;
	{a.push_back(b)};
	{a.begin()};
	{a.end()};
	{a.clear()};
	{a.emplace_back(b)};
};

template <typename T, typename U>
concept basic_container_v = requires(T a, U b) {
	requires basic_container<T>;
	std::is_same_v<typename T::value_type, U>;
};
} // namespace concepts
} // namespace tools
