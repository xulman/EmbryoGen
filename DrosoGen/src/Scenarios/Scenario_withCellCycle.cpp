#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Simulation.h"
#include "../Agents/NucleusAgent.h"
#include "../Agents/util/CellCycle.h"
#include "Scenarios.h"


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
	void startG1Phase(void) override { DEBUG_REPORT("q"); }
	void runG1Phase(const float time) override { DEBUG_REPORT("time=" << time); }
	void closeG1Phase(void) override { DEBUG_REPORT("q"); }

	void startSPhase(void) override { DEBUG_REPORT("q"); }
	void runSPhase(const float time) override { DEBUG_REPORT("time=" << time); }
	void closeSPhase(void) override { DEBUG_REPORT("q"); }

	void startG2Phase(void) override { DEBUG_REPORT("q"); }
	void runG2Phase(const float time) override { DEBUG_REPORT("time=" << time); }
	void closeG2Phase(void) override { DEBUG_REPORT("q"); }

	void startProphase(void) override { DEBUG_REPORT("q"); }
	void runProphase(const float time) override { DEBUG_REPORT("time=" << time); }
	void closeProphase(void) override { DEBUG_REPORT("q"); }

	void startMetaphase(void) override { DEBUG_REPORT("q"); }
	void runMetaphase(const float time) override { DEBUG_REPORT("time=" << time); }
	void closeMetaphase(void) override { DEBUG_REPORT("q"); }

	void startAnaphase(void) override { DEBUG_REPORT("q"); }
	void runAnaphase(const float time) override { DEBUG_REPORT("time=" << time); }
	void closeAnaphase(void) override { DEBUG_REPORT("q"); }

	void startTelophase(void) override { DEBUG_REPORT("q"); }
	void runTelophase(const float time) override { DEBUG_REPORT("time=" << time); }
	void closeTelophase(void) override { DEBUG_REPORT("q"); }

	void startCytokinesis(void) override { DEBUG_REPORT("q"); }
	void runCytokinesis(const float time) override { DEBUG_REPORT("time=" << time); }
	void closeCytokinesis(void) override { DEBUG_REPORT("q"); }
};


void Scenario_withCellCycle::initializeScenario(void)
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

		Spheres s(2);
		s.updateCentre(0,pos);
		s.updateRadius(0,10.0f);
		s.updateCentre(1,pos+Vector3d<float>(0,3,0));
		s.updateRadius(1,10.0f);

		myNucleusC* ag = new myNucleusC(ID++,"nucleus",s,currTime,incrTime);
		ag->setDetailedDrawingMode(true);
		startNewAgent(ag);
	}

	stopTime = 8.2f;
}
