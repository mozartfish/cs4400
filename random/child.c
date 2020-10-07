#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int i;
    for (i = 0; i < argc; i++)
        printf("%s ", argv[i]);
    printf("%d", getpid());
    return 0;
}