#include <stdio.h>

int main() {
  char s[8];

 fgets(s, 8, stdin);
  printf("Got %s\n", s);

  return 0;
}
