#pragma once

#include <renderer.h>

TSTRUCT(Shape){
	int position_count;
	vec3 *positions;

	int index_count;
	int *indices;

	GPUMesh gpu_mesh;
};

TSTRUCT(Simplex){
	int position_count;
	vec3 positions[4];
};

bool gjk(Shape *a, Shape *b);