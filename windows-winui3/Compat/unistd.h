#pragma once

#include <direct.h>
#include <io.h>
#include <process.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <windows.h>

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef ssize_t
typedef intptr_t ssize_t;
#endif

#define read _read
#define write _write
#define close _close
#define getpid _getpid
#define isatty _isatty

static inline unsigned int sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}
