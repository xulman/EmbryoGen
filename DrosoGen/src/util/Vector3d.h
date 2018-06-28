#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <iostream>
#include <cmath>

///simply a 3D vector...
template <typename T>
class Vector3d
{
public:
	///the vector data
	T x,y,z;

	///default constructor...
	Vector3d(void) :
		x(0), y(0), z(0) {}

	///init constructor...
	Vector3d(const T xx,const T yy,const T zz) :
		x(xx), y(yy), z(zz) {}

	///init constructor...
	Vector3d(const T xyz) :
		x(xyz), y(xyz), z(xyz) {}

	///copy constructor...
	Vector3d(const Vector3d<T>& vec)
	{
		this->x=vec.x;
		this->y=vec.y;
		this->z=vec.z;
	}

	Vector3d<T>& operator=(const Vector3d<T>& vec)
	{
		this->x=vec.x;
		this->y=vec.y;
		this->z=vec.z;
		return( *this );
	}

	Vector3d<T>& operator=(const T scalar)
	{
		this->x=scalar;
		this->y=scalar;
		this->z=scalar;
		return( *this );
	}

	Vector3d<T>& operator+=(const Vector3d<T>& vec)
	{
		this->x+=vec.x;
		this->y+=vec.y;
		this->z+=vec.z;
		return( *this );
	}

	Vector3d<T>& operator-=(const Vector3d<T>& vec)
	{
		this->x-=vec.x;
		this->y-=vec.y;
		this->z-=vec.z;
		return( *this );
	}

	Vector3d<T>& operator*=(const T& scal)
	{
		this->x*=scal;
		this->y*=scal;
		this->z*=scal;
		return( *this );
	}

	Vector3d<T>& operator/=(const T& scal)
	{
		this->x/=scal;
		this->y/=scal;
		this->z/=scal;
		return( *this );
	}

	T inline len(void)
	{
		return static_cast<T>( std::sqrt(len2()) );
	}

	T inline len2(void)
	{
		return (x*x + y*y + z*z);
	}
};

///reports vector as a position coordinate
template <typename T>
std::ostream& operator<<(std::ostream& s,const Vector3d<T>& v)
{
	s << "[" << v.x << "," << v.y << "," << v.z << "]";
	return s;
}

/* uncomment once needed... to prevent from -Wunused-function warnings
///function for dot product of two vectors
static
float dotProduct(const Vector3d<float>& u, const Vector3d<float>& v)
{
	return u.x*v.x+u.y*v.y;
}
*/

/** calculates addition of two vectors: vecA + vecB */
template <typename T>
Vector3d<T> operator+(const Vector3d<T>& vecA, const Vector3d<T>& vecB)
{
	Vector3d<T> res(vecA);
	res+=vecB;
	return (res);
}

/** calculates difference of two vectors: vecA - vecB */
template <typename T>
Vector3d<T> operator-(const Vector3d<T>& vecA, const Vector3d<T>& vecB)
{
	Vector3d<T> res(vecA);
	res-=vecB;
	return (res);
}

/** calculates scalar multiplication with vector: scal * vec */
template <typename T>
Vector3d<T> operator*(const T scal, const Vector3d<T>& vec)
{
	Vector3d<T> res(vec);
	res *= scal;
	return (res);
}
// ----------------------------------------------------------------------------


///current (lightweight) representation of the force name
typedef const char* ForceName;

/** a placeholder for ForceName type of "unknown force",
    to prevent of allocating it over and over again */
static ForceName unknownForceType = "unknown force";

///essentially a "named" 3D vector
template <typename T>
class ForceVector3d : public Vector3d<T>
{
public:
	///type of the force (only to find out how to report it)
	ForceName type;

	///default constructor...
	ForceVector3d(void) : Vector3d<T>(), type(unknownForceType) {}

	///init constructor...
	ForceVector3d(const T xx,const T yy,const T zz,const ForceName tt) :
			Vector3d<T>(xx,yy,zz), type(tt) {}

	///init constructor...
	ForceVector3d(const T xyz,const ForceName tt) :
			Vector3d<T>(xyz), type(tt) {}

	///init constructor...
	ForceVector3d(const Vector3d<T>& v,const ForceName tt) :
			Vector3d<T>(v), type(tt) {}

	///copy constructor...
	ForceVector3d(const ForceVector3d<T>& vec) : Vector3d<T>(vec.x,vec.y,vec.z)
	{
		this->type=vec.type;
	}

	ForceVector3d<T>& operator=(const Vector3d<T>& vec)
	{
		this->x=vec.x;
		this->y=vec.y;
		this->z=vec.z;
		this->type = vec.type;
		return( *this );
	}

	ForceVector3d<T>& operator=(const T scalar)
	{
		this->x=scalar;
		this->y=scalar;
		this->z=scalar;
		this->type = unknownForceType;
		return( *this );
	}
};
#endif
