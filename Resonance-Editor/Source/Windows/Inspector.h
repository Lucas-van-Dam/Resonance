#pragma once

#include <string>
#include <functional>
#include "ReflectionSystem.h"
#include "Reon.h"
namespace REON::EDITOR {
	class Inspector
	{
	public:
		static void Initialize();

		// Template to register handlers based on type
		template<typename T>
		static void RegisterHandler(const std::string& typeName, std::function<void(const FieldInfo&, void*)> handler);

		// Function to render a field based on the registered handler
		static void RenderField(const FieldInfo& field, void* instance);

		// Render fields dynamically based on reflection data
		static void InspectObject(std::shared_ptr<GameObject>& object);

	private:
		// Static registry for handlers, maps type names to handler functions
		static std::unordered_map<std::string, std::function<void(const FieldInfo&, void*)>> handlers;
	};
}

