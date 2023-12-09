#pragma once

#include <base.h>
#include <cglm/cglm.h>

TSTRUCT(BMFVertex){
	vec3 position;
	vec3 normal;
	vec2 texcoord;
};

TSTRUCT(BMFTextureGroup){
	unsigned char namelen;
	char name[32];
	int vertex_count;
};

TSTRUCT(BMFBoxCollider){
	vec3 half_extents;
	vec4 quaternion;
	vec3 position;
};

TSTRUCT(BMF){
	int vertex_count;
	BMFVertex *vertices;

	int texture_group_count;
	BMFTextureGroup *texture_groups;

	int box_collider_count;
	BMFBoxCollider *box_colliders;
};

void load_bmf(BMF *bmf, char *path);

void free_bmf(BMF *bmf);