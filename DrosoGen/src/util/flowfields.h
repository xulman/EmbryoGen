/**********************************************************************
*
* flowfields.h
*
* This file is part of MitoGen
*
* Copyright (C) 2013-2016 -- Centre for Biomedical Image Analysis (CBIA)
* http://cbia.fi.muni.cz/
*
* MitoGen is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* MitoGen is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with MitoGen. If not, see <http://www.gnu.org/licenses/>.
*
* Author: David Svoboda and Vladimir Ulman
*
* Description: Definition of basic datatypes.
*
***********************************************************************/


#ifndef MITOGEN_TOOLBOX_FLOWFIELDS_H
#define MITOGEN_TOOLBOX_FLOWFIELDS_H


/**
 * \ingroup toolbox
 *
 * This function transforms given image \e srcImg according to the given
 * vector flow field \e FF in the forward direction. This means that it
 * basically reads voxel value at certain position in the image \e srcImg
 * and places it in the \e dstImg at position shifted by a vector which is
 * found in the flow field at the same position. The shift vector may be,
 * however, any real number which renders the new position to be off the
 * voxel grid. In this case, the stored value is spread into surrounding voxels.
 *
 * The function assumes that vector flow field "fits" over the transformed
 * image \e srcImg perfectly, i.e. the size, resolution, and offset of the flow
 * fields are the same as of the \e srcImg. In this way, the flow field can be
 * easily applied. The flow field vectors are expected to represent the shift
 * in microns and are recalculated/projected to image coordinates using the image
 * resolution.
 *
 * The output image \e dstImg copies metadata information, inc. size, from
 * the source image \e srcImg. All voxel values are zeroed before the transformation.
 *
 * \param[in] srcImg	image to be transformed
 * \param[out] dstImg	result of the transformation
 * \param[in] FF	vector flow field encoding the transformation
 *
 * \author Vladimír Ulman
 * \date 2012
 */
template <class VT, class FT>
void ImageForwardTransformation(i3d::Image3d<VT> const &srcImg,
				i3d::Image3d<VT> &dstImg,
				FlowField<FT> const &FF);

/**
 * \ingroup toolbox
 *
 * A wrapper to the ConcatenateFlowFields() which first makes a copy of the
 * \e srcFF into \e resFF and then calls ConcatenateFlowFields(\e resFF, \e appFF).
 *
 * \param[in] srcFF	first motion flow field
 * \param[in] appFF	second motion flow field
 * \param[out] resFF	resulting, concatenated motion, flow field
 */
template <class FT>
void ConcatenateFlowFields(FlowField<FT> const &srcFF,
			   FlowField<FT> const &appFF,
			   FlowField<FT> &resFF);

/**
 * \ingroup toolbox
 *
 * This function concatenates \e appFF vector flow field to \e srcFF.
 * It simulates motion a of a pixel which is first moved along a vector
 * from \e srcFF to endup some place from which it is further moved along
 * a vector from \e appFF found at that place. A result of such movement
 * will be stored back in the \e srcFF flow field at the originating pixel position.
 *
 * \param[in,out] srcFF	first motion flow field which is changed into a concatenated one
 * \param[in] appFF	second motion flow field
 */
template <class FT>
void ConcatenateFlowFields(FlowField<FT> &srcFF,
			   FlowField<FT> const &appFF);

/**
 * \ingroup toolbox
 *
 * This function adds \e appFF vector flow field to \e srcFF. For every voxel, it just reads
 * vectors from both images, adds them together and store the result in the \e srcFF.
 * It simulates two motions happening at the same time.
 *
 * \param[in,out] srcFF	first motion flow field which is changed into a concatenated one
 * \param[in] appFF	second motion flow field
 */
template <class FT>
void AddFlowFields(FlowField<FT> &srcFF,
		   FlowField<FT> const &appFF);

/**
 * \ingroup toolbox
 *
 * A wrapper to the AddFlowFields() which first makes a copy of the
 * \e srcFF into \e resFF and then calls AddFlowFields(\e resFF, \e appFF).
 *
 * \param[in] srcFF	first motion flow field
 * \param[in] appFF	second motion flow field
 * \param[out] resFF	resulting, joined motion, flow field
 */
template <class FT>
void AddFlowFields(FlowField<FT> const &srcFF,
		   FlowField<FT> const &appFF,
		   FlowField<FT> &resFF);

/**
 * \ingroup toolbox
 *
 * The functions (re)inits the flow field \e FF to make it appropriate
 * for the given input image \e img. This means that size, resolution,
 * and offset of the flow field is copied from the \e img so that every
 * pixel positions in the flow field directly corresponds to pixel positions
 * in the source \e img.
 *
 * Original content of \e FF is lost. Flow fields (e.g., FlowField::x)
 * are allocated and zeroed.
 *
 * \param[in] img	template image
 * \param[out] FF	flow field to be (re)initialized
 */
template <class VT, class FT>
void InitFlowField(i3d::Image3d<VT> const &img, FlowField<FT> &FF);

/**
 * \ingroup toolbox
 *
 * Saves the flow field \e FF, if FF is not NULL, into several files.
 * If \e FF is NULL, the function immediately silently quits.
 *
 * The files are images containing the flow field \e FF element wise,
 * that is, one image with just x-elements of the flow field, another
 * image with just y-elements, possibly even an image with just z-elements.
 * A colour flow field visualization is saved as well, using the function
 * ColorRepresentationOf2DFlow().
 *
 * The names of the files are established using this pattern:
 * \verbatim
 * "%s_%s_X.ics", namePrefix, name
 * \endverbatim
 *
 * The 'X' is interchanged with 'Y', 'Z', and 'colour'.
 *
 * If \e namePrefix or \e name is NULL, '_' (underscore) is used instead.
 * Note that if both \e namePrefix and \e name are NULL, the function detects
 * it (of course, the detection takes place only if \e FF is not NULL) and
 * emits an exception.
 *
 * \param[in] FF		flow field to be saved
 * \param[in] namePrefix	first third of the filename
 * \param[in] name		middle third of the filename
 * \param[in] maxVectorLength		maximum length of vectors visualized in the flowfiled
 */
template <class FT>
void SaveFlowField(const FlowField<FT>* FF,
		   const char *namePrefix,
		   const char *name,
			float maxVectorLength);

/**
 * \ingroup toolbox
 *
 * The function adds a the same constant vector \e shift to every vector in the
 * vector flow field \e FF.
 *
 * Note that the function adds a vector, in contrast to
 * overwrite. In deed, the \e FF is not initialized at all.
 * In case a new flow field is to be created, one may consider using the
 * function InitFlowField().
 *
 * The function fills the whole flow field, every coordinate present in it.
 *
 * If \e eFF is not NULL, the function also fills this flow field provided
 * its pixel grid aligns with the one from \e FF. The \e eFF may overlay
 * over the \e FF freely. Parts of \e eFF outside the \e FF boundary are
 * filled with zero vectors. While \e FF may already contain something to which
 * the flow field created with this function is added, the \e eFF is always
 * filled with the added "increment" erasing any previous content of \e eFF.
 *
 * \param[in,out] FF		the flow field to be modified
 * \param[in] shift		the translation vector to be added
 * \param[in,out] eFF		the flow field to contain the export
 */
template <class FT>
void DescribeTranslation(FlowField<FT> &FF,
			 i3d::Vector3d<float> const &shift,
			 FlowField<FT>* const eFF=NULL);

/**
 * \ingroup toolbox
 *
 * The function adds translation vectors to every vector in the vector flow field \e FF.
 * Every translation vector at its position simulates rotational movement of the pixel
 * at the particular position in the xy plane along given \e centre by \e angle clockwise.
 * Since the \e centre point is given in microns with respect to the scene, the offset and
 * resolution of the flow field \e FF must be valid and reasonable.
 *
 * Note that the function adds a vector, in contrast to
 * overwrite. In deed, the \e FF is not initialized at all.
 * In case a new flow field is to be created, one may consider using the
 * function InitFlowField().
 *
 * The function fills the whole flow field, every coordinate present in it.
 *
 * If \e eFF is not NULL, the function also fills this flow field provided
 * its pixel grid aligns with the one from \e FF. The \e eFF may overlay
 * over the \e FF freely. Parts of \e eFF outside the \e FF boundary are
 * filled with zero vectors. While \e FF may already contain something to which
 * the flow field created with this function is added, the \e eFF is always
 * filled with the added "increment" erasing any previous content of \e eFF.
 *
 * \param[in,out] FF		the flow field to be modified
 * \param[in] centre		rotation centre, in absolute microns (=coordinates of the scene)
 * \param[in] angle		rotation angle, in radians, clockwise direction
 * \param[in,out] eFF		the flow field to contain the export
 */
template <class FT>
void DescribeRotation(FlowField<FT> &FF,
		      i3d::Vector3d<float> const &centre,
		      const float angle,
		      FlowField<FT>* const eFF=NULL);

/**
 * \ingroup toolbox
 *
 * The function adds "radially" distributed vectors to every vector in the vector
 * flow field \e FF.
 *
 * For every first \e BPOuterCnt points in the \e BPList, the point position is
 * projected to the flow field \e FF and a normalized (magnitude 1) vector pointing
 * from here towards the \e BPCentre is established. This established vector is multiplied
 * with some weighting function and the results are stored in the flow field at positions
 * along the line from \e BPCentre towards a position of the examined point from \e BPList.
 *
 * The weighting function is zero (0) in the \e BPCentre, increases with the distance from it,
 * reaches one (1) at the examined point from \e BPList and decreases further. The weighting
 * function is further multiplied with a value from the \e hintArray.
 *
 * As we are advancing in the \e BPList, we advance also in the \e hintArray. Advances in
 * the \e BPList are, however, in one-by-one fashion; in the \e hintArray advances are by
 * the value of \e hintInc. The length of the \e hintArray must be at least \e BPOuterCnt.
 * Note that a certain line from the Cell::scmPnHints image can be used or one may supply
 * just single constant while providing \e hintInc = 0 at the same time.
 *
 * With small cells (radius below 3-4 microns), there is rather strong disproportion
 * of volume added/removed to/from cell if a boundary point is supposed to move by given
 * number of microns. In other words, if we decide to move all boundary points radially
 * from a cell centre by X microns, the volume added in the end is actually larger than
 * what we can expect from just increasing radius by X. In order to achieve expected increase
 * of volume, one must compensate for it and move all boundary points radially from the
 * cell centre by a little bit less than X microns. The compensation can be turned on with
 * the last parameter \e compensation. The expected increase of volume is understood as
 * the number of (outer) boundary pixels times area of one pixel (depends on the image resolution)
 * times the increase (a height of the block) in microns. Yes, this is crude approximation.
 *
 * The function fills only portion of the flow field, in contrast to, e.g., DescribeTranslation().
 *
 * If \e eFF is not NULL, the function also fills this flow field provided
 * its pixel grid aligns with the one from \e FF. The \e eFF may overlay
 * over the \e FF freely. Parts of \e eFF outside the \e FF boundary are
 * filled with zero vectors. While \e FF may already contain something to which
 * the flow field created with this function is added, the \e eFF is always
 * filled with the added "increment" erasing any previous content of \e eFF.
 *
 * \param[in,out] FF		the flow field to be modified
 * \param[in] BPCentre		centre of "expansion", in microns
 * \param[in] BPOuterCnt	number of processed points in the \e BPList
 * \param[in] hintArray		additional weights to be applied
 * \param[in] hintInc		stepping of the hintArray
 * \param[in] compensation	whether to compensate for arc-segment effects
 * \param[in,out] eFF		the flow field to contain the export
 *
 * Default values for \e hintInc is 1, for \e compensation is false.
 */
template <class FT>
void DescribeRadialFlow(FlowField<FT> &FF,
			std::vector< i3d::Vector3d<float> /**/> const &BPList,
			i3d::Vector3d<float> const &BPCentre,
			const size_t BPOuterCnt,
			const float *hintArray,
			const int hintInc=1,
			const bool compensation=false,
			FlowField<FT>* const eFF=NULL);

/**
 * \ingroup toolbox
 *
 * Helper function for visualization of 2D flow fields. It creates an RGB image
 * \e Output of the same size as the input vector flow field \e FF. In this image,
 * colour of every corresponding pixel determines azimuth in XY of the flow vector and
 * intensity encodes the magnitude of the vector (the ligher, the longer). The
 * lightest intensity corresponds to vectors of the magnitude \e normalization.
 * Vectors are expected to express lengths in microns, image resolution is ignored.
 *
 * The FF.z is ignored.
 *
 * \param[out] Output		output colour vizualization
 * \param[in] FF		input flow field
 * \param[in] normalization	maximum vector magnitude
 *
 * \note The function uses piece of code originally written
 * by Andres Bruhn, University of Saarland.
 */
void ColorRepresentationOf2DFlow(i3d::Image3d<i3d::RGB> &Output, FlowField<float> const &FF,
				const float normalization);

/**
 * \ingroup toolbox
 *
 * \brief Spread the value to the nearest 4 or 8 (8 if GetSizeZ() > 1) voxels.
 *
 *  Example of the spreading schema for 2D situation:
 * \anchor imagegrid_interpolations
 * \verbatim
 *  Let |,-,+ denote borders between adjacent voxels.
 *  Let . denote an outline of the voxel given by real coordinate.
 *  Let x denote the real-valued coordinate (the centre of the voxel outlined with .).
 *  Let o denote the nearest integer coordinate to the x coordinate.
 *
 *  +-----------+-----------+
 *  | 2         |        1  |
 *  |  .............        |
 *  |  .  o     |  .        |   -------- integer line
 *  |  .        |  .        |
 *  |  .  B  x  |A .        |
 *  +--.--------+--.--------+
 *  |  .  C     |D .        |
 *  |  .............        |
 *  |           |           |   -------- integer line
 *  |           |           |
 *  | 3         |         4 |
 *  +-----------+-----------+
 *
 *        |           |
 *        |           |
 *        |           |
 *        ------------------------------ integer line
 * \endverbatim
 *
 * In the nearest neighbor case, the integer-valued center x would be positioned in the
 * center of, let's say, voxel 2 (as center x is nearest to the centre of voxel 2) and
 * the whole value will be stored in the voxel 2. Instead, in this function only portion
 * A of the inserted value is stored in voxel 1, portion B in the voxel 2, etc. All portions
 * sum up to exactly the inserted \e value. In other words, it splits the \e value according
 * to the areas A, B, C and D and stores it in the voxels 1, 2, 3 and 4, respectively (for 2D).
 *
 * The PutPixel function actually adds given \e value since several real-positioned
 * voxels can contribute to the respective integer-valued positions.
 *
 * \param[in,out] img		image to be modified
 * \param[in] x			real-valued pixel x-position of the inserted voxel
 * \param[in] y			real-valued pixel y-position of the inserted voxel
 * \param[in] z			real-valued pixel z-position of the inserted voxel
 * \param[in] value		inserted voxel value
 *
 * \author Vladimír Ulman
 * \date 2007
 */
template <class VT>
void PutPixel(i3d::Image3d<VT> &img,const float x,const float y,const float z,const VT value);

/**
 * \ingroup toolbox
 *
 * Returns a value from the \e img at some real valued pixel coordinate [\e x, \e y, \e z].
 *
 * Bi-linear, tri-linear in the case of 3D, interpolation is used, see \ref imagegrid_interpolations
 * "interpolation at pixel grid", to obtain values off the pixel grid.
 * If the asked coordinate is outside the image, zero is returned.
 *
 * \param[in] img		image to be read
 * \param[in] x			real-valued pixel x-position of the queried voxel
 * \param[in] y			real-valued pixel y-position of the queried voxel
 * \param[in] z			real-valued pixel z-position of the queried voxel
 */
template <class VT>
VT GetPixel(i3d::Image3d<VT> const &img,const float x,const float y,const float z);

/**
 * \ingroup toolbox
 *
 * This structure stores hints how to "copy from VOI" for x,y, and z-axis.
 * Here's how:
 *
 * \verbatim
 * example for the x-axis:
 * |---------------|----------------|----------------|
 * |               |                |                |
 * |->  skipA.x  <-|->   work.x   <-|->   skipB.x  <-|
 * Total length of the three sections yields the lenght of the whole line.
 * \endverbatim
 *
 * It can be filled by the IMCopyFromVOI_GetHints() function.
 */
struct IMCopyFromVOI_t {
  public:
	///empty constructor
  	IMCopyFromVOI_t() : offset(0,0,0),
			    skipA(0,0,0),
			    work(0,0,0),
			    skipB(0,0,0) {}

	///offset to the corner of the image to copy from
	i3d::Vector3d<int> offset;

	///how many pixels to skip at the beginning
	i3d::Vector3d<int> skipA;

	///how many pixels to process
	i3d::Vector3d<int> work;

	///how many pixels to skip in the end
	i3d::Vector3d<int> skipB;
};

/**
 * \ingroup toolbox
 *
 * Computes hints how to perform "copy from VOI", when \e fImg is the
 * one to be filled from \e gImg when the VOI is their intersection.
 * See the outline in docs of the IMCopyFromVOI_t structure.
 *
 * The \e hints should tell which and how many pixels to skip and which
 * to "use" in \e fImg such that couterparting pixels from \e gImg are
 * accessed and boundaries are not violated.
 *
 * The two input images should be aligned: they should be of the same
 * resolution and their pixel grids should overlay (difference of their
 * offsets should be integer multiple in pixel units).
 * If the \e gImg and \e fImg are images from some flow fields, the
 * FlowField::isAligned() should return true on them.
 *
 * \param[in]	gImg	a "global" image
 * \param[in]	fImg	a "floating" image
 * \param[out]	hints	the hints structure to be filled
 */
template <class VOXEL>
void IMCopyFromVOI_GetHints(i3d::Image3d<VOXEL> const &gImg,
			    i3d::Image3d<VOXEL> const &fImg,
			    IMCopyFromVOI_t &hints);

/**
 * \ingroup toolbox
 *
 * Performs the "copy from VOI" as explained in IMCopyFromVOI_GetHints().
 * In the "skip" sections (see the outline in docs of the IMCopyFromVOI_t
 * structure) the \e skipValue is used, if it is not NULL. If it is NULL,
 * nothing is changed in the "skip" sections.
 *
 * The two images should be aligned (see the IMCopyFromVOI_GetHints()) and allocated.
 *
 * \param[in]	gImg		a "global" image
 * \param[in,out] fImg		a "floating" image
 * \param[in]	hints		the hints structure to be filled
 * \param[in]	skipValue	pointer to the default value used in "skip" sections
 */
template <class VOXEL>
void IMCopyFromVOI_Apply(i3d::Image3d<VOXEL> const &gImg,
			 i3d::Image3d<VOXEL> &fImg,
			 IMCopyFromVOI_t const &hints,
			 const VOXEL *skipValue=NULL);

/**
 * \ingroup toolbox
 *
 * The same as IMCopyFromVOI_Apply(), which performs a "copy from VOI" operation,
 * when applied on an image with constant value \e setValue.
 * In the "skip" sections (see the outline in docs of the IMCopyFromVOI_t
 * structure) the \e skipValue is used, if it is not NULL. If it is NULL,
 * nothing is changed in the "skip" sections.
 *
 * \param[in,out] fImg		a "floating" image
 * \param[in]	hints		the hints structure to be filled
 * \param[in]   setValue	value to use in the "work" sections
 * \param[in]	skipValue	pointer to the default value used in "skip" sections
 */
template <class VOXEL>
void IMCopyFromVOI_Set(i3d::Image3d<VOXEL> &fImg,
		       IMCopyFromVOI_t const &hints,
		       const VOXEL setValue,
		       const VOXEL *skipValue=NULL);

#endif
