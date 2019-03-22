//======================================================================
//  bimg.c - A binary image class in C.
//
//  Dov Grobgeld <dov.grobgeld@gmail.com>
//  Sat Mar 23 09:48:50 2019
//----------------------------------------------------------------------
#include "bimg.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h>


static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt); 
    
    vfprintf(stderr, fmt, ap);
    exit(-1);
}

struct bimg_t {
    int width, height, stride;
    uint8_t *buf;
};

bimg_t *bimg_new(int width, int height, int stride)
{
    bimg_t *bimg = (bimg_t*)malloc(sizeof(bimg_t));
    if (stride%8!=0)
        die("stride must be divisible by 8!\n");
    bimg->width = width;
    bimg->height = height;
    bimg->stride = stride;
    bimg->buf = (uint8_t*)malloc(stride*width);
    return bimg;
}

void bimg_free(bimg_t* bimg)
{
    free(bimg->buf);
    free(bimg);
}

// Inverse all the bits ("not") in a single line
static void not_line(void *line_start,size_t line_size_in_bytes)
{
  size_t i,words_in_line = line_size_in_bytes / sizeof(size_t);
  size_t *long_line_ptr = (size_t *)line_start;

  for(i = 0; i < words_in_line; i++)
    *long_line_ptr++ ^= (size_t)~0;

  uint8_t *byte_line_ptr = (uint8_t *)long_line_ptr;

  for(i = 0; i < line_size_in_bytes - words_in_line * sizeof(size_t); i++)
    *byte_line_ptr++ ^= (uint8_t)~0;
}

static double rgb_to_intensity(int red, int green, int blue)
{
  return 0.299 * red+ 0.587*green+0.114*blue;
}

bimg_t *bimg_new_from_png(const char *filename)
{
    png_bytep *row_pointers;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        die("Failed opening file.");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png)
        die("Failed creating png structure!");

    png_infop info = png_create_info_struct(png);
    if(!info)
        die("Failed creating info structure!");
  
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;

    // This is delegated reading through MyReadData.
    png_init_io(png, fp);
    png_read_info(png, info);

    png_get_IHDR(png, info, &width, &height,
                 &bit_depth, &color_type, &interlace_type,
                 &compression_type, &filter_method);

    int do_invert = 0;
    if (bit_depth == 1) {
        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_colorp Palette;
            int PalleteNum;
            png_get_PLTE(png, info, &Palette, &PalleteNum);
      
            // Invert if the intensity of the first entry in the color table is brighter
            // than the second entry.
            double intensity[2];
            for (int i=0; i<2; i++)
                intensity[i] = rgb_to_intensity(Palette[i].red, Palette[i].green, Palette[i].blue);
            if (intensity[0] > intensity[1])
                do_invert = 1;
        }
    }
    else
        die("Only 1-bit png files supported!");

    int stride = (width + 7)/8;;
    bimg_t *bimg = bimg_new(width, height, stride);
    uint8_t *buf = bimg->buf;

    row_pointers = (png_byte** ) png_malloc (png, height * sizeof (png_byte *));
    for(int row_idx = 0; row_idx < height; row_idx++) 
        row_pointers[row_idx] = buf + row_idx * stride;
    png_read_image(png, row_pointers);

    if (do_invert)
        for (int row_idx=0; row_idx< height; row_idx++)
            not_line(row_pointers[row_idx], stride);

    png_free (png, row_pointers);

    return bimg;
}

int bimg_save_png(bimg_t*bimg, const char *filename)
{
    int save_status = -1;
  
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_byte ** row_pointers = NULL;

    //Open the file for writing
    FILE *fh = fopen(filename, "wb");
  
    if (!fh)
        die("Failed opening file!");

    //Prepare PNG data
    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
        die("Failed creating write struct!\n");

    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL)
        die("Failed creating info struct!\n");

    //Set up error handling.
    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure; 
    }
  
    //Set image attributes.
    png_set_IHDR (png_ptr,
                  info_ptr,
                  bimg->width,
                  bimg->height,
                  1, // BitDepth
                  PNG_COLOR_TYPE_GRAY,
                  PNG_INTERLACE_NONE, 
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);


    //Initialize rows of PNG.
    row_pointers = (png_byte** ) png_malloc (png_ptr, bimg->height * sizeof (png_byte *));
    for (int y = 0; y < bimg->height; y++)
        row_pointers[y] = bimg->buf + y * bimg->stride;

    //Write the image data to the file handle
    png_init_io (png_ptr, fh);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);


    //Writing Succeeded
    save_status = 0;
    png_free (png_ptr, row_pointers);

 png_failure:
    png_destroy_write_struct (&png_ptr, &info_ptr);
    fclose (fh);
    return save_status;
}

