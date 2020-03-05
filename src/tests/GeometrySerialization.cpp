//
// compile:
//
// g++ -o test -Wall GeometrySerialization.cpp  ../Geometries/*cpp -li3dalgo -li3dcore -Xlinker -defsym -Xlinker MAIN__=main

#include "../Geometries/Geometry.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/util/Serialization.h"
#include <iostream>

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


int main(void)
{
	//testSpheres();
	testSeriDeseri();
}
