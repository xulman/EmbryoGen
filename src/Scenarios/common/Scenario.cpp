#include "Scenario.h"

SceneControls DefaultSceneControls;


#if defined ENABLE_MITOGEN_FINALPREVIEW
  #include "../../util/synthoscopy/finalpreview.h"
#elif defined ENABLE_FILOGEN_PHASEIIandIII
  #include "../../util/synthoscopy/FiloGen_VM.h"
#endif
#include <i3d/filters.h>


void Scenario::initializePhaseIIandIII()
{
	DEBUG_REPORT("This scenario is using the default initialization routine.");

#ifdef ENABLE_FILOGEN_PHASEIIandIII
	//search the cmd line params for "-psf filePath"
	static const std::string psfSwitch("-psf");

	REPORT("You can request to use own PSF image via command line switch.");
	REPORT("e.g.:  " << psfSwitch << " ../2013-07-25_1_1_9_0_2_0_0_1_0_0_0_0_9_12.ics");

	//locate the switch string
	int argPos = 1;
	while (argPos < argc && psfSwitch.find(argv[argPos]) == std::string::npos) ++argPos;

	//move on the next parameter
	++argPos;

	//if we are still among existing params, copy the path
	if (argPos < argc)
		imgPSFuserPath = argv[argPos];

	//if there is non-empty path, try to load the file
	if (!imgPSFuserPath.empty())
	{
		REPORT("reading this PSF image " << imgPSFuserPath);
		imgPSF.ReadImage(imgPSFuserPath.c_str());
	}
#endif
}


void Scenario::doPhaseIIandIII()
{
#if defined ENABLE_MITOGEN_FINALPREVIEW
	REPORT("using default MitoGen synthoscopy");
	mitogen::PrepareFinalPreviewImage(params.imgPhantom,params.imgFinal);

#elif defined ENABLE_FILOGEN_PHASEIIandIII
	REPORT("using default FiloGen synthoscopy");
	//
	// phase II
	if (!imgPSFuserPath.empty())
	{
		filogen::PhaseII(params.imgPhantom, imgPSF);
	}
	else
	{
		const float xySigma = 0.6f; //can also be 0.9
		const float  zSigma = 1.8f; //can also be 2.7
		DEBUG_REPORT("fake PSF is used for PhaseII, with sigmas: "
			<< xySigma * params.imgPhantom.GetResolution().GetX() << " x "
			<< xySigma * params.imgPhantom.GetResolution().GetY() << " x "
			<<  zSigma * params.imgPhantom.GetResolution().GetZ() << " pixels");
		i3d::GaussIIR<float>(params.imgPhantom,
			xySigma * params.imgPhantom.GetResolution().GetX(),
			xySigma * params.imgPhantom.GetResolution().GetY(),
			 zSigma * params.imgPhantom.GetResolution().GetZ());
	}
	//
	// phase III
	filogen::PhaseIII(params.imgPhantom, params.imgFinal);

#else
	REPORT("WARNING: Empty function, no synthoscopy is going on.");
#endif
}
