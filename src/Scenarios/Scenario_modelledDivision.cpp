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

class SimpleDividingAgent: public NucleusNSAgent
{
public:
	/** c'tor for 1st generation of agents */
	SimpleDividingAgent(const int _ID, const std::string& _type,
	              const DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>& divModel2S,
	              const Spheres& shape,
	              const float _currTime, const float _incrTime):
		NucleusNSAgent(_ID,_type, shape, _currTime,_incrTime),
		divModels(divModel2S), divGeomModelled(2)
	{
		rotateDivModels();       //define the future division model
		rotateDivModels();       //to make it the current model and to obtain a new valid future one
		//NB: the rest comes already well defined for this case
		//TODO: randomize timeOfNextDivision

		advanceAgent(_currTime); //start up the agent's shape
		publishGeometry();       //publish/propagate the shape to geometries for forces and for collisions
	}

	/** c'tor for 2nd and on generation of agents */
	SimpleDividingAgent(const int _ID, const std::string& _type,
	              const DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>& divModel2S,
	              const Spheres& shape, const int daughterNo,
	              const DivisionModels2S<divModel_noOfSamples,divModel_noOfSamples>::DivModelType* daughterModel,
	              const float _currTime, const float _incrTime):
		NucleusNSAgent(_ID,_type, shape, _currTime,_incrTime),
		divModels(divModel2S), divGeomModelled(2)
	{
		//we gonna continue in the given division model
		divFutureModel = daughterModel;
		rotateDivModels(); //NB: daughterModel becomes the current model, future model becomes valid too

		divModelState = modelMeAsDaughter;
		//I am a daughter, what else needs to be defined for this to work well?
		timeOfNextDivision = _currTime;
		whichDaughterAmI = daughterNo;

		advanceAgent(_currTime); //start up the agent's shape
		publishGeometry();       //publish/propagate the shape to geometries for forces and for collisions
	}

	void publishGeometry() override
	{
		for (int i = 0; i < divGeomModelled.getNoOfSpheres(); ++i)
		{
			futureGeometry.updateCentre(i, divGeomModelled.getCentres()[i]);
			futureGeometry.updateRadius(i, divGeomModelled.getRadii()[i]);
		}
		//finish up the futureGeometry with SphereBuilder

		//call the upstream to clone futureGeometry into geometryAlias
		NucleusNSAgent::publishGeometry();
	}

	/*
	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>&) override
	{
		renderIntoPhantom(phantom);
	}
	*/

	void drawMask(DisplayUnit& du) override
	{
		/*
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
		*/

		NucleusNSAgent::drawMask(du);
		drawForDebug(du);
	}

	void advanceAgent(float time) override
	{
		if (divModelState == shouldBeDividedByNow)
		{
			int currentGen = std::stoi( this->getAgentType() ) +1;
			std::string agName = std::to_string(currentGen); agName += " gen";

			AbstractAgent *d1 = new SimpleDividingAgent(
			     Officer->getNextAvailAgentID(),agName,divModels,Spheres(futureGeometry.getNoOfSpheres()),
			     0,divModel,time,this->incrTime );

			AbstractAgent *d2 = new SimpleDividingAgent(
			     Officer->getNextAvailAgentID(),agName,divModels,Spheres(futureGeometry.getNoOfSpheres()),
			     1,divModel,time,this->incrTime );

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
				fo->getNextAvailAgentID(),"1 gen",divModel2S,twoS,params.constants.initTime,params.constants.incrTime
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