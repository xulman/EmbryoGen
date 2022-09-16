#include <string>
#include "common/Scenarios.hpp"

void Scenario_phaseIIandIII::initializeAgents(FrontOfficer*,int p,int)
{
	if (p != 1)
	{
report::message(fmt::format("Doing something only on the first FO (which is not this one)." ));
		return;
	}

	char fn[1024];
	bool useCannonicalName = true;

report::message(fmt::format("Note the optional usage patterns:" ));
report::message(fmt::format("{} synthoscopy ownFilepattern%05d.tif" , argv[0]));
report::message(fmt::format("{} synthoscopy ownFilepattern%05d.tif fromTimePoint tillTimePoint" , argv[0]));
report::message(fmt::format("{} synthoscopy fromTimePoint tillTimePoint" , argv[0]));

	int firstTP = 0;
	int  lastTP = 999;

	if (argc == 3)
	{
		useCannonicalName = false;
	}
	else if (argc == 4)
	{
		firstTP = std::stoi(std::string(argv[2]));
		 lastTP = std::stoi(std::string(argv[3]));
	}
	else if (argc == 5)
	{
		useCannonicalName = false;
		firstTP = std::stoi(std::string(argv[3]));
		 lastTP = std::stoi(std::string(argv[4]));
	}

	bool shouldTryOneMore = true;
	int frameCnt = firstTP;
	while (shouldTryOneMore && frameCnt <= lastTP)
	{
		try
		{
			if (useCannonicalName)
				sprintf(fn,params.constants.imgPhantom_filenameTemplate,frameCnt); //default filename
			else
				sprintf(fn,argv[2],frameCnt);           //user-given filename
report::message(fmt::format("READING: {}" , fn));

			params.imgPhantom.ReadImage(fn);
		}
		catch (...)
		{
			shouldTryOneMore = false;
		}

		//was the image opening successful?
		if (shouldTryOneMore)
		{
			//yes...
			//prepare the output image (allows to have inputs of different sizes...)
			params.imgFinal.CopyMetaData(params.imgPhantom);

			//populate and save it...
			doPhaseIIandIII();
			sprintf(fn,params.constants.imgFinal_filenameTemplate,frameCnt);
			params.imgFinal.SaveImage(fn);

			++frameCnt;
		}
	}

	//needs to be called here because the initializeScene(), which would have
	//been much more appropriate place for this, is called prior this method
	params.imagesSaving_disableForImgPhantom();
	params.imagesSaving_disableForImgFinal();
}


void Scenario_phaseIIandIII::initializeScene()
{
	params.disableWaitForUserPrompt();
}


SceneControls& Scenario_phaseIIandIII::provideSceneControls()
{
	SceneControls::Constants myConstants;
	myConstants.stopTime = 0;
	return *(new SceneControls(myConstants));
}
