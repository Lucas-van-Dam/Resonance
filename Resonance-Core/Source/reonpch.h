#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <numbers>
#include <chrono>
#include <any>

#include <stdint.h>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <filesystem>

#include "REON/Logger.h"
#include "REON/Profiler/ProfilerTimer.h"
#include "REON/Profiler/Profiler.h"

#ifdef REON_PLATFORM_WINDOWS
	#include <Windows.h>
#endif