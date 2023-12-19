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

void img_rect_decompose(Image *img, ColorRectList *crl, int min_dim){
	img_alpha255(img);//force max alpha for image because rect_decompose relies on 0-value pixels
	for (int y = 0; y < img->height; y++){
		for (int x = 0; x < img->width; x++){
			uint32_t c = img->pixels[y*img->width+x];
			if (c){
				int i, j;
				int lim = img->width;
				for (j = y; j < img->height; j++){
					for (i = x; i < lim; i++){
						if (img->pixels[j*img->width+i] != c){
							if (lim != img->width && i!=lim){
								i = lim;
								goto L0;
							} else {
								lim = i;
							}
						}
					}
				}
			L0:;
				int width = i-x;
				int height = j-y;
				if (width >= min_dim && height >= min_dim){
					ColorRect *r = ColorRectListMakeRoom(crl,1);
					r->left = x;
					r->right = i;
					r->top = y;
					r->bottom = j;
					r->color = c;
					for (int b = y; b < j; b++){
						for (int a = x; a < i; a++){
							img->pixels[b*img->width+a] = 0;
						}
					}
				}
			}
		}
	}
	memset(img->pixels,0,img->width*img->height*sizeof(*img->pixels));
	for (ColorRect *r = crl->elements; r < crl->elements+crl->used; r++){
		uint32_t color = RGBA(rand_int(256),rand_int(256),rand_int(256),255);
		for (int y = r->top; y < r->bottom; y++){
			for (int x = r->left; x < r->right; x++){
				img->pixels[y*img->width+x] = color;
			}
		}
	}
}