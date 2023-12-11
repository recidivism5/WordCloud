#include <renderer.h>
#include <fast_obj.h>
#include <bmf.h>
#include <gjk.h>

void shape_from_obj(Shape *s, char *path){
	fastObjMesh *obj = fast_obj_read(path);
	s->position_count = obj->position_count;
	s->positions = malloc_or_die(s->position_count*sizeof(vec3));
	memcpy(s->positions,obj->positions,s->position_count*sizeof(vec3));
	s->index_count = obj->index_count;
	s->indices = malloc_or_die(s->index_count*sizeof(int));
	for (int i = 0; i < s->index_count; i++){
		s->indices[i] = obj->indices[i].p;
	}
	fast_obj_destroy(obj);
	gpu_mesh_from_positions(&s->gpu_mesh,s->positions,s->position_count);
}

TSTRUCT(ShapeInstance){
	Shape *shape;
	vec3 *transformed_positions;
};

void new_shape_instance(ShapeInstance *i, Shape *s){
	i->shape = s;
	i->transformed_positions = malloc_or_die(s->position_count*sizeof(vec3));
}

void free_collider_instance(ShapeInstance *i){
	i->shape = 0;
	free(i->transformed_positions);
	i->transformed_positions = 0;
}

bool shape_instances_colliding(ShapeInstance *a, mat4 a_to_world, ShapeInstance *b, mat4 b_to_world){
	for (int i = 0; i < a->shape->position_count; i++){
		glm_mat4_mulv3(a_to_world,a->shape->positions[i],1.0f,a->transformed_positions[i]);
	}
	for (int i = 0; i < b->shape->position_count; i++){
		glm_mat4_mulv3(b_to_world,b->shape->positions[i],1.0f,b->transformed_positions[i]);
	}
	Shape sa = {
		.position_count = a->shape->position_count,
		.positions = a->transformed_positions,
		.index_count = a->shape->index_count,
		.indices = a->shape->indices
	};
	Shape sb = {
		.position_count = b->shape->position_count,
		.positions = b->transformed_positions,
		.index_count = b->shape->index_count,
		.indices = b->shape->indices
	};
	return gjk(&sa,&sb);
}

TSTRUCT(Box){
	float density;
	vec3 half_extents;
	vec3 position;
	vec3 velocity;
	vec4 rotation;
	vec3 angular_velocity;
	mat3 inertia;
};

void init_box(Box *b, vec3 half_extents, vec3 euler, vec3 position){
	mat4 mrot;
	glm_euler_zyx(euler,mrot);
	glm_mat4_quat(mrot,b->rotation);
	glm_vec3_copy(half_extents,b->half_extents);
	glm_vec3_copy(position,b->position);
}

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

GPUMesh cube_outline;
void init_cube_outline(){
	vec3 v[] = {
		0,1,0, 0,0,0,
		0,0,0, 0,0,1,
		0,0,1, 0,1,1,
		0,1,1, 0,1,0,

		1,1,0, 1,0,0,
		1,0,0, 1,0,1,
		1,0,1, 1,1,1,
		1,1,1, 1,1,0,

		0,1,0, 1,1,0,
		0,0,0, 1,0,0,
		0,0,1, 1,0,1,
		0,1,1, 1,1,1,
	};
	for (int i = 0; i < COUNT(v); i++){
		v[i][0] -= 0.5f;
		v[i][1] -= 0.5f;
		v[i][2] -= 0.5f;
		v[i][0] *= 2;
		v[i][1] *= 2;
		v[i][2] *= 2;
	}
	gpu_mesh_from_positions(&cube_outline,v,COUNT(v));
}

void draw_box(Box *b, mat4 view_project){
	mat4 model;
	glm_quat_mat4(b->rotation,model);
	model[0][0] *= b->half_extents[0];
	model[1][1] *= b->half_extents[1];
	model[2][2] *= b->half_extents[2];
	model[3][0] = b->position[0];
	model[3][1] = b->position[1];
	model[3][2] = b->position[2];
	mat4 mvp;
	glm_mat4_mul(view_project,model,mvp);
	glUniformMatrix4fv(uniform_color_shader.uMVP,1,GL_FALSE,mvp);
	glDrawArrays(GL_LINES,0,cube_outline.vertex_count);
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

	Texture car_texture;
	load_texture(&car_texture,"../res/car.png");

	Shape car_collider;
	shape_from_obj(&car_collider,"../res/car_collider.obj");

	ShapeInstance cca, ccb;
	new_shape_instance(&cca,&car_collider);
	new_shape_instance(&ccb,&car_collider);

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
		mat4 vp;
		get_view(vp,&camera);
		glm_mat4_mul(persp,vp,vp);

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

		/*glUseProgram(texture_diffuse_shader.id);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(texture_diffuse_shader.uTex,0);
		glUniform3f(texture_diffuse_shader.uLightDir,-1,-1,-1);
		mat4 model;
		static float rot = 0.0f;
		rot += dt;
		if (rot > 2.0f*M_PI) rot -= 2.0f*M_PI;
		get_model(model,(vec3){0,0,0},(vec3){0,rot,0},(vec3){1,1,1});
		mat4 mvp;
		glm_mat4_mul(vp,model,mvp);
		glUniformMatrix4fv(texture_diffuse_shader.uModel,1,GL_FALSE,model);
		glUniformMatrix4fv(texture_diffuse_shader.uMVP,1,GL_FALSE,mvp);
		glBindVertexArray(car_mesh.vao);
		glDrawArrays(GL_TRIANGLES,0,car_mesh.vertex_count);*/

		static float t = 0;
		t += dt;
		if (t > 2.0f*M_PI) t -= 2.0f*M_PI;
		mat4 a_to_world, b_to_world;
		glm_mat4_identity(a_to_world);
		get_model(b_to_world,(vec3){0,0.1f,3.0f*sinf(t)},(vec3){0.0f,0.5f,0.0f},(vec3){1,1,1});

		glUseProgram(uniform_color_shader.id);
		glUniform4f(uniform_color_shader.uColor,shape_instances_colliding(&cca,a_to_world,&ccb,b_to_world) ? 1.0f : 0.0f,0.0f,0.0f,0.5f);
		glBindVertexArray(car_collider.gpu_mesh.vao);
		mat4 mvp;
		glm_mat4_mul(vp,a_to_world,mvp);
		glUniformMatrix4fv(uniform_color_shader.uMVP,1,GL_FALSE,mvp);
		glDrawElements(GL_TRIANGLES,car_collider.index_count,GL_UNSIGNED_INT,car_collider.indices);
		glm_mat4_mul(vp,b_to_world,mvp);
		glUniformMatrix4fv(uniform_color_shader.uMVP,1,GL_FALSE,mvp);
		glDrawElements(GL_TRIANGLES,car_collider.index_count,GL_UNSIGNED_INT,car_collider.indices);

		glCheckError();
 
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
 
	glfwDestroyWindow(window);
 
	glfwTerminate();
}