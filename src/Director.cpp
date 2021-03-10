#include <chrono>
#include <thread>
#include "util/Vector3d.h"
#include "util/synthoscopy/SNR.h"
#include "FrontOfficer.h"
#include "Director.h"

void Director::init1_SMP(void)
{
	REPORT("Direktor initializing now...");
	currTime = scenario.params.constants.initTime;

	//a bit of stats before we start...
	const auto& sSum = scenario.params.constants.sceneSize;
	Vector3d<float> sSpx(sSum);
	sSpx.elemMult(scenario.params.constants.imgRes);
	std::string sMsg = buildStringFromStream(
	     "scenario suggests this scene size: "
	  << sSum.x << " x " << sSum.y << " x " << sSum.z
	  << " um -> "
	  << sSpx.x << " x " << sSpx.y << " x " << sSpx.z << " px");

	double sSpxTotal = sSpx.x * sSpx.y * sSpx.z;
	if (sSpxTotal > 1024.0*1024.0*1024.0)
	{
		sSpxTotal /= 1024.0*1024.0*1024.0;
		int sSpxTotal_int  = (int)sSpxTotal;
		int sSpxTotal_frac = (int)(sSpxTotal*10.0) %10;
		REPORT(sMsg << " (" << sSpxTotal_int << "." << sSpxTotal_frac << " " << "Gvoxels)");
	}
	else
	if (sSpxTotal > 1024.0*1024.0)
	{
		sSpxTotal /= 1024.0*1024.0;
		int sSpxTotal_int  = (int)sSpxTotal;
		int sSpxTotal_frac = (int)(sSpxTotal*10.0) %10;
		REPORT(sMsg << " (" << sSpxTotal_int << "." << sSpxTotal_frac << " " << "Mvoxels)");
	}
	else
	if (sSpxTotal > 1024.0)
	{
		sSpxTotal /= 1024.0;
		int sSpxTotal_int  = (int)sSpxTotal;
		int sSpxTotal_frac = (int)(sSpxTotal*10.0) %10;
		REPORT(sMsg << " (" << sSpxTotal_int << "." << sSpxTotal_frac << " " << "Kvoxels)");
	}
	else
	{
		REPORT(sMsg << " (" << sSpxTotal << " voxels)");
	}

	scenario.initializeScene();
	scenario.initializePhaseIIandIII();

	//"reminder" test
	if (scenario.params.imagesSaving_isEnabledForImgPhantom()
	 || scenario.params.imagesSaving_isEnabledForImgOptics())
	{
		if (! scenario.params.imagesSaving_isEnabledForImgOptics())
		{
			REPORT("===> Found enabled phantom images, but disabled optics images.");
			REPORT("===> Will actually not render agents until both is enabled.");
		}
		if (! scenario.params.imagesSaving_isEnabledForImgPhantom())
		{
			REPORT("===> Found enabled optics images, but disabled phantom images.");
			REPORT("===> Will actually not render agents until both is enabled.");
		}
	}
}

void Director::init2_SMP(void)
{
	prepareForUpdateAndPublishAgents();
	waitHereUntilEveryoneIsHereToo();

	updateAndPublishAgents();
	waitHereUntilEveryoneIsHereToo();
	postprocessAfterUpdateAndPublishAgents();

	reportSituation();
	REPORT("Direktor initialized");
}

void Director::init3_SMP(void)
{
	//NB: this method is here only for the cosmetic purposes:
	//    just wanted that in the SMP case the rendering happens
	//    as the very last operation of the entire init phase

	//will block itself until the full rendering is complete
#ifndef DISTRIBUTED
	renderNextFrame();
#endif
}


void Director::reportSituation()
{
	REPORT("--------------- " << currTime << " min ("
	  << agents.size() << " in the entire world) ---------------");
}


void Director::close(void)
{
	//mark before closing is attempted...
	isProperlyClosedFlag = true;
	DEBUG_REPORT("running the closing sequence");

	//TODO: should close/kill the service thread too

	//close tracks of all agents
	for (auto ag : agents)
	{
		//CTC logging?
		if ( tracks.isTrackFollowed(ag.first)      //was part of logging?
		&&  !tracks.isTrackClosed(ag.first) )      //wasn't closed yet?
			tracks.closeTrack(ag.first,frameCnt-1);
	}

	tracks.exportAllToFile("tracks.txt");
	DEBUG_REPORT("tracks.txt was saved...");
}


void Director::execute(void)
{
	REPORT("Direktor has just started the simulation");

	const float stopTime = scenario.params.constants.stopTime;
	const float incrTime = scenario.params.constants.incrTime;
	const float expoTime = scenario.params.constants.expoTime;

	//run the simulation rounds, one after another one
	while (currTime < stopTime)
	{
		//one simulation round is happening here,
		//will this one end with rendering?
		willRenderNextFrameFlag = currTime+incrTime >= (float)frameCnt*expoTime;

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
#ifdef DEBUG
		reportAgentsAllocation();
#endif
#endif

		// is this the right time to export data?
		if (willRenderNextFrameFlag)
		{
			//will block itself until the full rendering is complete
			renderNextFrame();
		}

		//this was promised to happen after every simulation round is over
#ifndef DISTRIBUTED
		FO->executeEndSub2();
#endif
		scenario.updateScene( currTime );
		waitHereUntilEveryoneIsHereToo();
	}
}


void Director::prepareForUpdateAndPublishAgents()
{
	//empty because Direktor is not (yet) managing spatial info
	//about all agents in the simulation

#ifndef DISTRIBUTED
	FO->prepareForUpdateAndPublishAgents();
#endif
}

void Director::updateAndPublishAgents()
{
#ifdef DEBUG
	auto time = tic();
	auto agentsTime = tic();
#endif
	//since the last call of this method, we have:
	//list of agents that were active after the last call in 'agents'
	//subset of these that became inactive in 'deadAgents'
	//list of newly created in 'newAgents'

	//we essentially only update our "maps"
	//so that we reliably know "who is living where",
	//there will be no (network) traffic now because all necessary
	//notifications had been transferred during the recent simulation

	//remove dead agents from both lists
	auto ag = deadAgents.begin();
	while (ag != deadAgents.end())
	{
		//remove if agentID matches
		agents.remove_if([ag](std::pair<int,int> p){ return p.first == ag->first; });
		ag = deadAgents.erase(ag);
	}

	//move new agents between both lists
	agents.splice(agents.begin(), newAgents);
	//
	DEBUG_REPORT("(dead)new agents are now (de)registered, took " << toc(agentsTime));

	//now tell the FOs to start interchanging AABBs of their active agents:
	//notice that every FO should be "prepared" for this since all
	//must have finished executing their prepareForUpdateAndPublishAgents()

	//notify the first FO to start broadcasting fresh agents' AABBs
	//this starts the round-robin-chain between FOs
	notify_publishAgentsAABBs(firstFOsID);

	//wait until we're notified from the last FO
	//that his broadcasting is over
	waitFor_publishAgentsAABBs();

#ifdef DISTRIBUTED
	//now that we were assured that all FOs have broadcast their AABBs,
	//but the messages sent by the last FO might still being processed
	//by some FOs and so wait here a bit and then ask FO, one by one,
	//to tell us how many AABBs it currently has
	DEBUG_REPORT("Waiting 1 second before checking all FOs...");
	std::this_thread::sleep_for((std::chrono::milliseconds)1000);
#endif

	//all FOs except myself (assuming i=0 addresses the Direktor)
#ifndef DISTRIBUTED
	for (int i = 1; i <= FOsCount; ++i)
	{
		if (request_CntOfAABBs(i) != agents.size())
			throw ERROR_REPORT("FO #" << i << " does not have a complete list of AABBs");
	}
#endif
#ifdef DEBUG
	REPORT(toc(time));
#endif
}

void Director::postprocessAfterUpdateAndPublishAgents()
{
	//currently empty

#ifndef DISTRIBUTED
	FO->postprocessAfterUpdateAndPublishAgents();
#endif
}


int Director::getNextAvailAgentID()
{
	return ++lastUsedAgentID;
}


void Director::startNewAgent(const int agentID,
                             const int associatedFO,
                             const bool wantsToAppearInCTCtracksTXTfile)
{
	//register the agent for adding into the system
	newAgents.emplace_back(agentID,associatedFO);

	//CTC logging?
	if (wantsToAppearInCTCtracksTXTfile)
		tracks.startNewTrack(agentID, frameCnt);
}


void Director::closeAgent(const int agentID,
                          const int associatedFO)
{
	//register the agent for removing from the system
	deadAgents.emplace_back(agentID,associatedFO);

	//CTC logging?
	if (tracks.isTrackFollowed(agentID))
		tracks.closeTrack(agentID, frameCnt-1);
}


void Director::startNewDaughterAgent(const int childID, const int parentID)
{
	//CTC logging: also add the parental link
	tracks.updateParentalLink(childID, parentID);
}


int Director::getFOsIDofAgent(const int agentID)
{
	auto ag = agents.begin();
	while (ag != agents.end())
	{
		if (ag->first == agentID) return ag->second;
		++ag;
	}

	throw ERROR_REPORT("Couldn't find a record about agent " << agentID);
}

void Director::setAgentsDetailedDrawingMode(const int agentID, const bool state)
{
	int FO = getFOsIDofAgent(agentID);
	notify_setDetailedDrawingMode(FO,agentID,state);
}

void Director::setAgentsDetailedReportingMode(const int agentID, const bool state)
{
	int FO = getFOsIDofAgent(agentID);
	notify_setDetailedReportingMode(FO,agentID,state);
}

void Director::setSimulationDebugRendering(const bool state)
{
	renderingDebug = state;
	broadcast_setRenderingDebug(state);
}


void Director::renderNextFrame()
{
	REPORT("Rendering time point " << frameCnt);
	SceneControls& sc = scenario.params;

	// ----------- OUTPUT EVENTS -----------
	//prepare the output images
	sc.imgMask.GetVoxelData()    = 0;
	sc.imgPhantom.GetVoxelData() = 0;
	sc.imgOptics.GetVoxelData()  = 0;

	// --------- the big round robin scheme ---------
	//start the round...:
	//this essentially sends out the empty images, and leaves them empty!
	request_renderNextFrame(firstFOsID);

	/* IF THE INITIAL IMAGES WERE NOT ZERO, HERE THEY MUST BE ZEROED
	//clear the output images
	sc.imgMask.GetVoxelData()    = 0;
	sc.imgPhantom.GetVoxelData() = 0;
	sc.imgOptics.GetVoxelData()  = 0;
	*/

	//WAIT HERE UNTIL WE GET THE IMAGES BACK
	//this essentially pours images from network into local images,
	//which must be for sure zero beforehand
	//this will block...
	waitFor_renderNextFrame(firstFOsID);

	//save the images
	static char fn[1024];
	if (sc.isProducingOutput(sc.imgMask))
	{
		sprintf(fn,sc.constants.imgMask_filenameTemplate,frameCnt);
		REPORT("Saving " << fn << ", hold on...");
		sc.imgMask.SaveImage(fn);

		sc.displayChannel_transferImgMask();
	}

	if (sc.isProducingOutput(sc.imgPhantom))
	{
		sprintf(fn,sc.constants.imgPhantom_filenameTemplate,frameCnt);
		REPORT("Saving " << fn << ", hold on...");
		sc.imgPhantom.SaveImage(fn);

		sc.displayChannel_transferImgPhantom();
	}

	if (sc.isProducingOutput(sc.imgOptics))
	{
		sprintf(fn,sc.constants.imgOptics_filenameTemplate,frameCnt);
		REPORT("Saving " << fn << ", hold on...");
		sc.imgOptics.SaveImage(fn);

		sc.displayChannel_transferImgOptics();
	}

	if (sc.isProducingOutput(sc.imgFinal))
	{
		sprintf(fn,sc.constants.imgFinal_filenameTemplate,frameCnt);
		REPORT("Creating " << fn << ", hold on...");
		scenario.doPhaseIIandIII();
		REPORT("Saving " << fn << ", hold on...");
		sc.imgFinal.SaveImage(fn);

		sc.displayChannel_transferImgFinal();

		if (sc.isProducingOutput(sc.imgMask)) mitogen::ComputeSNR(sc.imgFinal,sc.imgMask);
	}

	++frameCnt;

	// ----------- INPUT EVENTS -----------
	//if std::cin is closed permanently, wait here a couple of milliseconds
	//to prevent zeroMQ from flooding the Scenery
	if (std::cin.eof())
	{
		REPORT("waiting " << shallWaitForSimViewer_millis << " msec to give SimViewer some time to breath...")
		std::this_thread::sleep_for((std::chrono::milliseconds)shallWaitForSimViewer_millis);
		return;
	}

	//finish here, if user should not be prompted
	if (!shallWaitForUserPromptFlag) return;

	//wait for key...
	char key;
	do {
		//read the key
		REPORT_NOENDL("Waiting for a key [and press Enter], try 'H' 'Enter': ");
		std::cin >> key;

		//memorize original key for the inspections handling
		const char ivwKey = key;

		//some known action?
		switch (key) {
		case 'Q':
			throw ERROR_REPORT("User requested exit.");

		case 'H':
			//print summary of commands (and their keys)
			REPORT("key summary:");
			REPORT("Q - quits the program");
			REPORT("H - prints this summary");
			REPORT("E - no operation, just an empty command");
			REPORT("D - toggles whether agents' drawForDebug() is called");
			REPORT("I - toggles console (reporting) inspection of selected agents");
			REPORT("V - toggles visual inspection (in SimViewer) of selected agents");
			REPORT("W - toggles console and visual inspection of selected agents");
			REPORT("P - define a breath-in-breath-out delay before another rendering");
			REPORT("Ctrl+D - closes this input leaving the program without any breaks...");
			break;

		case 'E':
			//empty action... just a key press eater when user gets lots in the commanding scheme
			REPORT("No action taken");
			break;

		case 'D':
			renderingDebug ^= true;
			setSimulationDebugRendering(renderingDebug);
			REPORT("Debug rendering toggled to: " << (renderingDebug? "enabled" : "disabled"));
			break;

		case 'I':
		case 'V':
		case 'W':
			//inspection command(s) is followed by cell sub-command and agent ID(s)
			REPORT("Entering Inspection toggle mode: press 'e' or 'o' or '1' and agent ID to  enable its inspection");
			REPORT("Entering Inspection toggle mode: press      any key      and agent ID to disable its inspection");
			REPORT("Entering Inspection toggle mode: press  any key and 'E'     to leave the Inspection toggle mode");
			while (key != 'X')
			{
				std::cin >> key;
				bool state = (key == 'e' || key == 'o' || key == '1');

				int id;
				std::cin >> id;

				if (std::cin.good())
				{
					key = 'y'; //to detect if some agent has been modified
					for (auto c : agents)
					if (c.first == id)
					{
						if (ivwKey != 'I') setAgentsDetailedDrawingMode(c.first,state);
						if (ivwKey != 'V') setAgentsDetailedReportingMode(c.first,state);
						REPORT((ivwKey != 'I' ? "vizu " : "")
						    << (ivwKey != 'V' ? "console " : "")
						    << "inspection" << (ivwKey == 'W'? "s " : " ")
						    << (state ? "enabled" : "disabled") << " for ID = " << id);
						key = 'Y'; //signal we got here
					}

					if (key == 'y')
						REPORT("no agent ID = " << id << " found");
				}
				else
				{
					key = 'X';          //prevent from entering this loop again (but not the outter one!)
					std::cin.clear();   //prevent from giving up reading
				}
				//NB: key is now either 'y' or 'Y', or 'X'
			}
			REPORT("Leaving Inspection toggle mode");
			break;

		case 'P':
			REPORT("Enter now the delay period in miliseconds and then press Enter");
			float time; //better to read as float (and convert to int)
			std::cin >> time;
			if (time < 0)
			{
				REPORT("Expected positive value...");
				time = 0;
			}
			shallWaitForSimViewer_millis = (size_t)time;
			REPORT("Setting the delay to " << shallWaitForSimViewer_millis << " msec");
			key = 1;            //forces the prompt to come back
			break;

		default:
			//signal not-recognized action -> which means "do next simulation batch"
			key = 0;
		}
	}
	while (key != 0);
}


void Director::reportAgentsAllocation()
{
	REPORT("I now recognize these agents:");
	for (const auto& AgFO : agents)
		REPORT("agent ID " << AgFO.first << " at FO #" << AgFO.second);
}
