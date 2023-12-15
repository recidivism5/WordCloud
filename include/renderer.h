#pragma once

#include <base.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <png.h>
#include <jpeglib.h>    
#include <jerror.h>
#include <cglm/cglm.h>
#include <ft2build.h>
#include FT_FREETYPE_H

TSTRUCT(Image){
	int width,height;
	uint32_t *pixels;
};

TSTRUCT(Image8){
	int width,height;
	uint8_t *pixels;
};

TSTRUCT(Texture){
	GLuint id;
	int width, height;
};

TSTRUCT(GPUMesh){
	GLuint vao, vbo;
	int vertex_count;
};

TSTRUCT(ColorVertex){
	vec3 position;
	uint32_t color;
};

TSTRUCT(ColorShader){
	char *vert_src;
	char *frag_src;
	GLuint id;
	GLint aPosition;
	GLint aColor;
	GLint uMVP;
} color_shader;

TSTRUCT(TextureColorVertex){
	vec3 position;
	vec2 texcoord;
	uint32_t color;
};

TSTRUCT(TextureColorVertexList){
	int total, used;
	TextureColorVertex *elements;
};

TSTRUCT(TextureColorShader){
	char *vert_src;
	char *frag_src;
	GLuint id;
	GLint aPosition;
	GLint aTexCoord;
	GLint aColor;
	GLint uMVP;
	GLint uTex;
} texture_color_shader;

TSTRUCT(RoundedRectVertex){
	float Position[3];
	float Rectangle[4];
	float RoundingRadius;
	uint32_t color;
	uint32_t IconColor;
};

TSTRUCT(RoundedRectVertexList){
	int total, used;
	RoundedRectVertex *elements;
};

enum RoundedRectangleType {
	RR_CHANNEL=51,
	RR_FLAT=102,
	RR_DISH=153,
	RR_CONE=204
};

enum RoundedRectangleIconType {
	RR_ICON_NONE=36,
	RR_ICON_PLAY=72,
	RR_ICON_REVERSE_PLAY=108,
	RR_ICON_PAUSE=144,
	RR_ICON_NEXT=180,
	RR_ICON_PREVIOUS=216,
};

TSTRUCT(RoundedRectShader){
	char *vert_src;
	char *frag_src;
	GLuint id;
	GLint aPosition;
	GLint aRectangle;
	GLint aRoundingRadius;
	GLint aColor;
	GLint aIconColor;
	GLint proj;
} rounded_rect_shader;

void load_image(Image *img, char *path);

GLenum glCheckError_(const char *file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void texture_from_image(Texture *t, Image *i);

void texture_from_file(Texture *t, char *path);

void delete_texture(Texture *t);

void check_shader(char *name, char *type, GLuint id);

void check_program(char *name, char *status_name, GLuint id, GLenum param);

GLuint compile_shader(char *name, char *vert_src, char *frag_src);

TextureColorVertex *TextureColorVertexListMakeRoom(TextureColorVertexList *list, int count);

RoundedRectVertex *RoundedRectVertexListMakeRoom(RoundedRectVertexList *list, int count);

void append_rounded_rect(RoundedRectVertexList *verts, int x, int y, int z, int halfWidth, int halfHeight, float RoundingRadius, uint32_t color, uint32_t IconColor);

void gpu_mesh_from_color_verts(GPUMesh *m, ColorVertex *verts, int count);

void gpu_mesh_from_texture_color_verts(GPUMesh *m, TextureColorVertex *verts, int count);

void gpu_mesh_from_rounded_rect_verts(GPUMesh *m, RoundedRectVertex *verts, int count);

void delete_gpu_mesh(GPUMesh *m);

void compile_shaders();

void new_image(Image *i, int width, int height);

void blit_8_to_32(Image8 *src, int sx, int sy, int swidth, int sheight, Image *dst, int dx, int dy, uint32_t color);

void draw_string(Image *dst, int x, int y, FT_Face font_face, int font_height, uint32_t color, int char_count, char *string);

void draw_string_centered(Image *dst, int x, int y, FT_Face font_face, int font_height, uint32_t color, int char_count, char *string);