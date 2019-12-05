#ifndef SIMULATION_H
#define SIMULATION_H

#include <list>
#include <i3d/image3d.h>
#include <TransferImage.h>

//choose either ENABLE_MITOGEN_FINALPREVIEW or ENABLE_FILOGEN_PHASEIIandIII,
//if ENABLE_FILOGEN_PHASEIIandIII is choosen, one can enable ENABLE_FILOGEN_REALPSF
//#define ENABLE_MITOGEN_FINALPREVIEW
#define ENABLE_FILOGEN_PHASEIIandIII
#define ENABLE_FILOGEN_REALPSF

/**
 * This class contains all simulation agents, scene and simulation
 * parameters, and takes care of the iterations of the simulation.
 *
 * Essentially, the simulation is initiated with this class's constructor,
 * iterations happen in the execute() method and the simulation is closed
 * in this class's destructor. The execute() goes over all simulation agents
 * and calls their methods (via AbstractAgent methods) in the correct order etc.
 * The execute() is essentially a "commanding" method.
 *
 * Every agent, however, needs to examine its surrounding environment, e.g.
 * calculate distances to his neighbors, during its operation. To realize this,
 * a set of "callback" functions is provided in this class. Namely, a rough
 * distance (based on bounding boxes) and detailed (and computationally
 * expensive) distance measurements. The latter will be cached. Both
 * methods must be able to handle multi-threaded situation (should be
 * re-entrant).
 *
 * Author: Vladimir Ulman, 2018
 */
class Simulation
{
public:
	/** initializes the simulation parameters */
	Simulation(void)
		: transferImgChannelA("localhost:54545",30,"EmbryoGen's Image(s)"), //NB: no connection yet
		  transferImgChannelB("localhost:54546",30,"EmbryoGen's Image(s)")  //NB: no connection yet
	{
		//setup routing of img transfers, currently two channels are available
		//transferPhantomImgChannel = &transferImgChannelA;
		transferOpticsImgChannel  = &transferImgChannelA;
		//transferMaskImgChannel    = &transferImgChannelB;
		//transferFinalImgChannel   = &transferImgChannelB;
	}

public:

	/** util method to transfer image to the URL,
	    no transfer occurs if the URL is not defined */
	template <typename T>
	void transferImg(const i3d::Image3d<T>& img, const std::string& URL) const;

	/** util method to transfer sequence of images over the same connection channel */
	template <typename T>
	void transferImgs(const i3d::Image3d<T>& img, DAIS::ImagesAsEventsSender& channel) const;

	/** where to transfer images with defaultImgTransferChannel,
	    leave set to NULL to prohibit transfers of respective images */
	DAIS::ImagesAsEventsSender* transferMaskImgChannel    = NULL;
	DAIS::ImagesAsEventsSender* transferPhantomImgChannel = NULL;
	DAIS::ImagesAsEventsSender* transferOpticsImgChannel  = NULL;
	DAIS::ImagesAsEventsSender* transferFinalImgChannel   = NULL;

	/** the actual image transfer channels to choose from */
	DAIS::ImagesAsEventsSender transferImgChannelA, transferImgChannelB;

};
#endif
