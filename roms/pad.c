#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    const char *f = argv[1];
    int size = atoi(argv[2]);

    return truncate(f, size);
}

