#include "../util/DivisionModels.h"

void testInterpolation(void)
{
	const DivisionModels2S<5,5> sample("/Users/ulman/p_Johannes/Polyclad/divisionModelsForEmbryoGen.dat",3.0f);

	const int modelId = 1;
	const DivisionModels2S<5,5>::DivisionModel& m = sample.getModel(modelId);

	std::cout << "# model ID " << modelId << ":\n";
	m.printModel();
	std::cout << "\n\n";

	std::cout << "# evolution for mother from model ID " << modelId << ":\n";
	for (float t=-17; t < 0; t+=0.5)
		std::cout << t << ": " << m.getMotherRadius(t,0) << "-" << m.getMotherDist(t,0) << "-" << m.getMotherRadius(t,1) << "\n";
	std::cout << "\n\n";

	std::cout << "# evolution for daughter1 from model ID " << modelId << ":\n";
	for (float t=0; t < 15; t+=0.5)
		std::cout << t << ": " << m.getDaughterRadius(t,0,0) << "-" << m.getDaughterDist(t,0,0) << "-" << m.getDaughterRadius(t,0,1) << "\n";
	std::cout << "\n\n";

	std::cout << "# evolution for daughter2 from model ID " << modelId << ":\n";
	for (float t=0; t < 15; t+=0.5)
		std::cout << t << ": " << m.getDaughterRadius(t,1,0) << "-" << m.getDaughterDist(t,1,0) << "-" << m.getDaughterRadius(t,1,1) << "\n";
}


void testInvalidRangeExceptions(void)
{
	const DivisionModels2S<5,5> sample("/Users/ulman/p_Johannes/Polyclad/divisionModelsForEmbryoGen.dat",3.0f);
	bool sawException = false;

	try { sample.getModel(10); }
	catch (std::runtime_error* e)
	{
		std::cout << "Exception: " << e->what() << "\n";
		sawException = true;
	}
	std::cout << "exception test1 status: " << (sawException? "OK" : "fail") << "\n";

	sawException = false;
	try
	{ sample.getModel(1).getDaughterRadius(-1,2,1); }
	catch (std::runtime_error* e)
	{
		std::cout << "Exception: " << e->what() << "\n";
		sawException = true;
	}
	std::cout << "exception test2 status: " << (sawException? "OK" : "fail") << "\n";

	sawException = false;
	try
	{ sample.getModel(1).getDaughterRadius(-1,1,2); }
	catch (std::runtime_error* e)
	{
		std::cout << "Exception: " << e->what() << "\n";
		sawException = true;
	}
	std::cout << "exception test3 status: " << (sawException? "OK" : "fail") << "\n";
}


void testWeights(void)
{
	const DivisionModels2S<5,5> sample("/Users/ulman/p_Johannes/Polyclad/divisionModelsForEmbryoGen.dat",3.0f);

	const int fromThisNoOfRandomModels = 2;
	float weights[20][fromThisNoOfRandomModels];

	//sample.setFlatWeights<20,fromThisNoOfRandomModels>(weights);
	//sample.printWeights<20,fromThisNoOfRandomModels>(weights);

	//sample.setSineShiftingBoxWeights<20,fromThisNoOfRandomModels>(weights);
	sample.setSineShiftingGaussWeights<20,fromThisNoOfRandomModels>(weights);
	sample.printWeightsWithModelsVertically<20,fromThisNoOfRandomModels>(weights);
}


void testMixedModels(void)
{
	const DivisionModels2S<5,5> sample("/Users/ulman/p_Johannes/Polyclad/divisionModelsForEmbryoGen.dat",3.0f);

	const DivisionModels2S<5,5>::DivisionModel* models[2];
	models[0] = &sample.getModel(0);
	models[1] = &sample.getModel(1);

	float weights[10][2];
	//sample.setFlatWeights<10,2>(weights);
	sample.setSineShiftingGaussWeights<10,2>(weights);

	DivisionModels2S<5,5>::DivisionModel m;
	sample.getMixedFromGivenModels<10,2>(m,weights,models);

	models[0]->printModel();
	std::cout << "--------------\n";
	models[1]->printModel();
	std::cout << "--------------\n";
	m.printModel();
}


int main(void)
{
	//testInterpolation();
	//testInvalidRangeExceptions();
	testWeights();
	testMixedModels();

	return 0;
}
