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
#include <set>  

#include <filesystem>  

#ifdef REON_PLATFORM_WINDOWS 
	#define NOMINMAX
	#include <Windows.h>  
	#include <objbase.h>  
	#include <wrl/client.h>  
#endif

#include "REON/Logger.h"