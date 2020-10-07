#include <unistd.h>

extern char **environ;

int main(int argc, char **argv)
{
    argv[0] = "child";
    execve("child", argv, environ);
    return 0;
}