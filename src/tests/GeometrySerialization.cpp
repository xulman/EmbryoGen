//
// compile:
//
// g++ -o test -Wall GeometrySerialization.cpp  ../Geometries/*cpp -li3dalgo -li3dcore -Xlinker -defsym -Xlinker MAIN__=main

#include "../Geometries/Geometry.h"
#include "../Geometries/Spheres.h"
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

int main(void)
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
