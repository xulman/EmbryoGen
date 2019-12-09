#include "../DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Agents/NucleusAgent.h"
#include "../Agents/util/CellCycle.h"
#include "common/Scenarios.h"


class myNucleusC: public NucleusAgent, CellCycle
{
public:
	myNucleusC(const int _ID, const std::string& _type,
	           const Spheres& shape,
	           const float _currTime, const float _incrTime):
		NucleusAgent(_ID,_type, shape, _currTime,_incrTime),
		CellCycle(8)
	{
		//debug report
		startCycling(_currTime);
		if (ID == 1) reportPhaseDurations();
	}

	void advanceAgent(const float gTime) override
	{
		triggerCycleMethods(gTime);
	}

	//this is how you can override lenghts of the cell phases
	void determinePhaseDurations(void) override
	{
		phaseDurations[G1Phase    ] = 0.125f * fullCycleDuration;
		phaseDurations[SPhase     ] = 0.125f * fullCycleDuration;
		phaseDurations[G2Phase    ] = 0.125f * fullCycleDuration;
		phaseDurations[Prophase   ] = 0.125f * fullCycleDuration;
		phaseDurations[Metaphase  ] = 0.125f * fullCycleDuration;
		phaseDurations[Anaphase   ] = 0.125f * fullCycleDuration;
		phaseDurations[Telophase  ] = 0.125f * fullCycleDuration;
		phaseDurations[Cytokinesis] = 0.125f * fullCycleDuration;
	}

	//cycle debug reports -- proofs of being called
	void startG1Phase(void) override { REPORT("do some work"); }
	void runG1Phase(const float time) override { REPORT("time=" << time); }
	void closeG1Phase(void) override { REPORT("do some work"); }

	void startSPhase(void) override { REPORT("do some work"); }
	void runSPhase(const float time) override { REPORT("time=" << time); }
	void closeSPhase(void) override { REPORT("do some work"); }

	void startG2Phase(void) override { REPORT("do some work"); }
	void runG2Phase(const float time) override { REPORT("time=" << time); }
	void closeG2Phase(void) override { REPORT("do some work"); }

	void startProphase(void) override { REPORT("do some work"); }
	void runProphase(const float time) override { REPORT("time=" << time); }
	void closeProphase(void) override { REPORT("do some work"); }

	void startMetaphase(void) override { REPORT("do some work"); }
	void runMetaphase(const float time) override { REPORT("time=" << time); }
	void closeMetaphase(void) override { REPORT("do some work"); }

	void startAnaphase(void) override { REPORT("do some work"); }
	void runAnaphase(const float time) override { REPORT("time=" << time); }
	void closeAnaphase(void) override { REPORT("do some work"); }

	void startTelophase(void) override { REPORT("do some work"); }
	void runTelophase(const float time) override { REPORT("time=" << time); }
	void closeTelophase(void) override { REPORT("do some work"); }

	void startCytokinesis(void) override { REPORT("do some work"); }
	void runCytokinesis(const float time) override { REPORT("time=" << time); }
	void closeCytokinesis(void) override { REPORT("do some work"); }
};


//==========================================================================
void Scenario_withCellCycle::initializeAgents(FrontOfficer* fo,int p,int)
{
	if (p != 1)
	{
		REPORT("Populating only the first FO (which is not this one).");
		return;
	}

	//to obtain a sequence of IDs for new agents...
	int ID=1;

	const int howManyToPlace = 2;
	for (int i=0; i < howManyToPlace; ++i)
	{
		//the wished position relative to [0,0,0] centre
		Vector3d<float> pos((float)i*1.0f,0,50);

		//position is shifted to the scene centre
		pos.x += params.constants.sceneSize.x/2.0f;
		pos.y += params.constants.sceneSize.y/2.0f;
		pos.z += params.constants.sceneSize.z/2.0f;

		//position is shifted due to scene offset
		pos.x += params.constants.sceneOffset.x;
		pos.y += params.constants.sceneOffset.y;
		pos.z += params.constants.sceneOffset.z;

		Spheres s(2);
		s.updateCentre(0,pos);
		s.updateRadius(0,10.0f);
		s.updateCentre(1,pos+Vector3d<float>(0,3,0));
		s.updateRadius(1,10.0f);

		myNucleusC* ag = new myNucleusC(ID++,"nucleus",s,params.constants.initTime,params.constants.incrTime);
		ag->setDetailedDrawingMode(true);
		fo->startNewAgent(ag);
	}
}


void Scenario_withCellCycle::initializeScene()
{
	params.displayUnit.RegisterUnit( new SceneryBufferedDisplayUnit("localhost:8765") );
}


SceneControls& Scenario_withCellCycle::provideSceneControls()
{
	SceneControls::Constants myConstants;
	myConstants.stopTime = 8.2f;

	return *(new SceneControls(myConstants));
}
