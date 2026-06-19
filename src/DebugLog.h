#pragma once

#ifdef ENABLE_DEBUG_LOG
    #include <cstdio>
    #define DEBUG_LOG(...) std::printf(__VA_ARGS__)
#else
    #define DEBUG_LOG(...) ((void)0)
#endif
