#pragma once

#include <string>
#include <functional>
#include "ReflectionSystem.h"
#include "Reon.h"
#include <AssetManagement/CookPipeline.h>

namespace REON_EDITOR {
	class Inspector
	{
	public:
		static void Initialize();

		// Template to register handlers based on type
		template<typename T>
        static void RegisterHandler(const std::string& typeName,
                                    std::function<void(const REON::FieldInfo&, void*)> handler);

		// Function to render a field based on the registered handler
        static void RenderField(const REON::FieldInfo& field, void* instance);

		// Render fields dynamically based on reflection data
        static void InspectObject(std::shared_ptr<REON::GameObject>& object);

		static void InspectObject(std::filesystem::path assetPath, CookPipeline& cookPipeline);

	private:
		// Static registry for handlers, maps type names to handler functions
      static std::unordered_map<std::string, std::function<void(const REON::FieldInfo&, void*)>> handlers;
      static std::unordered_map<std::string, std::function<void(const std::shared_ptr<REON::GameObject>&)>>
          componentMap;
	};
}

