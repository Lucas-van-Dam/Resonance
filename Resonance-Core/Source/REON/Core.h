#ifdef REON_PLATFORM_WINDOWS
#else
	#error Resonance does not support any other platform other than windows
#endif

#ifdef REON_ENABLE_ASSERTS
#define REON_CORE_ASSERT(x, ...) { if(!(x)) {REON_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak();}}
#else
#define REON_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define REON_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#define REON_PROFILING

#ifdef REON_PROFILING
#define PROFILE_SCOPE(name) ProfilerTimer timer##__LINE__(name)
#else
#define PROFILE_SCOPE(name)
#endif

typedef unsigned int uint;