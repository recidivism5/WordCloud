#include "image_effects.h"

ColorRect *ColorRectListMakeRoom(ColorRectList *list, int count){
	if (list->used+count > list->total){
		if (!list->total) list->total = 1;
		while (list->used+count > list->total) list->total *= 2;
		list->elements = realloc_or_die(list->elements,list->total*sizeof(*list->elements));
	}
	list->used += count;
	return list->elements+list->used-count;
}

ivec2 *ivec2ListMakeRoom(ivec2List *list, int count){
	if (list->used+count > list->total){
		if (!list->total) list->total = 1;
		while (list->used+count > list->total) list->total *= 2;
		list->elements = realloc_or_die(list->elements,list->total*sizeof(*list->elements));
	}
	list->used += count;
	return list->elements+list->used-count;
}

void ivec2ListAppend(ivec2List *list, ivec2 *elements, int count){
	memcpy(ivec2ListMakeRoom(list,count),elements,count*sizeof(*elements));
}

void img_alpha255(Image *img){
	for (int i = 0; i < img->width*img->height; i++){
		uint8_t *p = img->pixels+i;
		p[3] = 255;
	}
}

void img_greyscale(Image *img){
	for (int i = 0; i < img->width*img->height; i++){
		uint8_t *p = img->pixels+i;
		uint8_t grey = MIN(255,0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2]);
		p[0] = grey;
		p[1] = grey;
		p[2] = grey;
	}
}

void img_gaussian_blur(Image *img, int strength){
	float *kernel = malloc_or_die(strength*sizeof(*kernel));
	float disx = 0.0f;
	for (int i = 0; i < strength; i++){
		kernel[i] = expf(-0.5f*disx*disx)/sqrtf(2.0f*M_PI); //This is the gaussian distribution with mean=0, standard_deviation=1
		disx += 3.0f / (strength-1);						//it happens to pretty much reach zero at x=3, so we divide that range by strength-1 for a step interval.
	}
	float sum = 0.0f;
	for (int i = 0; i < strength; i++){
		sum += (i ? 2 : 1) * kernel[i]; //Our kernel is half of a full odd dimensional kernel. The first element is the center, the other elements have to be counted twice.
	}
	for (int i = 0; i < strength; i++){
		kernel[i] /= sum; //This is the normalization step
	}
	Image b;
	b.width = img->width;
	b.height = img->height;
	b.pixels = malloc_or_die(b.width*b.height*sizeof(*b.pixels));
	for (int y = 0; y < img->height; y++){
		for (int x = 0; x < img->width; x++){
			float sums[3] = {0,0,0};
			for (int dx = -strength+1; dx < strength-1; dx++){
				uint8_t *p = img->pixels+y*img->width+CLAMP(x+dx,0,img->width-1);
				for (int i = 0; i < 3; i++){
					sums[i] = MIN(1.0f,sums[i]+(p[i]/255.0f)*kernel[abs(dx)]);
				}
			}
			uint8_t *p = b.pixels+y*b.width+x;
			for (int i = 0; i < 3; i++){
				p[i] = sums[i]*255;
			}
			p[3] = ((uint8_t *)(img->pixels+y*img->width+x))[3];
		}
	}
	for (int x = 0; x < img->width; x++){
		for (int y = 0; y < img->height; y++){
			float sums[3] = {0,0,0};
			for (int dy = -strength+1; dy < strength-1; dy++){
				uint8_t *p = b.pixels+CLAMP(y+dy,0,b.height-1)*b.width+x;
				for (int i = 0; i < 3; i++){
					sums[i] = MIN(1.0f,sums[i]+(p[i]/255.0f)*kernel[abs(dy)]);
				}
			}
			uint8_t *p = img->pixels+y*img->width+x;
			for (int i = 0; i < 3; i++){
				p[i] = sums[i]*255;
			}
			p[3] = ((uint8_t *)(b.pixels+y*b.width+x))[3];
		}
	}
	free(kernel);
	free(b.pixels);
}

void img_quantize(Image *img, int divisions){
	int mins[3] = {255,255,255};
	int maxes[3] = {0,0,0};
	for (int i = 0; i < img->width*img->height; i++){
		uint8_t *p = img->pixels+i;
		for (int j = 0; j < 3; j++){
			if (p[j] < mins[j]) mins[j] = p[j];
			else if (p[j] > maxes[j]) maxes[j] = p[j];
		}
	}
	int *invals = malloc_or_die(divisions*3*sizeof(*invals));
	int *outvals = malloc_or_die(divisions*sizeof(*invals));
	int ids[3];
	for (int i = 0; i < 3; i++){
		ids[i] = (maxes[i]-mins[i])/divisions;
	}
	int od = 255/divisions;
	int ov = 0;
	for (int i = 0; i < 3; i++){
		for (int j = 0; j < divisions; j++){
			invals[i*divisions+j] = mins[i];
			mins[i] += ids[i];
		}
	}
	for (int i = 0; i < divisions; i++){
		outvals[i] = ov;
		ov += od;
	}
	for (int i = 0; i < 3; i++){
		invals[i*divisions+divisions-1] = maxes[i];
	}
	outvals[divisions-1] = 255;

	for (int i = 0; i < img->width*img->height; i++){
		uint8_t *p = img->pixels+i;
		for (int j = 0; j < 3; j++){
			for (int k = 1; k < divisions; k++){
				if (p[j] <= invals[j*divisions+k]){
					if (p[j]-invals[j*divisions+k-1] > invals[j*divisions+k]-p[j]){
						p[j] = outvals[k];
					} else {
						p[j] = outvals[k-1];
					}
					break;
				}
			}
		}
	}
	free(invals);
	free(outvals);
}

TSTRUCT(PPixel){
	PPixel *parent;
	vec2 avg;
	int count;
};

void img_rect_decompose(Image *img, ColorRectList *crl){
	/*
	1. Find center of mass of largest contiguous region. If that point is not in the region, find the nearest point to it that is.
	2. Expand a rectangle out from that point in all directions until it hits edges.
	3. Go to 1 until the largest contiguous region has a dimension below minDim.
	*/

	img_alpha255(img);//force max alpha for image because rect_decompose relies on 0-value pixels

	PPixel *pc = malloc_or_die(img->width*img->height*sizeof(*pc));
	while (1){
		//parent all pixels to themselves:
		bool active = false;
		for (int y = 0; y < img->height; y++){
			for (int x = 0; x < img->width; x++){
				if (img->pixels[y*img->width+x]) active = true;
				PPixel *p = pc+y*img->width+x;
				p->parent = p;
				p->avg[0] = 0;
				p->avg[1] = 0;
				p->count = 1;
			}
		}
		if (!active) break;
		//run north-west connected component labeling using union find:
		for (int y = 0; y < img->height; y++){
			for (int x = 0; x < img->width; x++){
				uint32_t c = img->pixels[y*img->width+x];
				if (c){
					PPixel *cur = pc+y*img->width+x;
					while (cur != cur->parent) cur = cur->parent;
					if (x > 0 && img->pixels[y*img->width+x-1]==c){
						PPixel *left = pc+y*img->width+x-1;
						while (left != left->parent) left = left->parent;
						cur->parent = left;
						if (y > 0 && img->pixels[(y-1)*img->width+x]==c){
							PPixel *up = pc+(y-1)*img->width+x;
							while (up != up->parent) up = up->parent;
							up->parent = left;
						}
					} else if (y > 0 && img->pixels[(y-1)*img->width+x]==c){
						PPixel *up = pc+(y-1)*img->width+x;
						while (up != up->parent) up = up->parent;
						cur->parent = up;
					}
				}
			}
		}
		//add child positions to running average:
		for (int y = 0; y < img->height; y++){
			for (int x = 0; x < img->width; x++){
				if (img->pixels[y*img->width+x]){
					PPixel *p = pc+y*img->width+x;
					while (p != p->parent) p = p->parent;
					p->avg[0] += (x - p->avg[0]) / p->count;
					p->avg[1] += (y - p->avg[1]) / p->count;
					p->count++;
				}
			}
		}
		//expand rectangles out from centers of mass:
		for (int y = 0; y < img->height; y++){
			for (int x = 0; x < img->width; x++){
				uint32_t c = img->pixels[y*img->width+x];
				if (c){
					PPixel *p = pc+y*img->width+x;
					while (p != p->parent) p = p->parent;
					if (p->count){
						p->count = 0;
						ColorRect *cr = ColorRectListMakeRoom(crl,1);
						cr->color = c;
						if (img->pixels[((int)p->avg[1])*img->width+(int)p->avg[0]] == c){
							cr->left = p->avg[0];
							cr->right = cr->left;
							cr->top = p->avg[1];
							cr->bottom = cr->top;
						} else {
							cr->left = x;
							cr->right = x;
							cr->top = y;
							cr->bottom = y;
						}
						while (1){
							bool expanded = false;
							if (cr->left > 0){
								for (int j = cr->top; j <= cr->bottom; j++){
									if (img->pixels[j*img->width+cr->left-1] != c){
										goto L1;
									}
								}
								cr->left--;
								expanded = true;
							}
						L1:
							if (cr->right < (img->width-1)){
								for (int j = cr->top; j <= cr->bottom; j++){
									if (img->pixels[j*img->width+cr->right+1] != c){
										goto L2;
									}
								}
								cr->right++;
								expanded = true;
							}
						L2:
							if (cr->top > 0){
								for (int i = cr->left; i <= cr->right; i++){
									if (img->pixels[(cr->top-1)*img->width+i] != c){
										goto L3;
									}
								}
								cr->top--;
								expanded = true;
							}
						L3:
							if (cr->bottom < (img->height-1)){
								for (int i = cr->left; i <= cr->right; i++){
									if (img->pixels[(cr->bottom+1)*img->width+i] != c){
										goto L4;
									}
								}
								cr->bottom++;
								expanded = true;
							}
						L4:
							if (!expanded) break;
						}
						for (int j = cr->top; j <= cr->bottom; j++){
							for (int i = cr->left; i <= cr->right; i++){
								img->pixels[j*img->width+i] = 0;
							}
						}
					}
				}
			}
		}
	}
	//give each rect a random color for visualization:
	for (ColorRect *cr = crl->elements; cr < crl->elements+crl->used; cr++){
		uint32_t c = RGBA(rand_int(256),rand_int(256),rand_int(256),255);
		for (int y = cr->top; y <= cr->bottom; y++){
			for (int x = cr->left; x <= cr->right; x++){
				img->pixels[y*img->width+x] = c;
			}
		}
	}
}