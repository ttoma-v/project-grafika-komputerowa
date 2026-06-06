#pragma once

#include "Mesh.h"
#include "PTF.h"

#include <vector>

namespace Geometry {
Mesh makePlane(float width, float depth, int subdivisions);
Mesh makeLowPolyCoral(float scale, unsigned int seed);
Mesh makeKelpSegment();
Mesh makeKelpAlongSpline(const ParallelTransportSpline& spline, float maxWidth = 0.08f);
Mesh makeRock(float scale, unsigned int seed);
void computeTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
void addDoubleSidedFaces(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
}
