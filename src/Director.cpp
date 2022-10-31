#include "Director.hpp"
#include "../extern/hpc_datastore_cpp/src/hpc_ds_api.hpp"
#include "../extern/timer/timer.hpp"
#include "FrontOfficer.hpp"
#include "util/Vector3d.hpp"
#include "util/synthoscopy/SNR.hpp"
#include <chrono>
#include <cmath>
#include <fmt/core.h>
#include <thread>

Director::~Director() {
	report::debugMessage(fmt::format("running the closing sequence"));

	// TODO: should close/kill the service thread too

	// close tracks of all agents
	for (auto [id, _] : agents) {
		// CTC logging?
		if (tracks.isTrackFollowed(id)    // was part of logging?
		    && !tracks.isTrackClosed(id)) // wasn't closed yet?
			tracks.closeTrack(id, frameCnt - 1);
	}

	tracks.exportAllToFile("tracks.txt");
	report::debugMessage(fmt::format("tracks.txt was saved..."));
}

int Director::getFOsIDofAgent(int agentID) const {
	auto it = agents.find(agentID);
	if (it != agents.end())
		return it->second;

	throw report::rtError(
	    fmt::format("Couldn't find a record about agent {}", agentID));
}

bool Director::willRenderNextFrame() const { return willRenderNextFrameFlag; }
bool Director::willWaitForUserPrompt() const {
	return shallWaitForUserPromptFlag;
}
void Director::setWaitForUserPrompt(bool state) {
	shallWaitForUserPromptFlag = state;
}

void Director::setAgentsDetailedDrawingMode(const int agentID,
                                            const bool state) const {
	int FO = getFOsIDofAgent(agentID);
	notify_setDetailedDrawingMode(FO, agentID, state);
}

void Director::setAgentsDetailedReportingMode(const int agentID,
                                              const bool state) const {
	int FO = getFOsIDofAgent(agentID);
	notify_setDetailedReportingMode(FO, agentID, state);
}

void Director::setSimulationDebugRendering(const bool state) {
	renderingDebug = state;
	broadcast_setRenderingDebug(state);
}

void Director::reportSituation() const {
	report::message(fmt::format(
	    "--------------- {} min ({} in the entire world) ---------------",
	    currTime, agents.size()));
}

void Director::reportAgentsAllocation() const {
	report::message(fmt::format("I now recognize these agents:"));
	for (auto [id, FO] : agents)
		report::message(fmt::format("agent ID {} at FO #{}", id, FO));
}

void Director::init1() {
	report::message(fmt::format("Direktor initializing now..."));
	currTime = scenario->params->constants.initTime;

	// a bit of stats before we start...
	const auto& sSum = scenario->params->constants.sceneSize;
	Vector3d<float> sSpx(sSum);
	sSpx.elemMult(scenario->params->constants.imgRes);
	std::string sMsg = fmt::format(
	    "scenario suggests this scene size: {} x {} x {} um -> {} x {} x {} px",
	    sSum.x, sSum.y, sSum.z, sSpx.x, sSpx.y, sSpx.z);

	double sSpxTotal = sSpx.x * sSpx.y * sSpx.z;
	if (sSpxTotal > std::pow(1024.0, 3)) {
		report::message(fmt::format("{} ({:.2f} Gvoxels)", sMsg,
		                            sSpxTotal / std::pow(1024.0, 3)));
	} else if (std::pow(1024.0, 2)) {
		report::message(fmt::format("{} ({:.2f} Mvoxels)", sMsg,
		                            sSpxTotal / std::pow(1024.0, 2)));
	} else if (sSpxTotal > 1024.0) {
		report::message(
		    fmt::format("{} ({:.2f} Kvoxels)", sMsg, sSpxTotal / 1024.0));
	} else {
		report::message(fmt::format("{} ({} voxels)", sMsg, sSpxTotal));
	}

	scenario->initializeScene();
	scenario->initializePhaseIIandIII();

	//"reminder" test
	if (scenario->params->imagesSaving_isEnabledForImgPhantom() ||
	    scenario->params->imagesSaving_isEnabledForImgOptics()) {
		if (!scenario->params->imagesSaving_isEnabledForImgOptics()) {
			report::message(fmt::format("===> Found enabled phantom images, "
			                            "but disabled optics images."));
			report::message(fmt::format(
			    "===> Will actually not render agents until both is enabled."));
		}
		if (!scenario->params->imagesSaving_isEnabledForImgPhantom()) {
			report::message(fmt::format("===> Found enabled optics images, but "
			                            "disabled phantom images."));
			report::message(fmt::format(
			    "===> Will actually not render agents until both is enabled."));
		}
	}
}

void Director::init2() {
	prepareForUpdateAndPublishAgents();

	updateAndPublishAgents();

	postprocessAfterUpdateAndPublishAgents();
	reportSituation();

	report::message(fmt::format("Direktor initialized"));
}

void Director::init3() {
	// NB: this method is here only for the cosmetic purposes:
	//     just wanted that in the SMP case the rendering happens
	//     as the very last operation of the entire init phase

	// will block itself until the full rendering is complete
	renderNextFrame();
}

void Director::_execute() {
	report::message(fmt::format("Direktor has just started the simulation"));

	Timer.set_output("bm_times.txt");

	const float stopTime = scenario->params->constants.stopTime;
	const float incrTime = scenario->params->constants.incrTime;
	const float expoTime = scenario->params->constants.expoTime;

	// run the simulation rounds, one after another one
	while (currTime < stopTime) {
		// one simulation round is happening here,
		// will this one end with rendering?
		willRenderNextFrameFlag =
		    currTime + incrTime >= float(frameCnt) * expoTime;

		Timer.start("executeInternals");
		broadcast_executeInternals();
		waitHereUntilEveryoneIsHereToo();
		Timer.end("executeInternals");

		Timer.start("updateAndPublishAgents1");
		prepareForUpdateAndPublishAgents();
		updateAndPublishAgents();
		postprocessAfterUpdateAndPublishAgents();
		Timer.end("updateAndPublishAgents1");

		Timer.start("executeExternals");
		broadcast_executeExternals();
		waitHereUntilEveryoneIsHereToo();
		Timer.end("executeExternals");

		Timer.start("updateAndPublishAgents2");
		prepareForUpdateAndPublishAgents();
		updateAndPublishAgents();
		postprocessAfterUpdateAndPublishAgents();
		Timer.end("updateAndPublishAgents2");

		// move to the next simulation time point
		broadcast_executeEndSub1();

		currTime += incrTime;

#ifndef NDEBUG
		reportAgentsAllocation();
#endif

		// is this the right time to export data?
		if (willRenderNextFrameFlag) {
			Timer.start("renderNextFrame");
			// will block itself until the full rendering is complete
			renderNextFrame();
			// Resume simulation (FOs are waiting for user to finish input)
			waitHereUntilEveryoneIsHereToo();
			Timer.end("renderNextFrame");
		}

		// this was promised to happen after every simulation round is over
		broadcast_executeEndSub2();

		scenario->updateScene(currTime);
		waitHereUntilEveryoneIsHereToo();
	}
}

void Director::updateAndPublishAgents() {
	// since the last call of this method, we have:
	// list of agents that were active after the last call in 'agents'
	// subset of these that became inactive in 'deadAgents'
	// list of newly created in 'newAgents'

	// we essentially only update our "maps"
	// so that we reliably know "who is living where"

	// Remove closed agents
	auto closedAgents = request_closedAgents();
	std::set<int> to_remove;
	for (std::size_t fo = 1; fo < closedAgents.size(); ++fo)
		for (auto id : closedAgents[fo]) {
			to_remove.insert(id);
			// CTC logging?
			if (tracks.isTrackFollowed(id))
				tracks.closeTrack(id, frameCnt - 1);
		}

	std::erase_if(agents, [&](auto p) { return to_remove.contains(p.first); });

	// Fetch started agents
	auto startedAgents = request_startedAgents();
	for (std::size_t fo = 1; fo < startedAgents.size(); ++fo)
		for (auto [id, track] : startedAgents[fo]) {
#ifndef NDEBUG
			if (agents.contains(id))
				throw report::rtError(
				    fmt::format("Agent: {} already exists", id));

#endif
			agents[id] = int(fo);

			// CTC logging?
			if (track)
				tracks.startNewTrack(id, frameCnt);
		}

	// Update parental links
	auto parentalLinksUpdates = request_parentalLinksUpdates();
	for (std::size_t fo = 1; fo < parentalLinksUpdates.size(); ++fo)
		for (auto [childID, parentID] : parentalLinksUpdates[fo])
			tracks.updateParentalLink(childID, parentID);

	// now tell the FOs to start interchanging AABBs of their active agents:
	// notice that every FO should be "prepared" for this since all
	// must have finished executing their prepareForUpdateAndPublishAgents()

	// notify the first FO to start broadcasting fresh agents' AABBs
	// this starts the round-robin-chain between FOs
	notify_publishAgentsAABBs();

	// wait until we're notified from the last FO
	// that his broadcasting is over
	waitFor_publishAgentsAABBs();

#ifndef NDEBUG
	// all FOs except myself (assuming i=0 addresses the Direktor)
	auto counts = request_CntsOfAABBs();
	for (int fo = 1; fo <= getFOsCount(); ++fo) {
		if (counts[fo] != agents.size())
			throw report::rtError(fmt::format(
			    "FO #{} does not have a complete list of AABBs", fo));
	}
#endif
}

void Director::renderNextFrame() {
	report::message(fmt::format("Rendering time point {}", frameCnt));
	SceneControls& sc = *scenario->params;

	// ----------- OUTPUT EVENTS -----------
	// prepare the output images
	sc.imgMask.SetAllVoxels(0);
	sc.imgPhantom.SetAllVoxels(0);
	sc.imgOptics.SetAllVoxels(0);

	// Request filling of local images with scene images
	request_renderNextFrame();

	// WAIT HERE UNTIL WE GET THE IMAGES BACK
	// this essentially pours images from network into local images,
	// which must be for sure zero beforehand
	// this will block...
	waitFor_renderNextFrame();

	// save the images
	ds::Connection ds_conn(sc.constants.ds_serverAddress, sc.constants.ds_port,
	                       sc.constants.ds_datasetUUID);

	// SAVE MASK
	if (sc.isProducingOutput(sc.imgMask)) {
		auto fn = fmt::format(sc.constants.imgMask_filenameTemplate, frameCnt);

		if (sc.constants.outputToTiff) {
			report::message(fmt::format("Saving {}, hold on...", fn));
			sc.imgMask.SaveImage(fn.c_str());
		}

		if (sc.constants.outputToDatastore) {
			report::message(fmt::format(
			    "Sending timepoint {} to datastore, hold on...", frameCnt));
			ds_conn.write_with_pyramids(sc.imgMask, sc.constants.ds_maskChannel,
			                            frameCnt, 0, "latest",
			                            ds::SamplingMode::NEAREST_NEIGHBOUR);
		}

		sc.displayChannel_transferImgMask();
	}

	// SAVE PHANTOM
	if (sc.isProducingOutput(sc.imgPhantom)) {
		auto fn =
		    fmt::format(sc.constants.imgPhantom_filenameTemplate, frameCnt);

		if (sc.constants.outputToTiff) {
			report::message(fmt::format("Saving {}, hold on...", fn));
			sc.imgPhantom.SaveImage(fn.c_str());
		}

		if (sc.constants.outputToDatastore) {
			report::message(fmt::format(
			    "Sending timepoint {} to datastore, hold on...", frameCnt));
			ds_conn.write_with_pyramids(
			    sc.imgPhantom, sc.constants.ds_phantomChannel, frameCnt, 0,
			    "latest", ds::SamplingMode::NEAREST_NEIGHBOUR);
		}

		sc.displayChannel_transferImgPhantom();
	}

	// SAVE OPTICS
	if (sc.isProducingOutput(sc.imgOptics)) {
		auto fn =
		    fmt::format(sc.constants.imgOptics_filenameTemplate, frameCnt);

		if (sc.constants.outputToTiff) {
			report::message(fmt::format("Saving {}, hold on...", fn));
			sc.imgOptics.SaveImage(fn.c_str());
		}

		if (sc.constants.outputToDatastore) {
			report::message(fmt::format(
			    "Sending timepoint {} to datastore, hold on...", frameCnt));
			ds_conn.write_with_pyramids(
			    sc.imgOptics, sc.constants.ds_opticsChannel, frameCnt, 0,
			    "latest", ds::SamplingMode::NEAREST_NEIGHBOUR);
		}

		sc.displayChannel_transferImgOptics();
	}

	// SAVE FINAL
	if (sc.isProducingOutput(sc.imgFinal)) {
		auto fn = fmt::format(sc.constants.imgFinal_filenameTemplate, frameCnt);
		report::message(fmt::format("Creating {}, hold on...", fn));
		scenario->doPhaseIIandIII();

		if (sc.constants.outputToTiff) {
			report::message(fmt::format("Saving {}, hold on...", fn));
			sc.imgFinal.SaveImage(fn.c_str());
		}

		if (sc.constants.outputToDatastore) {
			report::message(fmt::format(
			    "Sending timepoint {} to datastore, hold on...", frameCnt));
			ds_conn.write_with_pyramids(
			    sc.imgFinal, sc.constants.ds_finalChannel, frameCnt, 0,
			    "latest", ds::SamplingMode::NEAREST_NEIGHBOUR);
		}

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
			renderingDebug = !renderingDebug;
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