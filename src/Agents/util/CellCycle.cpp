#include <cmath>
#include "../../util/rnd_generators.h"
#include "CellCycle.h"

float CellCycle::RandomizeCellCycleDuration(const float refDuration, const float spreadFactor)
{
	return ( GetRandomGauss(refDuration, spreadFactor * refDuration) );
}


/** (debug) report of the internal setting */
void CellCycle::reportPhaseDurations(void) const
{
	REPORT("full cycle duration is: " << fullCycleDuration << " minutes ("
	       << fullCycleDuration/60 << " hrs)");

	float phaseTotalTime = 0;
	for (int i=G1Phase; i <= Cytokinesis; ++i)
		phaseTotalTime += phaseDurations[i];
	REPORT("all cycles duration is: " << phaseTotalTime << " minutes");

	REPORT("  G1Phase    : " << phaseDurations[G1Phase    ] << " mins\t ("
	       << phaseDurations[G1Phase    ]/fullCycleDuration << "%)");
	REPORT("  SPhase     : " << phaseDurations[SPhase     ] << " mins\t ("
	       << phaseDurations[SPhase     ]/fullCycleDuration << "%)");
	REPORT("  G2Phase    : " << phaseDurations[G2Phase    ] << " mins\t ("
	       << phaseDurations[G2Phase    ]/fullCycleDuration << "%)");
	REPORT("  Prophase   : " << phaseDurations[Prophase   ] << " mins\t ("
	       << phaseDurations[Prophase   ]/fullCycleDuration << "%)");
	REPORT("  Metaphase  : " << phaseDurations[Metaphase  ] << " mins\t ("
	       << phaseDurations[Metaphase  ]/fullCycleDuration << "%)");
	REPORT("  Anaphase   : " << phaseDurations[Anaphase   ] << " mins\t ("
	       << phaseDurations[Anaphase   ]/fullCycleDuration << "%)");
	REPORT("  Telophase  : " << phaseDurations[Telophase  ] << " mins\t ("
	       << phaseDurations[Telophase  ]/fullCycleDuration << "%)");
	REPORT("  Cytokinesis: " << phaseDurations[Cytokinesis] << " mins\t ("
	       << phaseDurations[Cytokinesis]/fullCycleDuration << "%)");
}


void CellCycle::startCycling(const float currentGlobalTime)
{
	if (curPhase != newBorn)
		throw ERROR_REPORT("Re-initializing already initialized cell cycle");

	//define the phase durations
	determinePhaseDurations();
	//NB: overriding determinePhaseDurations() are not-visible in the c'tor,
	//    hence, this call is postponed into startCycling(), and this method
	//    was made unavoidable as it is the only that provides the global time

	lastPhaseChangeGlobalTime = currentGlobalTime;
	curPhase = G1Phase;
	startG1Phase();
}

/** this one calls the phase_methods (see docs of this class) in the correct order,
    until the cycle progresses to the given "wall time" (in 'currentGlobalTime'),
    it makes sure that no run_phase_method is called with zero progress ratio */
void CellCycle::triggerCycleMethods(const float currentGlobalTime)
{
	if (curPhase == newBorn)
		throw ERROR_REPORT("Not yet fully initialized cell cycle; did you call startCycling()?");

	//beyond the cell cycle, do nothing
	if (curPhase == RestInPeace) return;

	bool tryNextPhase = true;
	while (tryNextPhase)
	{
		const float progress = phaseDurations[curPhase] != 0 ?
		/* normal phase: */    (currentGlobalTime - lastPhaseChangeGlobalTime) / phaseDurations[curPhase]
		/*  empty phase: */  : (currentGlobalTime == lastPhaseChangeGlobalTime ?
		   /* |- A: */           0
		   /*  \ B: */         : std::copysign(1.0f, currentGlobalTime-lastPhaseChangeGlobalTime));
		//A: the same behaviour as for "normal phase", i.e. prevention from re-running with time=0
		//B: returns +1 (OK state) or -1 (indication of a sanity check fail, same as for "normal phase")
		DEBUG_REPORT("progress=" << progress);

		if (progress < 0)
		{
			throw ERROR_REPORT("lastPhaseChange is ahead given currentGlobalTime (or phase duration is negative), quitting confusedly");
		}
		else if (progress == 0)
		{
			//there's really no progress required from us, just skip this call
			DEBUG_REPORT("skipping this call...");
			tryNextPhase = false;
		}
		else if (progress < 1)
		{
			//call normally as some progress is required
			switch (curPhase)
			{
			case G1Phase:
				runG1Phase(progress);
				break;

			case SPhase:
				runSPhase(progress);
				break;

			case G2Phase:
				runG2Phase(progress);
				break;

			case Prophase:
				runProphase(progress);
				break;

			case Metaphase:
				runMetaphase(progress);
				break;

			case Anaphase:
				runAnaphase(progress);
				break;

			case Telophase:
				runTelophase(progress);
				break;

			case Cytokinesis:
				runCytokinesis(progress);
				break;

			default:
				throw ERROR_REPORT("Unknown cycle state!");
			}

			//couldn't cross the phase boundaries, yet has done its portion of the work
			tryNextPhase = false;
		}
		else
		{
			//got over a boundary between phases, then
			//call normally to finish to reach 1.0, close, incr. curPhase, open
			switch (curPhase)
			{
			case G1Phase:
				runG1Phase(1.0);
				closeG1Phase();
				//
				lastPhaseChangeGlobalTime += phaseDurations[curPhase];
				curPhase = SPhase;
				startSPhase();
				break;

			case SPhase:
				runSPhase(1.0);
				closeSPhase();
				//
				lastPhaseChangeGlobalTime += phaseDurations[curPhase];
				curPhase = G2Phase;
				startG2Phase();
				break;

			case G2Phase:
				runG2Phase(1.0);
				closeG2Phase();
				//
				lastPhaseChangeGlobalTime += phaseDurations[curPhase];
				curPhase = Prophase;
				startProphase();
				break;

			case Prophase:
				runProphase(1.0);
				closeProphase();
				//
				lastPhaseChangeGlobalTime += phaseDurations[curPhase];
				curPhase = Metaphase;
				startMetaphase();
				break;

			case Metaphase:
				runMetaphase(1.0);
				closeMetaphase();
				//
				lastPhaseChangeGlobalTime += phaseDurations[curPhase];
				curPhase = Anaphase;
				startAnaphase();
				break;

			case Anaphase:
				runAnaphase(1.0);
				closeAnaphase();
				//
				lastPhaseChangeGlobalTime += phaseDurations[curPhase];
				curPhase = Telophase;
				startTelophase();
				break;

			case Telophase:
				runTelophase(1.0);
				closeTelophase();
				//
				lastPhaseChangeGlobalTime += phaseDurations[curPhase];
				curPhase = Cytokinesis;
				startCytokinesis();
				break;

			case Cytokinesis:
				runCytokinesis(1.0);
				closeCytokinesis();
				//
				lastPhaseChangeGlobalTime += phaseDurations[curPhase];
				curPhase = RestInPeace;
				tryNextPhase = false;
				break;

			default:
				throw ERROR_REPORT("Unknown cycle state!");
			}

			//it might, however, still not be the end of this:
			//if lastPhaseChangeGlobalTime is still "multiple" whole phases
			//beyond the given currentGlobalTime, we still need "multiple" iterations
		}
	} //end of while (tryNextPhase)
}    //end of the method triggerCycleMethods()
