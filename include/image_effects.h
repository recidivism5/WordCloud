#pragma once

#include <image.h>
#include <cglm/cglm.h>

TSTRUCT(ColorRect){
	int left,top,right,bottom;
	uint32_t color;
};

TSTRUCT(ColorRectList){
	int total,used;
	ColorRect *elements;
};

TSTRUCT(ivec2List){
	int total,used;
	ivec2 *elements;
};

ColorRect *ColorRectListMakeRoom(ColorRectList *list, int count);

ivec2 *ivec2ListMakeRoom(ivec2List *list, int count);

void ivec2ListAppend(ivec2List *list, ivec2 *elements, int count);

void img_alpha255(Image *img);

void img_greyscale(Image *img);

void img_gaussian_blur(Image *img, int strength);

void img_quantize(Image *img, int divisions);

void img_rect_decompose(Image *img, ColorRectList *crl, int min_dim);