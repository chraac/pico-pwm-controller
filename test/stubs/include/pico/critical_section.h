#pragma once

// Fake pico/critical_section.h for host unit tests (single-threaded: no-ops).
#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct critical_section {
    int core_num;
} critical_section_t;

void critical_section_init(critical_section_t *sec);
void critical_section_deinit(critical_section_t *sec);
void critical_section_enter_blocking(critical_section_t *sec);
void critical_section_exit(critical_section_t *sec);

#ifdef __cplusplus
}
#endif
