#pragma once

#include "FrontOfficer.hpp"
#include "Scenarios/common/Scenario.hpp"
#include "TrackRecord_CTC.hpp"
#include "util/report.hpp"
#include <functional>
#include <memory>
#include <utility>

/** has access to Scenario, to reach its doPhaseIIandIII() */
class Director {
  public:
	Director(std::function<ScenarioUPTR()> scenarioFactory);

	Director(const Director&) = delete;
	Director& operator=(const Director&) = delete;

	Director(Director&&) = default;
	Director& operator=(Director&&) = default;
	~Director();

	// ==================== Implemented in Director.cpp ====================

	/** returns the ID of FO to which a given agent is associated to */
	int getFOsIDofAgent(int agentID) const;

	/** returns the state of the 'willRenderNextFrameFlag', that is if the
	    current simulation round with end up with the call to renderNextFrame()
	 */
	bool willRenderNextFrame() const;

	/** returns the state of the 'shallWaitForUserPromptFlag', that is if an
	    user will be prompted (and the simulation would stop and wait) at
	    the end of the renderNextFrame() */
	bool willWaitForUserPrompt() const;

	/** sets the 'shallWaitForUserPromptFlag', choosing, whether
	   renderNextFrame() should prompt the user (and stop the simulation and
	   wait... and also become "command-able") */
	void setWaitForUserPrompt(bool state);

	/** notifies the agent to enable/disable its detailed drawing routines */
	void setAgentsDetailedDrawingMode(const int agentID,
	                                  const bool state) const;

	/** notifies the agent to enable/disable its detailed reporting routines */
	void setAgentsDetailedReportingMode(const int agentID,
	                                    const bool state) const;

	/** sets the Director::renderingDebug flag */
	void setSimulationDebugRendering(const bool state);

	// -------------- debug --------------
	void reportSituation() const;
	void reportAgentsAllocation() const;

	// ======= Implemented in SimulationControl/Director*.cpp methods =======
	/** Signal to initialize simulation */
	void init();

	/** Singal to execute simulation */
	void execute();

	int getFOsCount() const;

  private:
	// ==================== Implemented in Director.cpp ====================
	/** stage 1/3 to do: scene heavy inits and synthoscopy warm up */
	void init1();
	/** stage 2/3 to do: scene heavy inits and synthoscopy warm up */
	void init2();
	/** stage 3/3 to do: renders the first frame */
	void init3();

	/** does the simulation loops, i.e. triggers calls of AbstractAgent's
	 * methods in the right order */
	void _execute();

	/** register the new agents, unregister the dead agents;
	    distribute the new and old existing agents to the sites */
	void updateAndPublishAgents();

	/** Asks all agents to render and raster their state into displayUnit and
	 * the images */
	void renderNextFrame();

	// ======= Implemented in SimulationControl/Director*.cpp methods =======

	/** housekeeping before the AABBs exchange takes place */
	void prepareForUpdateAndPublishAgents() const;

	/** housekeeping after the complete AABBs exchange took place,
	    "complete" means that _all_ FOs have broadcast all they wanted */
	void postprocessAfterUpdateAndPublishAgents() const;

	void waitHereUntilEveryoneIsHereToo() const;
	void notify_publishAgentsAABBs() const;
	void waitFor_publishAgentsAABBs() const;

	std::vector<std::size_t> request_CntsOfAABBs() const;
	std::vector<std::vector<std::pair<int, bool>>>
	request_startedAgents() const;
	std::vector<std::vector<int>> request_closedAgents() const;
	std::vector<std::vector<std::pair<int, int>>>
	request_parentalLinksUpdates() const;

	void
	notify_setDetailedDrawingMode(int FOsID, int agentID, bool state) const;
	void
	notify_setDetailedReportingMode(int FOsID, int agentID, bool state) const;

	void request_renderNextFrame() const;
	void waitFor_renderNextFrame() const;

	void broadcast_setRenderingDebug(bool setFlagToThis) const;
	void broadcast_executeInternals() const;
	void broadcast_executeExternals() const;
	void broadcast_executeEndSub1() const;
	void broadcast_executeEndSub2() const;

	// ==================== private variables ====================

	ScenarioUPTR scenario;

	/** flag whether an user will be prompted (and the simulation would stop
	    and wait) at the end of the renderNextFrame(); this attribute actually
	    shadows/overlays over the scenario.params.shallWaitForUserPromptFlag */
	bool& shallWaitForUserPromptFlag;

	/** map of all agents currently active in the simulation,
	    maps between agent ID and FO ID associated with this agent,
	    the main purpose of this map is to know which FO to ask when
	    detailed geometry (in a form of the ShadowAgent) is needed */
	std::map<int, int> agents;

	/** structure to hold durations of tracks and the mother-daughter relations
	 */
	TrackRecords_CTC tracks;

	/** current global simulation time [min] */
	float currTime = 0.0f;

	/** counter of exports/snapshots, used to numerate frames and output image
	 * files */
	int frameCnt = 0;

	/** flag if the renderNextFrame() will be called after this simulation round
	 */
	bool willRenderNextFrameFlag = false;

	/** time period (in msecs) the simulator waits in
	Director::renderNextFrame() when std. input is closed (after Ctrl+D)
	before it proceeds with the next round of the simulation (and next round
	of rendering eventually) */
	size_t shallWaitForSimViewer_millis = 1000;

	/** Flags if agents' drawForDebug() should be called with every
	 * this->renderNextFrame() */
	bool renderingDebug = false;

	/** Pointer to (yet) unknown data for implementation purposes
	 * (unique_ptr cannot point to void... => shared_ptr) **/
	std::shared_ptr<void> implementationData = nullptr;
};