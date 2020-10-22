#ifndef SCENARIO_H
#define SCENARIO_H

#include <map>
#include <set>
#include <TransferImage.h>
#include <i3d/image3d.h>
#include "../../util/Vector3d.h"
#include "../../util/report.h"
#include "../../DisplayUnits/BroadcasterDisplayUnit.h"

//instead of the #include statement, the FrontOfficer type is only declared to exists,
//FrontOfficer's definition depends on Scenario and so we'd end up in a definitions loop,
//and the same holds for the Director type
class FrontOfficer;
class Director;
template <typename T> void transferImgs(const i3d::Image3d<T>& img, DAIS::ImagesAsEventsSender& channel);

/**
 * Container of "external", declarative/descriptive parameters of
 * the simulated scene. Some parameters are immutable, some parameters'
 * values can "evolve" during the simulation (e.g., control online
 * the simulator's outputs, or the "wait for key" behaviour).
 *
 * This is the class with default parameters, one has to derive
 * her own in order to override the immutable parameters or the
 * (behaviour) parameters during the simulation.
 *
 * The class has a callback method (to possibly update the mutable
 * parameters) that is regularly executed by the Direktor and all FOs
 * after a full simulation round is over.
 * Since it is called from all of them, all instances (one in Direktor
 * and one per FO) of this class will remain synchronized.
 */
class SceneControls
{
	// to enable the Direktor's and FOs' access protected elements
	friend class FrontOfficer;
	friend class Director;

public:
	//a subset of controls that are treated immutable, and are defined below...
	class Constants;

	/** the very default c'tor */
	SceneControls()
		: constants()
	{
		//sets up all images but does not allocate them
		setOutputImgSpecs(constants.sceneOffset,constants.sceneSize,constants.imgRes);
	}

	/** c'tor that essentially allows to set the const'ed attributes with something own,
	    it achieves so by making a private const copy of the input Constants (which
	    makes it immune to any possible later changes in the given input Constants) */
	SceneControls(Constants& callersOwnConstants)
		: constants(callersOwnConstants) //makes own copy!
	{
		//sets up all images but does not allocate them
		setOutputImgSpecs(constants.sceneOffset,constants.sceneSize,constants.imgRes);
	}

	/** the callback method that is regularly executed by the
	    Direktor and all FOs after every full simulation round is over */
	virtual void updateControls(const float)
	{ DEBUG_REPORT_NOHEADER("This scenario is not updating its controls."); }


	/** a subset of controls that are treated immutable in the scenario,
	    it comes with own default values that are, however,
	    changeable here alone but not when this is part of the scenario */
	class Constants
	{
	public:
		//CTC drosophila:
		//x,y,z res = 2.46306,2.46306,0.492369 px/um
		//embryo along x-axis
		//bbox size around the embryo: 480,220,220 um
		//bbox size around the embryo: 1180,540,110 px

		Constants()
			: sceneOffset(0.f),
			  sceneSize(480.f,220.f,220.f),
			  imgRes(2.0f,2.0f,2.0f)
		{}

		/** set up the reference sandbox: offset of the scene [micrometer] */
		Vector3d<float> sceneOffset;
		/** set up the reference sandbox: size of the scene [micrometer] */
		Vector3d<float> sceneSize;

		/** resolution of the output images [pixels per micrometer] */
		Vector3d<float> imgRes;

		/** initial global simulation time, [min] */
		float initTime = 0.0f;

		/** increment of the current global simulation time, [min]
			 represents the time step of one simulation step */
		float incrTime = 0.1f;

		/** at what global time should the simulation stop [min] */
		float stopTime = 200.0f;

		/** export simulation status always after this amount of global time, [min]
			 should be multiple of incrTime to obtain regular sampling */
		float expoTime = 0.5f;
	};

	/** a subset of truly (that is, syntactically enforced) constant scene parameters */
	const Constants constants;


	/** output image into which the simulation will be iteratively
	    rasterized/rendered: instance masks */
	i3d::Image3d<i3d::GRAY16> imgMask;

	/** output image into which the simulation will be iteratively
	    rasterized/rendered: texture phantom image */
	i3d::Image3d<float> imgPhantom;
	//
	/** output image into which the simulation will be iteratively
	    rasterized/rendered: optical indices image */
	i3d::Image3d<float> imgOptics;

	/** output image into which the simulation will be iteratively
	    rasterized/rendered: final output image */
	i3d::Image3d<i3d::GRAY16> imgFinal;

	/** a hub to anytime connect/disconnect specific instances of DisplayUnit */
	BroadcasterDisplayUnit displayUnit;


	void displayChannel_createNew(const std::string& channelName,
	                              const char* destinationURL,
	                              const char* channelDisplayTitle = "EmbryoGen's Image(s)")
	{
		//delete to avoid overriding
		displayChannel_delete(channelName);

		transferChannels[channelName]
			= new DAIS::ImagesAsEventsSender(destinationURL,30,channelDisplayTitle);
	}

	void displayChannel_delete(const std::string& channelName)
	{
		//we need to call the d'tor before we remove
		auto existingChannel = transferChannels.find(channelName);
		if (existingChannel != transferChannels.end())
		{
			REPORT("Removing transfer channel \"" << channelName << "\", it might hang here if the send buffers are not empty...");
			delete existingChannel->second;
			transferChannels.erase(existingChannel);
		}
	}

	void displayChannel_enableForImgMask( const std::string& channelName) { imgMaskBroadcast.insert( channelName ); }
	void displayChannel_disableForImgMask(const std::string& channelName) { imgMaskBroadcast.erase(  channelName ); }
	void displayChannel_transferImgMask() { transferImg( imgMask, "mask", imgMaskBroadcast ); }

	void displayChannel_enableForImgPhantom( const std::string& channelName) { imgPhantomBroadcast.insert( channelName ); }
	void displayChannel_disableForImgPhantom(const std::string& channelName) { imgPhantomBroadcast.erase(  channelName ); }
	void displayChannel_transferImgPhantom() { transferImg( imgPhantom, "phantom", imgPhantomBroadcast ); }

	void displayChannel_enableForImgOptics( const std::string& channelName) { imgOpticsBroadcast.insert( channelName ); }
	void displayChannel_disableForImgOptics(const std::string& channelName) { imgOpticsBroadcast.erase(  channelName ); }
	void displayChannel_transferImgOptics() { transferImg( imgOptics, "optics", imgOpticsBroadcast ); }

	void displayChannel_enableForImgFinal( const std::string& channelName) { imgFinalBroadcast.insert( channelName ); }
	void displayChannel_disableForImgFinal(const std::string& channelName) { imgFinalBroadcast.erase(  channelName ); }
	void displayChannel_transferImgFinal() { transferImg( imgFinal, "final", imgFinalBroadcast ); }


	void imagesSaving_enableForImgMask()       {  enableProducingOutput(imgMask); };
	bool imagesSaving_isEnabledForImgMask()    { return isProducingOutput(imgMask); };
	void imagesSaving_disableForImgMask()      { disableProducingOutput(imgMask); };

	void imagesSaving_enableForImgPhantom()    {  enableProducingOutput(imgPhantom); };
	bool imagesSaving_isEnabledForImgPhantom() { return isProducingOutput(imgPhantom); };
	void imagesSaving_disableForImgPhantom()   { disableProducingOutput(imgPhantom); };

	void imagesSaving_enableForImgOptics()     {  enableProducingOutput(imgOptics); };
	bool imagesSaving_isEnabledForImgOptics()  { return isProducingOutput(imgOptics); };
	void imagesSaving_disableForImgOptics()    { disableProducingOutput(imgOptics); };

	void imagesSaving_enableForImgFinal()      {  enableProducingOutput(imgFinal); };
	bool imagesSaving_isEnabledForImgFinal()   { return isProducingOutput(imgFinal); };
	void imagesSaving_disableForImgFinal()     { disableProducingOutput(imgFinal); };

protected:
	/** internal (private) memory of the input of setOutputImgSpecs() for the enableProducingOutput() */
	Vector3d<size_t> lastUsedImgSize;

	std::map< std::string, DAIS::ImagesAsEventsSender* > transferChannels;
	std::set< std::string > imgMaskBroadcast;
	std::set< std::string > imgPhantomBroadcast;
	std::set< std::string > imgOpticsBroadcast;
	std::set< std::string > imgFinalBroadcast;

	template <typename T>
	void transferImg(const i3d::Image3d<T>& img, const char* imgName, std::set< std::string >& routes)
	{
		auto route = routes.begin();
		while (route != routes.end())
		{
			//does this route exists in the transferChannels?
			auto const channel = transferChannels.find(*route);

			if (channel != transferChannels.end())
			{
				//yes, transfer image over this route...
				REPORT("Sending " << imgName << " image over the channel \""
				      << channel->first << "\" (" << channel->second->getURL() << "), hold on...");
				transferImgs(img,*(channel->second));
				//...and move on the next one
				route++;
			}
			else
			{
				//no, delete the route from this image's broadcast set
				DEBUG_REPORT("Deleting incorrect \"" << *route << "\" from channels for " << imgName << " image");
				route = routes.erase(route);
			}
		}
	}

public:
	/** util method to setup all output images at once, disables all of them for the output,
	    and hence does not allocate memory for the images */
	void setOutputImgSpecs(const Vector3d<float>& imgOffsetInMicrons,
	                       const Vector3d<float>& imgSizeInMicrons)
	{
		setOutputImgSpecs(imgOffsetInMicrons,imgSizeInMicrons,
		                  Vector3d<float>(imgMask.GetResolution().GetRes()));
	}

	/** util method to setup all output images at once, disables all of them for the output,
	    and hence does not allocate memory for the images */
	void setOutputImgSpecs(const Vector3d<float>& imgOffsetInMicrons,
	                       const Vector3d<float>& imgSizeInMicrons,
	                       const Vector3d<float>& imgResolutionInPixelsPerMicron)
	{
		//sanity checks:
		if (!imgSizeInMicrons.elemIsGreaterThan(Vector3d<float>(0)))
			throw ERROR_REPORT("image dimensions (size) cannot be zero or negative along any axis");
		if (!imgResolutionInPixelsPerMicron.elemIsGreaterThan(Vector3d<float>(0)))
			throw ERROR_REPORT("image resolution cannot be zero or negative along any axis");

		//metadata...
		imgMask.SetResolution(i3d::Resolution( imgResolutionInPixelsPerMicron.toI3dVector3d() ));
		imgMask.SetOffset( imgOffsetInMicrons.toI3dVector3d() );

		//disable usage of this image (for now)
		disableProducingOutput(imgMask);

		//but remember what the correct pixel size would be
		lastUsedImgSize.from(
		  Vector3d<float>(imgSizeInMicrons).elemMult(imgResolutionInPixelsPerMicron).elemCeil() );

		//propagate also the same setting onto the remaining images
		imgPhantom.CopyMetaData(imgMask);
		imgOptics.CopyMetaData(imgMask);
		imgFinal.CopyMetaData(imgMask);
	}

protected:
	/** util method to enable the given image for the output, the method
	    immediately allocated the necessary memory for the image */
	template <typename T>
	void enableProducingOutput(i3d::Image3d<T>& img)
	{
		if ((long)&img == (long)&imgFinal && !isProducingOutput(imgPhantom))
			REPORT("WARNING: Requested synthoscopy but phantoms may not be produced.");

		DEBUG_REPORT("allocating "
		  << (lastUsedImgSize.x*lastUsedImgSize.y*lastUsedImgSize.z/(1 << 20))*sizeof(*img.GetFirstVoxelAddr())
		  << " MB of memory for image of size " << lastUsedImgSize << " px");
		img.MakeRoom( lastUsedImgSize.toI3dVector3d() );
	}

	/** util method to disable the given image for the output, the method
	    immediately frees the allocated memory of the image */
	template <typename T>
	void disableProducingOutput(i3d::Image3d<T>& img)
	{
		//image size flags if this image should be iteratively filled and saved:
		//zero image size signals "don't use this image"
		img.MakeRoom(0,0,0);
	}

	/** util method to report if the given image is enabled for the output */
	template <typename T>
	bool isProducingOutput(const i3d::Image3d<T>& img) const
	{
		return (img.GetImageSize() > 0);
	}

	/** flag whether an user will be prompted (and the simulation would stop
	    and wait) at the end of the renderNextFrame() */
	bool shallWaitForUserPromptFlag = true;

public:
	/** the same as Director::enableWaitForUserPrompt() */
	void enableWaitForUserPrompt(void)
	{ shallWaitForUserPromptFlag = true; }

	/** the same as Director::disableWaitForUserPrompt() */
	void disableWaitForUserPrompt(void)
	{ shallWaitForUserPromptFlag = false; }
};

/** an alias to a convenience shared instance with default SceneControls,
    which serves multiple purposes: easier code in scenarios that don't need
    special SceneControls, and access to individual attributes holding the default
    values (useful when cherry-pick changing only some of the const'ed attributes
    by using the SceneControls(...) c'tor */
extern SceneControls DefaultSceneControls;


/**
 * The base class for all scenarios. Provides methods that new scenario
 * has to implement in order to define its "shape" (setup the controls
 * by providing SceneControls (or derived) object, and create and place
 * the agents), and optionally also its own digital phantom to final image
 * conversion.
 *
 * It intentionally has only one specific default c'tor. This way, every
 * scenario must provide some scene parameters that contain at least what
 * is inside the SceneControls, and this is the only way to construct
 * the scenario.
 * Note that the SceneControls are meant to be mainly declarative, or
 * descriptive if you will. Any heavy memory allocations or initialization
 * routines shall happen later in the initializeScene(), which is always
 * called from the Direktor and from all FOs.
 *
 * A new scenarios should be declared in the Scenarios/common/Scenarios.h
 * according to the template in there and should be defined in own *cpp
 * files in the Scenarios folder.
 *
 * A new scenario must implement own:
 *
 * provideSceneControls() where one may either use DefaultSceneControls,
 * or devise an instance of own controls class, note the two CLI attributes
 * are not really available yet (they contain bogus, see their definition),
 * note also that this method is absent in this class, it is declared only
 * in the above mentioned template,
 *
 * initializeScene() where one may additionally modify this->params,
 * allocate output images if need-be (will happen in Direktor and'
 * correspondingly in all FOs), etc., the CLI attributes are valid here,
 *
 * initializeAgents() where one should create the agents for this simulation,
 *
 * initializePhaseIIandIII() and doPhaseIIandIII(), optionally, override
 * the standard digital phantom to final image conversion routine, the CLI
 * attributes are valid during the execution of these two methods, and both
 * methods are called only from the Direktor.
 */
class Scenario
{
public:
	/** the one and only must-provide-SceneControls-enforcer c'tor */
	Scenario(SceneControls& params)
		: params(params), displays(*this, params), disks(*this, params)
	{}

	// to shortcut the Direktor's and FOs' access to this->params
	friend class FrontOfficer;
	friend class Director;

protected:
	/** the control "module" for this scenario which comes via the c'tor
	    (the source object is typically created in new scene's own
	    provideSceneControls()) and whose updateControls() is
	    regularly called in this->updateScene() */
	SceneControls& params;

	/** CLI params that might be considered by some of the initialize...() methods */
	int argc = 0;
	/** CLI params that might be considered by some of the initialize...() methods */
	char** argv;

public:
	/** explicit additional setter of the CLI params, which is not part
	    of the c'tor in order to keep it simple for the scenarios and
	    which is always called right after this object is constructed */
	void setArgs(int argc, char** argv)
	{
		this->argc = argc;
		this->argv = argv;
	}

	/** provides (to the outside world) only a read-only look into
	    the current state of this scenario's controls */
	const SceneControls& seeCurrentControls() const
	{ return params; }

	/** A callback to ask this scenario to set up itself, except for instantiating
	    its agents (that shall happen inside initializeAgents()). This method
	    is called from the Direktor and all FOs. */
	virtual void initializeScene() =0;

	/** Creates and initializes the relevant portion of the agents.
	    Method may consider the command-line parameters via this->argc and this->argv.
	    The scenario will be divided into 'noOfAllFractions' and this call
	    is responsible only for the portion given by this 'fractionNumber'.
	    This method is called only from all FOs, not from the Direktor.
	    Note that the fractionNumber goes from 1 till noOfAllFractions. */
	virtual void initializeAgents(FrontOfficer* fo, int fractionNumber, int noOfAllFractions) =0;

	/** A callback that triggers SceneControls::updateControls() after every
	    full simulation round is over. This method is called from the Direktor
	    and all FOs. */
	void updateScene(const float currTime)
	{
		if (amIinDirektorContext())
		{
			DEBUG_REPORT_NOENDL("by Direktor: ");
		}
		else
		{
			DEBUG_REPORT_NOENDL("by FO #" << contextID << ": ");
		}
		params.updateControls(currTime);
	}

	/** A callback to ask this scenario to set up its digital phantom to final
	    image conversion stack. This method is called only from the Direktor,
	    not from any FO. */
	virtual void initializePhaseIIandIII();

	/** A callback to ask this scenario to realize the digital phantom to final
	    image conversion, acting on the images from this->params according to
	    their "enabled" states (see SceneControls::isProducingOutput()).

	    The idea was:
	    Initializes (allocates, sets resolution, etc.) and populates (fills content)
	    the params.imgFinal, which will be saved as the final testing image,
	    based on the current content of the phantom and/or optics and/or mask image.

	    Depending on the scenario used, some of these (phantom, optics, mask) images
	    might be empty (voxels are zero), or their image size may be actually be zero.
	    This really depends on what agents are used and how they are designed. Which
	    is why this method has became "virtual" and over-ridable in every scenario
	    to suit its needs.

	    The content of the Simulation::imgPhantom and/or imgOptics images can be
	    altered in this method because the said variables shall not be used anymore
	    in the (just finishing) simulation round. Don't change Simulation::imgMask
	    because this one is used for computation of the SNR. */
	virtual void doPhaseIIandIII();

private:
	/** Context in which this particular scenario object is executed. It is actually
	    merely a symbolic value that shall be positive whenever this object is living
	    inside some FrontOfficer, and that shall be -1 whenever this object is living
	    inside the Direktor.

	    Scenarios shall, nevertheless, make no difference in their "behaviour" (how
	    they are defined, e.g. values of constants, or how they are changing
	    themselves via this->updateScene()). The notion of the context is defined
	    here only for cases where it really makes sense to behave differently, such
	    as, creating an image transfer object on Direktor only (when no images
	    shall be transferred on FrontOfficer), or connecting to SimViewer only from
	    one particular process (be it Direktor, or one particular FrontOfficer). */
	int contextID = -1;

	/** Internal method to be executed only during a FrontOfficer construction
	    to define the appropriate context of this particular scenario object */
	void declareFOcontext(const int myPortion)
	{
		contextID = myPortion;
#ifndef DISTRIBUTED
		REPORT("Not distributed: Down-sizing local images because they are (normally) not used from FO.");
		params.setOutputImgSpecs(params.constants.sceneOffset,Vector3d<float>(0.000001f));
#endif
	}

	/** Similar to Scenario::declareFOcontext() but for Direktor */
	void declareDirektorContext()
	{
		contextID = -1;
#ifndef DISTRIBUTED
		REPORT("Not distributed: Keeping local images because they are used directly from FO.");
#endif
	}
protected:
	bool amIinDirektorContext() { return contextID == -1; }
	bool amIinFOContext()       { return contextID != -1; }
	int getFOContextID()        { return contextID; }

public:
	struct Displays {
		Displays(Scenario& thisScenario, SceneControls& thisSceneControls)
			: ctx(thisScenario), params(thisSceneControls) {}
		Scenario& ctx;
		SceneControls& params;

		//----- display units -----
		/** registers DisplayUnit that gets created on-demand and returns the created
		    DisplayUnit or NULL (if there was no demand to create one);
		    use this from Scenario::initializeScene()

		    if initializeScene() would host a code 'registerDisplayUnit( new someDisplayUnit() )'
		    then this unit is created always prior every call but is registered only sometimes,
		    note that Scenario::initializeScene() is called both from Direktor and from FrontOfficer(s)
		    but the object should not be registered and created! for the Direktor... */
		DisplayUnit* registerDisplayUnit(const std::function< DisplayUnit*(void) >& unitFactory)
		{
			if (ctx.amIinFOContext())
			{
				DisplayUnit* ds = unitFactory();
				params.displayUnit.RegisterUnit(ds);
				return ds;
			}
			else return NULL;
		}

		/** registers already existing DisplayUnit;
		    don't use from Scenario::initializeScene() */
		void registerDisplayUnit(DisplayUnit* ds)
		{
			if (ctx.amIinFOContext())
				params.displayUnit.RegisterUnit(ds);
		}
		void unregisterDisplayUnit(DisplayUnit* ds)
		{
			if (ctx.amIinFOContext())
				params.displayUnit.UnregisterUnit(ds);
		}

		//----- image transfer channels -----
		void registerImagingUnit(const std::string& unitNickName,
		                         const char* destinationURL,
		                         const char* thisSourceNickName = "EmbryoGen's Image(s)")
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_createNew(unitNickName,destinationURL,thisSourceNickName);
		}
		void unregisterImagingUnit(const std::string& unitNickName)
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_delete(unitNickName);
		}

		void enableImgMaskInImagingUnit(const std::string& unitNickName)
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_enableForImgMask(unitNickName);
		}
		void disableImgMaskInImagingUnit(const std::string& unitNickName)
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_disableForImgMask(unitNickName);
		}

		void enableImgPhantomInImagingUnit(const std::string& unitNickName)
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_enableForImgPhantom(unitNickName);
		}
		void disableImgPhantomInImagingUnit(const std::string& unitNickName)
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_disableForImgPhantom(unitNickName);
		}

		void enableImgOpticsInImagingUnit(const std::string& unitNickName)
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_enableForImgOptics(unitNickName);
		}
		void disableImgOpticsInImagingUnit(const std::string& unitNickName)
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_disableForImgOptics(unitNickName);
		}

		void enableImgFinalInImagingUnit(const std::string& unitNickName)
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_enableForImgFinal(unitNickName);
		}
		void disableImgFinalInImagingUnit(const std::string& unitNickName)
		{
			if (ctx.amIinDirektorContext())
				params.displayChannel_disableForImgFinal(unitNickName);
		}
	} displays;

	struct Disks {
		Disks(Scenario& thisScenario, SceneControls& thisSceneControls)
			: ctx(thisScenario), params(thisSceneControls) {}
		Scenario& ctx;
		SceneControls& params;

		//----- image storing channels -----
		//(essentially only a bunch of aliases in dedicated "namespace")
		void enableImgMaskTIFFs()
		{ params.imagesSaving_enableForImgMask(); }
		void disableImgMaskTIFFs()
		{ params.imagesSaving_disableForImgMask(); }

		void enableImgPhantomTIFFs()
		{ params.imagesSaving_enableForImgPhantom(); }
		void disableImgPhantomTIFFs()
		{ params.imagesSaving_disableForImgPhantom(); }

		void enableImgOpticsTIFFs()
		{ params.imagesSaving_enableForImgOptics(); }
		void disableImgOpticsTIFFs()
		{ params.imagesSaving_disableForImgOptics(); }

		void enableImgFinalTIFFs()
		{ params.imagesSaving_enableForImgFinal(); }
		void disableImgFinalTIFFs()
		{ params.imagesSaving_disableForImgFinal(); }
	} disks;

#ifdef ENABLE_FILOGEN_PHASEIIandIII
private:
	std::string imgPSFuserPath;
	i3d::Image3d<float> imgPSF;
#endif
};
#endif
