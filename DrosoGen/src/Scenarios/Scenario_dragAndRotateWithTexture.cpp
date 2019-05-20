#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/util/SpheresFunctions.h"
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
	}

	std::vector< SpheresFunctions::CoordsUpdater<FLOAT> > dotsCoordUpdater;

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

		//define own velocity
		velocity_CurrentlyDesired.x = 1.0f* std::cos(currTime/10.f * 6.28f);
		velocity_CurrentlyDesired.z = 1.0f* std::sin(currTime/10.f * 6.28f);

		//call the original method... takes care of own velocity, spheres mis-alignments, etc.
		Nucleus4SAgent::advanceAndBuildIntForces(0.f);
	}

	void adjustGeometryByExtForces(void) override
	{
		Nucleus4SAgent::adjustGeometryByExtForces();
		adjustTextureAfterGeometryChange();
	}

	void adjustTextureAfterGeometryChange()
	{
		//NB: this->velocities[]*this->incrTime contains the most recent displacement
		//NB: there's even still geometryAlias (old state) and futureGeometry (new state)

		//update only just before the texture rendering event... (to save some comp. time)
		if (Officer->willRenderNextFrame())
		{
			//backup the geometry for which the texture dots are valid
			Vector3d<FLOAT> prevCentre[4];
			FLOAT           prevRadius[4];
			for (unsigned int i=0; i < 4; ++i)
			{
				prevCentre[i] = dotsCoordUpdater[i].prevCentre;
				prevRadius[i] = dotsCoordUpdater[i].prevRadius;
			}

			//prepare the updating routines...
			dotsCoordUpdater[0].prepareUpdating( futureGeometry,0,
			      futureGeometry.getCentres()[0] - futureGeometry.getCentres()[1] );

			dotsCoordUpdater[1].prepareUpdating( futureGeometry,1,
		         futureGeometry.getCentres()[0] - futureGeometry.getCentres()[2] );

			dotsCoordUpdater[2].prepareUpdating( futureGeometry,2,
		         futureGeometry.getCentres()[1] - futureGeometry.getCentres()[3] );

			dotsCoordUpdater[3].prepareUpdating( futureGeometry,3,
		         futureGeometry.getCentres()[2] - futureGeometry.getCentres()[3] );


			//aux variables
			float weights[4];             //4 because we're Nucleus4SAgent
			float sum;
			Vector3d<float> tmp,newPos;

#ifdef DEBUG
			int outsideDots = 0;
#endif

			//shift texture particles
			for (auto& dot : dots)
			{
				//determine the weights
				for (unsigned int i=0; i < 4; ++i)
				{
					tmp  = dot.pos;
					tmp -= prevCentre[i];
					weights[i] = std::max(prevRadius[i] - tmp.len(), (FLOAT)0);
				}

				//normalization factor
				sum = weights[0] + weights[1] + weights[2] + weights[3];

				if (sum > 0)
				{
					//apply the weights
					newPos = 0;
					for (unsigned int i=0; i < 4; ++i)
					if (weights[i] > 0)
					{
						tmp = dot.pos;
						dotsCoordUpdater[i].updateCoord(tmp);
						newPos += (weights[i]/sum) * tmp;
					}
					dot.pos = newPos;
				}
				else
				{
#ifdef DEBUG
					++outsideDots;
#endif
				}
			}

#ifdef DEBUG
			if (outsideDots > 0)
				REPORT(outsideDots << " could not be updated (no matching sphere found, weird...)");
#endif

			//correct for outlying texture particles
			const int dotOutliers = CollectOutlyingDots(futureGeometry);
			DEBUG_REPORT(dotOutliers << " (" << 100.f*dotOutliers/dots.size()
							 << " %) dots had to be moved inside the initial geometry");
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
	enableProducingOutput( imgMask );
	enableProducingOutput( imgPhantom );

	//override the default stop time
	stopTime = 40.2f;
}
