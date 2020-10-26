#include <stdio.h>

int main() {
  char s[8];

  gets(s);
  printf("Got %s\n", s);

  return 0;
}
