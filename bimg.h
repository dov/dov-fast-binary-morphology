//======================================================================
//  bimg.h - A binary image class in C.
//
//  Dov Grobgeld <dov.grobgeld@gmail.com>
//  Sat Mar 23 09:48:50 2019
//----------------------------------------------------------------------
#ifndef BIMG_H
#define BIMG_H

#include <stdint.h>

typedef struct {
    int width, height, stride;
    uint8_t *buf;
} bimg_t;

bimg_t *bimg_new(int width, int height, int stride);
bimg_t *bimg_new_from_png(const char *filename);
int bimg_save_png(bimg_t*bimg, const char *filename);
void bimg_free(bimg_t*);

#endif /* BIMG */
