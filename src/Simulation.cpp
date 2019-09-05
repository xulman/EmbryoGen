#include "Simulation.h"

// -----------------------------------------------------------------------------
void Simulation::init(void)
{
	initializeScenario();
	updateAndPublishAgents();
	REPORT("--------------- " << currTime << " min ("
	  << agents.size() << " local and "
	  << shadowAgents.size() << " shadow agents) ---------------");

	renderNextFrame();
}


/** does the simulation loops, i.e. calls AbstractAgent's methods in the right order */
void Simulation::execute(void)
{
	//run the simulation rounds, one after another one
	while (currTime < stopTime)
	{
		//one simulation round is happening here,
		//will this one end with rendering?
		willRenderNextFrameFlag = currTime+incrTime >= (float)frameCnt*expoTime;

		//after this simulation round is done, all agents should
		//reach local times greater than this global time
		const float futureTime = currTime + incrTime -0.0001f;

		//develop (willingly) new shapes... (can run in parallel),
		//the agents' (external at least!) geometries must not change during this phase
		std::list<AbstractAgent*>::iterator c=agents.begin();
		for (; c != agents.end(); c++)
		{
			(*c)->advanceAndBuildIntForces(futureTime);
#ifdef DEBUG
			if ((*c)->getLocalTime() < futureTime)
				throw ERROR_REPORT("Agent is not synchronized.");
#endif
		}

		//propagate current internal geometries to the exported ones... (can run in parallel)
		c=agents.begin();
		for (; c != agents.end(); c++)
		{
			(*c)->adjustGeometryByIntForces();
			(*c)->publishGeometry();
		}

		updateAndPublishAgents();

		//react (unwillingly) to the new geometries... (can run in parallel),
		//the agents' (external at least!) geometries must not change during this phase
		c=agents.begin();
		for (; c != agents.end(); c++)
		{
			(*c)->collectExtForces();
		}

		//propagate current internal geometries to the exported ones... (can run in parallel)
		c=agents.begin();
		for (; c != agents.end(); c++)
		{
			(*c)->adjustGeometryByExtForces();
			(*c)->publishGeometry();
		}

		updateAndPublishAgents();

		//overlap reports:
		DEBUG_REPORT("max overlap: " << overlapMax
		        << ", avg overlap: " << (overlapSubmissionsCounter > 0 ? overlapAvg/float(overlapSubmissionsCounter) : 0.f)
		        << ", cnt of overlaps: " << overlapSubmissionsCounter);
		overlapMax = overlapAvg = 0.f;
		overlapSubmissionsCounter = 0;

		// move to the next simulation time point
		currTime += incrTime;
		REPORT("--------------- " << currTime << " min ("
		  << agents.size() << " local and "
		  << shadowAgents.size() << " shadow agents) ---------------");

		// is this the right time to export data?
		if (willRenderNextFrameFlag) renderNextFrame();
	}
}


/** frees simulation agents, writes the tracks.txt file */
void Simulation::close(void)
{
	//mark before closing is attempted...
	simulationProperlyClosedFlag = true;

	//delete all agents... also from newAgents & deadAgents, note that the
	//same agent may exist on the agents and deadAgents lists simultaneously
	std::list<AbstractAgent*>::iterator iter=agents.begin();
	while (iter != agents.end())
	{
		//CTC logging?
		if ( tracks.isTrackFollowed((*iter)->ID)      //was part of logging?
		&&  !tracks.isTrackClosed((*iter)->ID) )      //wasn't closed yet?
			tracks.closeTrack((*iter)->ID,frameCnt-1);

		//check and possibly remove from the deadAgents list
		auto daIt = deadAgents.begin();
		while (daIt != deadAgents.end() && *daIt != *iter) ++daIt;
		if (daIt != deadAgents.end())
		{
			DEBUG_REPORT("removing from deadAgents duplicate ID " << (*daIt)->ID);
			deadAgents.erase(daIt);
		}

		delete *iter; *iter = NULL;
		iter++;
	}

	tracks.exportAllToFile("tracks.txt");
	DEBUG_REPORT("tracks.txt was saved...");

	//now remove what's left on newAgents and deadAgents
	DEBUG_REPORT("will remove " << newAgents.size() << " and " << deadAgents.size()
	          << " agents from newAgents and deadAgents, respectively");
	while (!newAgents.empty())
	{
		delete newAgents.front();
		newAgents.pop_front();
	}
	while (!deadAgents.empty())
	{
		delete deadAgents.front();
		deadAgents.pop_front();
	}
}


// -----------------------------------------------------------------------------
void Simulation::getNearbyAgents(const ShadowAgent* const fromSA,
                     const float maxDist,
                     std::list<const ShadowAgent*>& l)
{
	//cache fromSA's bounding box
	const AxisAlignedBoundingBox& fromAABB = fromSA->getAABB();
	const float maxDist2 = maxDist*maxDist;

	//examine all full (local) agents
	for (std::list<AbstractAgent*>::const_iterator
	     c=agents.begin(); c != agents.end(); c++)
	{
		//don't evaluate against itself
		if (*c == fromSA) continue;

		//close enough?
		if (fromAABB.minDistance((*c)->getAABB()) < maxDist2)
			l.push_back(*c);
	}

	//examine all shadow (outside) agents
	for (std::list<ShadowAgent*>::const_iterator
	     c=shadowAgents.begin(); c != shadowAgents.end(); c++)
	{
		//close enough?
		if (fromAABB.minDistance((*c)->getAABB()) < maxDist2)
			l.push_back(*c);
	}
}


// -----------------------------------------------------------------------------
void Simulation::renderNextFrame(void)
{
	// ----------- OUTPUT EVENTS -----------
	//clear the output images
	imgMask.GetVoxelData()    = 0;
	imgPhantom.GetVoxelData() = 0;
	imgOptics.GetVoxelData()  = 0;

	//go over all cells, and render them
	std::list<AbstractAgent*>::const_iterator c=agents.begin();
	for (; c != agents.end(); c++)
	{
		//displayUnit should always exists in some form
		(*c)->drawTexture(displayUnit);
		(*c)->drawMask(displayUnit);
		if (renderingDebug)
			(*c)->drawForDebug(displayUnit);

		//raster images may not necessarily always exist,
		//always check for their availability first:
		if (isProducingOutput(imgPhantom) && isProducingOutput(imgOptics))
		{
			(*c)->drawTexture(imgPhantom,imgOptics);
		}
		if (isProducingOutput(imgMask))
		{
			(*c)->drawMask(imgMask);
			if (renderingDebug)
				(*c)->drawForDebug(imgMask); //TODO, should go into its own separate image
		}
	}

	//render the current frame
	displayUnit.Flush(); //make sure all drawings are sent before the "tick"
	displayUnit.Tick( ("Time: "+std::to_string(currTime)).c_str() );
	displayUnit.Flush(); //make sure the "tick" is sent right away too

	//save the images
	static char fn[1024];
	if (isProducingOutput(imgMask))
	{
		sprintf(fn,"mask%03d.tif",frameCnt);
		REPORT("Saving " << fn << ", hold on...");
		imgMask.SaveImage(fn);
	}

	if (isProducingOutput(imgPhantom))
	{
		sprintf(fn,"phantom%03d.tif",frameCnt);
		REPORT("Saving " << fn << ", hold on...");
		imgPhantom.SaveImage(fn);
	}

	if (isProducingOutput(imgOptics))
	{
		sprintf(fn,"optics%03d.tif",frameCnt);
		REPORT("Saving " << fn << ", hold on...");
		imgOptics.SaveImage(fn);
	}

	if (isProducingOutput(imgFinal))
	{
		sprintf(fn,"finalPreview%03d.tif",frameCnt);
		REPORT("Creating " << fn << ", hold on...");
		doPhaseIIandIII();
		REPORT("Saving " << fn << ", hold on...");
		imgFinal.SaveImage(fn);

		if (isProducingOutput(imgMask)) mitogen::ComputeSNR(imgFinal,imgMask);
	}

	++frameCnt;

	// ----------- INPUT EVENTS -----------
	//if std::cin is closed permanently, wait here a couple of milliseconds
	//to prevent zeroMQ from flooding the Scenery
	if (std::cin.eof())
	{
		REPORT("waiting 1000 ms to give SimViewer some time to breath...")
		std::this_thread::sleep_for((std::chrono::milliseconds)1000);
		return;
	}

	//finish up here, if user should not be prompted
	if (!shallWaitForUserPromptFlag) return;

	//wait for key...
	char key;
	do {
		//read the key
		REPORT_NOENDL("Waiting for a key [and press Enter]: ");
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
			break;

		case 'E':
			//empty action... just a key press eater when user gets lots in the commanding scheme
			REPORT("No action taken");
			break;

		case 'D':
			renderingDebug ^= true;
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
					if (c->ID == id)
					{
						if (ivwKey != 'I') c->setDetailedDrawingMode(state);
						if (ivwKey != 'V') c->setDetailedReportingMode(state);
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

		default:
			//signal not-recognized action -> which means "do next simulation batch"
			key = 0;
		}
	}
	while (key != 0);
}


// -----------------------------------------------------------------------------
#if defined ENABLE_MITOGEN_FINALPREVIEW
  #include "util/synthoscopy/finalpreview.h"
#elif defined ENABLE_FILOGEN_PHASEIIandIII
  #include "util/synthoscopy/FiloGen_VM.h"
#endif
#ifndef ENABLE_FILOGEN_REALPSF
  #include <i3d/filters.h>
#endif

void Simulation::doPhaseIIandIII(void)
{
#if defined ENABLE_MITOGEN_FINALPREVIEW
	REPORT("using default MitoGen synthoscopy");
	mitogen::PrepareFinalPreviewImage(imgPhantom,imgFinal);

#elif defined ENABLE_FILOGEN_PHASEIIandIII
	REPORT("using default FiloGen synthoscopy");
	//
	// phase II
#ifdef ENABLE_FILOGEN_REALPSF
	filogen::PhaseII(imgPhantom, imgPSF);
#else
	const float xySigma = 0.6f; //can also be 0.9
	const float  zSigma = 1.8f; //can also be 2.7
	DEBUG_REPORT("fake PSF is used for PhaseII, with sigmas: "
		<< xySigma * imgPhantom.GetResolution().GetX() << " x "
		<< xySigma * imgPhantom.GetResolution().GetY() << " x "
		<<  zSigma * imgPhantom.GetResolution().GetZ() << " pixels");
	i3d::GaussIIR<float>(imgPhantom,
		xySigma * imgPhantom.GetResolution().GetX(),
		xySigma * imgPhantom.GetResolution().GetY(),
		 zSigma * imgPhantom.GetResolution().GetZ());
#endif
	//
	// phase III
	filogen::PhaseIII(imgPhantom, imgFinal);

#else
	REPORT("WARNING: Empty function, no synthoscopy is going on.");
#endif
}
