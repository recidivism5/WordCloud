#include <renderer.h>

static void load_jpeg(Image *img, char *path){
	FILE *file = fopen(path,"rb");
	if (!file){
		fatal_error("load_jpeg: failed to open %s",path);
	}
	struct jpeg_decompress_struct info;
	struct jpeg_error_mgr err;

	info.err = jpeg_std_error(&err);
	jpeg_create_decompress(&info);

	jpeg_stdio_src(&info,file);
	jpeg_read_header(&info,TRUE);
	jpeg_start_decompress(&info);

	img->width = info.output_width;
	img->height = info.output_height;
	img->pixels = malloc_or_die(img->width*img->height*sizeof(*img->pixels));
	switch (info.num_components){
	case 1:{
		uint8_t *temp = malloc_or_die(img->width);
		for (int y = 0; y < img->height; y++){
			jpeg_read_scanlines(&info,&temp,1);
			for (int x = 0; x < img->width; x++){
				uint8_t t = temp[x];
				img->pixels[y*img->width+x] = RGBA(t,t,t,255);
			}
		}
		free(temp);
		break;
	}
	case 3:{
		uint8_t *temp = malloc_or_die(img->width*3);
		for (int y = 0; y < img->height; y++){
			jpeg_read_scanlines(&info,&temp,1);
			for (int x = 0; x < img->width; x++){
				uint8_t *t = temp+x*3;
				img->pixels[y*img->width+x] = RGBA(t[0],t[1],t[2],255);
			}
		}
		free(temp);
		break;
	}
	default:{
		fatal_error("load_jpeg: %s invalid component count (%d)",path,info.num_components);
	}
	}

	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);
	fclose(file);
}

static void load_png(Image *img, char *path){
	//from https://gist.github.com/Svensational/6120653
	png_image image = {0};
	image.version = PNG_IMAGE_VERSION;
	if (!png_image_begin_read_from_file(&image,path)){
		fatal_error("texture_from_file: failed to load %s",path);
	}
	image.format = PNG_FORMAT_RGBA;
	png_byte *buffer = malloc_or_die(PNG_IMAGE_SIZE(image));
	// a negative stride indicates that the bottom-most row is first in the buffer
	// (as expected by openGL)
	if (!png_image_finish_read(&image, NULL, buffer, -image.width*4, NULL)) {
		png_image_free(&image);
		fatal_error("texture_from_file: failed to load %s",path);
	}
	img->width = image.width;
	img->height = image.height;
	img->pixels = buffer;
}

void load_image(Image *img, char *path){
	int len = strlen(path);
	if (len < 5){
		fatal_error("texture_from_file: invalid path: %s",path);
	}
	if (!memcmp(path+len-4,".jpg",4)){
		load_jpeg(img,path);
	} else if (!memcmp(path+len-4,".png",4)){
		load_png(img,path);
	} else {
		fatal_error("texture_from_file: invalid file extension: %s. Expected .png/.jpg");
	}
}

GLenum glCheckError_(const char *file, int line){
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR){
		char *error;
		switch (errorCode){
		case GL_INVALID_ENUM:      error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:     error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:     error = "OUT_OF_MEMORY"; break;
		default: error = "UNKNOWN TYPE BEAT";break;
		}
		fatal_error("%s %s (%d)",error,file,line);
	}
	return errorCode;
}

void texture_from_image(Texture *t, Image *i){
	t->width = i->width;
	t->height = i->height;
	glGenTextures(1,&t->id);
	glBindTexture(GL_TEXTURE_2D,t->id);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t->width,t->height,0,GL_RGBA,GL_UNSIGNED_BYTE,i->pixels);
}

void texture_from_file(Texture *t, char *path){
	Image img;
	load_image(&img,path);
	texture_from_image(t,&img);
	free(img.pixels);
}

void delete_texture(Texture *t){
	glDeleteTextures(1,&t->id);
	memset(t,0,sizeof(*t));
}

void check_shader(char *name, char *type, GLuint id){
	GLint result;
	glGetShaderiv(id,GL_COMPILE_STATUS,&result);
	if (!result){
		char infolog[512];
		glGetShaderInfoLog(id,512,NULL,infolog);
		fatal_error("%s %s shader compile error: %s",name,type,infolog);
	}
}

void check_program(char *name, char *status_name, GLuint id, GLenum param){
	GLint result;
	glGetProgramiv(id,param,&result);
	if (!result){
		char infolog[512];
		glGetProgramInfoLog(id,512,NULL,infolog);
		fatal_error("%s shader %s error: %s",name,status_name,infolog);
	}
}

GLuint compile_shader(char *name, char *vert_src, char *frag_src){
	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(v,1,&vert_src,NULL);
	glShaderSource(f,1,&frag_src,NULL);
	glCompileShader(v);
	check_shader(name,"vertex",v);
	glCompileShader(f);
	check_shader(name,"fragment",f);
	GLuint p = glCreateProgram();
	glAttachShader(p,v);
	glAttachShader(p,f);
	glLinkProgram(p);
	check_program(name,"link",p,GL_LINK_STATUS);
	glDeleteShader(v);
	glDeleteShader(f);
	return p;
}

#define COMPILE_SHADER(s) s.id = compile_shader(#s,s.vert_src,s.frag_src)
#define GET_ATTRIB(s,a) s.a = glGetAttribLocation(s.id,#a)
#define GET_UNIFORM(s,u) s.u = glGetUniformLocation(s.id,#u)

struct ColorShader color_shader = {
	"#version 330 core\n"
	"in vec3 aPosition;\n"
	"in vec4 aColor;\n"
	"uniform mat4 uMVP;\n"
	"out vec4 vColor;\n"
	"void main(){\n"
	"	gl_Position = uMVP * vec4(aPosition,1.0f);\n"
	"	vColor = aColor;\n"
	"}",

	"#version 330 core\n"
	"in vec4 vColor;\n"
	"out vec4 FragColor;\n"
	"void main(){\n"
	"	FragColor = vColor;\n"
	"}"
};

struct TextureColorShader texture_color_shader = {
	"#version 330 core\n"
	"in vec3 aPosition;\n"
	"in vec2 aTexCoord;\n"
	"in vec4 aColor;\n"
	"uniform mat4 uMVP;\n"
	"out vec2 vTexCoord;\n"
	"out vec4 vColor;\n"
	"void main(){\n"
	"	gl_Position = uMVP * vec4(aPosition,1.0);\n"
	"	vTexCoord = aTexCoord;\n"
	"	vColor = aColor;\n"
	"}",

	"#version 330 core\n"
	"uniform sampler2D uTex;\n"
	"in vec2 vTexCoord;\n"
	"in vec4 vColor;\n"
	"out vec4 FragColor;\n"
	"void main(){\n"
	"	vec4 s = texture(uTex,vTexCoord);\n"
	"	FragColor = s * vColor;\n"
	"}"
};

RoundedRectShader rounded_rect_shader = {
	"#version 330 core\n"
	"in vec3 aPosition;\n"
	"in vec4 aRectangle;\n"//xy: center, zw: half extents
	"in float aRoundingRadius;\n"
	"in vec4 aColor;\n"
	"in vec4 aIconColor;\n"
	"uniform mat4 proj;\n"
	"out vec4 Rectangle;\n"
	"out float RoundingRadius;\n"
	"out vec4 Color;\n"
	"out vec4 IconColor;\n"
	"void main(){\n"
	"	gl_Position = proj * vec4(aPosition,1.0);\n"
	"	Rectangle = aRectangle;\n"
	"	RoundingRadius = aRoundingRadius;\n"
	"	Color = aColor;\n"
	"	IconColor = aIconColor;\n"
	"}",

	"#version 330 core\n"
	"#define PI 3.14159265\n"
	"in vec4 Rectangle;\n"
	"in float RoundingRadius;\n"
	"in vec4 Color;\n"//alpha selects shading type
	"in vec4 IconColor;\n"//alpha selects icon type
	"out vec4 FragColor;\n"
	"float DistanceAABB(vec2 p, vec2 he, float r){\n"//based on qmopey's shader: https://www.shadertoy.com/view/cts3W2
	"	vec2 d = abs(p) - he + r;\n"
	"	return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;\n"
	"}\n"
	// The MIT License
	// sdEquilateralTriangle Copyright  2017 Inigo Quilez https://www.shadertoy.com/view/Xl2yDW
	// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	"float DistanceEquilateralTriangle(vec2 p, float r){\n"
	"	float k = sqrt(3.0);\n"
	"	p.x = abs(p.x) - r;\n"
	"	p.y = p.y + r/k;\n"
	"	if( p.x+k*p.y>0.0 ) p=vec2(p.x-k*p.y,-k*p.x-p.y)/2.0;\n"
	"	p.x -= clamp( p.x, -2.0*r, 0.0 );\n"
	"	if (p.y > 0.0) return -length(p);\n"
	"	else return length(p);\n"
	"}\n"
	"vec2 rotate(vec2 p, float angle){\n"
	"	float c = cos(angle);\n"
	"	float s = sin(angle);\n"
	"	mat2 m = mat2(c,-s,s,c);\n"
	"	return m * p;\n"
	"}\n"
	"void main(){\n"
	"	vec2 p = gl_FragCoord.xy-Rectangle.xy;\n"
	"	vec2 pn = p;\n"
	"	if (pn.x < 0.0) pn.x += min(abs(pn.x),Rectangle.z-RoundingRadius);\n"
	"	else pn.x -= min(pn.x,Rectangle.z-RoundingRadius);\n"
	"	float d = DistanceAABB(p,Rectangle.zw,RoundingRadius);\n"
	"	if (Color.a < 0.25){\n"//CHANNEL
	"		vec3 outerColor = Color.rgb+dot(pn/RoundingRadius,vec2(1,-1))*vec3(0.1);\n"
	"		FragColor = vec4(outerColor,1.0-clamp(d,0.0,1.0));\n"
	"		return;\n"
	"	}\n"
	"	vec3 outerColor = Color.rgb+dot(pn/RoundingRadius,vec2(-1,1))*vec3(0.1);\n"
	"	float innerRadius = RoundingRadius*0.8;\n"
	"	vec3 innerColorShift;\n"
	"	vec3 innerColor;\n"
	"	if (Color.a < 0.5){\n"//FLAT
	"		innerColorShift = vec3(0.0);\n"
	"		innerColor = Color.rgb;\n"
	"	} else if (Color.a < 0.75){\n"//DISH
	"		innerColorShift = dot(pn/innerRadius,vec2(1,-1))*vec3(0.1);\n"
	"		innerColor = Color.rgb+innerColorShift;\n"
	"	} else {\n"//CONE
	"		innerColorShift = dot(normalize(pn/innerRadius),vec2(1,-1))*vec3(0.1);\n"
	"		innerColor = Color.rgb+innerColorShift;\n"
	"	}\n"
	"	if (IconColor.a < 0.1666) FragColor = vec4(mix(innerColor,outerColor,clamp(d+(RoundingRadius-innerRadius),0.0,1.0)),1.0-clamp(d,0.0,1.0));\n"
	"	else if (IconColor.a < 0.333){\n"
	"		vec3 iconColorShaded = IconColor.rgb+innerColorShift;\n"
	"		float iconD = DistanceEquilateralTriangle(rotate(p,-PI/2.0),RoundingRadius/4.0)-(RoundingRadius/16.0);\n"
	"		FragColor = vec4(mix(mix(iconColorShaded,innerColor,clamp(iconD,0.0,1.0)),outerColor,clamp(d+(RoundingRadius-innerRadius),0.0,1.0)),1.0-clamp(d,0.0,1.0));\n"
	"	} else if (IconColor.a < 0.5){\n"
	"		vec3 iconColorShaded = IconColor.rgb+innerColorShift;\n"
	"		float iconD = DistanceEquilateralTriangle(rotate(p,PI/2.0),RoundingRadius/4.0)-(RoundingRadius/16.0);\n"
	"		FragColor = vec4(mix(mix(iconColorShaded,innerColor,clamp(iconD,0.0,1.0)),outerColor,clamp(d+(RoundingRadius-innerRadius),0.0,1.0)),1.0-clamp(d,0.0,1.0));\n"
	"	} else if (IconColor.a < 0.666){\n"
	"		vec3 iconColorShaded = IconColor.rgb+innerColorShift;\n"
	"		vec2 halfExtents = vec2(innerRadius/9.0,innerRadius/3.0);\n"
	"		vec2 spacing = vec2(halfExtents.x*2.0,0.0);\n"
	"		float iconD = min(DistanceAABB(p+spacing,halfExtents,halfExtents.x),DistanceAABB(p-spacing,halfExtents,halfExtents.x));\n"
	"		FragColor = vec4(mix(mix(iconColorShaded,innerColor,clamp(iconD,0.0,1.0)),outerColor,clamp(d+(RoundingRadius-innerRadius),0.0,1.0)),1.0-clamp(d,0.0,1.0));\n"
	"	} else if (IconColor.a < 0.833){\n"
	"		vec3 iconColorShaded = IconColor.rgb+innerColorShift;\n"
	"		vec2 halfExtents = vec2(RoundingRadius/16.0,RoundingRadius/4.0);\n"
	"		float playRadius = RoundingRadius/5.0;\n"
	"		float playRoundingRadius = playRadius/4.0;\n"
	"		float iconD = min(DistanceEquilateralTriangle(rotate(p+vec2(halfExtents.x,0.0),-PI/2.0),playRadius)-playRoundingRadius,DistanceAABB(p-vec2(playRadius+playRoundingRadius-halfExtents.x,0.0),halfExtents,halfExtents.x));\n"
	"		FragColor = vec4(mix(mix(iconColorShaded,innerColor,clamp(iconD,0.0,1.0)),outerColor,clamp(d+(RoundingRadius-innerRadius),0.0,1.0)),1.0-clamp(d,0.0,1.0));\n"
	"	} else {\n"
	"		vec3 iconColorShaded = IconColor.rgb+innerColorShift;\n"
	"		vec2 halfExtents = vec2(RoundingRadius/16.0,RoundingRadius/4.0);\n"
	"		float playRadius = RoundingRadius/5.0;\n"
	"		float playRoundingRadius = playRadius/4.0;\n"
	"		float iconD = min(DistanceEquilateralTriangle(rotate(p-vec2(halfExtents.x,0.0),PI/2.0),playRadius)-playRoundingRadius,DistanceAABB(p+vec2(playRadius+playRoundingRadius-halfExtents.x,0.0),halfExtents,halfExtents.x));\n"
	"		FragColor = vec4(mix(mix(iconColorShaded,innerColor,clamp(iconD,0.0,1.0)),outerColor,clamp(d+(RoundingRadius-innerRadius),0.0,1.0)),1.0-clamp(d,0.0,1.0));\n"
	"	}\n"
	"}\n"
};

static void compile_color_shader(){
	COMPILE_SHADER(color_shader);
	GET_ATTRIB(color_shader,aPosition);
	GET_ATTRIB(color_shader,aColor);
	GET_UNIFORM(color_shader,uMVP);
}

static void compile_texture_color_shader(){
	COMPILE_SHADER(texture_color_shader);
	GET_ATTRIB(texture_color_shader,aPosition);
	GET_ATTRIB(texture_color_shader,aTexCoord);
	GET_ATTRIB(texture_color_shader,aColor);
	GET_UNIFORM(texture_color_shader,uMVP);
	GET_UNIFORM(texture_color_shader,uTex);
}

static void compile_rounded_rect_shader(){
	COMPILE_SHADER(rounded_rect_shader);
	GET_ATTRIB(rounded_rect_shader,aPosition);
	GET_ATTRIB(rounded_rect_shader,aRectangle);
	GET_ATTRIB(rounded_rect_shader,aRoundingRadius);
	GET_ATTRIB(rounded_rect_shader,aColor);
	GET_ATTRIB(rounded_rect_shader,aIconColor);
	GET_UNIFORM(rounded_rect_shader,proj);
}

void compile_shaders(){
	compile_color_shader();
	compile_texture_color_shader();
	compile_rounded_rect_shader();
}

TextureColorVertex *TextureColorVertexListMakeRoom(TextureColorVertexList *list, int count){
	if (list->used+count > list->total){
		if (!list->total) list->total = 1;
		while (list->used+count > list->total) list->total *= 2;
		list->elements = realloc_or_die(list->elements,list->total*sizeof(*list->elements));
	}
	list->used += count;
	return list->elements+list->used-count;
}

RoundedRectVertex *RoundedRectVertexListMakeRoom(RoundedRectVertexList *list, int count){
	if (list->used+count > list->total){
		if (!list->total) list->total = 1;
		while (list->used+count > list->total) list->total *= 2;
		list->elements = realloc_or_die(list->elements,list->total*sizeof(*list->elements));
	}
	list->used += count;
	return list->elements+list->used-count;
}

void append_rounded_rect(RoundedRectVertexList *verts, int x, int y, int z, int halfWidth, int halfHeight, float RoundingRadius, uint32_t color, uint32_t IconColor){
	RoundedRectVertex *v = RoundedRectVertexListMakeRoom(verts,6);
	int hwp = halfWidth+4;// pad by 4px to not cut off antialiased edges
	int hhp = halfHeight+4;

	v[0].Position[0] = -hwp + x;
	v[0].Position[1] = hhp + y;
	v[0].Position[2] = z;

	v[1].Position[0] = -hwp + x;
	v[1].Position[1] = -hhp + y;
	v[1].Position[2] = z;

	v[2].Position[0] = hwp + x;
	v[2].Position[1] = -hhp + y;
	v[2].Position[2] = z;

	v[3].Position[0] = v[2].Position[0];
	v[3].Position[1] = v[2].Position[1];
	v[3].Position[2] = v[2].Position[2];

	v[4].Position[0] = hwp + x;
	v[4].Position[1] = hhp + y;
	v[4].Position[2] = z;

	v[5].Position[0] = v[0].Position[0];
	v[5].Position[1] = v[0].Position[1];
	v[5].Position[2] = v[0].Position[2];

	for (int i = 0; i < 6; i++){
		v[i].Rectangle[0] = x;
		v[i].Rectangle[1] = y;
		v[i].Rectangle[2] = halfWidth;
		v[i].Rectangle[3] = halfHeight;
		v[i].RoundingRadius = RoundingRadius;
		v[i].color = color;
		v[i].IconColor = IconColor;
	}
}

void new_vao(GPUMesh *m, void *verts, int count, size_t size_of_element){
	glGenVertexArrays(1,&m->vao);
	glBindVertexArray(m->vao);
	glGenBuffers(1,&m->vbo);
	glBindBuffer(GL_ARRAY_BUFFER,m->vbo);
	glBufferData(GL_ARRAY_BUFFER,count*size_of_element,verts,GL_STATIC_DRAW);
	m->vertex_count = count;
}

void gpu_mesh_from_color_verts(GPUMesh *m, ColorVertex *verts, int count){
	new_vao(m,verts,count,sizeof(*verts));
	glEnableVertexAttribArray(color_shader.aPosition);
	glEnableVertexAttribArray(color_shader.aColor);
	glVertexAttribPointer(color_shader.aPosition,3,GL_FLOAT,GL_FALSE,sizeof(ColorVertex),0);
	glVertexAttribPointer(color_shader.aColor,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(ColorVertex),offsetof(ColorVertex,color));
}

void gpu_mesh_from_texture_color_verts(GPUMesh *m, TextureColorVertex *verts, int count){
	new_vao(m,verts,count,sizeof(*verts));
	glEnableVertexAttribArray(texture_color_shader.aPosition);
	glEnableVertexAttribArray(texture_color_shader.aTexCoord);
	glEnableVertexAttribArray(texture_color_shader.aColor);
	glVertexAttribPointer(texture_color_shader.aPosition,3,GL_FLOAT,GL_FALSE,sizeof(TextureColorVertex),0);
	glVertexAttribPointer(texture_color_shader.aTexCoord,2,GL_FLOAT,GL_FALSE,sizeof(TextureColorVertex),offsetof(TextureColorVertex,texcoord));
	glVertexAttribPointer(texture_color_shader.aColor,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(TextureColorVertex),offsetof(TextureColorVertex,color));
}

void gpu_mesh_from_rounded_rect_verts(GPUMesh *m, RoundedRectVertex *verts, int count){
	new_vao(m,verts,count,sizeof(*verts));
	glEnableVertexAttribArray(rounded_rect_shader.aPosition);
	glEnableVertexAttribArray(rounded_rect_shader.aRectangle);
	glEnableVertexAttribArray(rounded_rect_shader.aRoundingRadius);
	glEnableVertexAttribArray(rounded_rect_shader.aColor);
	glEnableVertexAttribArray(rounded_rect_shader.aIconColor);
	glVertexAttribPointer(rounded_rect_shader.aPosition,3,GL_FLOAT,GL_FALSE,sizeof(RoundedRectVertex),0);
	glVertexAttribPointer(rounded_rect_shader.aRectangle,4,GL_FLOAT,GL_FALSE,sizeof(RoundedRectVertex),offsetof(RoundedRectVertex,Rectangle));
	glVertexAttribPointer(rounded_rect_shader.aRoundingRadius,1,GL_FLOAT,GL_FALSE,sizeof(RoundedRectVertex),offsetof(RoundedRectVertex,RoundingRadius));
	glVertexAttribPointer(rounded_rect_shader.aColor,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(RoundedRectVertex),offsetof(RoundedRectVertex,color));
	glVertexAttribPointer(rounded_rect_shader.aIconColor,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(RoundedRectVertex),offsetof(RoundedRectVertex,IconColor));
}

void delete_gpu_mesh(GPUMesh *m){
	glDeleteBuffers(1,&m->vbo);
	glDeleteVertexArrays(1,&m->vao);
	m->vao = 0;
	m->vbo = 0;
	m->vertex_count = 0;
}

void new_image(Image *i, int width, int height){
	i->width = width;
	i->height = height;
	i->pixels = zalloc_or_die(width*height*sizeof(*i->pixels));
}

void blit_8_to_32(Image8 *src, int sx, int sy, int swidth, int sheight, Image *dst, int dx, int dy, uint32_t color){
	if (dx >= dst->width || dy >= dst->height) return;
	if (dx < 0){
		sx -= dx;
		swidth += dx;
		if (swidth < 1) return;
		dx = 0;
	}
	if (dx+swidth > dst->width){
		swidth -= (dx+swidth)-dst->width;
		if (swidth < 1) return;
	}
	if (dy < 0){
		sy -= dy;
		sheight += dy;
		if (sheight < 1) return;
		dy = 0;
	}
	if (dy+sheight > dst->height){
		sheight -= (dy+sheight)-dst->height;
		if (sheight < 1) return;
	}
	for (int i = 0; i < sheight; i++){
		for (int j = 0; j < swidth; j++){
			dst->pixels[(dy+i)*dst->width+dx+j] = (src->pixels[(sy+i)*src->width+sx+j]<<24) | color;
		}
	}
}

void draw_string(Image *dst, int x, int y, FT_Face font_face, int font_height, uint32_t color, int char_count, char *string){
	if (FT_Set_Pixel_Sizes(font_face,0,font_height)){
		fatal_error("Failed to set freetype char size.");
	}
	for (int i = 0; i < char_count; i++){
		if (FT_Load_Char(font_face,string[i],FT_LOAD_RENDER)){
			fatal_error("Failed to load freetype glyph");
		}
		Image8 glyph_image = {
			.width = font_face->glyph->bitmap.width,
			.height = font_face->glyph->bitmap.rows,
			.pixels = font_face->glyph->bitmap.buffer
		};
		blit_8_to_32(&glyph_image,0,0,glyph_image.width,glyph_image.height,dst,x+font_face->glyph->bitmap_left,y-font_face->glyph->bitmap_top,color & 0xffffff);
		x += font_face->glyph->advance.x >> 6;
	}
}

void draw_string_centered(Image *dst, int x, int y, FT_Face font_face, int font_height, uint32_t color, int char_count, char *string){
	if (FT_Set_Pixel_Sizes(font_face,0,font_height)){
		fatal_error("Failed to set freetype char size.");
	}
	int width = 0;
	for (int i = 0; i < char_count; i++){
		if (FT_Load_Char(font_face,string[i],FT_LOAD_RENDER)){
			fatal_error("Failed to load freetype glyph");
		}
		width += font_face->glyph->advance.x >> 6;
	}
	if (FT_Load_Char(font_face,'9',FT_LOAD_RENDER)){
		fatal_error("Failed to load freetype glyph");
	}
	int max_height = font_face->glyph->bitmap.rows;
	x -= (float)width / 2;
	y += (float)max_height / 2;
	for (int i = 0; i < char_count; i++){
		if (FT_Load_Char(font_face,string[i],FT_LOAD_RENDER)){
			fatal_error("Failed to load freetype glyph");
		}
		Image8 glyph_image = {
			.width = font_face->glyph->bitmap.width,
			.height = font_face->glyph->bitmap.rows,
			.pixels = font_face->glyph->bitmap.buffer
		};
		blit_8_to_32(&glyph_image,0,0,glyph_image.width,glyph_image.height,dst,x+font_face->glyph->bitmap_left,y-font_face->glyph->bitmap_top,color & 0xffffff);
		x += font_face->glyph->advance.x >> 6;
	}
}