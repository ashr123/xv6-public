#pragma once
#include "user.h"
#include "fcntl.h"

int isFileExists(const char *str)
{
    const int fd = open(str, O_RDONLY);
    close(fd);
    return fd >= 0;
}