#include "../DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "../DisplayUnits/FlightRecorderDisplayUnit.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/util/SpheresFunctions.h"
#include "../Agents/NucleusNSAgent.h"
#include "../Agents/util/Texture.h"
#include "../Agents/util/TextureFunctions.h"
#include "common/Scenarios.h"

class myDragAndTextureNucleus_common: public NucleusNSAgent, Texture
{
public:
	myDragAndTextureNucleus_common(const int _ID, const std::string& _type,
	          const Spheres& shape,
	          const float _currTime, const float _incrTime):
		NucleusNSAgent(_ID,_type, shape, _currTime,_incrTime),
		//TextureQuantized(60000, Vector3d<float>(2.0f,2.0f,2.0f), 8) //does not tear the texture in phantoms  (5x slower)
		Texture(20000) //tears the texture a bit in phantom images but it is not apparent in finalPreviews (5x faster)
	{
		cytoplasmWidth = 0.0f;

		/*
		for (int i=2; i < shape.getNoOfSpheres(); ++i)
		{
			TextureFunctions::addTextureAlongLine(dots, futureGeometry.getCentres()[0],futureGeometry.getCentres()[i],2000);
			TextureFunctions::addTextureAlongLine(dots, futureGeometry.getCentres()[1],futureGeometry.getCentres()[i],2000);
		}
		*/
		TextureFunctions::addTextureAlongGrid(dots, geometryAlias,
		         Vector3d<G_FLOAT>(0),Vector3d<G_FLOAT>(3),
		         Vector3d<G_FLOAT>(0.5f));
	}

	void advanceAndBuildIntForces(const float futureGlobalTime) override
	{
		//add own forces (mutually opposite) to "head&tail" spheres (to spin it)
		/*
		Vector3d<G_FLOAT> rotationVelocity;
		rotationVelocity.y = 1.5f* std::cos(currTime/30.f * 6.28f);
		rotationVelocity.z = 1.5f* std::sin(currTime/30.f * 6.28f);
		*/

		/*
		const int radialPos = (ID-1) % 3;
		const G_FLOAT velocity = (float)radialPos*0.4f + 0.6f; - rings of different velocities
		*/
		const G_FLOAT velocity = (float)1.0f;
		//REPORT("ID=" << ID << " radialPos=" << radialPos << ", velocity=" << velocity);
		Vector3d<G_FLOAT> travellingVelocity(0,velocity,0);
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
		NucleusNSAgent::advanceAndBuildIntForces(futureGlobalTime);
	}

	void adjustGeometryByExtForces(void) override
	{
		NucleusNSAgent::adjustGeometryByExtForces();

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

	virtual void updateTextureCoords(std::vector<Dot>&, const Spheres&)
	{ REPORT("don't you dare to call me!"); }

	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>&) override
	{
		renderIntoPhantom(phantom);
	}

	void drawMask(i3d::Image3d<i3d::GRAY16>& img) override
	{
		//shortcuts to the mask image parameters
		const Vector3d<G_FLOAT> res(img.GetResolution().GetRes());
		const Vector3d<G_FLOAT> off(img.GetOffset());

		//shortcuts to our Own spheres
		const Vector3d<G_FLOAT>* const centresO = futureGeometry.getCentres();
		const G_FLOAT* const radiiO             = futureGeometry.getRadii();
		const int iO                          = futureGeometry.getNoOfSpheres();

		//project and "clip" this AABB into the img frame
		//so that voxels to sweep can be narrowed down...
		//
		//   sweeping position and boundaries (relevant to the 'img')
		Vector3d<size_t> curPos, minSweepPX,maxSweepPX;
		futureGeometry.AABB.exportInPixelCoords(img, minSweepPX,maxSweepPX);
		//
		//micron coordinate of the running voxel 'curPos'
		Vector3d<G_FLOAT> centre;

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
class myDragAndTextureNucleus_NStexture: public myDragAndTextureNucleus_common, TextureUpdaterNS
{
public:
	myDragAndTextureNucleus_NStexture(const int _ID, const std::string& _type,
	          const Spheres& shape,
	          const float _currTime, const float _incrTime,
	          const int maxNeighs=1):
		myDragAndTextureNucleus_common(_ID,_type,shape,_currTime,_incrTime),
		TextureUpdaterNS(shape,maxNeighs)
	{}

	void updateTextureCoords(std::vector<Dot>& dots, const Spheres& newGeom) override
	{ this->TextureUpdaterNS::updateTextureCoords(dots,newGeom); }

	void drawMask(DisplayUnit& du) override
	{
		//show the usual stuff....
		NucleusNSAgent::drawMask(du);

		//and also the local orientations
		Vector3d<float> orientVec;
		int gdID = DisplayUnit::firstIdForSceneDebugObjects() + ID*40 +15000;
		for (int i=0; i < futureGeometry.getNoOfSpheres(); ++i)
		{
			getLocalOrientation(futureGeometry,i,orientVec);
			du.DrawVector(gdID++, futureGeometry.getCentres()[i],orientVec,0);
		}
	}
};


class myDragAndTextureNucleus_2pNStexture: public myDragAndTextureNucleus_common, TextureUpdater2pNS
{
public:
	myDragAndTextureNucleus_2pNStexture(const int _ID, const std::string& _type,
	          const Spheres& shape,
	          const float _currTime, const float _incrTime):
		myDragAndTextureNucleus_common(_ID,_type,shape,_currTime,_incrTime),
		TextureUpdater2pNS(shape,0,1)
	{}

	void updateTextureCoords(std::vector<Dot>& dots, const Spheres& newGeom) override
	{ this->TextureUpdater2pNS::updateTextureCoords(dots,newGeom); }

	void drawMask(DisplayUnit& du) override
	{
		//show the usual stuff....
		NucleusNSAgent::drawMask(du);

		//and also the local orientations
		Vector3d<float> orientVec = futureGeometry.getCentres()[sphereOnMainAxis];
		orientVec -= futureGeometry.getCentres()[sphereAtCentre];
		orientVec.changeToUnitOrZero();
		int gdID = DisplayUnit::firstIdForSceneDebugObjects() + ID*40 +15000;
		for (int i=0; i < futureGeometry.getNoOfSpheres(); ++i)
		{
			du.DrawVector(gdID++, futureGeometry.getCentres()[i],orientVec,0);
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

		fo->startNewAgent(
			new myDragAndTextureNucleus_NStexture(fo->getNextAvailAgentID(),"nucleus travelling NS (3) texture",
			                                      s, params.constants.initTime,params.constants.incrTime,3)
		);

		const Vector3d<float> shiftVec(14,0,0);
		for (int i = 0; i < s.getNoOfSpheres(); ++i)
			s.updateCentre(i, s.getCentres()[i] + shiftVec);

		fo->startNewAgent(
			new myDragAndTextureNucleus_NStexture(fo->getNextAvailAgentID(),"nucleus travelling NS texture",
			                                      s, params.constants.initTime,params.constants.incrTime)
		);

		for (int i = 0; i < s.getNoOfSpheres(); ++i)
			s.updateCentre(i, s.getCentres()[i] + shiftVec);

		fo->startNewAgent(
			new myDragAndTextureNucleus_2pNStexture(fo->getNextAvailAgentID(),"nucleus travelling 2pNS texture",
			                                        s, params.constants.initTime,params.constants.incrTime)
		);
	}
}


void Scenario_dragRotateAndTexture::initializeScene()
{
	//override the output images
	params.setOutputImgSpecs( Vector3d<float>(220,30,30), Vector3d<float>(70,160,160));

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
