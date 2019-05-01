#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Simulation.h"
#include "../Agents/NucleusAgent.h"
#include "Scenarios.h"

class myNucleusB: public NucleusAgent
{
public:
	myNucleusB(const int _ID, const std::string& _type,
	          const Spheres& shape,
	          const float _currTime, const float _incrTime):
		NucleusAgent(_ID,_type, shape, _currTime,_incrTime)
		{ cytoplasmWidth = 25.0f; }
};

void Scenario_pseudoDivision::initializeAgents(void)
{
	//to obtain a sequence of IDs for new agents...
	int ID=1;

	const int howManyToPlace = 2;
	for (int i=0; i < howManyToPlace; ++i)
	{
		//the wished position relative to [0,0,0] centre
		Vector3d<float> pos((float)i*1.0f,0,50);

		//position is shifted to the scene centre
		pos.x += sceneSize.x/2.0f;
		pos.y += sceneSize.y/2.0f;
		pos.z += sceneSize.z/2.0f;

		//position is shifted due to scene offset
		pos.x += sceneOffset.x;
		pos.y += sceneOffset.y;
		pos.z += sceneOffset.z;

		Spheres s(1);
		s.updateCentre(0,pos);
		s.updateRadius(0,10.0f);

		myNucleusB* ag = new myNucleusB(ID++,"nucleus",s,currTime,incrTime);
		ag->setOfficer(this);
		ag->setDetailedDrawingMode(true);
		agents.push_back(ag);
		tracks.startNewTrack(ag->ID,frameCnt);
	}
}
