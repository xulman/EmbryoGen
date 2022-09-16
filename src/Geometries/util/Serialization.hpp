#pragma once

#include "../../util/Vector3d.hpp"
#include <i3d/image3d.h>

/**
 * This is a container to represent a methods that convert various
 * data types into arrays of bytes (serialization), and the opposite
 * (de-serialization).
 *
 * The methods have common application schema: toBuffer() methods do serialization,
 * 1st param is always the original data type, 2nd param is the receiving buffer,
 * and it returns the number of bytes is has written.
 *
 * The fromBuffer() methods do de-serialization, 1st param is always the input
 * buffer, 2nd param is a reference on a target (to be filled) original type,
 * and it returns the number of bytes it has read.
 *
 * The return values are useful to hint the caller to see how much to advance
 * in the byte buffer.
 */
class Serialization
{
public:
	// -------------- basic integer and real numbers --------------
	static long toBuffer(const short number, char* buffer)
	{
		*((short*)buffer) = number;
		return sizeof(short);
	}

	static long toBuffer(const int number, char* buffer)
	{
		*((int*)buffer) = number;
		return sizeof(int);
	}

	static long toBuffer(const long number, char* buffer)
	{
		*((long*)buffer) = number;
		return sizeof(long);
	}

	static long toBuffer(const size_t number, char* buffer)
	{
		*((size_t*)buffer) = number;
		return sizeof(size_t);
	}

	static long toBuffer(const float number, char* buffer)
	{
		*((float*)buffer) = number;
		return sizeof(float);
	}

	static long toBuffer(const double number, char* buffer)
	{
		*((double*)buffer) = number;
		return sizeof(double);
	}

	// -------------- vectors --------------
	template <typename FT>
	static long toBuffer(const Vector3d<FT>& vector, char* buffer)
	{
		FT* bufferView = (FT*)buffer;

		*bufferView = vector.x; ++bufferView;
		*bufferView = vector.y; ++bufferView;
		*bufferView = vector.z;

		return 3*sizeof(FT);
	}

	template <typename FT>
	static long getSizeInBytes(const Vector3d<FT>&)
	{
		return 3*sizeof(FT);
	}

	// -------------- images --------------
	template <typename VT>
	static long toBuffer(const i3d::Image3d<VT>& image, char* buffer)
	{
		//voxel type         1x int
		//offset,resolution  6x float
		//size               3x long
		//image data         image.GetImageSize() * sizeof(VT)

		long off = 0;
		off += toBuffer(encodeVoxelType(image.GetFirstVoxelAddr()), buffer);

		Vector3d<float> vecFloat;
		Vector3d<size_t> vecSizet;

		off += toBuffer(vecFloat.fromI3dVector3d(image.GetOffset()), buffer+off);
		off += toBuffer(vecFloat.fromI3dVector3d(image.GetResolution().GetRes()), buffer+off);
		off += toBuffer(vecSizet.fromScalars( image.GetSizeX(),image.GetSizeY(),image.GetSizeZ() ), buffer+off);

		VT* bufferView = (VT*)(buffer+off);
		const VT* voxel = image.GetFirstVoxelAddr();
		for (size_t i=0; i < image.GetImageSize(); ++i)
		{
			*bufferView = *voxel;
			++bufferView;
			++voxel;
		}

		return off + (image.GetImageSize()*sizeof(VT));
	}

	template <typename VT>
	static long getSizeInBytes(const i3d::Image3d<VT>& image)
	{
		long size = 0;
		size += 1 * sizeof(int);    //voxel type         1x int
		size += 6 * sizeof(float);  //offset,resolution  6x float
		size += 3 * sizeof(size_t); //size               3x long
		//image data         image.GetImageSize() * sizeof(VT)
		size += (image.GetImageSize()*sizeof(VT));
		return size;
	}

	static int encodeVoxelType(const i3d::GRAY8*)  { return 1; }
	static int encodeVoxelType(const i3d::GRAY16*) { return 2; }
	static int encodeVoxelType(const float*)       { return 3; }
};


/** see documentation of the Serialization class */
class Deserialization
{
public:
	// -------------- basic integer and real numbers --------------
	static long fromBuffer(char* buffer, short& number)
	{
		number = *((short*)buffer);
		return sizeof(short);
	}

	static long fromBuffer(char* buffer, int& number)
	{
		number = *((int*)buffer);
		return sizeof(int);
	}

	static long fromBuffer(char* buffer, long& number)
	{
		number = *((long*)buffer);
		return sizeof(long);
	}

	static long fromBuffer(char* buffer, size_t& number)
	{
		number = *((size_t*)buffer);
		return sizeof(size_t);
	}

	static long fromBuffer(char* buffer, float& number)
	{
		number = *((float*)buffer);
		return sizeof(float);
	}

	static long fromBuffer(char* buffer, double& number)
	{
		number = *((double*)buffer);
		return sizeof(double);
	}

	// -------------- vectors --------------
	template <typename FT>
	static long fromBuffer(char* buffer, Vector3d<FT>& vector)
	{
		FT* bufferView = (FT*)buffer;

		vector.x = *bufferView; ++bufferView;
		vector.y = *bufferView; ++bufferView;
		vector.z = *bufferView;

		return 3*sizeof(FT);
	}

	// -------------- images --------------
	template <typename VT>
	static long fromBuffer(char* buffer, i3d::Image3d<VT>& image)
	{
		int vxType;
		long off = fromBuffer(buffer,vxType);

		//test that voxel of the given image matches the one in the buffer
		if (Serialization::encodeVoxelType(image.GetFirstVoxelAddr()) != vxType)
throw report::rtError(fmt::format("Deserialization mismatch: got image of type {} but buffer provides type {}", Serialization::encodeVoxelType(image.GetFirstVoxelAddr()), vxType));

		//voxel type         1x int
		//offset,resolution  6x float
		//size               3x long
		//image data         image.GetImageSize() * sizeof(VT)

		Vector3d<float> vecFloat;
		Vector3d<size_t> vecSizet;

		off += fromBuffer(buffer+off, vecFloat);
		image.SetOffset(vecFloat.toI3dVector3d());

		off += fromBuffer(buffer+off, vecFloat);
		image.SetResolution(i3d::Resolution(vecFloat.toI3dVector3d()));

		off += fromBuffer(buffer+off, vecSizet);
		image.MakeRoom(vecSizet.x,vecSizet.y,vecSizet.z);

		VT* bufferView = (VT*)(buffer+off);
		VT* voxel = image.GetFirstVoxelAddr();
		for (size_t i=0; i < image.GetImageSize(); ++i)
		{
			*voxel = *bufferView;
			++bufferView;
			++voxel;
		}

		return off + (image.GetImageSize()*sizeof(VT));
	}

	static i3d::Image3d<i3d::GRAY8>*  createImageOfType1() { return new i3d::Image3d<i3d::GRAY8>; }
	static i3d::Image3d<i3d::GRAY16>* createImageOfType2() { return new i3d::Image3d<i3d::GRAY16>; }
	static i3d::Image3d<float>*       createImageOfType3() { return new i3d::Image3d<float>; }
};