#include <bmf.h>

void load_bmf(BMF *bmf, char *path){
	FILE *f = fopen(path,"rb");
	if (!f){
		fatal_error("load_bmf: failed to open %s",path);
	}
	if (1 != fread(&bmf->vertex_count,sizeof(bmf->vertex_count),1,f)){
		fatal_error("load_bmf: %s invalid bmf file",path);
	}
	if (bmf->vertex_count < 3 || bmf->vertex_count > 0xffff){
		fatal_error("load_bmf: %s invalid vertex count: %d",path,bmf->vertex_count);
	}
	bmf->vertices = malloc_or_die(bmf->vertex_count*sizeof(*bmf->vertices));
	if (bmf->vertex_count != fread(bmf->vertices,sizeof(*bmf->vertices),bmf->vertex_count,f)){
		fatal_error("load_bmf: %s vertex data ends prematurely",path);
	}
	if (1 != fread(&bmf->texture_group_count,sizeof(bmf->texture_group_count),1,f)){
		fatal_error("load_bmf: %s texture group count missing",path);
	}
	if (bmf->texture_group_count < 1 || bmf->texture_group_count > 0xffff){
		fatal_error("load_bmf: %s invalid texture group count",path);
	}
	bmf->texture_groups = malloc_or_die(bmf->texture_group_count * sizeof(*bmf->texture_groups));
	for (int i = 0; i < bmf->texture_group_count; i++){
		BMFTextureGroup *t = bmf->texture_groups+i;
		if (1 != fread(&t->namelen,sizeof(t->namelen),1,f)){
			fatal_error("load_bmf: %s missing texture name length",path);
		}
		if (t->namelen < 0 || t->namelen > sizeof(t->name)){
			fatal_error("load_bmf: %s texture group name length > %d",path,sizeof(t->name));
		}
		if (t->namelen != fread(t->name,sizeof(unsigned char),t->namelen,f)){
			fatal_error("load_bmf: %s missing texture group name",path);
		}
		if (1 != fread(&t->vertex_count,sizeof(t->vertex_count),1,f)){
			fatal_error("load_bmf: %s missing vertex count for texture group \"%.*s\"",path,t->namelen,t->name);
		}
		if (t->vertex_count < 1 || t->vertex_count > 0xffff){
			fatal_error("load_bmf: %s invalid vertex count for texture group \"%.*s\"",path,t->namelen,t->name);
		}
	}
	if (1 != fread(&bmf->box_collider_count,sizeof(bmf->box_collider_count),1,f)){
		fatal_error("load_bmf: %s missing box collider count",path);
	}
	if (bmf->box_collider_count < 0 || bmf->box_collider_count > 0xffff){
		fatal_error("load_bmf: %s invalid box collider count",path);
	}
	bmf->box_colliders = malloc_or_die(bmf->box_collider_count*sizeof(*bmf->box_colliders));
	for (int i = 0; i < bmf->box_collider_count; i++){
		BMFBoxCollider *b = bmf->box_colliders+i;
		if (3 != fread(b->half_extents,sizeof(float),3,f)){
			fatal_error("load_bmf: %s expected 3 float half extents",path);
		}
		if (4 != fread(b->quaternion,sizeof(float),4,f)){
			fatal_error("load_bmf: %s expected 4 float quaternion",path);
		}
		if (3 != fread(b->position,sizeof(float),3,f)){
			fatal_error("load_bmf: %s expected 3 float position",path);
		}
	}
	fclose(f);
}

void free_bmf(BMF *bmf){
	free(bmf->vertices);
	free(bmf->texture_groups);
	if (bmf->box_collider_count > 0){
		free(bmf->box_colliders);
	}
}