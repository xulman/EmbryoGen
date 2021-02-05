#include <iostream>
#include <cmath>
#include "../DisplayUnits/SceneryDisplayUnit.h"
#include "../Geometries/util/SpheresFunctions.h"

void showGeom(DisplayUnit& du, const Spheres* srcGeom, const Spheres* expandedGeom)
{
	int i = 0;
	std::cout << "showing " << srcGeom->getNoOfSpheres() << " src spheres\n";
	for (; i < srcGeom->getNoOfSpheres(); ++i)
	{
		du.DrawPoint(i,srcGeom->getCentres()[i],srcGeom->getRadii()[i],3);
		//du.DrawVector(i,futureGeometry.getCentres()[i],orientVec,0);
	}

	if (expandedGeom == NULL)
	{
		std::cout << "showing NO expanded spheres\n";
		return;
	}
	std::cout << "showing " << expandedGeom->getNoOfSpheres() << " expanded spheres\n";
	for (i=0; i < expandedGeom->getNoOfSpheres(); ++i)
	{
		du.DrawPoint(10+i,expandedGeom->getCentres()[i],expandedGeom->getRadii()[i],2);
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


void mainForInterpolator(void)
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
}
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

#include "../util/rnd_generators.h"

void mainForLinkedSpheres(DisplayUnit& du)
{
	//reference two balls
	Spheres twoS(2);

	//initial geom of the two balls
	const float sDist = 4;
	const float maxAxisDev = 2.4f;
	twoS.updateCentre(0, Vector3d<float>(-9.f,
				GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist),
				GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist)));
	twoS.updateCentre(1, Vector3d<float>(+9.f,
				GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist),
				GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist)));
	//twoS.updateRadius(0, 3.0);
	//twoS.updateRadius(1, 4.5);
	twoS.updateRadius(0, 3.0);
	twoS.updateRadius(1, 1.0);

	//populate via LinkedSpheres
	SpheresFunctions::LinkedSpheres<float> builder(twoS, Vector3d<float>(0,1,0));

	//regularly placed lines
	float minAngle = -M_PI_2 *0.5f;
	float maxAngle = +M_PI_2 *0.5f;
	float stepAngle = (maxAngle-minAngle)/3.0f;
	//builder.resetAllAzimuthsToExtrusions(minAngle, stepAngle, maxAngle);
	builder.resetAllAzimuthsToExtrusions(minAngle, stepAngle, maxAngle, [](float f){ return 10*(sin(f*M_PI)+0.3*sin(f*4*M_PI));});
	builder.resetNoOfSpheresInAllAzimuths(19);

	//explicitly placed lines
	builder.defaultNoOfSpheresOnConnectionLines=2;
	builder.addOrChangeAzimuthToExtrusion(M_PI-0.4);
	builder.addOrChangeAzimuthToExtrusion(M_PI+0.4);

	//explicitly placed lines with own azimuth-shaker
	float myAng = M_PI;
	builder.addOrChangeAzimuth(myAng,
		SpheresFunctions::LinkedSpheres<float>::AzimuthDrivenPositionExtruder(myAng,builder,[](float){return 5.f;}),
		builder.defaultRadiusNoChg,
		5);

	//explicitly placed lines with specialities
	builder.addOrChangeAzimuth(myAng+000.1, builder.defaultPosNoAdjustment, [](float,float f){return 2.f - 2.f*std::abs(f-0.5f);}, 4);
	//builder.addOrChangeAzimuth(M_PI,[](Vector3d<float>& v,float f){v += Vector3d<float>(0,0,f-0.5f +15);},builder.defaultRadiusNoChg, 4);
	//builder.addOrChangeAzimuth(M_PI, builder.defaultPosNoAdjustment, builder.defaultRadiusNoChg, 4);

	//test for removal
	//builder.removeAzimuth(M_PI+0.2);
	//builder.removeAzimuth(8432903482);

	//builder.addToPlan(0,1,3); //content of Interpolator is forbidden in LinkedSpheres
	builder.printPlan();
	REPORT("necessary cnt: " << builder.getNoOfNecessarySpheres());

	//finally, commit the case and draw it
	Spheres manyS(builder.getNoOfNecessarySpheres());
	builder.buildInto(manyS);
	builder.printPlan();

	Vector3d<float> centre(twoS.getCentres()[0]);
	centre += twoS.getCentres()[1];
	centre *= 0.5f;

	int vecID = 1000;
	du.DrawVector(vecID++, centre, builder.getBasalSideAxis(), 1); //basal vector red
	du.DrawVector(vecID++, centre, builder.getRectifiedBasalDir(), 5); //rectified basal vector magenta
	du.DrawVector(vecID++, centre, builder.getAux3rdAxis(), 4); //aux 3rd cyan
	du.DrawLine(vecID++, twoS.getCentres()[0], twoS.getCentres()[1], 1); //main axis in red

	int cnt = 1;
	float refRadius = twoS.getRadii()[0];
	Vector3d<float> refCentre = twoS.getCentres()[1];

	char key = 0;
	while (key != 'a')
	{
		showGeom(du, &twoS, &manyS);
		builder.printSkeleton(du,3000,7, manyS);

		std::cin >> key;

		if (key == 'r')
		{
			twoS.updateRadius(0, refRadius + 2*sin(cnt));
		}
		if (key == 'p')
		{
			twoS.updateCentre(1, refCentre + Vector3d<float>(0, 4*sin(0.4*cnt), 0));
		}
		if (key == 'u')
		{
			builder.refreshThis(manyS);
		}
		if (key == 'n')
		{
			builder.rebuildInto(manyS);
		}
		++cnt;
	}

}


void mainForSimplyLinkedSpheres(DisplayUnit& du)
{
	//reference two balls
	Spheres twoS(2);

	//initial geom of the two balls
	const float sDist = 4;
	const float maxAxisDev = 2.4f;
	twoS.updateCentre(0, Vector3d<float>(-9.f,
				GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist),
				GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist)));
	twoS.updateCentre(1, Vector3d<float>(+9.f,
				GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist),
				GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist)));
	//twoS.updateRadius(0, 3.0);
	//twoS.updateRadius(1, 4.5);
	twoS.updateRadius(0, 3.0);
	twoS.updateRadius(1, 1.0);

	//populate via LinkedSpheres
	SpheresFunctions::LinkedSpheres<float> builder(twoS, Vector3d<float>(0,1,0));
	builder.addOrChangeAzimuth(0, builder.defaultPosNoAdjustment, builder.defaultRadiusNoChg, 4);

	//commit the case and draw it
	Spheres manyS(builder.getNoOfNecessarySpheres());
	builder.buildInto(manyS);
	builder.printPlan();

	int cnt = 1;
	float refRadius = twoS.getRadii()[0];
	Vector3d<float> refCentre = twoS.getCentres()[1];

	char key = 0;
	while (key != 'a')
	{
		showGeom(du, &twoS, &manyS);

		std::cin >> key;

		if (key == 'r')
		{
			twoS.updateRadius(0, refRadius + 2*sin(cnt));
		}
		if (key == 'p')
		{
			twoS.updateCentre(1, refCentre + Vector3d<float>(0, 4*sin(0.4*cnt), 0));
		}
		if (key == 'u')
		{
			builder.refreshThis(manyS);
		}
		if (key == 'n')
		{
			builder.printPlan();
			builder.buildInto(manyS);
		}
		++cnt;
	}
}
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

#define REPORT_EXCEPTION(x) std::cout << x << "\n";
int main(void)
{
	//mainForInterpolator();

	//SceneryDisplayUnit du("localhost:8765");
	SceneryDisplayUnit du("192.168.3.105:8765");

	try
	{

	char key = 0;
	while (key != 27)
	{
		std::cout << "=============== NEW MODEL ===============\n";
		mainForLinkedSpheres(du);
		//mainForSimplyLinkedSpheres(du);
		std::cin >> key;
	}

	}
	catch (const char* e)
	{
		REPORT_EXCEPTION("Got this message: " << e)
	}
	catch (std::string& e)
	{
		REPORT_EXCEPTION("Got this message: " << e)
	}
	catch (std::runtime_error* e)
	{
		REPORT_EXCEPTION("RuntimeError: " << e->what())
	}
	catch (i3d::IOException* e)
	{
		REPORT_EXCEPTION("i3d::IOException: " << e->what)
	}
	catch (i3d::LibException* e)
	{
		REPORT_EXCEPTION("i3d::LibException: " << e->what)
	}
	catch (std::bad_alloc&)
	{
		REPORT_EXCEPTION("Not enough memory.")
	}
	catch (...)
	{
		REPORT_EXCEPTION("System exception.")
	}

	return 0;
}
