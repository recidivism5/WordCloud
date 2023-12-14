#include <renderer.h>

FT_Library ftlib;
FT_Face uiface;

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
	switch (action){
		case GLFW_PRESS:{
			switch (button){
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

void clamp_euler(vec3 e){
	float fp = 4*M_PI;
	for (int i = 0; i < 3; i++){
		if (e[i] > fp) e[i] -= fp;
		else if (e[i] < -fp) e[i] += fp;
	}
}
void rotate_euler(vec3 e, float dx, float dy, float sens){
	e[1] += sens * dx;
	e[0] += sens * dy;
	clamp_euler(e);
}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
	//rotate_euler(player.head_euler,xpos,ypos,-0.001f);
	//glfwSetCursorPos(window, 0, 0);
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

void load_font(char *path, FT_Face *face){
	FT_Error error = FT_New_Face(ftlib,path,0,face);
	if (error == FT_Err_Unknown_File_Format){
		fatal_error("Failed to load %s: unsupported font format.",path);
	} else if (error){
		fatal_error("Failed to load %s: file not found.",path);
	}
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

		mat4 persp;
		glm_perspective(0.5f*M_PI,(float)width/(float)height,0.01f,1024.0f,persp);
		mat4 vp;
		get_view(vp,&camera);
		glm_mat4_mul(persp,vp,vp);
 
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		Image text_image;
		new_image(&text_image,width,height);
		draw_string(&text_image,width/2,height/2,uiface,32,RGB(0,0,0),4,"fuck");
		Texture text_texture;
		texture_from_image(&text_texture,&text_image);
		glUseProgram(texture_color_shader.id);
		TextureColorVertex screen_quad[6] = {
			-1,1,0, 0,0, 0xffffffff,
			-1,-1,0, 0,1, 0xffffffff,
			 1,-1,0, 1,1, 0xffffffff,

			 1,-1,0, 1,1, 0xffffffff,
			 1,1,0, 1,0, 0xffffffff,
			-1,1,0, 0,0, 0xffffffff,
		};
		GPUMesh m;
		gpu_mesh_from_texture_color_verts(&m,screen_quad,COUNT(screen_quad));
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(texture_color_shader.uTex,0);
		glUniformMatrix4fv(texture_color_shader.uMVP,1,GL_FALSE,GLM_MAT4_IDENTITY);
		glDrawArrays(GL_TRIANGLES,0,COUNT(screen_quad));
		delete_gpu_mesh(&m);
		delete_texture(&text_texture);
		free(text_image.pixels);

		glCheckError();
 
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
 
	glfwDestroyWindow(window);
 
	glfwTerminate();
}