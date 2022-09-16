#include "../DisplayUnits/SceneryBufferedDisplayUnit.hpp"
#include "../util/Vector3d.hpp"
#include "../Geometries/Spheres.hpp"
#include "../Agents/Nucleus4SAgent.hpp"
#include "common/Scenarios.hpp"
#include "../DisplayUnits/FlightRecorderDisplayUnit.hpp"

class myNucleus: public Nucleus4SAgent
{
public:
	myNucleus(const int _ID, const std::string& _type,
	          const Spheres& shape,
	          const float _currTime, const float _incrTime):
		Nucleus4SAgent(_ID,_type, shape, _currTime,_incrTime)
		{ cytoplasmWidth = 0.0f; }


	void advanceAndBuildIntForces(const float) override
	{
		//add own forces (mutually opposite) to "head&tail" spheres (to spin it)
		Vector3d<G_FLOAT> rotationVelocity;
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
};


//==========================================================================
void Scenario_dragAndRotate::initializeAgents(FrontOfficer* fo,int p,int)
{
	if (p != 1)
	{
report::message(fmt::format("Populating only the first FO (which is not this one)." ));
		return;
	}

	//to obtain a sequence of IDs for new agents...
	int ID=1;

	const int howManyToPlace = argc > 2? atoi(argv[2]) : 12;
report::debugMessage(fmt::format("will rotate {} nuclei..." , howManyToPlace));

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

		Spheres s(4);
		s.updateCentre(0,pos);
		s.updateRadius(0,3.0f);
		s.updateCentre(1,pos +6.0f*axis);
		s.updateRadius(1,5.0f);
		s.updateCentre(2,pos +12.0f*axis);
		s.updateRadius(2,5.0f);
		s.updateCentre(3,pos +18.0f*axis);
		s.updateRadius(3,3.0f);

		myNucleus* ag = new myNucleus(ID++,"nucleus",s,params.constants.initTime,params.constants.incrTime);
		ag->setDetailedDrawingMode(true);
		fo->startNewAgent(ag);
	}
}


void Scenario_dragAndRotate::initializeScene()
{
	displays.registerDisplayUnit( [](){ return new SceneryBufferedDisplayUnit("localhost:8765"); } );
	displays.registerDisplayUnit( [](){ return new FlightRecorderDisplayUnit("/temp/FR_dragAndRotate.txt"); } );
}


SceneControls& Scenario_dragAndRotate::provideSceneControls()
{ return DefaultSceneControls; }
