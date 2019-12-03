#ifndef SCENARIOS_H
#define SCENARIOS_H

#include <list>
#include "Scenario.h"

//instead of the #include statement, the FrontOfficer type is only declared to exists,
//FrontOfficer's definition depends on Scenario and so we'd end up in a definitions loop
class FrontOfficer;

/** template to create a new scenario that has all the required API to allow it
    to be successfully plugged into the simulator, it uses the DEFAULT digital
    phantom to final image conversion routine, which is however disabled by default
    (and one has to enable it with this->params.enableProducingOutput()) called
    from this->initializeScene() */
#define SCENARIO_DECLARATION_withDefOptSynthoscopy(c) \
	class c: public Scenario {                                                 \
	public:                                                                    \
	c(): Scenario( provideSceneControls() ) {};                                \
	SceneControls& provideSceneControls();                                     \
	void initializeScene() override;                                           \
	void initializeAgents(FrontOfficer*,int,int) override; };

/** template to create a new scenario that has all the required API to allow it
    to be successfully plugged into the simulator, it provides OWN digital
    phantom to final image conversion routine, which is however disabled by default
    (and one has to enable it with this->params.enableProducingOutput()) called
    from this->initializeScene() */
#define SCENARIO_DECLARATION_withItsOwnSynthoscopy(c) \
	class c: public Scenario {                                                 \
	public:                                                                    \
	c(): Scenario( provideSceneControls() ) {};                                \
	SceneControls& provideSceneControls();                                     \
	void initializeScene() override;                                           \
	void initializeAgents(FrontOfficer*,int,int) override;                     \
	void initializePhaseIIandIII(void) override;                               \
	void doPhaseIIandIII(void) override; };

/** a boilerplate code to handle pairing of scenarios with their
    command line names, and to instantiate the appropriate scenario */
#define SCENARIO_MATCHING(n,c) \
	availableScenarios.emplace_back(std::string((n)));                         \
	if (argc > 1 && scenario == NULL &&                                        \
	    availableScenarios.back().find(argv[1]) != std::string::npos)          \
	{                                                                          \
		REPORT("Going to use the scenario: " << availableScenarios.back());     \
		scenario = new c();                                                     \
	}


//                                          regular class name
//restoreMe//SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_AFewAgents )
//restoreMe//SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_DrosophilaRegular )
//restoreMe//SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_DrosophilaRandom )
//restoreMe//SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_pseudoDivision )
//restoreMe//SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_dragAndRotate )
//restoreMe//SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_withCellCycle )
//restoreMe//SCENARIO_DECLARATION_withItsOwnSynthoscopy( Scenario_withTexture )
//restoreMe//SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_dragRotateAndTexture )
//restoreMe//SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_phaseIIandIII )
//restoreMe//SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_PerlinShowCase )
SCENARIO_DECLARATION_withDefOptSynthoscopy( Scenario_Parallel )
//  ---> ADD NEW SCENARIO HERE AND ALSO BELOW <---

class Scenarios
{
private:
	/** list of system-recognized scenarios, actually simulations */
	std::list<std::string> availableScenarios;

	/** the scenario/simulation that is going to be used */
	Scenario* scenario = NULL;

public:
	/** find the right scenario to initialize the simulation with */
	Scenarios(int argc, char** argv)
	{
		//                  nickname            regular class name
		//restoreMe//SCENARIO_MATCHING( "aFewAgents",        Scenario_AFewAgents )
		//restoreMe//SCENARIO_MATCHING( "regularDrosophila", Scenario_DrosophilaRegular )
		//restoreMe//SCENARIO_MATCHING( "randomDrosophila",  Scenario_DrosophilaRandom )
		//restoreMe//SCENARIO_MATCHING( "pseudoDivision",    Scenario_pseudoDivision )
		//restoreMe//SCENARIO_MATCHING( "dragAndRotate",     Scenario_dragAndRotate )
		//restoreMe//SCENARIO_MATCHING( "cellCycle",         Scenario_withCellCycle )
		//restoreMe//SCENARIO_MATCHING( "fluoTexture",       Scenario_withTexture )
		//restoreMe//SCENARIO_MATCHING( "dragFluoTexture",   Scenario_dragRotateAndTexture )
		//restoreMe//SCENARIO_MATCHING( "synthoscopy",       Scenario_phaseIIandIII )
		//restoreMe//SCENARIO_MATCHING( "PerlinShowCase",    Scenario_PerlinShowCase )
		SCENARIO_MATCHING( "parallel",          Scenario_Parallel )
		//  ---> ADD NEW SCENARIO HERE (and add definition *cpp file, re-run cmake!) <---

		if (scenario == NULL)
		{
			if (argc == 1)
			{
				REPORT("Please run again and choose some of the available scenarios, e.g. as");
				REPORT(argv[0] << " regularDrosophila");
			}
			else
				REPORT("Couldn't match command-line scenario '" << argv[1] << "' to any known scenario.");

			std::cout << "Currently known scenarios are: ";
			for (auto rs : availableScenarios)
				std::cout << rs << ", ";
			std::cout << "\n";

			throw ERROR_REPORT("Unmatched scenario.");
		}

		//pass the CLI params inside, to be
		//possibly considered by Scenario::initialize...() methods
		scenario->setArgs(argc,argv);
	}

	Scenario& getScenario(void)
	{
		return *scenario;
	}
};
#endif
