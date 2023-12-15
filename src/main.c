#include <renderer.h>
#include <nfd.h>

TSTRUCT(Camera){
	vec3 position;
	vec3 euler;
} camera;

void get_model(mat4 m, vec3 object_position, vec3 object_euler, vec3 object_scale){
	glm_euler_zyx(object_euler,m);
	m[0][0] *= object_scale[0];
	m[1][1] *= object_scale[1];
	m[2][2] *= object_scale[2];
	m[3][0] = object_position[0];
	m[3][1] = object_position[1];
	m[3][2] = object_position[2];
}

void get_view(mat4 m, Camera *cam){
	mat4 trans;
	vec3 nt;
	glm_vec3_negate_to(cam->position,nt);
	glm_translate_make(trans,nt);
	glm_euler_zyx(cam->euler,m);
	glm_mat4_transpose(m);
	glm_mat4_mul(m,trans,m);
}

FT_Library ftlib;
FT_Face uiface;
void load_font(char *path, FT_Face *face){
	FT_Error error = FT_New_Face(ftlib,path,0,face);
	if (error == FT_Err_Unknown_File_Format){
		fatal_error("Failed to load %s: unsupported font format.",path);
	} else if (error){
		fatal_error("Failed to load %s: file not found.",path);
	}
}

TSTRUCT(ImageTexture){
	Image image;
	Texture texture;
};
ImageTexture images[5];
bool greyscale = false;
int gaussianBlurStrength = 9;
int quantizeDivisions = 4;
int rectangleDecomposeMinDim = 25;

int scale = 1;
bool interpolation = false;
vec3 pos;
bool pan = false;
ivec2 panPoint;
vec3 originalPos;
String imagePath;
String textPath;
int client_width, client_height;

TSTRUCT(Button){
	int x, y, halfWidth, halfHeight, roundingRadius;
	uint32_t color, IconColor;
	char *string;
	void (*func)(void);
};
#define BUTTON_GREY RGBA(127,127,127,RR_DISH)
#define BUTTON_GREY_HIGHLIGHTED RGBA(160,160,160,RR_DISH)
#define BUTTON_GREEN (0x7B9944 | (RR_DISH<<24))
#define BUTTON_GREEN_HIGHLIGHTED (0x9ABC56 | (RR_DISH<<24))
bool useNewDecompose = true;
void update(){
	size_t size = images[0].image.width*images[0].image.height*sizeof(*images[0].image.pixels);
	memcpy(images[1].image.pixels,images[0].image.pixels,size);
	if (greyscale){
		img_greyscale(&images[1].image);
	}
	img_gaussian_blur(&images[1].image,gaussianBlurStrength);
	memcpy(images[2].image.pixels,images[1].image.pixels,size);
	img_quantize(&images[2].image,quantizeDivisions);
	memcpy(images[3].image.pixels,images[2].image.pixels,size);

	/*ColorRectList crl = {0};
	useNewDecompose ? RectangleDecompose(&crl,&images[3].image,rectangleDecomposeMinDim) : OldRectangleDecompose(&crl,&images[3].image,rectangleDecomposeMinDim);

	if (gtext.ptr){
		WordArray wa;
		WordsByAspect(&wa,&gtext,NOUN|VERB,0,"Consolas",LOWER_CASE);
		WordReconstruct(&images[4].image,&crl,&wa,"Consolas");
		if (wa.len){
			free(wa.words);
		}
	}*/

	for (ImageTexture *it = images; it < images+COUNT(images); it++){
		texture_from_image(&it->texture,&it->image);
	}
}
void open_image(){
	nfdchar_t *path;
	nfdfilteritem_t filterItem[1] = {{ "Image", "png,jpg" }};
	nfdresult_t result = NFD_OpenDialog(&path, filterItem, 1, NULL);
	if (result == NFD_OKAY){
		if (images[0].image.pixels){
			for (int i = 0; i < COUNT(images); i++){
				free(images[i].image.pixels);
				memset(&images[i].image,0,sizeof(images[i].image));
			}
		}
		load_image(&images[0].image,path);
		for (int i = 1; i < COUNT(images); i++){
			images[i].image.width = images[0].image.width;
			images[i].image.height = images[0].image.height;
			images[i].image.pixels = malloc_or_die(images[0].image.width*images[0].image.height*sizeof(*images[0].image.pixels));
		}
		update();
		cstr_to_string(path,&imagePath);
		NFD_FreePath(path);
	} else if (result == NFD_CANCEL){

	} else {
		printf("Error: %s\n", NFD_GetError());
	}
}
Button buttons[] = {
	{50,14,46,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),"Open Image",open_image},
	{50,14+26*1,46,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),"Open Text",0},
	{50,14+26*2,46,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),"Greyscale: Off",0},
	{14,14+26*3,10,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),"-",0},{200,14+26*3,10,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),L"+",0},
	{14,14+26*4,10,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),"-",0},{200,14+26*4,10,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),L"+",0},
	{14,14+26*5,10,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),"-",0},{200,14+26*5,10,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),L"+",0},
	{50+68-46,14+26*6,68,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),"Rct. Decompose: New",0},
};
bool PointInButton(int buttonX, int buttonY, int halfWidth, int halfHeight, int x, int y){
	return abs(x-buttonX) < halfWidth && abs(y-buttonY) < halfHeight;
}

void error_callback(int error, const char* description){
	fprintf(stderr, "Error: %s\n", description);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	switch (action){
		case GLFW_PRESS:{
			switch (key){
			}
			break;
		}
		case GLFW_RELEASE:{
			switch (key){
			}
			break;
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	double xpos,ypos;
	glfwGetCursorPos(window,&xpos,&ypos);
	switch (action){
		case GLFW_PRESS:{
			switch (button){
				case GLFW_MOUSE_BUTTON_LEFT:{
					for (Button *b = buttons; b < buttons+COUNT(buttons); b++){
						if (PointInButton(b->x,b->y,b->halfWidth,b->halfHeight,xpos,ypos)){
							b->func();
							break;
						}
					}
					if (scale > 1){
						panPoint[0] = xpos;
						panPoint[1] = ypos;
						memcpy(originalPos,pos,sizeof(pos));
						pan = true;
					}
					break;
				}
			}
			break;
		}
		case GLFW_RELEASE:{
			switch (button){
				case GLFW_MOUSE_BUTTON_LEFT:{
					pan = false;
					break;
				}
			}
			break;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	double xpos,ypos;
	glfwGetCursorPos(window,&xpos,&ypos);
	ivec2 pt = {xpos,ypos};
	int oldScale = scale;
	int clamped_yoffset = yoffset > 0 ? 1 : -1;
	scale = CLAMP(scale+clamped_yoffset,1,100);
	if (oldScale != scale){
		pt[0] -= pos[0];
		pt[1] -= pos[1];
		if (scale > oldScale){
			pos[0] -= pt[0] / (float)(scale-1);
			pos[1] -= pt[1] / (float)(scale-1);
		} else {
			pos[0] += pt[0] / (float)(scale+1);
			pos[1] += pt[1] / (float)(scale+1);
		}
	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
	if (pan){
		pos[0] = originalPos[0] + (xpos - panPoint[0]);
		pos[1] = originalPos[1] + (ypos - panPoint[1]);
	}
	for (Button *b = buttons; b < buttons+COUNT(buttons); b++){
		if (PointInButton(b->x,b->y,b->halfWidth,b->halfHeight,xpos,ypos)){
			b->color = BUTTON_GREY_HIGHLIGHTED;
		} else {
			b->color = BUTTON_GREY;
		}
	}
}

GLFWwindow *create_centered_window(int width, int height, char *title){
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow *window = glfwCreateWindow(width,height,title,NULL,NULL);
	if (!window){
		glfwTerminate();
		fatal_error("Failed to create GLFW window");
	}
	GLFWmonitor *primary = glfwGetPrimaryMonitor();
	if (!primary){
		glfwTerminate();
		fatal_error("Failed to get primary monitor");
	}
	GLFWvidmode *vm = glfwGetVideoMode(primary);
	if (!vm){
		glfwTerminate();
		fatal_error("Failed to get primary monitor video mode");
	}
	glfwSetWindowPos(window,(vm->width-width)/2,(vm->height-height)/2);
	glfwShowWindow(window);
	return window;
}

void main(void)
{
	glfwSetErrorCallback(error_callback);
 
	if (!glfwInit()){
		fatal_error("Failed to initialize GLFW");
	}
 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
	GLFWwindow *window = create_centered_window(640,480,"WordCloud");
 
	glfwSetKeyCallback(window,key_callback);
	glfwSetCursorPosCallback(window,cursor_position_callback);
	glfwSetMouseButtonCallback(window,mouse_button_callback);
	glfwSetScrollCallback(window,scroll_callback);
 
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	compile_shaders();

	srand(time(0));

	if (FT_Init_FreeType(&ftlib)){
		fatal_error("Failed to initialize freetype");
	}
	load_font("../res/Nunito-Regular.ttf",&uiface);

	NFD_Init();

	camera.position[0] = 0;
	camera.position[1] = 2;
	camera.position[2] = 2;
	camera.euler[0] = -0.25f*M_PI;

	double t0 = glfwGetTime();
 
	while (!glfwWindowShouldClose(window))
	{
		double t1 = glfwGetTime();
		double dt = t1 - t0;
		t0 = t1;

		dt = MIN(1.0/60.0,dt);

		glfwGetFramebufferSize(window,&client_width,&client_height);

		mat4 ortho;
		glm_ortho(0,client_width,0,client_height,-10,10,ortho);
 
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, client_width, client_height);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(texture_color_shader.id);
		if (imagePath.len){
			float totalHeight = (float)images[0].image.height*COUNT(images);
			float width = MIN(client_width,images[0].image.width);
			float height = width * (totalHeight/(float)images[0].image.width);
			if (height > client_height){
				height = client_height;
				width = height * ((float)images[0].image.width/totalHeight);
			}
			width *= scale;
			height *= scale;
			if (scale == 1){
				pos[0] = (client_width-(int)width)/2;
				pos[1] = (client_height-(int)height)/2;
				pos[2] = -1;
			}

			glUniform1i(texture_color_shader.uTex,0);
			TextureColorVertex v[6] = {
				{{0,1,0},{0,0},RGBA(255,255,255,255)},
				{{0,0,0},{0,1},RGBA(255,255,255,255)},
				{{1,0,0},{1,1},RGBA(255,255,255,255)},
				{{1,0,0},{1,1},RGBA(255,255,255,255)},
				{{1,1,0},{1,0},RGBA(255,255,255,255)},
				{{0,1,0},{0,0},RGBA(255,255,255,255)}
			};
			GPUMesh imageQuad;
			gpu_mesh_from_texture_color_verts(&imageQuad,v,COUNT(v));
			glBindVertexArray(imageQuad.vao);
			mat4 mata,matb,matc;
			float individualHeight = height/COUNT(images);
			for (int i = 0; i < COUNT(images); i++){
				glBindTexture(GL_TEXTURE_2D,images[i].texture.id);
				glm_scale_make(matb,(vec3){width,individualHeight,1});
				glm_translate_make(mata,(vec3){pos[0],client_height-1-pos[1]-(i+1)*individualHeight,pos[2]});
				glm_mat4_mul(mata,matb,matc);
				glm_mat4_mul(ortho,matc,mata);
				glUniformMatrix4fv(texture_color_shader.uMVP,1,GL_FALSE,mata);
				glDrawArrays(GL_TRIANGLES,0,imageQuad.vertex_count);
			}
			delete_gpu_mesh(&imageQuad);
		}

		glUseProgram(rounded_rect_shader.id);
		glUniformMatrix4fv(rounded_rect_shader.proj,1,GL_FALSE,ortho);
		RoundedRectVertexList rrvl = {0};
		for (Button *b = buttons; b < buttons+COUNT(buttons); b++){
			append_rounded_rect(&rrvl,b->x,client_height-1-b->y,0,b->halfWidth,b->halfHeight,b->roundingRadius,b->color,b->IconColor);
		}
		if (rrvl.used){
			GPUMesh button_mesh;
			gpu_mesh_from_rounded_rect_verts(&button_mesh,rrvl.elements,rrvl.used);
			glDrawArrays(GL_TRIANGLES,0,rrvl.used);
			free(rrvl.elements);
			delete_gpu_mesh(&button_mesh);
		}

		glUseProgram(texture_color_shader.id);
		Image text_image;
		new_image(&text_image,client_width,client_height);
		for (Button *b = buttons; b < buttons+COUNT(buttons); b++){
			draw_string_centered(&text_image,b->x,abs(b->y),uiface,12,RGB(255,255,255),strlen(b->string),b->string);
		}
		Texture text_texture;
		texture_from_image(&text_texture,&text_image);
		TextureColorVertex screen_quad[6] = {
			-1,1,-1, 0,0, 0xffffffff,
			-1,-1,-1, 0,1, 0xffffffff,
			1,-1,-1, 1,1, 0xffffffff,

			1,-1,-1, 1,1, 0xffffffff,
			1,1,-1, 1,0, 0xffffffff,
			-1,1,-1, 0,0, 0xffffffff,
		};
		GPUMesh text_mesh;
		gpu_mesh_from_texture_color_verts(&text_mesh,screen_quad,COUNT(screen_quad));
		glUniform1i(texture_color_shader.uTex,0);
		glUniformMatrix4fv(texture_color_shader.uMVP,1,GL_FALSE,GLM_MAT4_IDENTITY);
		glDrawArrays(GL_TRIANGLES,0,COUNT(screen_quad));
		delete_gpu_mesh(&text_mesh);
		delete_texture(&text_texture);
		free(text_image.pixels);

		glCheckError();
 
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
 
	glfwDestroyWindow(window);
 
	NFD_Quit();
	glfwTerminate();
}