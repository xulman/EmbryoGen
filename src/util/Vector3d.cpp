#include "Vector3d.hpp"

/** template specialization for Vector3d<int>::toPixels(any Vector3d<>),
    this specialization is avoiding std::floor() because this implicitly
    happens during the casting to int */
template <>            //specialization of Vector3d<int>
template <typename FT> //FT = foreign type
Vector3d<int>& Vector3d<int>::toPixels(const Vector3d<FT>& floatPxPos)
{
	x = (int)floatPxPos.x;
	y = (int)floatPxPos.y;
	z = (int)floatPxPos.z;
	return *this;
}

/** template specialization for Vector3d<size_t>::toPixels(any Vector3d<>),
    this specialization is avoiding std::floor() because this implicitly
    happens during the casting to size_t */
template <>            //specialization of Vector3d<size_t>
template <typename FT> //FT = foreign type
Vector3d<size_t>& Vector3d<size_t>::toPixels(const Vector3d<FT>& floatPxPos)
{
	x = (size_t)floatPxPos.x;
	y = (size_t)floatPxPos.y;
	z = (size_t)floatPxPos.z;
	return *this;
}

//needs explicit instantiation to make these exist because, being hidden inside
//the cpp file, no one outside can request these to exist (and the compiler+linker
//will happily use the generic code instead of these specializations)
template Vector3d<int>& Vector3d<int>::toPixels(const Vector3d<float>&  floatPxPos);
template Vector3d<int>& Vector3d<int>::toPixels(const Vector3d<double>& floatPxPos);
template Vector3d<size_t>& Vector3d<size_t>::toPixels(const Vector3d<float>&  floatPxPos);
template Vector3d<size_t>& Vector3d<size_t>::toPixels(const Vector3d<double>& floatPxPos);

/** template specialization for Vector3d<size_t>::toPixels(), this specialization
    is avoiding std::floor() because there's nothing to round (we're integer type) */
template <>
Vector3d<int>& Vector3d<int>::toPixels(void)
{
	return *this;
}

/** template specialization for Vector3d<size_t>::toPixels(), this specialization
    is avoiding std::floor() because there's nothing to round (we're integer type) */
template <>
Vector3d<size_t>& Vector3d<size_t>::toPixels(void)
{
	return *this;
}
