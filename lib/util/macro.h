#ifdef __PEREVOD_DEBUG_MODE__
#include <iostream>

#define PEREVOD_DEBUG_LOG(x) std::cout << "[Perevod] " << x << std::endl;
#define PEREVOD_DEBUG_PRETTY_LOG(x) std::cout << "[Perevod] " << __PRETTY_FUNCTION__ << " " << x << std::endl;
#else
#define PEREVOD_DEBUG_LOG(x)
#define PEREVOD_DEBUG_PRETTY_LOG(x)
#endif