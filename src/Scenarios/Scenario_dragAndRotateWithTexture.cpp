#include "../DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "../DisplayUnits/FlightRecorderDisplayUnit.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/util/SpheresFunctions.h"
#include "../Agents/Nucleus2pNSAgent.h"
#include "../Agents/util/Texture.h"
#include "common/Scenarios.h"

class myDragAndTextureNucleus: public Nucleus2pNSAgent, Texture, TextureUpdater2pNS
{
public:
	myDragAndTextureNucleus(const int _ID, const std::string& _type,
	          const Spheres& shape, const Vector3d<FLOAT>& _basalPlaneNormal,
	          const float _currTime, const float _incrTime):
		Nucleus2pNSAgent(_ID,_type, shape,_basalPlaneNormal, _currTime,_incrTime),
		//TextureQuantized(60000, Vector3d<float>(2.0f,2.0f,2.0f), 8), //does not tear the texture in phantoms  (5x slower)
		Texture(10000), //tears the texture a bit in phantom images but it is not apparent in finalPreviews (5x faster)
		TextureUpdater2pNS(shape)
	{
		cytoplasmWidth = 0.0f;

		/*
		//texture img: resolution -- makes sense to match it with the phantom img resolution
		createPerlinTexture(futureGeometry, Vector3d<float>(2.0f),
		                    5.0,8,4,6,    //Perlin
		                    1.0f,         //texture intensity range centre
		                    0.1f, true);  //quantization and shouldCollectOutlyingDots
		*/
		for (int i=2; i < noOfSpheres; ++i)
		{
			addTextureAlongLine(futureGeometry.getCentres()[0],futureGeometry.getCentres()[i],2000);
			addTextureAlongLine(futureGeometry.getCentres()[1],futureGeometry.getCentres()[i],2000);
		}
	}

	void addTextureAlongLine(const Vector3d<float>& from, const Vector3d<float>& to,
	                         const size_t noOfParticles = 5000)
	{
		Vector3d<float> delta = to - from;
		delta /= (float)noOfParticles;

		for (size_t i=0; i < noOfParticles; ++i)
		{
			dots.emplace_back(from + (float)i*delta);
		}
	}

	void advanceAndBuildIntForces(const float futureGlobalTime) override
	{
		//add own forces (mutually opposite) to "head&tail" spheres (to spin it)
		/*
		Vector3d<FLOAT> rotationVelocity;
		rotationVelocity.y = 1.5f* std::cos(currTime/30.f * 6.28f);
		rotationVelocity.z = 1.5f* std::sin(currTime/30.f * 6.28f);
		*/

		Vector3d<FLOAT> travellingVelocity(0,1.0f,0);
		exertForceOnSphere(1,
			(weights[0]/velocity_PersistenceTime) * travellingVelocity,
			ftype_drive );

		/*
		rotationVelocity *= -1;
		forces.emplace_back(
			(weights[3]/velocity_PersistenceTime) * rotationVelocity,
			futureGeometry.getCentres()[3],3, ftype_drive );
		*/

		//define own velocity
		/*
		velocity_CurrentlyDesired.y = 1.0f* std::cos(currTime/10.f * 6.28f);
		velocity_CurrentlyDesired.z = 1.0f* std::sin(currTime/10.f * 6.28f);
		*/

		//call the original method... takes care of own velocity, spheres mis-alignments, etc.
		Nucleus2pNSAgent::advanceAndBuildIntForces(futureGlobalTime);
	}

	void adjustGeometryByExtForces(void) override
	{
		Nucleus2pNSAgent::adjustGeometryByExtForces();

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
							 << " %) dots had to be moved inside the current geometry");
#endif
		}
	}


	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>&) override
	{
		renderIntoPhantom(phantom);
	}

	void drawMask(i3d::Image3d<i3d::GRAY16>& img) override
	{
		//shortcuts to the mask image parameters
		const Vector3d<FLOAT> res(img.GetResolution().GetRes());
		const Vector3d<FLOAT> off(img.GetOffset());

		//shortcuts to our Own spheres
		const Vector3d<FLOAT>* const centresO = futureGeometry.getCentres();
		const FLOAT* const radiiO             = futureGeometry.getRadii();
		const int iO                          = futureGeometry.getNoOfSpheres();

		//project and "clip" this AABB into the img frame
		//so that voxels to sweep can be narrowed down...
		//
		//   sweeping position and boundaries (relevant to the 'img')
		Vector3d<size_t> curPos, minSweepPX,maxSweepPX;
		futureGeometry.AABB.exportInPixelCoords(img, minSweepPX,maxSweepPX);
		//
		//micron coordinate of the running voxel 'curPos'
		Vector3d<FLOAT> centre;

		//sweep and check intersection with spheres' volumes
		for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
			for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
				for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++)
				{
					//get micron coordinate of the current voxel's centre
					centre.toMicronsFrom(curPos, res,off);

					//check the current voxel against all spheres
					for (int i = 0; i < iO; ++i)
					{
						if ((centre-centresO[i]).len() <= radiiO[i])
						{
							img.SetVoxel(curPos.x,curPos.y,curPos.z, (i3d::GRAY16)(ID*100 + i));
						}
					}
				}
	}

};


//==========================================================================
void Scenario_dragRotateAndTexture::initializeAgents(FrontOfficer* fo,int p,int)
{
	if (p != 1)
	{
		REPORT("Populating only the first FO (which is not this one).");
		return;
	}

	const int howManyToPlace = argc > 2? atoi(argv[2]) : 12;
	DEBUG_REPORT("will rotate " << howManyToPlace << " nuclei...");

	const Vector3d<float> basalPlaneNormal(-1,0,0);

	for (int i=0; i < howManyToPlace; ++i)
	{
		const float ang = float(i)/float(howManyToPlace);
		const float radius = 40.0f;

		//the wished position relative to [0,0,0] centre
		Vector3d<float> axis(0,std::cos(ang*6.28f),std::sin(ang*6.28f));
		Vector3d<float> pos(0,radius * axis.y,radius * axis.z);

		//position is shifted to the scene centre
		pos.x += params.constants.sceneSize.x/2.0f;
		pos.y += params.constants.sceneSize.y/2.0f;
		pos.z += params.constants.sceneSize.z/2.0f;

		//position is shifted due to scene offset
		pos.x += params.constants.sceneOffset.x;
		pos.y += params.constants.sceneOffset.y;
		pos.z += params.constants.sceneOffset.z;

		Spheres s(6);
		//central axis
		s.updateCentre(0,pos);
		s.updateRadius(0,5.0f);
		s.updateCentre(1,pos +8.0f*axis);
		s.updateRadius(1,5.0f);

		//satellites
		s.updateCentre(2,pos +Vector3d<float>(0,+5,0) +6.0f*axis);
		s.updateRadius(2,3.0f);
		s.updateCentre(3,pos +Vector3d<float>(0,0,+5) +6.0f*axis);
		s.updateRadius(3,3.0f);
		s.updateCentre(4,pos +Vector3d<float>(0,-5,0) +6.0f*axis);
		s.updateRadius(4,3.0f);
		s.updateCentre(5,pos +Vector3d<float>(0,0,-5) +6.0f*axis);
		s.updateRadius(5,3.0f);

		myDragAndTextureNucleus* ag = new myDragAndTextureNucleus(fo->getNextAvailAgentID(),"nucleus travelling texture",
		                                                          s,basalPlaneNormal,
		                                                          params.constants.initTime,params.constants.incrTime);
		ag->setDetailedDrawingMode(true);
		fo->startNewAgent(ag);
	}
}


void Scenario_dragRotateAndTexture::initializeScene()
{
	//override the output images
	params.setOutputImgSpecs( Vector3d<float>(220,30,30), Vector3d<float>(40,160,160));

	disks.enableImgMaskTIFFs();
	disks.enableImgPhantomTIFFs();
	disks.enableImgOpticsTIFFs();

	displays.registerDisplayUnit( [](){ return new SceneryBufferedDisplayUnit("localhost:8765"); } );
	displays.registerDisplayUnit( [](){ return new FlightRecorderDisplayUnit("/temp/FR_dragRotateAndTexture.txt"); } );

	//displays.registerImagingUnit("localFiji","localhost:54545");
	//displays.enableImgPhantomInImagingUnit("localFiji");
}


SceneControls& Scenario_dragRotateAndTexture::provideSceneControls()
{
	SceneControls::Constants myConstants;

	//override the default stop time
	myConstants.stopTime = 40.2f;

	return *(new SceneControls(myConstants));
}
