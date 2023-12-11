#include <gjk.h>

int support(Shape *s, vec3 dir){
	float max_d = -FLT_MAX;
	int max_i = 0;
	for (int i = 0; i < s->position_count; i++){
		float d = glm_dot(s->positions[i],dir);
		if (d > max_d){
			max_d = d;
			max_i = i;
		}
	}
	return max_i;
}

void minkowski_difference_support(Shape *a, Shape *b, vec3 dir, vec3 out){
	vec3 ndir;
	glm_vec3_negate_to(dir,ndir);
	glm_vec3_sub(a->positions[support(a,dir)],b->positions[support(b,ndir)],out);
}

void simplex_push_front(Simplex *s, vec3 v){
	memmove(s->positions[1],s->positions[0],s->position_count*sizeof(vec3));
	memcpy(s->positions[0],v,sizeof(vec3));
	s->position_count++;
}

bool vectors_agree(vec3 a, vec3 b){
	return glm_dot(a,b) > 0;
}

bool line(Simplex *s, vec3 dir){
	vec3 ab, ao;
	glm_vec3_sub(s->positions[1],s->positions[0],ab);
	glm_vec3_negate_to(s->positions[0],ao);
	if (vectors_agree(ab,ao)){
		glm_cross(ab,ao,ao);
		glm_cross(ao,ab,dir);
	} else {
		s->position_count = 1;
		glm_vec3_copy(ao,dir);
	}
	return false;
}

inline void swap_vec3s(vec3 temp, vec3 a, vec3 b){
	glm_vec3_copy(a,temp);
	glm_vec3_copy(b,a);
	glm_vec3_copy(temp,b);
}

bool triangle(Simplex *s, vec3 dir){
	vec3 ab, ac, ao, abc, temp;

	glm_vec3_sub(s->positions[1],s->positions[0],ab);
	glm_vec3_sub(s->positions[2],s->positions[0],ac);
	glm_vec3_negate_to(s->positions[0],ao);
	glm_cross(ab,ac,abc);

	glm_cross(abc,ac,temp);
	if (vectors_agree(temp,ao)){
		s->position_count = 2;
		if (vectors_agree(ac,ao)){
			glm_vec3_copy(s->positions[2],s->positions[1]);
			glm_cross(ac,ao,temp);
			glm_cross(temp,ac,dir);
		} else {
			return line(s,dir);
		}
	} else {
		glm_cross(ab,abc,temp);
		if (vectors_agree(temp,ao)){
			s->position_count = 2;
			return line(s,dir);
		} else if (vectors_agree(abc,ao)){
			glm_vec3_copy(abc,dir);
		} else {
			swap_vec3s(temp,s->positions[1],s->positions[2]);
			glm_vec3_negate_to(abc,dir);
		}
	}
	return false;
}

bool tetrahedron(Simplex *s, vec3 dir){
	vec3 ab,ac,ad,ao, abc, acd, adb;

	glm_vec3_sub(s->positions[1],s->positions[0],ab);
	glm_vec3_sub(s->positions[2],s->positions[0],ac);
	glm_vec3_sub(s->positions[3],s->positions[0],ad);
	glm_vec3_negate_to(s->positions[0],ao);

	glm_cross(ab,ac,abc);
	glm_cross(ac,ad,acd);
	glm_cross(ad,ab,adb);

	if (vectors_agree(abc,ao)){
		s->position_count = 3;
		return triangle(s,dir); //a,b,c
	}

	if (vectors_agree(acd,ao)){
		s->position_count = 3;
		memmove(s->positions[1],s->positions[2],2*sizeof(vec3));
		return triangle(s,dir); //a,c,d
	}

	if (vectors_agree(adb,ao)){
		s->position_count = 3;
		glm_vec3_copy(s->positions[1],s->positions[2]);
		glm_vec3_copy(s->positions[3],s->positions[1]);
		return triangle(s,dir); //a,d,b
	}

	return true;
}

bool next_simplex(Simplex *s, vec3 dir){
	switch (s->position_count){
		case 2: return line(s,dir);
		case 3: return triangle(s,dir);
		case 4: return tetrahedron(s,dir);
	}
	return false;
}

bool gjk(Shape *a, Shape *b){
	vec3 support;
	vec3 direction;
	glm_vec3_sub(b,a,direction);
	minkowski_difference_support(a,b,direction,support);
	Simplex simplex = {0};
	simplex_push_front(&simplex,support);
	glm_vec3_negate_to(support,direction);
	while (1){
		minkowski_difference_support(a,b,direction,support);
		if (glm_dot(support,direction) <= 0){
			return false;
		}
		simplex_push_front(&simplex,support);
		if (next_simplex(&simplex,direction)){
			return true;
		}
	}
}