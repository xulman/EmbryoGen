#include "../Scenarios/common/Scenarios.h"
#include "../Scenarios/common/Scenario.h"
#include "../FrontOfficer.h"
#include "../util/report.h"

// ------------------- constants -------------------
SceneControls& provideSceneControls_withOwnConstants()
{
	SceneControls::Constants myConstants;
	myConstants.expoTime = 0.6;
	//NB: remaining constants are left with their default values

	//creates a SceneControls object that survives end of life of this method
	SceneControls* sc = new SceneControls(myConstants);

	//this line demonstrates that SceneControls.constants are indeed constants because
	//any changes in source object since now do not affect the one inside *sc
	myConstants.expoTime = 0.7;

	return *sc;
}

class provideSceneControls_withOwnUpdater: public SceneControls
{
public:
	void updateControls(const float currTime) override
	{
		REPORT("my own special updater at time " << currTime);
	}
};

class provideSceneControls_withOwnConstantsAndUpdater: public SceneControls
{
public:
	provideSceneControls_withOwnConstantsAndUpdater(Constants& c): SceneControls(c) {}

	void updateControls(const float currTime) override
	{
		REPORT("my own special updater at time " << currTime);
	}
};


// ------------------- new scenario with various SceneControls -------------------
SCENARIO_DECLARATION_withDefOptSynthoscopy( testCreatingAndSettingScenario )

SceneControls& testCreatingAndSettingScenario::provideSceneControls()
{
	REPORT("providing some SceneControls");

	//default SceneControls incl. default constants
	//return DefaultSceneControls;

	//SceneControls with own values for the constants
	//return provideSceneControls_withOwnConstants();

	//SceneControls with own updating method
	//return *(new provideSceneControls_withOwnUpdater());

	//SceneControls with own values for the constants and the updating method
	SceneControls::Constants myConstants;
	myConstants.expoTime = 0.7;
	return *(new provideSceneControls_withOwnConstantsAndUpdater(myConstants));
}

void testCreatingAndSettingScenario::initializeScene()
{
	REPORT("argc=" << argc);
	if (argc > 1)
		REPORT("argv[1]=" << argv[1]);
}

void testCreatingAndSettingScenario::initializeAgents(FrontOfficer* fo, int fractionNumber,int noOfAllFractions)
{
	REPORT("preparing " << fractionNumber << "/" << noOfAllFractions);
}

/*
//enable this if SCENARIO_DECLARATION_withItsOwnSynthoscopy is used
void testCreatingAndSettingScenario::initializePhaseIIandIII()
{
	REPORT("what to say here?");
}

void testCreatingAndSettingScenario::doPhaseIIandIII()
{
	REPORT("what to say here?");
}
*/


// ------------------- new scenario with more user-side methods -------------------
SCENARIO_DECLARATION_withDefOptSynthoscopy( testBasicScenario )

SceneControls& testBasicScenario::provideSceneControls()
{ return DefaultSceneControls; }

void testBasicScenario::initializeScene()
{ REPORT("init Scene"); }

void testBasicScenario::initializeAgents(FrontOfficer* fo, int fractionNumber,int noOfAllFractions)
{ REPORT("add agents " << fractionNumber << "/" << noOfAllFractions); }

class testExtendedScenario: public testBasicScenario
{
public:
	void testWork()
	{
		REPORT("constants.incrTime = " << params.constants.incrTime);
		REPORT("producingMasks? = " << params.isProducingOutput(params.imgMask));
		REPORT("producingMasks? = " << params.imagesSaving_isEnabledForImgMask());
		//params.constants.expoTime = 10.0f; FAILS which is OKay
		params.enableProducingOutput(params.imgFinal);
	}
};


// ------------------- the main() -------------------
int main(int argc, char** argv)
{
	//Scenario* s = new testCreatingAndSettingScenario();
	testCreatingAndSettingScenario s;
	s.setArgs(argc,argv);

	std::cout << " preInit: incrTime = " << s.seeCurrentControls().constants.incrTime << "\n";
	std::cout << " preInit: expoTime = " << s.seeCurrentControls().constants.expoTime << "\n";
	s.initializeScene();
	s.updateScene(99);
	std::cout << "postInit: incrTime = " << s.seeCurrentControls().constants.incrTime << "\n";
	std::cout << "postInit: expoTime = " << s.seeCurrentControls().constants.expoTime << "\n";


	REPORT_NOHEADER("---------------------------------------");
	testExtendedScenario es;
	es.testWork();

	const SceneControls& es_sc = es.seeCurrentControls();
	REPORT("producingMasks? = " << es_sc.isProducingOutput(es_sc.imgMask));
}
