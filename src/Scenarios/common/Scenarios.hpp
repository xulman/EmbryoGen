#pragma once

#include "Scenario.hpp"
#include <list>
#include <set>
#include <string>
#include <vector>
#include <memory>

// instead of the #include statement, the FrontOfficer type is only declared to
// exists, FrontOfficer's definition depends on Scenario and so we'd end up in a
// definitions loop
class FrontOfficer;

/** template to create a new scenario that has all the required API to allow it
    to be successfully plugged into the simulator, it uses the DEFAULT digital
    phantom to final image conversion routine, which is however disabled by
   default (and one has to enable it with disks.enableImgFinalTIFFs() called
   from this->initializeScene(), or with ctx().disks.enableImgFinalTIFFs()
   called from SceneControls::updateControls() */
#define SCENARIO_DECLARATION_withDefOptSynthoscopy(c)                          \
	class c : public Scenario {                                                \
	  public:                                                                  \
		c() : Scenario(provideSceneControls(this)){};                          \
		std::unique_ptr<SceneControls> provideSceneControls(Scenario* ctx) {   \
			auto controls = provideSceneControls();                            \
			controls->setCtx(ctx);                                             \
			return controls;                                                   \
		}                                                                      \
		std::unique_ptr<SceneControls> provideSceneControls() const;           \
		void initializeScene() override;                                       \
		void initializeAgents(FrontOfficer*, int, int) override;               \
	};

/** template to create a new scenario that has all the required API to allow it
    to be successfully plugged into the simulator, it provides OWN digital
    phantom to final image conversion routine, which is however disabled by
   default (and one has to enable it with disks.enableImgFinalTIFFs() called
   from this->initializeScene(), or with ctx().disks.enableImgFinalTIFFs()
   called from SceneControls::updateControls() */
#define SCENARIO_DECLARATION_withItsOwnSynthoscopy(c)                          \
	class c : public Scenario {                                                \
	  public:                                                                  \
		c() : Scenario(provideSceneControls(this)){};                          \
		std::unique_ptr<SceneControls> provideSceneControls(Scenario* ctx) {   \
			auto controls = provideSceneControls();                            \
			controls->setCtx(ctx);                                             \
			return controls;                                                   \
		}                                                                      \
		std::unique_ptr<SceneControls> provideSceneControls() const;           \
		void initializeScene() override;                                       \
		void initializeAgents(FrontOfficer*, int, int) override;               \
		void initializePhaseIIandIII(void) override;                           \
		void doPhaseIIandIII(void) override;                                   \
	};

//                                          regular class name
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_AFewAgents);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_DrosophilaRegular);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_DrosophilaRandom);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_pseudoDivision);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_modelledDivision);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_dragAndRotate);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_withCellCycle);
SCENARIO_DECLARATION_withItsOwnSynthoscopy(Scenario_withTexture);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_dragRotateAndTexture);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_phaseIIandIII);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_PerlinShowCase);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_Parallel);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_mpiDebug);
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_Tetris);
//  ---> ADD NEW SCENARIO HERE AND ALSO BELOW <---
SCENARIO_DECLARATION_withDefOptSynthoscopy(Scenario_oneAgent);

/** a boilerplate code to handle pairing of scenarios with their
    command line names, and to instantiate the appropriate scenario */
#define SCENARIO_MATCHING(n, c)                                                \
	availableScenarios.insert(n);                                              \
	if (!scenario && n == name) {                                              \
		report::debugMessage(                                                  \
		    fmt::format("Creating instance of scenario: {}", name));           \
		scenario = std::make_unique<c>();                                      \
	}

class Scenarios {
  private:
	/** list of system-recognized scenarios, actually simulations */
	std::set<std::string> availableScenarios;

	/** the scenario/simulation that is going to be used */
	std::string scenario_name;

	int scenario_argc;
	const char** scenario_argv;

	/** get new copy of scenario */
	std::unique_ptr<Scenario> getNewScenario(const std::string& name) {
		std::unique_ptr<Scenario> scenario;

		//                  nickname            regular class name
		SCENARIO_MATCHING("aFewAgents", Scenario_AFewAgents)
		SCENARIO_MATCHING("regularDrosophila", Scenario_DrosophilaRegular)
		SCENARIO_MATCHING("randomDrosophila", Scenario_DrosophilaRandom)
		SCENARIO_MATCHING("pseudoDivision", Scenario_pseudoDivision)
		SCENARIO_MATCHING("modelledDivision", Scenario_modelledDivision)
		SCENARIO_MATCHING("dragAndRotate", Scenario_dragAndRotate)
		SCENARIO_MATCHING("cellCycle", Scenario_withCellCycle)
		SCENARIO_MATCHING("fluoTexture", Scenario_withTexture)
		SCENARIO_MATCHING("dragFluoTexture", Scenario_dragRotateAndTexture)
		SCENARIO_MATCHING("synthoscopy", Scenario_phaseIIandIII)
		SCENARIO_MATCHING("PerlinShowCase", Scenario_PerlinShowCase)
		SCENARIO_MATCHING("parallel", Scenario_Parallel)
		SCENARIO_MATCHING("mpiDebug", Scenario_mpiDebug)
		SCENARIO_MATCHING("tetris", Scenario_Tetris)
		//  ---> ADD NEW SCENARIO HERE (and add definition *cpp file, re-run
		//  cmake!) <---
		SCENARIO_MATCHING("oneAgent", Scenario_oneAgent)

		return scenario;
	}

	void printAvailableScenarios() const {
		report::message("Currently known scenarios are: ", {false});
		for (const std::string& scenario_name : availableScenarios)
			report::message(scenario_name, {false});
		report::debugMessage("", {false});
	}

  public:
	/** find the right scenario to initialize the simulation with */
	Scenarios(int argc, const char** argv) {
		/** load available scenarios **/
		getNewScenario(std::string());

		if (argc == 1) {
			report::message(
			    fmt::format("Please run again and choose some of the "
			                "available scenarios, e.g. as"));
			report::message(fmt::format("{} regularDrosophila", argv[0]));
			printAvailableScenarios();
			throw report::rtError("Scenario not selected");
		} else {
			scenario_name = argv[1];
			if (!getNewScenario(scenario_name)) {
				report::message(
				    fmt::format("Couldn't match command-line scenario '{}' to "
				                "any known scenario.",
				                scenario_name));
				printAvailableScenarios();
				throw report::rtError("Unmatched scenario.");
			}
		}
		// pass the CLI params inside, to be
		// possibly considered by Scenario::initialize...() methods
		scenario_argc = argc;
		scenario_argv = argv;
		// ->setArgs(argc, argv);
	}

	ScenarioUPTR getScenario() {
		auto new_scenario = getNewScenario(scenario_name);
		new_scenario->setArgs(scenario_argc, scenario_argv);
		return new_scenario;
	}
};
