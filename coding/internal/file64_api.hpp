#pragma once

#include "base/base.hpp"

// POSIX standart.
#include <sys/types.h>
#define fseek64 fseeko
#define ftell64 ftello

#include <cstdio>
