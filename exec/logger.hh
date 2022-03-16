#pragma once

#include "pico/stdio.h"
#include "base_types.hh"

#define log_info(format,args...) printf(format, ## args)
#define log_debug(format,args...) printf(format, ## args)
