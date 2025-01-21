#pragma once

#include "Reon.h"
#include "assimp/Logger.hpp"
#include "assimp/DefaultLogger.hpp"

namespace REON::EDITOR {
	class AssetProcessorLogStream : public Assimp::LogStream {
	public:
		AssetProcessorLogStream()
		{
			// empty
		}
		// Destructor
		~AssetProcessorLogStream()
		{
			// empty
		}
		// Write womethink using your own functionality
		void write(const char* message)
		{
			REON_CORE_TRACE(message);
		}
	};
}