#include <renderer.h>

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
Button buttons[] = {
	{50,14,46,10,10,BUTTON_GREY,RGBA(0,0,0,RR_ICON_NONE),"Open Image",0},
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
							printf("Clicked %s\n",b->string);
						}
					}
					break;
				}
			}
			break;
		}
		case GLFW_RELEASE:{
			switch (button){
			}
			break;
		}
	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
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
 
	glfwSetCursorPosCallback(window,cursor_position_callback);
	glfwSetMouseButtonCallback(window,mouse_button_callback);
	glfwSetKeyCallback(window,key_callback);
 
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	compile_shaders();

	srand(time(0));

	if (FT_Init_FreeType(&ftlib)){
		fatal_error("Failed to initialize freetype");
	}
	load_font("../res/Nunito-Regular.ttf",&uiface);

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

		int width,height;
		glfwGetFramebufferSize(window,&width,&height);

		mat4 ortho;
		glm_ortho(0,width,0,height,-10,10,ortho);

		mat4 persp;
		glm_perspective(0.5f*M_PI,(float)width/(float)height,0.01f,1024.0f,persp);
		mat4 vp;
		get_view(vp,&camera);
		glm_mat4_mul(persp,vp,vp);
 
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(rounded_rect_shader.id);
		glUniformMatrix4fv(rounded_rect_shader.proj,1,GL_FALSE,ortho);
		RoundedRectVertexList rrvl = {0};
		for (Button *b = buttons; b < buttons+COUNT(buttons); b++){
			append_rounded_rect(&rrvl,b->x,height-1-b->y,0,b->halfWidth,b->halfHeight,b->roundingRadius,b->color,b->IconColor);
		}
		if (rrvl.used){
			GPUMesh button_mesh;
			gpu_mesh_from_rounded_rect_verts(&button_mesh,rrvl.elements,rrvl.used);
			glDrawArrays(GL_TRIANGLES,0,rrvl.used);
			free(rrvl.elements);
			delete_gpu_mesh(&button_mesh);
		}

		Image text_image;
		new_image(&text_image,width,height);
		for (Button *b = buttons; b < buttons+COUNT(buttons); b++){
			draw_string_centered(&text_image,b->x,abs(b->y),uiface,12,RGB(255,255,255),strlen(b->string),b->string);
		}
		draw_string(&text_image,width/2,height/2,uiface,32,RGB(255,255,255),4,"xtxt");
		Texture text_texture;
		texture_from_image(&text_texture,&text_image);
		glUseProgram(texture_color_shader.id);
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
		glActiveTexture(GL_TEXTURE0);
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
 
	glfwTerminate();
}