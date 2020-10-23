#include "../DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/util/SpheresFunctions.h"
#include "common/Scenarios.h"
#include "../Agents/NucleusNSAgent.h"
#include "../Agents/util/Texture.h"
#include "../util/texture/texture.h"

class TetrisNucleus: public NucleusNSAgent, TextureUpdaterNS
{
public:
	TetrisNucleus(const int _ID, const std::string& _type,
	              const Spheres& shape, const int _activeSphereIdx,
	              const float _currTime, const float _incrTime):
		NucleusNSAgent(_ID,_type, shape, _currTime,_incrTime),
		TextureUpdaterNS(shape,1),
		activeSphereIdx(_activeSphereIdx)
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

	void drawMask(DisplayUnit& du) override
	{
		int dID  = DisplayUnit::firstIdForAgentObjects(ID);
		int ldID = DisplayUnit::firstIdForAgentDebugObjects(ID);

		//spheres all green, except: 0th is white, "active" is red
		du.DrawPoint(dID++,futureGeometry.getCentres()[0],futureGeometry.getRadii()[0],0);
		for (int i=1; i < futureGeometry.getNoOfSpheres(); ++i)
			du.DrawPoint(dID++,futureGeometry.getCentres()[i],futureGeometry.getRadii()[i],i == activeSphereIdx ? 1 : 2);

		//sphere orientations as local debug, white vectors
		Vector3d<FLOAT> orientVec;
		for (int i=0; i < futureGeometry.getNoOfSpheres(); ++i)
		{
			getLocalOrientation(futureGeometry,i,orientVec);
			du.DrawVector(ldID++,futureGeometry.getCentres()[i],orientVec,0);
		}

		drawForDebug(du);
	}

	void advanceAgent(float time) override
	{
		const FLOAT velocity = (FLOAT)1.0f;
		const Vector3d<FLOAT> travellingVelocity(0,velocity,0);
		if ( ((int)time % 18) < 8 )
		{
			exertForceOnSphere(activeSphereIdx,
			                   (weights[activeSphereIdx]/velocity_PersistenceTime) * travellingVelocity,
			                   ftype_drive );
		}
	}

private:
	const int activeSphereIdx;
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
		case -1:
			agentName = buildStringFromStream("" << y << "__Box");
			spheres.updateCentre(0,currGridPos);
			spheres.updateCentre(1,currGridPos + Vector3d<float>(-sDist,-sDist/2.0f,0));
			spheres.updateCentre(2,currGridPos + Vector3d<float>(-sDist,+sDist/2.0f,0));
			spheres.updateCentre(3,currGridPos + Vector3d<float>(     0,-sDist/1.5f,0));
			spheres.updateCentre(4,currGridPos + Vector3d<float>(     0,+sDist/1.5f,0));
			spheres.updateCentre(5,currGridPos + Vector3d<float>(+sDist,-sDist/2.0f,0));
			spheres.updateCentre(6,currGridPos + Vector3d<float>(+sDist,+sDist/2.0f,0));
			break;
		case 0:
			agentName = buildStringFromStream("" << y << "__Line");
			spheres.updateCentre(0,currGridPos);
			spheres.updateCentre(1,currGridPos + Vector3d<float>(-sDist*3,-sDist*0.9f,0));
			spheres.updateCentre(2,currGridPos + Vector3d<float>(-sDist*2,-sDist*0.6f,0));
			spheres.updateCentre(3,currGridPos + Vector3d<float>(-sDist*1,-sDist*0.3f,0));
			spheres.updateCentre(4,currGridPos + Vector3d<float>(+sDist*1,+sDist*0.3f,0));
			spheres.updateCentre(5,currGridPos + Vector3d<float>(+sDist*2,+sDist*0.6f,0));
			spheres.updateCentre(6,currGridPos + Vector3d<float>(+sDist*3,+sDist*0.9f,0));
			break;
		case 1:
			agentName = buildStringFromStream("" << y << "__BigT");
			spheres.updateCentre(0,currGridPos);
			spheres.updateCentre(1,currGridPos + Vector3d<float>(-sDist*2,       0,0));
			spheres.updateCentre(2,currGridPos + Vector3d<float>(-sDist*1,       0,0));
			spheres.updateCentre(3,currGridPos + Vector3d<float>(       0,-sDist*1,0));
			spheres.updateCentre(4,currGridPos + Vector3d<float>(       0,-sDist*2,0));
			spheres.updateCentre(5,currGridPos + Vector3d<float>(+sDist*1,       0,0));
			spheres.updateCentre(6,currGridPos + Vector3d<float>(+sDist*2,       0,0));
			break;
		case 2:
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
				agentName,spheres,y < 2 ? y+1 : spheres.getNoOfSpheres()+y-4,
				params.constants.initTime,params.constants.incrTime
			) );
	}

	//right-most column with 2S
	int x = 3;
	for (int y = 0; y < 4; ++y)
	{
		currGridPos.fromScalars(x,y,2).elemMult(gridStep);
		currGridPos += gridStart;
		agentName = buildStringFromStream("" << y << "__2SModel");

		const int connLines = 5;
		const int inbetweeners = 5;
		Spheres twoS(2); // + connLines*inbetweeners);

		std::vector< Vector3d<FLOAT>* > lineUps;
		for (int i = 0; i < inbetweeners; ++i)
			lineUps.push_back( new Vector3d<FLOAT>() );

		const float maxAxisDev = 1.4f;
		twoS.updateCentre(0, currGridPos + Vector3d<float>(-9.f,
					GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist),
					GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist)));
		twoS.updateCentre(1, currGridPos + Vector3d<float>(+9.f,
					GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist),
					GetRandomUniform(-maxAxisDev*sDist,+maxAxisDev*sDist)));
		twoS.updateRadius(0, 4);
		twoS.updateRadius(1, 4);

		REPORT("-------------------------------------");
		SpheresFunctions::LinkedSpheres<float> builder(twoS, Vector3d<float>(0,1,0));
		float minAngle = -M_PI_2;
		float maxAngle = +M_PI_2;
		float stepAngle = (maxAngle-minAngle)/(float)connLines;
		builder.resetAllAzimuthsToExtrusions(minAngle, stepAngle, maxAngle);
		builder.resetNoOfSpheresInAllAzimuths(inbetweeners);
		builder.defaultNoOfSpheresOnConnectionLines=2;
		builder.addOrChangeAzimuthToExtrusion(M_PI-0.2);
		builder.addOrChangeAzimuthToExtrusion(M_PI+0.2);
		builder.addOrChangeAzimuth(M_PI, builder.defaultPosNoAdjustmentRef, builder.defaultRadiusNoChgRef, 4);
		builder.removeAzimuth(M_PI+0.2);
		//builder.addToPlan(0,1,3);
		builder.printPlan();
		REPORT("necessary cnt: " << builder.getNoOfNecessarySpheres());
		Spheres manyS(builder.getNoOfNecessarySpheres());
		builder.buildInto(manyS);
		builder.printPlan();

		/*
		fo->startNewAgent( new TetrisNucleus(
		        fo->getNextAvailAgentID(),
		        agentName,twoS,y < 2 ? 0 : 1,
		        params.constants.initTime,params.constants.incrTime
			));
		*/
		fo->startNewAgent( new NucleusAgent(
		        fo->getNextAvailAgentID(),
		        agentName,manyS,
		        params.constants.initTime,params.constants.incrTime
			));

		for (auto v : lineUps) delete v;
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
