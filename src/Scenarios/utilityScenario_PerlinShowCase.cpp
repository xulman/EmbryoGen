#include <i3d/image3d.h>
#include "common/Scenarios.h"
#include "../util/texture/texture.h"

void Scenario_PerlinShowCase::initializeAgents(FrontOfficer*,int p,int)
{
	if (p != 1)
	{
		REPORT("Doing something only on the first FO (which is not this one).");
		return;
	}

	//resolution of the Perlin noise image
	const float xRes = 2.0f; //as pixels per micron
	const float yRes = 2.0f;
	const float zRes = 2.0f;

	//size of the Perlin noise image
	const float xSize = 20.0f; //in microns
	const float ySize = 20.0f;
	const float zSize = 20.0f;

	REPORT("Note the optional usage patterns:");
	REPORT(argv[0] << " PerlinShowCase anySingleParameter  - produce also finalPreviews");
	REPORT(argv[0] << " PerlinShowCase var alpha beta n    - produce given single sample");

	params.imgPhantom.SetResolution( i3d::Resolution(xRes,yRes,zRes) );
	params.imgPhantom.MakeRoom( (size_t)std::ceil(xSize * xRes),
	                            (size_t)std::ceil(ySize * yRes),
	                            (size_t)std::ceil(zSize * zRes) );
	if (argc == 3) params.imgFinal.CopyMetaData(params.imgPhantom);

	REPORT("Perlin image size: " << params.imgPhantom.GetSizeX() << " x "
	       << params.imgPhantom.GetSizeY() << " x " << params.imgPhantom.GetSizeZ() << " pixels");

	char fn[1024];

	if (argc == 6)
	{
		//single sample
		double var   = std::stod(std::string(argv[2]));
		double alpha = std::stod(std::string(argv[3]));
		double beta  = std::stod(std::string(argv[4]));
		int    n     = std::stoi(std::string(argv[5]));

		DoPerlin3D(params.imgPhantom, var,alpha,beta,n);

		sprintf(fn,"Perlin_%02.2f_%02.2f_%02.2f_%d.ics",var,alpha,beta,n);
		REPORT("Saving Perlin phantom image: " << fn);
		params.imgPhantom.SaveImage(fn);
	}
	else
	{
		//iterate here (matrix of samples)
		for (double var   = 2.0; var   <=  8.0; var   += 2.0)
		for (double alpha = 4.0; alpha <= 12.0; alpha += 4.0)
		for (double  beta = 2.0;  beta <=  6.0;  beta += 2.0)
		for (int      n   = 4  ;   n   <=    8;   n   += 2)
		{
			DoPerlin3D(params.imgPhantom, var,alpha,beta,n);

			sprintf(fn,"Perlin_%02.2f_%02.2f_%02.2f_%d.ics",var,alpha,beta,n);
			REPORT("Saving Perlin phantom image: " << fn);
			params.imgPhantom.SaveImage(fn);

			if (argc == 3)
			{
				doPhaseIIandIII();
				sprintf(fn,"PerlinFinalPreview_%02.2f_%02.2f_%02.2f_%d.ics",var,alpha,beta,n);
				REPORT("Saving Perlin final image: " << fn);
				params.imgFinal.SaveImage(fn);
			}
		}
	}

	//needs to be called here because the initializeScene(), which would have
	//been much more appropriate place for this, is called prior this method
	params.imagesSaving_disableForImgPhantom();
	params.imagesSaving_disableForImgFinal();
}


void Scenario_PerlinShowCase::initializeScene()
{
	params.disableWaitForUserPrompt();
}


SceneControls& Scenario_PerlinShowCase::provideSceneControls()
{
	SceneControls::Constants myConstants;
	myConstants.stopTime = 0;
	return *(new SceneControls(myConstants));
}
