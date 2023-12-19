#include <image.h>

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