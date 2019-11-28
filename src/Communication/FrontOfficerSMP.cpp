#include "../FrontOfficer.h"

void FrontOfficer::triggered_publishGeometry()
{
	//round robin calls to all my agents and their publishGeometry()
}
void FrontOfficer::triggered_renderNextFrame()
{
	//shall obtain images to render into
	//round robin calls to all my agents and their draw*()
	//shall use: nextFOsID
}
