#include <renderer.h>
#include <fast_obj.h>
#include <bmf.h>

TSTRUCT(Camera){
	vec3 position;
	vec3 euler;
} camera;

void get_model(mat4 m, vec3 object_position, vec3 object_euler){
	glm_euler_zyx(object_euler,m);
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

struct {
	bool
		left,
		right,
		backward,
		forward;
} keys;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	switch (action){
		case GLFW_PRESS:{
			switch (key){
				case GLFW_KEY_ESCAPE:{
					glfwSetWindowShouldClose(window, GLFW_TRUE);
					break;
				}
				case GLFW_KEY_A: keys.left = true; break;
				case GLFW_KEY_D: keys.right = true; break;
				case GLFW_KEY_S: keys.backward = true; break;
				case GLFW_KEY_W: keys.forward = true; break;
				case GLFW_KEY_R:{
					static int prev_width, prev_height;
					static bool fullscreen = false;
					GLFWmonitor *primary = glfwGetPrimaryMonitor();
					GLFWvidmode *vm = glfwGetVideoMode(primary);
					if (!fullscreen){
						glfwGetFramebufferSize(window,&prev_width,&prev_height);
						glfwSetWindowMonitor(window,primary,0,0,vm->width,vm->height,GLFW_DONT_CARE);
						fullscreen = true;
					} else {
						glfwSetWindowMonitor(window,0,(vm->width-prev_width)/2,(vm->height-prev_height)/2,prev_width,prev_height,GLFW_DONT_CARE);
						fullscreen = false;
					}
					break;
				}
			}
			break;
		}
		case GLFW_RELEASE:{
			switch (key){
				case GLFW_KEY_A: keys.left = false; break;
				case GLFW_KEY_D: keys.right = false; break;
				case GLFW_KEY_S: keys.backward = false; break;
				case GLFW_KEY_W: keys.forward = false; break;
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
	glfwSetCursorPos(window, 0, 0);
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
 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow *window = create_centered_window(640,480,"Blocks");
 
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (glfwRawMouseMotionSupported()){
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
	glfwSetCursorPos(window, 0, 0);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);
 
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	compile_shaders();

	srand(time(0));

	BMF car_bmf;
	load_bmf(&car_bmf,"../res/car.bmf");
	printf("Car bmf:\n"
		"vertex_count: %d\n",
		car_bmf.vertex_count);

	GPUMesh car_mesh;
	gpu_mesh_from_texture_diffuse_verts(&car_mesh,car_bmf.vertices,car_bmf.vertex_count);

	Texture car_texture;
	load_texture(&car_texture,"../res/car.png");

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

		int width,height;
		glfwGetFramebufferSize(window, &width, &height);

		mat4 persp;
		glm_perspective(0.5f*M_PI,(float)width/(float)height,0.01f,1024.0f,persp);

		ivec2 move_dir;
		if (keys.left && keys.right){
			move_dir[0] = 0;
		} else if (keys.left){
			move_dir[0] = -1;
		} else if (keys.right){
			move_dir[0] = 1;
		} else {
			move_dir[0] = 0;
		}
		if (keys.backward && keys.forward){
			move_dir[1] = 0;
		} else if (keys.backward){
			move_dir[1] = 1;
		} else if (keys.forward){
			move_dir[1] = -1;
		} else {
			move_dir[1] = 0;
		}
 
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(texture_diffuse_shader.id);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(texture_diffuse_shader.uTex,0);
		glUniform3f(texture_diffuse_shader.uLightDir,-1,-1,-1);
		mat4 model;
		static float rot = 0.0f;
		rot += dt;
		if (rot > 2.0f*M_PI) rot -= 2.0f*M_PI;
		get_model(model,(vec3){0,0,0},(vec3){0,rot,0});
		mat4 view;
		get_view(view,&camera);
		mat4 mvp;
		glm_mat4_mul(view,model,mvp);
		glm_mat4_mul(persp,mvp,mvp);
		glUniformMatrix4fv(texture_diffuse_shader.uModel,1,GL_FALSE,model);
		glUniformMatrix4fv(texture_diffuse_shader.uMVP,1,GL_FALSE,mvp);
		glDrawArrays(GL_TRIANGLES,0,car_mesh.vertex_count);

		glCheckError();
 
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
 
	glfwDestroyWindow(window);
 
	glfwTerminate();
}