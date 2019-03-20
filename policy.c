#include "user.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf(2, "usage: policy not given...\n");
        exit(1);
    }
    policy(atoi(argv[1]));

    exit(0);
}