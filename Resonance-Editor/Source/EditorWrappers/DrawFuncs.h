#pragma once
#include "imgui.h"
#include <string>
#include <glm/ext/vector_float3.hpp>
#include <functional>

namespace REON::EDITOR {

	class DrawFuncs
	{
	public:
		static bool DrawVec3WithUndoRedo(const std::string& label, glm::vec3& values,
			std::function<void(const glm::vec3&)> setter, std::function<glm::vec3()> getter,
			float speed = 1.0f, float columnWidth = 100.0f);
		static bool DrawVec3(const std::string& label, glm::vec3& values, float speed = 1.0f, float columnWidth = 100.0f, bool* isEditingOut = nullptr);
		static bool DrawColorEdit(const std::string& label, glm::vec3& values);
		static bool DrawFloatWithUndoRedo(const std::string& label, float& value,
			std::function<void(const glm::vec3&)> setter, std::function<glm::vec3()> getter,
			float speed = 1.0f);
	};

}