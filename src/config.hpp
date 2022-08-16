#pragma once
#include "util/Vector3d.hpp"
#include <fmt/core.h>
#include <string>

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

	bool outputToTiff = true;
	bool outputToDatastore = true;

	/** output filename pattern in the std::format notation
	    that includes exactly one 'unsigned' parameter: instance masks */
	fmt::format_string<int&> imgMask_filenameTemplate = "mask{:03}.tif";

	/** output filename pattern in the std::format notation
	    that includes exactly one 'unsigned' parameter: texture phantom images
	 */
	fmt::format_string<int&> imgPhantom_filenameTemplate = "phantom{:03}.tif";

	/** output filename pattern in the std::format notation
	    that includes exactly one 'unsigned' parameter: optical indices images
	 */
	fmt::format_string<int&> imgOptics_filenameTemplate = "optics{:03}.tif";

	/** output filename pattern in the std::format notation
	    that includes exactly one 'unsigned' parameter: final output images */
	fmt::format_string<int&> imgFinal_filenameTemplate =
	    "finalPreview{:03}.tif";

	std::string ds_serverAddress = "127.0.0.1";
	int ds_port = 9080;
	std::string ds_datasetUUID = "59c4e076-c4c8-4703-b5ee-fbfdb47d340d";
	std::size_t ds_maskChannel = 0;
	std::size_t ds_phantomChannel = 1;
	std::size_t ds_opticsChannel = 2;
	std::size_t ds_finalChannel = 3;
};
} // namespace scenario

} // namespace config
