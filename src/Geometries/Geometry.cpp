#include "../util/report.h"
#include "Geometry.h"

template <typename T>
void AxisAlignedBoundingBox::adaptImage(i3d::Image3d<T>& img,
                                        const Vector3d<float>& res,
                                        const Vector3d<short>& pxFrameWidth) const
{
	const Vector3d<float> umFrameWidth( Vector3d<float>().from(pxFrameWidth).elemDivBy(res) );

	Vector3d<float> tmp(minCorner.to<float>());
	tmp -= umFrameWidth;

	img.SetResolution( i3d::Resolution(res.toI3dVector3d()) );
	img.SetOffset( tmp.toI3dVector3d() );

	tmp.from(maxCorner-minCorner);
	tmp += 2.0f * umFrameWidth;
	tmp.elemMult(res).elemCeil();
	img.MakeRoom( tmp.to<size_t>().toI3dVector3d() );

	DEBUG_REPORT("from AABB: minCorner=" << minCorner << " um --> maxCorner=" << maxCorner << " um");
	DEBUG_REPORT(" to image: imgOffset=" << img.GetOffset() << " um, imgSize="
	             << img.GetSize() << " px, imgRes=" << res << " px/um");
}


template <typename T>
void AxisAlignedBoundingBox::exportInPixelCoords(const i3d::Image3d<T>& img,
                                                 Vector3d<size_t>& minSweep,
                                                 Vector3d<size_t>& maxSweep) const
{
	//p is minCorner in img's px coordinates
	Vector3d<FLOAT> p(minCorner);
	p.toPixels(img.GetResolution().GetRes(),img.GetOffset());

	//make sure minCorner is within the image
	p.elemMax(Vector3d<FLOAT>(0));
	p.elemMin(Vector3d<FLOAT>(img.GetSize()));

	//obtain integer px coordinate that is already intersected with image dimensions
	minSweep.toPixels(p);

	//p is maxCorner in img's px coordinates
	p = maxCorner;
	p.toPixels(img.GetResolution().GetRes(),img.GetOffset());

	//make sure minCorner is within the image
	p.elemMax(Vector3d<FLOAT>(0));
	p.elemMin(Vector3d<FLOAT>(img.GetSize()));

	//obtain integer px coordinate that is already intersected with image dimensions
	maxSweep.toPixels(p);
}


/** returns SQUARED shortest distance along any axis between this and
    the given AABB, or 0.0 if they intersect */
FLOAT AxisAlignedBoundingBox::minDistance(const AxisAlignedBoundingBox& AABB) const
{
	FLOAT M = std::max(minCorner.x,AABB.minCorner.x);
	FLOAT m = std::min(maxCorner.x,AABB.maxCorner.x);
	FLOAT dx = M > m ? M-m : 0; //min dist along x-axis

	M = std::max(minCorner.y,AABB.minCorner.y);
	m = std::min(maxCorner.y,AABB.maxCorner.y);
	FLOAT dy = M > m ? M-m : 0; //min dist along y-axis

	M = std::max(minCorner.z,AABB.minCorner.z);
	m = std::min(maxCorner.z,AABB.maxCorner.z);
	FLOAT dz = M > m ? M-m : 0; //min dist along z-axis

	return (dx*dx + dy*dy + dz*dz);
}


// ------------- explicit instantiations -------------
template
void AxisAlignedBoundingBox::adaptImage(i3d::Image3d<i3d::GRAY8>& img,
                                        const Vector3d<float>& res,
                                        const Vector3d<short>& pxFrameWidth) const;
template
void AxisAlignedBoundingBox::adaptImage(i3d::Image3d<i3d::GRAY16>& img,
                                        const Vector3d<float>& res,
                                        const Vector3d<short>& pxFrameWidth) const;
template
void AxisAlignedBoundingBox::adaptImage(i3d::Image3d<float>& img,
                                        const Vector3d<float>& res,
                                        const Vector3d<short>& pxFrameWidth) const;
template
void AxisAlignedBoundingBox::adaptImage(i3d::Image3d<double>& img,
                                        const Vector3d<float>& res,
                                        const Vector3d<short>& pxFrameWidth) const;

template
void AxisAlignedBoundingBox::exportInPixelCoords(const i3d::Image3d<i3d::GRAY8>& img,
                                                 Vector3d<size_t>& minSweep,
                                                 Vector3d<size_t>& maxSweep) const;
template
void AxisAlignedBoundingBox::exportInPixelCoords(const i3d::Image3d<i3d::GRAY16>& img,
                                                 Vector3d<size_t>& minSweep,
                                                 Vector3d<size_t>& maxSweep) const;
template
void AxisAlignedBoundingBox::exportInPixelCoords(const i3d::Image3d<float>& img,
                                                 Vector3d<size_t>& minSweep,
                                                 Vector3d<size_t>& maxSweep) const;
template
void AxisAlignedBoundingBox::exportInPixelCoords(const i3d::Image3d<double>& img,
                                                 Vector3d<size_t>& minSweep,
                                                 Vector3d<size_t>& maxSweep) const;
