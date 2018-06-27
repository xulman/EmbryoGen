#include "util/Vector3d.h"
#include "Geometries/Spheres.h"
#include "Simulation.h"
#include "Agents/AbstractAgent.h"
#include "Agents/NucleusAgent.h"

void Simulation::initializeAgents(void)
{
	//stepping in all directions -> influences the final number of nuclei
	const float dx = 14.0f;

	//to obtain a sequence of IDs for new agents...
	int ID=1;

	//longer axis x
	//symmetric/short axes y,z

	const float Xside  = (0.9f*sceneSize.x)/2.0f;
	const float YZside = (0.9f*sceneSize.y)/2.0f;

	for (float z=-Xside; z <= +Xside; z += dx)
	{
		//radius at this z position
		const float radius = YZside * sinf(acosf(fabsf(z)/Xside));

		const int howManyToPlace = (int)ceil(6.28f*radius / dx);
		for (int i=0; i < howManyToPlace; ++i)
		{
			const float ang = float(i)/float(howManyToPlace);

			//the wished position relative to [0,0,0] centre
			Vector3d<float> pos(z,radius * cosf(ang*6.28f),radius * sinf(ang*6.28f));

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
			s.updateRadius(0,4.0f);

			AbstractAgent* ag = new NucleusAgent(ID++,s,currTime,incrTime);
			ag->setOfficer(this);
			agents.push_back(ag);
			tracks.startNewTrack(ag->ID,frameCnt);
		}
	}
} //end of initializeAgents()


void Simulation::initializeAgents_aFew(void)
{
	//to obtain a sequence of IDs for new agents...
	int ID=1;

	const float radius = 0.7f*sceneSize.y;
	const int howManyToPlace = 6;

	for (int i=0; i < howManyToPlace; ++i)
	{
		const float ang = float(i)/float(howManyToPlace);

		//the wished position relative to [0,0,0] centre
		Vector3d<float> pos(radius * cosf(ang*6.28f),radius * sinf(ang*6.28f),0.0f);

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
		s.updateRadius(0,4.0f);

		AbstractAgent* ag = new NucleusAgent(ID++,s,currTime,incrTime);
		ag->setOfficer(this);
		agents.push_back(ag);
		tracks.startNewTrack(ag->ID,frameCnt);
	}
}
