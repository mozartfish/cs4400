#include <stdio.h>

int main()
{
  int v;

  printf("give an integer as input: ");

  /* Read an integer from stdin using the scanf()
     function, and put the integer into v: */
  scanf("%d", &v);

  /* Print it back out: */
  printf("got %d\n", v);

  return 0;
}
