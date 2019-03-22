// Fast mathematical morphological operators on 1-bit images.
//
// Dov Grobgeld
// 2019-01-06 Sun

#ifndef MORPH_H
#define MORPH_H

#include "bimg.h"

// Do an inline morphological erosion.
void morph_erode8(bimg_t *bimg);
void morph_erode4(bimg_t *bimg);
void morph_erode_isotropic(bimg_t *bimg, int steps_num);

#endif
