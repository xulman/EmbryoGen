#include <TransferImage.h>
#include "Simulation.h"



// -----------------------------------------------------------------------------
template <typename T>
void Simulation::transferImg(const i3d::Image3d<T>& img, const std::string& URL) const
{
	if (URL.empty()) return;

	try {
		DAIS::imgParams_t imgParams;
		imgParams.dim = 3;
		imgParams.sizes = new int[3];
		imgParams.sizes[0] = (int)img.GetSizeX();
		imgParams.sizes[1] = (int)img.GetSizeY();
		imgParams.sizes[2] = (int)img.GetSizeZ();
		imgParams.voxelType = sizeof(*img.GetFirstVoxelAddr()) == 2 ? "UnsignedShortType" : "FloatType";
		imgParams.backendType = std::string("PlanarImg"); //IMPORTANT

		DAIS::connectionParams_t cnnParams;
		DAIS::StartSendingOneImage(imgParams,cnnParams,URL.c_str(),30);

		//set and send metadata
		std::list<std::string> metaData;
		//IMPORTANT two lines: 'imagename' and 'some name with allowed whitespaces'
		metaData.push_back(std::string("imagename"));
		metaData.push_back(std::string("EmbryoGen's Image(s)"));
		DAIS::SendMetadata(cnnParams,metaData);

		//send the raw pixel image data
		//ORIG// TransmitOneImage(cnnParams,imgParams,img.GetFirstVoxelAddr());

		if (sizeof(*img.GetFirstVoxelAddr()) == 2)
			DAIS::TransmitOneImage(cnnParams,imgParams,(unsigned short*)img.GetFirstVoxelAddr());
		else
			DAIS::TransmitOneImage(cnnParams,imgParams,(float*)img.GetFirstVoxelAddr());

		//close the connection, calls also cnnParams.clear()
		DAIS::FinishSendingOneImage(cnnParams);

		//clean up...
		imgParams.clear();
	}
	catch (std::exception* e)
	{
		REPORT("Transmission problem: " << e->what());
	}
}


template <typename T>
void Simulation::transferImgs(const i3d::Image3d<T>& img, DAIS::ImagesAsEventsSender& channel) const
{
	try {
		DAIS::imgParams_t imgParams;
		imgParams.dim = 3;
		imgParams.sizes = new int[3];
		imgParams.sizes[0] = (int)img.GetSizeX();
		imgParams.sizes[1] = (int)img.GetSizeY();
		imgParams.sizes[2] = (int)img.GetSizeZ();
		imgParams.voxelType = sizeof(*img.GetFirstVoxelAddr()) == 2 ? "UnsignedShortType" : "FloatType";
		imgParams.backendType = std::string("PlanarImg"); //IMPORTANT

		if (sizeof(*img.GetFirstVoxelAddr()) == 2)
			channel.sendImage(imgParams,(unsigned short*)img.GetFirstVoxelAddr());
		else
			channel.sendImage(imgParams,(float*)img.GetFirstVoxelAddr());

		//clean up...
		imgParams.clear();
	}
	catch (std::exception* e)
	{
		REPORT("Transmission problem: " << e->what());
	}
}
