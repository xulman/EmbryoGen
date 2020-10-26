#include "../DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "../Geometries/Spheres.h"
#include "../Geometries/util/SpheresFunctions.h"
#include "common/Scenarios.h"
#include "../Agents/NucleusNSAgent.h"
#include "../Agents/util/Texture.h"
#include "../util/texture/texture.h"
#include "../util/DivisionModels.h"

constexpr const int divModel_noOfSamples = 5;
constexpr const float divModel_deltaTimeBetweenSamples = 3;

float globeRadius = 40;

class SimpleDividingAgent: public NucleusAgent
{
public:
	/** c'tor for 1st generation of agents */
	SimpleDividingAgent(const int _ID, const std::string& _type,
	              const DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>& divModel2S,
	              const Spheres& shape,
	              const float _currTime, const float _incrTime):
		NucleusAgent(_ID,_type, shape, _currTime,_incrTime),
		divModels(divModel2S), divGeomModelled(2), futureGeometryBuilder(divGeomModelled,Vector3d<FLOAT>(1))
	{
		rotateDivModels();       //define the future division model
		rotateDivModels();       //to make it the current model and to obtain a new valid future one
		//NB: the rest comes already well defined for this case
		//TODO: randomize timeOfNextDivision

		advanceAgent(_currTime); //start up the agent's shape

		//set up agent's actual shape:
		Vector3d<FLOAT> position = getRandomPositionOnBigSphere(globeRadius);
		//
		//-position is our basal direction...
		position *= -1;
		Vector3d<FLOAT> polarity = getRandomPolarityVecGivenBasalDir(position);
		float centreHalfDist = 0.5f * (divGeomModelled.getCentres()[0] - divGeomModelled.getCentres()[1]).len();
		REPORT("Placing agent " << ID << " at " << position << ", with polarity " << polarity);
		//
		futureGeometry.updateCentre(0,-centreHalfDist*polarity -position);
		futureGeometry.updateCentre(1,+centreHalfDist*polarity -position);
		futureGeometry.updateRadius(0,divGeomModelled.getRadii()[0]);
		futureGeometry.updateRadius(1,divGeomModelled.getRadii()[1]);

		//setup the (re)builder that can iteratively update given geometry (here used
		//with 'futureGeometry') to the reference one (here 'divGeomModelled')
		basalBaseDir = position;
		basalBaseDir.changeToUnitOrZero();
		futureGeometryBuilder.setBasalSideDir(basalBaseDir);

		//only synchronizes geometryAlias with futureGeometry
		publishGeometry();
	}

	/** c'tor for 2nd and on generation of agents */
	SimpleDividingAgent(const int _ID, const std::string& _type,
	              const Spheres& shape, const Vector3d<FLOAT>& _basalSideDir,
	              const DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>& divModel2S,
	              const DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>::DivModelType* daughterModel,
	              const int daughterNo, const float _currTime, const float _incrTime):
		NucleusAgent(_ID,_type, shape, _currTime,_incrTime),
		divModels(divModel2S), divGeomModelled(2), futureGeometryBuilder(divGeomModelled,Vector3d<FLOAT>(1))
	{
		//we gonna continue in the given division model
		divFutureModel = daughterModel;
		rotateDivModels(); //NB: daughterModel becomes the current model, future model becomes valid too

		divModelState = modelMeAsDaughter;
		//I am a daughter, what else needs to be defined for this to work well?
		timeOfNextDivision = _currTime;
		whichDaughterAmI = daughterNo;

		advanceAgent(_currTime); //start up agent's reference shape

		//set up agent's actual shape:
		basalBaseDir = _basalSideDir;
		basalBaseDir.changeToUnitOrZero();
		futureGeometryBuilder.setBasalSideDir(basalBaseDir);
		futureGeometryBuilder.refreshThis(futureGeometry);
		futureGeometry.updateOwnAABB();

		//only synchronizes geometryAlias with futureGeometry
		publishGeometry();
	}

	void adjustGeometryByIntForces() override
	{
		//let the forces do their job first...
		NucleusAgent::adjustGeometryByForces();

		//...and then update the fresh futureGeometry with SphereBuilder
		futureGeometryBuilder.refreshThis(futureGeometry);
		futureGeometry.updateOwnAABB();

		basalBaseDir  = futureGeometry.getCentres()[0];
		basalBaseDir += futureGeometry.getCentres()[1];
		basalBaseDir.changeToUnitOrZero();
		basalBaseDir *= -1;
	}

	/*
	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>&) override
	{
		renderIntoPhantom(phantom);
	}
	*/

	void drawMask(DisplayUnit& du) override
	{
		int dID  = DisplayUnit::firstIdForAgentObjects(ID) + 10;
		du.DrawVector(++dID,0.5f*(futureGeometry.getCentres()[0]+futureGeometry.getCentres()[1]),basalBaseDir,0);
		//REPORT("Current polarity: " << (futureGeometry.getCentres()[1]-futureGeometry.getCentres()[0]).changeToUnitOrZero());

		/*
		for (int i = 0; i < futureGeometry.getNoOfSpheres(); ++i)
			du.DrawPoint(++dID,futureGeometry.getCentres()[i],futureGeometry.getRadii()[i],i);
		for (int i = 0; i < divGeomModelled.getNoOfSpheres(); ++i)
			du.DrawPoint(++dID,divGeomModelled.getCentres()[i],divGeomModelled.getRadii()[i],i);
		*/
		for (int i = 0; i < geometryAlias.getNoOfSpheres(); ++i)
			du.DrawPoint(++dID,geometryAlias.getCentres()[i],geometryAlias.getRadii()[i],(ID%3 +1)*2);
	}

	void drawForDebug(DisplayUnit& du) override
	{
	#ifdef DEBUG
		int gdID = DisplayUnit::firstIdForSceneDebugObjects() + ID*40 +10000;

		//forces:
		for (const auto& f : forcesForDisplay)
		{
			int color = 2; //default color: green (for shape hinter)
			if      (f.type == ftype_body)      color = 4; //cyan
			else if (f.type == ftype_repulsive || f.type == ftype_drive) color = 5; //magenta
			else if (f.type == ftype_slide)     color = 6; //yellow
			else if (f.type == ftype_friction)  color = 3; //blue
			else if (f.type == ftype_s2s) color = -1; //don't draw
			if (color > 0) du.DrawVector(gdID++, f.base,f, color);
		}
	#endif
	}

	void advanceAgent(float time) override
	{
		if (divModelState == shouldBeDividedByNow)
		{
			//int currentGen = std::stoi( this->getAgentType() ) +1;
			std::string agName = "nucleus"; //std::to_string(currentGen); agName += " gen";

			//syntactic short-cut
			const Vector3d<FLOAT>& mc0 = futureGeometry.getCentres()[0];
			const Vector3d<FLOAT>& mc1 = futureGeometry.getCentres()[1];

			Vector3d<FLOAT> sideStepDir = crossProduct(mc1-mc0,basalBaseDir);
			sideStepDir.changeToUnitOrZero();

			Spheres d1Geom(futureGeometry.getNoOfSpheres());
			Spheres d2Geom(futureGeometry.getNoOfSpheres());
			d1Geom.updateCentre(0,mc0-sideStepDir);
			d1Geom.updateCentre(1,mc0+sideStepDir);
			d2Geom.updateCentre(0,mc1-sideStepDir);
			d2Geom.updateCentre(1,mc1+sideStepDir);
			//NB: this just sets the initial direction, the actual shape of the daughters
			//will be "injected" into this later from their division model ref. geometries

			AbstractAgent *d1 = new SimpleDividingAgent(
			     Officer->getNextAvailAgentID(),agName,d1Geom,basalBaseDir,
			     divModels,divModel,0,time,this->incrTime );

			AbstractAgent *d2 = new SimpleDividingAgent(
			     Officer->getNextAvailAgentID(),agName,d2Geom,basalBaseDir,
			     divModels,divModel,1,time,this->incrTime );

			Officer->closeMotherStartDaughters(this,d1,d2);

			REPORT("============== divided ID " << ID << " to daughters' IDs " << d1->getID() << " & " << d2->getID() << " ==============");
			//we're done here, don't advance anything
			return;
		}
		if (divModelState == shouldBeModelledAsInterphase)
		{
			REPORT("============== switching to interphase regime ==============");
			//start planning the next division
			timeOfNextDivision += 2.5f * divModel_halfTimeSpan;
			timeOfInterphaseStart = time;
			timeOfInterphaseStop = timeOfNextDivision - divModel_halfTimeSpan;

			divModelState = modelMeAsInterphase;
			if (timeOfInterphaseStop <= timeOfInterphaseStart)
			{
				REPORT("WARNING: skipping the interphase stage completely...");
				divModelState = shouldBeModelledAsMother;
			}
		}
		if (divModelState == shouldBeModelledAsMother)
		{
			REPORT("============== switching back to mother regime ==============");
			//restart as mother again (in a new division model)
			whichDaughterAmI = -1;
			rotateDivModels();

			divModelState = modelMeAsMother;
		}

		if (divModelState == modelMeAsMother) updateDivGeom_asMother(time);
		else if (divModelState == modelMeAsDaughter) updateDivGeom_asDaughter(time);
		else if (divModelState == modelMeAsInterphase) updateDivGeom_asInterphase(time);
		else REPORT("?????? should never get here!");
	}

private:
	// ------------------------ division model data ------------------------
	const float divModel_halfTimeSpan = divModel_deltaTimeBetweenSamples * divModel_noOfSamples;

	const DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>& divModels;
	const DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>::DivModelType* divModel;
	const DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>::DivModelType* divFutureModel;

	void rotateDivModels()
	{
		divModel = divFutureModel;
		divFutureModel = &divModels.getRandomModel();
	}

	// ------------------------ division model state and planning ------------------------
	enum divModelStates
	{
		modelMeAsMother, shouldBeDividedByNow,
		modelMeAsDaughter, shouldBeModelledAsInterphase,
		modelMeAsInterphase, shouldBeModelledAsMother
	};
	divModelStates divModelState = modelMeAsMother;

	float timeOfInterphaseStart, timeOfInterphaseStop;
	float timeOfNextDivision = 18.7f; //when switch to 'shouldBeDividedByNow' should occur
	int whichDaughterAmI = -1;        //used also in 'modelMeAsDaughter'

	// ------------------------ model reference geometry ------------------------
	Spheres divGeomModelled;
	SpheresFunctions::LinkedSpheres<FLOAT> futureGeometryBuilder;
	Vector3d<FLOAT> basalBaseDir;

	void updateDivGeom_asMother(float currTime)
	{
		if (divModelState != modelMeAsMother)
		{
			DEBUG_REPORT("skipping... because my state (" << divModelState << ") is different");
			return;
		}

		float modelRelativeTime = currTime - timeOfNextDivision;
		if (modelRelativeTime > -0.0001f) divModelState = shouldBeDividedByNow;

		//assure the time is within sensible bounds
		modelRelativeTime = std::max(modelRelativeTime, -divModel_halfTimeSpan);
		modelRelativeTime = std::min(modelRelativeTime, -0.0001f);

		DEBUG_REPORT("abs. time = " << currTime << ", planned div time = " << timeOfNextDivision
		          << " ==> relative Mother time for the model = " << modelRelativeTime);

		divGeomModelled.updateCentre(0, Vector3d<FLOAT>(0));
		divGeomModelled.updateCentre(1, Vector3d<FLOAT>(0,divModel->getMotherDist(modelRelativeTime,0),0));
		divGeomModelled.updateRadius(0, divModel->getMotherRadius(modelRelativeTime,0));
		divGeomModelled.updateRadius(1, divModel->getMotherRadius(modelRelativeTime,1));
	}


	void updateDivGeom_asDaughter(float currTime)
	{
		if (divModelState != modelMeAsDaughter)
		{
			DEBUG_REPORT("skipping... because my state (" << divModelState << ") is different");
			return;
		}

		float modelRelativeTime = currTime - timeOfNextDivision;
		if (modelRelativeTime > divModel_halfTimeSpan) divModelState = shouldBeModelledAsInterphase;

		//assure the time is within sensible bounds
		modelRelativeTime = std::max(modelRelativeTime, 0.f);
		modelRelativeTime = std::min(modelRelativeTime, divModel_halfTimeSpan);

		DEBUG_REPORT("abs. time = " << currTime << ", planned div time = " << timeOfNextDivision
		          << " ==> relative Daughter #" << whichDaughterAmI << " time for the model = " << modelRelativeTime);

		divGeomModelled.updateCentre(0, Vector3d<FLOAT>(0));
		divGeomModelled.updateCentre(1, Vector3d<FLOAT>(0,divModel->getDaughterDist(modelRelativeTime,whichDaughterAmI,0),0));
		divGeomModelled.updateRadius(0, divModel->getDaughterRadius(modelRelativeTime,whichDaughterAmI,0));
		divGeomModelled.updateRadius(1, divModel->getDaughterRadius(modelRelativeTime,whichDaughterAmI,1));
	}


	void updateDivGeom_asInterphase(float currTime)
	{
		if (divModelState != modelMeAsInterphase)
		{
			DEBUG_REPORT("skipping... because my state (" << divModelState << ") is different");
			return;
		}

		float progress = (currTime - timeOfInterphaseStart) / (timeOfInterphaseStop - timeOfInterphaseStart);
		if (currTime > timeOfInterphaseStop) divModelState = shouldBeModelledAsMother;

		//assure the progress is within sensible bounds
		progress = std::max(progress, 0.f);
		progress = std::min(progress, 1.f);

		DEBUG_REPORT("abs. time = " << currTime << ", progress = " << progress);

		//interpolate linearly between the last daughter state to the first mother state
		divGeomModelled.updateCentre(0, Vector3d<FLOAT>(0));
		divGeomModelled.updateCentre(1,
		      (1.f-progress) * Vector3d<FLOAT>(0,divModel->getDaughterDist(+divModel_halfTimeSpan,whichDaughterAmI,0),0)
		      +     progress * Vector3d<FLOAT>(0,divFutureModel->getMotherDist(-divModel_halfTimeSpan,0),0)
		);

		divGeomModelled.updateRadius(0,
		      (1.f-progress) * divModel->getDaughterRadius(+divModel_halfTimeSpan,whichDaughterAmI,0)
		      +     progress * divFutureModel->getMotherRadius(-divModel_halfTimeSpan,0)
		);

		divGeomModelled.updateRadius(1,
		      (1.f-progress) * divModel->getDaughterRadius(+divModel_halfTimeSpan,whichDaughterAmI,1)
		      +     progress * divFutureModel->getMotherRadius(-divModel_halfTimeSpan,1)
		);
	}

	static Vector3d<FLOAT> getRandomPolarityVecGivenBasalDir(const Vector3d<FLOAT>& basalDir)
	{
		//suppose we're at coordinate centre,
		//we consider the equation for a plane through centre with normal of 'basalDir',
		//we find some random point in that plane
		FLOAT x,y,z;
		x = GetRandomUniform(-5.f,+5.f);
		y = GetRandomUniform(-5.f,+5.f);
		z = (basalDir.x*x + basalDir.y*y) / -basalDir.z;

		Vector3d<FLOAT> polarity;
		polarity.fromScalars(x,y,z);
		polarity.changeToUnitOrZero();
		return polarity; //"copy ellision", thank you!
	}

	static Vector3d<FLOAT> getRandomPositionOnBigSphere(const float radius=50)
	{
		FLOAT x,y,z;
		x = GetRandomUniform(-5.f,+5.f);
		y = GetRandomUniform(-5.f,+5.f);
		z = GetRandomUniform(-5.f,+5.f);

		Vector3d<FLOAT> position;
		position.fromScalars(x,y,z);
		position.changeToUnitOrZero();
		position *= radius;
		return position; //"copy ellision", thank you!
	}
};


//==========================================================================
void Scenario_modelledDivision::initializeAgents(FrontOfficer* fo,int p,int)
{
	if (p != 1)
	{
		REPORT("Populating only the first FO (which is not this one).");
		return;
	}

	static DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>
		divModel2S("../src/tests/data/divisionModelsForEmbryoGen.dat",divModel_deltaTimeBetweenSamples);

	const Vector3d<float> gridStart(params.constants.sceneOffset.x+10,params.constants.sceneSize.y,params.constants.sceneSize.z);
	const Vector3d<float> gridStep(20,0,0);

	Spheres twoS(2);
	for (int i = 0; i < 1; ++i)
	{
		fo->startNewAgent(new SimpleDividingAgent(
				fo->getNextAvailAgentID(),"nucleus",divModel2S,twoS,params.constants.initTime,params.constants.incrTime
				));
	}
}


void Scenario_modelledDivision::initializeScene()
{
	displays.registerDisplayUnit( [](){ return new SceneryBufferedDisplayUnit("localhost:8765"); } );
	//displays.registerDisplayUnit( [](){ return new FlightRecorderDisplayUnit("/temp/FR_tetris.txt"); } );

	//disks.enableImgMaskTIFFs();
	//disks.enableImgPhantomTIFFs();
	//disks.enableImgOpticsTIFFs();

	//displays.registerImagingUnit("localFiji","localhost:54545");
	//displays.enableImgPhantomInImagingUnit("localFiji");
}


SceneControls& Scenario_modelledDivision::provideSceneControls()
{
	//override the some defaults
	SceneControls::Constants myConstants;
	myConstants.stopTime = 100.2f;
	myConstants.imgRes.fromScalar(0.5f);
	myConstants.sceneSize.fromScalars(300,20,20);
	myConstants.sceneOffset = -0.5f * myConstants.sceneSize;

	myConstants.expoTime = 3.f*myConstants.incrTime;

	return *(new SceneControls(myConstants));
}
