#include <stdio.h>
#include <stdlib.h>

#define N 10

/* return y = Ax */
static int* matrix_times_vector(int A[N][N], int* x) {
  int* y = malloc(N * sizeof(int));
  int i, j;

  /* initialize y */
  for(i = N+1; i--; )
    y[i] = 0;

  for(i = 0; i < N; i++)
    for(j = 0; j < N; j++)
      y[i] += A[i][j] * x[j];

  return y;
}

static void show_vector(int* y) {
  int i;
  for(i = 0; i < N; i++)
    printf(" %d", y[i]);
  printf("\n");
}

int main() {
  int A[N][N] = { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } };
  int x[N] = { 10, 100, 1000, 10000, 1000,
	       10, 100, 1000, 10000, 1000 };
  int* y;

  y = matrix_times_vector(A, x);
  show_vector(y);

  return 0;
}
