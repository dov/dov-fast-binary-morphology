// A test implementation of fast mathematical morphology in C/C++.
//
// This code is released according to BSD 3-Clause License
// 
// Dov Grobgeld <dov.grobgeld@gmail.com>
// 2019-03-22 Fri

#include <math.h>
#include "morph.h"
#include "string.h"
#include <stdlib.h>
#include "stdint.h"
#include <byteswap.h>

// Fast one bit left shift.
static void left_shift(uint8_t *buf,
                       int len,
                       // output
                       uint8_t *buf_out
                       )
{
    int ulen = len/8;
    int len_rest = len - ulen * 8;
    uint64_t *ubuf = (uint64_t*)buf;
    uint64_t *ubuf_out = (uint64_t*)buf_out;

    // Left shift of entire uint64_t's.
    for (int i=0; i<ulen; i++) {
        uint64_t b = __builtin_bswap64(ubuf[i]) << 1;
        if (i<ulen-1)
            b|= (ubuf[i+1] >> 7)&1;
        ubuf_out[i] = __builtin_bswap64(b);
    }
  
    // Fix up right-most bit of the last uint64
    if (ulen > 0 && len_rest>0)
        buf_out[8*ulen-1]|= (buf[8*ulen]>>7)&1;

    // And the remaining uint8_t's.
    for (int i=len - len_rest; i<len; i++) {
        uint8_t b = buf[i] << 1;
        if (i<len-1)
            b|= (buf[i+1]&0x80)>>7;
        buf_out[i] = b;
    }
}

// Fast rightwise bit shift.
static void right_shift(uint8_t *buf,
                        int len,
                        // output
                        uint8_t *buf_out
                        )
{
    int ulen = len/8;
    int len_rest = len - ulen * 8;
    uint64_t *ubuf = (uint64_t*)buf;
    uint64_t *ubuf_out = (uint64_t*)buf_out;

    // The rightmost uint8_t's
    for (int i=len-1; i>=len-len_rest; i--) {
        uint8_t b = buf[i] >> 1;
        if (i>0) 
            b|= (buf[i-1]&1)<<7;
        buf_out[i] = b;
    }

    // The rest of the uint64_t's
    for (int i=ulen-1; i>=0; i--) {
        uint64_t b = __builtin_bswap64(ubuf[i]) >> 1;
        if (i>0) 
            b|= (ubuf[i-1] << 7) & 0x8000000000000000L;    
        ubuf_out[i] = __builtin_bswap64(b);
    }
}

// Erode with a fixed 3x3 structure element. This may be expanded
// later.
void morph_erode_structure_element_3x3(bimg_t *bimg,
                                       int *se_buf)
{
    int se_list[9], se_n=0;
    for (int i=0; i<9; i++)
        if (se_buf[i])
            se_list[se_n++] = i;
    int *se_end = se_list + se_n;

    // Pointers for the 3x3 directions. These will be treated as a fifo buffer for each
    // row 
    int stride = bimg->stride;
    int height = bimg->height;
    uint8_t *buf = bimg->buf;
    int usize = sizeof(uint64_t);
    int ustride = stride / usize;
    int stride_rest = stride - ustride * usize;

    // The 3x3 directions surrounding each pixel under consideration.
    // These will be treated as a fifo buffer of depth 3.
    uint8_t *dir[9];
    for (int i=0; i<9; i++)
        dir[i] = (uint8_t*)malloc(stride);
      
    // Initialize the dir buffers
    for (int row_idx=0; row_idx<3; row_idx++) {
        // Fill up the new dir
        memcpy(dir[row_idx*3+1], buf + row_idx*stride, stride);
        left_shift(dir[row_idx*3+1],
                   stride,
                   // output
                   dir[row_idx*3]);
        right_shift(dir[row_idx*3+1],
                    stride,
                    // output
                    dir[row_idx*3+2]);
    }

    // Loop over the rows and erode each row
    for (int row_idx=1; row_idx<height-1; row_idx++) {
        uint64_t *row = (uint64_t*)(buf + row_idx * stride);  // used for output

        // First do it in native chunks
        for (int i=0; i<ustride; i++) {
            // Loop over structure element
            int *sp = se_list;
            uint64_t s= ((uint64_t*)dir[*sp++])[i];
            for (; sp != se_end; sp++)
                s &= ((uint64_t*)dir[*sp])[i];
            row[i] = s;
        }

        // Do the same algo for the remaining bytes.
        for (int i=stride-stride_rest; i<stride; i++) {
            int *sp = se_list;
            uint8_t s=dir[*sp++][i];
            // Here is the logics of the neighbors.
            for (; sp != se_end; sp++)
                s &= dir[*sp][i];
            ((uint8_t*)row)[i] = s;
        }

        if (row_idx == height-2)
            break;

        // Roll the pointers. Note that we need to keep the old pointers
        // around, not because of the data it points to (which will be
        // replaced after the loop), but because of the pointers that
        // point to allocated memory.
        uint8_t *old_dir[3] = { dir[0], dir[1], dir[2] };
        for (int i=0; i<3; i++) {
            dir[i] = dir[i+3];
            dir[i+3] = dir[i+6];
            dir[i+6] = old_dir[i];
        }

        // Fill up the new dir
        memcpy(dir[7], buf + (row_idx+2)*stride, stride);

        left_shift(dir[7],
                   stride,
                   // output
                   dir[6]);
        right_shift(dir[7],
                    stride,
                    // output
                    dir[8]);
    }
  
    // Cleanup.
    for (int i=0; i<9; i++)
        free(dir[i]);
}

void morph_erode8(bimg_t *bimg)
{
    int se_buf[9];

    for (int i=0; i<9; i++)
        se_buf[i] = 1;

    morph_erode_structure_element_3x3(bimg, se_buf);
}

void morph_erode4(bimg_t *bimg)
{
    int se_buf[9];
    for (int i=0; i<9; i++)
        se_buf[i] = (i%2==1) || (i==4); // Center is on

    morph_erode_structure_element_3x3(bimg, se_buf);
}

void morph_erode_isotropic(bimg_t *bimg, int steps)
{
    double diag_dist = 0;
    double diag_step = 2; // Because linear contributes sqrt(2)!
    for (int i=0; i<steps; i++) {
        if (fabs(diag_dist + diag_step - i) < fabs(diag_dist - i)) {
            morph_erode8(bimg);
            diag_dist += diag_step;
        }
        else
            morph_erode4(bimg);
    }
}
