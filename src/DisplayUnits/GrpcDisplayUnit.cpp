#include "GrpcDisplayUnit.hpp"

void GrpcDisplayUnit::DrawPoint(const int /* ID */,
                                const Vector3d<float>& /* pos */,
                                const float /* radius = 1.0f */,
                                const int /* color = 0 */) /* override */ {}

void GrpcDisplayUnit::DrawLine(const int /* ID */,
                               const Vector3d<float>& /* posA */,
                               const Vector3d<float>& /* posB */,
                               const int /* color = 0*/) /* override */ {}

void GrpcDisplayUnit::DrawVector(const int /* ID */,
                                 const Vector3d<float>& /* pos */,
                                 const Vector3d<float>& /* vector */,
                                 const int /* color = 0 */) /* override */ {}

void GrpcDisplayUnit::DrawTriangle(const int /* ID */,
                                   const Vector3d<float>& /* posA */,
                                   const Vector3d<float>& /* posB */,
                                   const Vector3d<float>& /* posC */,
                                   const int /* color = 0 */) /* override */ {}