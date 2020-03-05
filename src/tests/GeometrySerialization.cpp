//
// compile:
//
// g++ -o test -Wall GeometrySerialization.cpp  ../Geometries/*cpp -li3dalgo -li3dcore -Xlinker -defsym -Xlinker MAIN__=main

#include "../Geometries/Geometry.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/ScalarImg.h"
#include "../Geometries/VectorImg.h"
#include "../Geometries/util/Serialization.h"
#include <iostream>
#include <i3d/image3d.h>

void describeSphere(Spheres& s)
{
	std::cout << "noOfSpheres = " << s.getNoOfSpheres() << "\n";
	for (int i=0; i < s.getNoOfSpheres(); ++i)
	{
		std::cout << i << ": c.x = " << s.getCentres()[i].x << "\n";
		std::cout << i << ": c.y = " << s.getCentres()[i].y << "\n";
		std::cout << i << ": c.z = " << s.getCentres()[i].z << "\n";
		std::cout << i << ": radius = " << s.getRadii()[i] << "\n";
	}
	std::cout << "------\n";
}

void testSpheres(void)
{
	//some testing sphere
	Spheres s(5);

	s.updateCentre(0, Vector3d<FLOAT>(1.f,2.f,3.f));
	s.updateCentre(1, Vector3d<FLOAT>(44.f,5.f,6.f));
	s.updateCentre(2, Vector3d<FLOAT>(7.f,8.f,9.f));
	s.updateRadius(0, 10.f);
	s.updateRadius(1, 22.f);
	s.updateRadius(2, 30.f);

	//test metadata
	std::cout << "Spheres type: " << s.getType() << "\n";
	std::cout << "Spheres size: " << s.getSizeInBytes() << "\n";
	
	//test plain data
	char* buffer = new char[ s.getSizeInBytes() ];

	s.serializeTo(buffer);
	std::cout << "seri done\n";

	Spheres& d = *Spheres::createAndDeserializeFrom(buffer);
	std::cout << "deseri done\n";

	//compare visually
	describeSphere(s);
	describeSphere(d);

	//one last test & compare
	s.deserializeFrom(buffer);
	describeSphere(s);

	//should fail!
	try {
		Spheres xx(2);
		xx.deserializeFrom(buffer);
	}
	catch (...) {
		std::cout << "failed as expected\n";
	}
}


void testSeriDeseri(void)
{
	short  sVal = 10;
	int    iVal = 20;
	long   lVal = 30;
	float  fVal = 10.4f;
	double dVal = 20.8;
	//
	short  got_sVal;
	int    got_iVal;
	long   got_lVal;
	float  got_fVal;
	double got_dVal;

	char* buffer = new char[1024];
	long a,b;

	a = Serialization::toBuffer(sVal,buffer);
	b = Deserialization::fromBuffer(buffer,got_sVal);
	std::cout << "sVal: in=" << sVal << ", out=" << got_sVal << ", offsets: " << a << "," << b << "\n";
	//
	a = Serialization::toBuffer(iVal,buffer);
	b = Deserialization::fromBuffer(buffer,got_iVal);
	std::cout << "iVal: in=" << iVal << ", out=" << got_iVal << ", offsets: " << a << "," << b << "\n";
	//
	a = Serialization::toBuffer(lVal,buffer);
	b = Deserialization::fromBuffer(buffer,got_lVal);
	std::cout << "lVal: in=" << lVal << ", out=" << got_lVal << ", offsets: " << a << "," << b << "\n";
	//
	a = Serialization::toBuffer(fVal,buffer);
	b = Deserialization::fromBuffer(buffer,got_fVal);
	std::cout << "fVal: in=" << fVal << ", out=" << got_fVal << ", offsets: " << a << "," << b << "\n";
	//
	a = Serialization::toBuffer(dVal,buffer);
	b = Deserialization::fromBuffer(buffer,got_dVal);
	std::cout << "dVal: in=" << dVal << ", out=" << got_dVal << ", offsets: " << a << "," << b << "\n";


	sVal += 10;
	iVal += 20;
	lVal += 30;
	fVal += 10.4f;
	dVal += 20.8;
	//
	long curOff = 0;
	curOff += Serialization::toBuffer(sVal,buffer+curOff);
	curOff += Serialization::toBuffer(iVal,buffer+curOff);
	curOff += Serialization::toBuffer(lVal,buffer+curOff);
	curOff += Serialization::toBuffer(fVal,buffer+curOff);
	curOff += Serialization::toBuffer(dVal,buffer+curOff);
	std::cout << "written in total: " << curOff << "\n";
	//
	curOff = 0;
	curOff += Deserialization::fromBuffer(buffer+curOff,got_sVal);
	curOff += Deserialization::fromBuffer(buffer+curOff,got_iVal);
	curOff += Deserialization::fromBuffer(buffer+curOff,got_lVal);
	curOff += Deserialization::fromBuffer(buffer+curOff,got_fVal);
	curOff += Deserialization::fromBuffer(buffer+curOff,got_dVal);
	std::cout << "read in total: " << curOff << "\n";
	//
	std::cout << "sVal: in=" << sVal << ", out=" << got_sVal << "\n";
	std::cout << "iVal: in=" << iVal << ", out=" << got_iVal << "\n";
	std::cout << "lVal: in=" << lVal << ", out=" << got_lVal << "\n";
	std::cout << "fVal: in=" << fVal << ", out=" << got_fVal << "\n";
	std::cout << "dVal: in=" << dVal << ", out=" << got_dVal << "\n";


	Vector3d<float> vf(2.f,2.5f,2.8f),got_vf;
	Vector3d<short> vs(4,5,6),got_vs;
	std::cout << "before: " << vf << " == " << got_vf << "\n";
	std::cout << "before: " << vs << " == " << got_vs << "\n";
	//
	curOff = 0;
	curOff += Serialization::toBuffer(vf,buffer+curOff);
	curOff += Serialization::toBuffer(vs,buffer+curOff);
	std::cout << "written in total: " << curOff << "\n";
	//
	curOff = 0;
	curOff += Deserialization::fromBuffer(buffer+curOff,got_vf);
	curOff += Deserialization::fromBuffer(buffer+curOff,got_vs);
	std::cout << "read in total: " << curOff << "\n";
	std::cout << " after: " << vf << " == " << got_vf << "\n";
	std::cout << " after: " << vs << " == " << got_vs << "\n";
}

template <typename VT>
long noOfPixelDifferences(const i3d::Image3d<VT>& a, const i3d::Image3d<VT>& b)
{
	const VT* aa = a.GetFirstVoxelAddr();
	const VT* bb = b.GetFirstVoxelAddr();
	const VT* const aaT = aa + a.GetImageSize();

	long cnt = 0;

	while (aa != aaT)
	{
		if (*aa != *bb) ++cnt;
		++aa; ++bb;
	}

	return cnt;
}

template <class VOXEL>
void GetImageInfo(const i3d::Image3d<VOXEL> & image);
void testImages(void)
{
	i3d::Image3d<float> fimg;
	fimg.MakeRoom(200,100,300);
	fimg.SetOffset(i3d::Vector3d<float>(999.9f,99.9f,9.9f));
	fimg.SetResolution(i3d::Resolution(9.5f,99.5f,999.5f));
	GetImageInfo(fimg);

	for (size_t z=0; z < fimg.GetSizeZ(); ++z)
	for (size_t y=0; y < fimg.GetSizeY(); ++y)
	for (size_t x=0; x < fimg.GetSizeX(); ++x)
	fimg.SetVoxel(x,y,z,(float)z);

	const long bufSize = Serialization::getSizeInBytes(fimg);
	std::cout << "can be serialized into " << bufSize << " bytes\n";

	char* buffer = new char[bufSize];
	std::cout << "offset shift: " << Serialization::toBuffer(fimg,buffer) << "\n";

	i3d::Image3d<float> got_fimg;
	i3d::Image3d<i3d::GRAY8> wrongImg;

	std::cout << "offset shift: " << Deserialization::fromBuffer(buffer,got_fimg) << "\n";
	GetImageInfo(got_fimg);
	std::cout << "no of different pixels: " << noOfPixelDifferences(fimg,got_fimg) << "\n";

	try {
		Deserialization::fromBuffer(buffer,wrongImg);
	}
	catch (std::runtime_error* e) {
		std::cout << "something is correctly wrong:\n";
		std::cout << e->what() << "\n";
	}

	std::cout << "----------------------------------\n";

	i3d::Image3d<i3d::GRAY8> gimg,got_gimg;
	gimg.MakeRoom(200,100,300);
	gimg.SetOffset(i3d::Vector3d<float>(999.9f,99.9f,9.9f));
	gimg.SetResolution(i3d::Resolution(9.5f,99.5f,999.5f));
	GetImageInfo(gimg);

	const long bufSizeB = Serialization::getSizeInBytes(gimg);
	std::cout << "can be serialized into " << bufSizeB << " bytes\n";

	char* bufferB = new char[bufSizeB];
	std::cout << "offset shift: " << Serialization::toBuffer(gimg,bufferB) << "\n";
	std::cout << "offset shift: " << Deserialization::fromBuffer(bufferB,got_gimg) << "\n";
	GetImageInfo(got_gimg);
	std::cout << "no of different pixels: " << noOfPixelDifferences(gimg,got_gimg) << "\n";
}


void testScalarImgGeometry()
{
	i3d::Image3d<i3d::GRAY8> initShape("../../DrosophilaYolk_mask_lowerRes.tif");
	ScalarImg si(initShape,ScalarImg::DistanceModel::GradIN_ZeroOUT);

	std::cout << "template image params:\n";
	GetImageInfo(initShape);

	long bufSize = si.getSizeInBytes();
	char* buffer = new char[bufSize];

	si.serializeTo(buffer);
	ScalarImg& sii = *ScalarImg::createAndDeserializeFrom(buffer);

	std::cout << "orig image size: " <<  si.getSizeInBytes() << "\n";
	std::cout << " new image size: " << sii.getSizeInBytes() << "\n";
	//
	GetImageInfo( si.getDistImg());
	GetImageInfo(sii.getDistImg());
}


void testVectorImgGeometry()
{
	i3d::Image3d<i3d::GRAY8> initShape("../../DrosophilaYolk_mask_lowerRes.tif");
	VectorImg vi(initShape,VectorImg::ChoosingPolicy::avgVec);

	std::cout << "template image params:\n";
	GetImageInfo(initShape);

	long bufSize = vi.getSizeInBytes();
	char* buffer = new char[bufSize];

	vi.serializeTo(buffer);
	VectorImg& vii = *VectorImg::createAndDeserializeFrom(buffer);

	std::cout << "orig image size: " <<  vi.getSizeInBytes() << "\n";
	std::cout << " new image size: " << vii.getSizeInBytes() << "\n";
	//
	Vector3d<FLOAT> vec;
	vi.getVector(20,30,40,vec);
	std::cout << "orig vector: " << vec << "\n";
	vii.getVector(20,30,40,vec);
	std::cout << " new vector: " << vec << "\n";
}


int main(void)
{
	//testSpheres();
	//testSeriDeseri();
	//testImages();
	testScalarImgGeometry();
	//testVectorImgGeometry();
}


template <class VOXEL>
void GetImageInfo(const i3d::Image3d<VOXEL> & image)
{
	 using namespace std;
    cout << "Image size px: " << image.GetSize() << " px\n";
    cout << "Image size um: " << i3d::PixelsToMicrons(image.GetSize(),image.GetResolution()) << " um\n";
    cout << "Image resolution: " << image.GetResolution().GetRes() << " px/um\n";
    cout << "Image offset: " << image.GetOffset() << " um\n";
	 size_t ImageSize=image.GetImageSize()*sizeof(VOXEL);
	 cout << "Image size: " << ImageSize << " B\n";
	 if (ImageSize > (size_t(1) << 30)) cout << "Image size human: " << (ImageSize >> 30) << " GB\n";
	 else
	 if (ImageSize > (size_t(1) << 20)) cout << "Image size human: " << (ImageSize >> 20) << " MB\n";
	 else
	 if (ImageSize > (size_t(1) << 10)) cout << "Image size human: " << (ImageSize >> 10) << " kB\n";
	 else
	 cout << "Image size human: " << ImageSize << " B\n";
}
