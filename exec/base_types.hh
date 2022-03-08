#pragma once

#include <cstdint>

#ifndef DISALLOW_COPY
#define DISALLOW_COPY(clz)     \
    clz(const clz &) = delete; \
    void operator=(const clz &) = delete

#define DISALLOW_MOVE(clz) \
    clz(clz &&) = delete;  \
    void operator=(clz &&) = delete
#endif
