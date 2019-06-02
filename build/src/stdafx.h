#pragma once

#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <string>
#include <algorithm>

using f32     = float;
using f64     = double;
using uint16  = uint16_t;
using uint32  = uint32_t;
using int16   = int16_t;
using int32   = int32_t;
using int64   = int64_t;
using uint64  = uint64_t;
using string  = std::string;

#ifdef PLATFORM_WIN32
  #include <Windows.h>
#endif