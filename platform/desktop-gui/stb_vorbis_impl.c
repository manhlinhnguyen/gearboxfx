/*
 * stb_vorbis OGG decoder — compiled as a plain C translation unit so that
 * stb_vorbis.c's internal macros (e.g. #define C ...) do not pollute the
 * C++ template code in spdlog/fmtlib.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "stb_vorbis.c"
#pragma GCC diagnostic pop
