#pragma once

#include "renderer.h"

#include "image_effects.h"

void img_greyscale(Image *img);

void img_gaussian_blur(Image *img, int strength);

void img_quantize(Image *img, int divisions);