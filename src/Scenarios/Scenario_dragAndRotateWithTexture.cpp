#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/util/SpheresFunctions.h"
#include "../Simulation.h"
#include "../Agents/Nucleus4SAgent.h"
#include "../Agents/util/Texture.h"
#include "common/Scenarios.h"

class myDragAndTextureNucleus: public Nucleus4SAgent, TextureQuantized, TextureUpdater4S
{
public:
	myDragAndTextureNucleus(const int _ID, const std::string& _type,
	          const Spheres& shape,
	          const float _currTime, const float _incrTime):
		Nucleus4SAgent(_ID,_type, shape, _currTime,_incrTime),
		TextureQuantized(60000, Vector3d<float>(2.0f,2.0f,2.0f), 8), //does not tear the texture in phantoms  (5x slower)
		//Texture(60000), //tears the texture a bit in phantom images but it is not apparent in finalPreviews (5x faster)
		TextureUpdater4S(shape)
	{
		cytoplasmWidth = 0.0f;

		//texture img: resolution -- makes sense to match it with the phantom img resolution
		createPerlinTexture(futureGeometry, Vector3d<float>(2.0f),
		                    5.0,8,4,6,    //Perlin
		                    1.0f,         //texture intensity range centre
		                    0.1f, true);  //quantization and shouldCollectOutlyingDots
	}

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

		//update only just before the texture rendering event... (to save some comp. time)
		if (Officer->willRenderNextFrame())
		{
			updateTextureCoords(dots, futureGeometry);

			//correct for outlying texture particles
#ifndef DEBUG
			collectOutlyingDots(futureGeometry);
#else
			const int dotOutliers = collectOutlyingDots(futureGeometry);
			DEBUG_REPORT(dotOutliers << " (" << (float)(100*dotOutliers)/(float)dots.size()
							 << " %) dots had to be moved inside the initial geometry");
#endif
		}
	}


	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>&) override
	{
		if (Officer->isProducingOutput(phantom)) renderIntoPhantom(phantom);
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

void Scenario_dragRotateAndTexture::initializeScenario(void)
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
