#include "../DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "../DisplayUnits/FlightRecorderDisplayUnit.h"
#include "../util/rnd_generators.h"
#include "../util/Vector3d.h"
#include "../Geometries/ScalarImg.h"
#include "../Geometries/VectorImg.h"
#include "../Agents/NucleusNSAgent.h"
#include "../Agents/ShapeHinter.h"
#include "../Agents/TrajectoriesHinter.h"
#include "../Geometries/util/SpheresFunctions.h"
#include "common/Scenarios.h"

class GrowableNucleusRand: public NucleusNSAgent
{
public:
	GrowableNucleusRand(const int _ID, const std::string& _type,
	                const Spheres& shape,
	                const float _currTime, const float _incrTime):
		NucleusNSAgent(_ID,_type, shape, _currTime,_incrTime),
		si(futureGeometry), presentationGeom( setupSpheresInterpolationAndReturnNoOfSpheres() )
	{}

	float startGrowTime = 99999999.f;
	float stopGrowTime  = 99999999.f;

protected:
	int incrCnt = 0;

	void adjustGeometryByIntForces() override
	{
		//adjust the shape at first
		if (currTime >= startGrowTime && currTime <= stopGrowTime && incrCnt < 30)
		{
			//"grow factor"
			const G_FLOAT dR = 0.05f;    //radius
			const G_FLOAT dD = 1.8f*dR;  //diameter

			//grow the current geometry
			SpheresFunctions::grow4SpheresBy(futureGeometry, dR,dD);

			//also update the expected distances
			for (int j=1; j <= 3; ++j) // (neighbors-1) in between
			for (int i=j; i < 4; ++i)
			{
				*distanceMatrix(i-j,i) += (G_FLOAT)j*dD;
				*distanceMatrix(i,i-j) += (G_FLOAT)j*dD;
			}

			//emergency break...
			++incrCnt;
		}

		//also call the upstream original method
		NucleusNSAgent::adjustGeometryByIntForces();
	}

	//expanded masks
	SpheresFunctions::Interpolator<G_FLOAT> si;
	int setupSpheresInterpolationAndReturnNoOfSpheres()
	{
		si.addToPlan(0,1,2);
		si.addToPlan(1,2,2);
		si.addToPlan(2,3,2);
		return si.getOptimalTargetSpheresNo();
	}
	Spheres presentationGeom;

	void publishGeometry(void) override
	{
		NucleusNSAgent::publishGeometry();
		si.expandSrcIntoThis(presentationGeom);
		presentationGeom.updateOwnAABB();
	}

	void drawMask(i3d::Image3d<i3d::GRAY16>& img) override
	{
		presentationGeom.renderIntoMask(img,(i3d::GRAY16)ID);
	}

	void drawMask(DisplayUnit& du) override
	{
		NucleusAgent::drawMask(du);
		int dID  = DisplayUnit::firstIdForAgentObjects(ID);
		for (int i = futureGeometry.getNoOfSpheres(); i < presentationGeom.getNoOfSpheres(); ++i)
			du.DrawPoint(dID+i+5, presentationGeom.getCentres()[i], presentationGeom.getRadii()[i],3);
	}
};


//==========================================================================
void Scenario_DrosophilaRandom::initializeAgents(FrontOfficer* fo,int p,int)
{
	if (p != 1)
	{
		REPORT("Populating only the first FO (which is not this one).");
		return;
	}

	//stepping in all directions -> influences the final number of nuclei
	const float dx = 14.0f;

	//to obtain a sequence of IDs for new agents...
	int ID=1;

	//longer axis x
	//symmetric/short axes y,z

	const float Xside  = (0.90f*params.constants.sceneSize.x)/2.0f;
	const float YZside = (0.75f*params.constants.sceneSize.y)/2.0f;

	//rnd shifter along axes
	rndGeneratorHandle coordShifterRNG;
	float angVar = 0.f;

	for (float z=-Xside; z <= +Xside; z += dx)
	{
		//radius at this z position
		const float radius = YZside * std::sin(std::acos(std::abs(z)/Xside));

		//shifts along the perimeter of the cylinder
		angVar += GetRandomGauss(0.f, 0.8f, coordShifterRNG);

		const int howManyToPlace = (int)ceil(6.28f*radius / dx);
		for (int i=0; i < howManyToPlace; ++i)
		{
			const float ang = float(i)/float(howManyToPlace) +angVar;

			//the wished position relative to [0,0,0] centre
			Vector3d<float> axis(0,std::cos(ang*6.28f),std::sin(ang*6.28f));
			Vector3d<float> pos(z,radius * axis.y,radius * axis.z);

			//position is shifted to the scene centre
			pos.x += params.constants.sceneSize.x/2.0f;
			pos.y += params.constants.sceneSize.y/2.0f;
			pos.z += params.constants.sceneSize.z/2.0f;

			//position is shifted due to scene offset
			pos.x += params.constants.sceneOffset.x;
			pos.y += params.constants.sceneOffset.y;
			pos.z += params.constants.sceneOffset.z;

			//also random shift along the main axis
			pos.x += GetRandomGauss(0.f,0.3f*dx,coordShifterRNG);

			Spheres s(4);
			s.updateCentre(0,pos);
			s.updateRadius(0,3.0f);
			s.updateCentre(1,pos +6.0f*axis);
			s.updateRadius(1,5.0f);
			s.updateCentre(2,pos +12.0f*axis);
			s.updateRadius(2,5.0f);
			s.updateCentre(3,pos +18.0f*axis);
			s.updateRadius(3,3.0f);

			GrowableNucleusRand* ag = new GrowableNucleusRand(ID++,"nucleus growable random",s,params.constants.initTime,params.constants.incrTime);
			ag->startGrowTime=10.0f;
			fo->startNewAgent(ag);
		}
	}

	//-------------
	//now, read the mask image and make it a shape hinter...
	i3d::Image3d<i3d::GRAY8> initShape("../../DrosophilaYolk_mask_lowerRes.tif");
	ScalarImg m(initShape,ScalarImg::DistanceModel::ZeroIN_GradOUT);
	//m.saveDistImg("GradIN_ZeroOUT.tif");

	//finally, create the simulation agent to register this shape
	ShapeHinter* ag = new ShapeHinter(ID++,"yolk",m,params.constants.initTime,params.constants.incrTime);
	fo->startNewAgent(ag, false);
}


void Scenario_DrosophilaRandom::initializeScene()
{
	displays.registerDisplayUnit( [](){ return new SceneryBufferedDisplayUnit("localhost:8765"); } );
	displays.registerDisplayUnit( [](){ return new FlightRecorderDisplayUnit("/temp/FR_randomDro.txt"); } );

	//disks.enableImgMaskTIFFs();
}

class mySceneControls : public SceneControls
{
	int doMasks = 0;

	void updateControls(const float currTime) override
	{
		if (currTime > 9.5 && doMasks == 0)
		{
			REPORT_NOHEADER("enabling export of masks")
			ctx().disks.enableImgMaskTIFFs();
			doMasks = 1;
		}
		else
		if (currTime > 11.1 && doMasks == 1)
		{
			REPORT_NOHEADER("disabling export of masks")
			ctx().disks.disableImgMaskTIFFs();
			doMasks = 2;
		}
		else
			REPORT_NOHEADER("no change");
	}
};


SceneControls& Scenario_DrosophilaRandom::provideSceneControls()
{ return *(new mySceneControls()); }
