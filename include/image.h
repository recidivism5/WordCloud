#pragma once

#include <base.h>

#include <png.h>
#include <jpeglib.h>
#include <jerror.h>

TSTRUCT(Image){
	int width,height;
	uint32_t *pixels;
};

TSTRUCT(Image8){
	int width,height;
	uint8_t *pixels;
};

void load_image(Image *img, char *path);