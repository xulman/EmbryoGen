#include "Director.hpp"
#include "FrontOfficer.hpp"
#include "util/Vector3d.hpp"
#include "util/synthoscopy/SNR.hpp"
#include <chrono>
#include <fmt/core.h>
#include <thread>
#include <unordered_set>

void Director::init1_SMP(void) {
	report::message(fmt::format("Direktor initializing now..."));
	currTime = scenario.params->constants.initTime;

	// a bit of stats before we start...
	const auto& sSum = scenario.params->constants.sceneSize;
	Vector3d<float> sSpx(sSum);
	sSpx.elemMult(scenario.params->constants.imgRes);
	std::string sMsg = fmt::format(
	    "scenario suggests this scene size: {} x {} x {} um -> {} x {} x {} px",
	    sSum.x, sSum.y, sSum.z, sSpx.x, sSpx.y, sSpx.z);

	double sSpxTotal = sSpx.x * sSpx.y * sSpx.z;
	if (sSpxTotal > 1024.0 * 1024.0 * 1024.0) {
		sSpxTotal /= 1024.0 * 1024.0 * 1024.0;
		int sSpxTotal_int = (int)sSpxTotal;
		int sSpxTotal_frac = (int)(sSpxTotal * 10.0) % 10;
		report::message(fmt::format("{} ({}.{} Gvoxels)", sMsg, sSpxTotal_int,
		                            sSpxTotal_frac));
	} else if (sSpxTotal > 1024.0 * 1024.0) {
		sSpxTotal /= 1024.0 * 1024.0;
		int sSpxTotal_int = (int)sSpxTotal;
		int sSpxTotal_frac = (int)(sSpxTotal * 10.0) % 10;
		report::message(fmt::format("{} ({}.{} Mvoxels)", sMsg, sSpxTotal_int,
		                            sSpxTotal_frac));
	} else if (sSpxTotal > 1024.0) {
		sSpxTotal /= 1024.0;
		int sSpxTotal_int = (int)sSpxTotal;
		int sSpxTotal_frac = (int)(sSpxTotal * 10.0) % 10;
		report::message(fmt::format("{} ({}.{} Kvoxels)", sMsg, sSpxTotal_int,
		                            sSpxTotal_frac));
	} else {
		report::message(fmt::format("{} ({} voxels)", sMsg, sSpxTotal));
	}

	scenario.initializeScene();
	scenario.initializePhaseIIandIII();

	//"reminder" test
	if (scenario.params->imagesSaving_isEnabledForImgPhantom() ||
	    scenario.params->imagesSaving_isEnabledForImgOptics()) {
		if (!scenario.params->imagesSaving_isEnabledForImgOptics()) {
			report::message(fmt::format("===> Found enabled phantom images, "
			                            "but disabled optics images."));
			report::message(fmt::format(
			    "===> Will actually not render agents until both is enabled."));
		}
		if (!scenario.params->imagesSaving_isEnabledForImgPhantom()) {
			report::message(fmt::format("===> Found enabled optics images, but "
			                            "disabled phantom images."));
			report::message(fmt::format(
			    "===> Will actually not render agents until both is enabled."));
		}
	}
}

void Director::init2_SMP(void) {
	prepareForUpdateAndPublishAgents();
	waitHereUntilEveryoneIsHereToo();

	updateAndPublishAgents();
	waitHereUntilEveryoneIsHereToo();
	postprocessAfterUpdateAndPublishAgents();

	reportSituation();
	report::message(fmt::format("Direktor initialized"));
}

void Director::init3_SMP(void) {
	// NB: this method is here only for the cosmetic purposes:
	//     just wanted that in the SMP case the rendering happens
	//     as the very last operation of the entire init phase

	// will block itself until the full rendering is complete
#ifndef DISTRIBUTED
	renderNextFrame();
#endif
}

void Director::reportSituation() {
	report::message(fmt::format(
	    "--------------- {} min ({} in the entire world) ---------------",
	    currTime, agents.size()));
}

void Director::close(void) {
	// mark before closing is attempted...
	isProperlyClosedFlag = true;
	report::debugMessage(fmt::format("running the closing sequence"));

	// TODO: should close/kill the service thread too

	// close tracks of all agents
	for (auto ag : agents) {
		// CTC logging?
		if (tracks.isTrackFollowed(ag.first)    // was part of logging?
		    && !tracks.isTrackClosed(ag.first)) // wasn't closed yet?
			tracks.closeTrack(ag.first, frameCnt - 1);
	}

	tracks.exportAllToFile("tracks.txt");
	report::debugMessage(fmt::format("tracks.txt was saved..."));
}

void Director::execute(void) {
	report::message(fmt::format("Direktor has just started the simulation"));

	const float stopTime = scenario.params->constants.stopTime;
	const float incrTime = scenario.params->constants.incrTime;
	const float expoTime = scenario.params->constants.expoTime;

	// run the simulation rounds, one after another one
	while (currTime < stopTime) {
		// one simulation round is happening here,
		// will this one end with rendering?
		willRenderNextFrameFlag =
		    currTime + incrTime >= (float)frameCnt * expoTime;

#ifndef DISTRIBUTED
		FO->willRenderNextFrameFlag = this->willRenderNextFrameFlag;
		FO->executeInternals();
#endif
		waitHereUntilEveryoneIsHereToo();

		prepareForUpdateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();

		updateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();
		postprocessAfterUpdateAndPublishAgents();

#ifndef DISTRIBUTED
		FO->executeExternals();
#endif
		waitHereUntilEveryoneIsHereToo();

		prepareForUpdateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();

		updateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();
		postprocessAfterUpdateAndPublishAgents();

		// move to the next simulation time point
#ifndef DISTRIBUTED
		FO->executeEndSub1();
#endif
		currTime += incrTime;
#ifdef DISTRIBUTED
		reportSituation();
#ifndef NDEBUG
		reportAgentsAllocation();
#endif
#endif

		// is this the right time to export data?
		if (willRenderNextFrameFlag) {
			// will block itself until the full rendering is complete
			renderNextFrame();
		}

		// this was promised to happen after every simulation round is over
#ifndef DISTRIBUTED
		FO->executeEndSub2();
#endif
		scenario.updateScene(currTime);
		waitHereUntilEveryoneIsHereToo();
	}
}

void Director::prepareForUpdateAndPublishAgents() {
	// empty because Direktor is not (yet) managing spatial info
	// about all agents in the simulation

#ifndef DISTRIBUTED
	FO->prepareForUpdateAndPublishAgents();
#endif
}

void Director::updateAndPublishAgents() {
	// since the last call of this method, we have:
	// list of agents that were active after the last call in 'agents'
	// subset of these that became inactive in 'deadAgents'
	// list of newly created in 'newAgents'

	// we essentially only update our "maps"
	// so that we reliably know "who is living where",
	// there will be no (network) traffic now because all necessary
	// notifications had been transferred during the recent simulation

	// remove dead agents from both lists
	std::unordered_set<int> to_remove;
	for (auto [id, _] : deadAgents)
		to_remove.insert(id);

	agents.remove_if(
	    [&](std::pair<int, int> p) { return to_remove.contains(p.first); });
	deadAgents.clear();

	// move new agents between both lists
	agents.splice(agents.begin(), newAgents);

	// now tell the FOs to start interchanging AABBs of their active agents:
	// notice that every FO should be "prepared" for this since all
	// must have finished executing their prepareForUpdateAndPublishAgents()

	// notify the first FO to start broadcasting fresh agents' AABBs
	// this starts the round-robin-chain between FOs
	notify_publishAgentsAABBs(firstFOsID);

	// wait until we're notified from the last FO
	// that his broadcasting is over
	waitFor_publishAgentsAABBs();

#ifdef DISTRIBUTED
	// now that we were assured that all FOs have broadcast their AABBs,
	// but the messages sent by the last FO might still being processed
	// by some FOs and so wait here a bit and then ask FO, one by one,
	// to tell us how many AABBs it currently has
	report::debugMessage(
	    fmt::format("Waiting 1 second before checking all FOs..."));
	std::this_thread::sleep_for((std::chrono::milliseconds)1000);
#endif

	// all FOs except myself (assuming i=0 addresses the Direktor)
#ifndef DISTRIBUTED
	for (int i = 1; i <= FOsCount; ++i) {
		if (request_CntOfAABBs(i) != agents.size())
			throw report::rtError(fmt::format(
			    "FO #{} does not have a complete list of AABBs", i));
	}
#endif
}

void Director::postprocessAfterUpdateAndPublishAgents() {
	// currently empty

#ifndef DISTRIBUTED
	FO->postprocessAfterUpdateAndPublishAgents();
#endif
}

int Director::getNextAvailAgentID() { return ++lastUsedAgentID; }

void Director::startNewAgent(const int agentID,
                             const int associatedFO,
                             const bool wantsToAppearInCTCtracksTXTfile) {
	// register the agent for adding into the system
	newAgents.emplace_back(agentID, associatedFO);

	// CTC logging?
	if (wantsToAppearInCTCtracksTXTfile)
		tracks.startNewTrack(agentID, frameCnt);
}

void Director::closeAgent(const int agentID, const int associatedFO) {
	// register the agent for removing from the system
	deadAgents.emplace_back(agentID, associatedFO);

	// CTC logging?
	if (tracks.isTrackFollowed(agentID))
		tracks.closeTrack(agentID, frameCnt - 1);
}

void Director::startNewDaughterAgent(const int childID, const int parentID) {
	// CTC logging: also add the parental link
	tracks.updateParentalLink(childID, parentID);
}

int Director::getFOsIDofAgent(const int agentID) {
	auto ag = agents.begin();
	while (ag != agents.end()) {
		if (ag->first == agentID)
			return ag->second;
		++ag;
	}

	throw report::rtError(
	    fmt::format("Couldn't find a record about agent {}", agentID));
}

void Director::setAgentsDetailedDrawingMode(const int agentID,
                                            const bool state) {
	int FO = getFOsIDofAgent(agentID);
	notify_setDetailedDrawingMode(FO, agentID, state);
}

void Director::setAgentsDetailedReportingMode(const int agentID,
                                              const bool state) {
	int FO = getFOsIDofAgent(agentID);
	notify_setDetailedReportingMode(FO, agentID, state);
}

void Director::setSimulationDebugRendering(const bool state) {
	renderingDebug = state;
	broadcast_setRenderingDebug(state);
}

void Director::renderNextFrame() {
	report::message(fmt::format("Rendering time point {}", frameCnt));
	SceneControls& sc = *scenario.params;

	// ----------- OUTPUT EVENTS -----------
	// prepare the output images
	sc.imgMask.GetVoxelData() = 0;
	sc.imgPhantom.GetVoxelData() = 0;
	sc.imgOptics.GetVoxelData() = 0;

	// --------- the big round robin scheme ---------
	// start the round...:
	// this essentially sends out the empty images, and leaves them empty!
	request_renderNextFrame(firstFOsID);

	/* IF THE INITIAL IMAGES WERE NOT ZERO, HERE THEY MUST BE ZEROED
	//clear the output images
	sc.imgMask.GetVoxelData()    = 0;
	sc.imgPhantom.GetVoxelData() = 0;
	sc.imgOptics.GetVoxelData()  = 0;
	*/

	// WAIT HERE UNTIL WE GET THE IMAGES BACK
	// this essentially pours images from network into local images,
	// which must be for sure zero beforehand
	// this will block...
	waitFor_renderNextFrame(firstFOsID);

	// save the images
	static char fn[1024];
	if (sc.isProducingOutput(sc.imgMask)) {
		sprintf(fn, sc.constants.imgMask_filenameTemplate, frameCnt);
		report::message(fmt::format("Saving {}, hold on...", fn));
		sc.imgMask.SaveImage(fn);

		sc.displayChannel_transferImgMask();
	}

	if (sc.isProducingOutput(sc.imgPhantom)) {
		sprintf(fn, sc.constants.imgPhantom_filenameTemplate, frameCnt);
		report::message(fmt::format("Saving {}, hold on...", fn));
		sc.imgPhantom.SaveImage(fn);

		sc.displayChannel_transferImgPhantom();
	}

	if (sc.isProducingOutput(sc.imgOptics)) {
		sprintf(fn, sc.constants.imgOptics_filenameTemplate, frameCnt);
		report::message(fmt::format("Saving {}, hold on...", fn));
		sc.imgOptics.SaveImage(fn);

		sc.displayChannel_transferImgOptics();
	}

	if (sc.isProducingOutput(sc.imgFinal)) {
		sprintf(fn, sc.constants.imgFinal_filenameTemplate, frameCnt);
		report::message(fmt::format("Creating {}, hold on...", fn));
		scenario.doPhaseIIandIII();
		report::message(fmt::format("Saving {}, hold on...", fn));
		sc.imgFinal.SaveImage(fn);

		sc.displayChannel_transferImgFinal();

		if (sc.isProducingOutput(sc.imgMask))
			mitogen::ComputeSNR(sc.imgFinal, sc.imgMask);
	}

	++frameCnt;

	// ----------- INPUT EVENTS -----------
	// if std::cin is closed permanently, wait here a couple of milliseconds
	// to prevent zeroMQ from flooding the Scenery
	if (std::cin.eof()) {
		report::message(fmt::format(
		    "waiting {} msec to give SimViewer some time to breath...",
		    shallWaitForSimViewer_millis));
		std::this_thread::sleep_for(
		    (std::chrono::milliseconds)shallWaitForSimViewer_millis);
		return;
	}

	// finish here, if user should not be prompted
	if (!shallWaitForUserPromptFlag)
		return;

	// wait for key...
	char key;
	do {
		// read the key
		report::message(
		    fmt::format(
		        "Waiting for a key [and press Enter], try 'H' 'Enter': "),
		    {true, false});
		std::cin >> key;

		// memorize original key for the inspections handling
		const char ivwKey = key;

		// some known action?
		switch (key) {
		case 'Q':
			throw report::rtError("User requested exit.");

		case 'H':
			// print summary of commands (and their keys)
			report::message(fmt::format("key summary:"));
			report::message(fmt::format("Q - quits the program"));
			report::message(fmt::format("H - prints this summary"));
			report::message(
			    fmt::format("E - no operation, just an empty command"));
			report::message(fmt::format(
			    "D - toggles whether agents' drawForDebug() is called"));
			report::message(fmt::format("I - toggles console (reporting) "
			                            "inspection of selected agents"));
			report::message(fmt::format("V - toggles visual inspection (in "
			                            "SimViewer) of selected agents"));
			report::message(fmt::format("W - toggles console and visual "
			                            "inspection of selected agents"));
			report::message(fmt::format("P - define a breath-in-breath-out "
			                            "delay before another rendering"));
			report::message(fmt::format("Ctrl+D - closes this input leaving "
			                            "the program without any breaks..."));
			break;

		case 'E':
			// empty action... just a key press eater when user gets lots in the
			// commanding scheme
			report::message(fmt::format("No action taken"));
			break;

		case 'D':
			renderingDebug ^= true;
			setSimulationDebugRendering(renderingDebug);
			report::message(
			    fmt::format("Debug rendering toggled to: {}",
			                (renderingDebug ? "enabled" : "disabled")));
			break;

		case 'I':
		case 'V':
		case 'W':
			// inspection command(s) is followed by cell sub-command and agent
			// ID(s)
			report::message(
			    fmt::format("Entering Inspection toggle mode: press 'e' or 'o' "
			                "or '1' and agent ID to  enable its inspection"));
			report::message(
			    fmt::format("Entering Inspection toggle mode: press      any "
			                "key      and agent ID to disable its inspection"));
			report::message(
			    fmt::format("Entering Inspection toggle mode: press  any key "
			                "and 'E'     to leave the Inspection toggle mode"));
			while (key != 'X') {
				std::cin >> key;
				bool state = (key == 'e' || key == 'o' || key == '1');

				int id;
				std::cin >> id;

				if (std::cin.good()) {
					key = 'y'; // to detect if some agent has been modified
					for (auto c : agents)
						if (c.first == id) {
							if (ivwKey != 'I')
								setAgentsDetailedDrawingMode(c.first, state);
							if (ivwKey != 'V')
								setAgentsDetailedReportingMode(c.first, state);
							report::message(fmt::format(
							    "{}{}inspection{}{} for ID = {}",
							    (ivwKey != 'I' ? "vizu " : ""),
							    (ivwKey != 'V' ? "console " : ""),
							    (ivwKey == 'W' ? "s " : " "),
							    (state ? "enabled" : "disabled"), id));
							key = 'Y'; // signal we got here
						}

					if (key == 'y')
						report::message(
						    fmt::format("no agent ID = {} found", id));
				} else {
					key = 'X'; // prevent from entering this loop again (but not
					           // the outter one!)
					std::cin.clear(); // prevent from giving up reading
				}
				// NB: key is now either 'y' or 'Y', or 'X'
			}
			report::message(fmt::format("Leaving Inspection toggle mode"));
			break;

		case 'P':
			report::message(fmt::format("Enter now the delay period in "
			                            "miliseconds and then press Enter"));
			float time; // better to read as float (and convert to int)
			std::cin >> time;
			if (time < 0) {
				report::message(fmt::format("Expected positive value..."));
				time = 0;
			}
			shallWaitForSimViewer_millis = (size_t)time;
			report::message(fmt::format("Setting the delay to {} msec",
			                            shallWaitForSimViewer_millis));
			key = 1; // forces the prompt to come back
			break;

		default:
			// signal not-recognized action -> which means "do next simulation
			// batch"
			key = 0;
		}
	} while (key != 0);
}

void Director::reportAgentsAllocation() {
	report::message(fmt::format("I now recognize these agents:"));
	for (const auto& AgFO : agents)
		report::message(
		    fmt::format("agent ID {} at FO #{}", AgFO.first, AgFO.second));
}
