#include <string>
#include "common/Scenarios.h"

void Scenario_phaseIIandIII::initializeScenario(void)
{
	char fn[1024];
	bool useCannonicalName = true;

	REPORT("Note the optional usage patterns:");
	REPORT(argv[0] << " synthoscopy ownFilepattern%05d.tif");
	REPORT(argv[0] << " synthoscopy ownFilepattern%05d.tif fromTimePoint tillTimePoint");
	REPORT(argv[0] << " synthoscopy fromTimePoint tillTimePoint");

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
	frameCnt = firstTP;
	while (shouldTryOneMore && frameCnt <= lastTP)
	{
		try
		{
			if (useCannonicalName)
				sprintf(fn,"phantom%03d.tif",frameCnt); //default filename
			else
				sprintf(fn,argv[2],frameCnt);           //user-given filename
			REPORT("READING: " << fn);

			imgPhantom.ReadImage(fn);
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
			imgFinal.CopyMetaData(imgPhantom);

			//populate and save it...
			doPhaseIIandIII();
			sprintf(fn,"finalPreview%03d.tif",frameCnt);
			imgFinal.SaveImage(fn);

			++frameCnt;
		}
	}

	//throw ERROR_REPORT("controlled exit after the synthoscopy is over");
	//nicer way:
	stopTime = 0.f;
	disableProducingOutput( imgPhantom );
	disableProducingOutput( imgFinal );
	disableWaitForUserPrompt();
}
