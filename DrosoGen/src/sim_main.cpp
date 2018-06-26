#include <list>
#include <map>
#include <fstream>
#include <i3d/image3d.h>

#include "tools.h"
#include "Vector3d.h"

#include "Agents/AbstractAgent.h"
#include "Agents/NucleusAgent.h"

#include "DisplayUnits/VoidDisplayUnit.h"
#include "DisplayUnits/ConsoleDisplayUnit.h"
#include "DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "DisplayUnits/BroadcasterDisplayUnit.h"

/** This class contains all simulation agents, scene and simulation
    parameters, and takes care of the iterations of the simulation */
class Simulation
{
private:
	//fixed parameters of the simulation:

	//CTC drosophila:
	//x,y,z res = 2.46306,2.46306,0.492369 px/um
	//embryo along x-axis
	//bbox size around the embryo: 480,220,220 um
	//bbox size around the embryo: 1180,540,110 px

	//set up the environment
	const Vector3d<float> sceneOffset; //[micrometer]
	const Vector3d<float> sceneSize;   //[micrometer]

	// --------------------------------------------------

	//original res
	//const Vector3d<float>  imgRes(2.46f,2.46f,0.49f); //[pixels per micrometer]
	//
	//my res
	const Vector3d<float>  imgRes;  //[pixels per micrometer]
	const Vector3d<size_t> imgSize; //[pixels]

	/*
	imgMaskFilename="Mask%05d.tif";
	imgPhantomFilename="Phantom%05d.tif";
	imgFluoFilename="FluoImg%05d.tif";
	tracksFilename="tracks.txt";
	*/

	//output image that will be iteratively re-rendered
	i3d::Image3d<i3d::GRAY16> img;

	//output display unit
	BroadcasterDisplayUnit displayUnit;

	//export counter
	int frameCnt = 0;

	// --------------------------------------------------

	/// current global simulation time [min]
	float currTime = 0.0f;

	/// increment of the current global simulation time, [min]
	/// represents the time step of one simulatino step
	const float incrTime = 0.1f;

	/// at what global time should the simulation stop [min]
	const float stopTime = 1.2f;

	/// export simulation status always after this amount of global time, [min]
	/// should be multiple of incrTime
	const float expoTime = 0.5f;

	// --------------------------------------------------

	//the list of all agents
	std::list<AbstractAgent*> agents;

	//list of agents to be removed
	std::list<AbstractAgent*> dead_agents;

	//the structure to hold all track data
	std::map<int,TrackRecord> tracks;

	// --------------------------------------------------

public:
	//only initializes the simulation parameters
	Simulation(void)
		: sceneOffset(0.f),             //[micrometer]
		  sceneSize(480.f,220.f,220.f), //[micrometer]
		  imgRes(2.0f,2.0f,2.0f),       //[pixels per micrometer]
		  imgSize((size_t)ceil(sceneSize.x * imgRes.x), //[pixels]
                (size_t)ceil(sceneSize.y * imgRes.y),
                (size_t)ceil(sceneSize.z * imgRes.z))
	{
		REPORT("scene size: "
		  << sceneSize.x << " x " << sceneSize.y << " x " << sceneSize.z
		  << " um =  "
		  << imgSize.x << " x " << imgSize.y << " x " << imgSize.z << " px");

		//output image that will be iteratively re-rendered
		img.MakeRoom(imgSize.x,imgSize.y,imgSize.z);
		//img.SetResolution(*(new i3d::Resolution(imgRes.x,imgRes.y,imgRes.z))); //TODO
		img.SetResolution(i3d::Resolution(imgRes.x,imgRes.y,imgRes.z));

		//init display/export units,
		//and make them persistent (to survive after this method is finished)
		//static VoidDisplayUnit vDU;
		static ConsoleDisplayUnit cDU;
		//static SceneryBufferedDisplayUnit sDU("localhost:8765");

		displayUnit.RegisterUnit(cDU);
		//displayUnit.RegisterUnit(sDU);

		// --------------------------------------------------

		//initializeAgents();
		initializeAgents_aFew();
		REPORT("--init--------- " << currTime << " (" << agents.size() << " agents) ---------------");

		renderNextFrame();
	}


	void execute(void)
	{
		//run the simulation
		while (currTime < stopTime)
		{
			//obtain develop new shapes... (can run in parallel)
			std::list<AbstractAgent*>::iterator c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->advanceAndBuildIntForces();
				(*c)->adjustGeometryByIntForces();
				(*c)->collectExtForces();
				(*c)->adjustGeometryByExtForces();
			}

			//to export new geometries ... (can run in parallel)
			c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->updateGeometry();
			}

			//remove removable (can't run in parallel)
			c=dead_agents.begin();
			while (c != dead_agents.end())
			{
				delete (*c);
				c=dead_agents.erase(c);
			}

			//TODO: remove after sometime
			if (dead_agents.size() != 0)
				REPORT("CONFUSION: dead_agents IS NOT EMPTY!");

			/*
			//create removable (can't run in parallel)
			c=agents.begin();
			while (c != agents.end())
			{
				if ((*c)->shouldDie)
				{
					//schedule this one for removal
					dead_agents.push_back(*c);

					if (tracks[(*c)->ID].toTimeStamp > -1)
						REPORT("CONFUSION: dying cell HAS CLOSED TRACK!");
					tracks[(*c)->ID].toTimeStamp=frameCnt-1;

					c=agents.erase(c);
					//this removed the agent from the list of active cells,
					//but does not remove it from the memory!
					//(so that other agents that still hold _pointer_
					//on this one in their Cell::listOfFriends lists
					//can _safely_ notice this one has Cell::shouldDie
					//flag on and take appropriate action)
					//
					//after the next round of Cell::applyForces() is over,
					//all agents have definitely removed this agent from
					//their Cell::listOfFriends, that is when we can delete
					//this agent from the memory
				}
				else c++;
			}
			*/
	#ifdef DEBUG
			if (dead_agents.size() > 0)
				REPORT(dead_agents.size() << " cells will be removed from memory");
	#endif

			// move to the next simulation time point
			currTime += incrTime;
			REPORT("--------------- " << currTime << " (" << agents.size() << " agents) ---------------");

			// is this the right time to export data?
			if (currTime >= frameCnt*expoTime) renderNextFrame();
		}
	}


	/** the destructor frees simulation agents and creates/writes the tracks.txt file */
	~Simulation()
	{
		//delete all agents...
		std::list<AbstractAgent*>::iterator iter=agents.begin();
		while (iter != agents.end())
		{
			delete *iter; *iter = NULL;
			iter++;
		}
		DEBUG_REPORT("all agents were removed...");

		// write out the track record:
		std::map<int, TrackRecord >::iterator itTr;
		std::ofstream of("tracks.txt");

		for (itTr = tracks.begin(); itTr != tracks.end(); itTr++)
		{
			//finish unclosed tracks....
			if (itTr->second.toTimeStamp < 0) itTr->second.toTimeStamp=frameCnt-1;

			//export on harddrive (only if the cell has really been displayed at least once)
			if (itTr->second.toTimeStamp >= itTr->second.fromTimeStamp)
				of << itTr->second.ID << " "
					<< itTr->second.fromTimeStamp << " "
					<< itTr->second.toTimeStamp << " "
					<< itTr->second.parentID << std::endl;
		}
		of.close();
		DEBUG_REPORT("tracks.txt were saved...");
	}


private:
	/**
	 * Just initializes all agents: positions, weights, etc.,
	 * and adds them into the this->agents, and into the this->tracks.
	 */
	void initializeAgents(void)
	{
		//stepping in all directions -> influences the final number of nuclei
		const float dx = 14.0f;

		//to obtain a sequence of IDs for new agents...
		int ID=1;

		//longer axis x
		//symmetric/short axes y,z

		const float Xside  = (0.9f*sceneSize.x)/2.0f;
		const float YZside = (0.9f*sceneSize.y)/2.0f;

		for (float z=-Xside; z <= +Xside; z += dx)
		{
			//radius at this z position
			const float radius = YZside * sinf(acosf(fabsf(z)/Xside));

			const int howManyToPlace = (int)ceil(6.28f*radius / dx);
			for (int i=0; i < howManyToPlace; ++i)
			{
				const float ang = float(i)/float(howManyToPlace);

				//the wished position relative to [0,0,0] centre
				Vector3d<float> pos(z,radius * cosf(ang*6.28f),radius * sinf(ang*6.28f));

				//position is shifted to the scene centre
				pos.x += sceneSize.x/2.0f;
				pos.y += sceneSize.y/2.0f;
				pos.z += sceneSize.z/2.0f;

				//position is shifted due to scene offset
				pos.x += sceneOffset.x;
				pos.y += sceneOffset.y;
				pos.z += sceneOffset.z;

				Spheres s(4);
				s.updateCentre(0,pos);
				s.updateRadius(0,4.0f);

				AbstractAgent* ag = new NucleusAgent(ID++,s,currTime,incrTime);
				agents.push_back(ag);
				tracks.insert(std::pair<int,TrackRecord>(ag->ID,TrackRecord(ag->ID,frameCnt,-1,0)));
			}
		}
	} //end of initializeAgents()

	void initializeAgents_aFew(void)
	{
		//to obtain a sequence of IDs for new agents...
		int ID=1;

		const float radius = 0.7f*sceneSize.y;
		const int howManyToPlace = 6;

		for (int i=0; i < howManyToPlace; ++i)
		{
			const float ang = float(i)/float(howManyToPlace);

			//the wished position relative to [0,0,0] centre
			Vector3d<float> pos(radius * cosf(ang*6.28f),radius * sinf(ang*6.28f),0.0f);

			//position is shifted to the scene centre
			pos.x += sceneSize.x/2.0f;
			pos.y += sceneSize.y/2.0f;
			pos.z += sceneSize.z/2.0f;

			//position is shifted due to scene offset
			pos.x += sceneOffset.x;
			pos.y += sceneOffset.y;
			pos.z += sceneOffset.z;

			Spheres s(4);
			s.updateCentre(0,pos);
			s.updateRadius(0,4.0f);

			AbstractAgent* ag = new NucleusAgent(ID++,s,currTime,incrTime);
			agents.push_back(ag);
			tracks.insert(std::pair<int,TrackRecord>(ag->ID,TrackRecord(ag->ID,frameCnt,-1,0)));
		}
	}


	void renderNextFrame(void)
	{
		static char fn[1024];

		//clear the output image
		img.GetVoxelData() = 0;

		//go over all cells, and render them
		std::list<AbstractAgent*>::const_iterator c=agents.begin();
		for (; c != agents.end(); c++)
		{
			(*c)->drawMask(displayUnit);
			(*c)->drawMask(img);
		}

		//render the current frame
		displayUnit.Flush();

		//save the image
		sprintf(fn,"mask%03d.tif",frameCnt++);
		//img.SaveImage(fn);

		//wait for key...
		REPORT_NOENDL("Wait for key [and press Enter]: ");
		std::cin >> fn[0];
	}
}; //end of the Simulation class


int main(void)
{
	Simulation s;    //init and render the first frame
	s.execute();     //execute the simulation
	return(0);       //closes the simulation (as a side effect)
}
