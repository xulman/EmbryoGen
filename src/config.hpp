#pragma once
#include "util/Vector3d.hpp"

namespace config {
namespace geometry {
/** accuracy of the geometry representation, choose float or double */
using precision_t = float;
} // namespace geometry
namespace scenario {

/** a subset of controls that are treated immutable in the scenario,
    it comes with own default values that are, however,
    changeable here alone but not when this is part of the scenario */
class ControlConstants {
  public:
	// CTC drosophila:
	// x,y,z res = 2.46306,2.46306,0.492369 px/um
	// embryo along x-axis
	// bbox size around the embryo: 480,220,220 um
	// bbox size around the embryo: 1180,540,110 px

	ControlConstants()
	    : sceneOffset(0.f), sceneSize(480.f, 220.f, 220.f),
	      imgRes(2.0f, 2.0f, 2.0f) {}

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

	/** export simulation status always after this amount of global time,
	   [min] should be multiple of incrTime to obtain regular sampling */
	float expoTime = 0.5f;

	/** output filename pattern in the printf() notation
	    that includes exactly one '%u' parameter: instance masks */
	const char* imgMask_filenameTemplate = "mask%03u.tif";

	/** output filename pattern in the printf() notation
	    that includes exactly one '%u' parameter: texture phantom images */
	const char* imgPhantom_filenameTemplate = "phantom%03u.tif";

	/** output filename pattern in the printf() notation
	    that includes exactly one '%u' parameter: optical indices images */
	const char* imgOptics_filenameTemplate = "optics%03u.tif";

	/** output filename pattern in the printf() notation
	    that includes exactly one '%u' parameter: final output images */
	const char* imgFinal_filenameTemplate = "finalPreview%03u.tif";
};
} // namespace scenario
} // namespace config
