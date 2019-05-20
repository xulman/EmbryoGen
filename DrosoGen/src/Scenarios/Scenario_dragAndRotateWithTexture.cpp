#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/util/SpheresFunctions.h"
#include "../Geometries/util/InnerAxes.h"            //indication points
#include <vector>                                    //indication points
#include "../Simulation.h"
#include "../Agents/Nucleus4SAgent.h"
#include "../Agents/util/Texture.h"
#include "../util/texture/texture.h"
#include "Scenarios.h"

class myDragAndTextureNucleus: public Nucleus4SAgent, TextureQuantized
{
public:
	myDragAndTextureNucleus(const int _ID, const std::string& _type,
	          const Spheres& shape,
	          const float _currTime, const float _incrTime):
		Nucleus4SAgent(_ID,_type, shape, _currTime,_incrTime),
		TextureQuantized(60000, Vector3d<float>(2.0f,2.0f,2.0f), 125)
	{
		cytoplasmWidth = 0.0f;

		//texture img: resolution -- makes sense to match it with the phantom img resolution
		i3d::Image3d<float> img;
		SetupImageForRasterizingTexture(img,Vector3d<float>(2.0f), futureGeometry);

		DoPerlin3D(img,5.0);
		img.GetVoxelData() += 0.6f;
		SampleDotsFromImage(img,futureGeometry,0.1f);

		const int dotOutliers = CollectOutlyingDots(futureGeometry);
		DEBUG_REPORT(dotOutliers << " (" << 100.f*dotOutliers/dots.size()
		             << " %) dots had to be moved inside the initial geometry");

		//img.GetVoxelData() = 0;
		//RenderIntoPhantom(img);
		//img.SaveImage("test.tif");

		//init the dotsCoordUpdater
		dotsCoordUpdater.reserve(4);
		dotsCoordUpdater.emplace_back(shape,0,shape.getCentres()[0] - shape.getCentres()[1]);
		dotsCoordUpdater.emplace_back(shape,1,shape.getCentres()[0] - shape.getCentres()[2]);
		dotsCoordUpdater.emplace_back(shape,2,shape.getCentres()[1] - shape.getCentres()[3]);
		dotsCoordUpdater.emplace_back(shape,3,shape.getCentres()[2] - shape.getCentres()[3]);

		//setup indication points
		InnerAxes<FLOAT> ia;
		ia.setFromSkeletonAxis( futureGeometry.getCentres()[0] - futureGeometry.getCentres()[1] );
		indicationPoints[0] = futureGeometry.getCentres()[0] + ia.mA;
		indicationPoints[1] = futureGeometry.getCentres()[0] + ia.sA;
		indicationPoints[2] = futureGeometry.getCentres()[0] + ia.tA;

		ia.setFromSkeletonAxis( futureGeometry.getCentres()[0] - futureGeometry.getCentres()[2] );
		indicationPoints[3] = futureGeometry.getCentres()[1] + ia.mA;
		indicationPoints[4] = futureGeometry.getCentres()[1] + ia.sA;
		indicationPoints[5] = futureGeometry.getCentres()[1] + ia.tA;

		ia.setFromSkeletonAxis( futureGeometry.getCentres()[1] - futureGeometry.getCentres()[3] );
		indicationPoints[6] = futureGeometry.getCentres()[2] + ia.mA;
		indicationPoints[7] = futureGeometry.getCentres()[2] + ia.sA;
		indicationPoints[8] = futureGeometry.getCentres()[2] + ia.tA;

		ia.setFromSkeletonAxis( futureGeometry.getCentres()[2] - futureGeometry.getCentres()[3] );
		indicationPoints[9]  = futureGeometry.getCentres()[3] + ia.mA;
		indicationPoints[10] = futureGeometry.getCentres()[3] + ia.sA;
		indicationPoints[11] = futureGeometry.getCentres()[3] + ia.tA;
	}

	std::vector< SpheresFunctions::CoordsUpdater<FLOAT> > dotsCoordUpdater;
	Coord3d<FLOAT> indicationPoints[12];

	void advanceAndBuildIntForces(const float) override
	{
		//add own forces (mutually opposite) to "head&tail" spheres (to spin it)
		Vector3d<FLOAT> rotationVelocity;
		rotationVelocity.y = 1.5f* std::cos(currTime/30.f * 6.28f);
		rotationVelocity.z = 1.5f* std::sin(currTime/30.f * 6.28f);

		forces.emplace_back(
			(weights[0]/velocity_PersistenceTime) * rotationVelocity,
			futureGeometry.getCentres()[0],0, ftype_drive );

		rotationVelocity *= -1;
		forces.emplace_back(
			(weights[3]/velocity_PersistenceTime) * rotationVelocity,
			futureGeometry.getCentres()[3],3, ftype_drive );
/*
		//define own velocity
		velocity_CurrentlyDesired.x = 1.0f* std::cos(currTime/10.f * 6.28f);
		velocity_CurrentlyDesired.z = 1.0f* std::sin(currTime/10.f * 6.28f);
*/
		//call the original method... takes care of own velocity, spheres mis-alignments, etc.
		Nucleus4SAgent::advanceAndBuildIntForces(0.f);
	}

	void adjustGeometryByExtForces(void) override
	{
		Nucleus4SAgent::adjustGeometryByExtForces();
		adjustTextureAfterGeometryChange();
	}

	int counter = 0;
	void adjustTextureAfterGeometryChange()
	{
		//NB: this->velocities[]*this->incrTime contains the most recent displacement
		//NB: there's even still geometryAlias (old state) and futureGeometry (new state)

		//update only just before the texture rendering event... (to save some comp. time)
		++counter;
		if (counter == 5)
		{
			counter = 0;

			//prepare the updating routines...
			dotsCoordUpdater[0].prepareUpdating( futureGeometry,0,
			      futureGeometry.getCentres()[0] - futureGeometry.getCentres()[1] );

			dotsCoordUpdater[1].prepareUpdating( futureGeometry,1,
		         futureGeometry.getCentres()[0] - futureGeometry.getCentres()[2] );

			dotsCoordUpdater[2].prepareUpdating( futureGeometry,2,
		         futureGeometry.getCentres()[1] - futureGeometry.getCentres()[3] );

			dotsCoordUpdater[3].prepareUpdating( futureGeometry,3,
		         futureGeometry.getCentres()[2] - futureGeometry.getCentres()[3] );

			//now update the "indication points" one by one
			dotsCoordUpdater[0].updateCoord(indicationPoints[0]);
			dotsCoordUpdater[0].updateCoord(indicationPoints[1]);
			dotsCoordUpdater[0].updateCoord(indicationPoints[2]);

			dotsCoordUpdater[1].updateCoord(indicationPoints[3]);
			dotsCoordUpdater[1].updateCoord(indicationPoints[4]);
			dotsCoordUpdater[1].updateCoord(indicationPoints[5]);

			dotsCoordUpdater[2].updateCoord(indicationPoints[6]);
			dotsCoordUpdater[2].updateCoord(indicationPoints[7]);
			dotsCoordUpdater[2].updateCoord(indicationPoints[8]);

			dotsCoordUpdater[3].updateCoord(indicationPoints[ 9]);
			dotsCoordUpdater[3].updateCoord(indicationPoints[10]);
			dotsCoordUpdater[3].updateCoord(indicationPoints[11]);
		}
	}


	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>&) override
	{
		if (Officer->isProducingOutput(phantom))
		{
			RenderIntoPhantom(phantom);
			DEBUG_REPORT(ID << " finished rendering into the phantom");
		}
	}

	void drawMask(DisplayUnit& du) override
	{
		int dID = ID << 17;

		//draw spheres
		for (int i=0; i < futureGeometry.getNoOfSpheres(); ++i)
			du.DrawPoint(dID++,futureGeometry.getCentres()[i],futureGeometry.getRadii()[i],5);

		//draw skeletons
		for (int i=1; i < futureGeometry.getNoOfSpheres(); ++i)
			du.DrawLine(dID++,futureGeometry.getCentres()[i-1],futureGeometry.getCentres()[i],2);

		//draw indication points as lines from the respective sphere's centre
		du.DrawVector(dID++, futureGeometry.getCentres()[0], indicationPoints[0] -futureGeometry.getCentres()[0], 4);
		du.DrawVector(dID++, futureGeometry.getCentres()[0], indicationPoints[1] -futureGeometry.getCentres()[0], 5);
		du.DrawVector(dID++, futureGeometry.getCentres()[0], indicationPoints[2] -futureGeometry.getCentres()[0], 6);

		du.DrawVector(dID++, futureGeometry.getCentres()[1], indicationPoints[3] -futureGeometry.getCentres()[1], 4);
		du.DrawVector(dID++, futureGeometry.getCentres()[1], indicationPoints[4] -futureGeometry.getCentres()[1], 5);
		du.DrawVector(dID++, futureGeometry.getCentres()[1], indicationPoints[5] -futureGeometry.getCentres()[1], 6);

		du.DrawVector(dID++, futureGeometry.getCentres()[2], indicationPoints[6] -futureGeometry.getCentres()[2], 4);
		du.DrawVector(dID++, futureGeometry.getCentres()[2], indicationPoints[7] -futureGeometry.getCentres()[2], 5);
		du.DrawVector(dID++, futureGeometry.getCentres()[2], indicationPoints[8] -futureGeometry.getCentres()[2], 6);

		du.DrawVector(dID++, futureGeometry.getCentres()[3], indicationPoints[ 9]-futureGeometry.getCentres()[3], 4);
		du.DrawVector(dID++, futureGeometry.getCentres()[3], indicationPoints[10]-futureGeometry.getCentres()[3], 5);
		du.DrawVector(dID++, futureGeometry.getCentres()[3], indicationPoints[11]-futureGeometry.getCentres()[3], 6);
	}
};

void Scenario_dragRotateAndTexture::initializeAgents(void)
{
	//to obtain a sequence of IDs for new agents...
	int ID=1;

	const int howManyToPlace = argc > 2? atoi(argv[2]) : 12;
	DEBUG_REPORT("will rotate " << howManyToPlace << " nuclei...");

	for (int i=0; i < howManyToPlace; ++i)
	{
		const float ang = float(i)/float(howManyToPlace);
		const float radius = 40.0f;

		//the wished position relative to [0,0,0] centre
		Vector3d<float> axis(0,std::cos(ang*6.28f),std::sin(ang*6.28f));
		Vector3d<float> pos(0,radius * axis.y,radius * axis.z);

		//position is shifted to the scene centre
		pos.x += sceneSize.x/2.0f;
		pos.y += sceneSize.y/2.0f;
		pos.z += sceneSize.z/2.0f;

		//position is shifted due to scene offset
		pos.x += sceneOffset.x;
		pos.y += sceneOffset.y;
		pos.z += sceneOffset.z;

		Spheres s(4);
		s.updateCentre(0,pos);
		s.updateRadius(0,3.0f);
		s.updateCentre(1,pos +6.0f*axis);
		s.updateRadius(1,5.0f);
		s.updateCentre(2,pos +12.0f*axis);
		s.updateRadius(2,5.0f);
		s.updateCentre(3,pos +18.0f*axis);
		s.updateRadius(3,3.0f);

		myDragAndTextureNucleus* ag = new myDragAndTextureNucleus(ID++,"nucleus travelling texture",s,currTime,incrTime);
		ag->setDetailedDrawingMode(true);
		startNewAgent(ag);
	}

	//override the output images
	setOutputImgSpecs( Vector3d<float>(220,30,30), Vector3d<float>(40,160,160));
	//enableProducingOutput( imgMask );
	//enableProducingOutput( imgPhantom );

	//override the default stop time
	stopTime = 40.2f;
}
