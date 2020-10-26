#include <stdio.h>
#include <stdlib.h>

typedef struct fish {
  double size;
} fish;

/* A school of fish is has an `int` for the number of fish followed
   by an array of fish. */

#define SCHOOL_COUNT(p)     *(int *)p
#define FISH_START(p)       (char *)p + sizeof(int)
#define NTH_FISH_SIZE(p, n) ((fish *)FISH_START(p))[n].size

static void* make_school_of_fish(int n) {
  void* p = malloc(sizeof(int) + n * sizeof(fish));
  int i;

  SCHOOL_COUNT(p) = n;
  for(i = 0; i < n; i++)
    NTH_FISH_SIZE(p, i) = i;

  return p;
}

int main() {
  void* p = make_school_of_fish(10);
  int i;

  for(i = 0; i < SCHOOL_COUNT(p); i++)
    printf(" %f", NTH_FISH_SIZE(p, i));
  printf("\n");

  return 0;
}
