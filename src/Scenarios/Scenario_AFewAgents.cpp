#include "../DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/ScalarImg.h"
#include "../Agents/Nucleus4SAgent.h"
#include "../Agents/ShapeHinter.h"
#include "common/Scenarios.h"

//no specific Nucleus (or other "prefilled" agent)


//==========================================================================
void Scenario_AFewAgents::initializeAgents(FrontOfficer* fo,int p,int)
{
	if (p != 1)
	{
		REPORT("Populating only the first FO (which is not this one).");
		return;
	}

	//to obtain a sequence of IDs for new agents...
	int ID=1;

	const float radius = 0.4f*params.constants.sceneSize.y;
	const int howManyToPlace = 6;

	Vector3d<float> sceneCentre(params.constants.sceneSize);
	sceneCentre /= 2.0f;
	sceneCentre += params.constants.sceneOffset;

	for (int i=0; i < howManyToPlace; ++i)
	{
		const float ang = float(i)/float(howManyToPlace);

		//the wished position relative to [0,0,0] centre
		Vector3d<float> pos(radius * std::cos(ang*6.28f),radius * std::sin(ang*6.28f),0.0f);

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

		Nucleus4SAgent* ag = new Nucleus4SAgent(ID++,"nucleus",s,params.constants.initTime,params.constants.incrTime);
		fo->startNewAgent(ag);
	}


	//shadow hinter geometry (micron size and resolution)
	Vector3d<float> size(2.9f*radius,2.2f*radius,1.5f*radius);
	const float xRes = 0.8f; //px/um
	const float yRes = 0.8f;
	const float zRes = 0.8f;
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

	//fill the actual shape (except for a dX x dY x dZ frame at the border)
	const size_t dZ = (size_t)(0.1*(double)Img.GetSizeZ());
	const size_t dY = (size_t)(0.2*(double)Img.GetSizeY());
	const size_t dX = (size_t)(0.1*(double)Img.GetSizeX());
	for (size_t z=dZ; z <= Img.GetSizeZ()-dZ; ++z)
	for (size_t y=dY; y <= Img.GetSizeY()-dY; ++y)
	for (size_t x=dX; x <= Img.GetSizeX()-dX; ++x)
		Img.SetVoxel(x,y,z,20);
	//Img.SaveImage("GradIN_ZeroOUT__original.tif");

	//now convert the actual shape into the shape geometry
	ScalarImg m(Img,ScalarImg::DistanceModel::GradIN_ZeroOUT);
	//m.saveDistImg("GradIN_ZeroOUT.tif");

	//finally, create the simulation agent to register this shape
	ShapeHinter* ag = new ShapeHinter(ID++,"yolk",m,params.constants.initTime,params.constants.incrTime);
	fo->startNewAgent(ag, false);
}


void Scenario_AFewAgents::initializeScene()
{
	displays.registerDisplayUnit( new SceneryBufferedDisplayUnit("localhost:8765") );
}


SceneControls& Scenario_AFewAgents::provideSceneControls()
{ return DefaultSceneControls; }
