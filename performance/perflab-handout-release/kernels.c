/*******************************************
 * Solutions for the CS:APP Performance Lab
 ********************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following student struct 
 */
student_t student = {
    "Pranav Rajan",      /* Full name */
    "u1136324@utah.edu", /* Email address */
};

/***************
 * COMPLEX KERNEL
 ***************/

/******************************************************
 * Your different versions of the complex kernel go here
 ******************************************************/

/* 
 * naive_complex - The naive baseline version of complex 
 */
char naive_complex_descr[] = "naive_complex: Naive baseline implementation";
void naive_complex(int dim, pixel *src, pixel *dest)
{
  int i, j;

  for (i = 0; i < dim; i++)
    for (j = 0; j < dim; j++)
    {

      // pixel  red = (int)src[RIDX(i, j, dim)].red
      // pixel  blue = (int)src[RIDX(i, j, dim)].blue
      // pixel  green = (int)src[RIDX(i, j, dim)].green
      dest[RIDX(dim - j - 1, dim - i - 1, dim)].red = ((int)src[RIDX(i, j, dim)].red +
                                                       (int)src[RIDX(i, j, dim)].green +
                                                       (int)src[RIDX(i, j, dim)].blue) /
                                                      3;

      dest[RIDX(dim - j - 1, dim - i - 1, dim)].green = ((int)src[RIDX(i, j, dim)].red +
                                                         (int)src[RIDX(i, j, dim)].green +
                                                         (int)src[RIDX(i, j, dim)].blue) /
                                                        3;

      dest[RIDX(dim - j - 1, dim - i - 1, dim)].blue = ((int)src[RIDX(i, j, dim)].red +
                                                        (int)src[RIDX(i, j, dim)].green +
                                                        (int)src[RIDX(i, j, dim)].blue) /
                                                       3;
    }
}
/* 
 * first_complex - The first re-write version of complex
 */
char first_complex_descr[] = "first_complex: reduce addition";
void first_complex(int dim, pixel *src, pixel *dest)
{
  int i, j;
  for (i = 0; i < dim; i++)
    for (j = 0; j < dim; j++)
    {
      pixel p = src[RIDX(i, j, dim)];
      int colorVal = (int)(p.red + p.green + p.blue) / 3;
      int pIndex = (RIDX(dim - j - 1, dim - i - 1, dim));
      dest[pIndex].red = colorVal;
      dest[pIndex].green = colorVal;
      dest[pIndex].blue = colorVal;
    }
}
/* 
 * second_complex - The second re-write version of complex
 */
char second_complex_descr[] = "second_complex: loop unrolling";
void second_complex(int dim, pixel *src, pixel *dest)
{
  int i, j, ii, jj, incr_i, incr_j;

  // increment value
  incr_i = 32;
  incr_j = 4;

  for (ii = 0; ii < dim; ii += incr_i)
  {
    int dimConst = dim - 1;
    for (jj = 0; jj < dim; jj += incr_j)
    {
      for (i = ii; i < ii + incr_i; i++)
      {
        int di = dimConst - i;
        for (j = jj; j < jj + incr_j; j++)
        {
          int dj = dimConst - j;
          pixel p = src[RIDX(i, j, dim)];
          int colorVal = (int)(p.red + p.green + p.blue) / 3;
          int pIndex = (RIDX(dj, di, dim));
          dest[pIndex].red = colorVal;
          dest[pIndex].green = colorVal;
          dest[pIndex].blue = colorVal;
        }
      }
    }
  }
}
/* 
 * complex - Your current working version of complex
 * IMPORTANT: This is the version you will be graded on
 */
char complex_descr[] = "complex: Current working version";
void complex(int dim, pixel *src, pixel *dest)
{
  second_complex(dim, src, dest);
}

/*********************************************************************
 * register_complex_functions - Register all of your different versions
 *     of the complex kernel with the driver by calling the
 *     add_complex_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_complex_functions()
{
  add_complex_function(&complex, complex_descr);
  // add_complex_function(&second_complex, first_complex_descr);
  add_complex_function(&first_complex, first_complex_descr);
  add_complex_function(&naive_complex, naive_complex_descr);
}

/***************
 * MOTION KERNEL
 **************/

/***************************************************************
 * Various helper functions for the motion kernel
 * You may modify these or add new ones any way you like.
 **************************************************************/

/* 
 * weighted_combo - Returns new pixel value at (i,j) 
 */
static pixel weighted_combo(int dim, int i, int j, pixel *src)
{
  int ii, jj;
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  int num_neighbors = 0;
  for (ii = 0; ii < 3; ii++)
    for (jj = 0; jj < 3; jj++)
      if ((i + ii < dim) && (j + jj < dim))
      {
        num_neighbors++;
        red += (int)src[RIDX(i + ii, j + jj, dim)].red;
        green += (int)src[RIDX(i + ii, j + jj, dim)].green;
        blue += (int)src[RIDX(i + ii, j + jj, dim)].blue;
      }

  current_pixel.red = (unsigned short)(red / num_neighbors);
  current_pixel.green = (unsigned short)(green / num_neighbors);
  current_pixel.blue = (unsigned short)(blue / num_neighbors);

  return current_pixel;
}
/**
 * Function that handles the 3x3 combo case
 * */
__attribute__((always_inline)) static pixel three_combo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  // grab three rows of pixels,
  // For 32 x 32, we reduce it to 30x30
  int row1 = i * dim;
  int row2 = (i + 1) * dim;
  int row3 = (i + 2) * dim;

  // get pixels for each row
  int r1_p1 = row1 + j + 0;
  int r1_p2 = row1 + j + 1;
  int r1_p3 = row1 + j + 2;

  int r2_p1 = row2 + j + 0;
  int r2_p2 = row2 + j + 1;
  int r2_p3 = row2 + j + 2;

  int r3_p1 = row3 + j + 0;
  int r3_p2 = row3 + j + 1;
  int r3_p3 = row3 + j + 2;

  // sum all the pixel colors for the rows
  // row 1 pixels
  red += (int)src[r1_p1].red + (int)src[r1_p2].red + (int)src[r1_p3].red;
  green += (int)src[r1_p1].green + (int)src[r1_p2].green + (int)src[r1_p3].green;
  blue += (int)src[r1_p1].blue + (int)src[r1_p2].blue + (int)src[r1_p3].blue;

  // row 2 pixels
  red += (int)src[r2_p1].red + (int)src[r2_p2].red + (int)src[r2_p3].red;
  green += (int)src[r2_p1].green + (int)src[r2_p2].green + (int)src[r2_p3].green;
  blue += (int)src[r2_p1].blue + (int)src[r2_p2].blue + (int)src[r2_p3].blue;

  // row 3 pixels
  red += (int)src[r3_p1].red + (int)src[r3_p2].red + (int)src[r3_p3].red;
  green += (int)src[r3_p1].green + (int)src[r3_p2].green + (int)src[r3_p3].green;
  blue += (int)src[r3_p1].blue + (int)src[r3_p2].blue + (int)src[r3_p3].blue;

  // set the rgb for the current pixel
  current_pixel.red = (unsigned short)(red / 9);
  current_pixel.green = (unsigned short)(green / 9);
  current_pixel.blue = (unsigned short)(blue / 9);

  // return the pixel
  return current_pixel;
}
/**
 * Function that handles the 3x2 combo case
 * */
__attribute__((always_inline)) static pixel two_combo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  // grab 3 rows of pixels
  // For 32x32 we are handling the case dealing with 31 and 32 since 30x30 is already handled
  int row1 = i * dim;
  int row2 = (i + 1) * dim;
  int row3 = (i + 2) * dim;

  // get pixels for each row
  int r1_p1 = row1 + j + 0;
  int r1_p2 = row1 + j + 1;

  int r2_p1 = row2 + j + 0;
  int r2_p2 = row2 + j + 1;

  int r3_p1 = row3 + j + 0;
  int r3_p2 = row3 + j + 1;

  // sum all the pixel colors for the rows
  // row1 pixels
  red += (int)src[r1_p1].red + (int)src[r1_p2].red;
  green += (int)src[r1_p1].green + (int)src[r1_p2].green;
  blue += (int)src[r1_p1].blue + (int)src[r1_p2].blue;

  // row2 pixels
  red += (int)src[r2_p1].red + (int)src[r2_p2].red;
  green += (int)src[r2_p1].green + (int)src[r2_p2].green;
  blue += (int)src[r2_p1].blue + (int)src[r2_p2].blue;

  // row3 pixels
  red += (int)src[r3_p1].red + (int)src[r3_p2].red;
  green += (int)src[r3_p1].green + (int)src[r3_p2].green;
  blue += (int)src[r3_p1].blue + (int)src[r3_p2].blue;

  // set the rgb for the current pixel
  current_pixel.red = (unsigned short)(red / 6);
  current_pixel.green = (unsigned short)(green / 6);
  current_pixel.blue = (unsigned short)(blue / 6);

  // return the pixel
  return current_pixel;
}
/**
 * Function that handles the 3x1 combo case
 * */
__attribute__((always_inline)) static pixel one_combo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  // grab 3 rows of pixels
  // For 32x32 we are handling the case dealing with 32 case since the 30 case and 31 case have been handled
  int row1 = i * dim;
  int row2 = (i + 1) * dim;
  int row3 = (i + 2) * dim;

  // get pixels for each row
  int r1_p1 = row1 + j + 0;

  int r2_p1 = row2 + j + 0;

  int r3_p1 = row3 + j + 0;

  // sum all the pixel colors for the rows
  // row1 pixels
  red += (int)src[r1_p1].red;
  green += (int)src[r1_p1].green;
  blue += (int)src[r1_p1].blue;

  // row2 pixels
  red += (int)src[r2_p1].red;
  green += (int)src[r2_p1].green;
  blue += (int)src[r2_p1].blue;

  // row3 pixels
  red += (int)src[r3_p1].red;
  green += (int)src[r3_p1].green;
  blue += (int)src[r3_p1].blue;

  // set the rgb for the current pixel
  current_pixel.red = (unsigned short)(red / 3);
  current_pixel.green = (unsigned short)(green / 3);
  current_pixel.blue = (unsigned short)(blue / 3);

  // return the pixel
  return current_pixel;
}

/**
 * Function that handles the last two rows combo case (2x3 case)
 * */
__attribute__((always_inline)) static pixel bottom_two_rows_combo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  // grab two rows of pixels,
  // For 32 x 32, we are handling the case where i = 31, 32
  int row1 = i * dim;
  int row2 = (i + 1) * dim;

  // get pixels for each row
  int r1_p1 = row1 + j + 0;
  int r1_p2 = row1 + j + 1;
  int r1_p3 = row1 + j + 2;

  int r2_p1 = row2 + j + 0;
  int r2_p2 = row2 + j + 1;
  int r2_p3 = row2 + j + 2;

  // sum all the pixel colors for the rows
  // row 1 pixels
  red += (int)src[r1_p1].red + (int)src[r1_p2].red + (int)src[r1_p3].red;
  green += (int)src[r1_p1].green + (int)src[r1_p2].green + (int)src[r1_p3].green;
  blue += (int)src[r1_p1].blue + (int)src[r1_p2].blue + (int)src[r1_p3].blue;

  // row 2 pixels
  red += (int)src[r2_p1].red + (int)src[r2_p2].red + (int)src[r2_p3].red;
  green += (int)src[r2_p1].green + (int)src[r2_p2].green + (int)src[r2_p3].green;
  blue += (int)src[r2_p1].blue + (int)src[r2_p2].blue + (int)src[r2_p3].blue;

  // set the rgb for the current pixel
  current_pixel.red = (unsigned short)(red / 6);
  current_pixel.green = (unsigned short)(green / 6);
  current_pixel.blue = (unsigned short)(blue / 6);

  // return the pixel
  return current_pixel;
}

/**
 * Function that handles the last row combo case
 * */
__attribute__((always_inline)) static pixel bottom_one_row_combo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  // grab one of pixels,
  // For 32 x 32, we are handling the case where i = 32 (the final row)
  int row1 = i * dim;

  // get pixels for each row
  int r1_p1 = row1 + j + 0;
  int r1_p2 = row1 + j + 1;
  int r1_p3 = row1 + j + 2;

  // sum all the pixel colors for the rows
  // row 1 pixels
  red += (int)src[r1_p1].red + (int)src[r1_p2].red + (int)src[r1_p3].red;
  green += (int)src[r1_p1].green + (int)src[r1_p2].green + (int)src[r1_p3].green;
  blue += (int)src[r1_p1].blue + (int)src[r1_p2].blue + (int)src[r1_p3].blue;

  // set the rgb for the current pixel
  current_pixel.red = (unsigned short)(red / 3);
  current_pixel.green = (unsigned short)(green / 3);
  current_pixel.blue = (unsigned short)(blue / 3);

  // return the pixel
  return current_pixel;
}

/**
 * Function that handles the 2x2 case
 * */
__attribute__((always_inline)) static pixel two_by_two_combo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  // grab two rows of pixels,
  int row1 = i * dim;
  int row2 = (i + 1) * dim;

  // get pixels for each row
  int r1_p1 = row1 + j + 0;
  int r1_p2 = row1 + j + 1;

  int r2_p1 = row2 + j + 0;
  int r2_p2 = row2 + j + 1;

  // sum all the pixel colors for the rows
  // row 1 pixels
  red += (int)src[r1_p1].red + (int)src[r1_p2].red;
  green += (int)src[r1_p1].green + (int)src[r1_p2].green;
  blue += (int)src[r1_p1].blue + (int)src[r1_p2].blue;

  // row 2 pixels
  red += (int)src[r2_p1].red + (int)src[r2_p2].red;
  green += (int)src[r2_p1].green + (int)src[r2_p2].green;
  blue += (int)src[r2_p1].blue + (int)src[r2_p2].blue;

  // set the rgb for the current pixel
  current_pixel.red = (unsigned short)(red / 4);
  current_pixel.green = (unsigned short)(green / 4);
  current_pixel.blue = (unsigned short)(blue / 4);

  // return the pixel
  return current_pixel;
}

/**
 * Function that handles the 2x1 case
 * */
__attribute__((always_inline)) static pixel two_by_one_combo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  // grab two rows of pixels,
  int row1 = i * dim;
  int row2 = (i + 1) * dim;

  // get pixels for each row
  int r1_p1 = row1 + j + 0;

  int r2_p1 = row2 + j + 0;

  // sum all the pixel colors for the rows
  // row 1 pixels
  red += (int)src[r1_p1].red;
  green += (int)src[r1_p1].green;
  blue += (int)src[r1_p1].blue;

  // row 2 pixels
  red += (int)src[r2_p1].red;
  green += (int)src[r2_p1].green;
  blue += (int)src[r2_p1].blue;

  // set the rgb for the current pixel
  current_pixel.red = (unsigned short)(red / 2);
  current_pixel.green = (unsigned short)(green / 2);
  current_pixel.blue = (unsigned short)(blue / 2);

  // return the pixel
  return current_pixel;
}

/**
 * Function that handles the 1x2 case
 * */
__attribute__((always_inline)) static pixel one_by_two_combo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  // grab 1 rows of pixels,
  int row1 = i * dim;

  // get pixels for each row
  int r1_p1 = row1 + j + 0;
  int r1_p2 = row1 + j + 1;

  // sum all the pixel colors for the rows
  // row 1 pixels
  red += (int)src[r1_p1].red + (int)src[r1_p2].red;
  green += (int)src[r1_p1].green + (int)src[r1_p2].green;
  blue += (int)src[r1_p1].blue + (int)src[r1_p2].blue;

  // set the rgb for the current pixel
  current_pixel.red = (unsigned short)(red / 2);
  current_pixel.green = (unsigned short)(green / 2);
  current_pixel.blue = (unsigned short)(blue / 2);

  // return the pixel
  return current_pixel;
}

/**
 * Function that handles the 1x1 case
 * */
__attribute__((always_inline)) static pixel one_by_one_combo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  // grab 1 rows of pixels,
  int row1 = i * dim;

  // get pixels for each row
  int r1_p1 = row1 + j + 0;

  // sum all the pixel colors for the rows
  red += (int)src[r1_p1].red;
  green += (int)src[r1_p1].green;
  blue += (int)src[r1_p1].blue;

  // set the rgb for the current pixel
  current_pixel.red = (unsigned short)(red / 1);
  current_pixel.green = (unsigned short)(green / 1);
  current_pixel.blue = (unsigned short)(blue / 1);

  // return the pixel
  return current_pixel;
}

/******************************************************
 * Your different versions of the motion kernel go here
 ******************************************************/

/*
 * naive_motion - The naive baseline version of motion 
 */
char naive_motion_descr[] = "naive_motion: Naive baseline implementation";
void naive_motion(int dim, pixel *src, pixel *dst)
{
  int i, j;

  for (i = 0; i < dim; i++)
    for (j = 0; j < dim; j++)
      dst[RIDX(i, j, dim)] = weighted_combo(dim, i, j, src);
}

char first_motion_descr[] = "first_motion: first attempt implementation";
void first_motion(int dim, pixel *src, pixel *dst)
{
  int i, j;

  // 30x30 case, i = 30 j = 30
  for (i = 0; i < dim - 2; i++)
  {
    for (j = 0; j < dim - 2; j++)
    {
      dst[RIDX(i, j, dim)] = three_combo(dim, i, j, src);
    }
    // j = 31 and 32 case
    dst[RIDX(i, j, dim)] = two_combo(dim, i, j, src);

    // j = 32 case
    dst[RIDX(i, j + 1, dim)] = one_combo(dim, i, j + 1, src);
  }

  // At this point have up to i = 30 where i represents rows and j represents cols
  // 2 rows for i = 31, 32
  int k;
  for (k = 0; k < dim - 2; k++)
  {
    dst[RIDX(i, k, dim)] = bottom_two_rows_combo(dim, i, k, src);
    dst[RIDX(i + 1, k, dim)] = bottom_one_row_combo(dim, i + 1, k, src);
  }

  // 2x2 case
  dst[RIDX(i, j, dim)] = two_by_two_combo(dim, i, j, src);
  // 2x1 case
  dst[RIDX(i, j + 1, dim)] = two_by_one_combo(dim, i, j + 1, src);
  // 1x2 case
  dst[RIDX(i + 1, j, dim)] = one_by_two_combo(dim, i + 1, j, src);
  // 1x1 case
  dst[RIDX(i + 1, j + 1, dim)] = one_by_one_combo(dim, i + 1, j + 1, src);
}

/**
 * Implement the general case of motion (the non-edge pixel case)
**/

/**
 * motion - Your current working version of motion. 
 * IMPORTANT: This is the version you will be graded on
 */
char motion_descr[] = "motion: Current working version";
void motion(int dim, pixel *src, pixel *dst)
{
  first_motion(dim, src, dst);
  // naive_motion(dim, src, dst);
}

/********************************************************************* 
 * register_motion_functions - Register all of your different versions
 *     of the motion kernel with the driver by calling the
 *     add_motion_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_motion_functions()
{
  add_motion_function(&motion, motion_descr);
  add_motion_function(&naive_motion, naive_motion_descr);
  // add_motion_function(&first_motion, first_motion_descr);
}
