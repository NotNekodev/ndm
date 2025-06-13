#include <core/log.h>

#include <stdarg.h>
#include <stdio.h>

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stdout, "[\033[1;32mINFO\033[0m]: ");
    vfprintf(stdout, fmt, args);
    va_end(args);

    fprintf(stdout, "\n");
}

void log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "[\033[1;33mWARN\033[0m]: ");
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "[\033[1;31mERROR\033[0m]: ");
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}
