#pragma once

#include "pico/stdio.h"
#include "base_types.hh"

#define log_info(format,args...) printf(format, ## args)
#ifdef DEBUG
#define log_debug(format,args...) printf(format, ## args)
#else
#define log_debug(format,args...) (void)0
#endif

