#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Simulation.h"
#include "Scenarios.h"
#include "../Agents/NucleusAgent.h"
#include "../Agents/util/Texture.h"
#include "../util/texture/texture.h"

class myTexturedNucleus: public NucleusAgent, TextureQuantized
{
public:
	myTexturedNucleus(const int _ID, const std::string& _type,
	          const Spheres& shape,
	          const float _currTime, const float _incrTime):
		NucleusAgent(_ID,_type, shape, _currTime,_incrTime),
		TextureQuantized(60000, Vector3d<float>(2.0f,2.0f,2.0f), 125)
	{
		//texture img: resolution -- makes sense to match it with the phantom img resolution
		i3d::Image3d<float> img;
		SetupImageForRasterizingTexture(img,Vector3d<float>(2.0f), futureGeometry);

		DoPerlin3D(img,5.0);
		img.GetVoxelData() += 0.6f;
		SampleDotsFromImage(img,futureGeometry,0.1f);

		const int dotOutliers = CollectOutlyingDots(futureGeometry);
		DEBUG_REPORT(dotOutliers << " (" << 100.f*dotOutliers/dots.size()
		             << " %) dots had to be moved inside the initial geometry");
	}

	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>&) override
	{
		if (Officer->isProducingOutput(phantom)) RenderIntoPhantom(phantom);
	}
};

void Scenario_withTexture::initializeAgents(void)
{
	//to obtain a sequence of IDs for new agents...
	int ID=1;

	const float radius = 0.4f*sceneSize.y;
	const int howManyToPlace = argc > 2? atoi(argv[2]) : 6;

	Vector3d<float> sceneCentre(sceneSize);
	sceneCentre /= 2.0f;
	sceneCentre += sceneOffset;

	for (int i=0; i < howManyToPlace; ++i)
	{
		const float ang = float(i)/float(howManyToPlace);

		//the wished position relative to [0,0,0] centre
		Vector3d<float> pos(radius * std::cos(ang*6.28f),radius * std::sin(ang*6.28f),0.0f);

		//position is shifted to the scene centre, accounting for scene offset
		pos += sceneCentre;

		Spheres s(2);
		s.updateCentre(0,pos+Vector3d<float>(0,0,-3));
		s.updateRadius(0,5.0f);
		s.updateCentre(1,pos+Vector3d<float>(0,0,+3));
		s.updateRadius(1,5.0f);

		myTexturedNucleus* ag = new myTexturedNucleus(ID++,"nucleus with texture",s,currTime,incrTime);
		startNewAgent(ag);
	}

	stopTime = 0.2f;
	enableProducingOutput(imgPhantom);
	enableProducingOutput(imgFinal);
}

void Scenario_withTexture::doPhaseIIandIII(void)
{
	REPORT("hello");
}
