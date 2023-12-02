#pragma once

#include <zlib.h>
#include <tinydir.h>

#include <renderer.h>
#include <perlin_noise.h>

TSTRUCT(BlockType){
	char *name;
	bool transparent;
	int faces[6][2];
};

BlockType block_types[];

typedef enum {
	BLOCK_AIR,
	BLOCK_BEDROCK,
	BLOCK_STONE,
	BLOCK_DIRT,
	BLOCK_GRASS,
	BLOCK_LOG,
	BLOCK_GLASS,
	BLOCK_BRICK,
} BlockId;

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 256
TSTRUCT(Block){
	uint8_t id, r, g, b;
};
TSTRUCT(Chunk){
	bool neighbors_exist[4];
	Block blocks[CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT];
	GPUMesh mesh;
};

TSTRUCT(ChunkLinkedHashListBucket){
	ChunkLinkedHashListBucket *prev, *next;
	ivec2 position;
	Chunk *chunk;
};

TSTRUCT(ChunkLinkedHashList){
	size_t total, used, tombstones;
	ChunkLinkedHashListBucket *buckets, *first, *last;
};

#define TOMBSTONE UINTPTR_MAX

ChunkLinkedHashListBucket *ChunkLinkedHashListGet(ChunkLinkedHashList *list, ivec2 position);

ChunkLinkedHashListBucket *ChunkLinkedHashListGetChecked(ChunkLinkedHashList *list, ivec2 position);

void ChunkLinkedHashListRemove(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b);

ChunkLinkedHashListBucket *ChunkLinkedHashListNew(ChunkLinkedHashList *list, ivec2 position);

#define BLOCK_AT(x,y,z) ((y)*CHUNK_WIDTH*CHUNK_WIDTH + (z)*CHUNK_WIDTH + (x))

void gen_chunk(ChunkLinkedHashListBucket *b);

extern vec3 cube_verts[];

extern GPUMesh block_outline;

void init_block_outline();

void append_block_face(TextureColorVertexList *tvl, int x, int y, int z, int face_id, BlockType *bt);

void mesh_chunk(ChunkLinkedHashList *list, ChunkLinkedHashListBucket *b);

TSTRUCT(Region){
	FILE *file_ptr;
	struct {
		int sector_number;
		int sector_count;
	} chunk_positions[32*32];
	int sector_count;
	bool *sector_usage;
};

TSTRUCT(World){
	ChunkLinkedHashList chunks;
};

Block *get_block(World *w, ivec3 pos);

bool set_block(World *w, ivec3 pos, int id);

Block *cast_ray_into_blocks(World *w, vec3 origin, vec3 ray, float *t, ivec3 block_pos, ivec3 face_normal);