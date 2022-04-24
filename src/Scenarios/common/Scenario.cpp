#include "Scenario.hpp"

SceneControls DefaultSceneControls;

#if defined ENABLE_MITOGEN_FINALPREVIEW
#include "../../util/synthoscopy/finalpreview.hpp"
#elif defined ENABLE_FILOGEN_PHASEIIandIII
#include "../../util/synthoscopy/FiloGen_VM.hpp"
#endif
#include <i3d/filters.h>

void Scenario::initializePhaseIIandIII() {
	report::debugMessage(
	    "This scenario is using the default initialization routine.");

#ifdef ENABLE_FILOGEN_PHASEIIandIII
	// search the cmd line params for "-psf filePath"
	static const std::string psfSwitch("-psf");

	report::message(
	    "You can request to use own PSF image via command line switch.");
	report::message(fmt::format(
	    "e.g.:  {} ../2013-07-25_1_1_9_0_2_0_0_1_0_0_0_0_9_12.ics", psfSwitch));

	// locate the switch string
	int argPos = 1;
	while (argPos < argc && psfSwitch.find(argv[argPos]) == std::string::npos)
		++argPos;

	// move on the next parameter
	++argPos;

	// if we are still among existing params, copy the path
	if (argPos < argc)
		imgPSFuserPath = argv[argPos];

	// if there is non-empty path, try to load the file
	if (!imgPSFuserPath.empty()) {
		report::message(
		    fmt::format("reading this PSF image {}", imgPSFuserPath));
		imgPSF.ReadImage(imgPSFuserPath.c_str());
	}
#endif
}

void Scenario::doPhaseIIandIII() {
#if defined ENABLE_MITOGEN_FINALPREVIEW
	report::message("using default MitoGen synthoscopy");
	mitogen::PrepareFinalPreviewImage(params.imgPhantom, params.imgFinal);

#elif defined ENABLE_FILOGEN_PHASEIIandIII
	report::message("using default FiloGen synthoscopy");
	//
	// phase II
	if (!imgPSFuserPath.empty()) {
		filogen::PhaseII(params.imgPhantom, imgPSF);
	} else {
		const float xySigma = 0.6f; // can also be 0.9
		const float zSigma = 1.8f;  // can also be 2.7
		report::debugMessage(fmt::format(
		    "fake PSF is used for PhaseII, with sigmas: {} x {} x {} pixels",
		    xySigma * params.imgPhantom.GetResolution().GetX(),
		    xySigma * params.imgPhantom.GetResolution().GetY(),
		    zSigma * params.imgPhantom.GetResolution().GetZ()));
		i3d::GaussIIR<float>(params.imgPhantom,
		                     xySigma * params.imgPhantom.GetResolution().GetX(),
		                     xySigma * params.imgPhantom.GetResolution().GetY(),
		                     zSigma * params.imgPhantom.GetResolution().GetZ());
	}
	//
	// phase III
	filogen::PhaseIII(params.imgPhantom, params.imgFinal);

#else
	report::message("WARNING: Empty function, no synthoscopy is going on.");
#endif
}

// -----------------------------------------------------------------------------
//#include <TransferImage.h>

/** util method to transfer image to the URL, no transfer occurs if the URL is
 * not defined */
/*template <typename T>
void transferImg(const i3d::Image3d<T>& img, const std::string& URL)
{
    if (URL.empty()) return;

    try {
        DAIS::imgParams_t imgParams;
        imgParams.dim = 3;
        imgParams.sizes = new int[3];
        imgParams.sizes[0] = (int)img.GetSizeX();
        imgParams.sizes[1] = (int)img.GetSizeY();
        imgParams.sizes[2] = (int)img.GetSizeZ();
        imgParams.voxelType = sizeof(*img.GetFirstVoxelAddr()) == 2 ?
"UnsignedShortType" : "FloatType"; imgParams.backendType =
std::string("PlanarImg"); //IMPORTANT

        DAIS::connectionParams_t cnnParams;
        DAIS::StartSendingOneImage(imgParams,cnnParams,URL.c_str(),30);

        //set and send metadata
        std::vector<std::string> metaData;
        //IMPORTANT two lines: 'imagename' and 'some name with allowed
whitespaces' metaData.push_back(std::string("imagename"));
        metaData.push_back(std::string("EmbryoGen's Image(s)"));
        DAIS::SendMetadata(cnnParams,metaData);

        //send the raw pixel image data
        //ORIG// TransmitOneImage(cnnParams,imgParams,img.GetFirstVoxelAddr());

        if (sizeof(*img.GetFirstVoxelAddr()) == 2)
            DAIS::TransmitOneImage(cnnParams,imgParams,(unsigned
short*)img.GetFirstVoxelAddr()); else
            DAIS::TransmitOneImage(cnnParams,imgParams,(float*)img.GetFirstVoxelAddr());

        //close the connection, calls also cnnParams.clear()
        DAIS::FinishSendingOneImage(cnnParams);

        //clean up...
        imgParams.clear();
    }
    catch (std::exception* e)
    {
report::message(fmt::format("Transmission problem: {}" , e->what()));
    }
}

*/
/** util method to transfer sequence of images over the same connection channel
 */
/*template <typename T>
void transferImgs(const i3d::Image3d<T>& img, DAIS::ImagesAsEventsSender&
channel)
{
    try {
        DAIS::imgParams_t imgParams;
        imgParams.dim = 3;
        imgParams.sizes = new int[3];
        imgParams.sizes[0] = (int)img.GetSizeX();
        imgParams.sizes[1] = (int)img.GetSizeY();
        imgParams.sizes[2] = (int)img.GetSizeZ();
        imgParams.voxelType = sizeof(*img.GetFirstVoxelAddr()) == 2 ?
"UnsignedShortType" : "FloatType"; imgParams.backendType =
std::string("PlanarImg"); //IMPORTANT

        if (sizeof(*img.GetFirstVoxelAddr()) == 2)
            channel.sendImage(imgParams,(unsigned
short*)img.GetFirstVoxelAddr()); else
            channel.sendImage(imgParams,(float*)img.GetFirstVoxelAddr());

        //clean up...
        imgParams.clear();
    }
    catch (std::exception* e)
    {
report::message(fmt::format("Transmission problem: {}" , e->what()));
    }
}

template void transferImgs(const i3d::Image3d<i3d::GRAY8>& img,
DAIS::ImagesAsEventsSender& channel); template void transferImgs(const
i3d::Image3d<i3d::GRAY16>& img, DAIS::ImagesAsEventsSender& channel); template
void transferImgs(const i3d::Image3d<float>& img, DAIS::ImagesAsEventsSender&
channel); template void transferImgs(const i3d::Image3d<double>& img,
DAIS::ImagesAsEventsSender& channel);
*/
