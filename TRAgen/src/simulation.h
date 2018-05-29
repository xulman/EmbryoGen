#ifndef SIMULATION_H
#define SIMULATION_H


/**
 * Just initializes all agents: positions, weights, etc.
 *
 * The \e mode tells what initial positions they should be.
 *
 * mode = 0 (default) aligned on a grid
 * other modes are not yet defined
 *
 * The function may possible open some files for reporting the progress.
 */
void initializeAgents(const int mode=0);

/**
 * Do one step of the simulation:
 *
 * - calculate all forces
 * - update the agent positions accordingly
 *
 * The function may add some lines to the files opened in the initializeAgents().
 */
void moveAgents(const float timeDelta);

/**
 * The function merely only closes files opened in the initializeAgents().
 */
void closeAgents(void);


/**
 * Updates the tracks structure with a cell divison event:
 * Mo - mother cell, DoA and DoB are daughters.
 */
void ReportNewBornDaughters(const int MoID,
	const int DoAID, const int DoBID);


#endif
