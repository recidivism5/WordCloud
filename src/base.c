#include <base.h>

void fatal_error(char *format, ...){
	va_list args;
	va_start(args,format);
	char msg[1024];
	vsnprintf(msg,COUNT(msg),format,args);
	boxerShow(msg,"Error",BoxerStyleError,BoxerButtonsQuit);
	va_end(args);
	exit(1);
}

void *malloc_or_die(size_t size){
	void *p = malloc(size);
	if (!p) fatal_error("malloc failed.");
	return p;
}

void *zalloc_or_die(size_t size){
	void *p = calloc(1,size);
	if (!p) fatal_error("zalloc failed.");
	return p;
}

void *realloc_or_die(void *ptr, size_t size){
	void *p = realloc(ptr,size);
	if (!p) fatal_error("realloc failed.");
	return p;
}

char *load_file(char *path, int *size){
	FILE *f = fopen(path,"rb");
	if (!f){
		fatal_error("Failed to open %s",path);
	}
	fseek(f,0,SEEK_END);
	*size = ftell(f);
	fseek(f,0,SEEK_SET);
	char *bytes = malloc_or_die(*size);
	fread(bytes,1,*size,f);
	fclose(f);
	return bytes;
}

bool is_alpha_numeric(char c){
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

int rand_int(int n){
	if ((n - 1) == RAND_MAX){
		return rand();
	} else {
		// Chop off all of the values that would cause skew...
		int end = RAND_MAX / n; // truncate skew
		end *= n;
		// ... and ignore results from rand() that fall above that limit.
		// (Worst case the loop condition should succeed 50% of the time,
		// so we can expect to bail out of this loop pretty quickly.)
		int r;
		while ((r = rand()) >= end);
		return r % n;
	}
}

int rand_int_range(int min, int max){
	return rand_int(max-min+1) + min;
}

uint32_t fnv_1a(char *key, int keylen){
	uint32_t index = 2166136261u;
	for (int i = 0; i < keylen; i++){
		index ^= key[i];
		index *= 16777619;
	}
	return index;
}

int modulo(int i, int m){
	return (i % m + m) % m;
}

void cstr_to_string(char *cstr, String *s){
	s->len = strlen(cstr);
	s->data = malloc_or_die(s->len);
	memcpy(s->data,cstr,s->len);
}

void string_to_lower(size_t len, char *str){
	for (size_t i = 0; i < len; i++){
		str[i] = tolower(str[i]);
	}
}