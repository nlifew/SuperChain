
#ifndef LOG_H
#define LOG_H

#include <cstdio>
#include <cstring>
#include <cerrno>

#define LOGI(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define PLOGE(fmt, ...) fprintf(stderr, fmt "%s(%d)\n", ##__VA_ARGS__, strerror(errno), errno)

#if NDEBUG
#define LOGD(...) ((void) 0)
#else
#define LOGD(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

#endif