#include "../DisplayUnits/SceneryDisplayUnit.h"
#include "../Geometries/util/SpheresFunctions.h"

void showGeom(DisplayUnit& du, const Spheres* srcGeom, const Spheres* expandedGeom)
{
	int i = 0;
	std::cout << "showing " << srcGeom->getNoOfSpheres() << " src spheres\n";
	for (; i < srcGeom->getNoOfSpheres(); ++i)
	{
		du.DrawPoint(i,srcGeom->getCentres()[i],srcGeom->getRadii()[i],2);
		//du.DrawVector(i,futureGeometry.getCentres()[i],orientVec,0);
	}

	if (expandedGeom == NULL)
	{
		std::cout << "showing NO expanded spheres\n";
		return;
	}
	std::cout << "showing " << expandedGeom->getNoOfSpheres() << " expanded spheres\n";
	for (; i < expandedGeom->getNoOfSpheres(); ++i)
	{
		du.DrawPoint(i,expandedGeom->getCentres()[i],expandedGeom->getRadii()[i],3);
		//du.DrawVector(i,futureGeometry.getCentres()[i],orientVec,0);
	}
}


void testBuildAndShowPlan(const Spheres& srcGeom)
{
	SpheresFunctions::Interpolator<FLOAT> si(srcGeom);
	si.printPlan();

	si.addToPlan(0,1,1);
	si.addToPlan(1,2,2);
	si.addToPlan(3,4,1);
	si.addToPlan(3,4,2);
	si.addToPlan(3,4,1);
	si.printPlan();

	std::cout << "removing 3,4,false\n";
	si.removeFromPlan(3,4);
	si.printPlan();

	std::cout << "removing 3,4,true\n";
	si.removeFromPlan(3,4,true);
	si.printPlan();
}


void testExpansion(DisplayUnit& du, const Spheres& srcGeom)
{
	SpheresFunctions::Interpolator<FLOAT> si(srcGeom);
	si.addToPlan(0,1,2);
	si.addToPlan(1,2,2);
	si.addToPlan(0,3,2);
	si.addToPlan(3,4,2);
	si.addToPlan(4,2,2);
	si.printPlan();

	Spheres expandedGeom(si.getOptimalTargetSpheresNo());
	si.expandSrcIntoThis(expandedGeom);
	showGeom(du,&srcGeom,&expandedGeom);
}


int main(void)
{
	SceneryDisplayUnit du("localhost:8765");

	Spheres s(5);
	s.updateCentre(0,Vector3d<FLOAT>(10, 0,0));
	s.updateCentre(1,Vector3d<FLOAT>(20, 0,0));
	s.updateCentre(2,Vector3d<FLOAT>(30, 0,0));
	s.updateCentre(3,Vector3d<FLOAT>(15,10,0));
	s.updateCentre(4,Vector3d<FLOAT>(25,10,0));
	s.updateRadius(0,6);
	s.updateRadius(1,4);
	s.updateRadius(2,6);
	s.updateRadius(3,5);
	s.updateRadius(4,7);
	showGeom(du,&s,NULL);

	//testBuildAndShowPlan(s);
	testExpansion(du,s);

	return 0;
}
