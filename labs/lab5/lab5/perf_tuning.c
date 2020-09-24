#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

// Size of the input/output arrays
#define DIM 10000
// Number of tests to run
#define ITERS 100000
// Error tolerance for the check function
#define EPSILON 1e-6

// Put direct assembly in to our program
// to get the current clock value
unsigned long get_ticks() {
  unsigned int lo, hi;
  asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
  return (unsigned long)hi << 32 | lo;
}

// Initialize input with random numbers
void init_input(float* arr)
{
  srand(0);
  int i, j;
  // Random numbers between [0, 1]
  for(i = 0; i < DIM; i++)
    arr[i] = (float)rand() / INT_MAX;
}


// Verify that the window_average function produced the right result
void check(float* dst, float* src, int len)
{
  int i, j, w;
  float temp;
  for(i = 0; i < len; i++)
  {
    temp = 0.0f;

    // Sum the 3-number window centered at i
    for(j = i - 1; j <= i + 1; j++)
    {
      w = j;
      // Check for wraparound
      if(w == -1)
	w = len - 1;
      if(w == len)
	w = 0;

      temp += src[w];
    }    
    // Divide for average
    temp /= 3;

    if(fabs(dst[i] - temp) > EPSILON)
    {
      printf("ERROR: index %d has value %f, should be %f\n", i, dst[i], temp);
      exit(1);
    }
  }
}


// Compute the average of each set of consecutive 3 numbers in src, 
// and store them in dst
void window_average(float* dst, float* src, int len)
{
  int i, j, w;

  for(i = 0; i < len; i ++)
  {
    // Initialize the sum
    dst[i] = 0.0;
  
    // Sum the 3-number window centered at i
    for(j = i - 1; j <= i + 1; j++)
    {
      w = j;
      // Check for wraparound
      if(w == -1)
	w = len - 1;
      if(w == len)
	w = 0;

      dst[i] += src[w];
    }    
    // Divide for average
    dst[i] /= 3;
  }
}

int main(int argc, char** argv)
{
  int i;
  
  float* src = malloc(DIM * sizeof(float));
  float* dst = malloc(DIM * sizeof(float));

  // Populate src with some random numbers
  init_input(src);
  
  // timing variables
  unsigned long ticks, total_ticks;
  total_ticks = 0;

  // Run many times for more accurate timing.
  for(i = 0; i < ITERS; i++)
  {
    ticks = get_ticks();
    
    window_average(dst, src, DIM);

    total_ticks += get_ticks() - ticks;
  }


  // Turn this on for debugging
  // Also make sure to reduce DIM to a reasonable number (like 10)
#if 0
  printf("src:\n");
  for(i = 0; i < DIM; i++)
    printf("%f ", src[i]);
  printf("\ndst\n");
  for(i = 0; i < DIM; i++)
    printf("%f ", dst[i]);
  printf("\n");
#endif

  // Check the result
  check(dst, src, DIM);

  unsigned long elements = DIM * ITERS;

  printf("CPE: %f\n", (double)total_ticks / elements);  

}
