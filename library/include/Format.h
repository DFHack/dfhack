#pragma once

#ifdef USE_FMTLIB

#include <fmt/format.h>
#include <fmt/std.h>
#include <fmt/ostream.h>

#else

#include <format>

namespace fmt = std;

#endif
