#include "../util/report.h"
#include "Geometry.h"
#include "Mesh.h"
#include "ScalarImg.h"
#include "Spheres.h"
#include "VectorImg.h"


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
	Vector3d<G_FLOAT> p(minCorner);
	p.toPixels(img.GetResolution().GetRes(),img.GetOffset());

	//make sure minCorner is within the image
	p.elemMax(Vector3d<G_FLOAT>(0));
	p.elemMin(Vector3d<G_FLOAT>(img.GetSize()));

	//obtain integer px coordinate that is already intersected with image dimensions
	minSweep.toPixels(p);

	//p is maxCorner in img's px coordinates
	p = maxCorner;
	p.toPixels(img.GetResolution().GetRes(),img.GetOffset());

	//make sure minCorner is within the image
	p.elemMax(Vector3d<G_FLOAT>(0));
	p.elemMin(Vector3d<G_FLOAT>(img.GetSize()));

	//obtain integer px coordinate that is already intersected with image dimensions
	maxSweep.toPixels(p);
}


/** returns SQUARED shortest distance along any axis between this and
    the given AABB, or 0.0 if they intersect */
G_FLOAT AxisAlignedBoundingBox::minDistance(const AxisAlignedBoundingBox& AABB) const
{
	G_FLOAT M = std::max(minCorner.x,AABB.minCorner.x);
	G_FLOAT m = std::min(maxCorner.x,AABB.maxCorner.x);
	G_FLOAT dx = M > m ? M-m : 0; //min dist along x-axis

	M = std::max(minCorner.y,AABB.minCorner.y);
	m = std::min(maxCorner.y,AABB.maxCorner.y);
	G_FLOAT dy = M > m ? M-m : 0; //min dist along y-axis

	M = std::max(minCorner.z,AABB.minCorner.z);
	m = std::min(maxCorner.z,AABB.maxCorner.z);
	G_FLOAT dz = M > m ? M-m : 0; //min dist along z-axis

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

/*static*/ Geometry * Geometry::createAndDeserializeFrom(int g_type, char * buffer) 
{
		switch(g_type)
		{
			case ListOfShapeForms::Spheres:
				return Spheres::createAndDeserializeFrom(buffer);
			case ListOfShapeForms::Mesh:
				return Mesh::createAndDeserializeFrom(buffer);
			case ListOfShapeForms::ScalarImg:
				return ScalarImg::createAndDeserializeFrom(buffer);
			case ListOfShapeForms::VectorImg:
				return VectorImg::createAndDeserializeFrom(buffer);
			case ListOfShapeForms::undefGeometry:
			default:
				return NULL;
		}
}
