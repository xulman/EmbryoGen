#ifndef CELLCYCLE_H
#define CELLCYCLE_H

#include "../rnd_generators.h"

/** A datatype enumerating the particular phases of cell cycle */
typedef enum
{
	G1Phase=0,
	SPhase=1,
	G2Phase=2,
	Prophase=3,
	Metaphase=4,
	Anaphase=5,
	Telophase=6,
	Cytokinesis=7
} ListOfPhases;


/** A holder of simulation parameters relevant to a cell itself.
    Note that any (more specialized) cell can extend this class
    with attributes it needs to have... */
class CellCycleParams
{
public:
	/** the cell cycle length [min] */
	float cellCycleLength;

	/** the length of individual cell cycle phases [min] */
	float cellPhaseDuration[8];

	/** constructor with default settings */
	CellCycleParams(void)
	{
		//adapted for cell cycle length of 24 hours
		//params.cellCycleLength = 14.5*60; //[min]
		cellCycleLength = 30.f; //[min]
		determinePhaseDurations();
	}

	/** a randomizing! no-exact-copy constructor */
	CellCycleParams(const CellCycleParams& otherCellParams)
	{
		//a randomized copy of the cell cycle
		cellCycleLength = GetRandomGauss(cellCycleLength, 0.17f*cellCycleLength);
		determinePhaseDurations();
	}

private:
	void determinePhaseDurations(void)
	{
		cellPhaseDuration[G1Phase]    = 0.5f     * cellCycleLength;
		cellPhaseDuration[SPhase]     = 0.3f     * cellCycleLength;
		cellPhaseDuration[G2Phase]    = 0.15f    * cellCycleLength;
		cellPhaseDuration[Prophase]   = 0.0125f  * cellCycleLength;
		cellPhaseDuration[Metaphase]  = 0.0285f  * cellCycleLength;
		cellPhaseDuration[Anaphase]   = 0.0025f  * cellCycleLength;
		cellPhaseDuration[Telophase]  = 0.00325f * cellCycleLength;
		cellPhaseDuration[Cytokinesis]= 0.00325f * cellCycleLength;
		//                              --------
		//                      sums to 1.0f
	}
};
#endif
