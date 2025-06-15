#include "DrawFuncs.h"
#include <imgui_internal.h>
#include <Commands/CommandManager.h>
#include <Commands/PropertyChangeCommand.h>

namespace REON::EDITOR {



    bool DrawFuncs::DrawVec3WithUndoRedo(const std::string& label, glm::vec3& values, std::function<void(const glm::vec3&)> setter, std::function<glm::vec3()> getter, float speed, float columnWidth)
    {
        static bool wasActive = false;
        bool isEditing = false;
		bool changed = DrawVec3(label, values, speed, columnWidth, &isEditing);

        if (changed) {
            if (!wasActive && isEditing) {
                CommandManager::startBatch(std::make_unique<PropertyChangeCommand<glm::vec3>>(
                    setter, getter, values));
                wasActive = true;
            }
            else if (isEditing) {
                CommandManager::updateBatch([&](ICommand* cmd) { static_cast<PropertyChangeCommand<glm::vec3>*>(cmd)->UpdateValue(values); });
            }
        }

        if (wasActive && !isEditing) {
			CommandManager::endBatch();
			wasActive = false;
        }

        return changed;
    }

    bool DrawFuncs::DrawVec3(const std::string& label, glm::vec3& values, float speed, float columnWidth, bool* isEditingOut)
	{
        bool changed = false;
        bool anyActive = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
        ImGuiIO& io = ImGui::GetIO();
        ImFont* boldFont = io.Fonts->Fonts[0]; // Optional: switch to bold font if you have one

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 8, 0 });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        // X Button
        ImGui::Text("X");
        ImGui::SameLine();
        if(ImGui::DragFloat("##X", &values.x, 0.1f))
			changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Y Button
        ImGui::Text("Y");
        ImGui::SameLine();
        if(ImGui::DragFloat("##Y", &values.y, 0.1f))
			changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Z Button
        ImGui::Text("Z");
        ImGui::SameLine();
        if (ImGui::DragFloat("##Z", &values.z, 0.1f))
            changed = true;
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
        
        if(isEditingOut)
        {
            *isEditingOut = anyActive;
		}

        return changed;
	}

    bool DrawFuncs::DrawColorEdit(const std::string& label, glm::vec3& values)
    {
        bool edited = false;

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        if (ImGui::ColorEdit3("##ColorEdit", &values.r,
            ImGuiColorEditFlags_NoLabel))// |
            //ImGuiColorEditFlags_NoInputs); // Optional: hide RGB float inputs
        {
			edited = true;
        }

        ImGui::Columns(1);
        ImGui::PopID();
		return edited;
    }

    bool DrawFuncs::DrawFloatWithUndoRedo(const std::string& label, float& value, std::function<void(const glm::vec3&)> setter, std::function<glm::vec3()> getter, float speed)
    {
        return false;
    }



}