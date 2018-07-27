#include "util/Vector3d.h"
#include "Geometries/Spheres.h"
#include "Geometries/MaskImg.h"
#include "Simulation.h"
#include "Agents/AbstractAgent.h"
#include "Agents/NucleusAgent.h"
#include "Agents/ShapeHinter.h"

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

	const float radius = 0.4f*sceneSize.y;
	const int howManyToPlace = 6;

	Vector3d<float> sceneCentre(sceneSize);
	sceneCentre /= 2.0f;
	sceneCentre += sceneOffset;

	for (int i=0; i < howManyToPlace; ++i)
	{
		const float ang = float(i)/float(howManyToPlace);

		//the wished position relative to [0,0,0] centre
		Vector3d<float> pos(radius * cosf(ang*6.28f),radius * sinf(ang*6.28f),0.0f);

		//position is shifted to the scene centre, accounting for scene offset
		pos += sceneCentre;

		Spheres s(4);
		s.updateCentre(0,pos+Vector3d<float>(0,0,-9));
		s.updateRadius(0,3.0f);
		s.updateCentre(1,pos+Vector3d<float>(0,0,-3));
		s.updateRadius(1,5.0f);
		s.updateCentre(2,pos+Vector3d<float>(0,0,+3));
		s.updateRadius(2,5.0f);
		s.updateCentre(3,pos+Vector3d<float>(0,0,+9));
		s.updateRadius(3,3.0f);

		AbstractAgent* ag = new NucleusAgent(ID++,s,currTime,incrTime);
		ag->setOfficer(this);
		agents.push_back(ag);
		tracks.startNewTrack(ag->ID,frameCnt);
	}


	//shadow hinter geometry (micron size and resolution)
	Vector3d<float> size(2.9f*radius,2.2f*radius,0.5f*radius);
	const float xRes = 0.35f; //px/um
	const float yRes = 0.35f;
	const float zRes = 0.35f;
	DEBUG_REPORT("Shape hinter image size   [um]: " << size);

	//allocate and init memory for the hinter representation
	i3d::Image3d<i3d::GRAY8> Img;
	Img.MakeRoom((size_t)(size.x*xRes),(size_t)(size.y*yRes),(size_t)(size.z*zRes));
	Img.GetVoxelData() = 0;

	//metadata...
	size *= -0.5f;
	size += sceneCentre;
	Img.SetOffset(     i3d::Offset(size.x,size.y,size.z) );
	Img.SetResolution( i3d::Resolution(xRes,yRes,zRes) );
	DEBUG_REPORT("Shape hinter image offset [um]: " << size);

	//fill the actual shape
	for (size_t z=(size_t)(0.1*Img.GetSizeZ()); z <= (size_t)(0.9*Img.GetSizeZ()); ++z)
	for (size_t y=(size_t)(0.2*Img.GetSizeY()); y <= (size_t)(0.8*Img.GetSizeY()); ++y)
	for (size_t x=(size_t)(0.1*Img.GetSizeX()); x <= (size_t)(0.9*Img.GetSizeX()); ++x)
		Img.SetVoxel(x,y,z,20);
	Img.SaveImage("GradIN_ZeroOUT__original.tif");

	//now convert the actual shape into the shape geometry
	MaskImg m(Img,MaskImg::DistanceModel::GradIN_GradOUT);
	m.saveDistImg("GradIN_ZeroOUT.tif");

	//finally, create the simulation agent to register this shape
	AbstractAgent* ag = new ShapeHinter(ID++,m,currTime,incrTime);
	ag->setOfficer(this);
	agents.push_back(ag);
}
