#include "../DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "common/Scenarios.h"
#include "../Agents/NucleusNSAgent.h"
#include "../Agents/util/Texture.h"
#include "../util/texture/texture.h"
#include "../util/rnd_generators.h"

class TetrisNucleus: public NucleusNSAgent //, TextureUpdaterNS
{
public:
	TetrisNucleus(const int _ID, const std::string& _type,
	              const Spheres& shape,
	              const float _currTime, const float _incrTime):
		NucleusNSAgent(_ID,_type, shape, _currTime,_incrTime)
		//Texture(60000)
		//TextureQuantized(60000, Vector3d<float>(2.0f,2.0f,2.0f), 8)
	{
		//texture img: resolution -- makes sense to match it with the phantom img resolution
		/*
		createPerlinTexture(futureGeometry, Vector3d<float>(2.0f),
		                    5.0,8,4,6,    //Perlin
		                    1.0f,         //texture intensity range centre
		                    0.1f, true);  //quantization and shouldCollectOutlyingDots
		*/
	}

	/*
	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>&) override
	{
		renderIntoPhantom(phantom);
	}
	*/
};


//==========================================================================
void Scenario_Tetris::initializeAgents(FrontOfficer* fo,int p,int)
{
	if (p != 1)
	{
		REPORT("Populating only the first FO (which is not this one).");
		return;
	}


	//shapes: 2x2 box, 1x4 line, big-T, random bulk
	//arranged in a 4x4 grid (that is, boundaries of 5x5 blocks)
	const Vector3d<float> gridStart( Vector3d<float>(params.constants.sceneSize).elemMathOp( [](float e){ return e/5; } ) );
	const Vector3d<float> gridStep(gridStart);
	Vector3d<float> currGridPos;

	Spheres spheres(7);
	std::string agentName;

	const float sDist = 4;
	const float sRadius = 3;

	for (int x = 0; x < 4; ++x)
	for (int y = 0; y < 4; ++y)
	{
		currGridPos.fromScalars(x,y,2).elemMult(gridStep);
		currGridPos += gridStart;

		switch (x) {
		case 0:
			agentName = buildStringFromStream("" << y << "__Box");
			spheres.updateCentre(0,currGridPos);
			spheres.updateCentre(1,currGridPos + Vector3d<float>(-sDist,-sDist/2.0f,0));
			spheres.updateCentre(2,currGridPos + Vector3d<float>(-sDist,+sDist/2.0f,0));
			spheres.updateCentre(3,currGridPos + Vector3d<float>(     0,-sDist/1.5f,0));
			spheres.updateCentre(4,currGridPos + Vector3d<float>(     0,+sDist/1.5f,0));
			spheres.updateCentre(5,currGridPos + Vector3d<float>(+sDist,-sDist/2.0f,0));
			spheres.updateCentre(6,currGridPos + Vector3d<float>(+sDist,+sDist/2.0f,0));
			break;
		case 1:
			agentName = buildStringFromStream("" << y << "__Line");
			spheres.updateCentre(0,currGridPos);
			spheres.updateCentre(1,currGridPos + Vector3d<float>(-sDist*3,-sDist*0.9f,0));
			spheres.updateCentre(2,currGridPos + Vector3d<float>(-sDist*2,-sDist*0.6f,0));
			spheres.updateCentre(3,currGridPos + Vector3d<float>(-sDist*1,-sDist*0.3f,0));
			spheres.updateCentre(4,currGridPos + Vector3d<float>(+sDist*1,+sDist*0.3f,0));
			spheres.updateCentre(5,currGridPos + Vector3d<float>(+sDist*2,+sDist*0.6f,0));
			spheres.updateCentre(6,currGridPos + Vector3d<float>(+sDist*3,+sDist*0.9f,0));
			break;
		case 2:
			agentName = buildStringFromStream("" << y << "__BigT");
			spheres.updateCentre(0,currGridPos);
			spheres.updateCentre(1,currGridPos + Vector3d<float>(-sDist*2,       0,0));
			spheres.updateCentre(2,currGridPos + Vector3d<float>(-sDist*1,       0,0));
			spheres.updateCentre(3,currGridPos + Vector3d<float>(       0,-sDist*1,0));
			spheres.updateCentre(4,currGridPos + Vector3d<float>(       0,-sDist*2,0));
			spheres.updateCentre(5,currGridPos + Vector3d<float>(+sDist*1,       0,0));
			spheres.updateCentre(6,currGridPos + Vector3d<float>(+sDist*2,       0,0));
			break;
		case 3:
			agentName = buildStringFromStream("" << y << "__Bulk");
			for (int i=0; i < spheres.getNoOfSpheres(); ++i)
			{
				spheres.updateCentre(i, currGridPos + Vector3d<float>(
					GetRandomUniform(-0.8f*sDist,+0.8f*sDist),
					GetRandomUniform(-0.8f*sDist,+0.8f*sDist),
					GetRandomUniform(-0.8f*sDist,+0.8f*sDist)
				));
			}
			break;
		default:
			REPORT("WARNING: no agents at grid pos " << y << "," << x);
		}

		for (int i=0; i < spheres.getNoOfSpheres(); ++i)
			spheres.updateRadius(i,sRadius + GetRandomUniform(-0.8f,+0.8f));

		fo->startNewAgent( new TetrisNucleus(
				fo->getNextAvailAgentID(),
				agentName,spheres,
				params.constants.initTime,params.constants.incrTime
			) );
	}
}


void Scenario_Tetris::initializeScene()
{
	displays.registerDisplayUnit( [](){ return new SceneryBufferedDisplayUnit("localhost:8765"); } );
	//displays.registerDisplayUnit( [](){ return new FlightRecorderDisplayUnit("/temp/FR_tetris.txt"); } );

	//disks.enableImgMaskTIFFs();
	//disks.enableImgPhantomTIFFs();
	//disks.enableImgOpticsTIFFs();

	//displays.registerImagingUnit("localFiji","localhost:54545");
	//displays.enableImgPhantomInImagingUnit("localFiji");
}


SceneControls& Scenario_Tetris::provideSceneControls()
{
	SceneControls::Constants myConstants;

	//override the default stop time
	myConstants.stopTime = 40.2f;
	myConstants.sceneSize.fromScalars(250,250,50); //microns, offset is at [0,0,0]
	//image res 2px/1um

	return *(new SceneControls(myConstants));
}
