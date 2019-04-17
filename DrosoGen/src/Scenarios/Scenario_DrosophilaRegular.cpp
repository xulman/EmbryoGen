#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/ScalarImg.h"
#include "../Geometries/VectorImg.h"
#include "../Simulation.h"
#include "../Agents/AbstractAgent.h"
#include "../Agents/NucleusAgent.h"
#include "../Agents/ShapeHinter.h"
#include "../Agents/TrajectoriesHinter.h"
#include "Scenarios.h"

void Scenario_DrosophilaRegular::initializeAgents(void)
{
	//stepping in all directions -> influences the final number of nuclei
	const float dx = 14.0f;

	//to obtain a sequence of IDs for new agents...
	int ID=1;

	//longer axis x
	//symmetric/short axes y,z

	const float Xside  = (0.90f*sceneSize.x)/2.0f;
	const float YZside = (0.75f*sceneSize.y)/2.0f;

	for (float z=-Xside; z <= +Xside; z += dx)
	{
		//radius at this z position
		const float radius = YZside * std::sin(std::acos(std::abs(z)/Xside));

		const int howManyToPlace = (int)ceil(6.28f*radius / dx);
		for (int i=0; i < howManyToPlace; ++i)
		{
			const float ang = float(i)/float(howManyToPlace);

			//the wished position relative to [0,0,0] centre
			Vector3d<float> axis(0,std::cos(ang*6.28f),std::sin(ang*6.28f));
			Vector3d<float> pos(z,radius * axis.y,radius * axis.z);

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

			NucleusAgent* ag = new NucleusAgent(ID++,"nucleus",s,currTime,incrTime);
			ag->setOfficer(this);
			ag->startGrowTime=1.0f;
			agents.push_back(ag);
			tracks.startNewTrack(ag->ID,frameCnt);
		}
	}

	//-------------
	//now, read the mask image and make it a shape hinter...
	i3d::Image3d<i3d::GRAY8> initShape("../DrosophilaYolk_mask_lowerRes.tif");
	ScalarImg m(initShape,ScalarImg::DistanceModel::ZeroIN_GradOUT);
	//m.saveDistImg("GradIN_ZeroOUT.tif");

	//finally, create the simulation agent to register this shape
	AbstractAgent* ag = new ShapeHinter(ID++,"yolk",m,currTime,incrTime);
	ag->setOfficer(this);
	agents.push_back(ag);

	//-------------
	TrajectoriesHinter* at = new TrajectoriesHinter(ID++,"trajectories",
	                           initShape,VectorImg::ChoosingPolicy::avgVec,
	                           currTime,incrTime);
	at->setOfficer(this);
	agents.push_back(at);

	//the trajectories hinter:
	at->talkToHinter().readFromFile("../DrosophilaYolk_movement.txt", Vector3d<float>(2.f), 10.0f, 10.0f);
	REPORT("Timepoints: " << at->talkToHinter().size()
	    << ", Tracks: " << at->talkToHinter().knownTracks.size());
}
